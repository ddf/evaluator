//
//  expression.cpp
//  evaluator
//
//  Created by Damien Di Fede on 11/25/16.
//
//

#include "expression.hpp"
#include <stdlib.h>
#include <math.h>
#include <stack>

namespace Compartmental
{
    namespace Vst
    {
        EvalValue Expression::Eval()
        {
            EvalValue res = 0;
            if ( !_ops.empty() )
            {
                std::stack<EvalValue> stack;
                for( auto op : _ops )
                {
                    switch( op.operands() )
                    {
                        case 0:
                        {
                            stack.push( op.eval() );
                        }
                        break;
                            
                        case 1:
                        {
                            EvalValue a = stack.top();
                            stack.pop();
                            stack.push( op.eval(a) );
                        }
                        break;
                            
                        case 2:
                        {
                            EvalValue b = stack.top();
                            stack.pop();
                            EvalValue a = stack.top();
                            stack.pop();
                            stack.push( op.eval(a,b, _err) );
                        }
                        break;
                    }
                }
                res = stack.top();
            }
            return res;
        }
        
        EXPR_EVAL_ERR Expression::Compile(EVAL_CHAR *expr)
        {
            _paren_count = 0;
            _err = EEE_NO_ERROR;
            
            _ops.clear();
            
            ParseOR(expr);
            
            if ( _err == EEE_NO_ERROR )
            {
                // Now, expr should point to '\0', and _paren_count should be zero
                if(_paren_count != 0 || *expr == ')')
                {
                    _err = EEE_PARENTHESIS;
                    _err_pos = expr;
                }
                else if(*expr != '\0')
                {
                    _err = EEE_WRONG_CHAR;
                    _err_pos = expr;
                }
            }
            return _err;
        }
        
        int Expression::ParseOR(EVAL_CHAR*& expr)
        {
            if ( ParseXOR(expr) ) return 1;
            for(;;)
            {
                while (isspace(*expr) )
                {
                    expr++;
                }
                EVAL_CHAR op = *expr;
                if ( op != '|' )
                {
                    return 0;
                }
                expr++;
                if ( ParseXOR(expr) ) return 1;
                _ops.push_back( Op(OR) );
            }
        }
        
        int Expression::ParseXOR(EVAL_CHAR*& expr)
        {
            if ( ParseAND(expr) ) return 1;
            for(;;)
            {
                while (isspace(*expr) )
                {
                    expr++;
                }
                EVAL_CHAR op = *expr;
                if ( op != '^' )
                {
                    return 0;
                }
                expr++;
                if ( ParseAND(expr) ) return 1;
                _ops.push_back( Op(XOR) );
            }
        }
        
        int Expression::ParseAND(EVAL_CHAR*& expr)
        {
            if ( ParseBitshift(expr) ) return 1;
            for(;;)
            {
                while (isspace(*expr) )
                {
                    expr++;
                }
                EVAL_CHAR op = *expr;
                if ( op != '&' )
                {
                    return 0;
                }
                expr++;
                if ( ParseBitshift(expr) ) return 1;
                _ops.push_back( Op(AND) );
            }
        }
        
        int Expression::ParseBitshift(EVAL_CHAR *&expr)
        {
            if ( ParseSummands(expr) ) return 1;
            for(;;)
            {
                while( isspace(*expr) )
                {
                    expr++;
                }
                EVAL_CHAR op = *expr;
                if ( op != '<' && op != '>' )
                {
                    return 0;
                }
                expr++;
                EVAL_CHAR op2 = *expr;
                if ( op2 != op )
                {
                    _err = EEE_WRONG_CHAR;
                    _err_pos = expr;
                    return 1;
                }
                expr++;
                if ( ParseSummands(expr) ) return 1;
                switch( op )
                {
                    case '<': _ops.push_back( Op(BSL) ); break;
                    case '>': _ops.push_back( Op(BSR) ); break;
                }
            }
        }
        
