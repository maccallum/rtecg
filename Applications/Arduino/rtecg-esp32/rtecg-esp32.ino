#include <WiFi.h>
#include <WiFiUdp.h>
//#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoOTA.h>
#include "rtecg.h"
#include "rtecg_filter.h"
#include "rtecg_pantompkins.h"

//#define ECG_DEBUG
#define ECG_WIFI
#define ECG_SERIAL

int ecg_send_full = 1;

typedef struct _osctime
{
	uint32_t sec;
	uint32_t frac_sec;
} osctime;

const char *osc_pfxs[16] = {"/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h", "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p"};
const char *get_osc_pfx(uint8_t id)
{
	if(id > 15){
		id = 0;
	}
	return osc_pfxs[id];
}

uint8_t id = 0;

#ifdef ECG_WIFI
// WIFI AP
//const char *ssid = "TP-LINK_40FE00";
//const char *pass = "78457393";
const char *ssid = "aether";
const char *pass = "industrialmusic";
//const char *ssid = "CLOUDnet";
//const char *pass = "draadloos";
//const char *peak_ip = "192.168.0.200";
//const char *peak_ip = "192.168.0.15";
IPAddress peak_ip(192, 168, 0, 15);
const unsigned int peak_port = 9999;

IPAddress full_ip(192, 168, 0, 15);
const unsigned int full_port = 9998;          // remote port to receive OSC

IPAddress remote_ip(192, 168, 0, 15);
const unsigned int remote_port = peak_port;

// prefix for all OSC addresses
char *oscpfx = "/ad"; // must be 3 or fewer const uint8_tacters

// main OSC bundle
char *oscbndl_address = "/ecg";
char *oscbndl_typetags = ",ItiiiiItifItiffffffff\0\0";
char oscbndl[20 	// header and message size
	     + 8	// address
	     + 24	// typetags
	     + 4	// packet number
	     + 8	// time
	     + 4	// sampling frequency
	     + 4	// raw
	     + 4	// filtered
	     + 4	// mwi
	     + 4	// spkf sample number
	     + 8	// spkf time
	     + 4	// spkf value
	     + 4	// spkf confidence
	     + 4	// spki sample number
	     + 8	// spki time
	     + 4	// spki value
	     + 4	// spki confidence
	     + 4	// rr
	     + 4	// avg1
	     + 4	// avg2
	     + 4	// f1
	     + 4	// f2
	     + 4	// i1
	     + 4];	// i2
int oscbndl_pnum = 52;
int oscbndl_time = oscbndl_pnum + 4;
int oscbndl_fs = oscbndl_time + 8;
int oscbndl_raw = oscbndl_fs + 4;
int oscbndl_filt = oscbndl_raw + 4;
int oscbndl_mwi = oscbndl_filt + 4;
int oscbndl_spkfn = oscbndl_mwi + 4;
int oscbndl_spkft = oscbndl_spkfn + 4;
int oscbndl_spkfv = oscbndl_spkft + 8;
int oscbndl_spkfc = oscbndl_spkfv + 4;
int oscbndl_spkin = oscbndl_spkfc + 4;
int oscbndl_spkit = oscbndl_spkin + 4;
int oscbndl_spkiv = oscbndl_spkit + 8;
int oscbndl_spkic = oscbndl_spkiv + 4;
int oscbndl_rr = oscbndl_spkic + 4;
int oscbndl_avg1 = oscbndl_rr + 4;
int oscbndl_avg2 = oscbndl_avg1 + 4;
int oscbndl_f1 = oscbndl_avg2 + 4;
int oscbndl_f2 = oscbndl_f1 + 4;
int oscbndl_i1 = oscbndl_f2 + 4;
int oscbndl_i2 = oscbndl_i1 + 4;
int32_t oscbndl_size = oscbndl_i2 + 4;

#define BUFINIT_BUNDLE '#', 'b', 'u', 'n', 'd', 'l', 'e', 0
#define BUFINIT_TIMETAG 0, 0, 0, 0, 0, 0, 0, 0
#define BUFINIT_SIZE_8BIT(s) 0, 0, 0, s

int didreset = 1;
osctime reset_time = (osctime){0, 0};
// bundle to send when we need to report a reset or that the flash button was pressed
char *resetaddr = "/reset";
char *flashaddr = "/flash";
char statusbndl[] = {BUFINIT_BUNDLE, BUFINIT_TIMETAG, BUFINIT_SIZE_8BIT(24), oscpfx[0], oscpfx[1], oscpfx[2], '/', 'r', 'e', 's', 'e', 't', 0, 0, 0, ',', 't', 0, 0, BUFINIT_TIMETAG};

#define NSYNCNTP 8
#define SYNCNTP_IVAL_MICROS 2000000
int ntp_syncing = 0;

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
osctime theta_min = (osctime){0xFFFFFFFF, 0xFFFFFFFF};
osctime delta_theta_min = (osctime){0, 0};
int nsyncntp = 0;
uint32_t lastntpsync_micros = 0;

// WiFi remote host
WiFiUDP udp;                                // A UDP instance to let us send and receive packets over UDP

IPAddress ntp_time_server(209, 208, 79, 69); // pool.ntp.org
const int timeZone = 0;
#endif

// state of classifier etc
uint32_t tmicros;
rtecg_ptlp lp;
rtecg_pthp hp;
rtecg_ptd d;
rtecg_pti mwi;
rtecg_pk pkf;
rtecg_pk pki;
rtecg_pt pts;
osctime tlst[RTECG_FS * 2];
int tptr = 0;
// int ntimestampbndls = 0;
// uint32_t lasttimestampbndl_micros = 0;
uint32_t tmicros_prev, tmicros_ival;

/* #ifdef ECG_SERIAL */
/* #include <SLIPEncodedSerial.h> */
/* SLIPEncodedSerial SLIPSerial(Serial); */
/* #endif // ECG_SERIAL */

//const int pin_ss = 5; // slave select pin
const int pin_sqw = 34; // square wave pin
const int pin_led = 13; // GPIO pin that controls the LED 
//const int pin_flash = pin_led; // GPIO pin that the flash button is wired to
const int pin_id1 = 39;
const int pin_id2 = 36;
const int pin_id3 = 4;
//const int pin_id4 = 0;

// used by a1 isr
volatile uint32_t micros_ref = 0;
osctime current_date;
volatile int int_a1 = 0;

// used by flash button isr
volatile int int_flash = 0;
volatile uint32_t int_flash_down_micros = 0;
volatile uint32_t int_flash_up_micros = 0;

// endianness...
#define OSC_BYTE_SWAP16(x)			\
	((((x) & 0xff00) >> 8) |		\
	 (((x) & 0x00ff) << 8))

