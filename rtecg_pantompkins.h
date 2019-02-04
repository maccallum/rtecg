#ifndef __RTECG_PANTOMPKINS_H__
#define __RTECG_PANTOMPKINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtecg.h"
#include <stddef.h>

// the confidence value is crude at this point:
// 1: peak was found normally
// .75: peak was found in searchback and was above f1
// .5: peak was found in searchback and was above f2
// .25: peak was found in searchback and was below f2
// 0: no peak was found
typedef struct _rtecg_spk
{
	rtecg_ctr x;
	rtecg_int y;
	rtecg_int maxslope;
	rtecg_float confidence;
} rtecg_spk;

//#define RTECG_PT_LARGE_DROP_THRESH1 1.6
//#define RTECG_PT_LARGE_DROP_THRESH2 1 / 1.4

typedef struct _rtecg_pt
{
	rtecg_float spkf, spki, npkf, npki, f1, f2, i1, i2;
	rtecg_float tspkf, tspki, tnpkf, tnpki, tf1, tf2, ti1, ti2;
	rtecg_spk *pkf, *pki;//pkf[RTECG_PT_HISTLEN], pki[RTECG_PT_HISTLEN];
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
	rtecg_int rravg2_missed_ctr;
	rtecg_int havefirstpeak;
	rtecg_int searchback;
	//rtecg_int large_drop_event;
	//rtecg_int large_drop_event_from, large_drop_event_to;
} rtecg_pt;

rtecg_pt rtecg_pt_init(void);
rtecg_pt rtecg_pt_reset(rtecg_pt s);
rtecg_pt rtecg_pt_process(rtecg_pt s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei, char *buf, size_t buflen, int bufptr);
rtecg_pt rtecg_pt_searchback(rtecg_pt s, char *buf, size_t buflen, int bufptr);
rtecg_spk rtecg_pt_last_spkf(rtecg_pt s);
rtecg_spk rtecg_pt_last_spki(rtecg_pt s);

#ifdef __cplusplus
}
#endif

#endif
