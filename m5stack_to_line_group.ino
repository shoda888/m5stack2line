#include "config.h" // 定数呼び出し
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <M5Stack.h>
#include <TinyGPS++.h>
#include "Ambient.h"

HardwareSerial GPS_s(2);
TinyGPSPlus gps;

#define REDRAW 20 // msec
#define PERIOD 10 // sec

float pre_lat = 0; //緯度
float pre_lng = 0; //経度
float distance = 0; //累積距離
float d_lat = 0; //緯度差分
float d_lng = 0; //経度差分
int loopcount = 0;

const char* server = "maker.ifttt.com";  // Server URL
WiFiClient client;
Ambient ambient;

void wifiBegin() {
  WiFi.begin(ssid, password);
}

bool checkWifiConnected() {
  // attempt to connect to Wifi network:
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.print(".");
    // wait 1 second for re-trying
    delay(1000);

    count++;
    if (count > 15) { // about 15s
      M5.Lcd.println("(wifiConnect) failed!");
      return false;
    }
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  return true;
}

void send(String value1) {
  M5.Lcd.clear(BLACK);
  M5.Lcd.setCursor(3, 35);
  while (!checkWifiConnected()) {
    wifiBegin();
  }

  M5.Lcd.println("Starting connection to server...");
  if (!client.connect(server, 80)) {
    M5.Lcd.println("Connection failed!");
  } else {
    M5.Lcd.println("Connected to server!");
    HTTPClient http;
    http.begin("https://notify-api.line.me/api/notify?message=" + value1);
    http.addHeader("Authorization", "Bearer " + lineGroup);
    int httpResponseCode = http.POST("POSTING from M5stack");

    M5.Lcd.print("Waiting for response "); //WiFiClientSecure uses a non blocking implementation


    if(httpResponseCode>0){
      String response = http.getString();
      M5.Lcd.println(httpResponseCode);
      M5.Lcd.println(response);
    }else{
      M5.Lcd.print("Error on sending POST: ");
      M5.Lcd.println(httpResponseCode);
    }
    http.end();
  }
}

void setup() {
  M5.begin();
  dacWrite(25, 0); // Speaker OFF
  
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(65, 10);
  M5.Lcd.println("GPS EXAMPLE");
  M5.Lcd.setCursor(3, 35);
  GPS_s.begin(9600);
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(1000);

  M5.Lcd.print("Attempting to connect to SSID: ");
  M5.Lcd.println(ssid);
  wifiBegin();
  while (!checkWifiConnected()) {
    wifiBegin();
  }
  ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化
  ambient.delete_data(userKey); // 前回のAmbientデータを削除
  
  M5.Lcd.println("WiFiconnected!!");
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    M5.Lcd.println("start walking!!");
    while(1){
         M5.update();
         delay(REDRAW);
         if (++loopcount > PERIOD * 1000 / REDRAW) {
            char buf[16];
            Serial.println(distance);
            loopcount = 0;
    
            while (!gps.location.isUpdated()) {
                while (GPS_s.available() > 0) {
                    if (gps.encode(GPS_s.read())) {
                        break;
                    }
                    delay(0);
                }
            }
            
            if(pre_lat && pre_lng) {
              pre_lat = gps.location.lat();
              pre_lng = gps.location.lng();
              d_lat = gps.location.lat() - pre_lat;
              d_lng = gps.location.lng() - pre_lng;
              distance += sqrt(sq(d_lat) + sq(d_lng)) * 100 * 1000;
            } else {
              pre_lat = gps.location.lat();
              pre_lng = gps.location.lng();
            }
            M5.Lcd.printf("distance: %5.2fm\r\n", distance);
            dtostrf(gps.location.lat(), 12, 8, buf);
            ambient.set(9, buf);
            dtostrf(gps.location.lng(), 12, 8, buf);
            ambient.set(10, buf);
            ambient.send();
         }
         if (M5.BtnB.wasPressed()){
            M5.Lcd.println("finish walking!!");
            send(String(distance) + "m");
            send("https://ambidata.io/ch/channel.html?id=" + String(channelId) +"&private=true");
            M5.powerOFF();
         }
     }
  }
}
