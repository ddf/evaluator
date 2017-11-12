//
//  Presets.h
//  Evaluator
//
//  Created by Damien Quartz on 10/9/17
//
//

#pragma once
namespace Presets
{
	struct Data
	{
		const char * const name;
		const float volume;
		const int   bitDepth;
		const int   timeType;
		const int   V0, V1, V2, V3, V4, V5, V6, V7;
		const char * const program;
		const char * const W0, *const W1, *const W2, *const W3, *const W4, *const W5, *const W6, *const W7, *const W8, *const W9;
	};

	// how many pre-defined presets are there
	int Count();

	const Data& Get(int idx);
};

