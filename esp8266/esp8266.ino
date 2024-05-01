#include <SoftwareSerial.h>
#include <Arduino.h>
#include <random>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>               
#include <TimeLib.h> 
#include <TinyGPS++.h>

SoftwareSerial espSerial(3, 1); // RX TX

#define analogsensor A0
#define smokesensor D7
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

const long utcOffsetInSeconds = (8 * 60 * 60);  // set offset (GMT * 60 * 60)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);

// GPS
TinyGPSPlus gps;
SoftwareSerial SerialGPS(4, 5); // RX, TX
float latF, lonF;
String lat, lon;

//buzzer
#define buzzerpin D6

// Firebase
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#define FIREBASE_PROJECT_ID ""
#define FIREBASE_CLIENT_EMAIL ""
const char PRIVATE_KEY[] PROGMEM = "";
#define DEVICE_REGISTRATION_ID_TOKEN ""
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long lastTime = 0;
int count = 0;

// Pin to switch analog
#define plusFlame D5

void setup() { 
  pinMode(analogsensor, INPUT);
  pinMode(smokesensor, INPUT);
  pinMode(buzzerpin, OUTPUT);
  Serial.begin(9600);
  espSerial.begin(9600);
  SerialGPS.begin(115200);

  randomSeed(analogRead(0));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

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

  // Init the firebase config
  config.service_account.data.client_email = FIREBASE_CLIENT_EMAIL;
  config.service_account.data.project_id = FIREBASE_PROJECT_ID;
  config.service_account.data.private_key = PRIVATE_KEY;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);

  timeClient.begin();
}

unsigned long getEpoch(){
  timeClient.update();
  return timeClient.getEpochTime();
}

void sendData(String desc, unsigned long date) {
  // GPS
  if (gps.encode(SerialGPS.read())){
    if (gps.location.isValid()){
      latF = gps.location.lat();
      lat = String(latF , 6);
      lonF = gps.location.lng();
      lon = String(lonF , 6);
    }
  }

  //FCM
  if (Firebase.ready() && (millis() - lastTime > 60 * 1000 || lastTime == 0)){
    lastTime = millis();

    Serial.print("Send Firebase Cloud Messaging... ");

    FCM_HTTPv1_JSON_Message msg;
    msg.token = DEVICE_REGISTRATION_ID_TOKEN;
    msg.notification.body = "Klik untuk detail";
    msg.notification.title = "🔔Kebakaran";

    if (Firebase.FCM.send(&fbdo, &msg)){
      Serial.printf("ok\n%s\n\n", Firebase.FCM.payload(&fbdo).c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
    
    count++;
  }
  
  // Here, "none" is needed because of the undefined data when sending these string to ESP32
  String send = desc+"|"+String(date)+"|"+lat+"|none|"+lon;
  espSerial.println(send);
}

void loop() {
  int analogSensor = analogRead(analogsensor);
  // Always make the flame sensor LOW first
  digitalWrite(plusFlame, LOW);
  // If the smoke sensor detect smoke
  // Then the flame sensor will turned on
  if(analogRead(analogsensor) < 100){ 
    digitalWrite(plusFlame, HIGH);
    if(analogRead(analogsensor) > 800){
      Serial.println("Tidak ada api");
    }else if(analogRead(analogsensor) > 500){
      Serial.println("Kecil.");
      digitalWrite(buzzerpin, HIGH);
      delay(1000);
      digitalWrite(buzzerpin, LOW);
    }else if(analogRead(analogsensor) > 100 ){
      Serial.println("Api Sedang.");
      sendData("Sedang", getEpoch());
    }else {
      timeClient.update();
      Serial.println("Api Tinggi.");
      sendData("Tinggi", getEpoch());
    }
  }
  delay(1000);
}