#define OSC_BYTE_SWAP32(x)			\
	((((x) & 0xff000000) >> 24) |		\
	 (((x) & 0x00ff0000) >> 8) |		\
	 (((x) & 0x0000ff00) << 8) |		\
	 (((x) & 0x000000ff) << 24))

#define OSC_BYTE_SWAP64(x)			\
	((((x) & 0xff00000000000000LL) >> 56) | \
	 (((x) & 0x00ff000000000000LL) >> 40) | \
	 (((x) & 0x0000ff0000000000LL) >> 24) | \
	 (((x) & 0x000000ff00000000LL) >> 8)  | \
	 (((x) & 0x00000000ff000000LL) << 8)  | \
	 (((x) & 0x0000000000ff0000LL) << 24) | \
	 (((x) & 0x000000000000ff00LL) << 40) | \
	 (((x) & 0x00000000000000ffLL) << 56))

#ifdef BYTE_ORDER
#define OSC_BYTE_ORDER BYTE_ORDER
#else
#ifdef __BYTE_ORDER
#define OSC_BYTE_ORDER __BYTE_ORDER
#endif
#endif

#ifdef LITTLE_ENDIAN
#define OSC_LITTLE_ENDIAN LITTLE_ENDIAN
#else
#ifdef __LITTLE_ENDIAN
#define OSC_LITTLE_ENDIAN __LITTLE_ENDIAN
#endif
#endif

#if OSC_BYTE_ORDER == OSC_LITTLE_ENDIAN
#define hton16(x) OSC_BYTE_SWAP16(x)
#define ntoh16(x) OSC_BYTE_SWAP16(x)
#define hton32(x) OSC_BYTE_SWAP32(x)
#define ntoh32(x) OSC_BYTE_SWAP32(x)
#define hton64(x) OSC_BYTE_SWAP64(x)
#define ntoh64(x) OSC_BYTE_SWAP64(x)

#define htonf32(x) OSC_BYTE_SWAP32(*((uint32_t *)&x))
#define ntohf32(x) OSC_BYTE_SWAP32(*((uint32_t *)&x))
#define htonf64(x) OSC_BYTE_SWAP64(*((uint64_t *)&x))
#define ntohf64(x) OSC_BYTE_SWAP64(*((uint64_t *)&x))

#else

#define hton16(x) (x)
#define ntoh16(x) (x)
#define hton32(x) (x)
#define ntoh32(x) (x)
#define hton64(x) (x)
#define ntoh64(x) (x)

#define htonf32(x) (x)
#define ntohf32(x) (x)
#define htonf64(x) (x)
#define ntohf64(x) (x)

#endif

RTC_DS3231 rtc;

// // control register masks
// #define DS3234_A1IE 0x1
// #define DS3234_A2IE 0x2
// #define DS3234_INTCN 0x4
// #define DS3234_RS1 0x8
// #define DS3234_RS2 0x10
// #define DS3234_CONV 0x20
// #define DS3234_BBSQW 0x40
// #define DS3234_EOSC 0x80

// // status register masks
// #define DS3234_A1F 0x1
// #define DS3234_A2F 0x2

// #define DS3234_CREG_WRITE 0x8E // write to the control register
// #define DS3234_CREG_READ 0x0E // read to the control register
// #define DS3234_SREG_WRITE 0x8F // write to the status register
// #define DS3234_SREG_READ 0x0F // read the status register

// struct DS3234_date
// {
// 	uint8_t sec, min, hour, wday, mday, mon_plus_cent, year;
// };

// // utility functions for communicating with the DS3234
// void DS3234_set_reg(const uint8_t addr, const uint8_t val)
// {
// 	digitalWrite(pin_ss, LOW);
// 	SPI.transfer(addr);
// 	SPI.transfer(val);
// 	digitalWrite(pin_ss, HIGH);
// }

// uint8_t DS3234_get_reg(const uint8_t addr)
// {
// 	uint8_t rv;

// 	digitalWrite(pin_ss, LOW);
// 	SPI.transfer(addr);
// 	rv = SPI.transfer(0x00);
// 	digitalWrite(pin_ss, HIGH);
// 	return rv;
// }

// struct DS3234_date DS3234_get_date()
// {
// 	struct DS3234_date d;
// 	char *dc = (char *)(&d);
// 	for(int i = 0; i < 7; i++){
// 		digitalWrite(pin_ss, LOW);
// 		SPI.transfer(i);
// 		dc[i] = SPI.transfer(0);
// 		digitalWrite(pin_ss, HIGH);
// 	}
// 	return d;
// }

// void DS3234_set_date(struct DS3234_date d)
// {
// 	char *dc = (char *)(&d);	
// 	for(int i = 0; i < 7; i++){
// 		digitalWrite(pin_ss, LOW);
// 		SPI.transfer(i + 0x80);
// 		SPI.transfer(dc[i]);
// 		digitalWrite(pin_ss, HIGH);
// 	}
// }

// uint8_t dectobcd(const uint8_t val)
// {
// 	return ((val / 10 * 16) + (val % 10));
// }

// uint8_t bcdtodec(const uint8_t val)
// {
// 	return ((val / 16 * 10) + (val % 16));
// }

// struct DS3234_date make_DS3234_date(uint8_t sec,
// 				    uint8_t min,
// 				    uint8_t hour,
// 				    uint8_t wday,
// 				    uint8_t mday,
// 				    uint8_t mon,
// 				    uint16_t year)
// {
// 	int century = year >= 2000 ? 1 : 0;
// 	struct DS3234_date d = (struct DS3234_date){
// 		dectobcd(sec),
// 		dectobcd(min),
// 		dectobcd(hour),
// 		dectobcd(wday),
// 		dectobcd(mday),
// 		//mday,
// 		dectobcd(mon) + century * 0x80,
// 		dectobcd((uint8_t)(year - (1900 + (century * 100))))
// 	};
// 	return d;
// }

void date_to_string(DateTime d, long buflen, char *buf)
{
	snprintf(buf,
		 buflen,
		 "%d.%02d.%02d %02d:%02d:%02d", d.year(), d.month(), d.day(), d.hour(), d.minute(), d.second());
}

osctime date_to_osctime(DateTime d)
{
	//uint8_t i;
	//uint16_t day;
	//int16_t year;
	//int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	osctime time = {0, 0};

	// int d_year = bcdtodec(d.year()) + (1900 + (((d.mon_plus_cent & 0x80) >> 7) * 100));
	// if (d_year >= 2000) {
	// 	year = (d_year - 2000) + 100;
	// } else {
	// 	return time;
	// }

	//int d_mon = bcdtodec(d.mon_plus_cent & 0x7F);
	//day = bcdtodec(d.mday) - 1;
	// for(i = 1; i < d_mon; i++){
	// 	day += days_in_month[i - 1];
	// }
	// if(d_mon > 2 && year % 4 == 0){
	// 	day++;
	// }
	// count leap days
	//day += (365 * year + (year + 3) / 4);

	//time.sec = ((day * 24UL + bcdtodec(d.hour)) * 60 + bcdtodec(d.min)) * 60 + bcdtodec(d.sec);// + 946684800;
	time.sec = d.unixtime() + 2208988800L;
	return time;
}

