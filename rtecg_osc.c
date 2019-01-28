#include <rtecg_osc.h>
#include <lib/libo/osc_byteorder.h>
#include <lib/libo/osc_timetag.c>

#ifndef RTECG_OSC_ADDRESS_LEN
#define RTECG_OSC_ADDRESS_LEN 8 // pfx + address
#endif
#ifndef RTECG_OSC_BNDL_NPFXS
#define RTECG_OSC_BNDL_NPFXS 16
#endif
const uint8_t rtecg_osc_bndl_npfxs = RTECG_OSC_BNDL_NPFXS;
#ifndef RTECG_OSC_BNDL_PFXS
#define RTECG_OSC_BNDL_PFXS {"/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h", "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p"}
#endif
//const char *rtecg_osc_bndl_pfxs[16] = {"/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h", "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p"};
const char *rtecg_osc_bndl_pfxs[] = RTECG_OSC_BNDL_PFXS;
#ifndef RTECG_OSC_BNDL_ADDRESS
#define RTECG_OSC_BNDL_ADDRESS "/ecg"
#endif
//const char *rtecg_osc_bndl_address = "/ecg";
const char *rtecg_osc_bndl_address = RTECG_OSC_BNDL_ADDRESS;
//const char *rtecg_osc_getpfx(uint8_t id, const char *pfxs[]);

const char *rtecg_osc_getpfx(uint8_t id, uint8_t npfxs, const char **pfxs)
{
	if(id > npfxs){
		id = 0;
	}
	return pfxs[id];
}

char *rtecg_osc_bndl_typetags = ",ItiiiiItifItiffffffff\0\0";
char rtecg_osc_bndl[20 	// header and message size
		    + RTECG_OSC_ADDRESS_LEN	// address
		    + 24	// typetags
		    + 4	// packet number
		    + 8	// time
		    + 4	// sampling frequency
		    + 4 // sample width---the actual time between this sample and the last in microseconds
		    + 4	// raw
		    + 4	// filtered
		    + 4	// mwi
		    + 4	// spkf sample number
		    + 8 // spkf sample time
		    + 4	// spkf value
		    + 4	// spkf confidence
		    + 4	// spki sample number
		    + 8 // spki sample time
		    + 4	// spki value
		    + 4	// spki confidence
		    + 4	// rr
		    + 4	// avg1
		    + 4	// avg2
		    + 4	// f1
		    + 4	// f2
		    + 4	// i1
		    + 4];	// i2
int rtecg_osc_bndl_pnum;
int rtecg_osc_bndl_time;
int rtecg_osc_bndl_fs;
int rtecg_osc_bndl_width;
int rtecg_osc_bndl_raw;
int rtecg_osc_bndl_filt;
int rtecg_osc_bndl_mwi;
int rtecg_osc_bndl_spkfn;
int rtecg_osc_bndl_spkft;
int rtecg_osc_bndl_spkfv;
int rtecg_osc_bndl_spkfc;
int rtecg_osc_bndl_spkin;
int rtecg_osc_bndl_spkit;
int rtecg_osc_bndl_spkiv;
int rtecg_osc_bndl_spkic;
int rtecg_osc_bndl_rr;
int rtecg_osc_bndl_avg1;
int rtecg_osc_bndl_avg2;
int rtecg_osc_bndl_f1;
int rtecg_osc_bndl_f2;
int rtecg_osc_bndl_i1;
int rtecg_osc_bndl_i2;
int32_t rtecg_osc_bndl_size;

