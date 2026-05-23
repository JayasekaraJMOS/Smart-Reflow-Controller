#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SPI.h>

#define SS_PIN 10
#define RST_PIN 9
#define SWITCH_PIN 2  // Toggle switch pin

Servo myservo;  // create Servo object to control a servo


MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// List of authorized tags
byte authorizedTags[1][4] = {
  { 0x2E, 0x73, 0x95, 0x04 }
};
// White Card
const byte SafetyBreak[][4] = { 0xD4, 0xCF, 0x69, 0x05 };

int safety=0;
int sensorPin = A0;  // LM35 output connected to A0
int sensorValue = 0;
float temperatureC = 0;



// Pin definitions
const int CS_PIN = 4;  // Chip Select (you said D4–D7 free, we pick D4)

// Reflow profile parameters
const int T0=25;      // Start temp
const int Tpre = 90;   // End of preheat
const int Tsoak = 140;  // End of soak
const int Tpeak = 200;  // Peak temp

const int t_pre = 90;   // Preheat duration (s)
const int t_soak = 90;  // Soak duration (s)
const int t_ramp = 60;  // Ramp to peak (s)
const int t_peak = 10;  // Peak hold (s)
const int r_cool = 4;   // Cooling rate (°C/s)

unsigned long startMillis;

float reflowSetpoint(unsigned long elapsedSec) {
  if (elapsedSec < t_pre) {
    // Preheat: 25 → 150
    return T0 + (float)(Tpre - T0) / t_pre * elapsedSec;
  } else if (elapsedSec < t_pre + t_soak) {
    // Soak: 150 → 200
    return Tpre + (float)(Tsoak - Tpre) / t_soak * (elapsedSec - t_pre);
  } else if (elapsedSec < t_pre + t_soak + t_ramp) {
    // Ramp: 200 → 260
    return Tsoak + (float)(Tpeak - Tsoak) / t_ramp * (elapsedSec - (t_pre + t_soak));
  } else if (elapsedSec < t_pre + t_soak + t_ramp + t_peak) {
    // Peak: hold at 260
    return Tpeak;
  } else {
    // Cooling: drop from 260 down
    unsigned long coolTime = elapsedSec - (t_pre + t_soak + t_ramp + t_peak);
    float temp = Tpeak - r_cool * coolTime;
    if (temp < 25) temp = 25;
    return temp;
  }
}




double readTemperature() {
  digitalWrite(CS_PIN, LOW);  // select sensor
  delayMicroseconds(10);

  // Read 16 bits from sensor
  uint16_t v = SPI.transfer16(0x00);

  digitalWrite(CS_PIN, HIGH);  // deselect sensor

  // Check if thermocouple is connected (D2 bit = 0 means OK)
  if (v & 0x4) {
    return NAN;  // no thermocouple connected
  }

  v >>= 3;                        // remove lower 3 bits
  double temperature = v * 0.25;  // each bit = 0.25 °C

  return temperature;
}

void setup() {
  lcd.init();
  myservo.attach(3);

  Serial.begin(9600);

  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);  // keep CS high (inactive)

  pinMode(SWITCH_PIN, INPUT_PULLUP);  // Switch with internal pull-up

  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);

  SPI.begin();
  rfid.PCD_Init();
  lcd.setCursor(0, 0);

  // Wait until toggle switch is ON
  while (digitalRead(SWITCH_PIN) == HIGH) {
  }
  lcd.backlight();
  delay(1000);
  lcd.print("Well Come!!!");
  delay(1000);
  lcd.clear();
}

void loop() {

  // Switch is ON → start RFID scanning
  lcd.setCursor(0, 0);
  lcd.print("Scan to start");

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Check if scanned UID is authorized
  if (isAuthorized()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Authorized!");
    lcd.setCursor(0, 0);

    startProgram();  // Your program code

  } 
  else {
    lcd.setCursor(0, 0);
    lcd.print("Unauthorized tag");

  // Display UID on LCD
    lcd.setCursor(0, 1);
    lcd.print("UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
    String uidPart = String(rfid.uid.uidByte[i], HEX);
    if (uidPart.length() < 2) uidPart = "0" + uidPart;
    uidPart.toUpperCase();
    lcd.print(uidPart);
    lcd.print(" ");
  }

    //Blue LED and Beep
    digitalWrite(7, HIGH);
    delay(1000);
    digitalWrite(7, LOW);
    delay(1000);
    lcd.clear();
  }

  rfid.PICC_HaltA();  // Stop communication with the card
  delay(2000);        // Wait a moment before next scan
}