osctime timenow()
{
	noInterrupts();
	osctime now = current_date;//DS3234_date_to_osctime(DS3234_get_date());
	now.frac_sec = (uint32_t)(0xFFFFFFFFUL * ((micros() - micros_ref) / 1000000.));
	interrupts();
	return now;
}

// flags are: A1M1 (seconds), A1M2 (minutes), A1M3 (hour),
// A1M4 (day) 0 to enable, 1 to disable, DY/DT (dayofweek == 1/dayofmonth == 0)
// void DS3234_set_a1(const uint8_t pin, const uint8_t s, const uint8_t mi, const uint8_t h, const uint8_t d, const uint8_t * flags)
// {
// 	uint8_t t[4] = { s, mi, h, d };
// 	uint8_t i;

// 	for (i = 0; i < 4; i++) {
// 		digitalWrite(pin, LOW);
// 		SPI.transfer(i + 0x87);
// 		SPI.transfer(dectobcd(t[i]) | (flags[i] << 7));
// 		digitalWrite(pin, HIGH);
// 	}
// }

void osc_timetag_encodeForHeader(osctime t, char *buf)
{
	if(!buf){
		return;
	}
	char *p1 = buf;
	char *p2 = buf + 4;
	char *ttp1 = (char *)&t;
	char *ttp2 = ttp1 + 4;

	*((uint32_t *)p1) = hton32(*((uint32_t *)ttp1));
	*((uint32_t *)p2) = hton32(*((uint32_t *)ttp2));
}

osctime osc_timetag_decodeFromHeader(char *buf)
{
	if(!buf){
		return (osctime){0., 0.};
	}
	char *p1 = buf;
	char *p2 = buf + 4;
	osctime tt = {0., 0.};
	char *ttp1 = (char *)&tt;
	char *ttp2 = ttp1 + 4;
	*((uint32_t *)ttp1) = ntoh32(*((uint32_t *)p1));
	*((uint32_t *)ttp2) = ntoh32(*((uint32_t *)p2));
	return tt;
}

osctime osc_timetag_add(osctime t1, osctime t2)
{
	osctime t1_ntp = *((osctime *)(&t1));
	osctime t2_ntp = *((osctime *)(&t2));
	osctime r;
	r.sec = t1_ntp.sec + t2_ntp.sec;
	r.frac_sec = t1_ntp.frac_sec + t2_ntp.frac_sec;

	if(r.frac_sec < t1_ntp.frac_sec) { // rollover occurred
		r.sec += 1;
	}
	return *((osctime *)(&r));
}

osctime osc_timetag_subtract(osctime t1, osctime t2)
{
	osctime t1_ntp = *((osctime *)(&t1));
	osctime t2_ntp = *((osctime *)(&t2));
	osctime r;

	if(t1_ntp.sec > t2_ntp.sec || (t1_ntp.sec == t2_ntp.sec && t1_ntp.frac_sec >= t2_ntp.frac_sec)){
		r.sec = t1_ntp.sec - t2_ntp.sec;
		if(t1_ntp.frac_sec >= t2_ntp.frac_sec){
			r.frac_sec = t1_ntp.frac_sec - t2_ntp.frac_sec;
		}else{
			if(r.sec == 0){
				r.frac_sec = t2_ntp.frac_sec - t1_ntp.frac_sec;
			}else{
				r.sec--;
				r.frac_sec = t1_ntp.frac_sec - t2_ntp.frac_sec;
			}
		}
	}else{
		r.sec = t2_ntp.sec - t1_ntp.sec;
		if(t1_ntp.frac_sec >= t2_ntp.frac_sec){
			r.frac_sec = t1_ntp.frac_sec - t2_ntp.frac_sec;
		}else{
			r.frac_sec = t2_ntp.frac_sec - t1_ntp.frac_sec;
		}
	}

	return *((osctime *)(&r));
}

int osc_timetag_compare(osctime t1, osctime t2)
{
	osctime t1_ntp = *((osctime *)(&t1));
	osctime t2_ntp = *((osctime *)(&t2));
	if(t1_ntp.sec < t2_ntp.sec || (t1_ntp.sec == t2_ntp.sec && t1_ntp.frac_sec < t2_ntp.frac_sec)) {
		return -1;
	}

	if(t1_ntp.sec > t2_ntp.sec || (t1_ntp.sec == t2_ntp.sec && t1_ntp.frac_sec > t2_ntp.frac_sec)) {
		return 1;
	}

	return 0;
}

osctime float_to_osctime(double d)
{
	double sec;
	double frac_sec;

	//frac_sec = fmod(d, 1.0);
	frac_sec = d - (uint64_t)d;
	sec = d - frac_sec;

	osctime n;
	n.sec = (uint32_t)(sec);
	n.frac_sec= (uint32_t)(frac_sec * 4294967295.0);
	return n;
}

double osctime_to_float(osctime n)
{
	return ((double)(n.sec)) + ((double)((uint64_t)(n.frac_sec))) / 4294967295.0;
}

//Function isleap:  Determines if year is a leap year or not, without using modulo.
//Year is a leap year if year mod 400 = 0, OR if (year mod 4 = 0 AND year mod 100 <> 0).
unsigned short int osc_timetag_isleap(unsigned short int year)
{
	short int yrtest;
	yrtest=year;

	//year modulo 400 = 0? if so, it's a leap year
	while(yrtest>0){
		yrtest-=400;
	}
	if(yrtest==0){ //then year modulo 400 = 0
		return 1; //it's a leap year
	}
	yrtest=year;

	//year modulo 4 = 0 and year modulo 100 <>0? if so, it's a leap year
	while(yrtest>0){
		yrtest-=4;
	}
	if(yrtest==0){ //then year modulo 4 = 0
		yrtest=year;
		while(yrtest>0){  //so test for modulo 100
			yrtest-=100;
		}
		if(yrtest<0){  //then year modulo 100 <>0
			return 1; //it's a leap year
		}else{
			return 0; //not a leap year
		}
	}else{
		return 0;  //year modulo 4 <> 0, not a leap year
	}
}

