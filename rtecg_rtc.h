#ifndef __RTECG_RTC_H__
#define __RTECG_RTC_H__

// #ifdef __cplusplus
// extern "C" {
// #endif

#include <lib/libo/osc_timetag.h>

void rtecg_rtc_init(int rtc_square_wave_pin);
int rtecg_rtc_wait(void);
int rtecg_rtc_tick(void);
t_osc_timetag rtecg_rtc_now(void);
t_osc_timetag rtecg_rtc_then(int nticks_in_past);

// #ifdef __cplusplus
// }
// #endif

#endif
