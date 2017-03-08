#ifndef __RTECG_PANTOMPKINS_H__
#define __RTECG_PANTOMPKINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtecg.h"

#define RTECG_PT_HISTLEN 24 // number of candidate peaks to keep around

// the confidence value is crude at this point:
// 1:		peakf >= f1 && peaki >= i1
// .666:	peakf >= f2 && peaki >= i2 (searchback)
// .333:	peakf >= f2 || peaki >= i2 (searchback)
// 0: 		peakf < f2 && peaki < i2 (searchback)
typedef struct _rtecg_spk
{
	rtecg_int x;
	rtecg_int y;
	rtecg_int maxslope;
	rtecg_float confidence;
} rtecg_spk;

typedef struct _rtecg_pt
{
	rtecg_float spkf, spki, npkf, npki, f1, f2, i1, i2;
	rtecg_float tspkf, tspki, tnpkf, tnpki, tf1, tf2, ti1, ti2;
	rtecg_spk pkf[RTECG_PT_HISTLEN], pki[RTECG_PT_HISTLEN];
	rtecg_int ptrf, tptrf, ptri;
	rtecg_ctr ctr;
	rtecg_spk last_spkf, last_spki, last_last_spkf, last_last_spki;
	rtecg_int havepeak;
	rtecg_float rr;
	rtecg_float rrbuf1[8], rrbuf2[8];
	rtecg_int rrptr1, rrptr2;
	rtecg_float rrsum1, rrsum2;
	rtecg_float rravg1, rravg2;
	rtecg_int burn_avg1, burn_avg2;
	rtecg_int havefirstpeak;
	rtecg_int searchback;
} rtecg_pt;

rtecg_pt rtecg_pt_init(void);
rtecg_pt rtecg_pt_process(rtecg_pt s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei);
rtecg_pt rtecg_pt_searchback(rtecg_pt s);
rtecg_spk rtecg_pt_last_spkf(rtecg_pt s);
rtecg_spk rtecg_pt_last_spki(rtecg_pt s);

#ifdef __cplusplus
}
#endif

#endif