//Function getmonth computes the month and day of month given the day of the year,
//accounting for leap years
unsigned short int osc_timetag_getmonth(unsigned short int *day, unsigned short int leap)
{
	const unsigned short int mJAN = 1;
	const unsigned short int mFEB = 2;
	const unsigned short int mMAR = 3;
	const unsigned short int mAPR = 4;
	const unsigned short int mMAY = 5;
	const unsigned short int mJUN = 6;
	const unsigned short int mJUL = 7;
	const unsigned short int mAUG = 8;
	const unsigned short int mSEP = 9;
	const unsigned short int mOCT = 10;
	const unsigned short int mNOV = 11;
	const unsigned short int mDEC = 12;

	unsigned short int JAN_LAST = 31;
	unsigned short int FEB_LAST = 59;
	unsigned short int MAR_LAST = 90;
	unsigned short int APR_LAST = 120;
	unsigned short int MAY_LAST = 151;
	unsigned short int JUN_LAST = 181;
	unsigned short int JUL_LAST = 212;
	unsigned short int AUG_LAST = 243;
	unsigned short int SEP_LAST = 273;
	unsigned short int OCT_LAST = 304;
	unsigned short int NOV_LAST = 334;
	unsigned short int DEC_LAST = 365;

	//correct monthly end dates for leap years (leap=1=leap year, 0 otherwise)
	if(leap > 0){
		if(leap <= 1){
			FEB_LAST += leap;
			MAR_LAST += leap;
			APR_LAST += leap;
			MAY_LAST += leap;
			JUN_LAST += leap;
			JUL_LAST += leap;
			AUG_LAST += leap;
			SEP_LAST += leap;
			OCT_LAST += leap;
			NOV_LAST += leap;
			DEC_LAST += leap;

		}else if(leap > 1){ //error condition
			return 0;
		}
	}else if(leap < 0){  //error condition
		return 0;
	}
	//Determine month & day of month from day of year
	if(*day <= JAN_LAST){
		return mJAN;  //day is already correct
	}else if(*day <= FEB_LAST){
		*day -= JAN_LAST;
		return mFEB;
	}else if(*day <= MAR_LAST){
		*day -= (FEB_LAST);
		return mMAR;
	}else if(*day <= APR_LAST){
		*day -= (MAR_LAST);
		return mAPR;
	}else if(*day <= MAY_LAST){
		*day -= (APR_LAST);
		return mMAY;
	}else if(*day <= JUN_LAST){
		*day -= (MAY_LAST);
		return mJUN;
	}else if(*day <= JUL_LAST){
		*day -= (JUN_LAST);
		return mJUL;
	}else if(*day <= AUG_LAST){
		*day -= (JUL_LAST);
		return mAUG;
	}else if(*day <= SEP_LAST){
		*day -= (AUG_LAST);
		return mSEP;
	}else if(*day <= OCT_LAST){
		*day -= (SEP_LAST);
		return mOCT;
	}else if(*day <= NOV_LAST){
		*day -= (OCT_LAST);
		return mNOV;
	}else if(*day <= DEC_LAST){
		*day -= (NOV_LAST);
		return mDEC;
	}else{
		return 0;
	}
}

#define DWORD uint64_t
DateTime osctime_to_date(osctime timetag)
{
	char s1[20];
	uint32_t secs = timetag.sec;//osc_timetag_ntp_getSeconds(timetag);
	unsigned short int year, month, day, hour, minute, yrcount, leap = 0;

	const DWORD SEC_PER_YEAR = 31536000;
	const DWORD SEC_PER_DAY = 86400;
	const unsigned short int SEC_PER_HR = 3600;
	const unsigned short int SEC_PER_MIN = 60;


	//secs=-2208988800;//SNTPGetUTCSeconds();
	//secs = abs(secs);
	for(year = 0; secs >= SEC_PER_YEAR; year++){ //determine # years elapse since epoch
		secs -= SEC_PER_YEAR;
		if(osc_timetag_isleap(year)){
			secs -= SEC_PER_DAY;
		}
	}
	//year+=1970;  //1/1/1970, 00:00 is epoch
	year += 1900;  //1/1/1900, 00:00 is ntp epoch
	//for (yrcount=1970;yrcount<year;yrcount++) //scroll from 1970 to last year to find leap yrs.
	/*
      for(yrcount = 1900; yrcount < year; yrcount++){
      leap = osc_timetag_isleap(yrcount);  
      if(leap == 1){
      secs -= SEC_PER_DAY;  //if it's a leap year, subtract a day's worth of seconds
      }
      } 
	*/
	leap = osc_timetag_isleap(year); //Is this a leap year?
	unsigned short int JAN_LAST = 31;
	unsigned short int FEB_LAST = 59;
	unsigned short int MAR_LAST = 90;
	unsigned short int APR_LAST = 120;
	unsigned short int MAY_LAST = 151;
	unsigned short int JUN_LAST = 181;
	unsigned short int JUL_LAST = 212;
	unsigned short int AUG_LAST = 243;
	unsigned short int SEP_LAST = 273;
	unsigned short int OCT_LAST = 304;
	unsigned short int NOV_LAST = 334;
	unsigned short int DEC_LAST = 365;
	if(leap <= 1){
		FEB_LAST += leap;
		MAR_LAST += leap;
		APR_LAST += leap;
		MAY_LAST += leap;
		JUN_LAST += leap;
		JUL_LAST += leap;
		AUG_LAST += leap;
		SEP_LAST += leap;
		OCT_LAST += leap;
		NOV_LAST += leap;
		DEC_LAST += leap;
	}

	for(day = 1; secs >= SEC_PER_DAY; day++){ //determine # days elapsed in current year
		secs -= SEC_PER_DAY;
	}
	unsigned short int months[] = {JAN_LAST, FEB_LAST, MAR_LAST, APR_LAST, MAY_LAST, JUN_LAST, JUL_LAST, AUG_LAST, SEP_LAST, OCT_LAST, NOV_LAST, DEC_LAST};
	unsigned short int mday = day;
	for(int i = 0; i < 11; i++){
		if(day < months[i] && i > 0){
			mday -= months[i - 1];
			break;
		}
	}
	mday -= 1;

	for(hour = 0; secs >= SEC_PER_HR; hour++){  //determine hours elapsed in current day
		secs -= SEC_PER_HR;
	}
	for(minute = 0; secs >= SEC_PER_MIN; minute++){  //determine minutes elapsed in current hour
		secs -= SEC_PER_MIN;
	}

	//The value of secs at the end of the minutes loop is the seconds elapsed in the
	//current minute.
	//Given the year & day of year, determine month & day of month
	month = osc_timetag_getmonth(&day, leap);
	DateTime d(year, month, day, hour, minute, secs);
	return d;
}

void osc_dispatch(long len, char *bndl)
{
	if(!bndl || len == 0){
		return;
	}
	char *p = (char *)bndl + 16;
	while(p < bndl + len){
		int32_t size = ntoh32(*((int32_t *)p));
		Serial.print("size = ");
		Serial.println(size);
		Serial.print("pfx = ");
		Serial.println(get_osc_pfx(get_id()));
		if(!strncmp(p + 4, get_osc_pfx(get_id()), 2)){
			Serial.println("have prefix");
			if(!strncmp(p + 6, "/set", 4)){
				Serial.println("have set");
				if(!strncmp(p + 10, "/time", 5)){
					Serial.println("set time");
					if(p[17] == 't'){
						Serial.println("have timetag");
						char t[64];
						DateTime d = osctime_to_date(osc_timetag_decodeFromHeader(p + 20));
						rtc.adjust(d);
						//char buf[128];
						//date_to_string(d, 128, buf);
						//Serial.println(buf);
					}
				}
			}
		}
		p += size + 4;
	}
}

