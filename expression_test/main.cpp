//
//  main.cpp
//  expression_test
//
//  Created by Damien Quartz on 11/25/16.
//
//

#include <iostream>
#include <iomanip>
#include <math.h>
#include <cassert>
#include "../Program.h"

// Timer from http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
class Timer
{
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const {
        return std::chrono::duration_cast<second_>
        (clock_::now() - beg_).count(); }
    
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};

static Program::Value w = 1<<15;
static Program::Value n = 64;
static Program::Value t = 112344324;
static Program::Value m = t / (Program::Value)(44100/1000);
static Program::Value p = 234125150;

#define EEE_NO_ERROR Program::CE_NONE
#define EEE_PARENTHESIS Program::CE_MISSING_PAREN
#define EEE_WRONG_CHAR Program::CE_UNEXPECTED_CHAR

static Program::Value $(Program::Value a)
{
    Program::Value hr = w/2;
    Program::Value r1 = w+1;
    double s = sin(2 * M_PI * ((double)(a%r1)/r1));
    return Program::Value(s*hr + hr);
}

static Program::Value s(Program::Value a)
{
    return a%w < w/2 ? 0 : w-1;
}

static Program::Value F(Program::Value a)
{
	double f = round(4.0 * 3.023625 * pow(2.0, (double)a / 12.0));
    return (Program::Value)f;
}

static Program::Value T(Program::Value a)
{
	a *= 2;
	return a*((a / w) % 2) + (w - a - 1)*(1 - (a / w) % 2);
}

// ddf (12/5/16)
// if b is greater than the width of the int being shifted
// and is present in the lamba as a numeric constant
// the optimizer will recognize this fact and optimize out the operation.
// however, when shift right runs in the Expression code,
// it is operating on variables that the optimizer does not know the value of,
// so the operation will actually execute and behavior is that b is wrapped to the width of type being shifted.
static Program::Value sr(Program::Value a, Program::Value b)
{
    return a>>(b%64);
}

struct Test
{
    const char * expr;
    Program::Value (*eval)(void);
    const Program::CompileError error;
};

#define EVAL(x) []()->Program::Value{ return x; }

