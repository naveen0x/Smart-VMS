#include <WiFi.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <LiquidCrystal_I2C.h>


const char* ssid = "xxxxxx";
const char* password = "xxxxxxxxx";
#define API_KEY "AIzaSyDD1L5LqzJW-A9pz60LX5A8iQaoHKLXkK4"
#define DATABASE_URL "https://parking-9e566-default-rtdb.firebaseio.com"
#define SOUND_SPEED 0.034
int lcdColumns = 20;
int lcdRows = 4;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

int QRValue;
int S1Value;
int S2Value;
int G1Value;

Servo myservo;
Servo myservo2;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

long duration;
float distanceCm;
const int trig1 = 5;
const int echo1 = 18;
const int trig2 = 0;
const int echo2 = 2;

int led1 = 13;
int led2 = 12;


void get_network_info() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("ESP32 Connected To ");
    Serial.println(ssid);
    Serial.println("BSSID :       " + WiFi.BSSIDstr());
    Serial.print("Gateway IP :  ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet Mask : ");
    Serial.println(WiFi.subnetMask());
    Serial.println((String)"RSSI :       " + WiFi.RSSI() + " dB");
    Serial.print("ESP32 IP :    ");
    Serial.println(WiFi.localIP());
  }
}

void setup() {

  Serial.begin(115200);
  delay(1000);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected to the WiFi network");
  get_network_info();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  lcd.init();
  lcd.backlight();
  myservo.attach(4);
  myservo2.attach(16);
  pinMode(trig1, OUTPUT);
  pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT);
  pinMode(echo2, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
}

float ultrasonic(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  duration = pulseIn(echo, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  if (distanceCm != 0.0) {
    if (distanceCm > 3.0) {
      return true;
    } else {
      return false;
    }
  }
}


void loop() {
  lcd.setCursor(0, 0);
  lcd.print(" FCT - CAR PARKING");


  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.getInt(&fbdo, "/QR")) {
      if (fbdo.dataType() == "int") {
        QRValue = fbdo.intData();
        Serial.println(QRValue);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getInt(&fbdo, "/Slot1")) {
      if (fbdo.dataType() == "int") {
        S1Value = fbdo.intData();
        Serial.println(S1Value);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getInt(&fbdo, "/Slot2")) {
      if (fbdo.dataType() == "int") {
        S2Value = fbdo.intData();
        Serial.println(S2Value);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "/Gate2")) {
      if (fbdo.dataType() == "int") {
        G1Value = fbdo.intData();
        Serial.println(G1Value);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }



  //lcd.clear();

  if ((!ultrasonic(trig1, echo1) || S1Value == 1) && (!ultrasonic(trig2, echo2) || S2Value == 1)) {
    lcd.setCursor(0, 1);
    lcd.print("    PARKING FULL    ");
  } else if (QRValue == 0) {
    lcd.setCursor(0, 1);
    lcd.print("      WELCOME!      ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("    >>DRIVE IN<<    ");
  }

  if (!ultrasonic(trig1, echo1)) {
    lcd.setCursor(0, 3);
    lcd.print("FULL  ");
  } else if (S1Value == 1) {
    lcd.setCursor(0, 3);
    lcd.print("BOOKED");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("EMPTY ");
  }

  if (!ultrasonic(trig2, echo2)) {
    lcd.setCursor(13, 3);
    lcd.print("FULL  ");
  } else if (S2Value == 1) {
    lcd.setCursor(13, 3);
    lcd.print("BOOKED");
  } else {
    lcd.setCursor(13, 3);
    lcd.print("EMPTY ");
  }

  lcd.setCursor(0, 2);
  lcd.print("Slot-01");
  lcd.setCursor(13, 2);
  lcd.print("Slot-02");



  //QR Pass
  if (S1Value == 0 && QRValue == 1 && ultrasonic(trig1, echo1)) {
    myservo.write(90);
    Firebase.RTDB.setInt(&fbdo, "/QR", 0);
    digitalWrite(led1, HIGH);
    delay(4000);
    myservo.write(0);
    while (ultrasonic(trig1, echo1)) {
      digitalWrite(led1, HIGH);
      delay(500);
    }
    delay(500);
    digitalWrite(led1, LOW);

  } else if (S2Value == 0 && QRValue == 1 && ultrasonic(trig2, echo2)) {
    myservo.write(90);
    Firebase.RTDB.setInt(&fbdo, "/QR", 0);
    digitalWrite(led2, HIGH);
    delay(4000);
    myservo.write(0);
    while (ultrasonic(trig2, echo2)) {
      digitalWrite(led2, HIGH);
      delay(500);
    }
    delay(500);
    digitalWrite(led2, LOW);

    //Bookings
  } else if (QRValue == 11) {
    myservo.write(90);
    Firebase.RTDB.setInt(&fbdo, "/QR", 0);
    digitalWrite(led1, HIGH);
    delay(4000);
    myservo.write(0);
    while (ultrasonic(trig1, echo1)) {
      digitalWrite(led1, HIGH);
      delay(500);
    }
    delay(500);
    digitalWrite(led1, LOW);
  } else if (QRValue == 12) {
    myservo.write(90);
    Firebase.RTDB.setInt(&fbdo, "/QR", 0);
    digitalWrite(led2, HIGH);
    delay(4000);
    myservo.write(0);
    while (ultrasonic(trig2, echo2)) {
      digitalWrite(led2, HIGH);
      delay(500);
    }
    delay(500);
    digitalWrite(led2, LOW);
  } else {
    myservo.write(0);
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
  }

  if (G1Value == 1) {
    myservo2.write(90);
    Firebase.RTDB.setInt(&fbdo, "/Gate2", 0);
    delay(4000);
    myservo2.write(0);
  } else {
    myservo2.write(0);
  }

  //Serial.println(ultrasonic(trig1, echo1));
  //Serial.println(ultrasonic(trig2, echo2));
  delay(100);
}
