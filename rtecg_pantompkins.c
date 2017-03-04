#include "rtecg.h"
#include "rtecg_pantompkins.h"
#include <string.h>
#include <stdio.h>

//#define RTECG_PT_PRINTSTATE
#define RTECG_PT_PRINTSTATE2
#ifdef RTECG_PT_PRINTSTATE
#define pl()printf("**************************************************\n");
#define pd(fmt, ...)printf("%s(%d): "fmt, __func__, __LINE__, __VA_ARGS__);
#define ps(s)printf("%s(%d): STATE:\n\tspkf = %f\n\tnpkf = %f\n\tspki = %f\n\tnpki = %f\n\tf1 = %f\n\tf2 = %f\n\ti1 = %f\n\ti2 = %f\n\tlast_spkf = %u\n\tlast_spki = %u\n\tfilt_peaks(%d): [%u, %u, %u, %u, %u, %u]\n\tmwi_peaks(%d): [%u, %u, %u, %u, %u, %u]\n", __func__, __LINE__, s.spkf, s.npkf, s.spki, s.npki, s.f1, s.f2, s.i1, s.i2, s.last_spkf, s.last_spki, s.filt_peaks_pos, s.filt_peaks[0], s.filt_peaks[1], s.filt_peaks[2], s.filt_peaks[3], s.filt_peaks[4], s.filt_peaks[5], s.mwi_peaks_pos, s.mwi_peaks[0], s.mwi_peaks[1], s.mwi_peaks[2], s.mwi_peaks[3], s.mwi_peaks[4], s.mwi_peaks[5]);
#elif defined(RTECG_PT_PRINTSTATE2)
#define pl()
#define pd(fmt, ...)
#define ps(s)
#define pl2()printf("**************************************************\n");
#define pd2(fmt, ...)printf("%s(%d): "fmt, __func__, __LINE__, __VA_ARGS__);
#define ps2(s)
#else
#define pl()
#define pd(fmt, ...)
#define ps(s)
#define pl2()
#define pd2(fmt, ...)
#define ps2(s)
#endif

/*
typedef struct _rtecg_pt2
{
	rtecg_float spkf, spki, npkf, npki, f1, f2, i1, i2;
	rtecg_float tspkf, tspki, tnpkf, tnpki, tf1, tf2, ti1, ti2;
	struct pt pkf[RTECG_PT_HISTLEN], pki[RTECG_PT_HISTLEN];
	rtecg_int ptrf, tptrf, ptri, tptri;
	rtecg_ctr ctr;
	struct pt last_spkf, last_spki;
} rtecg_pt2;
*/

rtecg_pt rtecg_pt_init(void)
{
	rtecg_pt s;
	memset(&s, 0, sizeof(rtecg_pt));
	return s;
}

rtecg_pt2 rtecg_pt2_init(void)
{
	rtecg_pt2 s;
	memset(&s, 0, sizeof(rtecg_pt2));
	return s;
}