        int Expression::ParseSummands(EVAL_CHAR *&expr)
        {
            if ( ParseFactors(expr) ) return 1;
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
                    return 0;
                }
                expr++;
                if ( ParseFactors(expr) ) return 1;
                switch( op )
                {
                    case '-': _ops.push_back( Op(SUB) ); break;
                    case '+': _ops.push_back( Op(ADD) ); break;
                }
            }
        }
        
        int Expression::ParseFactors(EVAL_CHAR *&expr)
        {
            if ( ParseAtom(expr) ) return 1;
            for(;;)
            {
                // Skip spaces
                while( isspace(*expr) )
                {
                    expr++;
                }
                
                // Save the operation and position
                EVAL_CHAR op = *expr;
                // was needed to report the dived by zero error position properly, but that happens at eval time now
                //EVAL_CHAR* pos = expr;
                if(op != '/' && op != '*' && op != '%' )
                {
                    return 0;
                }
                expr++;
                if ( ParseAtom(expr) ) return 1;
                // Perform the saved operation
                if(op == '/')
                {
                    _ops.push_back( Op(DIV) );
                }
                else if ( op == '%' )
                {
                    _ops.push_back( Op(MOD) );
                }
                else
                {
                    _ops.push_back( Op(MUL) );
                }
            }
        }
        
        int Expression::ParseAtom(EVAL_CHAR *&expr)
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
            
            bool square = false;
            
            if ( *expr == '#' )
            {
                square = true;
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
                if ( ParseOR(expr) ) return 1;
                if(*expr != ')')
                {
                    // Unmatched opening parenthesis
                    _err = EEE_PARENTHESIS;
                    _err_pos = expr;
                    return 1;
                }
                expr++;
                _paren_count--;
                if ( negative )
                {
                    _ops.push_back( Op(NEG) );
                }
                if ( sine )
                {
                    _ops.push_back( Op(SIN, this) );
                }
                if ( square )
                {
                    _ops.push_back( Op(SQR, this) );
                }
                if ( freq )
                {
                    _ops.push_back( Op(FREQ) );
                }
                
                return 0;
            }
            
            // It should be a number; convert it to unsigned int
            EVAL_CHAR* end_ptr;
            EVAL_CHAR c = *expr;
            if ( isalpha(c) )
            {
                auto iter = _vars.find(c);
                // error, but a non-fatal on, so we don't return
                if ( iter == _vars.end() )
                {
                    _err = EEE_UNKNOWN_VAR;
                    _err_pos = expr;
                }
                
                _ops.push_back( Op(this, c) );
                
                end_ptr = expr+1;
            }
            else
            {
                EvalValue res = (EvalValue)strtoull(expr, &end_ptr, 10);
                _ops.push_back( Op(res) );
            }
            
            if(end_ptr == expr)
            {
                // Report error
                _err = EEE_WRONG_CHAR;
                _err_pos = expr;
                return 1;
            }
            // Advance the pointer and return the result
            expr = end_ptr;
            if ( negative )
            {
                _ops.push_back( Op(NEG) );
            }
            if ( sine )
            {
                _ops.push_back( Op(SIN, this) );
            }
            if ( square )
            {
                _ops.push_back( Op(SQR, this) );
            }
            if ( freq )
            {
                _ops.push_back( Op(FREQ) );
            }
            
            return 0;
        }
        
        int Expression::Op::operands() const
        {
            switch(code)
            {
                case NUM:
                case VAR:
                    return 0;
                    
                case NEG:
                case SIN:
                case SQR:
                case FREQ:
                    return 1;
                    
                default:
                    return 2;
            }
        }
        
        EvalValue Expression::Op::eval() const
        {
            switch(code)
            {
                case NUM: return val;
                case VAR: return expr->GetVar(var);
                    
                default: return 0;
            }
        }
        
        EvalValue Expression::Op::eval(EvalValue a) const
        {
            switch(code)
            {
                case NEG: return -a;
                case SIN:
                {
                    EvalValue r = expr->GetVar('r');
                    EvalValue hr = r/2;
                    r += 1;
                    double s = sin(2 * M_PI * ((double)(a%r)/r));
                    return EvalValue(s*hr + hr);
                }
                    
                case SQR:
                {
                    EvalValue r = expr->GetVar('r');
                    return a%r < r/2 ? 0 : r-1;
                }
                    
                case FREQ:
                {
                    double f = round(3 * pow(2.0, (double)a/12.0));
                    return (EvalValue)f;
                }
                    
                default: return 0;
            }
        }
        
        EvalValue Expression::Op::eval(EvalValue a, EvalValue b, EXPR_EVAL_ERR& err) const
        {
            switch(code)
            {
                case MUL: return a*b;
                case DIV: return b==0 ? err=EEE_DIVIDE_BY_ZERO : a/b;
                case MOD: return b==0 ? err=EEE_DIVIDE_BY_ZERO : a%b;
                case ADD: return a+b;
                case SUB: return a-b;
                case BSL: return a<<b;
                case BSR: return a>>b;
                case AND: return a&b;
                case OR: return a|b;
                case XOR: return a^b;
                    
                default: return 0;
            }
        }
    }
}