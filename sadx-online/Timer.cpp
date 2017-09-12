#include "stdafx.h"
#include "Timer.h"

Timer::Timer(clock::duration dur, bool start_now)
	: Stopwatch(start_now),
	  duration(dur)
{
}

bool Timer::done() const
{
	return elapsed() > duration;
}
