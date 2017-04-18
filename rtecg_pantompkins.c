#include "rtecg.h"
#include "rtecg_pantompkins.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

//#define RTECG_PT_PRINTSTATE
#ifdef RTECG_PT_PRINTSTATE
#define pl() bufptr += snprintf(buf + bufptr, buflen - bufptr, "**************************************************\n");
#define pd(fmt, ...)bufptr += snprintf(buf + bufptr, buflen - bufptr, "%d: "fmt, __LINE__, __VA_ARGS__);
#define ps(s)
#else
#define pl()
#define pd(fmt, ...)
#define ps(s)
#endif

//#define RTECG_PT_RRLOWLIMIT 0.92
//#define RTECG_PT_RRHIGHLIMIT 1.16
#define RTECG_PT_RRLOWLIMIT 0.85
#define RTECG_PT_RRHIGHLIMIT 1.26
#define RTECG_PT_RRMISSEDLIMIT 1.66

rtecg_pt rtecg_pt_reset(rtecg_pt s)
{
	rtecg_spk *pkf = s.pkf;
	rtecg_spk *pki = s.pki;
	memset(&s, 0, sizeof(rtecg_pt));
	s.pkf = pkf;
	s.pki = pki;
	s.burn_avg1 = s.burn_avg2 = 1;
	return s;
}

rtecg_pt rtecg_pt_init(void)
{
	rtecg_pt s;
	memset(&s, 0, sizeof(rtecg_pt));
	s.pkf = (rtecg_spk *)calloc(RTECG_PTBUFLEN, sizeof(rtecg_spk));
	s.pki = (rtecg_spk *)calloc(RTECG_PTBUFLEN, sizeof(rtecg_spk));
	s.burn_avg1 = s.burn_avg2 = 1;
	return s;
}

rtecg_pt rtecg_pt_computerr(rtecg_pt s, char *buf, size_t buflen, int bufptr)
{
	if(s.havefirstpeak){
		rtecg_float rr = (s.last_spkf.x - s.last_last_spkf.x);
		pd("rr = %d - %d\n", (rtecg_int)s.last_spkf.x, (rtecg_int)s.last_last_spkf.x);
		s.rr = rr;
		s.rrsum1 = 0;
		s.rrbuf1[s.rrptr1] = rr;
		s.rrptr1++;
		if(s.burn_avg1){
			for(int i = 0; i < s.rrptr1; i++){
				s.rrsum1 += s.rrbuf1[i];
			}
			s.rravg1 = s.rrsum1 / (rtecg_float)s.rrptr1;
		}else{
			for(int i = 0; i < 8; i++){
				s.rrsum1 += s.rrbuf1[i];
			}
			s.rravg1 = s.rrsum1 / 8.;
		}
		if(s.rrptr1 == 8){
			s.rrptr1 = 0;
			s.burn_avg1 = 0;
		}
		if(s.rravg2_missed_ctr == 8){
			s.rravg2_missed_ctr = 0;
			// s.rrptr2 = 0;
			// s.burn_avg2 = 1;
			// s.rrsum2 = 0;
			// memcpy(s.rrbuf2, s.rrbuf1, 8 * sizeof(rtecg_float));
		}
		if(s.burn_avg2){
			// avg 2
			s.rrsum2 += rr;
			s.rrbuf2[s.rrptr2] = rr;
			s.rrptr2++;
			s.rravg2 = s.rrsum2 / (rtecg_float)s.rrptr2;
			if(s.rrptr2 == 8){
				s.rrptr2 = 0;
				s.burn_avg2 = 0;
			}
		}else{
			// avg 2
			if(rr >= RTECG_PT_RRLOWLIMIT * s.rravg2 && rr <= RTECG_PT_RRHIGHLIMIT * s.rravg2){
				s.rrbuf2[s.rrptr2] = rr;
				s.rrsum2 = 0;
				for(int i = 0; i < 8; i++){
					s.rrsum2 += s.rrbuf2[i];
				}
				if(++(s.rrptr2) == 8){
					s.rrptr2 = 0;
				}
				s.rravg2 = s.rrsum2 / 8.;
			}else{
				s.rravg2_missed_ctr++;
			}
		}
		pd("rr = %d, %d, %d\n", (rtecg_int)s.rr, (rtecg_int)s.rravg1, (rtecg_int)s.rravg2);
	}
	return s;
}

