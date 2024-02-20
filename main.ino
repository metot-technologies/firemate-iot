#define analogsensor A0

#include <Arduino.h>
#include <random>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define API_KEY ""
#define DATABASE_URL "" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

#include <WiFiUdp.h>
#include <NTPClient.h>               
#include <TimeLib.h>    

const long utcOffsetInSeconds = (8 * 60 * 60);  // set offset (GMT * 60 * 60)
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);
char Time[ ] = "Jam: 00:00:00";
char Date[ ] = "Tgl: 00/00/2000";
char dateTime [100];
byte last_second, second_, minute_, hour_, day_, month_;
int year_;

// GPS
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
TinyGPSPlus gps;
SoftwareSerial SerialGPS(4, 5); // RX, TX
float latF, lonF;
String lat, lon;

//buzzer
#define buzzerpin D6
  
void setup() { 
  pinMode(analogsensor, INPUT);
  pinMode(buzzerpin, OUTPUT);
  Serial.begin(115200);

  randomSeed(analogRead(0));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  timeClient.begin();

  SerialGPS.begin(115200);
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
}


unsigned long getEpoch(){
  timeClient.update();
  return timeClient.getEpochTime();
}
void loop() {
  Serial.print("Analog value: ");
  Serial.println(analogRead(analogsensor));
  if(analogRead(analogsensor) > 800){
    Serial.println("Tidak ada api");
  }else if(analogRead(analogsensor) > 500){
    Serial.println("Kecil.");
    digitalWrite(buzzerpin, HIGH);
    delay(1000);
    digitalWrite(buzzerpin, LOW);
  }else if(analogRead(analogsensor) > 100 ){
    Serial.println("Api Sedang.");

    sendFirebase("Sedang", getEpoch());
    sendMessage();
  }else {
    timeClient.update();
    Serial.println("Api Tinggi.");

    sendFirebase("Tinggi", getEpoch());
    sendMessage();
  }
  delay(1000);
}
