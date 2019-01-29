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
#include <chrono>
#include <ctype.h>
#include <deque>
#include <math.h>
#include <map>

const std::map<Program::Char, Program::Op::Code> UnaryOperators =
{
	{ '@', Program::Op::PEK },
	{ 'F', Program::Op::FRQ },
	{ '#', Program::Op::SQR },
	{ '$', Program::Op::SIN },
	{ 'T', Program::Op::TRI },
	{ '+', Program::Op::NOP },
	{ '-', Program::Op::NEG },
	{ '~', Program::Op::COM },
	{ '!', Program::Op::NOT },
	{ 'C', Program::Op::CCV },
	{ 'V', Program::Op::VCV },
	{ 'R', Program::Op::RND }
};

// used to store const values relating to [*], 
// which allows for reading the sum of all inputs or writing the same value to all outputs.
namespace Wildcard
{
	const Program::Char  Char = '*';
	const Program::Value Value = -1;
}

Program::Program(const std::vector<Op>& inOps, const size_t userMemorySize)
	: ops(inOps)
	, userMemSize(userMemorySize)
	, memSize(userMemorySize + 256) // 256 to enough room for all possible values of Char
	, rng(std::chrono::system_clock::now().time_since_epoch().count())
{
	mem = new Value[memSize];
	memset(mem, 0, sizeof(Value)*memSize);
	// initialize cc memory space - we want to accurately represent the midi device
	memset(cc, 0, sizeof(cc));
	memset(vc, 0, sizeof(vc));
	// default sample rate so the F operator will function
	Set('~', 44100);
}

Program::~Program()
{
	delete[] mem;
}

// static
Program::Value Program::GetAddress(const Char var, const size_t userMemorySize)
{
	// we static cast to unsigned char because we want to convert the variable
	// to a memory offset that begins at 0.
	return userMemorySize + static_cast<unsigned char>(var);
}

//////////////////////////////////////////////////////////////////////////
// COMPILATION
//////////////////////////////////////////////////////////////////////////
#pragma region Compilation

const char * Program::GetErrorString(Program::CompileError error)
{
	switch (error)
	{
	case Program::CE_NONE:
		return "None";
	case Program::CE_MISSING_PAREN:
		return "Mismatched parens";
	case Program::CE_MISSING_BRACKET:
		return "Missing ']'";
	case Program::CE_MISSING_BRACE:
		return "Missing '}'";
	case Program::CE_MISSING_COLON_IN_TERNARY:
		return "Incomplete ternary statement - expected ':'";
	case Program::CE_UNEXPECTED_CHAR:
		return "Unexpected character";
	case Program::CE_FAILED_TO_PARSE_NUMBER:
		return "Failed to parse a numeric value";
	case Program::CE_ILLEGAL_ASSIGNMENT:
		return "Left side of '=' must be assignable.\n(a variable, address, or output)";
	case Program::CE_ILLEGAL_STATEMENT_TERMINATION:
		return "Illegal statement termination.\n"
			   "Semi-colon may not appear within parens\nor ternary operators.";
	case Program::CE_ILLEGAL_VARIABLE_NAME:
		return "Illegal variable name.\n(uppercase letters are reserved for operators)";
	case Program::CE_MISSING_PUT:
		return "The program does not output any values.\n"
			   "Assign something to [0], [1], or [*].";
	default:
		return "Unknown";
	}
}

// used during compilation to keep track of things
struct CompilationState
{
	const Program::Char* const source;
	const size_t userMemSize;
	int parsePos;
	int parenCount;
	int bracketCount;
	int parseDepth;
	Program::CompileError error;
	std::vector<Program::Op> ops;

	CompilationState(const Program::Char* inSource, const size_t userMemorySize)
		: source(inSource)
		, userMemSize(userMemorySize)
		, parsePos(0)
		, parenCount(0)
		, bracketCount(0)
		, parseDepth(0)
		, error(Program::CE_NONE)
	{

	}

