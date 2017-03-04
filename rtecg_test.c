#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>
#include "rtecg.h"
#include "rtecg_filter.h"
#include "rtecg_peak.h"
#include "rtecg_pantompkins.h"
#include "testdat.h"

#define FS 200
#define PREBURNLEN_MS 500
#define BURNLEN_MS 2500

void lowPassFilter(double *vectorLPF, double *initialSamples, int *channelSize)
{
	int contLPF = 12;

	while( contLPF < *channelSize ){
		vectorLPF[ contLPF ]= 2 * vectorLPF[ contLPF - 1 ] - vectorLPF[ contLPF - 2 ] + initialSamples[ contLPF ] - 2 * initialSamples[ contLPF - 6 ]
			+ initialSamples[ contLPF - 12 ];
		contLPF++;
	}
}

void highPassFilter(double *vectorHPF, double *vectorLPF, int *channelSize)
{

	int contHPF = 32;

	while( contHPF < *channelSize ){
		vectorHPF[ contHPF ]= 32 * vectorLPF[ contHPF - 16 ] - ( 1.0 / 32.0 ) * ( ( vectorHPF[ contHPF - 1 ] + vectorLPF[ contHPF ] - vectorLPF[ contHPF - 32 ] ) );
		contHPF++;
	}

}

int get_max_in_range(rtecg_float *buf, rtecg_ctr mini, rtecg_ctr maxi)
{
	rtecg_float _max = buf[mini];
	rtecg_ctr _maxi = mini;
	for(int i = mini; i < maxi; i++){
		if(buf[i] > _max){
			_max = buf[i];
			_maxi = i;
		}
	}
	return _maxi;
}

