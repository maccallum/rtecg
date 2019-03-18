#include "rtecg_rand.h"
#include <stdlib.h>

uint32_t rtecg_def_rand(void)
{
	return (uint32_t)rand();
}

uint32_t (*_rtecg_rand)(void) = rtecg_def_rand;

uint32_t _rtecg_rand_max = RAND_MAX;

uint32_t rtecg_set_rand_max(uint32_t rm)
{
	_rtecg_rand_max = rm;
}

uint32_t rtecg_rand_max(void)
{
	return _rtecg_rand_max;
}

void rtecg_set_rand(uint32_t (*r)(void))
{
	_rtecg_rand = r;
}

uint32_t rtecg_rand(void)
{
	return _rtecg_rand();
}
