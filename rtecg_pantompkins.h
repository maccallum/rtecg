#ifndef __RTECG_PANTOMPKINS_H__
#define __RTECG_PANTOMPKINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtecg.h"

#define RTECG_PT_HISTLEN 64 // number of candidate peaks to keep around
#define RTECG_PT_MAX_DIST_BTN_MWI_AND_FILT 40 // maximum number of samples between a candidate peak in filt and mwi
typedef struct _rtecg_pt
{
	rtecg_float spkf, spki, npkf, npki, f1, f2, i1, i2;
	rtecg_ctr last_spkf, last_spki;
	rtecg_ctr filt_peaks[RTECG_PT_HISTLEN], mwi_peaks[RTECG_PT_HISTLEN];
	int filt_peaks_pos, mwi_peaks_pos;
} rtecg_pt;

rtecg_pt rtecg_pt_init(void);
rtecg_pt rtecg_pt_process(rtecg_pt s,
			  rtecg_float *filt,
			  rtecg_float *filt_peaks,
			  rtecg_ctr filtpos_r,
			  rtecg_ctr filtlen,
			  rtecg_float *mwi,
			  rtecg_float *mwi_peaks,
			  rtecg_ctr mwipos_r,
			  rtecg_ctr mwilen);

#ifdef __cplusplus
}
#endif

#endif
