//
//  Presets.cpp
//  Evaluator
//
//  Created by Damien Quartz on 10/9/17
//
//

#include "Presets.h"

namespace Presets
{
	const Data kPresets[] = 
	{
		// name, expression, volume, bit depth
		{ "saw wave", "t*Fn", 50, 15 },
		{ "square wave", "#(t*Fn)", 50, 15 },
		{ "sine wave", "$(t*Fn)", 50, 15 },
		{ "triangle wave", "T(t*Fn)", 50, 13 },
		{ "amplitude modulation", "t*Fn | $(t)", 50, 15 },
		{ "frequency modulation", "t*Fn + $(t*2)", 50, 15 },
		{ "bouncing balls", "$(t*(1000 - m%500))", 50, 15 },
		{ "little ditty", "(t*128 + $(t)) | t>>(t%(8*r))/r | t>>128", 50, 15 },
		{ "aggressive texture", "(t*64 + $(t^$(m/2000))*$(m/2000)) | t*32", 50, 15 },
		{ "overtone waterfall", "t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", 50, 17 },
		{ "computer music" , "$(t*F(n + 7*((m/125)%3) - 3*((m/125)%5) + 2*((m/125)%7)))", 50, 11 },
		{ "blurp", "(t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32", 50, 15 },
		{ "garbage trash", "(r/2 - (256*(m/16%16)) + (t*(m/16%16)%(512*(m/16%16)+1))) * (m/16)", 50, 15 },
		{ "nonsense can", "p = (1 + $(m)%32) ^ (t*128 & t*64 & t*32) | (p/16)<<p%4 | $(p/128)>>p%4", 50, 15 },
		{ "ellipse", "(m/250+1)*$(t*128) | (m/500+1)*$((t+r/2*128))", 50, 12 },
		{ "moving average", "p = p + ( ((t+1)*256 ^ (t+1)*64 & (t+1)*32) - p)/(t+1)", 50, 15 },
		{ "oink oink ribbit", "p = (t*128 | t*17>>2) | ((t-4500)*64 | (t-4500)*5>>3) | p<<12", 30, 18 },
		{ "rhythmic glitch sine", "p = $(t*Fn) | t*n/10>>4 ^ p>>(m/250%12)", 15, 13 }
	};

	const int kCount = sizeof(kPresets) / sizeof(Data);

	int Count() { return kCount; }

	const Data& Get(int idx) { return kPresets[idx]; }
};
