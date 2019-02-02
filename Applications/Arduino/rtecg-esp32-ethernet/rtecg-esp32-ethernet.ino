#include <stdint.h>

#include "esp_system.h"

#include <Ethernet.h>
#include <EthernetUdp.h>
#define UDP_TX_PACKET_MAX_SIZE 256

#include <rtecg.h>
#include <rtecg_filter.h>
#include <rtecg_pantompkins.h>
#include <rtecg_osc.h>
//#include <rtecg_rtc.h>
#include <rtecg_time.h>
#include <rtecg_heartbeat.h>

#define pin_ecg A2
#define pin_led 13


// classifier state data structures
uint32_t tmicros;
rtecg_ptlp lp; // low pass filter
rtecg_pthp hp; // high pass filter
rtecg_ptd d; // derivative filter
rtecg_pti mwi; // moving window integrator
rtecg_pk pkf; // peaks in the filtered signal
rtecg_pk pki; // peaks in the integrated signal
rtecg_pt pts; // pan-tompkins algorithm applied to pkf and pki

// networking
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ipaddy(192, 168, 0, 200);
IPAddress localipaddy(192, 168, 0, 130);
const unsigned int port = 9998;

EthernetUDP udp;

void setup()
{
	Serial.begin(115200);
	pinMode(pin_ecg, INPUT); // analog pin that the ecg is attached to
	pinMode(pin_led, OUTPUT); // led to blink when there's a heartbeat
	digitalWrite(pin_led, LOW);

	//Ethernet.init(15); // ESP8266 with Adafruit Featherwing Ethernet
	Ethernet.init(33); // ESP32 with Adafruit Featherwing Ethernet
	Ethernet.begin(mac);
	if(Ethernet.hardwareStatus() == EthernetNoHardware){
		Serial.println("No Ethernet hardware found.");
	}
	if(Ethernet.linkStatus() == LinkOFF){
		Serial.println("Ethernet cable is not connected.");
	}
	udp.begin(8888);

	// initialize feature classification data structures
	lp = rtecg_ptlp_init();
	hp = rtecg_pthp_init();
	d = rtecg_ptd_init();
	mwi = rtecg_pti_init();
	pkf = rtecg_pk_init();
	pki = rtecg_pk_init();
	pts = rtecg_pt_init();

	rtecg_osc_init_pt(0);

	//rtecg_rtc_init(pin_rtc_sqw);
	rtecg_time_init();
}

void loop()
{
	//int sample_width = rtecg_rtc_wait();
	int sample_width = rtecg_time_wait();

	// now read ECG pin
	rtecg_int ecgval = analogRead(pin_ecg);

	//rtecg_rtc_tick();
	rtecg_time_tick();

	// yield control in case any housekeeping needs to get done
	yield();

	// turn off LED if it was on from previous loop
	if(digitalRead(pin_led) == HIGH){
		digitalWrite(pin_led, HIGH);
	}

	// filter ECG signal
	lp = rtecg_ptlp_hx0(lp, ecgval);
	hp = rtecg_pthp_hx0(hp, rtecg_ptlp_y0(lp));
	d = rtecg_ptd_hx0(d, rtecg_pthp_y0(hp));
	mwi = rtecg_pti_hx0(mwi, rtecg_ptd_y0(d));
	// mark peaks
	pkf = rtecg_pk_mark(pkf, rtecg_pthp_y0(hp));
	pki = rtecg_pk_mark(pki, rtecg_pti_y0(mwi));
	int buflen = 1;
	char buf[buflen];
	buf[0] = 0;
	pts = rtecg_pt_process(pts, rtecg_pk_y0(pkf) * rtecg_pk_xm82(pkf), rtecg_pk_maxslope(pkf), rtecg_pk_y0(pki) * rtecg_pk_xm82(pki), rtecg_pk_maxslope(pki), buf, buflen, 0);

	// yield again in case the classification took a long time
	yield();

	// perform searchback if a peak hasn't been found in the expected time frame
	if(pts.searchback){
		buf[0] = 0;
		pts = rtecg_pt_searchback(pts, buf, buflen, 0);
		// yield again just in case
		yield();
	}

	float rr = pts.rr / (float)RTECG_FS;
	if(rr){
		rr = 60. / rr;
	}
	float rravg1 = pts.rravg1 / (float)RTECG_FS;
	if(rravg1){
		rravg1 = 60. / rravg1;
	}
	float rravg2 = pts.rravg2 / (float)RTECG_FS;
	if(rravg2){
		rravg2 = 60. / rravg2;
	}
	char *oscbndl = NULL;
	int oscbndl_size = rtecg_osc_wrap_pt(pts.ctr, // packet number
					     //rtecg_rtc_then(0), // micros
					     rtecg_time_then(0),
					     RTECG_FS, // fs
					     sample_width,
					     ecgval, // raw
					     rtecg_pthp_y0(hp), // filtered
					     rtecg_pti_y0(mwi), // mwi
					     pts.ctr - rtecg_pt_last_spkf(pts).x, // spkf sample num
					     //rtecg_rtc_then(rtecg_pt_last_spkf(pts).x + RTECG_PKDEL + 1),
					     rtecg_time_then(rtecg_pt_last_spkf(pts).x + RTECG_PKDEL + 1),
					     rtecg_pt_last_spkf(pts).y, // spkf sample val
					     rtecg_pt_last_spkf(pts).confidence, // spkf confidence
					     pts.ctr - rtecg_pt_last_spki(pts).x, // spki sample num
					     //rtecg_rtc_then(rtecg_pt_last_spki(pts).x + RTECG_PKDEL + 1),
					     rtecg_time_then(rtecg_pt_last_spki(pts).x + RTECG_PKDEL + 1),
					     rtecg_pt_last_spki(pts).y, // spki sample val
					     rtecg_pt_last_spki(pts).confidence, // spki confidence
					     rr, 
					     rravg1, 
					     rravg2,
					     pts.f1,
					     pts.f2,
					     pts.i1,
					     pts.i2,
					     &oscbndl);
	int ret = udp.beginPacket(ipaddy, port);
	ret = udp.write((const uint8_t *)oscbndl, oscbndl_size);
	udp.endPacket();

	//flash LED if we have a peak
	if(pts.havepeak){
		if(digitalRead(pin_led) == HIGH){
			digitalWrite(pin_led, LOW);		
		}
	}
}