// Check if scanned UID matches any authorized tag
bool isAuthorized() {
  for (byte t = 0; t < sizeof(authorizedTags) / sizeof(authorizedTags[0]); t++) {
    bool match = true;
    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != authorizedTags[t][i]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

// Safety Break
bool isNoSafety() {
  for (byte t = 0; t < sizeof(SafetyBreak) / sizeof(SafetyBreak[0]); t++) {
    bool match = true;
    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != SafetyBreak[t][i]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}


// Your main program code
void startProgram() 
{

  lcd.clear();
  delay(1000);  

while(1){

    lcd.clear();  
    lcd.setCursor(0,0);
    lcd.print("Choose the Mode");
    delay(500);

//button
if (digitalRead(SWITCH_PIN)==LOW) {
  safety=1;

  //Display
  lcd.clear(); 
  lcd.setCursor(0,0);
  lcd.print("Safety ON");


  for (int i = 90; i >= 30; i--) {
    myservo.write(i);  // tell servo to go to position
    delay(25);
  }; 

   break;
}

else if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if(isNoSafety()){
      safety=0;
    lcd.clear();  
    lcd.setCursor(0,0);
    lcd.print("Safety OFF");
    
    //Beep and LED
    digitalWrite(7, HIGH);
    delay(100);  
    digitalWrite(7, LOW);
      
    
    rfid.PICC_HaltA();  // Stop communication with the card
    break;
    }
    else{
    lcd.clear();  
    // Display UID on LCD
    lcd.setCursor(0, 1);
    lcd.print("UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
    String uidPart = String(rfid.uid.uidByte[i], HEX);
    if (uidPart.length() < 2) uidPart = "0" + uidPart;
    uidPart.toUpperCase();
    lcd.print(uidPart);
    lcd.print(" ");
                    }
    rfid.PICC_HaltA();  // Stop communication with the card

    //Blue LED and Beep
    digitalWrite(7, HIGH);
    delay(1000);
    digitalWrite(7, LOW);
    lcd.clear();
    }
}  
  lcd.clear();
}
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place the PCB &");
  lcd.setCursor(0, 1);
  lcd.print("Press the button");
 
  
  while (digitalRead(SWITCH_PIN) == HIGH) {}
  lcd.clear();
 

  // starting timer of the Chart of temp
  startMillis = millis();
  while (1) {

    unsigned long elapsedSec = (millis() - startMillis) / 1000;
    float target = reflowSetpoint(elapsedSec);
    
    if(readTemperature()>=200 ){break;}

    if (readTemperature() < target) {
      //Activating the heater
      digitalWrite(8, HIGH);
    } else if (readTemperature() > target) {
      //Activating the heater
      digitalWrite(8, LOW);
    }
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(readTemperature());
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("TIME:");
    lcd.print(elapsedSec);
    lcd.print(" S");

    Serial.println(readTemperature());  // send to temperature to plotter
    delay(500);

    lcd.clear();

    //Changing the Led Base on Temperatuer
    if(readTemperature()>45){
      digitalWrite(5,HIGH);
      digitalWrite(6,LOW);
    }
    else{
      digitalWrite(5,LOW);
      digitalWrite(6,HIGH);
    }
    if (elapsedSec > 120 & readTemperature() < 40) {
      break;
    }
  }
  //DeActivating the heater
  digitalWrite(8, LOW);
 while (readTemperature()>40) {

    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(readTemperature());
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Cooling.");
    lcd.setCursor(0, 1);
    lcd.print("Cooling..");
    lcd.setCursor(0, 1);
    lcd.print("Cooling...");
    

    Serial.println(readTemperature());  // send to temperature to plotter
    delay(500);

    lcd.clear();

    //Changing the Led Base on Temperatuer
    if(readTemperature()>45){
      digitalWrite(5,HIGH);
      digitalWrite(6,LOW);
    }
    else{
      digitalWrite(5,LOW);
      digitalWrite(6,HIGH);
    }
  }

  //TURN Red & Green Off
  digitalWrite(5,LOW);
  digitalWrite(6,LOW);


  for (int i = 0; i < 3; i++) {
    digitalWrite(7, HIGH);
    delay(1000);
    digitalWrite(7, LOW);
    delay(1000);
  }
if (safety){
  for (int i = 30; i <= 90; i++) {
    myservo.write(i);  // tell servo to go to position
    delay(25);
  }
}  
  lcd.clear();

  
  }

