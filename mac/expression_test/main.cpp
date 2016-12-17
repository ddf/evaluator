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

// Timer from http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
class Timer
{
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const {
        return std::chrono::duration_cast<second_>
        (clock_::now() - beg_).count(); }
    
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};

static EvalValue r = 1<<15;
static EvalValue n = 64;
static EvalValue t = 112344324;
static EvalValue m = t / (EvalValue)(44100/1000);
static EvalValue p = 234125150;

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

// ddf (12/5/16)
// if b is greater than the width of the int being shifted
// and is present in the lamba as a numeric constant
// the optimizer will recognize this fact and optimize out the operation.
// however, when shift right runs in the Expression code,
// it is operating on variables that the optimizer does not know the value of,
// so the operation will actually execute and behavior is that b is wrapped to the width of type being shifted.
static EvalValue sr(EvalValue a, EvalValue b)
{
    return a>>(b%64);
}

struct Test
{
    const char * expr;
    EvalValue (*eval)(void);
    const EXPR_EVAL_ERR error;
};

#define EVAL(x) []()->EvalValue{ return x; }

Test tests[] = {
    { "1234", EVAL(1234) , EEE_NO_ERROR },
    { "1+2", EVAL(1+2), EEE_NO_ERROR },
    { "2-1", EVAL(2-1), EEE_NO_ERROR },
    { "2*2", EVAL(2*2), EEE_NO_ERROR },
    { "2/2", EVAL(2/2), EEE_NO_ERROR },
    { "1+2*3", EVAL(1+2*3), EEE_NO_ERROR },
    { "fn", EVAL(f(n)), EEE_NO_ERROR },
    { "--2", EVAL(2), EEE_NO_ERROR },
    { "2--2", EVAL(4), EEE_NO_ERROR },
    { "2+-2", EVAL(0), EEE_NO_ERROR },
    { "2-+-2", EVAL(4), EEE_NO_ERROR },
    { "#$2", EVAL(s($(2))), EEE_NO_ERROR },
    { "$#2", EVAL($(s(2))), EEE_NO_ERROR },
    { "$(#2)", EVAL($((s(2)))), EEE_NO_ERROR },
    { "$fn", EVAL($(f(n))), EEE_NO_ERROR },
    { "$(fn)", EVAL($(f(n))), EEE_NO_ERROR },
    { "(t*fn)*((t*fn/r)%2) + (r-t*fn-1)*(1 - (t*fn/r)%2)", EVAL((t*f(n))*((t*f(n)/r)%2) + (r-t*f(n)-1)*(1 - (t*f(n)/r)%2)), EEE_NO_ERROR },
    { "(t*128 + $(t)) | t>>(t%(8*r))/r | t>>128", EVAL((t*128 + $(t)) | t>>(t%(8*r))/r | sr(t,128)), EEE_NO_ERROR },
    { "(t*64 + $(t^$(m/2000))*$(m/2000)) | t*32", EVAL((t*64 + $(t^$(m/2000))*$(m/2000)) | t*32), EEE_NO_ERROR },
    { "t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", EVAL(t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128), EEE_NO_ERROR },
    { "$(t*f(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))", EVAL($(t*f(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))), EEE_NO_ERROR },
    { "(t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32", EVAL((t<<t/(1024*8) | sr(t,t/16) & sr(t,t/32)) / (t%(t/512+1) + 1) * 32), EEE_NO_ERROR },
    { "(r/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)", EVAL((r/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)), EEE_NO_ERROR },
    { "(1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", EVAL((1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4), EEE_NO_ERROR },
    { "$(t*fn) | t*n/10>>4 ^ p>>(m/250%12)", EVAL($(t*f(n)) | t*n/10>>4 ^ p>>(m/250%12)), EEE_NO_ERROR },
    { "(t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", EVAL((t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12), EEE_NO_ERROR },
    
    // test syntax errors
    { "5*(2*$(1+3+1)))", EVAL(0), EEE_PARENTHESIS },
    { "5*/2", EVAL(0), EEE_WRONG_CHAR },
};

const int testCount = sizeof(tests) / sizeof(Test);
const int testIterations = 1024*8;

void set(Expression& e, EvalValue _t, EvalValue _p)
{
    t = _t;
    m = t / (EvalValue)(44100/1000);
    e.SetVar('t', t);
    e.SetVar('m', m);
    
    p = _p;
    e.SetVar('p', _p);
}

int main(int argc, const char * argv[])
{
    Timer timer;
    Expression expr;
    expr.SetVar('r', r);
    expr.SetVar('n', n);
    expr.SetVar('t', t);
    expr.SetVar('m', m);
    expr.SetVar('p', p);
    EVAL_CHAR str[128];
    for(int i = 0; i < testCount; ++i)
    {
        Test& test = tests[i];
        strcpy(str, test.expr);
        std::cout << '"' << str << '"';
        switch( test.error )
        {
            case EEE_NO_ERROR:
            {
                EXPR_EVAL_ERR err = expr.Compile(str);
                if ( err != EEE_NO_ERROR )
                {
                    std::cout << " FAILED with error: "  << Expression::ErrorStr(err) << '\n';
                    auto off = expr.GetErrPos() - str;
                    for(int i = 0; i < off; ++i)
                    {
                        std::cout << ' ';
                    }
                    std::cout << "^\n";
                }
                else
                {
                    std::cout << " compiled to " << expr.GetInstructionCount() << " instructions.";
                    set(expr, 0, 0);
                    double elapsed = 0;
                    for(int i = 0; i < testIterations; ++i)
                    {
                        timer.reset();
                        EvalValue result1 = expr.Eval();
                        elapsed += timer.elapsed();
                        EvalValue result2 = test.eval();
                        if ( result1 != result2 )
                        {
                            std::cout << " FAILED! " << result1 << " != " << result2 << '\n';
                        }
                        assert( result1 == result2 );
                        set(expr, t+1, result1);
                    }
                    elapsed /= testIterations;
                    std::cout << " PASSED with " << (elapsed*1000) << " ms avg execution time" << std::endl;
                }
            }
            break;
                
            default:
            {
                EXPR_EVAL_ERR err = expr.Compile(str);
                if ( err != test.error )
                {
                    std::cout << " FAILED! " << Expression::ErrorStr(err) << " != " << Expression::ErrorStr(test.error);
                }
                else
                {
                    std::cout << " PASSED" << std::endl;
                }
                assert( err == test.error);
            }
            break;
        }
    }
    return 0;
}
