#pragma once

#include "main.h"


//------------------------------------------------------------------------------
void
timer_start(timer_st *timer)
{
	memset(timer, 0, sizeof(*timer));
	timer->start = clock();
}

void
timer_stop(timer_st *timer)
{
	timer->stop = clock();
}


//------------------------------------------------------------------------------
void
timer_diff_print(timer_st *timer)
{
	// just in case...
	if (   0 == timer->stop
		|| 0 == timer->start
		|| timer->stop < timer->start)
	{
		printf("\nERR: TIMER: invalid timer start and/or stop time\n\n");
		fflush(stdout);
		return;
	}

	//--------------------------------------------------------------
	clock_t diff = timer->stop - timer->start;
	int     msec = diff * 1000 / CLOCKS_PER_SEC;	// milliseconds

	printf("%ds, %dms", msec/1000, msec%1000);
}
