void sendFirebase(String desc, unsigned long date) {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // generate random string
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

    // GPS
    if (gps.encode(SerialGPS.read())){
      if (gps.location.isValid()){
        latF = gps.location.lat();
        lat = String(latF , 6);
        lonF = gps.location.lng();
        lon = String(lonF , 6);
      }
    }

    FirebaseJson json;
    json.set("time", date);
    json.set("desc", desc);
    json.set("is_done", false);
    json.set("lat", lat);
    json.set("lon", lon);

    Firebase.RTDB.setJSON(&fbdo, "datas/"+randomString, &json) ? Serial.println("Sent.") : Serial.println("FAILED! REASON: " + fbdo.errorReason());
  }
}
