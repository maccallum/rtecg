#include "rtecg.h"
#include "rtecg_pantompkins.h"
#include <string.h>
#include <stdio.h>

rtecg_pt rtecg_pt_init(void)
{
	rtecg_pt s;
	memset(&s, 0, sizeof(rtecg_pt));
	return s;
}

rtecg_pt rtecg_pt_process(rtecg_pt s,
			  rtecg_float *filt,
			  rtecg_float *filt_peaks,
			  rtecg_ctr filtpos_r,
			  rtecg_ctr filtlen,
			  rtecg_float *mwi,
			  rtecg_float *mwi_peaks,
			  rtecg_ctr mwipos_r,
			  rtecg_ctr mwilen)
{
	printf("**************************************************\n");
	printf("%s(%d): filtpos = %u, mwipos = %u\n", __func__, __LINE__, filtpos_r, mwipos_r);
	int found_spk = 0;
	if(filt_peaks[filtpos_r % filtlen]){
		printf("%s(%d): filt peak\n", __func__, __LINE__);
		if(filt[(filtpos_r - 17) % filtlen] < s.f2){
			printf("%s(%d): %f < s.f2 (%f), bail\n", __func__, __LINE__, filt[(filtpos_r - 17) % filtlen], s.f2);
			// if peak in the filtered signal is less than the second threshold, classify it as a noise peak
			s.npkf = .125 * filt[(filtpos_r - 17) % filtlen] + .875 * s.npkf;
		}else{
			printf("%s(%d): adding to list\n", __func__, __LINE__);
			// if peak in the filtered signal is greater than or equal to the second threshold,
			// add it to the list of potential peaks
			s.filt_peaks[s.filt_peaks_pos % RTECG_PT_HISTLEN] = filtpos_r;
			s.filt_peaks_pos++;
		}
	}
	if(mwi_peaks[mwipos_r % mwilen]){
		printf("%s(%d): mwi peak\n", __func__, __LINE__);
		if(mwi[(mwipos_r - 17) % mwilen] < s.i2){
			printf("%s(%d): %f < s.f2 (%f), bail\n", __func__, __LINE__, mwi[(mwipos_r - 17) % mwilen], s.f2);
			// if peak in the mwi signal is less than the second threshold, classify it as a noise peak
			s.npki = .125 * mwi[(mwipos_r - 17) % mwilen] + .875 * s.npki;
		}else{
			printf("%s(%d): adding to list\n", __func__, __LINE__);
			// add to the list of candidate peaks
			s.mwi_peaks[s.mwi_peaks_pos % RTECG_PT_HISTLEN] = mwipos_r;
			s.mwi_peaks_pos++;
			
			// calculate temporary values for spkf and spki as if the list of candidate peaks had been considered noise
			rtecg_float npkf = s.npkf;
			rtecg_float npki = s.npki;
			rtecg_float f1 = s.f1, i1 = s.i1;
			for(rtecg_ctr i = 0; i < s.filt_peaks_pos - 1; i++){
				npkf = 0.125 * filt[s.filt_peaks[i] - 17] + .875 * npkf;
				f1 = npkf + .25 * (s.spkf - npkf);
			}
			for(rtecg_ctr i = 0; i < s.mwi_peaks_pos - 1; i++){
				npki = 0.125 * mwi[s.mwi_peaks[i] - 17] + .875 * npki;
				i1 = npki + .25 * (s.spki - npki);
			}
			if(mwi[(mwipos_r - 17) % mwilen] >= i1){
				printf("%s(%d): mwi[(mwipos_r - 17) % mwilen] (%f) >= i1 (%f)\n", __func__, __LINE__, mwi[(mwipos_r - 17) % mwilen], i1);
				// if peak in the mwi signal is greater than or equal to the second threshold,
				// consider it a signal peak if there is a signal peak within xxx samples in the
				// filtered signal. if not, add it to a list of potential peaks in case we have to search back
				if(s.filt_peaks_pos > 0){
					printf("%s(%d): there are filter peaks\n", __func__, __LINE__);
					if(filt[s.filt_peaks[(s.filt_peaks_pos - 1) % RTECG_PT_HISTLEN] - 17] >= f1 &&
					   mwipos_r - s.filt_peaks[(s.filt_peaks_pos - 1) % RTECG_PT_HISTLEN] <= RTECG_PT_MAX_DIST_BTN_MWI_AND_FILT){
						printf("%s(%d)\n", __func__, __LINE__);
						// this is a signal peak---update thresholds
						s.npkf = npkf;
						s.npki = npki;
						s.spkf = .125 * filt[s.filt_peaks[(s.filt_peaks_pos - 1) % RTECG_PT_HISTLEN]] + .875 * s.spkf;
						s.spki = .125 * mwi[s.mwi_peaks[(s.mwi_peaks_pos - 1) % RTECG_PT_HISTLEN]] + .875 * s.spki;
						f1 = s.npkf + .25 * (s.spkf - s.npkf);
						i1 = s.npki + .25 * (s.spki - s.npki);
						s.last_spkf = s.filt_peaks[(s.filt_peaks_pos - 1) % RTECG_PT_HISTLEN];
						s.last_spki = s.mwi_peaks[(s.mwi_peaks_pos - 1) % RTECG_PT_HISTLEN];
						s.filt_peaks_pos = 0;
						s.mwi_peaks_pos = 0;
					}else{
						printf("%s(%d)\n", __func__, __LINE__);
						// this mwi peak is greater or equal to i2, so keep it around in case we have to search back
						s.mwi_peaks[s.mwi_peaks_pos % RTECG_PT_HISTLEN] = mwipos_r;
						s.mwi_peaks_pos++;
					}
				}else{
					printf("up in ya\n");
					// this is a peak in the mwi, but we haven't had a candidate in the filtered signal, so it must be noise
					s.npki = .125 * mwi[mwipos_r] + .875 * s.npki;
				}
			}else{
				printf("%s(%d)\n", __func__, __LINE__);
				// this mwi peak is greater or equal to i2, so keep it around in case we have to search back
				s.mwi_peaks[s.mwi_peaks_pos % RTECG_PT_HISTLEN] = mwipos_r;
				s.mwi_peaks_pos++;
			}
		}
	}
	if(!found_spk){
		// check to see if we need to search back
	}
	return s;
}
