#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>

// Disable brownout problems
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Firebase
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define API_KEY ""
#define DATABASE_URL ""
#define STORAGE_BUCKET_ID ""
#define FILE_PHOTO_PATH ""
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String uid;
unsigned long sendDataPrevMillis = 0;
int count = 0;
// Firebase user
#define USER_EMAIL ""
#define USER_PASSWORD ""

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

const int ESP32_RX_PIN = 3; 
const int ESP32_TX_PIN = 0; 
HardwareSerial espSerial(2);

// Camera pin
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

bool takeNewPhoto = true;
bool taskCompleted = false;
const char delimiter = '|';
String elements[3];
String indexArr[5] = {"desc", "time", "lat", "filler","lon"};

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);

  // Wifi connect
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

  // Firebase
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Init littleFS
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  } else {
    delay(500);
    Serial.println("LittleFS mounted successfully");
  }

  // init camera
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();  
  } 
}

void fcsUploadCallback(FCS_UploadStatusInfo info){
    if (info.status == firebase_fcs_upload_status_init){
      Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
    } else if (info.status == firebase_fcs_upload_status_upload){
      Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    } else if (info.status == firebase_fcs_upload_status_complete){
      Serial.println("Upload completed\n");
      FileMetaInfo meta = fbdo.metaData();
      Serial.printf("Name: %s\n", meta.name.c_str());
      Serial.printf("Bucket: %s\n", meta.bucket.c_str());
      Serial.printf("contentType: %s\n", meta.contentType.c_str());
      Serial.printf("Size: %d\n", meta.size);
      Serial.printf("Generation: %lu\n", meta.generation);
      Serial.printf("Metageneration: %lu\n", meta.metageneration);
      Serial.printf("ETag: %s\n", meta.etag.c_str());
      Serial.printf("CRC32: %s\n", meta.crc32.c_str());
      Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
      Serial.printf("Download URL: %s\n\n", fbdo.downloadURL().c_str());
    } else if (info.status == firebase_fcs_upload_status_error){
      Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void capturePhotoSaveLittleFS( void ) {
  // Dispose first pictures because of bad quality
  camera_fb_t* fb = NULL;
  // Skip first 3 frames (increase/decrease number as needed).
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
  // Take a new photo
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
}

void loop() {
  if (espSerial.available()) {
    // reset the variable
    String url = "";
    taskCompleted = false;
    takeNewPhoto = true;
    delay(1000);
    // random string
    String randomString = "";
    int stringLength = 16;
    for (int i = 0; i < stringLength; i++) {
      char randomChar = random(65, 91);
      if (random(2) == 0) {
        randomChar += 32;
      } else if (random(3) == 0) {
        randomChar = random(48, 58);
      }
      randomString += randomChar;
    }

    // Decode received data
    String data = espSerial.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(data);
    
    int index = 0;
    int startIndex = 0;
    int endIndex;

    // Explode string from esp8266
    while ((endIndex = data.indexOf(delimiter, startIndex)) != -1) {
      elements[index] = data.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
      index++;
    }
    // Add the last element
    elements[index] = data.substring(startIndex);

    // Make the photo name random string so its doesnt replace other image
    String bucketPath = "/data/"+randomString+".jpg";
    Serial.println(bucketPath);

    if (takeNewPhoto) {
      capturePhotoSaveLittleFS();
      takeNewPhoto = false;
    }
    delay(1);
    if (Firebase.ready() && !taskCompleted){
      taskCompleted = true;
      Serial.print("Uploading picture... ");
      //MIME type should be valid to avoid the download problem.
      //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, bucketPath, "image/jpeg", fcsUploadCallback)){
        Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
        url = fbdo.downloadURL().c_str();
      } else {
        Serial.println(fbdo.errorReason());
      }
    }
    if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
      sendDataPrevMillis = millis();

      // assign data to firebasejson
      FirebaseJson json;
      json.set("image_url", url);
      json.set("is_done", false);
      for (int i = 0; i < index + 1; i++) {
        if (indexArr[i] != "filler"){
          json.set(indexArr[i], String(elements[i]));
        }
      }

      if (Firebase.RTDB.setJSON(&fbdo, String("datas/"+randomString), &json)){
        Serial.println("Sent.");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      } else {
        Serial.println("FAILED");
        Serial.println("Data type: " + fbdo.dataType());
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
  }
}
