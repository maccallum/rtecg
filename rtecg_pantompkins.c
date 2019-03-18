#include "rtecg.h"
#include "rtecg_pantompkins.h"
#include "rtecg_rand.h"
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

//#define pb(fmt, ...)bufptr += snprintf(buf + bufptr, buflen - bufptr, "%d: "fmt, __LINE__, __VA_ARGS__);
#define pb(fmt, ...)

//#define RTECG_PT_RRLOWLIMIT 0.85
//#define RTECG_PT_RRHIGHLIMIT 1.26
#define RTECG_PT_RRLOWLIMIT 0.92 // orig eq. 26
#define RTECG_PT_RRHIGHLIMIT 1.16 // orig eq. 27
#define RTECG_PT_RRMISSEDLIMIT 1.66 // orig eq. 28

rtecg_spk rtecg_pt_nullPeak = (rtecg_spk){0, 0, 0, -1.};

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
	if(s.havefirstpeak){// && s.last_spkf.confidence != -1 && s.last_last_spkf.confidence != -1){
	        rtecg_float rr = (s.last_spkf.x - s.last_last_spkf.x);
		pd("rr = %d - %d\n", (rtecg_int)s.last_spkf.x, (rtecg_int)s.last_last_spkf.x);
		s.rr = rr;
		// rr avg 1
		s.rrsum1 -= s.rrbuf1[s.rrptr1];
		s.rrsum1 += rr;
		s.rrbuf1[s.rrptr1] = rr;
		s.rrptr1++;
		if(s.burn_avg1){
			s.rravg1 = s.rrsum1 / s.rrptr1;
		}else{
			s.rravg1 = s.rrsum1 / 8;
		}
		if(s.rrptr1 >= 8){
			s.rrptr1 = 0;
			s.burn_avg1 = 0;
		}

		// rr avg 2
		if(s.burn_avg2){
			pb("%s", "burning avg 2\n");
			s.rrsum2 += rr;
			s.rrbuf2[s.rrptr2] = rr;
			s.rrptr2++;
			s.rravg2 = s.rrsum2 / s.rrptr2;
		}else{
			int accept = 0;
			if(rr >= RTECG_PT_RRLOWLIMIT * s.rravg2 && rr <= RTECG_PT_RRHIGHLIMIT * s.rravg2){
				pb("rr is within limits (%f <= %f <= %f)\n", RTECG_PT_RRLOWLIMIT * s.rravg2, rr, RTECG_PT_RRHIGHLIMIT * s.rravg2);
				accept = 1;
			}else{
				rtecg_float u = (rtecg_float)rtecg_rand() / (rtecg_float)rtecg_rand_max();
				rtecg_float p = (rtecg_float)++s.rravg2_missed_ctr / (rtecg_float)8;
				rtecg_float v = 0;
				if(rr < RTECG_PT_RRLOWLIMIT * s.rravg2){
					pb("rr is too low (%f < %f)\n", rr, RTECG_PT_RRLOWLIMIT * s.rravg2);
					v = rr / (s.rravg2 * RTECG_PT_RRLOWLIMIT);
				}else{
					pb("rr is too high (%f > %f)\n", rr, RTECG_PT_RRHIGHLIMIT * s.rravg2);
					v = (s.rravg2 * RTECG_PT_RRHIGHLIMIT) / rr;
				}
				p *= (v * v * v * v);
				pb("u = %f, v = %f, p = %f: ", u, v, p);
				if(u <= p){
					pb("%s", "ACCEPT\n");
					accept = 1;
				}else{
					pb("%s", "REJECT\n");
				}
				
				pb("rravg2_missed_ctr = %d\n", s.rravg2_missed_ctr);
			}
			if(accept){
				s.rrsum2 -= s.rrbuf2[s.rrptr2];
				s.rrsum2 += rr;
				s.rravg2 = s.rrsum2 / 8;
				s.rrbuf2[s.rrptr2] = rr;
				s.rrptr2++;
				s.rravg2_missed_ctr--;
				if(s.rravg2_missed_ctr < 0){
					s.rravg2_missed_ctr = 0;
				}
			}
		}
		if(s.rrptr2 >= 8){
			s.rrptr2 = 0;
			s.burn_avg2 = 0;
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

rtecg_pt rtecg_pt_recordMissedPeak(rtecg_pt s, char *buf, size_t buflen, int bufptr)
{
	s.havefirstpeak = 0;
	s.havepeak = 0;
	// we record this as a null peak with confidence -1
	// that way we have a record that we missed a peak,
	// and we don't try to calculate an RR interval based on it
	s.last_last_spkf = s.last_spkf;
	s.last_last_spki = s.last_spki;
	s.last_spkf = rtecg_pt_nullPeak;
	s.last_spki = rtecg_pt_nullPeak;

	// reset pointers and counter
	s.ptrf = 0;
	s.tptrf = 0;
	s.ptri = 0;
	return s;
}

rtecg_pt rtecg_pt_process(rtecg_pt s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei, char *buf, size_t buflen, int bufptr)
{
	pd("%s\n", "rtecg_pt_process");
	s.searchback = 0;
	s.ctr++;
	// there's often some noise when the ECG is powered up, so skip the first N samples
	if(s.ctr < RTECG_PREBURNLEN){
		pd("%s\n", "preburn");
		return s;
	}
	// We don't have any reasonable way to estimate the thresholds, so
	// we look for the two largest peaks in some number of samples and calculate
	// them based on those two peaks
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
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
			s.i1 = s.npki + .25 * (s.spki - s.npki);
			s.i2 = s.i1 * .5;
		}
		return s;
	}
	s.havepeak = 0;
	if(pkf){
		if(s.havefirstpeak && /*s.last_spkf.confidence != -1 && */ (s.ctr - s.last_spkf.x < RTECG_MTOS(200))){
			// it's been less than 200ms since the last signal peak,
			// so this is classified as noise
			pd("%s\n", "pkf is lt 200ms after last peak");
			rtecg_float onpkf = s.npkf;
			s.npkf = 0.125 * pkf + .875 * s.npkf;
			pd("npkf %d -> %d\n", (rtecg_int)onpkf, (rtecg_int)s.npkf);
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
		}else{
			pd("%s\n", "adding pkf to list");
			s.pkf[s.ptrf] = (rtecg_spk){s.ctr, pkf, maxslopef, 0};
			s.ptrf++;
		}
	}
	if(pki){
		if((s.havefirstpeak && /*s.last_spki.confidence != -1 && s.last_spkf.confidence != -1 && */s.ctr - s.last_spkf.x < RTECG_MTOS(200))){
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
				// ---the corresponding peak in the filtered signal should be RTECG_DERIVDEL + RTECG_MWILEN +- RTECG_MTOS(20) samples in the past.
				// ---update tnpkf, tf1, and tf2 for all pkf values from tptrf to ptrf - (RTECG_MWILEN + RTECG_MTOS(20)).
				// ---compare all subsequent peaks in the filtered signal to tf1, updating local temp tnpkf and tf1 values.
				// ---if there is more than one peak, take the max
				// ---if a signal peak is found, record it, update all estimates and thresholds, and reset ptrs and ctr.
				// ---if a signal peak is not found, mwi is a noise peak---update temporary estimates and thresholds for mwi, but not filt.
				pd("%s\n", "pki > i1");
				//rtecg_ctr pkfmint = s.ctr - ((RTECG_DERIVDEL + RTECG_MWIDEL) * 2);
				//rtecg_ctr pkfmaxt = s.ctr;
				rtecg_ctr pkfmint = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL + RTECG_MTOS(25));
				rtecg_ctr pkfmaxt = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL - RTECG_MTOS(25));
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
					if(/*s.last_spki.confidence != -1 && */s.ctr - s.last_spki.x < RTECG_MTOS(360) && (float)maxslopei / (float)s.last_spki.maxslope < 0.5){
						// "When an RR interval is less than 360 ms (it must be greater
						// than the 200 ms latency), a judgement is made to determine
						// whether the current QRS complex has been correctly identified
						// or whether it is really a T wave. If the maximal slope that
						// occurs during this waveform is less than half that of the QRS
						// waveform that preceeded it, it is identified to be a T wave; other-
						// wise it is called a QRS complex." P. 234
						pd("%s\n", "rejecting---likely a t-wave");
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

	if((pkf || pki) && s.havefirstpeak && s.havepeak == 0 && (!s.burn_avg2) && (s.ctr - s.last_spki.x) > (s.rravg2 * 1.66)){
		// "If a QRS complex is not found during the interval specified
		// by the RR MISSED LIMIT [166% RR AVERAGE2, eq. 28], [trigger search back]
		// P. 234
		pd("%s\n", "triggering searchback");
		pd("s.rravg2 = %f, s.rravg2 * 1.66 = %f\n", s.rravg2, s.rravg2 * 1.66);
		s.searchback = 1;
		//s = rtecg_pt_recordMissedPeak(s, buflen, buf, bufptr);
	}
	
	// this seems extreme, but it will happen almost never, only when there's a signal
	// that's noisy to the point of being unusable
	if(s.ptrf == RTECG_PTBUFLEN){
		pd("%s\n", "buffer overrun (FIL)");
		return rtecg_pt_reset(s);
	}
	if(s.ptri == RTECG_PTBUFLEN){
		pd("%s\n", "buffer overrun (MWI)");
		return rtecg_pt_reset(s);
	}
	return s;
}

// #undef pl
// #undef pd
// #undef ps
// #define pl() bufptr += snprintf(buf + bufptr, buflen - bufptr, "**************************************************\n");
// #define pd(fmt, ...)bufptr += snprintf(buf + bufptr, buflen - bufptr, "%d: "fmt, __LINE__, __VA_ARGS__);
// #define ps(s)

rtecg_pt rtecg_pt_searchback(rtecg_pt s, char *buf, size_t buflen, int bufptr)
{
	// "If a QRS complex is not found during the interval specified
	// by the RR MISSED LIMIT [166% RR AVERAGE2, eq. 28], the maximal peak reserved between
	// the two established thresholds [92% RR AVERAGE2 and 116% RR AVERAGE2] is considered to be a QRS
	// candidate." P. 234
	// the paper isn't clear about whether "the maximal peak" should be a peak that is
	// in both the MWI signal as well as the filtered signal. 

	// search for a peak in the MWI signal
	if(s.ptri == 0){
		pd("%s\n", "no mwi peaks to search through!");
		s.searchback = 0;
		return rtecg_pt_recordMissedPeak(s, buf, buflen, bufptr);
	}
	
	// increment ptri to the first peak that is >= RRLOWLIMIT
	rtecg_int ptri = 0;
	while(ptri < s.ptri && (s.pki[ptri].x - s.last_spki.x) < s.rravg2 * RTECG_PT_RRLOWLIMIT){
		pd("rejecting pki: %d lt %d\n", s.pki[ptri].x - s.last_spki.x, (rtecg_int)(s.rravg2 * RTECG_PT_RRLOWLIMIT));
		ptri++;
	}

	// search for the max peak between RRLOWLIMIT and RRHIGHLIMIT
	rtecg_spk pkimax = {0, 0, 0, 0};
	rtecg_int havepki = 0;
	while(ptri < s.ptri && (s.pki[ptri].x - s.last_spki.x) <= s.rravg2 * RTECG_PT_RRHIGHLIMIT){
		rtecg_spk pki = s.pki[ptri];
		if(pki.y > pkimax.y){
			pd("pki %d gt %d\n", pki.y, pkimax.y);
			pkimax = pki;
			havepki = 1;
		}else{
			pd("rejecting pki: %d lt %d\n", pki.y, pkimax.y);
		}
		ptri++;
	}

	if(!havepki){
		pd("%s\n", "no mwi peaks between RRLOWavg2 and RRHIGHavg2!");
		s.searchback = 0;
		return rtecg_pt_recordMissedPeak(s, buf, buflen, bufptr);
	}

	// now search for a corresponding peak in the filtered signal
	if(s.ptri == 0){
		pd("%s\n", "no filt peaks to search through!");
		s.searchback = 0;
		return rtecg_pt_recordMissedPeak(s, buf, buflen, bufptr);
	}
	rtecg_int ptrf = 0;
	rtecg_spk pkfmax = {0, 0, 0, 0};
	rtecg_int havepkf = 0;
	while(ptrf < s.ptrf){
		rtecg_spk pkf = s.pkf[ptrf];
		//if(pkf.x < (pkimax.x - ((RTECG_DERIVDEL + RTECG_MWIDEL) * 2))){
		if(pkf.x >= (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL + RTECG_MTOS(50))) &&
		   pkf.x <= (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL - RTECG_MTOS(50)))){
			//pd("rejecting pkf: %d lt %d\n", pkf.x, (pkimax.x - ((RTECG_DERIVDEL + RTECG_MWIDEL) * 2)));
			if(pkf.y > pkfmax.y){
				pd("pkf %d gt %d\n", pkf.x, pkfmax.x);
				pkfmax = pkf;
				havepkf = 1;
			}
			//ptrf++;
			//continue;
		}else{
			pd("rejecting pkf.x (%d): outside the range of the mwi peak (%d - %d)\n", pkf.x, (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL + RTECG_MTOS(50))), (pkimax.x - (RTECG_DERIVDEL + RTECG_MWIDEL - RTECG_MTOS(50))));
		}
		// if(pkf.y > pkfmax.y){
		// 	pd("pkf %d gt %d\n", pkf.x, pkfmax.x);
		// 	pkfmax = pkf;
		// 	havepkf = 1;
		// }
		ptrf++;
	}

	if(!havepkf){
		pd("%s\n", "have pki, but not pkf");
		s.searchback = 0;
		return rtecg_pt_recordMissedPeak(s, buf, buflen, bufptr);
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