// void getNtpTime()
// {
// #ifdef ECG_WIFI
// 	while (udp.parsePacket() > 0) ; // discard any previously received packets
// 	Serial.println("Transmit NTP Request");
// 	sendNTPpacket(ntp_time_server);
// 	uint32_t beginWait = millis();
// 	while (millis() - beginWait < 1500) {
// 		int size = udp.parsePacket();
// 		if (size >= NTP_PACKET_SIZE) {
// 			Serial.println("Receive NTP Response");
// 			udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
// 			//yield();
// 			/* uint64_t t1 = ntoh64(*((uint64_t *)(packetBuffer + 16))); */
// 			/* uint64_t t2 = ntoh64(*((uint64_t *)(packetBuffer + 24))); */
// 			/* uint64_t t3 = ntoh64(*((uint64_t *)(packetBuffer + 32))); */
// 			/* uint64_t t4 = ntoh64(*((uint64_t *)(packetBuffer + 40))); */
// 			/* uint64_t t1 = ntoh32(*((uint32_t *)(packetBuffer + 16))) << 32 | ntoh32(*((uint32_t *)(packetBuffer + 20))); */
// 			/* uint64_t t2 = ntoh32(*((uint32_t *)(packetBuffer + 24))) << 32 | ntoh32(*((uint32_t *)(packetBuffer + 28))); */
// 			/* uint64_t t3 = ntoh32(*((uint32_t *)(packetBuffer + 32))) << 32 | ntoh32(*((uint32_t *)(packetBuffer + 36))); */
// 			/* uint64_t t4 = ntoh32(*((uint32_t *)(packetBuffer + 40))) << 32 | ntoh32(*((uint32_t *)(packetBuffer + 44))); */
// 			osctime t1 = (osctime){ntoh32(*((uint32_t *)(packetBuffer + 16))), ntoh32(*((uint32_t *)(packetBuffer + 20)))};
// 			osctime t2 = (osctime){ntoh32(*((uint32_t *)(packetBuffer + 24))), ntoh32(*((uint32_t *)(packetBuffer + 28)))};
// 			osctime t3 = (osctime){ntoh32(*((uint32_t *)(packetBuffer + 32))), ntoh32(*((uint32_t *)(packetBuffer + 36)))};
// 			osctime t4 = (osctime){ntoh32(*((uint32_t *)(packetBuffer + 40))), ntoh32(*((uint32_t *)(packetBuffer + 44)))};
			
// 			osctime delta = osc_timetag_subtract(osc_timetag_subtract(t4, t1), osc_timetag_subtract(t3, t2));
// 			osctime theta = float_to_osctime(0.5 * osctime_to_float(osc_timetag_add(osc_timetag_subtract(t2, t1), osc_timetag_subtract(t3, t4))));
// 			if(osc_timetag_compare(theta,  theta_min) < 0){
// 				Serial.println("better theta");
// 				theta_min = theta;
// 				delta_theta_min = delta;
// 				printf("0x%llx\n", t4);
// 				// set time
// 				Serial.println(t4.frac_sec);
// 				Serial.println((double)t4.frac_sec / 0xFFFFFFFF);
// 				printf("%llu %llu\n", delta, theta);
// 				//ttt.sec = ntoh32(ttt.sec);
// 				//ttt.frac_sec = ntoh32(ttt.frac_sec);
// 				//uint32_t tmicros = micros() + (1000000 - (((double)t4.frac_sec / 0xFFFFFFFF) * 1000000));
// 				uint32_t ival_micros = (1000000 - (((double)t4.frac_sec / 0xFFFFFFFF) * 1000000));
// 				Serial.print("waiting for ");
// 				Serial.print((1000000 - (((double)t4.frac_sec / 0xFFFFFFFF) * 1000000)));
// 				Serial.print("us, ");
// 				Serial.print((1000000 - (((double)t4.frac_sec / 0xFFFFFFFF) * 1000000)) / (float)1000);
// 				Serial.println("ms");
// 				t4.sec++;
// 				uint32_t cur_micros = micros();
// 				uint32_t prev_micros = cur_micros;
// 				//while(micros() < tmicros){
// 				while(cur_micros - prev_micros < ival_micros){
// 				      cur_micros = micros();
// 				}
// 				DS3234_set_date(osctime_to_date(t4));
// 				yield();
// 				return;
// 			}else{
// 				yield();
// 				return;
// 			}
// 		}
// 		yield();
// 	}
// 	Serial.println("No NTP Response :-(");
// #endif
// }

// // send an NTP request to the time server at the given address
// void sendNTPpacket(IPAddress &address)
// {
// #ifdef ECG_WIFI
// 	// set all bytes in the buffer to 0
// 	memset(packetBuffer, 0, NTP_PACKET_SIZE);
// 	// Initialize values needed to form NTP request
// 	// (see URL above for details on the packets)
// 	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
// 	packetBuffer[1] = 0;     // Stratum, or type of clock
// 	packetBuffer[2] = 6;     // Polling Interval
// 	packetBuffer[3] = 0xEC;  // Peer Clock Precision
// 	// 8 bytes of zero for Root Delay & Root Dispersion
// 	packetBuffer[12]  = 49;
// 	packetBuffer[13]  = 0x4E;
// 	packetBuffer[14]  = 49;
// 	packetBuffer[15]  = 52;
// 	// all NTP fields have been given values, now
// 	// you can send a packet requesting a timestamp:
// 	udp.beginPacket(address, 123); //NTP requests are to port 123
// 	udp.write(packetBuffer, NTP_PACKET_SIZE);
// 	udp.endPacket();
// #endif
// }

// void ntp_sync(void)
// {
// 	if(nsyncntp){
// 		getNtpTime();
// 		lastntpsync_micros = micros();
// 		nsyncntp--;
// 		if(nsyncntp == 0){
// 			theta_min = (osctime){0xFFFFFFFF, 0xFFFFFFFF};
// 			delta_theta_min = (osctime){0, 0};
// 			ntp_syncing = 0;
// 			// send_timestampbndl_start();
// 		}
// 	}
// }

// void ntp_sync_start(void)
// {
// 	nsyncntp = NSYNCNTP;
// 	ntp_syncing = 1;
// 	ntp_sync();
// }

