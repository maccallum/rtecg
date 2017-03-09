#include "rtecg.h"
#include "rtecg_pantompkins.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define RTECG_PT_PRINTSTATE
#ifdef RTECG_PT_PRINTSTATE
#define pl()printf("**************************************************\n");
#define pd(fmt, ...)printf("%s(%d): "fmt, __func__, __LINE__, __VA_ARGS__);
#define ps(s)
#else
#define pl()
#define pd(fmt, ...)
#define ps(s)
#endif

rtecg_pt rtecg_pt_init(void)
{
	rtecg_pt s;
	memset(&s, 0, sizeof(rtecg_pt));
	s.pkf = (rtecg_spk *)calloc(RTECG_PTBUFLEN, sizeof(rtecg_spk));
	s.pki = (rtecg_spk *)calloc(RTECG_PTBUFLEN, sizeof(rtecg_spk));
	s.burn_avg1 = s.burn_avg2 = 1;
	return s;
}

rtecg_pt rtecg_pt_process(rtecg_pt s, rtecg_int pkf, rtecg_int maxslopef, rtecg_int pki, rtecg_int maxslopei)
{
	pl();
	s.searchback = 0;
	s.ctr++;
	if(s.ctr < RTECG_PREBURNLEN){
		return s;
	}
	if(s.ctr <= RTECG_PREBURNLEN + RTECG_BURNLEN){
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
		pd("+ FIL: %u %d\n", s.ctr, pkf);
		if(s.havefirstpeak &&
		   (s.ctr - s.last_spkf.x < RTECG_MTOS(200))){
			pd("\t\t-> it's only been %d ms since the last peak\n", RTECG_STOM(s.ctr - s.last_spkf.x));
			// it's been less than 200ms since the last signal peak,
			// so this is classified as noise
			s.npkf = 0.125 * pkf + .875 * s.npkf;
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
#ifdef RTECG_LIMIT_BPM_INCREASES
		}else if((s.rr && !s.burn_avg1 && (60. / ((s.ctr - s.last_spkf.x) / (float)RTECG_FS) > RTECG_MAX_BPM_INCREASE * (60. / (s.rravg1 / RTECG_FS))))){
		        pd("\t\t-> this peak would make a heart rate of %f which is more than %fx the previous bpm: %f (%f)\n", 60. / ((s.ctr - s.last_spkf.x) / (float)RTECG_FS), RTECG_MAX_BPM_INCREASE, RTECG_MAX_BPM_INCREASE * (60. / (s.rravg1 / RTECG_FS)),(60. / (s.rr / RTECG_FS)));
			s.npkf = 0.125 * pkf + .875 * s.npkf;
			s.f1 = s.npkf + .25 * (s.spkf - s.npkf);
			s.f2 = s.f1 * .5;
#endif
		}else{
			pd("\t\t-> adding to list (%d)\n", s.ptrf);
			s.pkf[s.ptrf] = (rtecg_spk){s.ctr, pkf, maxslopef, 0};
			s.ptrf++;
		}
	}
	if(pki){
		pd("+ MWI: %u %d\n", s.ctr, pki);
		if((s.havefirstpeak && s.ctr - s.last_spkf.x < RTECG_MTOS(200))){
			pd("\t\t-> it's only been %d ms since the last peak\n", RTECG_STOM(s.ctr - s.last_spkf.x));
			// it's been less than 200ms since the last signal peak,
			// so this is classified as noise
			s.npki = 0.125 * pki + .875 * s.npki;
			s.i1 = s.npki + .25 * (s.spki - s.npki);
			s.i2 = s.i1 * .5;
		}else if(pki < s.ti2 / 2.){
			// it's less than half of the second threshold---just skip it
			;
		}else{
			pd("\t\t-> adding to list (%d)\n", s.ptri);
			s.pki[s.ptri] = (rtecg_spk){s.ctr, pki, maxslopei, 0};
			if(pki >= s.ti1){
				pd("\t\t-> %s\n", "potential signal peak");
				// this is potentially a signal peak.
				// ---the corresponding peak in the filtered signal should be RTECG_MWIDEL +- RTECG_MTOS(20) samples in the past.
				// ---update tnpkf, tf1, and tf2 for all pkf values from tptrf to ptrf - (RTECG_MWIDEL + RTECG_MTOS(20)).
				// ---compare all subsequent peaks in the filtered signal to tf1, updating local temp tnpkf and tf1 values.
				// ---if there is more than one peak, take the max
				// ---if a signal peak is found, record it, update all estimates and thresholds, and reset ptrs and ctr.
				// ---if a signal peak is not found, mwi is a noise peak---update temporary estimates and thresholds for mwi, but not filt.
				rtecg_int pkfmint = s.ctr - (RTECG_DERIVDEL + RTECG_MWIDEL);
				rtecg_int pkfmaxt = s.ctr;
				if(pkfmaxt < 0){
					pd("\t\t-> %s\n", "too early, bail");
					s.tnpki = .125 * s.pki[s.ptri].y + .875 * s.tnpki;
					s.ptri++;
					return s;
				}
				if(pkfmint < 0){
					pkfmint = 0; 
				}
				pd("\t\t-> pkfmint: %d, pkfmaxt: %d\n", pkfmint, pkfmaxt);
				pd("\t\t-> oldest peak is at %d\n", s.pkf[s.tptrf].x);
				// any peaks that are before pkfmint will be too far in the past
				// to correspond to any subsequent mwi peaks, so classify them as noise
				while(s.pkf[s.tptrf].x < pkfmint && s.tptrf < s.ptrf){
					pd("\t\t-> peak at %d (s.ptrf = %d, s.tptrf = %d) is too far away from %d\n", s.pkf[s.tptrf].x, s.ptrf, s.tptrf, pkfmint);
					s.tnpkf = 0.125 * s.pkf[s.tptrf].y + .875 * s.tnpkf;
					s.tptrf++;
				}
				s.tf1 = s.tnpkf + .25 * (s.tspkf - s.tnpkf);
				s.tf2 = s.tf1 * .5;
				rtecg_int tptrf = s.tptrf;
				rtecg_int pkmax = 0;
				rtecg_int pkidx = 0;
				pd("\t\t-> looking for peak while %d <= %d && %d < %d\n", s.pkf[tptrf].x, pkfmaxt, tptrf, s.ptrf);
				while(s.pkf[tptrf].x <= pkfmaxt && tptrf < s.ptrf){ 
					pd("\t\t-> %d >= %f = %d, %d > %d = %d\n", s.pkf[tptrf].y, s.tf1, s.pkf[tptrf].y >= s.tf1, s.pkf[tptrf].y, pkmax, s.pkf[tptrf].y > pkmax);
					// this is a small enough window that i'm too lazy to
					// update the estimates and thresholds every time.
					if(s.pkf[tptrf].y >= s.tf1 && s.pkf[tptrf].y > pkmax){
						pkmax = s.pkf[tptrf].y;
						pkidx = tptrf;
					}
					tptrf++;
				}
				if(pkmax){
					if(s.ctr - s.last_spki.x < RTECG_MTOS(360) && (float)maxslopei / (float)s.last_spki.maxslope < 0.5){
						// this is likely a T-wave
						pd("\t\t-> peak is %d samples after previous peak; more than 200ms (%d samples), but less than 360ms (%d samples). slope of previous peak is %d, slope of this peak is %d (%f)\n", s.ctr - s.last_spki.x, RTECG_MTOS(200), RTECG_MTOS(360), s.last_spki.maxslope, maxslopei, (rtecg_float)maxslopei / (rtecg_float)s.last_spki.maxslope);
						s.tnpki = 0.125 * s.pki[s.ptri].y + .875 * s.tnpki;
						s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
						s.ti2 = s.ti1 * .5;
						s.ptri++;
					}else{
						s.havepeak = 1;
						pd("\t\t-> we have a peak!\n\t\t\t\tin the filtered signal, its value is %d, and it's %d samples in the past\n\t\t\t\tin the mwi signal, its value is %d, and it's %d samples in the past\n", pkmax, s.ctr - s.pkf[pkidx].x, pki, 0);
						// compute rr interval
						if(s.havefirstpeak){
							rtecg_float rr = (s.pkf[pkidx].x - s.last_spkf.x);
							s.rr = rr;
							if(rr){
								pd("\t\t-> rr = %f\n", 60. / (rr / RTECG_FS));
							}else{
								pd("\t\t-> rr = %f\n", 0.);
							}
							// avg 1
							s.rrsum1 += rr;
							s.rrsum1 -= s.rrbuf1[s.rrptr1];
							s.rrbuf1[s.rrptr1] = rr;
							if(s.burn_avg1){
								s.rravg1 = s.rrsum1 / (rtecg_float)s.rrptr1;
							}else{
								s.rravg1 = s.rrsum1 / 8.;
							}
							if(++(s.rrptr1) == 8){
								s.rrptr1 = 0;
								s.burn_avg1 = 0;
							}
							
							if(rr){
								pd("\t\t-> avg 1 = %f\n", 60. / (s.rravg1 / RTECG_FS));
							}else{
								pd("\t\t-> avg 1 = %f\n", 0.);
							}
							if(s.burn_avg2){
								pd("\t\t-> %s", "burning avg2...\n");
								// avg 2
								s.rrsum2 += rr;
								s.rrsum2 -= s.rrbuf2[s.rrptr2];
								s.rrbuf2[s.rrptr2] = rr;
								s.rravg2 = s.rrsum2 / (rtecg_float)s.rrptr2;
								if(++(s.rrptr2) == 8){
									s.rrptr2 = 0;
									s.burn_avg2 = 0;
								}
							}else{
								// avg 2
								if(rr >= .92 * s.rravg2 && rr <= 1.16 * s.rravg2){
									pd("\t\t-> %f <= %f <= %f\n", .92 * s.rravg2, rr, 1.16 * s.rravg2);
									s.rrsum2 += rr;
									s.rrsum2 -= s.rrbuf2[s.rrptr2];
									s.rrbuf2[s.rrptr2] = rr;
									if(++(s.rrptr2) == 8){
										s.rrptr2 = 0;
									}
									s.rravg2 = s.rrsum2 / 8.;
								}
							}
							if(rr){
								pd("\t\t-> avg 2 = %f\n", 60. / (s.rravg2 / RTECG_FS));
							}else{
								pd("\t\t-> avg 2 = %f\n", 0.);
							}
						}
						// record peaks
						s.last_last_spkf = s.last_spkf;
						s.last_last_spki = s.last_spki;
						s.last_spkf = (rtecg_spk){s.pkf[pkidx].x, s.pkf[pkidx].y, s.pkf[pkidx].maxslope, 1.};
						s.last_spki = (rtecg_spk){s.pki[s.ptri].x, s.pki[s.ptri].y, s.pki[s.ptri].maxslope, 1.};
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
					
						s.havefirstpeak = 1;
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
	}
	if(s.havefirstpeak && s.havepeak == 0 && !s.burn_avg2 && rtecg_pt_last_spki(s).x > (s.rravg2 * 1.66)){
		s.searchback = 1;
	}
	
	// this seems extreme, but it will happen almost never, only when there's a signal
	// that's noisy to the point of being unusable
	if(s.ptrf == RTECG_PTBUFLEN){
		pd("%s\n", "s.ptrf == RTECG_PTBUFLEN\n");
		rtecg_int i = 1;
		while(i < s.tptrf && (s.pkf[i].x - s.last_spkf.x) > RTECG_FS * 2){
			i++;
		}
		memmove(s.pkf, s.pkf + i, RTECG_PTBUFLEN - i);
		s.ptrf--;
		s.tptrf--;
	}
	if(s.ptri == RTECG_PTBUFLEN){
		pd("%s\n", "s.ptri == RTECG_PTBUFLEN\n");
		rtecg_int i = 1;
		while((s.pki[i].x - s.last_spki.x) > RTECG_FS * 2){
			i++;
		}
		memmove(s.pki, s.pki + i, RTECG_PTBUFLEN - i);
		s.ptri--;
	}
	return s;
}

rtecg_pt rtecg_pt_searchback(rtecg_pt s)
{
	pd("%s\n", "SEARCHBACK");
	rtecg_float spkf = s.spkf;
	rtecg_float spki = s.spki;
	rtecg_float npkf = s.npkf;
	rtecg_float npki = s.npki;
	rtecg_float f1 = s.f1;
	rtecg_float f2 = s.f2;
	rtecg_float i1 = s.i1;
	rtecg_float i2 = s.i2;
	rtecg_int ptri = 0;
	rtecg_int pkimax = 0, pkiidx = -1;
	rtecg_int pkfmax = 0, pkfidx = -1;
	while(ptri < s.ptri){
		rtecg_int x = s.pki[ptri].x;
		rtecg_int y = s.pki[ptri].y;
		// we don't care about this peak if there isn't a corresponding one in the filtered signal
		rtecg_int ptrf = 0;
		rtecg_int pkfmint = x - (RTECG_DERIVDEL + RTECG_MWIDEL);
		rtecg_int pkfmaxt = x;
		if(pkfmaxt < 0){
			ptri++;
			continue;
		}
		if(pkfmint < 0){
			pkfmint = 0; 
		}
		while(s.pkf[ptrf].x < pkfmint && ptrf < s.ptrf){
			ptrf++;
		}
		while(s.pkf[ptrf].x < pkfmaxt && ptrf < s.ptrf){
			if(s.pkf[ptrf].y >= pkfmax){
				pkfmax = s.pkf[ptrf].y;
				pkfidx = ptrf;
			}
			ptrf++;
		}
		if(pkfidx == -1){
			ptri++;
			continue;
		}
		// this mwi peak has a corresponding peak in the filtered signal, so see if it's a maximum
		if(y > pkimax){
			pkimax = y;
			pkiidx = ptri;
			/*
			  }else if(y == pkimax &&
			  s.pki[ptri].x - s.last_spki.x >= .92 * s.rravg2 &&
			  s.pki[ptri].x - s.last_spki.x <= 1.16 * s.rravg2){
			  pkimax = y;
			  pkiidx = ptri;
			*/
		}
		ptri++;
	}
	if(pkiidx > -1){
		pd("\t\t-> %s\n", "we have an mwi peak with a corresponding peak in the filtered signal");
		// we have an mwi peak with a corresponding peak in the filtered signal.
		// check to see if they're both above their respective second thresholds
		s.havepeak = 1;
		ptri = 0;
		while(ptri < pkiidx){
			npki = 0.125 * s.pki[ptri].y + .875 * npki;
			ptri++;
		}
		rtecg_int ptrf = 0;
		while(ptrf < pkfidx){
			npkf = 0.125 * s.pkf[ptrf].y + .875 * npkf;
			ptrf++;
		}
		f1 = npkf + .25 * (spkf - npkf);
		f2 = f1 * .5;
		i1 = npki + .25 * (spki - npki);
		i2 = i1 * .5;
		rtecg_float c = 0.;
		if(s.pkf[pkfidx].y > f2){
			c += 1. / 3.;
		}
		if(s.pki[pkiidx].y > i2){
			c += 1. / 3.;
		}
		pd("\t\t-> %d %d, confidence: %f\n", s.pki[pkiidx].x, s.pki[pkiidx].y, c);
		s.last_last_spkf = s.last_spkf;
		s.last_last_spki = s.last_spki;
		s.last_spkf = (rtecg_spk){s.pkf[pkfidx].x, s.pkf[pkfidx].y, s.pkf[pkfidx].maxslope, c};
		s.last_spki = (rtecg_spk){s.pki[pkiidx].x, s.pki[pkiidx].y, s.pki[pkiidx].maxslope, c};
		// update signal estimates for filtered signal and mwi
		// this was found using searchback, so use the other formula
		spkf = .25 * s.pkf[pkfidx].y + .75 * spkf;
		spki = .125 * s.pki[pkiidx].y + .75 * spki;
		// transfer temp values to permanent ones
		s.spkf = s.tspkf = spkf;
		s.spki = s.tspki = spki;
		s.npkf = s.tnpkf = npkf;
		s.npki = s.tnpki = npki;
		s.f1 = s.tf1 = s.tnpkf + .25 * (s.tspkf - s.tnpkf);
		s.f2 = s.tf2 = s.f1 * .5;
		s.i1 = s.ti1 = s.tnpki + .25 * (s.tspki - s.tnpki);
		s.i2 = s.ti2 = s.i1 * .5;
		// reset pointers and counter to the found peak
		s.ptrf = pkfidx;
		s.tptrf = pkfidx;
		s.ptri = pkiidx;
					
		s.havefirstpeak = 1;
		s.searchback = 0;
	}else{
		pd("\t\t-> %s\n", "we did not find a peak during searchback");
		s.tspkf = s.spkf;
		s.tspki = s.spki;
		s.tnpkf = s.npkf;
		s.tnpki = s.npki;
		s.tf1 = s.f1;
		s.ti2 = s.i2;
		s.ti1 = s.i1;
		s.ptrf = 0;
		s.tptrf = 0;
		s.ptri = 0;
		s.havefirstpeak = 0;
	}
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
