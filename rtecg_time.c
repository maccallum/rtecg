#include <rtecg.h>
#include <rtecg_time.h>
#include <lib/libo/osc_timetag.h>
#include <inttypes.h>

static uint32_t micros_ival = 1000000. / RTECG_FS;
static uint32_t micros_prev = 0;
//static uint32_t micros_ref = 0;
static t_osc_timetag current_date = {0, 0};
static t_osc_timetag tlst[RTECG_FS * 2];
static int tptr = 0;

void rtecg_time_init(void)
{
	memset(tlst, 0, sizeof(t_osc_timetag) * (RTECG_FS * 2));
	//micros_ref =
	micros_prev = micros();
}

void rtecg_time_wait(void)
{
	uint32_t micros_cur = micros();
	while(micros_cur - micros_prev < micros_ival){
		micros_cur = micros();
	}
	micros_prev = micros_cur;
}

int rtecg_time_tick(void)
{
	int ret = 0;
	t_osc_timetag prev = current_date;
	t_osc_timetag now = osc_timetag_floatToTimetag(micros_prev / 1000000.);
	if(prev.sec != now.sec){
		ret = 1;
	}
	current_date = now;
	tlst[tptr++] = now;
	if(tptr == (RTECG_FS * 2)){
		tptr = 0;
	}
	return ret;
}

t_osc_timetag rtecg_time_now(void)
{
	// t_osc_timetag now = current_date;
	// now.frac_sec = (uint32_t)(0xFFFFFFFFUL * ((micros() - micros_ref) / 1000000.));
	// return now;
	t_osc_timetag now = osc_timetag_floatToTimetag(micros() / 1000000.);
}

t_osc_timetag rtecg_time_then(int nticks_in_past)
{
	int idx = tptr - nticks_in_past - 1;
	while(idx < 0){
		idx += (RTECG_FS * 2);
	}
	return tlst[idx];
}
