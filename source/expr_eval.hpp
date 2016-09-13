//
// Original implementation (c) Peter Kankowski, 2007. http://smallcode.weblogs.us mailto:kankowski@narod.ru
// From http://www.strchr.com/expression_evaluator
//
// Modified to evaluate with unsigned ints and to handle more than just basic arithmetic operators
//

#ifndef _expr_eval
#define _expr_eval

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <map>
#include <math.h>

namespace Compartmental {
namespace Vst {

// ================================
//   Simple expression evaluator
// ================================

// Error codes
enum EXPR_EVAL_ERR {
    EEE_NO_ERROR = 0,
    EEE_PARENTHESIS = 1,
    EEE_WRONG_CHAR = 2,
    EEE_DIVIDE_BY_ZERO = 3,
    EEE_UNKNOWN_VAR
};

typedef char EVAL_CHAR;

class ExprEval {
private:
    EXPR_EVAL_ERR _err;
    EVAL_CHAR* _err_pos;
    unsigned int _paren_count;
    
    std::map<char, unsigned int> _vars;
    
    unsigned int Sin(unsigned int v)
    {
        unsigned int r = _vars['r'];
        unsigned int hr = r/2;
        r += 1;
        double s = sin(2 * M_PI * ((float)(v%r)/r));
        return (unsigned int)(s*hr + hr);
    }
    
    unsigned int Freq(unsigned int v)
    {
        double f = round(3 * pow(2.0, (double)v/12.0));
        return (unsigned int)f;
    }
    
    // Parse a number or an expression in parenthesis
    unsigned int ParseAtom(EVAL_CHAR*& expr)
    {
        // Skip spaces
        while( isspace( *expr ) )
        {
            expr++;
        }
        
        // Handle the sign before parenthesis (or before number)
        bool negative = false;
        
        if(*expr == '-')
        {
            negative = true;
            expr++;
        }
        
        if(*expr == '+')
        {
            expr++;
        }
        
        bool sine = false;
        
        if(*expr == '$')
        {
            sine = true;
            expr++;
        }
        
        bool freq = false;
        if (*expr == 'f')
        {
            freq = true;
            expr++;
        }
        
        // Check if there is parenthesis
        if(*expr == '(')
        {
            expr++;
            _paren_count++;
            unsigned int res = ParseOR(expr);
            if(*expr != ')')
            {
                // Unmatched opening parenthesis
                _err = EEE_PARENTHESIS;
                _err_pos = expr;
                return 0;
            }
            expr++;
            _paren_count--;
            unsigned int v = negative ? -res : res;
            if ( sine )
            {
                v = Sin(v);
            }
            if ( freq )
            {
                v = Freq(v);
            }
            
            return v;
        }
        
        // It should be a number; convert it to unsigned int
        char* end_ptr;
        unsigned int res = 0;
        char c = *expr;
        if ( isalpha(c) )
        {
            auto iter = _vars.find(c);
            if ( iter != _vars.end() )
            {
                res = iter->second;
                end_ptr = expr+1;
            }
            else // error, but a non-fatal one
            {
                _err = EEE_UNKNOWN_VAR;
                _err_pos = expr;
                end_ptr = expr+1;
            }
        }
        else
        {
            res = (unsigned int)strtoul(expr, &end_ptr, 10);
        }
        
        if(end_ptr == expr)
        {
            // Report error
            _err = EEE_WRONG_CHAR;
            _err_pos = expr;
            return 0;
        }
        // Advance the pointer and return the result
        expr = end_ptr;
        unsigned int v = negative ? -res : res;
        if ( sine )
        {
            v = Sin(v);
        }
        if ( freq )
        {
            v = Freq(v);
        }
        
        return v;
    }
    
