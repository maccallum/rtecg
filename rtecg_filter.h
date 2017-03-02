#ifndef __RTECG_FILTER_H__
#define __RTECG_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rtecg_ptlp
{
	rtecg_int y0, y1, y2;
	rtecg_int xs[RTECG_LPBUFLEN];
	rtecg_int xm24, xm48;
} rtecg_ptlp;

typedef struct _rtecg_pthp
{
	rtecg_int y, z;
	rtecg_int xs[RTECG_HPBUFLEN];
	rtecg_int xm64, xm128;
} rtecg_pthp;

typedef struct _rtecg_ptd
{
	rtecg_int y;
	rtecg_int xs[RTECG_DERIVLEN];
	rtecg_int xm1;
} rtecg_ptd;

typedef struct _rtecg_ptmwi
{
	rtecg_int sum;
	rtecg_int xs[RTECG_MWILEN];
	rtecg_int ptr;
} rtecg_ptmwi;

/*
typedef struct _rtecg_bq
{
	rtecg_float xs[3];
	rtecg_float ys[3];
	rtecg_float a1, a2, b0, b1, b2;
	rtecg_float gain;
} rtecg_bq;

typedef struct _rtecg_bw
{
	int n;
	rtecg_bq bq[24];
} rtecg_bw;
*/

rtecg_ptlp rtecg_ptlp_init(void);
rtecg_ptlp rtecg_ptlp_hx0(rtecg_ptlp s, rtecg_int x0);
rtecg_int rtecg_ptlp_y0(rtecg_ptlp s);

rtecg_pthp rtecg_pthp_init(void);
rtecg_pthp rtecg_pthp_hx0(rtecg_pthp s, rtecg_int x0);
rtecg_int rtecg_pthp_y0(rtecg_pthp s);

rtecg_ptd rtecg_ptd_init(void);
rtecg_ptd rtecg_ptd_hx0(rtecg_ptd s, rtecg_int x0);
rtecg_int rtecg_ptd_y0(rtecg_ptd s);

rtecg_ptmwi rtecg_ptmwi_init(void);
rtecg_ptmwi rtecg_ptmwi_hx0(rtecg_ptmwi s, rtecg_int x0);
rtecg_int rtecg_ptmwi_y0(rtecg_ptmwi s);

/*
rtecg_bq rtecg_bq_init(rtecg_float gain, rtecg_float a1, rtecg_float a2, rtecg_float b0, rtecg_float b1, rtecg_float b2);
rtecg_bq rtecg_bq_hx0(rtecg_bq s, rtecg_float x0);
rtecg_float rtecg_bq_y0(rtecg_bq s);

rtecg_bw rtecg_bw_init(int type, int order, rtecg_float fc, rtecg_float fs, rtecg_float gain);
rtecg_bw rtecg_bw_hx0(rtecg_bw s, rtecg_float x0);
rtecg_float rtecg_bw_y0(rtecg_bw s);
*/

#ifdef __cplusplus
}
#endif
#endif