// void send_statusbndl(osctime t, char *address)
// {
// 	memcpy(statusbndl + 20 + 3, address, 6);
// 	osc_timetag_encodeForHeader(t, statusbndl + (sizeof(statusbndl) - 8));
// #ifdef ECG_WIFI
// 	udp.beginPacket(remote_ip, remote_port);
// 	udp.write((const uint8_t *)statusbndl, sizeof(statusbndl));
// 	udp.endPacket();
// #endif
// }

// void send_resetbndl(osctime t)
// {
// 	send_statusbndl(t, resetaddr);
// }

// void send_flashbndl(osctime t)
// {
// 	send_statusbndl(t, flashaddr);
// }

uint8_t get_id(void)
{
	uint8_t p1 = digitalRead(pin_id1);
	uint8_t p2 = digitalRead(pin_id2);
	uint8_t p3 = digitalRead(pin_id3);
	Serial.print("get_id: ");
	Serial.print(p1);
	Serial.print(" ");
        Serial.print(p2);
	Serial.print(" ");
	Serial.println(p3);

	//uint8_t p4 = digitalRead(pin_id4);
	//return (p4 << 3) | (p3 << 2) | (p2 << 1) | p1;
	return (p3 << 2) | (p2 << 1) | p1;
}

const char heartbeat_bndl_size = 64;
char heartbeat_bndl[heartbeat_bndl_size];
void init_heartbeat_bndl(void)
{
	memset(heartbeat_bndl, 0, heartbeat_bndl_size);
	char *p = heartbeat_bndl;
	snprintf(p, heartbeat_bndl_size - (p - heartbeat_bndl), "#bundle");
	p += 16;
	*((int32_t *)p) = hton32(28);
	p += 4;
	IPAddress ipaddy = WiFi.localIP();
	snprintf(p, heartbeat_bndl_size - (p - heartbeat_bndl), "/ip");
	p += 4;
	snprintf(p, heartbeat_bndl_size - (p - heartbeat_bndl), ",IIII");
	p += 8;
	Serial.print(ipaddy[0]);
	Serial.print(".");
	Serial.print(ipaddy[1]);
	Serial.print(".");
	Serial.print(ipaddy[2]);
	Serial.print(".");
	Serial.print(ipaddy[3]);
	Serial.println();
	*((uint32_t *)p) = hton32(ipaddy[0]);
	p += 4;
	*((uint32_t *)p) = hton32(ipaddy[1]);
	p += 4;
	*((uint32_t *)p) = hton32(ipaddy[2]);
	p += 4;
	*((uint32_t *)p) = hton32(ipaddy[3]);
	p += 4;

	*((int32_t *)p) = hton32(12);
	p += 4;
	snprintf(p, heartbeat_bndl_size - (p - heartbeat_bndl), "/px");
	p += 4;
	snprintf(p, heartbeat_bndl_size - (p - heartbeat_bndl), ",s");
	p += 4;
	snprintf(p, heartbeat_bndl_size - (p - heartbeat_bndl), "%s", get_osc_pfx(get_id()));
}

void send_heartbeat_bndl(void)
{
#ifdef ECG_WIFI
	udp.beginPacket(full_ip, full_port);
	udp.write((const uint8_t *)heartbeat_bndl, heartbeat_bndl_size);
	udp.endPacket();
#endif // ECG_WIFI
}

// isr for RTC alarm 1
void isr_a1()
{
	micros_ref = micros();
	int_a1 = 1;
}

osctime int_flashdown_time = (osctime){0, 0};
osctime int_flashup_time = (osctime){0, 0};
int int_send_flashbndl = 0;
// isr to handle flash button press
void isr_flash()
{
	// if(digitalRead(pin_flash) == LOW){
	// 	int_flash = 1;
	// 	int_send_flashbndl = 1;
	// 	int_flash_down_micros = micros();
	// 	int_flashdown_time = timenow();
	// 	digitalWrite(pin_led, LOW);
	// }else{
	// 	int_flash_up_micros = micros();
	// 	int_flashup_time = timenow();
	// }
}

boolean connected = false;

void wifi_event_handler(WiFiEvent_t e)
{
	switch(e){
	case SYSTEM_EVENT_STA_GOT_IP:
		Serial.println(WiFi.localIP());
		udp.begin(WiFi.localIP(), 8888);
		connected = true;
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		connected = false;
		break;
	}
}

void connect_to_wifi(const char *ssid, const char *pass)
{
	WiFi.disconnect(true);
	WiFi.onEvent(wifi_event_handler);
	WiFi.begin(ssid, pass);
	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		Serial.println("Connection Failed! Rebooting...");
		delay(5000);
		ESP.restart();
	}
}

#if (ARDUINO >= 100)
 #include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
 // Macro to deal with the difference in I2C write functions from old and new Arduino versions.
 #define _I2C_WRITE write
 #define _I2C_READ  read
#else
 #include <WProgram.h>
 #define _I2C_WRITE send
 #define _I2C_READ  receive
#endif

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire._I2C_WRITE((byte)val);
  Wire.endTransmission();
}

static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire._I2C_READ();
}

