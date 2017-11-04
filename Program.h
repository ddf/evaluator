//
//  Program.h
//  Evaluator
//
//  Created by Damien Quartz on 10/9/17
//
//

#pragma once

#include <stdint.h>
#include <vector>
#include <stack>

class Program
{
public:
	enum CompileError
	{
		CE_NONE,
		CE_MISSING_PAREN,
		CE_MISSING_BRACKET,
		CE_MISSING_COLON_IN_TERNARY,
		CE_UNEXPECTED_CHAR,
		CE_FAILED_TO_PARSE_NUMBER,
		CE_ILLEGAL_ASSIGNMENT,
		CE_ILLEGAL_STATEMENT_TERMINATION, // found a semi-colon where one isn't allowed
		CE_ILLEGAL_VARIABLE_NAME, // found an uppercase letter where we expected a lowercase one
	};

	enum RuntimeError 
	{
		RE_NONE,
		RE_DIVIDE_BY_ZERO,
		RE_MISSING_OPERAND, // the stack did not contain data for the operation
		RE_MISSING_OPCODE, // the implementation for an Op::Code is missing
		RE_INCONSISTENT_STACK, // the stack was not empty after popping the final result
		RE_EMPTY_PROGRAM, // the program has no instructions to execute
		RE_GET_OUT_OF_BOUNDS, // index for GET was bigger than the size of the results array
		RE_PUT_OUT_OF_BOUNDS, // index for PUT was bigger than the size of the results array
	};

	// type of the string expression for Compile
	typedef char	 Char;
	// type of the value returned by evaluation
	typedef uint64_t Value;

	struct Op
	{
	public:

		enum Code
		{
			NUM,
			PEK, // get the value at a memory address
			POK, // set the value at a memory address
			FRQ,
			SQR,
			SIN,
			TRI,
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
			XOR,
            CLT, // compare <
            CGT, // compare >
            TRN, // ternary operator - ?:
            POP, // ;
			GET, // get the the current value of a result. eg [0] or [1].
			PUT, // assign to an output result using [0] = expression.
			RND, // random number operator - operand is used to wrap value returned by rand() - so like Random.Range(0, operand)
			CCV, // use the operand to look up the current value of a midi control change value, eg 'a = C1'
			VCV, // use the operand to look up the current value of a "voltage" control value, eg 'a = V5'
		};

		// need default constructor or we can't use vector
		Op() : code(NUM), val(0) {}
		Op(Code _code, Value _val) : code(_code), val(_val) {}

		const Code code;
		const Value val;
	};

	static const uint32_t kMemorySize = 1024 * 4;
	static const size_t kCCSize = 128;
	static const size_t kVCSize = 16;

	static Program* Compile(const Char* source, CompileError& outError, int& outErrorPosition);
	static const char * GetErrorString(CompileError error);
    static const char * GetErrorString(RuntimeError error);

	Program(const std::vector<Op>& inOps);
	~Program();

	uint64_t GetInstructionCount() const { return ops.size(); }

	// run the program placing the value it evaluates to into the results array.
	// count is provided so that we can prevent the program from overrunning the array.
	RuntimeError Run(Value* results, const size_t size);

	// get the current value of a var, eg Get('t')
	Value Get(const Char var) const;
	// set the value of a var, eg Set('m', 128)
	void  Set(const Char var, const Value value);

	// get the value at this memory address
	Value Peek(const Value address) const;
	// set the value at this memory address
	void  Poke(const Value address, const Value value);

	// get/set a control change. these are accessible in the program with the 'C' operator
	Value GetCC(const Value idx) const;
	void  SetCC(const Value idx, const Value value);

	// get/set a "voltage" change. these are accessible in the program with the 'V' operator
	Value GetVC(const Value idx) const;
	void  SetVC(const Value idx, const Value value);

private:

	RuntimeError Exec(const Op& op, Value* results, size_t size);

	// the compiled code
	std::vector<Op> ops;
	// the memory space
	Value mem[kMemorySize];
	// memory for storing MIDI CC values
	Value cc[kCCSize];
	// memory for storing VC values
	Value vc[kVCSize];
	// the execution stack (reused each time Run is called)
	std::stack<Value> stack;
};

