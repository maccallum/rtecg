#ifndef __RTECG_PROCESS_H__
#define __RTECG_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

rtecg_float rtecg_pt_preprocess(rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int mwilen);

#ifdef __cplusplus
}
#endif

#endif
