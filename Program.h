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
#include <random>

class Program
{
public:
	enum CompileError
	{
		CE_NONE,
		CE_MISSING_PAREN,
		CE_MISSING_BRACKET,
		CE_MISSING_BRACE,
		CE_MISSING_COLON_IN_TERNARY,
		CE_UNEXPECTED_CHAR,
		CE_FAILED_TO_PARSE_NUMBER,
		CE_ILLEGAL_ASSIGNMENT,
		CE_ILLEGAL_STATEMENT_TERMINATION, // found a semi-colon where one isn't allowed
		CE_ILLEGAL_VARIABLE_NAME, // found an uppercase letter where we expected a lowercase one
		CE_MISSING_PUT, // the program does not contain any PUT instructions
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
			NOP, // no operation (doesn't pop from the stack or push to it)
			PSH, // push a constant value onto the stack (eg when a numeric value is used)
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
			CEQ, // compare ==
			CNE, // compare !=
			CLT, // compare <
			CLE, // compare <=
			CGT, // compare >
			CGE, // compare >=
			CND, // conditional - ?: and ?
			POP, // ; (pop a value from the stack and do nothing with it)
			GET, // get the the current value of a result. eg [0] or [1].
			PUT, // assign to an output result using [0] = expression.
			RND, // random number operator - operand is used to wrap value returned by rand() - so like Random.Range(0, operand)
			CCV, // use the operand to look up the current value of a midi control change value, eg 'a = C1'
			VCV, // use the operand to look up the current value of a "voltage" control value, eg 'a = V5'
			NOT,
			COM,
			JMP, // JMP to the address indicated by val
		};

		// need default constructor or we can't use vector
		Op() : code(PSH), val(0) {}
		Op(Code _code, Value _val) : code(_code), val(_val) {}

		const Code code;
		// this is mutable because the compiler needs to be able to change it in some cases
		Value val;
	};

	// userMemorySize is used to determine the size of read/write memory used by the program.
	// "user" memory is memory that is accessible only via the @ operator and is otherwise 
	// not modified by the program (but can be externally modified from C++ by calling Peek).
	static Program* Compile(const Char* source, const size_t userMemorySize, CompileError& outError, int& outErrorPosition);
	// get the address in memory of a variable declared in a program with a particular userMemorySize.
	static Value GetAddress(const Char var, size_t userMemorySize);

	// get human-readable descriptions of errors
	static const char * GetErrorString(CompileError error);
	static const char * GetErrorString(RuntimeError error);

	Program(const std::vector<Op>& inOps, const size_t userMemorySize);
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

	static const size_t kCCSize = 128;
	static const size_t kVCSize = 8;

	// the compiled code
	std::vector<Op> ops;
	size_t pc; // program counter, stored here because it can be changed by TRN and JMP
	const size_t userMemSize; // how much of mem is "user" memory
	const size_t memSize; // the actual size of mem
	// the memory space - read/write memory for the program (use Peek/Poke from C++)
	// this includes "user" memory accessible with @, where @0 maps to mem[0]
	// and also includes "variable" memory accessible with lowercase letters like 'a', 'b', 'c', etc.
	// it is also possible to access variable values with @ if you know the address of the variable.
	// for safety, we always wrap the address to the size of the array to prevent invalid access.
	Value* mem;
	// memory for storing MIDI CC values - readonly from within a program
	Value cc[kCCSize];
	// memory for storing VC values = readonly from within a program
	Value vc[kVCSize];
	// the execution stack (reused each time Run is called)
	std::stack<Value> stack;
	// rng because rand() doesn't generate a large enough range
	std::default_random_engine rng;
};

