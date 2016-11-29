//
//  main.cpp
//  expression_test
//
//  Created by Damien Di Fede on 11/25/16.
//
//

#include <iostream>
#include <iomanip>
#include <math.h>
#include <cassert>
#include "expression.hpp"

using namespace Compartmental::Vst;

static const EvalValue r = 1<<15;
static const EvalValue n = 64;
static const EvalValue t = 112344324;
static const EvalValue m = t / (EvalValue)(44100/1000);
static const EvalValue p = 234125150;

static EvalValue $(EvalValue a)
{
    EvalValue hr = r/2;
    EvalValue r1 = r+1;
    double s = sin(2 * M_PI * ((double)(a%r1)/r1));
    return EvalValue(s*hr + hr);
}

static EvalValue s(EvalValue a)
{
    return a%r < r/2 ? 0 : r-1;
}

static EvalValue f(EvalValue a)
{
    double f = round(3 * pow(2.0, (double)a/12.0));
    return (EvalValue)f;
}

static EvalValue sr(EvalValue a, EvalValue b)
{
    return a>>b;
}

static EvalValue sl(EvalValue a, EvalValue b)
{
    return a<<b;
}

struct test
{
    const char * expr;
    const EvalValue result;
    const EXPR_EVAL_ERR error;
};

test expressions[] = {
    { "1234", 1234, EEE_NO_ERROR },
    { "1+2", 1+2, EEE_NO_ERROR },
    { "2-1", 2-1, EEE_NO_ERROR },
    { "2*2", 2*2, EEE_NO_ERROR },
    { "2/2", 2/2, EEE_NO_ERROR },
    { "1+2*3", 1+2*3, EEE_NO_ERROR },
    { "5*(2*$(1+3+1)))", 0, EEE_PARENTHESIS },
    { "fn", f(n), EEE_NO_ERROR },
    { "--2", 2, EEE_NO_ERROR },
    { "2--2", 4, EEE_NO_ERROR },
    { "2+-2", 0, EEE_NO_ERROR },
    { "2-+-2", 4, EEE_NO_ERROR },
    { "5*/2", 0, EEE_WRONG_CHAR },
    { "#$2", s($(2)), EEE_NO_ERROR },
    { "$#2", $(s(2)), EEE_NO_ERROR },
    { "$(#2)", $((s(2))), EEE_NO_ERROR },
    { "$fn", $(f(n)), EEE_NO_ERROR },
    { "$(fn)", $(f(n)), EEE_NO_ERROR },
    { "(t*fn)*((t*fn/r)%2) + (r-t*fn-1)*(1 - (t*fn/r)%2)", (t*f(n))*((t*f(n)/r)%2) + (r-t*f(n)-1)*(1 - (t*f(n)/r)%2), EEE_NO_ERROR },
    { "(t*128 + $(t)) | t>>(t%(8*r))/r | t>>128", (t*128 + $(t)) | t>>(t%(8*r))/r | sr(t,128), EEE_NO_ERROR },
    { "(t*64 + $(t^$(m/2000))*$(m/2000)) | t*32", (t*64 + $(t^$(m/2000))*$(m/2000)) | t*32, EEE_NO_ERROR },
    { "t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128, EEE_NO_ERROR },
    { "$(t*f(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))", $(t*f(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7))), EEE_NO_ERROR },
    { "(t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32", (sl(t,t/(1024*8)) | sr(t,t/16) & sr(t,t/32)) / (t%(t/512+1) + 1) * 32, EEE_NO_ERROR },
    { "(r/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)", (r/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16), EEE_NO_ERROR },
    { "(1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", (1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4, EEE_NO_ERROR },
    { "$(t*fn) | t*n/10>>4 ^ p>>(m/250%12)", $(t*f(n)) | t*n/10>>4 ^ p>>(m/250%12), EEE_NO_ERROR },
    { "(t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", (t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12, EEE_NO_ERROR },
};

const int exprCount = sizeof(expressions) / sizeof(test);

int main(int argc, const char * argv[])
{
    Expression expr;
    expr.SetVar('r', r);
    expr.SetVar('n', n);
    expr.SetVar('t', t);
    expr.SetVar('m', m);
    expr.SetVar('p', p);
    EVAL_CHAR str[128];
    for(int i = 0; i < exprCount; ++i)
    {
        strcpy(str, expressions[i].expr);
        EXPR_EVAL_ERR err = expr.Compile(str);
        if ( err == EEE_NO_ERROR )
        {
            EvalValue result = expr.Eval();
            err = expr.GetErr();
            if ( err == EEE_NO_ERROR )
            {
                std::cout << str << " = " << result << '\n';
            }
            if ( result != expressions[i].result )
            {
                std::cout << "expressions[i].result == " << expressions[i].result << '\n';
            }
            assert( result == expressions[i].result );
            assert( err == expressions[i].error );
        }
        
        if ( err != EEE_NO_ERROR )
        {
            std::cout << str << " = ERROR: " << Expression::ErrorStr(err) << '\n';
            auto off = expr.GetErrPos() - str;
            for(int i = 0; i < off; ++i)
            {
                std::cout << ' ';
            }
            std::cout << "^\n";
            assert( err == expressions[i].error );
        }
    }
    return 0;
}
