#ifndef __RTECG_H__
#define __RTECG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef double rtecg_float;
typedef uint32_t rtecg_ctr;
typedef int rtecg_int;

#define RTECG_FS 200
#define RTECG_MS_PER_SAMP (1000. / (double)RTECG_FS)
#define RTECG_MTOS(ms)((rtecg_int)(((ms) / RTECG_MS_PER_SAMP) + 0.5))
#define RTECG_STOM(samps)((rtecg_int)(samps * RTECG_MS_PER_SAMP))

// buffer lengths
#define RTECG_LPBUFLEN ((RTECG_MTOS(25)) * 2) // low pass buffer length
#define RTECG_HPBUFLEN RTECG_MTOS(125) // high pass buffer length
#define RTECG_DERIVLEN RTECG_MTOS(10)  // derivative length
#define RTECG_MWILEN RTECG_MTOS(80) // moving window integration length
#define RTECG_PKWINLEN ((RTECG_MTOS(165)) | 1) // length of window to search for peaks in
#define RTECG_PKKNEIGH (((RTECG_PKWINLEN) - 1) / 2) // number of samples to the left and right of candidate peak
// delays incurred at each stage of processing
#define RTECG_LPDEL (((RTECG_LPBUFLEN) / 2) - 1) // number of samples delay for lp filter
#define RTECG_HPDEL (((RTECG_HPBUFLEN) - 1) / 2) // number of samples delay for hp filter
#define RTECG_DERIVDEL ((RTECG_DERIVLEN) / 2) // number of samples delay for derivative filter
#define RTECG_MWIDEL RTECG_MWILEN // number of samples delay for moving window integrator
#define RTECG_PKDEL RTECG_PKKNEIGH + 1 // number of samples delay for peak window

// various values
#define RTECG_BURNLEN RTECG_MTOS(6000) // burn in length
// have to add search back, debounce

#ifdef __cplusplus
}
#endif

#endif
