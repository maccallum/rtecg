#include "rtecg.h"
#include "rtecg_pantompkins.h"
#include <string.h>
#include <stdio.h>

//#define RTECG_PT_PRINTSTATE
#ifdef RTECG_PT_PRINTSTATE
#define pl()printf("**************************************************\n");
#define pd(fmt, ...)printf("%s(%d): "fmt, __func__, __LINE__, __VA_ARGS__);
#define ps(s)printf("%s(%d): STATE:\n\tspkf = %f\n\tnpkf = %f\n\tspki = %f\n\tnpki = %f\n\tf1 = %f\n\tf2 = %f\n\ti1 = %f\n\ti2 = %f\n\tlast_spkf = %u\n\tlast_spki = %u\n\tfilt_peaks(%d): [%u, %u, %u, %u, %u, %u]\n\tmwi_peaks(%d): [%u, %u, %u, %u, %u, %u]\n", __func__, __LINE__, s.spkf, s.npkf, s.spki, s.npki, s.f1, s.f2, s.i1, s.i2, s.last_spkf, s.last_spki, s.filt_peaks_pos, s.filt_peaks[0], s.filt_peaks[1], s.filt_peaks[2], s.filt_peaks[3], s.filt_peaks[4], s.filt_peaks[5], s.mwi_peaks_pos, s.mwi_peaks[0], s.mwi_peaks[1], s.mwi_peaks[2], s.mwi_peaks[3], s.mwi_peaks[4], s.mwi_peaks[5]);
#else
#define pl()
#define pd(fmt, ...)
#define ps(s)
#endif

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
	pl();
	pd("pos = %u, %d %d\n", filtpos_r, s.filt_peaks_pos, s.mwi_peaks_pos);
	// calculate temporary values for spkf and spki as if the list of candidate peaks had been considered noise
	rtecg_float npkf = s.npkf;
	rtecg_float npki = s.npki;
	rtecg_float f1 = s.f1, i1 = s.i1;
	for(int i = 0; i < s.filt_peaks_pos - 1; i++){
		npkf = 0.125 * filt[s.filt_peaks[i] - 17] + .875 * npkf;
		f1 = npkf + .25 * (s.spkf - npkf);
	}
	for(int i = 0; i < s.mwi_peaks_pos - 1; i++){
		npki = 0.125 * mwi[s.mwi_peaks[i] - 17] + .875 * npki;
		i1 = npki + .25 * (s.spki - npki);
	}
	int found_spk = 0;
	if(filt_peaks[filtpos_r % filtlen]){
		pd("FILT: filt_peaks <- %u\n", filtpos_r);
		// if there is a peak in the filter signal, add it to the list to be compared with an mwi peak
		s.filt_peaks[s.filt_peaks_pos % RTECG_PT_HISTLEN] = filtpos_r;
		s.filt_peaks_pos++;
	}
	if(mwi_peaks[mwipos_r % mwilen]){
		pd("MWI: mwi_peaks <- %u\n", mwipos_r);
		s.mwi_peaks[s.mwi_peaks_pos % RTECG_PT_HISTLEN] = mwipos_r;
		s.mwi_peaks_pos++;
		if(mwi[(mwipos_r - 17) % mwilen] >= i1){
			pd("MWI: %f >= %f\n", mwi[(mwipos_r - 17) % mwilen], i1);
			// if peak in the mwi signal is greater than or equal to the second threshold,
			// consider it a signal peak if there is a signal peak within xxx samples in the
			// filtered signal. if not, add it to a list of potential peaks in case we have to search back
			if(s.filt_peaks_pos > 0){
				pd("%s\n", "looking for filter peak that corresponds to mwi peak\n");
				for(int i = (s.filt_peaks_pos - 1) % RTECG_PT_HISTLEN; i >= 0; i--){
					if(mwipos_r - s.filt_peaks[i] > RTECG_PT_MAX_DIST_BTN_MWI_AND_FILT){
						pd("FILT peak %d is too far in the past (%u samples)\n", s.filt_peaks[i], mwipos_r - s.filt_peaks[i]);
						break;
					}
					// have to recalculate f1 each time...
					// f1 needs to be recalculated from the beginning of the peak lists
					// up to the current one (i). if we find a signal peak, all peaks before
					// this one (i) are noise, as are all peaks after
					if(filt[s.filt_peaks[i] - 17] >= f1){
						pd("FILT peak at %u corresponds to MWI peak\n", s.filt_peaks[i]);
						// this is a signal peak---update thresholds
						s.npkf = npkf;
						s.npki = npki;
						s.spkf = .125 * filt[s.filt_peaks[i] - 17] + .875 * s.spkf;
						s.spki = .125 * mwi[s.mwi_peaks[(s.mwi_peaks_pos - 1) % RTECG_PT_HISTLEN] - 17] + .875 * s.spki;
						s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
						s.f2 = .5 * s.f1;
						s.i1 = s.npki + .25 * (s.spki - s.npki);
						s.i2 = .5 * s.i1;
						s.last_spkf = s.filt_peaks[i];
						s.last_spki = s.mwi_peaks[(s.mwi_peaks_pos - 1) % RTECG_PT_HISTLEN];
						s.filt_peaks_pos = 0;
						s.mwi_peaks_pos = 0;
						ps(s);
						break;
					}else{
						pd("FILT peak at %u does not correspond to MWI peak\n", s.filt_peaks[i]);
					}
				}
			}else{
				pd("%s\n", "no FILT peaks exist to compare MWI peak to\n");
				// this is a peak in the mwi, but we haven't had a candidate in the filtered signal, so it must be noise
				s.npki = .125 * mwi[(mwipos_r - 17) % mwilen] + .875 * s.npki;
				s.filt_peaks_pos--;
			}
		}else{
			pd("MWI: %f < %f\n", mwi[(mwipos_r - 17) % mwilen], i1);
			// nothing to do
		}
	}
	if(!found_spk){
		// check to see if we need to search back
	}
	return s;
}