int main(int ac, char **av)
{
	int LPBUFLEN = RTECG_LPBUFLEN;
	int HPBUFLEN = RTECG_HPBUFLEN;
	int DERIVLEN = RTECG_DERIVLEN;
	int MWILEN = RTECG_MWILEN;
	int PKWINLEN = RTECG_PKWINLEN;
	int PKKNEIGH = RTECG_PKKNEIGH;

	int LPDEL = RTECG_LPDEL;
	int HPDEL = RTECG_HPDEL;
	int DERIVDEL = RTECG_DERIVDEL;
	int PKDEL = RTECG_PKDEL;
	int MWIDEL = RTECG_MWIDEL;

#define ppp(x)printf(#x" = %d\n", x)
	ppp(RTECG_LPBUFLEN);
	ppp(RTECG_HPBUFLEN);
	ppp(RTECG_DERIVLEN);
	ppp(RTECG_MWILEN);
	ppp(RTECG_PKWINLEN);
	ppp(RTECG_PKKNEIGH);

	ppp(RTECG_LPDEL);
	ppp(RTECG_HPDEL);
	ppp(RTECG_DERIVDEL);
	ppp(RTECG_PKDEL);
	ppp(RTECG_MWIDEL);

	int n = sizeof(testdat) / sizeof(int);
	//char *keys[] = {"raw", "filt1", "filt2", "filt3", "filt4", "filt5", "filt6", "dsq", "mwi", "raw_peaks", "mwi_peaks"};
	char *keys[] = {"raw", "lp", "hp", "mwi", "peaksf", "peaksi", "spkf", "npkf", "spki", "npki", "f1", "f2", "i1", "i2"};
	rtecg_float out[sizeof(keys) / sizeof(char*)][n];
	memset(out, 0, (sizeof(keys) / sizeof(char*)) * n * sizeof(rtecg_float));
	for(int i = 0; i < n; i++){
		//out[0][i] = (testdat[i] / 512.) - 1.;
		out[0][i] = testdat[i];
	}

	enum{
		RAW = 0,
		LP,
		HP,
		MWI,
		PKF,
		PKI,
		SPKF,
		NPKF,
		SPKI,
		NPKI,
		F1,
		F2,
		I1,
		I2
	};
	const int FILT = HP;
	
	//int filter_order = 12;
	//rtecg_bw bw = rtecg_bw_init(0, filter_order, 10, 200, 1.0);
	rtecg_ptlp lp = rtecg_ptlp_init();
	rtecg_pthp hp = rtecg_pthp_init();
	rtecg_ptd d = rtecg_ptd_init();
	rtecg_pti mwi = rtecg_pti_init();
	rtecg_pk pkf = rtecg_pk_init();
	rtecg_pk pki = rtecg_pk_init();
	rtecg_pt pts = rtecg_pt_init();
	rtecg_pt2 pt2s = rtecg_pt2_init();

	// burn in
	rtecg_ctr ispkf1 = 0, inpkrf1 = 0, ispki1 = 0, inpkri1 = 0;
	for(int i = RTECG_MTOS(PREBURNLEN_MS); i < RTECG_MTOS(BURNLEN_MS); i++){
		// filter
		//out[LP][i] = rtecg_ptlp_hx0(out[RAW], i, n, out[LP], i, n);
		//out[HP][i] = rtecg_pthp_hx0(out[LP], i, n, out[HP], i, n);
		lp = rtecg_ptlp_hx0(lp, out[RAW][i]);
		out[LP][i] = rtecg_ptlp_y0(lp);
		hp = rtecg_pthp_hx0(hp, out[LP][i]);
		out[HP][i] = rtecg_pthp_y0(hp);
		//hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
		//out[FILT][i] = rtecg_pthp_y0(hp);
		//out[FILT][i] = rtecg_ptlp_y0(lp);
		//bw = rtecg_bw_hx0(bw, out[RAW][i]);
		//ouT[FILT][i] = rtecg_bw_y0(bw);
		// preprocess
		//out[MWI][i] = rtecg_pt_preprocess(out[HP], i, n, mwilen);
		d = rtecg_ptd_hx0(d, out[HP][i]);
		mwi = rtecg_pti_hx0(mwi, rtecg_ptd_y0(d));
		out[MWI][i] = rtecg_pti_y0(mwi);
		// filtered signal
		//out[PKF][i] = rtecg_peak0(out[FILT], i, n, 16);
		pkf = rtecg_pk_mark(pkf, rtecg_pthp_y0(hp));
		out[PKF][i] = rtecg_pk_y0(pkf);

		// mwi
		//out[PKI][i]= rtecg_peak0(out[MWI], i, n, 16);
		pki = rtecg_pk_mark(pki, rtecg_pti_y0(mwi));
		out[PKI][i] = rtecg_pk_y0(pki);
	}
	ispkf1 = get_max_in_range(out[FILT], RTECG_MTOS(PREBURNLEN_MS), RTECG_MTOS(PREBURNLEN_MS) + RTECG_MTOS(BURNLEN_MS / 2));//200, 600);
	ispki1 = get_max_in_range(out[MWI], RTECG_MTOS(PREBURNLEN_MS), RTECG_MTOS(PREBURNLEN_MS) + RTECG_MTOS(BURNLEN_MS / 2));//200, 600);
	inpkrf1 = get_max_in_range(out[FILT], ispkf1 + RTECG_MTOS(200), ispkf1 + RTECG_MTOS(360));
	inpkri1 = get_max_in_range(out[MWI], ispki1 + RTECG_MTOS(200), ispki1 + RTECG_MTOS(360));
	/*
	for(int i = 1; i < 100; i++){
		if(out[PKF][i + ispkf1]){
			inpkrf1 = i + ispkf1;
		}
		if(out[PKI][i + ispki1]){
			inpkri1 = i + ispki1;
		}
	}
	*/
	
	/* printf("%d\n", ispkf1); */
	/* printf("%d\n", inpkrf1); */
	/* printf("%d\n", ispki1); */
	/* printf("%d\n", inpkri1); */

	rtecg_ctr ispkf2 = 0, inpkrf2 = 0, ispki2 = 0, inpkri2 = 0;
	ispkf2 = get_max_in_range(out[FILT], RTECG_MTOS(PREBURNLEN_MS) + RTECG_MTOS(BURNLEN_MS / 2), RTECG_MTOS(PREBURNLEN_MS) + RTECG_MTOS(BURNLEN_MS));//200, 600);
	ispki2 = get_max_in_range(out[MWI], RTECG_MTOS(PREBURNLEN_MS) + RTECG_MTOS(BURNLEN_MS / 2), RTECG_MTOS(PREBURNLEN_MS) + RTECG_MTOS(BURNLEN_MS));//200, 600);
	inpkrf2 = get_max_in_range(out[FILT], ispkf2 + RTECG_MTOS(200), ispkf2 + RTECG_MTOS(360));
	inpkri2 = get_max_in_range(out[MWI], ispki2 + RTECG_MTOS(200), ispki2 + RTECG_MTOS(360));
	/*
	for(int i = 1; i < 100; i++){
		if(out[PKF][i + ispkf2]){
			inpkrf2 = i + ispkf2;
		}
		if(out[PKI][i + ispki2]){
			inpkri2 = i + ispki2;
		}
	}
	*/
	/* printf("******\n"); */
	/* printf("%d\n", ispkf2); */
	/* printf("%d\n", inpkrf2); */
	/* printf("%d\n", ispki2); */
	/* printf("%d\n", inpkri2); */

	memset(out[PKF], 0, n * sizeof(rtecg_float));
	memset(out[PKI], 0, n * sizeof(rtecg_float));
	out[PKF][ispkf1 + RTECG_PKDEL] = 1;
	out[PKF][inpkrf1 + RTECG_PKDEL] = 1;
	out[PKI][ispki1 + RTECG_PKDEL] = 1;
	out[PKI][inpkri1 + RTECG_PKDEL] = 1;

	out[PKF][ispkf2 + RTECG_PKDEL] = 1;
	out[PKF][inpkrf2 + RTECG_PKDEL] = 1;
	out[PKI][ispki2 + RTECG_PKDEL] = 1;
	out[PKI][inpkri2 + RTECG_PKDEL] = 1;

	pts.spkf = (out[FILT][ispkf1] + out[FILT][ispkf2]) / 2.;
	pts.npkf = (out[FILT][inpkrf1] + out[FILT][inpkrf2]) / 2.;
	pts.spki = (out[MWI][ispki1] + out[MWI][ispki2]) / 2.;
	pts.npki = (out[MWI][inpkri1] + out[MWI][inpkri2]) / 2.;
	pts.f1 = pts.npkf + .25 * (pts.spkf - pts.npkf);
	pts.f2 = pts.f1 * .5;
	pts.i1 = pts.npki + .25 * (pts.spki - pts.npki);
	pts.i2 = pts.i1 * .5;

	pt2s.tspkf = pt2s.spkf = (out[FILT][ispkf1] + out[FILT][ispkf2]) / 2.;
	pt2s.tnpkf = pt2s.npkf = (out[FILT][inpkrf1] + out[FILT][inpkrf2]) / 2.;
	pt2s.tspki = pt2s.spki = (out[MWI][ispki1] + out[MWI][ispki2]) / 2.;
	pt2s.tnpki = pt2s.npki = (out[MWI][inpkri1] + out[MWI][inpkri2]) / 2.;
	pt2s.f1 = pt2s.npkf + .25 * (pt2s.spkf - pt2s.npkf);
	pt2s.f2 = pt2s.f1 * .5;
	pt2s.i1 = pt2s.npki + .25 * (pt2s.spki - pt2s.npki);
	pt2s.i2 = pt2s.i1 * .5;

	pt2s.ctr = RTECG_MTOS(BURNLEN_MS) - 1;//1199;

	memcpy(out[SPKF], out[PKF], n * sizeof(rtecg_float));
	memcpy(out[SPKI], out[PKI], n * sizeof(rtecg_float));
	for(int i = RTECG_MTOS(BURNLEN_MS); i < n; i++){
		// filter
		//out[LP][i] = rtecg_ptlp_hx0(out[RAW], i, n, out[LP], i, n);
		//out[HP][i] = rtecg_pthp_hx0(out[LP], i, n, out[HP], i, n);
		lp = rtecg_ptlp_hx0(lp, out[RAW][i]);
		out[LP][i] = rtecg_ptlp_y0(lp);
		hp = rtecg_pthp_hx0(hp, out[LP][i]);
		out[HP][i] = rtecg_pthp_y0(hp);
		//lp = rtecg_ptlp_hx0(lp, out[RAW][i], out[RAW], i, n, out[FILT], i, n);
		//hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
		//out[FILT][i] = rtecg_pthp_y0(hp);
		//out[FILT][i] = rtecg_ptlp_y0(lp);
		//bw = rtecg_bw_hx0(bw, out[RAW][i]);
		//out[FILT][i] = rtecg_bw_y0(bw);
		// preprocess
		//out[MWI][i] = rtecg_pt_preprocess(out[HP], i, n, mwilen);
		d = rtecg_ptd_hx0(d, out[HP][i]);
		mwi = rtecg_pti_hx0(mwi, rtecg_ptd_y0(d));
		out[MWI][i] = rtecg_pti_y0(mwi);
		// filtered signal
		//out[PKF][i] = rtecg_peak0(out[FILT], i, n, 16);
		pkf = rtecg_pk_mark(pkf, rtecg_pthp_y0(hp));
		out[PKF][i] = rtecg_pk_y0(pkf);

		// mwi
		//out[PKI][i]= rtecg_peak0(out[MWI], i, n, 16);
		pki = rtecg_pk_mark(pki, rtecg_pti_y0(mwi));
		out[PKI][i] = rtecg_pk_y0(pki);

		pts = rtecg_pt_process(pts, out[HP], out[PKF], i, n, out[MWI], out[PKI], i, n);
		pt2s = rtecg_pt2_process(pt2s, rtecg_pk_y0(pkf) * rtecg_pk_xm82(pkf), rtecg_pk_maxslope(pkf), rtecg_pk_y0(pki) * rtecg_pk_xm82(pki), rtecg_pk_maxslope(pki));
	  	if(pt2s.havepeak){
			out[SPKF][i - rtecg_pt2_last_spkf(pt2s).x] = 1;
			out[SPKI][i - rtecg_pt2_last_spki(pt2s).x] = 1;
		}
		out[F1][i] = pt2s.f1;
		out[F2][i] = pt2s.f2;
		out[I1][i] = pt2s.i1;
		out[I2][i] = pt2s.i2;
		/*
		out[SPKF][pts.last_spkf] = 1;
		out[SPKI][pts.last_spki] = 1;
		out[F1][i] = pts.f1;
		out[F2][i] = pts.f2;
		out[I1][i] = pts.i1;
		out[I2][i] = pts.i2;
		*/
	}

	// write to file
	FILE *fp = fopen("testdat_dict.py", "w");
	fprintf(fp, "{");
	//fprintf(fp, "'raw_peak_win' : %d, 'mwi_peak_win' : %d, 'total_delay' : %d,", s.peak_win_size_raw, s.peak_win_size_mwi, PT_DELAY);
	fprintf(fp, "'lpfilter_delay' : %d, ", RTECG_LPDEL);
	fprintf(fp, "'hpfilter_delay' : %d, ", RTECG_LPDEL + RTECG_HPDEL);
	fprintf(fp, "'mwi_delay' : %d, ", RTECG_MWIDEL + RTECG_DERIVDEL);
	fprintf(fp, "'peak_delay' : %d, ", RTECG_PKDEL);
	for(int i = 0; i < sizeof(keys) / sizeof(char*); i++){
		fprintf(fp, "'%s' : [", keys[i]);
		for(int j = 0; j < n - 1; j++){
			fprintf(fp, "%f, ", out[i][j]);
		}
		if(i == sizeof(keys) / sizeof(char*) - 1){
			fprintf(fp, "%f]", out[i][n - 1]);
		}else{
			fprintf(fp, "%f],", out[i][n - 1]);
		}
	}
	fprintf(fp, "}");
	fclose(fp);
	return 0;
}
