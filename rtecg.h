#ifndef __RTECG_H__
#define __RTECG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef uint32_t rtecg_ctr;
typedef int rtecg_int;
typedef float rtecg_float;

#define RTECG_FS 200
#define RTECG_MS_PER_SAMP (1000. / (double)RTECG_FS)
#define RTECG_MTOS(ms)((rtecg_int)(((ms) / RTECG_MS_PER_SAMP) + 0.5))
#define RTECG_STOM(samps)((rtecg_int)(samps * RTECG_MS_PER_SAMP))

// buffer lengths
//#define RTECG_LPBUFLEN ((RTECG_MTOS(25)) * 2) // low pass buffer length
#define RTECG_LPBUFLEN 12
//#define RTECG_HPBUFLEN RTECG_MTOS(125) // high pass buffer length
#define RTECG_HPBUFLEN 32
//#define RTECG_DERIVLEN RTECG_MTOS(10)  // derivative length
#define RTECG_DERIVLEN 4
//#define RTECG_MWILEN RTECG_MTOS(80) // moving window integration length
#define RTECG_MWILEN RTECG_MTOS(150)
//#define RTECG_PKWINLEN ((RTECG_MTOS(165)) | 1) // length of window to search for peaks in
#define RTECG_PKWINLEN ((RTECG_MTOS(100)) | 1) // length of window to search for peaks in
#define RTECG_PKKNEIGH (((RTECG_PKWINLEN) - 1) / 2) // number of samples to the left and right of candidate peak
#define RTECG_PTBUFLEN (RTECG_MTOS(2000)) // number of candidate peaks to keep

// filter delays incurred at each stage of processing
//#define RTECG_LPDEL (((RTECG_LPBUFLEN) / 2) - 1) // number of samples delay for lp filter
#define RTECG_LPDEL 5
//#define RTECG_HPDEL (((RTECG_HPBUFLEN) - 1) / 2) // number of samples delay for hp filter
#define RTECG_HPDEL 16
//#define RTECG_DERIVDEL ((RTECG_DERIVLEN) / 2) // number of samples delay for derivative filter
#define RTECG_DERIVDEL 2
#define RTECG_MWIDEL RTECG_MWILEN / 2 // number of samples delay for moving window integrator
#define RTECG_PKDEL RTECG_PKKNEIGH + 1 // number of samples delay for peak window

// burn in 
#define RTECG_PREBURNLEN RTECG_MTOS(500)
#define RTECG_BURNLEN RTECG_MTOS(2500) // burn in length


#ifdef __cplusplus
}
#endif

#endif
