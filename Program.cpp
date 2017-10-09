//
//  Program.cpp
//  Evaluator
//
//  Created by Damien Quartz on 10/9/17
//
//

// required to get M_PI on windows
#define _USE_MATH_DEFINES

#include "Program.h"
#include <ctype.h>
#include <deque>
#include <math.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////
// COMPILATION
//////////////////////////////////////////////////////////////////////////
#pragma region Compilation

// used during compilation to keep track of things
struct CompilationState
{
	const Program::Char* const source;
	int parsePos;
	int parenCount;
	Program::CompileError error;
	std::vector<Program::Op> ops;

	CompilationState(const Program::Char* inSource)
		: source(inSource)
		, parsePos(0)
		, parenCount(0)
		, error(Program::CE_NONE)
	{

	}

	// some helpers
	Program::Char operator*() const { return source[parsePos]; }
	void Push(const Program::Op::Code code) { ops.push_back(Program::Op(code)); }
	void Push(const Program::Value value) { ops.push_back(Program::Op(value)); }
	void Push(const Program::Char var) { ops.push_back(Program::Op(var)); }
};

int ParseOR(CompilationState& state);

static int ParseAtom(CompilationState& state)
{
	// Skip spaces
	while (isspace(*state))
	{
		state.parsePos++;
	}

	std::stack<Program::Op::Code> unaryOps;

	Program::Char op = *state;
	while (op == '-' || op == '+' || op == '$' || op == '#' || op == 'F' || op == 'T')
	{
		switch (op)
		{
		case '-': unaryOps.push(Program::Op::NEG); break;
		case '+': break; // no op
		case '$': unaryOps.push(Program::Op::SIN); break;
		case '#': unaryOps.push(Program::Op::SQR); break;
		case 'F': unaryOps.push(Program::Op::FREQ); break;
		case 'T': unaryOps.push(Program::Op::TRI); break;
		}
		state.parsePos++;
		op = *state;
	}

	// Check if there is parenthesis
	if (*state == '(')
	{
		state.parsePos++;
		state.parenCount++;
		if (ParseOR(state)) return 1;
		if (*state != ')')
		{
			// Unmatched opening parenthesis
			state.error = Program::CE_MISSING_PAREN;
			return 1;
		}
		state.parsePos++;
		state.parenCount--;

		while (!unaryOps.empty())
		{
			state.Push(unaryOps.top());
			unaryOps.pop();
		}

		return 0;
	}

	if (isalpha(*state))
	{
		//                auto iter = _vars.find(c);
		//                // error, but a non-fatal on, so we don't return
		//                if ( iter == _vars.end() )
		//                {
		//                    _err = EEE_UNKNOWN_VAR;
		//                    _err_pos = expr;
		//                }
		state.Push(*state);
		state.parsePos++;
	}
	else // parse a numeric value
	{
		const Program::Char* startPtr = state.source + state.parsePos;
		Program::Char* endPtr = nullptr;
		Program::Value res = (Program::Value)strtoull(startPtr, &endPtr, 10);
		// failed to parse a number
		if (endPtr == startPtr)
		{
			state.error = Program::CE_UNEXPECTED_CHAR;
			return 1;
		}
		state.Push(res);
		// advance our index based on where the end pointer wound up
		state.parsePos += (endPtr - startPtr) / sizeof(Program::Char);
	}

	while (!unaryOps.empty())
	{
		state.Push(unaryOps.top());
		unaryOps.pop();
	}

	return 0;
}

static int ParseFactors(CompilationState& state)
{
	if (ParseAtom(state)) return 1;
	for (;;)
	{
		// Skip spaces
		while (isspace(*state))
		{
			state.parsePos++;
		}

		// Save the operation and position
		Program::Char op = *state;
		if (op != '/' && op != '*' && op != '%')
		{
			return 0;
		}
		state.parsePos++;
		if (ParseAtom(state)) return 1;
		// Perform the saved operation
		if (op == '/')
		{
			state.Push(Program::Op::DIV);
		}
		else if (op == '%')
		{
			state.Push(Program::Op::MOD);
		}
		else
		{
			state.Push(Program::Op::MUL);
		}
	}
}

