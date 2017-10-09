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
		const char * const program;
		const float volume;
		const int   bitDepth;
	};

	// how many pre-defined presets are there
	int Count();

	const Data& Get(int idx);
};