	// some helpers
	Program::Char operator*() const { return source[parsePos]; }
	size_t Push(Program::Op::Code code, Program::Value value = 0) { ops.push_back(Program::Op(code, value)); return ops.size()-1; }
	void SkipWhitespace()
	{
		while (isspace(source[parsePos]))
		{
			++parsePos;
		}

		// also skip any commented text while we are at it
		if (source[parsePos] == '/' && source[parsePos + 1] == '/')
		{
			parsePos += 2;
			// read to the end of line or end of file
			while (source[parsePos] != '\n' && source[parsePos] != '\0')
			{
				++parsePos;
			}

			// if we reached end of line and this is not end of file
			// then we might have more whitespace to skip on the next line
			if (source[parsePos] != '\0')
			{
				SkipWhitespace();
			}
		}
	}
};

// forward declare Parse so that we can recurse back to it from anywhere.
static int Parse(CompilationState& state);

static int ParseAtom(CompilationState& state)
{
	// Skip spaces
	state.SkipWhitespace();

	std::stack<Program::Op::Code> unaryOps;

	// see if the current character is a unary operator
	// and push the appropriate opcode onto the unaryOps stack.
	// we don't push a NOP because it's pointless to have any.
	while( UnaryOperators.count(*state) )
	{
		const Program::Op::Code code = UnaryOperators.find(*state)->second;
		if ( code != Program::Op::NOP )
		{
			unaryOps.push(code);
		}
		state.parsePos++;
	}

	// Check if there is parenthesis
	if (*state == '(')
	{
		state.parsePos++;
		state.parenCount++;
		if (Parse(state)) return 1;
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

	// check for bracket '['
	if (*state == '[')
	{
		state.parsePos++;
		state.bracketCount++;
		// check for wildcard before attempting to parse an expression
		state.SkipWhitespace();
		if (*state == Wildcard::Char)
		{
			state.parsePos++;
			state.Push(Program::Op::PSH, Wildcard::Value);
			state.SkipWhitespace();
		}
		else if (Parse(state))
		{
			return 1;
		}
		if (*state != ']')
		{
			state.error = Program::CE_MISSING_BRACKET;
			return 1;
		}
		state.parsePos++;
		state.bracketCount--;

		state.Push(Program::Op::GET);

		while (!unaryOps.empty())
		{
			state.Push(unaryOps.top());
			unaryOps.pop();
		}

		return 0;
	}

	if (isalpha(*state))
	{
		if (islower(*state))
		{
			const Program::Char var = *state;
			// push the address of the variable, which peek will need
			const Program::Value varAddress = Program::GetAddress(var, state.userMemSize);
			state.Push(Program::Op::PSH, varAddress);
			state.Push(Program::Op::PEK);
			state.parsePos++;
		}
		else
		{
			state.error = Program::CE_ILLEGAL_VARIABLE_NAME;
			return 1;
		}
	}
	else // parse a numeric value
	{
		const Program::Char* startPtr = state.source + state.parsePos;
		Program::Char* endPtr = nullptr;
		Program::Value res = (Program::Value)strtoull(startPtr, &endPtr, 0);
		// failed to parse a number
		if (endPtr == startPtr)
		{
			state.error = Program::CE_FAILED_TO_PARSE_NUMBER;
			return 1;
		}
		state.Push(Program::Op::PSH, res);
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
		state.SkipWhitespace();

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
		state.SkipWhitespace();
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

static int ParseCmpOrShift(CompilationState& state)
{
	if (ParseSummands(state)) return 1;
	for (;;)
	{
		state.SkipWhitespace();
		Program::Char op = *state;
		if (op != '<' && op != '>')
		{
			return 0;
		}
		state.parsePos++;
		Program::Char op2 = *state;
		// not a bitshift, so do compare
		if (op2 != op)
		{
			// for <= and >= we need to eat the equals character and push a different opcode
			bool isEqual = op2 == '=';
			if (isEqual)
			{
				state.parsePos++;
			}
			if (ParseSummands(state)) return 1;
			switch (op)
			{
			case '<': state.Push(isEqual ? Program::Op::CLE : Program::Op::CLT); break;
			case '>': state.Push(isEqual ? Program::Op::CGE : Program::Op::CGT); break;
			}
		}
		else
		{
			// is a bitshift, so eat it and continue
			state.parsePos++;
			if (ParseSummands(state)) return 1;
			switch (op)
			{
			case '<': state.Push(Program::Op::BSL); break;
			case '>': state.Push(Program::Op::BSR); break;
			}
		}
	}
}

static int ParseCEQ(CompilationState& state)
{
	if (ParseCmpOrShift(state)) return 1;
	for (;;)
	{
		state.SkipWhitespace();
		const Program::Char op = *state;
		if (op != '=' && op != '!')
		{
			return 0;
		}
		state.parsePos++;
		if (*state != '=')
		{
			// don't want to eat the first character if it is not followed by an equals, otherwise we will mess up assignments and negation
			state.parsePos--;
			return 0;
		}
		// eat the equals sign
		state.parsePos++;
		if (ParseCmpOrShift(state)) return 1;
		switch (op)
		{
		case '=': state.Push(Program::Op::CEQ); break;
		case '!': state.Push(Program::Op::CNE); break;
		}
	}
}

static int ParseAND(CompilationState& state)
{
	if (ParseCEQ(state)) return 1;
	for (;;)
	{
		state.SkipWhitespace();
		Program::Char op = *state;
		if (op != '&')
		{
			return 0;
		}
		state.parsePos++;
		if (ParseCEQ(state)) return 1;
		state.Push(Program::Op::AND);
	}
}

static int ParseXOR(CompilationState& state)
{
	if (ParseAND(state)) return 1;
	for (;;)
	{
		state.SkipWhitespace();
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
		state.SkipWhitespace();
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

static int ParseCND(CompilationState& state)
{
	if (ParseOR(state)) return 1;
	for (;;)
	{
		state.SkipWhitespace();
		Program::Char op = *state;
		if (op != '?')
		{
			return 0;
		}
		state.parsePos++;

		// result of the expression before the ? will be on the top of the stack now,
		// the CND instruction needs to check that value and jump over the next expression if it is false.
		// we won't know where to jump until after generating the instructions for the expression,
		// so we stash where in the the ops list our CND op needs to go, which allows us to insert it when we have the address.
		size_t cndOpAddr = state.Push(Program::Op::CND);

		// parse expression following the ?
		// we decrement parseDepth before calling Parse because it's OK if the expression ends with a semi-colon.
		// this will make the statement behave like an if statement.
		state.parseDepth--;
		if (Parse(state)) return 1;
		state.parseDepth++;

		state.SkipWhitespace();

		// this means it ended with a semi-colon, we need to insert some instructions before this, so we remove it and add it back
		bool hasPop = state.ops.back().code == Program::Op::POP;
		if (hasPop)
		{
			state.ops.pop_back();
		}
		
		// add a JMP instruction so we can skip what comes next, which is the "false" part of the expression
		size_t jmpOpAddr = state.Push(Program::Op::JMP);
		// CND needs to jump to the instruction that follows the JMP
		state.ops[cndOpAddr].val = state.ops.size();
		
		// if there is a colon, the user has provided code to execute for "false".
		// if there isn't, then we need to provide the result of the expression, which will simply be 0.
		// in other words:
		//		a = b ? c;
		// is simply syntactic sugar for:
		//		a = b ? c : 0;
		if (*state == ':')
		{	
			// if they put a semi-colon before the colon, Parse won't have caught it, so we do so here
			if (hasPop)
			{
				state.error = Program::CE_ILLEGAL_STATEMENT_TERMINATION;
				return 1;
			}

			// eat the colon
			state.parsePos++;
			// when parsing what follows the colon, we decrement parse depth before recursing to Parse.
			// this is so that if the line terminates with a semi-colon, we won't get an
			// illegal statement termination error, unless we were already within parens.
			state.parseDepth--;
			if (Parse(state)) return 1;
			state.parseDepth++;
		}	
		else
		{
			state.Push(Program::Op::PSH, 0);

			// include the semi-colon that is in the source
			if (hasPop)
			{
				state.Push(Program::Op::POP);
			}
		}

		// if the statement terminated in a semi-colon we will have a POP on the end of the list.
		// we need to jump directly to that POP because it *might* be replaced with a PEK or PUT.
		if (state.ops.back().code == Program::Op::POP)
		{
			state.ops[jmpOpAddr].val = state.ops.size() - 1;
		}
		else
		{
			// when there's no POP we need to JMP to the instruction that will follow it.
			state.ops[jmpOpAddr].val = state.ops.size();
		}
	}
}

static int ParsePOK(CompilationState& state)
{
	if (ParseCND(state)) return 1;
	for (;;)
	{
		state.SkipWhitespace();
		Program::Char op = *state;
		if (op != '=')
		{
			return 0;
		}
		state.parsePos++;
		// PEK and GET work by popping a value from the stack to use as the lookup address.
		// so when we want to POK or PUT, we can use that same address to know where in memory to assign the result of the right side.
		// we just need to remove the existing instruction so the address will still be on the stack after the instructions generated by ParseTRN complete.
		// we require the presence of a PEK or GET instruction because we don't want to allow statements like '5 = 4'
		// instead, we accept '@5 = 4', which means "set memory address 5 to the value 4".
		// similarly, 'a = 4' will assign 4 to the memory address reserved for the variable 'a' (see ParseAtom).
		// [0] = 5 will put the value 5 into first output result
		Program::Op::Code code = state.ops.back().code;
		if (code == Program::Op::PEK || code == Program::Op::GET)
		{
			state.ops.pop_back();
		}
		else
		{
			state.error = Program::CE_ILLEGAL_ASSIGNMENT;
			return 1;
		}

		state.SkipWhitespace();
		// how many values to POK or PUT
		int pcount = 0;
		// check for the beginning of an "array" on the right side of the equals sign.		
		if (*state == '{')
		{
			state.parsePos++;
			for(;;)
			{
				state.parseDepth--;
				if (Parse(state)) return 1;
				state.parseDepth++;
				++pcount;
				
				if (*state == ',')
				{					
					state.parsePos++;
					continue;
				}

				// if we didn't find a comma, we *should* find the closing brace
				if (*state == '}')
				{
					state.parsePos++;
					break;					
				}
				else
				{
					state.error = Program::CE_MISSING_BRACE;
					return 1;
				}
			}
		}
		else // no array, so it's just a single expression
		{
			// decrement the parse depth before recursing because it's 
			// OK if the expression on the right hand side terminates in a semi-colon
			state.parseDepth--;
			if (Parse(state)) return 1;
			state.parseDepth++;	
			pcount = 1;
		}

		// the statement on the right side of the '=' might have ended with a semi-colon,
		// which means the last op will be a POP. we need to POK or PUT before that.
		const bool hasPOP = state.ops.back().code == Program::Op::POP;
		if (hasPOP)
		{
			state.ops.pop_back();
		}
		switch (code)
		{
		case Program::Op::PEK:
			state.Push(Program::Op::POK, pcount);
			break;

		case Program::Op::GET:
			state.Push(Program::Op::PUT, pcount);
			break;
			
		// fix warning in osx
		default:
			break;
		}
		if (hasPOP)
		{
			state.Push(Program::Op::POP);
		}
	}
}

// we start here so that we don't have to change the forward declare for ParseAtom when we add another level
static int Parse(CompilationState& state)
{
	state.parseDepth++;
	{
		if (ParsePOK(state)) return 1;
		// check for statement termination
		state.SkipWhitespace();
		if (*state == ';')
		{
			// if we have recursed into Parse due to opening parens
			// or due to parsing a section of a ternary operator,
			// we should throw an error if we encounter a semi-colon
			// because those constructs will not evaluate correctly
			// if a POP appears in the middle of the instructions.
			if (state.parseDepth != 1)
			{
				state.error = Program::CE_ILLEGAL_STATEMENT_TERMINATION;
				return 1;
			}
			state.parsePos++;
			state.Push(Program::Op::POP);
			// skip space immediately after statement termination
			// in case this is the last symbol of the program but there is trailing whitespace
			state.SkipWhitespace();
		}
	}
	state.parseDepth--;
	return 0;
}

Program* Program::Compile(const Char* source, const size_t userMemorySize, CompileError& outError, int& outErrorPosition)
{
	Program* program = nullptr;
	CompilationState state(source, userMemorySize);

	while (*state != '\0')
	{
		if (Parse(state)) break;
		// if we aren't at the end yet, we should have a POP as the last op.
		if (*state != '\0' && state.ops.back().code != Op::POP)
		{
			state.error = CE_UNEXPECTED_CHAR;
			break;
		}
	}

	// final error checking
	if (state.error == CE_NONE)
	{
		// Now, expr should point to '\0', and _paren_count should be zero
		if (state.parenCount != 0 || *state == ')')
		{
			state.error = CE_MISSING_PAREN;
		}
		else if (*state != '\0')
		{
			state.error = CE_UNEXPECTED_CHAR;
		}
		// make sure the program includes at least one PUT.
		// we consider it a compilation error if it doesn't
		// since the resulting program would output nothing 
		// and therefore be useless.
		else
		{
			bool hasPut = false;
			for (auto& op : state.ops)
			{
				hasPut = hasPut || op.code == Op::PUT;
			}

			if (!hasPut)
			{
				state.error = CE_MISSING_PUT;
			}
		}
	}

	// now create a program or don't
	if (state.error == CE_NONE)
	{
		outError = CE_NONE;
		outErrorPosition = -1;
		program = new Program(state.ops, userMemorySize);
	}
	else
	{
		outError = state.error;
		outErrorPosition = state.parsePos;
	}

	return program;
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////
// EXECUTION
//////////////////////////////////////////////////////////////////////////
#pragma region Execution

const char * Program::GetErrorString(Program::RuntimeError error)
{
	switch (error)
	{
	case RE_NONE:
		return "None";
	case RE_DIVIDE_BY_ZERO:
		return "Divide by zero";
	case RE_MISSING_OPERAND:
		return "Missing operand";
	case RE_MISSING_OPCODE:
		return "Unimplemented opcode";
	case RE_INCONSISTENT_STACK:
		return "Inconsistent stack";
	case RE_EMPTY_PROGRAM:
		return "Empty program (instruction count is zero)";
	case RE_GET_OUT_OF_BOUNDS:
		return "Input access is out of bounds";
	case RE_PUT_OUT_OF_BOUNDS:
		return "Output access is out of bounds";
	default:
		return "Unknown";
	}
}

Program::RuntimeError Program::Run(Value* results, const size_t size)
{
	RuntimeError error = RE_NONE;
	const uint64_t icount = GetInstructionCount();
	if (icount > 0)
	{
		pc = 0;
		for (; pc < icount && error == RE_NONE; ++pc)
		{
			error = Exec(ops[pc], results, size);
		}

		// under error-free execution we should have either 1 or 0 values in the stack.
		// 1 when a program terminates with the result of an expression (eg: t*Fn)
		// 0 when a program terminates with a POP (eg: t*Fn;)
		// in the case of the POP, the value of the expression will already be in result.
		if (error == RE_NONE)
		{
			if (stack.size() > 1)
			{
				error = RE_INCONSISTENT_STACK;
			}
		}

		// clear the stack so it doesn't explode in size due to continual runtime errors
		while (stack.size() > 0)
		{
			stack.pop();
		}
	}
	else
	{
		error = RE_EMPTY_PROGRAM;
	}

	return error;
}

#define POP1 if ( stack.size() < 1 ) goto bad_stack; Value a = stack.top(); stack.pop();
#define POP2 if ( stack.size() < 2 ) goto bad_stack; Value b = stack.top(); stack.pop(); Value a = stack.top(); stack.pop();
#define POP3 if ( stack.size() < 3 ) goto bad_stack; Value c = stack.top(); stack.pop(); Value b = stack.top(); stack.pop(); Value a = stack.top(); stack.pop();
#define POP(n) if (stack.size() < n) goto bad_stack; std::vector<Value> args; for(int i = 0; i < n; ++i) { args.push_back(stack.top()); stack.pop(); }

// perform the operation
Program::RuntimeError Program::Exec(const Op& op, Value* results, size_t size)
{
	RuntimeError error = RE_NONE;
	switch (op.code)
	{
		// no operands - result is pushed to the stack
	case Op::PSH:
		stack.push(op.val);
		break;

	case Op::POP:
	{
		stack.pop();
		// stack should now be empty, if it isn't that's an error
		if (stack.size() > 0)
		{
			error = RE_INCONSISTENT_STACK;
		}
	}
	break;

		// one operand - operand value is popped from the stack, result is pushed back on 
	case Op::PEK:
	{
		POP1;
		stack.push(Peek(a));
	}
	break;

	case Op::GET:
	{
		POP1;
		Value v = 0;
		// wildcard GET should return the sum of all channels
		if (a == Wildcard::Value)
		{
			for (size_t i = 0; i < size; ++i)
			{
				v += results[i];
			}
		}
		else if (a < size)
		{
			v = results[a];
		}
		else
		{
			error = RE_GET_OUT_OF_BOUNDS;
		}
		stack.push(v);
	}
	break;

	case Op::NEG:
	{
		POP1;
		stack.push(-a);
	}
	break;

	case Op::SIN:
	{
		POP1;
		Value r = Get('w');
		Value hr = r / 2;
		r += 1;
		double s = sin(2 * M_PI * ((double)(a%r) / r));
		stack.push(Value(s*hr + hr));
	}
	break;

	case Op::SQR:
	{
		POP1;
		const Value r = Get('w');
		const Value v = a%r < r / 2 ? 0 : r - 1;
		stack.push(v);
	}
	break;

	case Op::FRQ:
	{
		POP1;
		if (a == 0)
		{
			stack.push(0);
		}
		else
		{
			// 3.023625 is a magic number arrived at by comparing our output to the Saw Wave in ReaSynth.
			// 3.0 is what we'd expect to see if we were operating in floating point,
			// but if we use 3.0 here, the pitch winds up being a little bit flat.
			double f = round(4.0 * 3.023625 * pow(2.0, (double)a / 12.0) * (44100.0 / Get('~')));
			stack.push((Value)f);
		}
	}
	break;

	case Op::TRI:
	{
		POP1;
		a *= 2;
		const Value r = Get('w');
		const Value v = a*((a / r) % 2) + (r - a - 1)*(1 - (a / r) % 2);
		stack.push(v);
	}
	break;

	case Op::RND:
	{
		POP1;
		stack.push(rng() % a);
	}
	break;

	case Op::CCV:
	{
		POP1;
		stack.push(GetCC(a));
	}
	break;

	case Op::VCV:
	{
		POP1;
		stack.push(GetVC(a));
	}
	break;
			
	case Op::NOT:
	{
		POP1;
		stack.push(!a);
	}
	break;
	
	case Op::COM:
	{
		POP1;
		stack.push(~a);
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
		const auto s = b % 64;
		stack.push(a << s);
	}
	break;

	case Op::BSR:
	{
		POP2;
		const auto s = b % 64;
		stack.push(a >> s);
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

	case Op::CEQ:
	{
		POP2;
		stack.push(a == b);
	}
	break;

	case Op::CNE:
	{
		POP2;
		stack.push(a != b);
	}
	break;

	case Op::CLT:
	{
		POP2;
		stack.push(a < b);
	}
	break;

	case Op::CLE:
	{
		POP2;
		stack.push(a <= b);
	}
	break;

	case Op::CGT:
	{
		POP2;
		stack.push(a > b);
	}
	break;

	case Op::CGE:
	{
		POP2;
		stack.push(a >= b);
	}
	break;

	// number of operands is variable
	case Op::POK:
	{
		// pop off all of the results
		POP(op.val);
		// pop off the address for the first result
		POP1;

		// the order of the args is last to first,
		// ie: { 1, 2, 3 } winds up in args as { 3, 2, 1 }
		// this is why we pop values for Poke from the back of args.
		for (int i = 0; i < op.val; ++i)
		{
			Value b = args.back();
			args.pop_back();

			Value address = a + i;
			Poke(address, b);
		}

		// the result of this operation should be what is now in the *first* memory address (ie 'a')
		// this is to make chained assignment statements work as expected. 
		// for example:
		//
		//	a = @1 = { 1, 2, 3 };
		//
		// should result in the value of 'a' being equal to the value of '@1'
		stack.push(Peek(a));
	}
	break;

	case Op::PUT:
	{
		// pop off all of the results
		POP(op.val);
		// pop off the address for the first result
		POP1;

		Value b = args.back();
		args.pop_back();

		// [*] = should fill the entire output
		// so we assign results in order until we run out
		// and then repeat the last one to the remaining outputs.
		// so, if there is only 1 result, it is copied to all outputs.
		if (a == Wildcard::Value)
		{	
			// since [*] returns the sum of all values (see GET), we need to sum up the output as we go
			Value c = 0;

			for (size_t i = 0; i < size; ++i)
			{
				results[i] = b;
				c += b;
				if (!args.empty())
				{
					b = args.back();
					args.pop_back();
				}
			}

			stack.push(c);
		}
		else if (a < size)
		{
			// as with POK, the result of this operation should be equal to the first place we put a value.
			// for example:
			//
			// a = [0] = { 1, 2 }
			//
			// should make a and [0] equal to 1, while [1] would be equal to 2
			stack.push(b);

			// begin assigning results starting from the provided index,
			// but stop if we run out of args or get to the end of the output array.
			results[a++] = b;
			while (a < size && !args.empty())
			{
				b = args.back();
				args.pop_back();
				results[a++] = b;
			}
		}
		else
		{
			error = RE_PUT_OUT_OF_BOUNDS;
		}		
	}
	break;

	// flow control.
	// the pc is set to one less than the target address because it will be incremented after this method returns.
	case Op::CND:
	{
		POP1;
		if (!a) 
		{ 
			pc = op.val-1; 
		}
	}
	break;

	case Op::JMP:
	{
		pc = op.val-1;
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
	return Peek(GetAddress(var, userMemSize));
}

void Program::Set(const Char var, const Value value)
{
	Poke(GetAddress(var, userMemSize), value);
}

Program::Value Program::GetCC(const Value idx) const
{
	// prevent array reading overrun by wrapping around, since this is how accessing memory works
	return cc[idx % kCCSize];
}

void Program::SetCC(const Value idx, const Value value)
{
	cc[idx % kCCSize] = value;
}

Program::Value Program::GetVC(const Value idx) const
{
	return vc[idx % kVCSize];
}

void Program::SetVC(const Value idx, const Value value)
{
	vc[idx % kVCSize] = value;
}

Program::Value Program::Peek(const Value address) const
{
	// peeks wrap around so we never go outside of our memory space
	return mem[address%memSize];
}


void Program::Poke(const Value address, const Value value)
{
	// pokes wrap around so we never go outside of our memory space
	mem[address%memSize] = value;
}

#pragma endregion
