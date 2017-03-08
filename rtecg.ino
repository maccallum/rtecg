#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
/* #ifdef ESP8266 */
/* extern "C" { */
/* #include "user_interface.h" */
/* } */
/* #endif */
#include "rtecg.h"
#include "rtecg_filter.h"
#include "rtecg_pantompkins.h"

// prefix for all OSC addresses
char *oscpfx = "/aa"; // must be 3 or fewer characters

// main OSC bundle
char *oscbndl_address = "/ecg";
char *oscbndl_typetags = ",Itiiitiftiffff\0";
char oscbndl[20 	// header and message size
	     + 8	// address
	     + 16	// typetags
	     + 4	// packet number
	     + 8	// time
	     + 4	// raw
	     + 4	// filtered
	     + 4	// mwi
	     + 8	// spkf time
	     + 4	// spkf value
	     + 4	// spkf confidence
	     + 8	// spki time
	     + 4	// spki value
	     + 4	// spki confidence
	     + 4	// rr
	     + 4	// avg1
	     + 4];	// avg2
int oscbndl_pnum = 44;
int oscbndl_time = oscbndl_pnum + 4;
int oscbndl_raw = oscbndl_time + 8;
int oscbndl_filt = oscbndl_raw + 4;
int oscbndl_mwi = oscbndl_filt + 4;
int oscbndl_spkft = oscbndl_mwi + 4;
int oscbndl_spkfv = oscbndl_spkft + 8;
int oscbndl_spkfc = oscbndl_spkfv + 4;
int oscbndl_spkit = oscbndl_spkfc + 4;
int oscbndl_spkiv = oscbndl_spkit + 8;
int oscbndl_spkic = oscbndl_spkiv + 4;
int oscbndl_rr = oscbndl_spkic + 4;
int oscbndl_avg1 = oscbndl_rr + 4;
int oscbndl_avg2 = oscbndl_avg1 + 4;
int32_t oscbndl_size = oscbndl_avg2 + 4;

