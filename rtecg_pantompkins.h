#ifndef __RTECG_PANTOMPKINS_H__
#define __RTECG_PANTOMPKINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtecg.h"

#define RTECG_PT_HISTLEN 4096 // number of candidate peaks to keep around
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

typedef struct _rtecg_spk
{
	rtecg_int x;
	rtecg_int y;
	rtecg_int maxslope;
} rtecg_spk;
	
typedef struct _rtecg_pt2
{
	rtecg_float spkf, spki, npkf, npki, f1, f2, i1, i2;
	rtecg_float tspkf, tspki, tnpkf, tnpki, tf1, tf2, ti1, ti2;
	rtecg_spk pkf[RTECG_PT_HISTLEN], pki[RTECG_PT_HISTLEN];
	rtecg_int ptrf, tptrf, ptri;
	rtecg_ctr ctr;
	rtecg_spk last_spkf, last_spki;
	rtecg_int havepeak;
} rtecg_pt2;

rtecg_pt2 rtecg_pt2_init(void);
rtecg_pt2 rtecg_pt2_process(rtecg_pt2 s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei);
rtecg_spk rtecg_pt2_last_spkf(rtecg_pt2 s);
rtecg_spk rtecg_pt2_last_spki(rtecg_pt2 s);

#ifdef __cplusplus
}
#endif

#endif
