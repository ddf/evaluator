//
//  expression.hpp
//  evaluator
//
//  Created by Damien Di Fede on 11/25/16.
//
//

#ifndef expression_hpp
#define expression_hpp

#include <unordered_map>
#include <vector>
#include "ftypes.h"

namespace Compartmental
{
    namespace Vst
    {
        // Error codes
        enum EXPR_EVAL_ERR {
            EEE_NO_ERROR = 0,
            EEE_PARENTHESIS = 1,
            EEE_WRONG_CHAR = 2,
            EEE_DIVIDE_BY_ZERO = 3,
            EEE_UNKNOWN_VAR
        };
        
        typedef char EVAL_CHAR;
        typedef Steinberg::uint64 EvalValue;
        
        class Expression
        {
        public:
            Expression()
            {
                memset(_vars, 0, sizeof(_vars));
            }
            
            void SetVar( EVAL_CHAR var, EvalValue val )
            {
                _vars[(int)var+128] = val;
            }
            
            EvalValue GetVar( EVAL_CHAR var ) const
            {
                return _vars[(int)var+128];
            }
            
            EXPR_EVAL_ERR GetErr() const { return _err; }
            
            EVAL_CHAR* GetErrPos() const
            {
                return _err_pos;
            }
            
            EXPR_EVAL_ERR Compile(EVAL_CHAR* expr);
            unsigned long GetInstructionCount() const { return _ops.size(); }
            
            EvalValue Eval();
            
            static const char * ErrorStr(EXPR_EVAL_ERR err)
            {
                switch( err )
                {
                    case EEE_NO_ERROR: break;
                    case EEE_PARENTHESIS: return "mismatched parens"; break;
                    case EEE_WRONG_CHAR: return "unexpected or missing character"; break;
                    case EEE_DIVIDE_BY_ZERO: return "divide by zero"; break;
                    case EEE_UNKNOWN_VAR: return "unknown var"; break;
                }
                
                return "no error";
            }
            
        private:
            
            int ParseOR(EVAL_CHAR*& expr);
            int ParseXOR(EVAL_CHAR*& expr);
            int ParseAND(EVAL_CHAR*& expr);
            int ParseBitshift(EVAL_CHAR*& expr);
            int ParseSummands(EVAL_CHAR*& expr);
            int ParseFactors(EVAL_CHAR*& expr);
            int ParseAtom(EVAL_CHAR*& expr);
            
            EXPR_EVAL_ERR _err;
            EVAL_CHAR* _err_pos;
            unsigned int _paren_count;
            
            EvalValue _vars[256];
            
            enum OpCode
            {
                NUM,
                VAR,
                FREQ,
                SQR,
                SIN,
                NEG,
                MUL,
                DIV,
                MOD,
                ADD,
                SUB,
                BSL,
                BSR,
                AND,
                OR,
                XOR
            };
            
            class Op
            {
            public:
                Op() : code(NUM), expr(0), val(0) {}
                Op(OpCode _code) : code(_code), expr(0) {}
                Op(OpCode _code, const Expression * const _expr) : code(_code), expr(_expr) {}
                Op(EvalValue _val) : code(NUM), expr(0), val(_val) {}
                Op(const Expression * const _expr, EVAL_CHAR _var) : code(VAR), expr(_expr), var(_var) {}
                
                // how many operands this Op requires
                int operands() const;
                
                // caller should call appropriate method based on return value of operands()
                EvalValue eval() const;
                EvalValue eval(EvalValue a) const;
                EvalValue eval(EvalValue a, EvalValue b, EXPR_EVAL_ERR& err) const;
                
            private:
                OpCode code;
                const Expression * expr;
                union
                {
                    EvalValue val;
                    EVAL_CHAR var;
                };
            };
            
            void add(const Op& op)
            {
                _ops.push_back(op);
            }
            
            void clear()
            {
                _ops.clear();
            }
            
            std::vector<Op> _ops;
        };
    }
}

#endif /* expression_hpp */