// bundle to send when we do a rest
char resetbndl[] = {'#', 'b', 'u', 'n', 'd', 'l', 'e', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, oscpfx[0], oscpfx[1], oscpfx[2], '/', 'r', 'e', 's', 'e', 't', 0, 0, 0};
// bundle with a single time tag that we send to the client so that they can do some statistics on the network traffic in response to pressing the flash button
char timestampbndl[] = {'#', 'b', 'u', 'n', 'd', 'l', 'e', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, oscpfx[0], oscpfx[1], oscpfx[2], '/', 't', 0, 0, 0, ',', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// WIFI AP
char ssid[] = "TP-LINK_40FE00";
char pass[] = "78457393";

// WiFi remote host
WiFiUDP udp;                                // A UDP instance to let us send and receive packets over UDP
const IPAddress remote_ip(192,168,0,100);        // remote IP of your computer
const unsigned int remote_port = 9999;          // remote port to receive OSC

//#define ECG_DEBUG
//#define ECG_WIFI
#define ECG_SERIAL

typedef struct _osctime
{
	uint32_t seconds;
	uint32_t fractionofseconds;
} osctime;

// state of classifier etc
unsigned long tmicros;
rtecg_ptlp lp;
rtecg_pthp hp;
rtecg_ptd d;
rtecg_pti mwi;
rtecg_pk pkf;
rtecg_pk pki;
rtecg_pt pts;
osctime tlst[RTECG_FS * 2];
int tptr = 0;
int ntimestampbndls = 0;
unsigned long lasttimestampbndl_micros = 0; 

/* #ifdef ECG_SERIAL */
/* #include <SLIPEncodedSerial.h> */
/* SLIPEncodedSerial SLIPSerial(Serial); */
/* #endif // ECG_SERIAL */

const int pin_ss = 5; // slave select pin
const int pin_sqw = 4; // square wave pin
const int pin_led = 2; // GPIO pin that controls the LED on the ESP-12
const int pin_flash = 0; // GPIO pin that the flash button is wired to

// used by a1 isr
volatile unsigned long micros_ref = 0;
volatile int int_a1 = 0;

// used by flash button isr
volatile int int_flash = 0;
volatile unsigned long int_flash_down_micros = 0;
volatile unsigned long int_flash_up_micros = 0;

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

// control register masks
#define DS3234_A1IE 0x1
#define DS3234_A2IE 0x2
#define DS3234_INTCN 0x4
#define DS3234_RS1 0x8
#define DS3234_RS2 0x10
#define DS3234_CONV 0x20
#define DS3234_BBSQW 0x40
#define DS3234_EOSC 0x80

// status register masks
#define DS3234_A1F 0x1
#define DS3234_A2F 0x2

#define DS3234_CREG_WRITE 0x8E // write to the control register
#define DS3234_CREG_READ 0x0E // read to the control register
#define DS3234_SREG_WRITE 0x8F // write to the status register
#define DS3234_SREG_READ 0x0F // read the status register 

struct DS3234_date
{
	uint8_t sec, min, hour, wday, mday, mon_plus_cent, year;
};

// utility functions for communicating with the DS3234
void DS3234_set_reg(const uint8_t addr, const uint8_t val)
{
	digitalWrite(pin_ss, LOW);
	SPI.transfer(addr);
	SPI.transfer(val);
	digitalWrite(pin_ss, HIGH);
}

uint8_t DS3234_get_reg(const uint8_t addr)
{
	uint8_t rv;

	digitalWrite(pin_ss, LOW);
	SPI.transfer(addr);
	rv = SPI.transfer(0x00);
	digitalWrite(pin_ss, HIGH);
	return rv;
}

struct DS3234_date DS3234_get_date()
{
	struct DS3234_date d;
	char *dc = (char *)(&d);
	for(int i = 0; i < 7; i++){
		digitalWrite(pin_ss, LOW);
		SPI.transfer(i);
		dc[i] = SPI.transfer(0);
		digitalWrite(pin_ss, HIGH);
	}
	return d;
}

void DS3234_set_date(struct DS3234_date d)
{
	char *dc = (char *)(&d);	
	for(int i = 0; i < 7; i++){
		digitalWrite(pin_ss, LOW);
		SPI.transfer(i + 0x80);
		SPI.transfer(dc[i]);
		digitalWrite(pin_ss, HIGH);
	}
}

uint8_t dectobcd(const uint8_t val)
{
	return ((val / 10 * 16) + (val % 10));
}

uint8_t bcdtodec(const uint8_t val)
{
	return ((val / 16 * 10) + (val % 16));
}

struct DS3234_date make_DS3234_date(uint8_t sec,
				    uint8_t min,
				    uint8_t hour,
				    uint8_t wday,
				    uint8_t mday,
				    uint8_t mon,
				    uint16_t year)
{
	int century = year >= 2000 ? 1 : 0;
	struct DS3234_date d = (struct DS3234_date){
		dectobcd(sec),
		dectobcd(min),
		dectobcd(hour),
		dectobcd(wday),
		dectobcd(mday),
		dectobcd(mon) + century * 0x80,
		dectobcd((uint8_t)(year - (1900 + (century * 100))))
	};
	return d;
}

void DS3234_date_to_string(struct DS3234_date d, long buflen, char *buf)
{
	snprintf(buf,
		 buflen,
		 "%d.%02d.%02d %02d:%02d:%02d",
		 bcdtodec(d.year) + (1900 + (((d.mon_plus_cent & 0x80) >> 7) * 100)),
		 bcdtodec(d.mon_plus_cent & 0x7F),
		 bcdtodec(d.mday),
		 bcdtodec(d.hour),
		 bcdtodec(d.min),
		 bcdtodec(d.sec));
}

osctime DS3234_date_to_osctime(struct DS3234_date d)
{
	uint8_t i;
	uint16_t day;
	int16_t year;
	int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	osctime time = {0, 0};

	int d_year = bcdtodec(d.year) + (1900 + (((d.mon_plus_cent & 0x80) >> 7) * 100));
	if (d_year >= 2000) {
		year = (d_year - 2000) + 100;
	} else {
		return time;
	}

	int d_mon = bcdtodec(d.mon_plus_cent & 0x7F);
	day = bcdtodec(d.mday - 1);
	for(i = 1; i < d_mon; i++){
		day += days_in_month[i - 1];
	}
	if(d_mon > 2 && year % 4 == 0){
		day++;
	}
	// count leap days
	day += (365 * year + (year + 3) / 4);

	time.seconds = ((day * 24UL + bcdtodec(d.hour)) * 60 + bcdtodec(d.min)) * 60 + bcdtodec(d.sec);// + 946684800;
	return time;
}

// flags are: A1M1 (seconds), A1M2 (minutes), A1M3 (hour),
// A1M4 (day) 0 to enable, 1 to disable, DY/DT (dayofweek == 1/dayofmonth == 0)
void DS3234_set_a1(const uint8_t pin, const uint8_t s, const uint8_t mi, const uint8_t h, const uint8_t d, const uint8_t * flags)
{
	uint8_t t[4] = { s, mi, h, d };
	uint8_t i;

	for (i = 0; i < 4; i++) {
		digitalWrite(pin, LOW);
		SPI.transfer(i + 0x87);
		SPI.transfer(dectobcd(t[i]) | (flags[i] << 7));
		digitalWrite(pin, HIGH);
	}
}

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

osctime now()
{
	noInterrupts();
	osctime now = DS3234_date_to_osctime(DS3234_get_date());
	now.fractionofseconds = (uint32_t)(0xFFFFFFFFUL * ((micros() - micros_ref) / 1000000.));
	interrupts();
	return now;
}

void send_resetbndl()
{
	osc_timetag_encodeForHeader(now(), resetbndl + 8);
#ifdef ECG_WIFI
	udp.beginPacket(remote_ip, remote_port);
	udp.write(resetbndl, sizeof(resetbndl));
	udp.endPacket();
#endif // ECG_WIFI
}

void send_timestampbndls(void)
{
	Serial.print("sending timestamp bndl (");
	Serial.print(ntimestampbndls);
	Serial.println(")");
	if(ntimestampbndls){
		osctime t = now();
		osc_timetag_encodeForHeader(t, timestampbndl + (sizeof(timestampbndl) - 8));
		lasttimestampbndl_micros = micros();
#ifdef ECG_WIFI
		udp.beginPacket(remote_ip, remote_port);
		udp.write(resetbndl, sizeof(resetbndl));
		udp.endPacket();
#endif // ECG_WIFI
		ntimestampbndls--;
	}
}

void ntp_sync(void)
{
	Serial.println("NTP sync");
}

// isr for RTC alarm 1
void isr_a1()
{
	micros_ref = micros();
	int_a1 = 1;
}

// isr to handle flash button press
void isr_flash()
{
	Serial.println(digitalRead(pin_flash));
	if(digitalRead(pin_flash) == LOW){
		int_flash = 1;
		int_flash_down_micros = micros();
		digitalWrite(pin_led, LOW);
	}else{
		int_flash_up_micros = micros();
	}
}

void setup()
{
	//ESP.wdtDisable();
	//ESP.wdtEnable(WDTO_8S);
#ifdef ECG_SERIAL
	Serial.begin(115200);
#endif // ECG_SERIAL

	pinMode(pin_led, OUTPUT);
	digitalWrite(pin_led, HIGH);

	pinMode(pin_flash, INPUT);
	attachInterrupt(digitalPinToInterrupt(pin_flash), isr_flash, CHANGE);

	pinMode(pin_sqw, INPUT); // this is the RTC alarm
	digitalWrite(pin_sqw, HIGH);
	attachInterrupt(digitalPinToInterrupt(pin_sqw), isr_a1, FALLING);

#ifdef ECG_WIFI
	// set up WiFi
	WiFi.begin(ssid, pass);
	int status = WiFi.waitForConnectResult();
	while(status != WL_CONNECTED){
		delay(250);
	}
	udp.begin(8888);
#endif // ECG_WIFI

	lp = rtecg_ptlp_init();
	hp = rtecg_pthp_init();
	d = rtecg_ptd_init();
	mwi = rtecg_pti_init();
	pkf = rtecg_pk_init();
	pki = rtecg_pk_init();
	pts = rtecg_pt_init();
	memset(tlst, 0, sizeof(tlst) / sizeof(osctime));
	
	// set up RTC
	pinMode(pin_ss, OUTPUT);
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE1);

	// set up header, size, address, and typetags for OSC bundle
	memset(oscbndl, 0, sizeof(oscbndl)); 
	char *h = "#bundle\0\0\0\0\0\0\0\0\0";
	memcpy(oscbndl, h, 16); // header
	*((int32_t *)(oscbndl + 16)) = hton32(oscbndl_size - 20); // message size
	memcpy(oscbndl + 20, oscpfx, 3); // /aa
	memcpy(oscbndl + 23, oscbndl_address, 4); // /ecg
	memcpy(oscbndl + 28, oscbndl_typetags, 16); // typetags

	// notify the world that we did a reset
	send_resetbndl();

	// set alarm 1 to go off every second
	DS3234_set_a1(pin_ss, 0, 0, 0, 0, (const uint8_t[]){1, 1, 1, 1});
	DS3234_set_reg(DS3234_SREG_WRITE, DS3234_get_reg(DS3234_SREG_READ) & ~DS3234_A1F);
	
	// wait for a transition and then read the microsecond counter
	while(DS3234_get_reg(DS3234_SREG_READ) & DS3234_A1F == 0){
		;
	}
	// micros_ref is set when the second counter transitions to a new value.
	// we use it as a reference for generating a fraction of a second.
	micros_ref = micros();
	DS3234_set_reg(DS3234_SREG_WRITE, DS3234_get_reg(DS3234_SREG_READ) & ~DS3234_A1F);
	
	// set up alarm 1 as an interrupt
	DS3234_set_reg(DS3234_CREG_WRITE, DS3234_get_reg(DS3234_CREG_READ) | DS3234_A1IE);

	// send 8 bundles with time stamps to the client so that they can do statistics on the network jitter / latency
	ntimestampbndls = 8;
	send_timestampbndls();

	//tmicros = micros_ref + 4500;
}

