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
#include <rtecg_rand.h>

#define pin_ecg A2
#define pin_led 13
#define pin_bat A13
#define pin_lom 39
#define pin_lop 36

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
char macstr[14];
IPAddress ip_local, ip_remote(192, 168, 0, 200), ip_bcast;
const unsigned int port_local = 8888;
unsigned int port_remote = 9998;
const unsigned int port_bcast = 323232;

EthernetUDP udp;

// variables
int perform_searchback = 1;

void setup()
{
	Serial.begin(115200);

	pinMode(pin_ecg, INPUT); // analog pin that the ecg is attached to
	pinMode(pin_led, OUTPUT); // led to blink when there's a heartbeat
	digitalWrite(pin_led, LOW);
	pinMode(pin_bat, INPUT);
	pinMode(pin_lom, INPUT);
	pinMode(pin_lop, INPUT);

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

	snprintf(macstr, 14, "/%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	rtecg_osc_init_pt(macstr, 13);
	
	rtecg_time_init();
	ip_local = Ethernet.localIP();
	ip_bcast = ip_local;
	ip_bcast[3] = 255;
	uint32_t ipl = (uint32_t)ip_local;
	uint32_t ipr = (uint32_t)ip_remote;
	rtecg_heartbeat_init((char *)mac, (char *)(&ipl), port_local, (char *)(&ipr), port_remote, macstr, 12);

	rtecg_set_rand_max(UINT32_MAX);
	rtecg_set_rand(esp_random);
}

uint32_t hbtime = 0;
char incoming_packet[UDP_TX_PACKET_MAX_SIZE];
void loop()
{
	// read ECG pin and lead off states right away
	rtecg_int ecgval = analogRead(pin_ecg);
	rtecg_int lom = digitalRead(pin_lom);
	rtecg_int lop = digitalRead(pin_lop);	
	
	int size = udp.parsePacket();
	if(size){
		udp.read(incoming_packet, UDP_TX_PACKET_MAX_SIZE);
		t_osc_timetag t = rtecg_osc_getTimetagFromHeader(size, incoming_packet);

		int sample_width = 0;

		rtecg_time_set(t);

		// yield control in case any housekeeping needs to get done
		yield();

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
		pts = rtecg_pt_process(pts, rtecg_pk_y0(pkf) * rtecg_pk_xmNm1d2(pkf), rtecg_pk_maxslope(pkf), rtecg_pk_y0(pki) * rtecg_pk_xmNm1d2(pki), rtecg_pk_maxslope(pki), buf, buflen, 0);
		//Serial.print(buf);

		// yield again in case the classification took a long time
		yield();

		// perform searchback if a peak hasn't been found in the expected time frame
		if(pts.searchback){
			if(perform_searchback){
				buf[0] = 0;
				pts = rtecg_pt_searchback(pts, buf, buflen, 0);
				//Serial.println(buf);
			}else{
				pts = rtecg_pt_recordMissedPeak(pts, buf, buflen, 0);
			}
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
						     rtecg_time_then(0),
						     RTECG_FS, // fs
						     sample_width,
						     lom, // lead off
					     	     lop, // lead off
						     ecgval, // raw
						     rtecg_pthp_y0(hp), // filtered
						     rtecg_pti_y0(mwi), // mwi
						     pts.ctr - rtecg_pt_last_spkf(pts).x, // spkf sample num
						     rtecg_time_then(rtecg_pt_last_spkf(pts).x + RTECG_PKDEL), // spkf sample time
						     rtecg_pt_last_spkf(pts).y, // spkf sample val
						     rtecg_pt_last_spkf(pts).confidence, // spkf confidence
						     pts.ctr - rtecg_pt_last_spki(pts).x, // spki sample num
						     rtecg_time_then(rtecg_pt_last_spki(pts).x + RTECG_PKDEL), // spki sample time
						     rtecg_pt_last_spki(pts).y, // spki sample val
						     rtecg_pt_last_spki(pts).confidence, // spki confidence
						     rr, 
						     rravg1, 
						     rravg2,
						     pts.f1,
						     pts.f2,
						     pts.i1,
						     pts.i2,
						     (analogRead(pin_bat) * 2.) / 1000., // battery
						     &oscbndl);

		IPAddress ipaddy = udp.remoteIP();
		char ip[4] = {ipaddy[0], ipaddy[1], ipaddy[2], ipaddy[3]};
	        rtecg_osc_getIPAddress(size, incoming_packet, ip);
		ipaddy[0] = ip[0];
		ipaddy[1] = ip[1];
		ipaddy[2] = ip[2];
		ipaddy[3] = ip[3];
		uint32_t port = 0;
		rtecg_heartbeat_set_ip_remote(ip);
		int ret = rtecg_osc_getPort(size, incoming_packet, &port);
		if(!ret){
			port_remote = port;
			rtecg_heartbeat_set_port_remote(port);
		}
		ret = udp.beginPacket(ipaddy, port_remote);
		ret = udp.write((const uint8_t *)oscbndl, oscbndl_size);
		ret = udp.endPacket();

		// flash LED if we have a peak
		if(pts.havepeak){
			digitalWrite(pin_led, HIGH);
		}else{
			digitalWrite(pin_led, LOW);
		}
		//}else{
		//yield();
	}
	//delay(1);
	yield();
	//static int hbctr;
	//if(++hbctr == 1000){
	uint32_t hbt = millis();
	if(hbt - hbtime >= 1000){
		int ret = udp.beginPacket(ip_bcast, port_bcast);
		ret = udp.write(rtecg_heartbeat_bndl(), rtecg_heartbeat_len());
		ret = udp.endPacket();
		//hbctr = 0;
		hbtime = hbt;
	}
}
