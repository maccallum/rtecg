#ifndef __RTECG_TIME_H__
#define __RTECG_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lib/libo/osc_timetag.h>

void rtecg_time_init(void);
int rtecg_time_wait(void);
int rtecg_time_tick(void);
t_osc_timetag rtecg_time_now(void);
t_osc_timetag rtecg_time_then(int nticks_in_past);

#ifdef __cplusplus
}
#endif

#endif
