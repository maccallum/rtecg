#ifndef __RTECG_H__
#define __RTECG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#ifndef RTECG_BUFLEN
#define RTECG_BUFLEN 256
#endif
#ifndef RTECG_FS
#define RTECG_FS 256
#endif
#ifndef RTECG_MWILEN
#define RTECG_MWILEN 38
#endif
#ifndef RTECG_DELAY
#define RTECG_DELAY 52
#endif

typedef double rtecg_float;
typedef uint32_t rtecg_ctr;

typedef struct _rtecg
{
	rtecg_float raw[RTECG_BUFLEN];
	rtecg_float h[RTECG_BUFLEN];
	rtecg_float mwi[RTECG_BUFLEN];
	rtecg_float raw_peaks[RTECG_BUFLEN];
	rtecg_float mwi_peaks[RTECG_BUFLEN];
	rtecg_ctr rawp, hp, dsqp, mwip, raw_peaksp, mwi_peaksp; // buffer positions
	rtecg_float rawm, mwim, rawS, mwiS; // running mean and sum for peak detector
	int peak_win_size_raw, peak_win_size_mwi;
	rtecg_float stdevx_raw, stdevx_mwi;
	rtecg_float spki, npki, spkf, npkf;
} rtecg;

#ifdef __cplusplus
}
#endif

#endif
