#include <OSCBundle.h>
#include <OSCBoards.h>

#ifdef BOARD_HAS_USB_SERIAL
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
#else
#include <SLIPEncodedSerial.h>
 SLIPEncodedSerial SLIPSerial(Serial);
#endif

void setup() {
  Serial.begin(115200);
}

unsigned long pcm = 0;

void loop() {
  unsigned long cm = micros();
  if(pcm == 0){
    pcm = micros();
  }
  while(cm - pcm < 5000){
    cm = micros();
  }
  pcm = cm;
  OSCBundle b;
  b.add("/a0").add(analogRead(A0));
  b.add("/a1").add(analogRead(A1));
  b.add("/a2").add(analogRead(A2));
  SLIPSerial.beginPacket();
  b.send(SLIPSerial);
  SLIPSerial.endPacket();
}