rtecg_pt2 rtecg_pt2_process(rtecg_pt2 s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei)
{
	pl2();
	s.ctr++;
	s.havepeak = 0;
	// if we have a peak in the filtered signal, just add it to the list
	if(pkf){
		pd2("+ FIL: %u %d\n", s.ctr, pkf);
		s.pkf[s.ptrf] = (rtecg_spk){s.ctr, pkf, maxslopef};
		s.ptrf++;
	}
	if(pki){
		pd2("+ MWI: %u %d\n", s.ctr, pki);
		s.pki[s.ptri] = (rtecg_spk){s.ctr, pki, maxslopei};
		if(pki >= s.ti1){
			pd2("\t-> %s\n", "potential signal peak");
			// this is potentially a signal peak.
			// ---the corresponding peak in the filtered signal should be RTECG_MWIDEL +- RTECG_MTOS(20) samples in the past.
			// ---update tnpkf, tf1, and tf2 for all pkf values from tptrf to ptrf - (RTECG_MWIDEL + RTECG_MTOS(20)).
			// ---compare all subsequent peaks in the filtered signal to tf1, updating local temp tnpkf and tf1 values.
			// ---if there is more than one peak, take the max
			// ---if a signal peak is found, record it, update all estimates and thresholds, and reset ptrs and ctr.
			// ---if a signal peak is not found, mwi is a noise peak---update temporary estimates and thresholds for mwi, but not filt.
			//rtecg_int pkfmint = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL + RTECG_MTOS(50));
			//rtecg_int pkfmaxt = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL - RTECG_MTOS(50));
			rtecg_int pkfmint = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL);
			rtecg_int pkfmaxt = s.ctr;
			if(pkfmaxt < 0){
				pd2("\t-> %s\n", "too early, bail");
				s.tnpki = .125 * s.pki[s.ptri].y + .875 * s.tnpki;
				s.ptri++;
				return s;
			}
			if(pkfmint < 0){
				pkfmint = 0; 
			}
			pd2("\t-> pkfmint: %d, pkfmaxt: %d\n", pkfmint, pkfmaxt);
			pd2("\t-> oldest peak is at %d\n", s.pkf[s.tptrf].x);
			while(s.pkf[s.tptrf].x < pkfmint){
				pd2("\t-> peak at %d (s.ptrf = %d, s.tptrf = %d) is too far away from %d\n", s.pkf[s.tptrf].x, s.ptrf, s.tptrf, pkfmint);
				s.tnpkf = 0.125 * s.pkf[s.tptrf].y + .875 * s.tnpkf;
				s.tptrf++;
			}
			s.tf1 = s.tnpkf + .25 * (s.tspkf - s.tnpkf);
			s.tf2 = s.tf1 * .5;
			rtecg_int tptrf = s.tptrf;
			rtecg_int pkmax = 0;
			rtecg_int pkidx = 0;
			pd2("\t-> looking for peak while %d <= %d && %d < %d\n", s.pkf[tptrf].x, pkfmaxt, tptrf, s.ptrf);
			while(s.pkf[tptrf].x <= pkfmaxt && tptrf < s.ptrf){ 
				pd2("\t-> %d >= %f = %d, %d > %d = %d\n", s.pkf[tptrf].y, s.tf1, s.pkf[tptrf].y >= s.tf1, s.pkf[tptrf].y, pkmax, s.pkf[tptrf].y > pkmax);
				// this is a small enough window that i'm too lazy to
				// update the estimates and thresholds every time.
				if(s.pkf[tptrf].y >= s.tf1 && s.pkf[tptrf].y > pkmax){
					pkmax = s.pkf[tptrf].y;
					pkidx = tptrf;
				}
				tptrf++;
			}
			if(pkmax){
				if(s.ctr - s.last_spki.x < RTECG_MTOS(200)){
					pd2("\t-> %s\n", "peak is less than 200ms after the previous peak, bailing");
					// too quick
					s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
					s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
					s.ti2 = s.ti1 * .5;
					s.ptri++;
				}else if(s.ctr - s.last_spki.x < RTECG_MTOS(360) && (float)maxslopei / (float)s.last_spki.maxslope < 0.5){
					// could be a T-wave---compare slope with slope of last peak
					pd2("\t-> peak is %d samples after previous peak; more than 200ms (%d samples), but less than 360ms (%d samples). slope of previous peak is %d, slope of this peak is %d (%f)\n", s.ctr - s.last_spki.x, RTECG_MTOS(200), RTECG_MTOS(360), s.last_spki.maxslope, maxslopei, (float)maxslopei / (float)s.last_spki.maxslope);
					s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
					s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
					s.ti2 = s.ti1 * .5;
					s.ptri++;
				}else{
					s.havepeak = 1;
					pd2("\t-> we have a peak!\n\t\tin the filtered signal, its value is %d, and it's %d samples in the past\n\t\tin the mwi signal, its value is %d, and it's %d samples in the past\n", pkmax, s.ctr - s.pkf[pkidx].x, pki, 0);
					// check to see if it occurred too soon since the last one
					// record peaks
					//s.last_spkf = (struct pt){s.ctr - s.pkf[pkidx].x, s.pkf[pkidx].y};
					//s.last_spki = (struct pt){s.ctr - s.pki[s.ptri].x, s.pki[s.ptri].y};
					s.last_spkf = (rtecg_spk){s.pkf[pkidx].x, s.pkf[pkidx].y, maxslopef};
					s.last_spki = (rtecg_spk){s.pki[s.ptri].x, s.pki[s.ptri].y, maxslopei};
					// update noise estimates for filtered signal
					while(s.tptrf < pkidx){
						s.tnpkf = 0.125 * s.pkf[s.tptrf].y + .875 * s.tnpkf;
						s.tptrf++;
					}
					s.tnpkf = pkidx + 1;
					while(s.tptrf < s.ptrf){
						s.tnpkf = 0.125 * s.pkf[s.tptrf].y + .875 * s.tnpkf;
						s.tptrf++;
					}
					// update noise estimates for mwi
					//while(s.tptri < s.ptri){
					//s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
					//s.tptri++;
					//}
					// update signal estimates for filtered signal and mwi
					s.tspkf = .125 * s.pkf[pkidx].y + .875 * s.tspkf;
					s.tspki = .125 * s.pki[s.ptri].y + .875 * s.tspki;
					// transfer temp values to permanent ones
					s.spkf = s.tspkf;
					s.spki = s.tspki;
					s.npkf = s.tnpkf;
					s.npki = s.tnpki;
					s.f1 = s.tf1 = s.tnpkf + .25 * (s.tspkf - s.tnpkf);
					s.f2 = s.tf2 = s.f1 * .5;
					s.i1 = s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
					s.i2 = s.ti2 = s.i1 * .5;
					// reset pointers and counter
					s.ptrf = 0;
					s.tptrf = 0;
					s.ptri = 0;
				}
			}else{
				
				// mwi doesn't have a corresponding peak in the filtered signal, so it's a noise peak
				s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
				s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
				s.ti2 = s.ti1 * .5;
				s.ptri++;
			}
		}else{
			s.ptri++;
		}
	}
	return s;
}

rtecg_spk rtecg_pt2_last_spkf(rtecg_pt2 s)
{
	return (rtecg_spk){s.ctr - s.last_spkf.x, s.last_spkf.y, s.last_spkf.maxslope};
}

rtecg_spk rtecg_pt2_last_spki(rtecg_pt2 s)
{
	return (rtecg_spk){s.ctr - s.last_spki.x, s.last_spki.y, s.last_spki.maxslope};
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
