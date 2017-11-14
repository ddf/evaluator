//
//  Presets.cpp
//  Evaluator
//
//  Created by Damien Quartz on 10/9/17
//
//

#include "Presets.h"
#include "Evaluator.h" // for the TimeType enum

#define CR "\r\n"

namespace Presets
{
#if SA_API
	const TimeType TTProjectTime = TTAlways;
#endif

	const Data kPresets[] =
	{
		// name, volume, bit depth, t-mode, program text
		{
			"saw wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// t is automatically incremented before generating a sample frame." CR
			"// n is set to the most recent MIDI Note On value." CR
			"// F is a unary operator that converts its operand to a 'frequency'." CR
			"// If F's operand is a MIDI note number," CR
			"// the result can be multiplied with t to create an oscillator" CR 
			"// whose pitch will be the same the MIDI note's pitch." CR
			"// increasing BIT will lower the octave, decreasing BIT will raise the octave." CR
			"// since t increases linearly, this oscillator creates a saw wave." CR
			"// assigning to [*] sends the value to all output channels of the program." CR
			"[*] = t*Fn",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"square wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// # is a unary operator that creates a 'square' of its operand." CR
			"// it does this by taking the operand modulo w" CR
			"// and returning 0 if the result is less than w/2" CR
			"// or w-1 if greater than or equal to w/2." CR
			"// ie: #a == a%w < w/2 ? 0 : w-1" CR
			"// this lets us turn a saw wave into a square wave" CR
			"[*] = #(t*Fn)",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"sine wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// $ is a unary operator that returns the 'sine' of its operand." CR
			"// the 'sine' is calculated in relation to the current value of w." CR
			"// this lets us turn a saw wave into a sine wave" CR
			"[*] = $(t*Fn)",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"triangle wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// T is a unary operator that returns the 'triangle' of its operand." CR
			"// like $ and #, the operation uses the current value of w." CR
			"// this lets us easily turn a saw wave into a triangle wave" CR
			"[*] = T(t*Fn)",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"pulse wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// a pulse wave is a square wave that spends longer" CR
			"// on one of the two values every cycle." CR
			"// we use a modified version of the square formula to do this:" CR
			"// first we create our wrapped saw wave and save it in a variable" CR
			"// (any lowercase letter can be used as a variable)." CR
			"a = (t*Fn)%w;" CR
			"// we want the high part of the wave to last longer than the low part" CR
			"// so we use the less-than operator to create a 0 or 1 value" CR
			"b = a < 8800;" CR
			"// we then multiply w-1 by our 0 or 1 value to create the pulse wave" CR
			"[*] = (w-1)*b",
			// watches
			"a", "b", "", "", "", "", "", "", "", "",
		},

		{
			"ternary arp",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// the ternary operator '?:' can be used to create expressions" CR
			"// that will resolve to one of two values." CR
			"// here we use it to modify n every other sixteenth note." CR
			"// q is incremented every 128th note, so we can divide by 32" CR
			"// to get a number that increments every sixteenth note." CR
			"s = q/32;" CR
			"// if s is odd, we want to increase the note by an octave." CR
			"a = n + (s%2 ? 12 : 0);" CR
			"// finally, we can create our oscillator" CR
			"[*] = t*Fa",
			// watches
			"s", "a", "", "", "", "", "", "", "", "",
		},

		{
			"frequency modulation",
			50, 15, TTWithNoteResetting,
			2, 0, 0, 0, 0, 0, 0, 0,
			"// a saw wave can be frequency modulated by adding a sine wave to it." CR
			"// we use the value of the V0 knob to control the modulation frequency." CR
			"[*] = t*Fn + $(t*V0)",
			// watches
			"V0", "", "", "", "", "", "", "", "", "",
		},

		{
			"amplitude modulation",
			50, 15, TTAlways,
			12, 0, 0, 0, 0, 0, 0, 0,
			"// to amplitude modulate we need to scale the range up and down," CR
			"// but keep the values 'centered' around w/2." CR
			"// so we wrap to [0, w) before scaling with our modulator." CR
			"o = (t*Fn) % w;" CR
			"// we step the phase of our modulator forward every 8 milliseconds," CR
			"// which lets us smoothly control the frequency with the V0 knob." CR
			"p = m % 8 ? p : p + V0 * 2;" CR
			"// if there isn't a note on, we reset the phase so that $p will be 0." CR
			"p = n < 1 ? 3 * w / 4 : p;" CR
			"m = $p;" CR
			"// m is a [0,w) value, so we decrease o by that ratio" CR
			"o = o * m/w;" CR
			"// then to center, we offset by how much w/2 is changed by this ratio" CR
			"[*] = o + ((w/2) - (w/2) * m/w);",
			// watches
			"V0", "o", "p", "", "", "", "", "", "", "",
		},

		{
			"midi pitch sweep",
			15, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// the C operator allows us to access incoming midi control change values." CR
			"// here we use the modwheel to control the pitch of an oscillator." CR
			"// in order to have the pitch change smoothly, we step the phase explicitly" CR
			"// instead of using a multiple of t as in other presets." CR
			"// we want the modwheel to sweep up an octave so we add in" CR
			"// a fraction of the distance to the note an octave above n." CR
			"// by doing this after converting to 'frequency' we get a smoother sweep." CR
			"a = Fn + (F(n+12) - Fn)*C1/127;" CR
			"o = n>0 ? o + a : w/2;" CR
			"[*] = o;",
			// watches
			"a", "o", "C1", "", "", "", "", "", "", "",
		},

		{
			"little ditty",
			50, 15, TTProjectTime,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// here we make a rhythmic ditty by combining three signals:" CR
			"// a frequency modulated saw wave (try changing the frequency of modulation)" CR
			"a = (t * 128 + $(t));" CR
			"// >> is a bit-shift operator that shifts all bits to the right" CR
			"// by the number of bits indicated by the right hand side." CR
			"// try changing the hard-coded 8 in here." CR
			"b = t >> (t % (8 * w)) / w;" CR
			"// the right hand side of >> is wrapped to 64 before the shift," CR
			"// so this line is the same as 'c = t;'" CR
			"c = t >> 128;" CR
			"// finally, we use bitwise OR to combine the three values." CR
			"// this is similar to summing floating point signals." CR
			"[*] = a | b | c",
			// watches
			"a", "b", "c", "", "", "", "", "", "", "",
		},

		{
			"overtone waterfall",
			50, 17, TTAlways,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// here is another example of combining multiple signals with bitwise OR." CR
			"// first a vanilla low frequency saw" CR
			"a = t*128;" CR
			"// then a saw that descends through the overtone series," CR
			"// changing pitch every 50 milliseconds" CR
			"b = a*(32-(m/50)%32);" CR
			"// and then a saw that ascends through the overtone series," CR
			"// changing pitch every 100 milliseconds" CR
			"c = a*((m/100)%64);" CR
			"[*] = a | b | c",
			// watches
			"a", "b", "c", "", "", "", "", "", "", "",
		},

		{
			"computer music" ,
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// here we create a random note sequence" CR
			"// based on the incoming MIDI note." CR
			"// r is how often the note changes - every 125 milliseconds" CR
			"r = 125;" CR
			"// R is a unary operator that produces a random value using the operand." CR
			"// R22 will range between 0 and 21." CR
			"// we choose a new value when p equals zero and is less than m%r" CR
			"a = p<1 & p<m%r ? R22 : a;" CR
			"p = m%r;" CR
			"[*] = $(t*F(n + a))",
			// watches
			"s", "a", "", "", "", "", "", "", "", "",
		},

		{
			"aggressive texture",
			50, 15, TTProjectTime,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// a low frequency sine wave" CR
			"s = $(m/2000);" CR
			"// ^ is bitwise XOR" CR
			"a = $(t^s);" CR
			"// you can cleary hear the held low frequency t*32 here" CR
			"// whereas the left side of the OR is sweeping wildly" CR
			"[*] = (t*64 + a*s) | t*32",
			// watches
			"s", "a", "", "", "", "", "", "", "", "",
		},

		// this maybe takes too long to get interesting and is just unpleasant noise mostly, might want to ditch it.
		{
			"blurp",
			50, 15, TTAlways,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// if you can stand the noise, let this one run for a long time."
			"// << shifts the bits of the left hand side" CR
			"// by the number on the right hand side modulo 64" CR
			"a = t<<t/(1024*8);" CR
			"b = t>>t/16;" CR
			"c = t>>t/32;" CR
			"// & is bitwise AND" CR
			"e = (a | b & c);" CR
			"// we add 1 in two places here to prevent divide-by-zero errors" CR
			"d = t%(t/512+1) + 1;" CR
			"// remember that a is divided by d and then multiplied by 32" CR
			"[*] = e/d * 32",
			// watches
			"a", "b", "c", "d", "e", "", "", "", "", "",
		},

		{
			"garbage trash",
			50, 15, TTAlways,
			16, 0, 0, 0, 0, 0, 0, 0,
			"// let this one run for a while, it subtly changes over time." CR
			"// r will increase by one every V0+1 milliseconds." CR
			"// we add one to V0 to prevent divide-by-zero." CR
			"r = m/(V0+1);" CR
			"// s will cycle through [0,15] at the same rate." CR
			"s = r%16;" CR
			"// note here that due to operator precedence," CR
			"// t is multiplied by s and then the modulo is applied" CR
			"[*] = (256*s + t*s%(512*s+1)) * r",
			// watches
			"r", "s", "", "", "", "", "", "", "", "",
		},

		{
			"nonsense can",
			50, 15, TTAlways,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// this demonstrates using the previous value of the program" CR
			"// in the calculation for output value" CR
			"a = 1 + $(m)%32;" CR
			"b = t*128 & t*64 & t*32;" CR
			"c = (p/16) << p%4;" CR
			"d = $(p/128) >> p%4;" CR
			"p = a ^ b | c | d;" CR
			"[*] = p;",
			// watches
			"a", "b", "c", "d", "p", "", "", "", "", "",
		},

		{
		"moving average",
		50, 17, TTAlways,
		0, 0, 0, 0, 0, 0, 0, 0,
		"// this sonifies the cumulative moving average for the function" CR
		"// f(x) = x*256 ^ x*64 & x*32" CR
		"// see https://en.wikipedia.org/wiki/Moving_average" CR
		"x = t+1;" CR
		"f = x*256 ^ x*64 & x*32;" CR
		"p = p + (f - p)/x;" CR
		"[*] = p;",
		// watches
		"x", "f", "p", "", "", "", "", "", "", "",
		},

		{
			"stereo ellipse",
			50, 22, TTAlways,
			20, 100, 0, 0, 0, 0, 0, 0,
			"// this sonifies the parametric equations for an ellipse." CR
			"// the x-coordinate is sent to the left output channel." CR
			"// the y-coordinate is sent to the right output channel." CR
			"// V0 and V1 can be used to control the radii of the ellipse" CR
			"a = V0+1;" CR
			"b = V1+1;" CR
			"// the cosine of t*128" CR
			"c = $((t+w/2)*128);" CR
			"// the sine of t*128;" CR
			"s = $(t*128);" CR
			"// assigning to [0] sets only the first output channel (left)" CR
			"[0] = a*c;" CR
			"// assigning to [1] sets only the second output channel (right)" CR
			"[1] = b*s;",
			// watches
			"a", "b", "c", "s", "", "", "", "", "", "",
		},

		{
			"sample and hold effect",
			100, 15, TTAlways,
			36, 0, 0, 0, 0, 0, 0, 0,
			"// this modifies the audio coming into the program" CR
			"// to create a sample and hold effect, with V0 controlling the intensity." CR
			"// s is how many samples to wait before grabbing a new value from the input" CR
			"s = V0 + 2;" CR
			"// r holds a 'bool' to indicate whether or not we should sample the input" CR
			"r = p<1 & p<t%s;" CR
			"p = t%s;" CR
			"// when r is 'true' we sample the input," CR
			"// otherwise we keep the value we already have" CR
			"a = r ? [0] : a;" CR
			"b = r ? [1] : b;" CR
			"// and now we output our result" CR
			"[0] = a; [1] = b;",
			// watches
			"V0", "s", "r", "a", "b", "", "", "", "", "",
		},

		{
			"oink oink ribbit",
			30, 18, TTProjectTime,
			17, 2, 45, 5, 3, 0, 0, 0,
			"// another example of using the previous value as part of the program," CR
			"// with several V knobs incorporated" CR
			"o = t*128;" CR
			"a = t*V0 >> V1;" CR
			"b = t - V2*100;" CR
			"c = b*64 | b*V3>>V4;" CR
			"p = (o | a) | c | p<<12;" CR
			"[*] = p;",
			// watches
			"a", "b", "c", "p", "V0", "V1", "V2", "V3", "V4", "",
		},

		{
			"rhythmic glitch sine",
			15, 15, TTAlways,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// programs have a large 'user' memory space that can be accessed with '@'." CR
			"// this allows us to store many previous values of the program." CR
			"// c is how many values to store (try changing this number)" CR
			"c = 1024*4;" CR
			"// s is where to save the current value" CR
			"s = t%c;" CR
			"// r is which previous value to read" CR
			"r = (t+512)%c;" CR
			"// we write into the address s and incorporate address r," CR
			"// but only if there is an active midi note." CR
			"// otherwise we 'reset' @s to w/2, which will produce silence" CR
			"@s = n>0 ? $(t*Fn) + @r : w/2;"
			"[*] = @s;",
			// watches
			"p", "r", "@p", "@r", "", "", "", "", "", "",
		},

		{
			"memory sequence",
			50, 16, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// another way to use 'user' memory could be to store values of a sequence." CR
			"// here we create an arpeggiator that uses the midi note as the root of a major chord." CR
			"@0 = 0; @1 = 4; @2 = 7; @3 = 12;" CR
			"// we want advance through the sequence every sixteenth note, wrapping around." CR
			"i = q/32 % 4;" CR
			"[*] = n>0 ? t*F(n+@i) : w/2;",
			// watches
			"i", "", "", "", "", "", "", "", "", "",
		},
	};

	const int kCount = sizeof(kPresets) / sizeof(Data);

	int Count() { return kCount; }

	const Data& Get(int idx) { return kPresets[idx]; }
};
