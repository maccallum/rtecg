#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>
#include "rtecg.h"
#include "rtecg_filter.h"
#include "rtecg_process.h"
#include "rtecg_peak.h"
#include "rtecg_pantompkins.h"
#include "testdat.h"


#define FS 200

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
	int n = sizeof(testdat) / sizeof(int);
	//char *keys[] = {"raw", "filt1", "filt2", "filt3", "filt4", "filt5", "filt6", "dsq", "mwi", "raw_peaks", "mwi_peaks"};
	char *keys[] = {"raw", "filt", "mwi", "peaksf", "peaksi", "spkf", "npkf", "spki", "npki", "f1", "f2", "i1", "i2"};
	rtecg_float out[sizeof(keys) / sizeof(char*)][n];
	memset(out, 0, (sizeof(keys) / sizeof(char*)) * n * sizeof(rtecg_float));
	for(int i = 0; i < n; i++){
		out[0][i] = (testdat[i] / 512.) - 1.;
	}
	/*
	float a1[] = {-1.56342996066964134982, -1.58439134266949421814, -1.62651970581501825741, -1.69007216670843707362, -1.77501375864393784454, -1.88042432673360337958};
	float a2[] = {0.61173153495920273848, 0.63334051088737186586, 0.67677041379697944201, 0.74228630380993054771, 0.82985213393732903953, 0.93851932145579664013};
	float b0[] = {0.19413423252039860301, 0.18332974455631403932, 0.16161479310151033451, 0.12885684809503472614, 0.08507393303133542473, 0.03074033927210166259};
	float b1[] = {0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000};
	float b2[] = {-0.19413423252039860301, -0.18332974455631403932, -0.16161479310151033451, -0.12885684809503472614, -0.08507393303133542473, -0.03074033927210166259};
	t_bq h[6];
	for(int i = 0; i < 6; i++){
		h[i] = bq_init(1.0, a1[i], a2[i], b0[i], b1[i], b2[i]);
	}
	*/
	#define RAW 0
	#define FILT 1
	#define MWI 2
	#define PKF 3
	#define PKI 4
	#define SPKF 5
	#define NPKF 6
	#define SPKI 7
	#define NPKI 8
	#define F1 9
	#define F2 10
	#define I1 11
	#define I2 12
	//int filter_order = 12;
	//rtecg_bw bw = rtecg_bw_init(0, filter_order, 10, 256, 1.0);
	rtecg_ptlp lp = rtecg_ptlp_init();
	rtecg_pthp hp = rtecg_pthp_init();
	int mwilen = 30;
	rtecg_pt pts = rtecg_pt_init();

	// burn in
	rtecg_ctr ispkf1 = 0, inpkrf1 = 0, ispki1 = 0, inpkri1 = 0;
	for(int i = 0; i < 1200; i++){
		// filter
		lp = rtecg_ptlp_hx0(lp, out[RAW][i]);
		hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
		out[FILT][i] = rtecg_pthp_y0(hp);
		// preprocess
		out[MWI][i] = rtecg_pt_preprocess(out[FILT], i, n, mwilen);
		// filtered signal
		out[PKF][i] = rtecg_peak0(out[FILT], i, n, 16);

		// mwi
		out[PKI][i]= rtecg_peak0(out[MWI], i, n, 16);
	}
	ispkf1 = get_max_in_range(out[FILT], 200, 600);
	ispki1 = get_max_in_range(out[MWI], 200, 600);
	for(int i = 1; i < 100; i++){
		if(out[PKF][i + ispkf1]){
			inpkrf1 = i + ispkf1;
		}
		if(out[PKI][i + ispki1]){
			inpkri1 = i + ispki1;
		}
	}
	
	printf("%d\n", ispkf1);
	printf("%d\n", inpkrf1);
	printf("%d\n", ispki1);
	printf("%d\n", inpkri1);

	rtecg_ctr ispkf2 = 0, inpkrf2 = 0, ispki2 = 0, inpkri2 = 0;
	ispkf2 = get_max_in_range(out[FILT], 700, 1100);
	ispki2 = get_max_in_range(out[MWI], 700, 1100);
	for(int i = 1; i < 100; i++){
		if(out[PKF][i + ispkf2]){
			inpkrf2 = i + ispkf2;
		}
		if(out[PKI][i + ispki2]){
			inpkri2 = i + ispki2;
		}
	}
	printf("******\n");
	printf("%d\n", ispkf2);
	printf("%d\n", inpkrf2);
	printf("%d\n", ispki2);
	printf("%d\n", inpkri2);

	memset(out[PKF], 0, n * sizeof(rtecg_float));
	memset(out[PKI], 0, n * sizeof(rtecg_float));
	out[PKF][ispkf1 + 17] = 1;
	out[PKF][inpkrf1] = 1;
	out[PKI][ispki1 + 17] = 1;
	out[PKI][inpkri1] = 1;

	out[PKF][ispkf2 + 17] = 1;
	out[PKF][inpkrf2] = 1;
	out[PKI][ispki2 + 17] = 1;
	out[PKI][inpkri2] = 1;	

	pts.spkf = (out[FILT][ispkf1] + out[FILT][ispkf2]) / 2.;
	pts.npkf = (out[FILT][inpkrf1] + out[FILT][inpkrf2]) / 2.;
	pts.spki = (out[MWI][ispki1] + out[MWI][ispki2]) / 2.;
	pts.npki = (out[MWI][inpkri1] + out[MWI][inpkri2]) / 2.;
	pts.f1 = pts.npkf + .25 * (pts.spkf - pts.npkf);
	pts.f2 = pts.f1 * .5;
	pts.i1 = pts.npki + .25 * (pts.spki - pts.npki);
	pts.i2 = pts.i1 * .5;

	memcpy(out[SPKF], out[PKF], n * sizeof(rtecg_float));
	memcpy(out[SPKI], out[PKI], n * sizeof(rtecg_float));
	for(int i = 1200; i < n; i++){
		// filter
		lp = rtecg_ptlp_hx0(lp, out[RAW][i]);
		hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
		out[FILT][i] = rtecg_pthp_y0(hp);
		// preprocess
		out[MWI][i] = rtecg_pt_preprocess(out[FILT], i, n, mwilen);
		// peak identification
		// filtered signal
		out[PKF][i] = rtecg_peak0(out[FILT], i, n, 16);
		// mwi
		out[PKI][i] = rtecg_peak0(out[MWI], i, n, 16);

		pts = rtecg_pt_process(pts, out[FILT], out[PKF], i, n, out[MWI], out[PKI], i, n);
		out[SPKF][pts.last_spkf] = 1;
		out[SPKI][pts.last_spki] = 1;
		out[F1][i] = pts.f1;
		out[F2][i] = pts.f2;
		out[I1][i] = pts.i1;
		out[I1][i] = pts.i1;
	}

	// write to file
	FILE *fp = fopen("testdat_dict.py", "w");
	fprintf(fp, "{");
	//fprintf(fp, "'raw_peak_win' : %d, 'mwi_peak_win' : %d, 'total_delay' : %d,", s.peak_win_size_raw, s.peak_win_size_mwi, PT_DELAY);
	fprintf(fp, "'filter_delay' : %d, ", 21);
	fprintf(fp, "'mwi_len' : %d, ", 30);
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
