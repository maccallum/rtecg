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
byte mac[6];
char macstr[13];
IPAddress ip_local, ip_remote(192, 168, 0, 200), ip_bcast;
const unsigned int port_local = 8888;
const unsigned int port_remote = 9998;
const unsigned int port_bcast = 323232;

EthernetUDP udp;

void setup()
{
	Serial.begin(115200);
	pinMode(pin_ecg, INPUT); // analog pin that the ecg is attached to
	pinMode(pin_led, OUTPUT); // led to blink when there's a heartbeat
	digitalWrite(pin_led, LOW);

	//Ethernet.init(15); // ESP8266 with Adafruit Featherwing Ethernet
	Ethernet.init(33); // ESP32 with Adafruit Featherwing Ethernet

	// ESP32 has an ethernet mac address which is the basemac with 3 added to the last octet
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
	mac[5] += 3;
	Ethernet.begin(mac);
	if(Ethernet.hardwareStatus() == EthernetNoHardware){
		Serial.println("No Ethernet hardware found.");
	}
	if(Ethernet.linkStatus() == LinkOFF){
		Serial.println("Ethernet cable is not connected.");
	}
	udp.begin(port_local);

	// initialize feature classification data structures
	lp = rtecg_ptlp_init();
	hp = rtecg_pthp_init();
	d = rtecg_ptd_init();
	mwi = rtecg_pti_init();
	pkf = rtecg_pk_init();
	pki = rtecg_pk_init();
	pts = rtecg_pt_init();

	snprintf(macstr, 13, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	rtecg_osc_init_pt(macstr, 12);
	
	//rtecg_rtc_init(pin_rtc_sqw);
	rtecg_time_init();
	ip_local = Ethernet.localIP();
	ip_bcast = ip_local;
	ip_bcast[3] = 255;
	uint32_t ip = (uint32_t)ip_local;
	rtecg_heartbeat_init((char *)mac, (char *)(&ip), port_local, macstr, 12);
}

char incoming_packet[UDP_TX_PACKET_MAX_SIZE];
void loop()
{
	int size = udp.parsePacket();
	if(size){
		udp.read(incoming_packet, UDP_TX_PACKET_MAX_SIZE);
		t_osc_timetag t = {0, 0};
		if(size >= 16 && !strncmp(incoming_packet, "#bundle\0", 8)){
			t = osc_timetag_decodeFromHeader(incoming_packet + 8);
		}
		//int sample_width = rtecg_rtc_wait();
		//int sample_width = rtecg_time_wait();
		int sample_width = 0;

		// now read ECG pin
		rtecg_int ecgval = analogRead(pin_ecg);

		rtecg_time_set(t);
		//rtecg_rtc_tick();
		//rtecg_time_tick();

		// yield control in case any housekeeping needs to get done
		yield();

		// turn off LED if it was on from previous loop
		// if(digitalRead(pin_led) == LOW){
		// 	digitalWrite(pin_led, HIGH);
		// }

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
		int ret = udp.beginPacket(ip_remote, port_remote);
		ret = udp.write((const uint8_t *)oscbndl, oscbndl_size);
		ret = udp.endPacket();

		// flash LED if we have a peak
		if(pts.havepeak){
			//if(digitalRead(pin_led) == LOW){
			digitalWrite(pin_led, HIGH);
				//}
		}else{
			digitalWrite(pin_led, LOW);
		}
	}else{
		yield();
	}
	delay(1);
	static int hbctr;
	if(++hbctr == 1000){
		int ret = udp.beginPacket(ip_bcast, port_bcast);
		ret = udp.write(rtecg_heartbeat_bndl(), rtecg_heartbeat_len());
		ret = udp.endPacket();
		hbctr = 0;
	}
}