    // Parse multiplication and division
    unsigned int ParseFactors(EVAL_CHAR*& expr)
    {
        unsigned int num1 = ParseAtom(expr);
        for(;;)
        {
            // Skip spaces
            while( isspace(*expr) )
            {
                expr++;
            }
            
            // Save the operation and position
            EVAL_CHAR op = *expr;
            EVAL_CHAR* pos = expr;
            if(op != '/' && op != '*' && op != '%' )
            {
                return num1;
            }
            expr++;
            unsigned int num2 = ParseAtom(expr);
            // Perform the saved operation
            if(op == '/')
            {
                // Handle division by zero
                if(num2 == 0)
                {
                    _err = EEE_DIVIDE_BY_ZERO;
                    _err_pos = pos;
                    return 0;
                }
                num1 /= num2;
            }
            else if ( op == '%' )
            {
                if ( num2 == 0 )
                {
                    _err = EEE_DIVIDE_BY_ZERO;
                    _err_pos = pos;
                    return 0;
                }
                num1 = num1 % num2;
            }
            else
            {
                num1 *= num2;
            }
        }
    }
    
    // Parse addition and subtraction
    unsigned int ParseSummands(EVAL_CHAR*& expr)
    {
        unsigned int num1 = ParseFactors(expr);
        for(;;)
        {
            // Skip spaces
            while( isspace(*expr) )
            {
                expr++;
            }
            EVAL_CHAR op = *expr;
            if(op != '-' && op != '+')
            {
                return num1;
            }
            expr++;
            unsigned int num2 = ParseFactors(expr);
            switch( op )
            {
                case '-': num1 -= num2; break;
                case '+': num1 += num2; break;
            }
        }
    }
    
    // Parse bitshifting << and >>
    unsigned int ParseBitshift(EVAL_CHAR*& expr)
    {
        unsigned int num1 = ParseSummands(expr);
        for(;;)
        {
            while( isspace(*expr) )
            {
                expr++;
            }
            EVAL_CHAR op = *expr;
            if ( op != '<' && op != '>' )
            {
                return num1;
            }
            expr++;
            EVAL_CHAR op2 = *expr;
            if ( op2 != op )
            {
                _err = EEE_WRONG_CHAR;
                _err_pos = expr;
                return 0;
            }
            expr++;
            unsigned int num2 = ParseSummands(expr);
            switch( op )
            {
                case '<': num1 <<= num2; break;
                case '>': num1 >>= num2; break;
            }
        }
    }
    
    unsigned int ParseAND(EVAL_CHAR*& expr)
    {
        unsigned int num1 = ParseBitshift(expr);
        for(;;)
        {
            while (isspace(*expr) )
            {
                expr++;
            }
            EVAL_CHAR op = *expr;
            if ( op != '&' )
            {
                return num1;
            }
            expr++;
            unsigned int num2 = ParseBitshift(expr);
            num1 &= num2;
        }
    }
    
    unsigned int ParseXOR(EVAL_CHAR*& expr)
    {
        unsigned int num1 = ParseAND(expr);
        for(;;)
        {
            while (isspace(*expr) )
            {
                expr++;
            }
            EVAL_CHAR op = *expr;
            if ( op != '^' )
            {
                return num1;
            }
            expr++;
            unsigned int num2 = ParseAND(expr);
            num1 ^= num2;
        }
    }
    
    unsigned int ParseOR(EVAL_CHAR*& expr)
    {
        unsigned int num1 = ParseXOR(expr);
        for(;;)
        {
            while (isspace(*expr) )
            {
                expr++;
            }
            EVAL_CHAR op = *expr;
            if ( op != '|' )
            {
                return num1;
            }
            expr++;
            unsigned int num2 = ParseXOR(expr);
            num1 |= num2;
        }
    }
    
public:
    void SetVar( char var, unsigned int val )
    {
        _vars[var] = val;
    }
    
    unsigned int GetVar( char var )
    {
        return _vars[var];
    }
    
    unsigned int Eval(EVAL_CHAR* expr)
    {
        _paren_count = 0;
        _err = EEE_NO_ERROR;
        unsigned int res = ParseOR(expr);
        // Now, expr should point to '\0', and _paren_count should be zero
        if(_paren_count != 0 || *expr == ')')
        {
            _err = EEE_PARENTHESIS;
            _err_pos = expr;
            return 0;
        }
        if(*expr != '\0')
        {
            _err = EEE_WRONG_CHAR;
            _err_pos = expr;
            return 0;
        }
        return res;
    };
    
    EXPR_EVAL_ERR GetErr()
    {
        return _err;
    }
    
