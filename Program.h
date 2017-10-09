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
		CE_UNEXPECTED_CHAR,
		CE_UNKNOWN_VAR
	};

	enum RuntimeError 
	{
		RE_NONE,
		RE_DIVIDE_BY_ZERO,
		RE_MISSING_OPERAND, // the stack did not contain data for the operation
		RE_MISSING_OPCODE, // the implementation for an Op::Code is missing
		RE_INCONSISTENT_STACK, // the stack was not empty after popping the final result
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
			VAR,
			FREQ,
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
			XOR
		};

		// need default constructor or we can't use vector
		Op() : code(NUM), val(0) {}
		Op(Code _code) : code(_code) {}
		Op(Value _val) : code(NUM), val(_val) {}
		Op(Char _var) : code(VAR), var(_var) {}

		const Code code;
		const union
		{
			Value val;
			Char  var;
		};
	};

	static const uint32_t kMemorySize = 1024 * 4;

	static Program* Compile(const Char* source, CompileError& outError, int& outErrorPosition);
	static const char * GetErrorString(CompileError error);

	Program(const std::vector<Op>& inOps);
	~Program();

	uint64_t GetInstructionCount() const { return ops.size(); }

	// run the program placing the value it evaluates to into result
	RuntimeError Run(Value& result);
	// get the current value of a var, eg Get('t')
	Value Get(const Char var) const;
	// set the value of a var, eg Set('m', 128)
	void  Set(const Char var, const Value value);

private:

	// get the value at this memory address
	Value Peek(const uint32_t address) const;
	void  Poke(const uint32_t address, Value value);

	RuntimeError Exec(const Op& op);

	// the compiled code
	std::vector<Op> ops;
	// the memory space
	Value mem[kMemorySize];
	// the execution stack (reused each time Run is called)
	std::stack<Value> stack;
};