void loop()
{
	// spinlock to get as close to 5ms as we can before yielding
	while((micros()) < tmicros){
		;
	}
	// get time and read pin
	noInterrupts();
	osctime now = DS3234_date_to_osctime(DS3234_get_date());
	now.fractionofseconds = (uint32_t)(0xFFFFFFFFUL * ((micros() - micros_ref) / 1000000.));
	interrupts();
	tlst[tptr] = now;
	rtecg_int a0 = analogRead(A0);
	// now yield
	yield();
	// turn off LED if it was on from previous loop
	if(digitalRead(pin_flash) == HIGH){
		digitalWrite(pin_led, HIGH);
	}
	// if alarm 1 went off, clear it
	if(int_a1){
		int_a1 = 0;
		DS3234_set_reg(DS3234_SREG_WRITE, DS3234_get_reg(DS3234_SREG_READ) & ~DS3234_A1F);
	}
	// if flash button was pressed, check state and see look at interval to see if there's something we should do
	if(int_flash){
		if(digitalRead(pin_flash) == LOW){
			// flash button is still down
			if(micros() - int_flash_down_micros >= 5000000){
				// flash button has been down for more than 5 seconds. flash led and start NTP sync
				digitalWrite(pin_led, HIGH);
				delay(100);
				digitalWrite(pin_led, LOW);
				delay(100);
				digitalWrite(pin_led, HIGH);
				delay(100);
				digitalWrite(pin_led, LOW);
				delay(100);
				digitalWrite(pin_led, HIGH);
				// NTP sync
				ntp_sync();
				int_flash = 0;
			}
		}else{
			// flash button has been released
			digitalWrite(pin_flash, HIGH);
			ntimestampbndls = 8;
			send_timestampbndls();
			int_flash = 0;
		}
	}
	if(ntimestampbndls && micros() - lasttimestampbndl_micros >= 2000000){
		send_timestampbndls();
	}
	// filter ECG signal
        lp = rtecg_ptlp_hx0(lp, a0);
	hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
	d = rtecg_ptd_hx0(d, rtecg_pthp_y0(hp));
	mwi = rtecg_pti_hx0(mwi, rtecg_ptd_y0(d));
	// mark peaks
	pkf = rtecg_pk_mark(pkf, rtecg_pthp_y0(hp));
	pki = rtecg_pk_mark(pki, rtecg_pti_y0(mwi));
	// classify
	pts = rtecg_pt_process(pts, rtecg_pk_y0(pkf) * rtecg_pk_xm82(pkf), rtecg_pk_maxslope(pkf), rtecg_pk_y0(pki) * rtecg_pk_xm82(pki), rtecg_pk_maxslope(pki));
	if(pts.searchback){
		pts = rtecg_pt_searchback(pts);
	}
	if(pts.havepeak){
		Serial.println(pts.last_spki.x);
		if(digitalRead(pin_flash) == HIGH){
			digitalWrite(pin_led, LOW);		
		}
	}
	//yield();

	// put data in OSC bundle
	float tmpf;
	*((uint32_t *)(oscbndl + oscbndl_pnum)) = hton32(pts.ctr);
	osc_timetag_encodeForHeader(now, oscbndl + oscbndl_time);
	*((int32_t *)(oscbndl + oscbndl_raw)) = hton32(a0);
	*((int32_t *)(oscbndl + oscbndl_filt)) = hton32(rtecg_pthp_y0(hp));
	*((int32_t *)(oscbndl + oscbndl_mwi)) = hton32(rtecg_pti_y0(mwi));
	int idx = tptr - (rtecg_pt_last_spkf(pts).x + RTECG_PKDEL);
	if(idx < 0){
		idx += RTECG_FS * 2;
	}
	osc_timetag_encodeForHeader(tlst[idx], oscbndl + oscbndl_spkft);
	*((int32_t *)(oscbndl + oscbndl_spkfv)) = hton32(rtecg_pt_last_spkf(pts).y);
	tmpf = rtecg_pt_last_spkf(pts).confidence;
	*((int32_t *)(oscbndl + oscbndl_spkfc)) = hton32(*((int32_t *)&(tmpf)));
	idx = tptr - (rtecg_pt_last_spki(pts).x + RTECG_PKDEL);
	if(idx < 0){
		idx += RTECG_FS * 2;
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
	
	// ship it
#ifdef ECG_WIFI
	udp.beginPacket(remote_ip, remote_port);
	udp.write(oscbndl, oscbndl_size);
	udp.endPacket();
#endif // ECG_WIFI
#ifdef ECG_SERIAL
		//SLIP//Serial.beginPacket();
		//SLIP//Serial.endPacket();
		////Serial.println(a0);
#endif // ECG_SERIAL

	if(++tptr == (RTECG_FS * 2)){
		tptr = 0;
	}
	tmicros += 5000;

	// don't delay or yield here---we want to get close to 5ms, and we don't know how long
	// a we'll lose control for if we yield or delay
}