    EVAL_CHAR* GetErrPos()
    {
        return _err_pos;
    }
};

// =======
//  Tests
// =======

#ifdef _DEBUG
void TestExprEval()
{
    ExprEval eval;
    // Some simple expressions
    assert(eval.Eval("1234") == 1234 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("1+2*3") == 7 && eval.GetErr() == EEE_NO_ERROR);
    
    // Parenthesis
    assert(eval.Eval("5*(4+4+1)") == 45 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("5*(2*(1+3)+1)") == 45 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("5*((1+3)*2+1)") == 45 && eval.GetErr() == EEE_NO_ERROR);
    
    // Spaces
    assert(eval.Eval("5 * ((1 + 3) * 2 + 1)") == 45 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("5 - 2 * ( 3 )") == -1 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("5 - 2 * ( ( 4 )  - 1 )") == -1 && eval.GetErr() == EEE_NO_ERROR);
    
    // Sign before parenthesis
    assert(eval.Eval("-(2+1)*4") == -12 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("-4*(2+1)") == -12 && eval.GetErr() == EEE_NO_ERROR);
    
    // Fractional numbers
    assert(eval.Eval("1.5/5") == 0.3 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("1/5e10") == 2e-11 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("(4-3)/(4*4)") == 0.0625 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("1/2/2") == 0.25 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("0.25 * .5 * 0.5") == 0.0625 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval(".25 / 2 * .5") == 0.0625 && eval.GetErr() == EEE_NO_ERROR);
    
    // Repeated operators
    assert(eval.Eval("1+-2") == -1 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("--2") == 2 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("2---2") == 0 && eval.GetErr() == EEE_NO_ERROR);
    assert(eval.Eval("2-+-2") == 4 && eval.GetErr() == EEE_NO_ERROR);
    
    // === Errors ===
    // Parenthesis error
    eval.Eval("5*((1+3)*2+1");
    assert(eval.GetErr() == EEE_PARENTHESIS && strcmp(eval.GetErrPos(), "") == 0);
    eval.Eval("5*((1+3)*2)+1)");
    assert(eval.GetErr() == EEE_PARENTHESIS && strcmp(eval.GetErrPos(), ")") == 0);
    
    // Repeated operators (wrong)
    eval.Eval("5*/2");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "/2") == 0);
    
    // Wrong position of an operator
    eval.Eval("*2");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "*2") == 0);
    eval.Eval("2+");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "") == 0);
    eval.Eval("2*");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "") == 0);
    
    // Division by zero
    eval.Eval("2/0");
    assert(eval.GetErr() == EEE_DIVIDE_BY_ZERO && strcmp(eval.GetErrPos(), "/0") == 0);
    eval.Eval("3+1/(5-5)+4");
    assert(eval.GetErr() == EEE_DIVIDE_BY_ZERO && strcmp(eval.GetErrPos(), "/(5-5)+4") == 0);
    eval.Eval("2/"); // Erroneously detected as division by zero, but that's ok for us
    assert(eval.GetErr() == EEE_DIVIDE_BY_ZERO && strcmp(eval.GetErrPos(), "/") == 0);
    
    // Invalid characters
    eval.Eval("~5");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "~5") == 0);
    eval.Eval("5x");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "x") == 0);
    
    // Multiply errors
    eval.Eval("3+1/0+4$"); // Only one error will be detected (in this case, the last one)
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "$") == 0);
    eval.Eval("3+1/0+4");
    assert(eval.GetErr() == EEE_DIVIDE_BY_ZERO && strcmp(eval.GetErrPos(), "/0+4") == 0);
    eval.Eval("q+1/0)"); // ...or the first one
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "q+1/0)") == 0);
    eval.Eval("+1/0)");
    assert(eval.GetErr() == EEE_PARENTHESIS && strcmp(eval.GetErrPos(), ")") == 0);
    eval.Eval("+1/0");
    assert(eval.GetErr() == EEE_DIVIDE_BY_ZERO && strcmp(eval.GetErrPos(), "/0") == 0);
    
    // An emtpy string
    eval.Eval("");
    assert(eval.GetErr() == EEE_WRONG_CHAR && strcmp(eval.GetErrPos(), "") == 0);
}
#endif
    
} // Vst
} // Compartmental

#endif // _expr_eval