rtecg_pt rtecg_pt_recordPeak(rtecg_pt s, rtecg_spk spkf, rtecg_spk spki, rtecg_int searchback, char *buf, size_t buflen, int bufptr)
{
	s.havepeak = 1;
	// record peaks
	s.last_last_spkf = s.last_spkf;
	s.last_last_spki = s.last_spki;
	s.last_spkf = spkf;
	s.last_spki = spki;

	// compute rr interval
	if(s.havefirstpeak){
		s = rtecg_pt_computerr(s, buf, buflen, bufptr);
	}

	// update signal estimates for filtered signal and mwi
	if(searchback){
		s.spkf = .25 * spkf.y + .75 * s.spkf;
		s.spki = .25 * spki.y + .75 * s.spki;
	}else{
		s.spkf = .125 * spkf.y + .875 * s.spkf;
		s.spki = .125 * spki.y + .875 * s.spki;
	}
	
	// update noise estimates for filtered signal
	rtecg_ctr ptr = 0;
	while(s.pkf[ptr].x != spkf.x && ptr < s.ptrf){
		s.npkf = 0.125 * s.pkf[ptr].y + .875 * s.npkf;
		ptr++;
	}
	rtecg_float npkf = s.npkf;
	ptr++;
	while(ptr < s.ptrf){
		s.npkf = 0.125 * s.pkf[ptr].y + .875 * s.npkf;
		ptr++;
	}
	
	// update noise estimates for mwi
	ptr = 0;
	while(s.pki[ptr].x != spki.x && ptr < s.ptri){
		s.npki = 0.125 * s.pki[ptr].y + .875 * s.npki;
		ptr++;
	}
	rtecg_float npki = s.npki;
	ptr++;
	while(ptr < s.ptri){
		s.npki = 0.125 * s.pki[ptr].y + .875 * s.npki;
		ptr++;
	}

	// compute confidence. we want to compute the confidence
	// based on the signal and noise estimates up to and including
	// where the peak is, but not taking into account subsequent peaks
	if(searchback){
		rtecg_float f1 = npkf + .25 * (s.spkf - npkf);
		rtecg_float i1 = npki + .25 * (s.spki - npki);
		rtecg_float cf = 0.;
		if(spkf.y >= f1){
			cf = .75;
		}else if(spkf.y >= f1 * 0.5){
			cf = .5;
		}else if(spkf.y > 0){
			cf = .25;
		}else{
			cf = 0;
		}
		rtecg_float ci = 0.;
		if(spki.y >= i1){
			ci = .75;
		}else if(spki.y >= i1 * 0.5){
			ci = .5;
		}else if(spki.y > 0){
			ci = .25;
		}else{
			ci = 0;
		}
		s.last_spkf.confidence = cf;
		s.last_spki.confidence = ci;
	}else{
		s.last_spkf.confidence = 1.;
		s.last_spki.confidence = 1.;
	}
	
	// update thresholds
	s.f1 = s.tf1 = s.npkf + .25 * (s.spkf - s.npkf);
	s.i1 = s.ti1 = s.npki + .25 * (s.spki - s.npki);
	s.f2 = s.tf2 = s.f1 * .5;
	s.i2 = s.ti2 = s.i1 * .5;
	// reset pointers and counter
	s.ptrf = 0;
	s.tptrf = 0;
	s.ptri = 0;
					
	s.havefirstpeak = 1;
	return s;
}

