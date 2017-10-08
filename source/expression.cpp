//
//  expression.cpp
//  evaluator
//
//  Created by Damien Di Fede on 11/25/16.
//
//

//#define STDSTACK

// required to get M_PI on windows
#define _USE_MATH_DEFINES

#include "expression.hpp"
#include <stdlib.h>
#include <math.h>
#include <stack>
#include <ctype.h>


namespace Compartmental
{
    namespace Vst
    {
        EvalValue Expression::Eval()
        {
            EvalValue res = 0;
            const auto count = GetInstructionCount();
            if ( count > 0 )
            {
#ifdef STDSTACK
                std::stack<EvalValue> stack;
#else
                EvalValue stack[128];
                int top = -1;
#endif
                for(unsigned long i = 0; i < count; ++i)
                {
                    auto& op = _ops[i];
                    switch( op.operands() )
                    {
                        case 0:
                        {
#ifdef STDSTACK
                            stack.push( op.eval() );
#else
                            stack[++top] = op.eval();
#endif
                        }
                        break;
                            
                        case 1:
                        {
#ifdef STDSTACK
                            EvalValue a = stack.top();
                            stack.pop();
                            stack.push( op.eval(a) );
#else
                            EvalValue a = stack[top--];
                            stack[++top] = op.eval(a);
#endif
                        }
                        break;
                            
                        case 2:
                        {
#ifdef STDSTACK
                            EvalValue b = stack.top();
                            stack.pop();
                            EvalValue a = stack.top();
                            stack.pop();
                            stack.push( op.eval(a,b, _err) );
#else
                            EvalValue b = stack[top--];
                            EvalValue a = stack[top--];
                            stack[++top] = op.eval(a,b,_err);
#endif
                        }
                        break;
                    }
                }
#ifdef STDSTACK
                res = stack.top();
#else
                res = stack[top];
#endif
            }
            return res;
        }
        
        EXPR_EVAL_ERR Expression::Compile(EVAL_CHAR *expr)
        {
            _paren_count = 0;
            _err = EEE_NO_ERROR;
            
            clear();
            
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
                add( Op(OR) );
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
                add( Op(XOR) );
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
                add( Op(AND) );
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
                    case '<': add( Op(BSL) ); break;
                    case '>': add( Op(BSR) ); break;
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
                    case '-': add( Op(SUB) ); break;
                    case '+': add( Op(ADD) ); break;
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
                    add( Op(DIV) );
                }
                else if ( op == '%' )
                {
                    add( Op(MOD) );
                }
                else
                {
                    add( Op(MUL) );
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
            
            std::stack<Op> unaryOps;
            
            EVAL_CHAR op = *expr;
            while( op == '-' || op == '+' || op == '$' || op == '#' || op == 'F' || op == 'T' )
            {
                switch(op)
                {
                    case '-': unaryOps.push( Op(NEG) ); break;
                    case '+': break; // no op
                    case '$': unaryOps.push( Op(SIN,this) ); break;
                    case '#': unaryOps.push( Op(SQR, this) ); break;
                    case 'F': unaryOps.push( Op(FREQ, this) ); break;
                    case 'T': unaryOps.push( Op(TRI, this) ); break;
                }
                expr++;
                op = *expr;
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
                
                while( !unaryOps.empty() )
                {
                    add(unaryOps.top());
                    unaryOps.pop();
                }
                
                return 0;
            }
            
            // It should be a number; convert it to unsigned int
            EVAL_CHAR* end_ptr;
            EVAL_CHAR c = *expr;
            if ( isalpha(c) )
            {
//                auto iter = _vars.find(c);
//                // error, but a non-fatal on, so we don't return
//                if ( iter == _vars.end() )
//                {
//                    _err = EEE_UNKNOWN_VAR;
//                    _err_pos = expr;
//                }
                
                add( Op(this, c) );
                
                end_ptr = expr+1;
            }
            else
            {
                EvalValue res = (EvalValue)strtoull(expr, &end_ptr, 10);
                add( Op(res) );
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
            
            while( !unaryOps.empty() )
            {
                add(unaryOps.top());
                unaryOps.pop();
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
                case TRI:
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
                    double f = round(3 * pow(2.0, (double)a/12.0) * (44100.0 / expr->_sample_rate));
                    return (EvalValue)f;
                }
                    
                case TRI:
                {
                    EvalValue r = expr->GetVar('r');
                    a *= 2;
                    return a*((a/r)%2) + (r-a-1)*(1 - (a/r)%2);
                }
                    
                default: return 0;
            }
        }
        
        EvalValue Expression::Op::eval(EvalValue a, EvalValue b, EXPR_EVAL_ERR& err) const
        {
            EvalValue v = 0;
            
            switch(code)
            {
                case MUL: v = a*b; break;
                case DIV: if ( b ) { v = a/b; } else { err = EEE_DIVIDE_BY_ZERO; } break;
                case MOD: if ( b ) { v = a%b; } else { err = EEE_DIVIDE_BY_ZERO; } break;
                case ADD: v = a+b; break;
                case SUB: v = a-b; break;
                case BSL: v = a<<b; break;
                case BSR: v = a>>(b%64); break;
                case AND: v = a&b; break;
                case OR: v = a|b; break;
                case XOR: v = a^b;
                    
                default: break;
            }
            return v;
        }
    }
}
