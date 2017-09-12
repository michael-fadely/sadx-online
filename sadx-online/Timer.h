#pragma once
#include "Stopwatch.h"

class Timer : public Stopwatch
{
public:
	clock::duration duration;

	explicit Timer(clock::duration dur, bool start_now = false);

	bool done() const;
};