rtecg_pt rtecg_pt_process(rtecg_pt s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei, char *buf, size_t buflen, int bufptr)
{
	pd("%s\n", "rtecg_pt_process");
	s.searchback = 0;
	s.ctr++;
	if(s.ctr < RTECG_PREBURNLEN){
		pd("%s\n", "preburn");
		return s;
	}
	if(s.ctr <= RTECG_PREBURNLEN + RTECG_BURNLEN){
		pd("%s\n", "burn");
		if(pkf > s.last_spkf.y){
			s.last_last_spkf = s.last_spkf;
			s.last_spkf = (rtecg_spk){s.ctr, pkf, maxslopef, 0};
		}
		if(pki > s.last_spki.y){
			s.last_last_spki = s.last_spki;
			s.last_spki = (rtecg_spk){s.ctr, pki, maxslopei, 0};
		}
		if(s.ctr == RTECG_PREBURNLEN + RTECG_BURNLEN){
			s.tspkf = s.spkf = (s.last_last_spkf.y + s.last_spkf.y) / 2.;
			s.tspki = s.spki = (s.last_last_spki.y + s.last_spki.y) / 2.;
			s.tnpkf = s.npkf = 0.25 * s.spkf;
			s.tnpki = s.npki = 0.25 * s.spki;
			//s.tnpkf = s.npkf = (out[FILT][inpkrf1] + out[FILT][inpkrf2]) / 2.;
			//s.tnpki = s.npki = (out[MWI][inpkri1] + out[MWI][inpkri2]) / 2.;
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
			s.i1 = s.npki + .25 * (s.spki - s.npki);
			s.i2 = s.i1 * .5;
		}
		return s;
	}
	s.havepeak = 0;
	if(pkf){
		if(s.havefirstpeak &&
		   (s.ctr - s.last_spkf.x < RTECG_MTOS(200))){
			// it's been less than 200ms since the last signal peak,
			// so this is classified as noise
			pd("%s\n", "pkf is lt 200ms after last peak");
			rtecg_float onpkf = s.npkf;
			s.npkf = 0.125 * pkf + .875 * s.npkf;
			pd("npkf %d -> %d\n", (rtecg_int)onpkf, (rtecg_int)s.npkf);
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
#ifdef RTECG_LIMIT_BPM_INCREASES
		}else if((s.rr && !s.burn_avg1 && (60. / ((s.ctr - s.last_spkf.x) / (float)RTECG_FS) > RTECG_MAX_BPM_INCREASE * (60. / (s.rravg1 / RTECG_FS))))){
			pd("%s\n", "reject pkf due to RTECG_LIMIT_BPM_INCREASES criteria");
			rtecg_float onpkf = s.npkf;
			s.npkf = 0.125 * pkf + .875 * s.npkf;
			pd("npkf %d -> %d\n", (rtecg_int)onpkf, (rtecg_int)s.npkf);
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
#endif
		}else{
			pd("%s\n", "adding pkf to list");
			s.pkf[s.ptrf] = (rtecg_spk){s.ctr, pkf, maxslopef, 0};
			s.ptrf++;
		}
	}
	if(pki){
		if((s.havefirstpeak && s.ctr - s.last_spkf.x < RTECG_MTOS(200))){
			// it's been less than 200ms since the last signal peak,
			// so this is classified as noise
			pd("%s\n", "pki is lt 200ms after last peak");
			rtecg_float onpki = s.npki;
			s.npki = 0.125 * pki + .875 * s.npki;
			pd("npki %d -> %d\n", (rtecg_int)onpki, (rtecg_int)s.npki);
			s.i1 = s.npki + .25 * (s.spki - s.npki);
			s.i2 = s.i1 * .5;
		}else{
			pd("%s\n", "adding pki to list");
			s.pki[s.ptri] = (rtecg_spk){s.ctr, pki, maxslopei, 0};
			if(pki >= s.ti1){
				// this is potentially a signal peak.
				// ---the corresponding peak in the filtered signal should be RTECG_MWIDEL +- RTECG_MTOS(20) samples in the past.
				// ---update tnpkf, tf1, and tf2 for all pkf values from tptrf to ptrf - (RTECG_MWIDEL + RTECG_MTOS(20)).
				// ---compare all subsequent peaks in the filtered signal to tf1, updating local temp tnpkf and tf1 values.
				// ---if there is more than one peak, take the max
				// ---if a signal peak is found, record it, update all estimates and thresholds, and reset ptrs and ctr.
				// ---if a signal peak is not found, mwi is a noise peak---update temporary estimates and thresholds for mwi, but not filt.
				pd("%s\n", "pki > i1");
				rtecg_ctr pkfmint = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL);// + RTECG_MWILEN);
				rtecg_ctr pkfmaxt = s.ctr;// - (RTECG_DERIVDEL + RTECG_MWIDEL);
				pd("searching for pkf between %u and %u that correspond to a pki at %d\n", pkfmint, pkfmaxt, s.ctr);
				// any peaks that are before pkfmint will be too far in the past
				// to correspond to any subsequent mwi peaks, so classify them as noise
				while(s.pkf[s.tptrf].x < pkfmint && s.tptrf < s.ptrf){
					pd("rejecting (%d, %d)\n", s.pkf[s.tptrf].x, s.pkf[s.tptrf].y);
					s.tnpkf = 0.125 * s.pkf[s.tptrf].y + .875 * s.tnpkf;
					s.tptrf++;
				}
				s.tf1 = s.tnpkf + .25 * (s.tspkf - s.tnpkf);
				s.tf2 = s.tf1 * .5;
				rtecg_int tptrf = s.tptrf;
				rtecg_int pkmax = 0;
				rtecg_int pkidx = 0;
				while(s.pkf[tptrf].x <= pkfmaxt && tptrf < s.ptrf){ 
					// this is a small enough window that i'm too lazy to
					// update the estimates and thresholds every time.
					if(s.pkf[tptrf].y >= s.tf1 && s.pkf[tptrf].y > pkmax){
						pkmax = s.pkf[tptrf].y;
						pkidx = tptrf;
					}
					tptrf++;
				}
				if(pkmax){
					pd("found a pkf that corresponds to our pki: pkf: (%d, %d), pki: (%d, %d)\n", s.pkf[pkidx].x, s.pkf[pkidx].y, s.pki[s.ptri].x, s.pki[s.ptri].y);
					if(s.ctr - s.last_spki.x < RTECG_MTOS(360) && (float)maxslopei / (float)s.last_spki.maxslope < 0.5){
						pd("%s\n", "rejecting---likely a t-wave");
						// this is likely a T-wave
						s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
						s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
						s.ti2 = s.ti1 * .5;
						s.ptri++;
					}else{
						pd("%s\n", "recording peak");
						s = rtecg_pt_recordPeak(s, s.pkf[pkidx], s.pki[s.ptri], 0, buf, buflen, bufptr);
					}
				}else{
					pd("%s\n", "pki doesn't have a corresponding pkf");
					// mwi doesn't have a corresponding peak in the filtered signal, so it's a noise peak
					s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
					s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
					s.ti2 = s.ti1 * .5;
					s.ptri++;
				}
			}else{
				pd("%s\n", "pki lt i1");
				s.ptri++;
			}
		}
	}
	if((pkf || pki) && s.havefirstpeak && s.havepeak == 0 && !s.burn_avg2 && (s.ctr - s.last_spki.x) > (s.rravg1 * 1.66)){
		pd("%s\n", "triggering searchback");
		s.searchback = 1;
	}
	
	// this seems extreme, but it will happen almost never, only when there's a signal
	// that's noisy to the point of being unusable
	if(s.ptrf == RTECG_PTBUFLEN){
		pd("%s\n", "buffer overrun (FIL)");
		return rtecg_pt_reset(s);
		rtecg_int i = 1;
		while(i < s.tptrf && (s.pkf[i].x - s.last_spkf.x) > RTECG_FS * 2){
			i++;
		}
		memmove(s.pkf, s.pkf + i, RTECG_PTBUFLEN - i);
		s.ptrf--;
		s.tptrf--;
	}
	if(s.ptri == RTECG_PTBUFLEN){
		pd("%s\n", "buffer overrun (MWI)");
		return rtecg_pt_reset(s);
		rtecg_int i = 1;
		while((s.pki[i].x - s.last_spki.x) > RTECG_FS * 2){
			i++;
		}
		memmove(s.pki, s.pki + i, RTECG_PTBUFLEN - i);
		s.ptri--;
	}
	return s;
}