static int ParseSummands(CompilationState& state)
{
	if (ParseFactors(state)) return 1;
	for (;;)
	{
		// Skip spaces
		while (isspace(*state))
		{
			state.parsePos++;
		}
		Program::Char op = *state;
		if (op != '-' && op != '+')
		{
			return 0;
		}
		state.parsePos++;
		if (ParseFactors(state)) return 1;
		switch (op)
		{
		case '-': state.Push(Program::Op::SUB); break;
		case '+': state.Push(Program::Op::ADD); break;
		}
	}
}

static int ParseBitshift(CompilationState& state)
{
	if (ParseSummands(state)) return 1;
	for (;;)
	{
		while (isspace(*state))
		{
			state.parsePos++;
		}
		Program::Char op = *state;
		if (op != '<' && op != '>')
		{
			return 0;
		}
		state.parsePos++;
		Program::Char op2 = *state;
		if (op2 != op)
		{
			state.error = Program::CE_UNEXPECTED_CHAR;
			return 1;
		}
		state.parsePos++;
		if (ParseSummands(state)) return 1;
		switch (op)
		{
		case '<': state.Push(Program::Op::BSL); break;
		case '>': state.Push(Program::Op::BSR); break;
		}
	}
}

static int ParseAND(CompilationState& state)
{
	if (ParseBitshift(state)) return 1;
	for (;;)
	{
		while (isspace(*state))
		{
			state.parsePos++;
		}
		Program::Char op = *state;
		if (op != '&')
		{
			return 0;
		}
		state.parsePos++;
		if (ParseBitshift(state)) return 1;
		state.Push(Program::Op::AND);
	}
}

static int ParseXOR(CompilationState& state)
{
	if (ParseAND(state)) return 1;
	for (;;)
	{
		while (isspace(*state))
		{
			state.parsePos++;
		}
		Program::Char op = *state;
		if (op != '^')
		{
			return 0;
		}
		state.parsePos++;
		if (ParseAND(state)) return 1;
		state.Push(Program::Op::XOR);
	}
}

static int ParseOR(CompilationState& state)
{
	if (ParseXOR(state)) return 1;
	for (;;)
	{
		while (isspace(*state))
		{
			state.parsePos++;
		}
		Program::Char op = *state;
		if (op != '|')
		{
			return 0;
		}
		state.parsePos++;
		if (ParseXOR(state)) return 1;
		state.Push(Program::Op::OR);
	}
}

Program* Program::Compile(const Char* source, CompileError& outError, int& outErrorPosition)
{
	Program* program = nullptr;
	CompilationState state(source);

	ParseOR(state);

	// final error checking
	if (state.error == CE_NONE)
	{
		// Now, expr should point to '\0', and _paren_count should be zero
		if (state.parenCount != 0 || *state == ')')
		{
			state.error = Program::CE_MISSING_PAREN;
		}
		else if (*state != '\0')
		{
			state.error = Program::CE_UNEXPECTED_CHAR;
		}
	}

	// now create a program or don't
	if (state.error == CE_NONE)
	{
		outError = Program::CE_NONE;
		outErrorPosition = -1;
		program = new Program(state.ops);
	}
	else
	{
		outError = state.error;
		outErrorPosition = state.parsePos;
	}

	return program;
}

Program::Program(const std::vector<Op>& inOps)
: ops(inOps)
{
	// default sample rate so the F operator will function
	Set('~', 44100);
}

