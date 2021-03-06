/*
 * CocoaBit as Kadecot GPIO device
 *  MIT License. (C) 2016 Seigo Tanaka & Shigeru Owada
 *  Repository: https://github.com/SonyCSL/CocoaBitKadecot
 *  Japanese instructions : http://qiita.com/sgrowd/items/9ef56370a49f4f10c96c
 *
 */

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <Nefry.h>
#include<NefryWriteMode.h>

#define USE_SERIAL Nefry

#define KADECOT_DEFAULT_SERIAL_PORT 41316

int INPUT_ANALOG_PORT = A0;
int OUTPUT_PORT = D3;

void p(const char* str){ Nefry.print(str); }
void pln(const char* str){ Nefry.println(str); }
void p(String& str){ Nefry.print(str); }
void pln(String& str){ Nefry.println(str); }

// WriteMode ----------------------------------------------
void WriteModeSetup() {
  pln("Write Mode Setup");
}
void WriteModeloop() {
  Nefry.setLed(random(250), random(255), random(255));
  Nefry.ndelay(500);
}
NefryWriteMode WriteMode(WriteModeSetup, WriteModeloop);

// Main ----------------------------------------------
WebSocketsClient webSocket;

int port;
String ip;

String recvStr = "" ;
int sensorValue = -1;
int sensorValuePrevious = 500;

byte strbuf[256] ;
void sendSerial(String txt){
  txt.getBytes(strbuf,256);
  webSocket.sendTXT(strbuf) ;
  webSocket.sendTXT(";") ;
}

void onSerial(String txt){
    // Split txt into command tokens
  String cmd[txt.length()] ;
  int cmd_len = 0 ;
  {  
    int idx=0 ;
    while(1) {
      int nidx = txt.indexOf(":",idx) ;
      cmd[cmd_len++] = txt.substring(idx,nidx) ;
      if( nidx == -1 ) break ;
      idx = nidx+1 ;
    }
  }

  if( cmd[0].equals("set") ){
    // Ignore pin number (cmd[1])  / should be 3
    digitalWrite(OUTPUT_PORT, cmd[2].toFloat() < 0.5f ? LOW : HIGH ) ;
  } else if( cmd[0].equals("get") ){
    // Ignore pin number (cmd[1])  / should be zero
    sendSerial( String("rep:")+(sensorValue/1023.0f)+":"+cmd[2] ) ;
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    String txt = String((const char *) payload) ;
  
    switch(type) {
        case WStype_DISCONNECTED:
              pln("Disconnected");
            break;
        case WStype_CONNECTED:
              pln("Connected:"+txt);
              // Kadecot format declaration of generic GPIO device over WebSocket/Socket
              sendSerial(String(Nefry.getModuleName())+"/in:0/out:0/mode:gpio") ;
            break;
        case WStype_TEXT:
              //pln("text: "+ txt );
              int scp ;
              while( (scp = txt.indexOf(';')) >= 0 ){
                onSerial(recvStr + txt.substring(0,scp)) ;
                recvStr = "" ;
                txt = txt.substring(scp+1) ;
              }
              recvStr += txt ;
            break;
        case WStype_BIN:
              //pln("Binary len:"+length);
              hexdump(payload, length);
            break;
    }

}

void setup() {
    pinMode(OUTPUT_PORT, OUTPUT);
    p("Booting");
    for(uint8_t t = 4; t > 0; t--) {
      p(".");
      delay(1000);
    }
    pln("ok.");
    Nefry.setConfHtml("Kadecot IP",0);
    //Nefry.setConfHtml("Kadecot Port",KADECOT_DEFAULT_SERIAL_PORT);
    ip=Nefry.getConfStr(0);
    //port=Nefry.getConfValue(0);
    port=KADECOT_DEFAULT_SERIAL_PORT;
    webSocket.begin(ip.c_str (), port);
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    sensorValue = analogRead( INPUT_ANALOG_PORT);
    //pln(sensorValue);
    Nefry.ndelay(10) ;


    int spanValue = abs( sensorValuePrevious - sensorValue );

    // 値に大きく変化があったら
    if( spanValue > 50 ){
      pln( String(Nefry.getModuleName()) + ":" + port +":" + String(sensorValue) );
      Nefry.ndelay(10) ;
      sendSerial( String("pub:0:")+(sensorValue/1023.0f) ) ;

      sensorValuePrevious = sensorValue;
    }
    Nefry.ndelay(10);

    webSocket.loop();
}
