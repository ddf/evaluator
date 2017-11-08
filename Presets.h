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
		const char * const program;
	};

	// how many pre-defined presets are there
	int Count();

	const Data& Get(int idx);
};

