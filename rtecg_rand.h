#ifndef __RTECG_RAND_H__
#define __RTECG_RAND_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

uint32_t rtecg_set_rand_max(uint32_t rm);
uint32_t rtecg_rand_max(void);
void rtecg_set_rand(uint32_t (*r)(void));
uint32_t rtecg_rand(void);


#ifdef __cplusplus
}
#endif

#endif