Test tests[] = {
    { "[*] = 1234", EVAL(1234) , EEE_NO_ERROR },
    { "[*] = 1+2", EVAL(1+2), EEE_NO_ERROR },
    { "[*] = 2-1", EVAL(2-1), EEE_NO_ERROR },
    { "[*] = 2*2", EVAL(2*2), EEE_NO_ERROR },
    { "[*] = 2/2", EVAL(2/2), EEE_NO_ERROR },
    { "[*] = 1+2*3", EVAL(1+2*3), EEE_NO_ERROR },
    { "[*] = Fn", EVAL(F(n)), EEE_NO_ERROR },
	{ "[*] = Tn", EVAL(T(n)), EEE_NO_ERROR },
    { "[*] = --2", EVAL(2), EEE_NO_ERROR },
    { "[*] = 2--2", EVAL(4), EEE_NO_ERROR },
    { "[*] = 2+-2", EVAL(0), EEE_NO_ERROR },
    { "[*] = 2-+-2", EVAL(4), EEE_NO_ERROR },
    { "[*] = #$2", EVAL(s($(2))), EEE_NO_ERROR },
    { "[*] = $#2", EVAL($(s(2))), EEE_NO_ERROR },
    { "[*] = $(#2)", EVAL($((s(2)))), EEE_NO_ERROR },
    { "[*] = $Fn", EVAL($(F(n))), EEE_NO_ERROR },
    { "[*] = $(Fn)", EVAL($(F(n))), EEE_NO_ERROR },
    { "[*] = (t*Fn)*((t*Fn/w)%2) + (w-t*Fn-1)*(1 - (t*Fn/w)%2)", EVAL((t*F(n))*((t*F(n)/w)%2) + (w-t*F(n)-1)*(1 - (t*F(n)/w)%2)), EEE_NO_ERROR },
    { "[*] = (t*128 + $(t)) | t>>(t%(8*w))/w | t>>128", EVAL((t*128 + $(t)) | t>>(t%(8*w))/w | sr(t,128)), EEE_NO_ERROR },
    { "[*] = (t*64 + $(t^$(m/2000))*$(m/2000)) | t*32", EVAL((t*64 + $(t^$(m/2000))*$(m/2000)) | t*32), EEE_NO_ERROR },
    { "[*] = t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", EVAL(t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128), EEE_NO_ERROR },
    { "[*] = $(t*F(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))", EVAL($(t*F(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))), EEE_NO_ERROR },
    { "[*] = (t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32", EVAL((t<<t/(1024*8) | sr(t,t/16) & sr(t,t/32)) / (t%(t/512+1) + 1) * 32), EEE_NO_ERROR },
    { "[*] = (w/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)", EVAL((w/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)), EEE_NO_ERROR },
    { "[*] = (1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", EVAL((1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4), EEE_NO_ERROR },
    { "[*] = $(t*Fn) | t*n/10>>4 ^ p>>(m/250%12)", EVAL($(t*F(n)) | t*n/10>>4 ^ p>>(m/250%12)), EEE_NO_ERROR },
    { "[*] = (t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", EVAL((t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12), EEE_NO_ERROR },
    
    // test syntax errors
    { "[*] = 5*(2*$(1+3+1)", EVAL(0), EEE_PARENTHESIS },
    { "[*] = 5*/2", EVAL(0), Program::CE_FAILED_TO_PARSE_NUMBER },
};

const int testCount = sizeof(tests) / sizeof(Test);
const int testIterations = 1024*8;

void set(Program& e, Program::Value _t, Program::Value _p)
{
    t = _t;
    m = t / (Program::Value)(44100/1000);
    e.Set('t', t);
    e.Set('m', m);
    
    p = _p;
    e.Set('p', _p);
}

int main(int argc, const char * argv[])
{
    Timer timer;
    Program::Char str[1024];
    for(int i = 0; i < testCount; ++i)
    {
        Test& test = tests[i];
        strcpy(str, test.expr);
        std::cout << '"' << str << '"';
		Program::CompileError err;
		int errPos;
		Program* program = Program::Compile(str, 1024, err, errPos);
		if (program != nullptr)
		{
			program->Set('w', w);
			program->Set('n', n);
			program->Set('t', t);
			program->Set('m', m);
			program->Set('p', p);
		}
        switch( test.error )
        {
            case EEE_NO_ERROR:
            {
                if ( err != EEE_NO_ERROR )
                {
                    std::cout << " FAILED with error: "  << Program::GetErrorString(err) << '\n';
                    auto off = errPos - (int)str;
                    for(int i = 0; i < off; ++i)
                    {
                        std::cout << ' ';
                    }
                    std::cout << "^\n";
                }
                else
                {
                    std::cout << " compiled to " << program->GetInstructionCount() << " instructions.";
                    set(*program, 0, 0);
                    double elapsed = 0;
					Program::Value result[2];
                    for(int i = 0; i < testIterations; ++i)
                    {
                        timer.reset();
						program->Run(result, 2);
                        elapsed += timer.elapsed();
                        result[1] = test.eval();
                        if ( result[0] != result[1] )
                        {
                            std::cout << " FAILED! " << result[0] << " != " << result[1] << '\n';
                        }
                        assert( result[0] == result[1] );
                        set(*program, t+1, result[0]);
                    }
                    elapsed /= testIterations;
                    std::cout << " PASSED with " << (elapsed*1000) << " ms avg execution time" << std::endl;
                }
            }
            break;
                
            default:
            {
                if ( err != test.error )
                {
                    std::cout << " FAILED! " << Program::GetErrorString(err) << " != " << Program::GetErrorString(test.error) << std::endl;
                }
                else
                {
                    std::cout << " PASSED" << std::endl;
                }
            }
            break;
        }

		assert(err == test.error);
    }
    return 0;
}
