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
			"// t is automatically incremented before generating a sample frame" CR
			"// n is set to the most recent MIDI Note On value" CR
			"// F is a unary operator that converts its operand to a 'frequency'" CR
			"// F is implemented so that if you use it on a MIDI note number" CR
			"// the result will be a number that can be multiplied by t" CR
			"// to create an oscillator that has the same pitch" CR
			"// as the one the MIDI note number corresponds to if BIT is set to 15" CR
			"// increasing BIT will lower the octave, decreasing bit will raise the octave" CR
			"// since t increases linearly, this oscillator creates a saw wave" CR
			"t*Fn",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"square wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// # is a unary operator that creates a 'square' of its operand" CR
			"// it does this by taking the operand modulo w and" CR
			"// returning 0 if the result is less than w/2" CR
			"// or w-1 if greater than or equal to w/2" CR
			"// ie: #a == a%w < w/2 ? 0 : w-1" CR
			"// this lets us turn a saw wave into a square wave" CR
			"#(t*Fn)",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"sine wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// $ is a unary operator that returns the 'sine' of its operand" CR
			"// the 'sine' is calculated in relation to the current value of w" CR
			"// this lets us turn a saw wave into a sine wave" CR
			"$(t*Fn)",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"triangle wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// T is a unary operator that returns the 'triangle' of its operand" CR
			"// like $ and #, the operation uses the current value of w" CR
			"// this lets us easily turn a saw wave into a triangle wave" CR
			"T(t*Fn)",
			// watches
			"", "", "", "", "", "", "", "", "", "",
		},

		{
			"pulse wave",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// a pulse wave is a square wave that spends longer" CR
			"// on one of the two values every cycle" CR
			"// we use a modified version of the square formula to do this:" CR
			"// first we create our wrapped saw wave and save it in a variable" CR
			"// any lowercase letter can be used as a variable" CR
			"// and variable values persist from one sample to the next" CR
			"a = (t*Fn)%w;" CR
			"// we want the high part of the wave to last longer than the low part" CR
			"// so we use the less-than operator to create a 0 or 1 value" CR
			"b = a < 8800;" CR
			"// we then multiply w-1 by our 0 or 1 value to create the pulse wave" CR
			"(w-1)*b",
			// watches
			"a", "b", "", "", "", "", "", "", "", "",
		},

		{
			"ternary arp",
			50, 15, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// the ternary operator '?:' can be used to create expressions" CR
			"// that will resolve to one of two values" CR
			"// here we use it to modify n every other sixteenth note" CR
			"// q is incremented every 128th note,  so we can divide by 32" CR
			"// to get a number that increments every sixteenth note" CR
			"s = q/32;" CR
			"// if s is odd, we want to increase the note by an octave" CR
			"a = n + (s%2 ? 12 : 0);" CR
			"// finally, we can create our oscillator" CR
			"t*Fa",
			// watches
			"s", "a", "", "", "", "", "", "", "", "",
		},

		{
			"frequency modulation",
			50, 15, TTWithNoteResetting,
			2, 0, 0, 0, 0, 0, 0, 0,
			"// a saw wave can be smoothly frequency modulated" CR
			"// by adding a sine wave to it." CR
			"// here we use the value of the V0 knob to control" CR
			"// the speed of the frequency modulation" CR
			"t*Fn + $(t*V0)",
			// watches
			"V0", "", "", "", "", "", "", "", "", "",
		},

		{
			"amplitude modulation",
			50, 15, TTAlways,
			12, 0, 0, 0, 0, 0, 0, 0,
			"// to amplitude modulate we need to scale the range up and down," CR 
			"// but keep the values 'centered' around w/2." CR 
			"// so we wrap to [0, w) before scaling with our modulator" CR
			"o = (t*Fn) % w;" CR
			"// we step the phase of our modulator forward every 8 milliseconds" CR
			"// which lets us smoothly control the frequency with the V0 knob" CR
			"p = m % 8 ? p : p + V0 * 2;" CR
			"// if there is no note on, we reset the phase so that $p will be 0" CR
			"p = n < 1 ? 3 * w / 4 : p;" CR
			"m = $p;" CR
			"// m is a [0,w) value, so we decrease o by that ratio" CR
			"o = o * m/w;" CR
			"// then to center, we offset by how much w/2 is changed by this ratio" CR
			"o = o + ((w/2) - (w/2) * m/w);",
			// watches
			"V0", "o", "p", "", "", "", "", "", "", "",
		},

		{
			"little ditty",
			50, 15, TTProjectTime,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// here we make a surprising rhythmic ditty by combining three signals:" CR
			"// a frequency modulated saw wave (try changing the frequency of modulation)" CR
			"a = (t * 128 + $(t));" CR
			"// >> is a bit-shift operator that shifts all bits to right" CR
			"// the number of bits indicated by the right hand side." CR
			"// the right hand side is wrapped to 64 before the shift," CR
			"// which means you have very large values there and still get useful results." CR
			"// try changing the hard-coded 8 in here." CR
			"b = t >> (t % (8 * w)) / w;" CR
			"// for example, this line is the same as c = t;" CR
			"c = t >> 128;" CR
			"// finally, we use bitwise OR to combine the three values." CR
			"// this is similar to summing two floating point signals." CR
			"a | b | c",
			// watches
			"a", "b", "c", "", "", "", "", "", "", "",
		},

		{
			"overtone waterfall",
			50, 17, TTAlways,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// another example of combining multiple signals with bitwise OR" CR
			"// first a vanilla low frequency saw" CR
			"a = t*128;" CR
			"// then a saw that descends through the overtone series" CR
			"// changing pitch every 50 milliseconds" CR
			"b = a*(32-(m/50)%32);" CR
			"// and then a saw that ascends through the overtone series" CR
			"// changing pitch every 100 milliseconds" CR
			"c = a*((m/100)%64);" CR
			"a | b | c",
			// watches
			"a", "b", "c", "", "", "", "", "", "", "",
		},

		{
			"computer music" ,
			50, 11, TTWithNoteResetting,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// here we create a random sounding note sequence" CR
			"// but based on the incoming MIDI note" CR
			"// s is how often the note changes - every 125 milliseconds" CR
			"s = m/125;" CR
			"a = 7*(s%3); // 0, 7, 14, 21" CR
			"b = 3*(s%5); // 0, 3, 6, 9, 12" CR
			"c = 2*(s%7); // 0, 2, 4, 6, 8, 10, 12" CR
			"$(t*F(n + a - b + c))",
			// watches
			"s", "a", "b", "c", "", "", "", "", "", "",
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
			"(t*64 + a*s) | t*32",
			// watches
			"s", "a", "", "", "", "", "", "", "", "",
		},

		// this maybe takes too long to get interesting and is just unpleasant noise mostly, might want to ditch it.
		{
			"blurp",
			50, 15, TTAlways,
			0, 0, 0, 0, 0, 0, 0, 0,
			"// << shifts the bits of the left hand side" CR 
			"// by the number of the right hand side modulo 64" CR
			"a = t<<t/(1024*8);" CR
			"b = t>>t/16;" CR
			"c = t>>t/32;" CR
			"// & is bitwise AND" CR
			"e = (a | b & c);" CR
			"// we add 1 in two places here to prevent divide-by-zero errors" CR
			"d = t%(t/512+1) + 1;" CR
			"// remember that a is divided by d and then multiplied by 32" CR
			"e/d * 32",
			// watches
			"a", "b", "c", "d", "e", "", "", "", "", "",
		},

		{
			"garbage trash",
			50, 15, TTAlways,
			16, 0, 0, 0, 0, 0, 0, 0,
			"// let this one run for a while, it subtly changes over time" CR
			"// r will increase by one every V0+1 milliseconds" CR
			"// we add one to V0 to prevent divide-by-zero" CR
			"r = m/(V0+1);" CR
			"// s will cycle through [0,15] at the same rate" CR
			"s = r%16;" CR
			"// note here that due to operator precedence" CR
			"// t is multiplied by s and then the modulo is applied" CR
			"(256*s + t*s%(512*s+1)) * r",
			// watches
			"r", "s", "", "", "", "", "", "", "", "",
		},
		//{ "nonsense can", "p = (1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", 50, 15, TTAlways },
		//{ "ellipse", "(m/250+1)*$(t*128) | (m/500+1)*$((t+w/2*128))", 50, 12, TTAlways },
		//{ "moving average", "p = p + ( ((t+1)*256 ^ (t+1)*64 & (t+1)*32) - p)/(t+1)", 50, 15, TTAlways },
		//{ "oink oink ribbit", "p = (t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", 30, 18, TTProjectTime },
		//{ "rhythmic glitch sine", "p = $(t*Fn) | t*n/10>>4 ^ p>>(m/250%12)", 15, 13, TTAlways }
	};

	const int kCount = sizeof(kPresets) / sizeof(Data);

	int Count() { return kCount; }

	const Data& Get(int idx) { return kPresets[idx]; }
};
