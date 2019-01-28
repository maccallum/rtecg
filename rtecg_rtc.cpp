#include <rtecg.h>
#include <rtecg_rtc.h>
#include <lib/libo/osc_timetag.h>
#include <inttypes.h>
#include <Wire.h>
#include <RTClib.h>

static uint32_t micros_ival = 1000000. / RTECG_FS;
static uint32_t micros_prev = 0;

static int pin_rtc_sqw = -1;
static int int_a1 = 0;
static uint32_t micros_ref = 0;
static t_osc_timetag current_date;
static t_osc_timetag tlst[RTECG_FS * 2];
static int tptr = 0;

static RTC_DS3231 rtc;

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

t_osc_timetag rtecg_rtc_dateToOSCTimetag(DateTime d)
{
	t_osc_timetag time = {0, 0};
	time.sec = d.unixtime() + 2208988800L;
	return time;
}

void rtecg_rtc_isr_a1()
{
	micros_ref = micros();
	int_a1 = 1;
}

void rtecg_rtc_init(int rtc_square_wave_pin)
{
	memset(tlst, 0, sizeof(t_osc_timetag) * (RTECG_FS * 2));
	pin_rtc_sqw = rtc_square_wave_pin;
	pinMode(pin_rtc_sqw, INPUT); // this is the RTC alarm
	digitalWrite(pin_rtc_sqw, HIGH);
	attachInterrupt(digitalPinToInterrupt(pin_rtc_sqw), rtecg_rtc_isr_a1, FALLING);
	int r = rtc.begin();
	if(r){
		write_i2c_register(DS3231_ADDRESS, 0x0e, 0);
	}
	micros_prev = micros();
}

int rtecg_rtc_wait(void)
{
	uint32_t micros_cur = micros();
	int d;
	while((d = (micros_cur - micros_prev)) < micros_ival){
		micros_cur = micros();
	}
	micros_prev = micros_cur;
	return d;
}

int rtecg_rtc_tick(void)
{
	int ret = 0;
	if(int_a1){
		int_a1 = 0;
		DateTime d = rtc.now();
		current_date = rtecg_rtc_dateToOSCTimetag(d);
		ret = 1;
	}else{
	}
	//tlst[tptr++] = rtecg_rtc_now();
	noInterrupts();
	t_osc_timetag now = current_date;
	interrupts();
	//now.frac_sec = (uint32_t)(0xFFFFFFFFUL * ((micros_prev - micros_ref) / 1000000.));
	double d = (micros_prev - micros_ref) / 1000000.;
	if(d < 0){
		now = osc_timetag_subtract(now, osc_timetag_floatToTimetag(d));
	}else{
		now = osc_timetag_add(now, osc_timetag_floatToTimetag(d));
	}
	tlst[tptr++] = now;
	if(tptr == (RTECG_FS * 2)){
		tptr = 0;
	}
	return ret;
}

t_osc_timetag rtecg_rtc_now(void)
{
	noInterrupts();
	t_osc_timetag now = current_date;
	interrupts();
	//now.frac_sec = (uint32_t)(0xFFFFFFFFUL * ((micros() - micros_ref) / 1000000.));
	uint32_t m = micros();
	double d = (m - micros_ref) / 1000000.;
	if(d < 0){
		now = osc_timetag_subtract(now, osc_timetag_floatToTimetag(d));
	}else{
		now = osc_timetag_add(now, osc_timetag_floatToTimetag(d));
	}
	return now;
}

t_osc_timetag rtecg_rtc_then(int nticks_in_past)
{
	int idx = tptr - nticks_in_past - 1;
	while(idx < 0){
		idx += (RTECG_FS * 2);
	}
	return tlst[idx];
}