void setup()
{
#ifdef ECG_SERIAL
	Serial.begin(115200);
#endif // ECG_SERIAL
	pinMode(33, INPUT);
	
	pinMode(pin_led, OUTPUT);
	digitalWrite(pin_led, LOW);

	pinMode(pin_id1, INPUT);
	pinMode(pin_id2, INPUT);
	pinMode(pin_id3, INPUT);

	// pinMode(pin_flash, INPUT);
	// attachInterrupt(digitalPinToInterrupt(pin_flash), isr_flash, CHANGE);

	pinMode(pin_sqw, INPUT); // this is the RTC alarm
	digitalWrite(pin_sqw, HIGH);
	attachInterrupt(digitalPinToInterrupt(pin_sqw), isr_a1, FALLING);

#ifdef ECG_WIFI
	connect_to_wifi(ssid, pass);
#endif // ECG_WIFI

	ArduinoOTA.setPort(3232);
	ArduinoOTA.setHostname(get_osc_pfx(get_id()));
	ArduinoOTA
		.onStart([]() {
				String type;
				if (ArduinoOTA.getCommand() == U_FLASH)
					type = "sketch";
				else // U_SPIFFS
					type = "filesystem";

				// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
				Serial.println("Start updating " + type);
			})
		.onEnd([]() {
				Serial.println("\nEnd");
			})
		.onProgress([](unsigned int progress, unsigned int total) {
				Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
			})
		.onError([](ota_error_t error) {
				Serial.printf("Error[%u]: ", error);
				if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
				else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
				else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
				else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
				else if (error == OTA_END_ERROR) Serial.println("End Failed");
			});

	ArduinoOTA.begin();

	lp = rtecg_ptlp_init();
	hp = rtecg_pthp_init();
	d = rtecg_ptd_init();
	mwi = rtecg_pti_init();
	pkf = rtecg_pk_init();
	pki = rtecg_pk_init();
	pts = rtecg_pt_init();
	memset(tlst, 0, sizeof(tlst) / sizeof(osctime));
	
	// set up RTC
	// pinMode(pin_ss, OUTPUT);
	// SPI.begin();
	// SPI.setBitOrder(MSBFIRST);
	// SPI.setDataMode(SPI_MODE1);

	// set up header, size, address, and typetags for OSC bundle
	memset(oscbndl, 0, sizeof(oscbndl)); 
	char *h = "#bundle\0\0\0\0\0\0\0\0\0";
	memcpy(oscbndl, h, 16); // header
	*((int32_t *)(oscbndl + 16)) = hton32(oscbndl_size - 20); // message size
	//memcpy(oscbndl + 20, oscpfx, 3); // /aa
	memcpy(oscbndl + 20, get_osc_pfx(get_id()), 2); // /aa
	memcpy(oscbndl + 22, oscbndl_address, 4); // /ecg
	memcpy(oscbndl + 28, oscbndl_typetags, 24); // typetags
	*((int32_t *)(oscbndl + oscbndl_fs)) = hton32(RTECG_FS);

	if(!rtc.begin()){
		Serial.println("no rtc attached");
	}else{
		//write_i2c_register(DS3231_ADDRESS, 0x07, 0xF);
		write_i2c_register(DS3231_ADDRESS, 0x0e, 0);
		//rtc.writeSqwPinMode(DS3231_OFF);
		
		//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}
	uint8_t r = read_i2c_register(DS3231_ADDRESS, 0x07);
	Serial.println(r);
	r = read_i2c_register(DS3231_ADDRESS, 0x08);
	Serial.println(r);
	r = read_i2c_register(DS3231_ADDRESS, 0x09);
	Serial.println(r);
	r = read_i2c_register(DS3231_ADDRESS, 0x0a);
	Serial.println(r);
	r = read_i2c_register(DS3231_ADDRESS, 0x0e);
	Serial.println(r);
	r = read_i2c_register(DS3231_ADDRESS, 0x0f);
	Serial.println(r);

	// notify the world that we did a reset
	reset_time = timenow();
	didreset = 1;

	// set alarm 1 to go off every second
	//DS3234_set_a1(pin_ss, 0, 0, 0, 0, (const uint8_t[]){1, 1, 1, 1});
	//DS3234_set_reg(DS3234_SREG_WRITE, DS3234_get_reg(DS3234_SREG_READ) & ~DS3234_A1F);
	
	// wait for a transition and then read the microsecond counter
	//while(DS3234_get_reg(DS3234_SREG_READ) & DS3234_A1F == 0){
	//;
	//}
	// micros_ref is set when the second counter transitions to a new value.
	// we use it as a reference for generating a fraction of a second.
	//micros_ref = micros();
	//DS3234_set_reg(DS3234_SREG_WRITE, DS3234_get_reg(DS3234_SREG_READ) & ~DS3234_A1F);
	
	// set up alarm 1 as an interrupt
	//DS3234_set_reg(DS3234_CREG_WRITE, DS3234_get_reg(DS3234_CREG_READ) | DS3234_A1IE);

	// send 8 bundles with time stamps to the client so that they can do statistics on the network jitter / latency
	//send_timestampbndl_start();

	tmicros_ival = 1000000. / RTECG_FS;
	tmicros_prev = micros();

	while(connected == 0){
		sleep(1);
	}
	init_heartbeat_bndl();
}

void loop()
{
	ArduinoOTA.handle();
	int packetSize = udp.parsePacket();
	if (packetSize) {
		unsigned char packetBuffer[255];
		int len = udp.read(packetBuffer, 255);
		if (len > 0) {
			packetBuffer[len] = 0;
			osc_dispatch(len, (char *)packetBuffer);
			udp.beginPacket(udp.remoteIP(), full_port);
			udp.write(packetBuffer, len);
			udp.endPacket();
		}
	}
	// spinlock to get as close to 5ms as we can before yielding
	uint32_t tmicros_cur = micros();
	while(tmicros_cur - tmicros_prev < tmicros_ival){
		tmicros_cur = micros();
	}
	tmicros_prev = tmicros_cur;
	// read pin
	rtecg_int a0 = analogRead(A9);
	if(int_a1){
		int id = get_id();
		int_a1 = 0;
		DateTime d = rtc.now();
		current_date = date_to_osctime(d);
		send_heartbeat_bndl();
	}
	
	// timenow refers to a global var that holds the current time from the RTC
	osctime now = timenow();
	tlst[tptr] = now;
	
	// now yield
	yield();
	// turn off LED if it was on from previous loop
	// if(digitalRead(pin_flash) == HIGH){
	// 	digitalWrite(pin_led, HIGH);
	// }
	// // if flash button was pressed, check state and see look at interval to see if there's something we should do
	// if(int_flash){
	// 	Serial.println("flash is or was down");
	// 	if(int_send_flashbndl){
	// 		Serial.println("should send flash bndl");
	// 		send_flashbndl(int_flashdown_time);
	// 		int_send_flashbndl = 0;
	// 	}
	// 	if(digitalRead(pin_flash) == LOW){
	// 		// flash button is still down
	// 		if(micros() - int_flash_down_micros >= 5000000){
	// 			// flash button has been down for more than 5 seconds. flash led and start NTP sync
	// 			digitalWrite(pin_led, HIGH);
	// 			delay(100);
	// 			digitalWrite(pin_led, LOW);
	// 			delay(100);
	// 			digitalWrite(pin_led, HIGH);
	// 			delay(100);
	// 			digitalWrite(pin_led, LOW);
	// 			delay(100);
	// 			digitalWrite(pin_led, HIGH);
	// 			// NTP sync
	// 			ntp_sync_start();
	// 			int_flash = 0;
	// 		}
	// 	}else{
	// 		// flash button has been released
	// 		digitalWrite(pin_flash, HIGH);
	// 		//send_timestampbndl_start();
	// 		int_flash = 0;
	// 	}
	// }
	if(didreset){
		// when the loop begins, we can't send packets right away, but there doesn't seem
		// to be a way to find out when we can, so just wait for the first 50 to pass by, then send a reset
		// if(didreset == 50){
		// 	send_resetbndl(reset_time);
		// 	didreset = 0;
		// }else{
		// 	didreset++;
		// }
	}
	// send a timestamp bundle if we're in the middle of that
	// if(sending_timestampbndls && micros() - lasttimestampbndl_micros >= TIMESTAMPBNDL_IVAL_MICROS){
	// 	send_timestampbndl();
	// }

	// do an NTP sync if we're in the middle of that
	// if(nsyncntp && micros() - lastntpsync_micros >= 2000000){
	// 	ntp_sync();
	// }
	// filter ECG signal
        lp = rtecg_ptlp_hx0(lp, a0);
	hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
	d = rtecg_ptd_hx0(d, rtecg_pthp_y0(hp));
	mwi = rtecg_pti_hx0(mwi, rtecg_ptd_y0(d));
	// mark peaks
	pkf = rtecg_pk_mark(pkf, rtecg_pthp_y0(hp));
	pki = rtecg_pk_mark(pki, rtecg_pti_y0(mwi));
	// classify
	// if(rtecg_pk_y0(pkf) || rtecg_pk_y0(pki)){
	// 	Serial.printf("++++++++++CLASSIFY ENTER\n");
	// 	Serial.printf("ctr: %u, pkf: %d, pki: %d\n", pts.ctr + 1, rtecg_pk_y0(pkf), rtecg_pk_y0(pki));
	// }
	int buflen = 1;//1200;
	char buf[buflen];
	buf[0] = 0;
	pts = rtecg_pt_process(pts, rtecg_pk_y0(pkf) * rtecg_pk_xm82(pkf), rtecg_pk_maxslope(pkf), rtecg_pk_y0(pki) * rtecg_pk_xm82(pki), rtecg_pk_maxslope(pki), buf, buflen, 0);
	// if(rtecg_pk_y0(pkf) || rtecg_pk_y0(pki)){
	// 	Serial.print(buf);
	// 	Serial.printf("-----------CLASSIFY EXIT\n");
	// 	Serial.printf("%s\n", pts.havepeak ? "FOUND PEAK": "NO PEAK");
	// }
	yield();
	if(pts.searchback){
	       	//Serial.printf("++++++++++SEARCHBACK ENTER\n");
		buf[0] = 0;
		pts = rtecg_pt_searchback(pts, buf, buflen, 0);
		// Serial.print(buf);
		// Serial.printf("-----------SEARCHBACK EXIT\n");
		// if(pts.havepeak == 0){
		// 	Serial.printf("**************************************************\nNO PEAK FOUND DURING SEARCHBACK!!!\n**************************************************\n");
		// 	//pts = rtecg_pt_reset(pts);
		// }
		yield();
	}
	if(pts.havepeak){
		// if(digitalRead(pin_flash) == HIGH){
		// 	digitalWrite(pin_led, LOW);		
		// }
	}
	//Serial.printf("-----------LOOP EXIT\n");
	//yield();
	//Serial.print(tmicros_cur);
	if (WiFi.status() != WL_CONNECTED) {
		yield();
		// WiFi.begin(ssid, pass);
		// int status = WiFi.waitForConnectResult();
	        // while(status != WL_CONNECTED){
		// 	delay(250);
		// }
		connect_to_wifi(ssid, pass);
	}
	if(ecg_send_full || pts.havepeak){
		// put data in OSC bundle
		float tmpf;
		*((uint32_t *)(oscbndl + oscbndl_pnum)) = hton32(pts.ctr);
		osc_timetag_encodeForHeader(now, oscbndl + oscbndl_time);
		*((int32_t *)(oscbndl + oscbndl_raw)) = hton32(a0);
		*((int32_t *)(oscbndl + oscbndl_filt)) = hton32(rtecg_pthp_y0(hp));
		*((int32_t *)(oscbndl + oscbndl_mwi)) = hton32(rtecg_pti_y0(mwi));
		*((uint32_t *)(oscbndl + oscbndl_spkfn)) = hton32(pts.ctr - rtecg_pt_last_spkf(pts).x);
		int idx = tptr - (rtecg_pt_last_spkf(pts).x + RTECG_PKDEL) + 1;
		if(idx < 0){
			idx += (RTECG_FS * 2);
		}
		osc_timetag_encodeForHeader(tlst[idx], oscbndl + oscbndl_spkft);
		*((int32_t *)(oscbndl + oscbndl_spkfv)) = hton32(rtecg_pt_last_spkf(pts).y);
		tmpf = rtecg_pt_last_spkf(pts).confidence;
		*((int32_t *)(oscbndl + oscbndl_spkfc)) = hton32(*((int32_t *)&(tmpf)));
		*((uint32_t *)(oscbndl + oscbndl_spkin)) = hton32(pts.ctr - rtecg_pt_last_spki(pts).x);
		idx = tptr - (rtecg_pt_last_spki(pts).x + RTECG_PKDEL) + 1;
		if(idx < 0){
			idx += (RTECG_FS * 2);
		}
		osc_timetag_encodeForHeader(tlst[idx], oscbndl + oscbndl_spkit);
		*((int32_t *)(oscbndl + oscbndl_spkiv)) = hton32(rtecg_pt_last_spki(pts).y);
		tmpf = rtecg_pt_last_spki(pts).confidence;
		*((int32_t *)(oscbndl + oscbndl_spkic)) = hton32(*((int32_t *)&(tmpf)));
		float r = pts.rr / (float)RTECG_FS;
		if(r){
			r = 60. / r;
		}
		*((int32_t *)(oscbndl + oscbndl_rr)) = hton32(*((int32_t *)&r));
		r = pts.rravg1 / (float)RTECG_FS;
		if(r){
			r = 60. / r;
		}
		*((int32_t *)(oscbndl + oscbndl_avg1)) = hton32(*((int32_t *)&r));
		r = pts.rravg2 / (float)RTECG_FS;
		if(r){
			r = 60. / r;
		}
		*((int32_t *)(oscbndl + oscbndl_avg2)) = hton32(*((int32_t *)&r));
		*((int32_t *)(oscbndl + oscbndl_f1)) = hton32(*((int32_t *)&(pts.f1)));
		*((int32_t *)(oscbndl + oscbndl_f2)) = hton32(*((int32_t *)&(pts.f2)));
		*((int32_t *)(oscbndl + oscbndl_i1)) = hton32(*((int32_t *)&(pts.i1)));
		*((int32_t *)(oscbndl + oscbndl_i2)) = hton32(*((int32_t *)&(pts.i2)));
	
		// ship it
#ifdef ECG_WIFI
		//Serial.print(" sending");
		//if(ecg_send_full){
			udp.beginPacket(full_ip, full_port);
			udp.write((const uint8_t *)oscbndl, oscbndl_size);
			udp.endPacket();
		// }
		// if(pts.havepeak){
		// 	udp.beginPacket(peak_ip, peak_port);
		// 	udp.write(oscbndl, oscbndl_size);
		// 	udp.endPacket();
		// }
		//Serial.println(" done");
#endif // ECG_WIFI
	}
#ifdef ECG_SERIAL
		//SLIP//Serial.beginPacket();
		//SLIP//Serial.endPacket();
		////Serial.println(a0);
#endif // ECG_SERIAL
	if(++tptr == (RTECG_FS * 2)){
		tptr = 0;
	}
	// don't delay or yield here---we want to get close to 5ms, and we don't know how long
	// a we'll lose control for if we yield or delay
}

