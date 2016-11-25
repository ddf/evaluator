//
//  main.cpp
//  expression_test
//
//  Created by Damien Di Fede on 11/25/16.
//
//

#include <iostream>
#include "expression.hpp"

using namespace Compartmental::Vst;

const char* expressions[] = {
//    "1234",
//    "1+2",
//    "2-1",
//    "2*2",
//    "2/2",
//    "1+2*3",
//    "5*(2*$(1+3+1)))",
//    "fn",
    "--2",
    "2--2",
    "2+-2",
    "2-+-2",
    "5*/2"
};

const int exprCount = sizeof(expressions) / sizeof(const char*);

int main(int argc, const char * argv[])
{
    Expression expr;
    expr.SetVar('r', 1<<15);
    expr.SetVar('n', 60);
    EVAL_CHAR str[128];
    for(int i = 0; i < exprCount; ++i)
    {
        strcpy(str, expressions[i]);
        EXPR_EVAL_ERR err = expr.Compile(str);
        if ( err == EEE_NO_ERROR )
        {
            EvalValue result = expr.Eval();
            err = expr.GetErr();
            if ( err == EEE_NO_ERROR )
            {
                std::cout << str << " = " << result << '\n';
            }
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
        }
    }
    return 0;
}
