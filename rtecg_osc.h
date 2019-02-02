#ifndef __RTECG_OSC_H__
#define __RTECG_OSC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <lib/libo/osc_timetag.h>

void rtecg_osc_init_pt(char *oscpfx, int pfxlen);
int rtecg_osc_wrap_pt(uint32_t packet_num,
		      t_osc_timetag time,
		      int32_t fs,
		      int32_t width,
		      int32_t raw,
		      int32_t filtered,
		      int32_t mwi,
		      uint32_t spkf_sample_num,
		      t_osc_timetag spkf_sample_time,
		      int32_t spkf_val,
		      float spkf_conf,
		      uint32_t spki_sample_num,
		      t_osc_timetag spki_sample_time,
		      int32_t spki_val,
		      float spki_conf,
		      float rr,
		      float rr_avg1,
		      float rr_avg2,
		      float f1,
		      float f2,
		      float i1,
		      float i2,
		      char **oscbndl);

#ifdef __cplusplus
}
#endif

#endif