Program::~Program()
{
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////
// EXECUTION
//////////////////////////////////////////////////////////////////////////
#pragma region Execution
Program::RuntimeError Program::Run(Value& result)
{
	RuntimeError error = RE_NONE;
	const uint64_t count = GetInstructionCount();

	if (count > 0)
	{
		for (uint64_t i = 0; i < count; ++i)
		{
			Exec(ops[i]);
		}

		if (error != RE_NONE)
		{
			result = 0;
		}
		else if (stack.size() == 1)
		{
			result = stack.top();
			stack.pop();
		}
		else
		{
			error = RE_INCONSISTENT_STACK;
			result = 0;
		}
	}
	else
	{
		result = 0;
	}

	return error;
}

#define POP1 if ( stack.size() < 1 ) goto bad_stack; Value a = stack.top(); stack.pop();
#define POP2 if ( stack.size() < 2 ) goto bad_stack; Value b = stack.top(); stack.pop(); Value a = stack.top(); stack.pop();

// perform the operation
Program::RuntimeError Program::Exec(const Op& op)
{
	RuntimeError error = RE_NONE;
	switch (op.code)
	{
		// no operands - result is pushed to the stack
		case Op::NUM: 
			stack.push(op.val); 
			break;

		case Op::VAR: 
			stack.push(Get(op.var)); 
			break;

		// one operand - operand value is popped from the stack, result is pushed back on 
		case Op::NEG:
		{
			POP1;
			stack.push(-a);
		}
		break;

		case Op::SIN:
		{
			POP1;
			Value r = Get('r');
			Value hr = r / 2;
			r += 1;
			double s = sin(2 * M_PI * ((double)(a%r) / r));
			stack.push(Value(s*hr + hr));
		}
		break;

		case Op::SQR:
		{
			POP1;
			const Value r = Get('r');
			const Value v = a%r < r / 2 ? 0 : r - 1;
			stack.push(v);
		}
		break;

		case Op::FREQ:
		{
			POP1;
			double f = round(3 * pow(2.0, (double)a / 12.0) * (44100.0 / Get('~')));
			stack.push((Value)f);
		}
		break;

		case Op::TRI:
		{
			POP1;
			a *= 2;
			const Value r = Get('r');			
			const Value v = a*((a / r) % 2) + (r - a - 1)*(1 - (a / r) % 2);
			stack.push(v);
		}
		break;

		// two operands - both are popped from the stack, result is pushed back on
		case Op::MUL: 
		{
			POP2;
			stack.push(a*b);
		}
		break;

		case Op::DIV:
		{
			POP2;
			Value v = 0;
			if (b) { v = a / b; }
			else { error = RE_DIVIDE_BY_ZERO; }
			stack.push(v);
		}
		break;

		case Op::MOD: 
		{
			POP2;
			Value v = 0;
			if (b) { v = a%b; }
			else { error = RE_DIVIDE_BY_ZERO; }
			stack.push(v);
		}
		break;

		case Op::ADD: 
		{
			POP2;
			stack.push(a + b);
		}
		break;

		case Op::SUB: 
		{
			POP2;
			stack.push(a - b);
		}
		break;

		case Op::BSL: 
		{
			POP2;
			stack.push(a << b);
		}
		break;

		case Op::BSR: 
		{
			POP2;
			stack.push(a >> (b % 64));
		}
		break;

		case Op::AND: 
		{
			POP2;
			stack.push(a&b);
		}
		break;

		case Op::OR: 
		{
			POP2;
			stack.push(a | b);
		}
		break;

		case Op::XOR: 
		{
			POP2;
			stack.push(a^b);
		}
		break;

		// perform a no-op, but set the error as a result
		default:
		{
			error = RE_MISSING_OPCODE;
		}
		break;

		bad_stack:
		{
			error = RE_MISSING_OPERAND;
		}
		break;
	}

	return error;
}

Program::Value Program::Get(const Char var) const
{
	return Peek((int)var + 128);
}

void Program::Set(const Char var, const Value value)
{
	Poke((int)var + 128, value);
}


Program::Value Program::Peek(const uint32_t address) const
{
	// peeks wrap around so we never go outside of our memory space
	return mem[address%kMemorySize];
}


void Program::Poke(const uint32_t address, Value value)
{
	// pokes wrap around so we never go outside of our memory space
	mem[address%kMemorySize] = value;
}

#pragma endregion
