//
//  Presets.cpp
//  Evaluator
//
//  Created by Damien Quartz on 10/9/17
//
//

#include "Presets.h"
#include "Evaluator.h" // for the TimeType enum

namespace Presets
{
#if SA_API
	const TimeType TTProjectTime = TTAlways;
#endif

	const Data kPresets[] =
	{
		// name, expression, volume, bit depth
		//{ "test", "a = n<60 ? t*Fn : #(t*Fn); [0] = [1] = a;", 50, 15, TTWithNoteResetting },
		{ "saw wave", "t*Fn", 50, 15, TTWithNoteResetting },
		{ "square wave", "#(t*Fn)", 50, 15, TTWithNoteResetting },
		{ "sine wave", "$(t*Fn)", 50, 15, TTWithNoteResetting },
		{ "triangle wave", "T(t*Fn)", 50, 13, TTWithNoteResetting },
		{ "pulse wave", "(w-1)*((t*Fn)%r < 8800)", 50, 15, TTWithNoteResetting },
		{ "ternary arp", "q%64/32 ? t*Fn : t*F(n+12) * (n>0)", 50, 15, TTWithNoteResetting },
		{ "amplitude modulation", "t*Fn | $(t)", 50, 15, TTWithNoteResetting },
		{ "frequency modulation", "t*Fn + $(t*2)", 50, 15, TTWithNoteResetting },
		{ "bouncing balls", "$(t*(1000 - m%500))", 50, 15, TTAlways },
		{ "little ditty", "(t*128 + $(t)) | t>>(t%(8*w))/w | t>>128", 50, 15, TTProjectTime },
		{ "aggressive texture", "(t*64 + $(t^$(m/2000))*$(m/2000)) | t*32", 50, 15, TTProjectTime },
		{ "overtone waterfall", "t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", 50, 17, TTAlways },
		{ "computer music" , "$(t*F(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))", 50, 11, TTAlways },
		{ "blurp", "(t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32", 50, 15, TTAlways },
		{ "garbage trash", "(w/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)", 50, 15, TTAlways },
		{ "nonsense can", "p = (1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", 50, 15, TTAlways },
		{ "ellipse", "(m/250+1)*$(t*128) | (m/500+1)*$((t+w/2*128))", 50, 12, TTAlways },
		{ "moving average", "p = p + ( ((t+1)*256 ^ (t+1)*64 & (t+1)*32) - p)/(t+1)", 50, 15, TTAlways },
		{ "oink oink ribbit", "p = (t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", 30, 18, TTProjectTime },
		{ "rhythmic glitch sine", "p = $(t*Fn) | t*n/10>>4 ^ p>>(m/250%12)", 15, 13, TTAlways }
	};

	const int kCount = sizeof(kPresets) / sizeof(Data);

	int Count() { return kCount; }

	const Data& Get(int idx) { return kPresets[idx]; }
};
