#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>
#include "rtecg.h"
#include "rtecg_filter.h"
#include "rtecg_pantompkins.h"
#include "testdat2.h"

#define FS 200
#define PREBURNLEN_MS 500
#define BURNLEN_MS 2500

void print_defines(void);

int main(int ac, char **av)
{
	print_defines();
	int n = sizeof(testdat) / sizeof(int);
	//n *= 2;
	char *keys[] = {"raw", "lp", "hp", "mwi", "peaksf", "peaksi", "spkf", "npkf", "spki", "npki", "f1", "f2", "i1", "i2", "rr", "rravg1", "rravg2"};
	rtecg_float out[sizeof(keys) / sizeof(char*)][n];
	memset(out, 0, (sizeof(keys) / sizeof(char*)) * n * sizeof(rtecg_float));
	for(int i = 0; i < n; i++){
		out[0][i] = testdat[i];
		//out[0][i] = (rand() / (double)RAND_MAX) * 1023.;
	}
	/* for(int i = n / 2; i < n; i++){ */
	/* 	out[0][i] = testdat[i - (n / 2)]; */
	/* 	//out[0][i] = rand(); */
	/* } */

	enum{
		RAW = 0, LP, HP, MWI, PKF, PKI, SPKF, NPKF, SPKI, NPKI, F1, F2, I1, I2, RR, RRAVG1, RRAVG2
	};
	
	rtecg_ptlp lp = rtecg_ptlp_init();
	rtecg_pthp hp = rtecg_pthp_init();
	rtecg_ptd d = rtecg_ptd_init();
	rtecg_pti mwi = rtecg_pti_init();
	rtecg_pk pkf = rtecg_pk_init();
	rtecg_pk pki = rtecg_pk_init();
	rtecg_pt pts = rtecg_pt_init();

	for(int i = 0; i < n; i++){
		// filter
		lp = rtecg_ptlp_hx0(lp, out[RAW][i]);
		out[LP][i] = rtecg_ptlp_y0(lp);
		hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
		out[HP][i] = rtecg_pthp_y0(hp);
		d = rtecg_ptd_hx0(d, rtecg_pthp_y0(hp));
		mwi = rtecg_pti_hx0(mwi, rtecg_ptd_y0(d));
		out[MWI][i] = rtecg_pti_y0(mwi);
		
		// peaks
		// filtered signal
		pkf = rtecg_pk_mark(pkf, rtecg_pthp_y0(hp));
		out[PKF][i] = rtecg_pk_y0(pkf);

		// mwi
		pki = rtecg_pk_mark(pki, rtecg_pti_y0(mwi));
		out[PKI][i] = rtecg_pk_y0(pki);
		
		// classification
		pts = rtecg_pt_process(pts, rtecg_pk_y0(pkf) * rtecg_pk_xm82(pkf), rtecg_pk_maxslope(pkf), rtecg_pk_y0(pki) * rtecg_pk_xm82(pki), rtecg_pk_maxslope(pki));
		if(pts.searchback){
			pts = rtecg_pt_searchback(pts);
		}
	  	if(pts.havepeak){
			out[SPKF][i - rtecg_pt_last_spkf(pts).x] = 1;
			out[SPKI][i - rtecg_pt_last_spki(pts).x] = 1;
		}
		out[F1][i] = pts.f1;
		out[F2][i] = pts.f2;
		out[I1][i] = pts.i1;
		out[I2][i] = pts.i2;
		out[RR][i] = pts.rr ? 60. / (pts.rr / RTECG_FS) : 0;
		out[RRAVG1][i] = pts.rravg1 ? 60. / (pts.rravg1 / RTECG_FS) : 0;
		out[RRAVG2][i] = pts.rravg2 ? 60. / (pts.rravg2 / RTECG_FS) : 0;
	}

	// write to file
	FILE *fp = fopen("testdat_dict.py", "w");
	fprintf(fp, "{");
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

void print_defines(void)
{
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
}