rtecg_pt rtecg_pt_searchback(rtecg_pt s, char *buf, size_t buflen, int bufptr)
{
	if(s.ptri == 0){
		pd("%s\n", "no mwi peaks to search through!");
		s.searchback = 0;
		return s;
	}
	rtecg_int ptri = 0;
	while(ptri < s.ptri && (s.pki[ptri].x - s.last_spki.x) < s.rravg1 * RTECG_PT_RRLOWLIMIT){
		pd("rejecting pki: %d lt %d\n", s.pki[ptri].x - s.last_spki.x, (rtecg_int)(s.rravg1 * RTECG_PT_RRLOWLIMIT));
		ptri++;
	}
	rtecg_spk pkimax = {0, 0, 0, 0};
	rtecg_int havepki = 0;
	while(ptri < s.ptri && (s.pki[ptri].x - s.last_spki.x) <= s.rravg1 * RTECG_PT_RRHIGHLIMIT){
		rtecg_spk pki = s.pki[ptri];
		if(pki.y > pkimax.y && pki.x - s.last_spki.x > RTECG_MTOS(200)){
			pd("pki %d gt %d\n", pki.y, pkimax.y);
			pkimax = pki;
			havepki = 1;
		}else{
			pd("rejecting pki: %d lt %d or too early\n", pki.y, pkimax.y);
		}
		ptri++;
	}
	if(!havepki){
		pd("%s\n", "no mwi peaks between RRLOWavg2 and RRHIGHavg2!");
		pd("recording fake pkf by adding the avg: %d pl %d eq %d\n", s.last_spkf.x, (rtecg_int)s.rravg1, s.last_spkf.x + (rtecg_int)s.rravg1);
		s = rtecg_pt_recordPeak(s, (rtecg_spk){s.last_spkf.x + s.rravg1, 0, 0, 0}, (rtecg_spk){s.last_spki.x + s.rravg1, 0, 0, 0}, 1, buf, buflen, bufptr);
		s.searchback = 0;
		return s;
	}
	rtecg_int ptrf = 0;
	rtecg_spk pkfmax;
	memset(&pkfmax, 0, sizeof(rtecg_spk));
	rtecg_int havepkf = 0;
	while(ptrf < s.ptrf){
		rtecg_spk pkf = s.pkf[ptrf];
		if(pkf.x < (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL))){// + RTECG_MWILEN))){
			//if(pkf.x < (pkimax.x - (RTECG_DERIVDEL + RTECG_MWILEN))){
			pd("rejecting pkf: %d lt %d\n", pkf.x, (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL)));
			ptrf++;
			continue;
		}
		if(pkf.x > (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL))){
			break;
		}
		if(pkf.y > pkfmax.y){
			pd("pkf %d gt %d\n", pkf.x, pkfmax.x);
			pkfmax = pkf;
			havepkf = 1;
		}
		ptrf++;
	}
	if(havepkf == 0){
		// we have a peak in the mwi, but not in the filtered signal. we record a peak
		// anyway, and the confidence value for the filtered signal will be 0.
		pd("no matching pkf for (%d %d), putting one at %d\n", pkimax.x, pkimax.y, pkimax.x - (RTECG_DERIVDEL + (RTECG_MWILEN / 2)));
		s = rtecg_pt_recordPeak(s, (rtecg_spk){pkimax.x - (RTECG_DERIVDEL + (RTECG_MWILEN / 2)), 0, 0, 0}, pkimax, 1, buf, buflen, bufptr);
		s.searchback = 0;
		return s;
	}
	pd("recording peak: (%d %d) (%d %d)\n", pkfmax.x, pkfmax.y, pkimax.x, pkimax.y);
	s = rtecg_pt_recordPeak(s, pkfmax, pkimax, 1, buf, buflen, bufptr);
	s.searchback = 0;
	return s;
}

rtecg_spk rtecg_pt_last_spkf(rtecg_pt s)
{
	return (rtecg_spk){s.ctr - s.last_spkf.x, s.last_spkf.y, s.last_spkf.maxslope, s.last_spkf.confidence};
}

rtecg_spk rtecg_pt_last_spki(rtecg_pt s)
{
	return (rtecg_spk){s.ctr - s.last_spki.x, s.last_spki.y, s.last_spki.maxslope, s.last_spki.confidence};
}