void rtecg_osc_init_pt(int id)
{
	rtecg_osc_bndl_pnum = 44 + RTECG_OSC_ADDRESS_LEN;
	rtecg_osc_bndl_time = rtecg_osc_bndl_pnum + 4;
	rtecg_osc_bndl_fs = rtecg_osc_bndl_time + 8;
	rtecg_osc_bndl_width = rtecg_osc_bndl_fs + 4;
	rtecg_osc_bndl_raw = rtecg_osc_bndl_width + 4;
	rtecg_osc_bndl_filt = rtecg_osc_bndl_raw + 4;
	rtecg_osc_bndl_mwi = rtecg_osc_bndl_filt + 4;
	rtecg_osc_bndl_spkfn = rtecg_osc_bndl_mwi + 4;
	rtecg_osc_bndl_spkft = rtecg_osc_bndl_spkfn + 4;
	rtecg_osc_bndl_spkfv = rtecg_osc_bndl_spkft + 8;
	rtecg_osc_bndl_spkfc = rtecg_osc_bndl_spkfv + 4;
	rtecg_osc_bndl_spkin = rtecg_osc_bndl_spkfc + 4;
	rtecg_osc_bndl_spkit = rtecg_osc_bndl_spkin + 4;
	rtecg_osc_bndl_spkiv = rtecg_osc_bndl_spkit + 8;
	rtecg_osc_bndl_spkic = rtecg_osc_bndl_spkiv + 4;
	rtecg_osc_bndl_rr = rtecg_osc_bndl_spkic + 4;
	rtecg_osc_bndl_avg1 = rtecg_osc_bndl_rr + 4;
	rtecg_osc_bndl_avg2 = rtecg_osc_bndl_avg1 + 4;
	rtecg_osc_bndl_f1 = rtecg_osc_bndl_avg2 + 4;
	rtecg_osc_bndl_f2 = rtecg_osc_bndl_f1 + 4;
	rtecg_osc_bndl_i1 = rtecg_osc_bndl_f2 + 4;
	rtecg_osc_bndl_i2 = rtecg_osc_bndl_i1 + 4;
	rtecg_osc_bndl_size = rtecg_osc_bndl_i2 + 4;
	
	memset(rtecg_osc_bndl, 0, rtecg_osc_bndl_size);
	char *ptr = rtecg_osc_bndl;
	memcpy(ptr, "#bundle\0\0\0\0\0\0\0\0\0", 16);
	ptr += 16;
	*((uint32_t *)(ptr)) = hton32(rtecg_osc_bndl_size - 20);
	ptr += 4;
	snprintf(ptr, 8, "%s%s", rtecg_osc_getpfx(id, rtecg_osc_bndl_npfxs, rtecg_osc_bndl_pfxs), rtecg_osc_bndl_address);
	ptr += 8;
	//memcpy(rtecg_osc_bndl + 20, rtecg_osc_getpfx(id, rtecg_osc_pfxs), 2);
	//memcpy(rtecg_osc_bndl + 22, rtecg_osc_address, 4);
	memcpy(ptr, rtecg_osc_bndl_typetags, 24);
}

int rtecg_osc_wrap_pt(uint32_t packet_num,
		      t_osc_timetag time,
		      int32_t fs,
		      int32_t width,
		      int32_t raw,
		      int32_t filtered,
		      int32_t mwi,
		      uint32_t spkf_sample_num,
		      t_osc_timetag spkf_sample_time,
		      int32_t spkf_val,
		      float spkf_conf,
		      uint32_t spki_sample_num,
		      t_osc_timetag spki_sample_time,
		      int32_t spki_val,
		      float spki_conf,
		      float rr,
		      float rr_avg1,
		      float rr_avg2,
		      float f1,
		      float f2,
		      float i1,
		      float i2,
		      char **oscbndl)
{
	*((uint32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_pnum)) = hton32(packet_num);
	//*((uint32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_micros)) = hton32(micros);
	osc_timetag_encodeForHeader(time, rtecg_osc_bndl + rtecg_osc_bndl_time);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_fs)) = hton32(fs);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_width)) = hton32(width);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_raw)) = hton32(raw);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_filt)) = hton32(filtered);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_mwi)) = hton32(mwi);
	*((uint32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_spkfn)) = hton32(spkf_sample_num);
	osc_timetag_encodeForHeader(spkf_sample_time, rtecg_osc_bndl + rtecg_osc_bndl_spkft);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_spkfv)) = hton32(spkf_val);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_spkfc)) = hton32(*((int32_t *)&(spkf_conf)));
	*((uint32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_spkin)) = hton32(spki_sample_num);
	osc_timetag_encodeForHeader(spki_sample_time, rtecg_osc_bndl + rtecg_osc_bndl_spkit);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_spkiv)) = hton32(spki_val);
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_spkic)) = hton32(*((int32_t *)&(spki_conf)));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_rr)) = hton32(*((int32_t *)&rr));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_avg1)) = hton32(*((int32_t *)&rr_avg1));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_avg2)) = hton32(*((int32_t *)&rr_avg2));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_f1)) = hton32(*((int32_t *)&(f1)));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_f2)) = hton32(*((int32_t *)&(f2)));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_i1)) = hton32(*((int32_t *)&(i1)));
	*((int32_t *)(rtecg_osc_bndl + rtecg_osc_bndl_i2)) = hton32(*((int32_t *)&(i2)));
	*oscbndl = rtecg_osc_bndl;
	return rtecg_osc_bndl_size;
}
