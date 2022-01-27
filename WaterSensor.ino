#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <dht.h>
#include <TimeLib.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

dht DHT;

#define DHT11_PIN 7
#define DHTTYPE DHT22
#define ECHO_TO_SERIAL 1 //Sends datalogging to serial if 1, nothing if 0
#define LOG_INTERVAL 360000 //milliseconds between entries (6 minutes = 360000)

const int soilMoisturePin = A0;
int waterLED = 13; //LED lights when needed to be watered
const float wateringThreshold = 15; //Value below which the garden gets watered
float soilMoistureRaw = 0; //Raw analog input of soil moisture sensor (volts)
float soilMoisture = 0; //Scaled value of volumetric water content in soil (percent)
bool wateredToday = false;
const int chipSelect = 10;

time_t nowTime; //Track when last watered
File logfile;

void error(char *str) {
  
  Serial.print("error: ");
  Serial.println(str);

}

void setup() {

  Serial.begin(9600); //for testing
  Serial.println("Card initialized.");

  lcd.begin(16, 2);

  pinMode(waterLED, OUTPUT);
  pinMode(chipSelect, OUTPUT); //Pin for writing to SD card
  

  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  
  nowTime = now();
}


void loop() {
  
   //delay software
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

  //reset whether watered today
  if (!(day(nowTime)==day(now()))) {
  wateredToday = false;
  }
  
  nowTime = now(); //track time

  // log time
  logfile.print(year(nowTime), DEC);
  logfile.print("/");
  logfile.print(month(nowTime), DEC);
  logfile.print("/");
  logfile.print(day(nowTime), DEC);
  logfile.print(" ");
  logfile.print(hour(nowTime), DEC);
  logfile.print(":");
  logfile.print(minute(nowTime), DEC);
  logfile.print(":");
  logfile.print(second(nowTime), DEC);
  logfile.print(",");
  #if ECHO_TO_SERIAL
  Serial.print(year(nowTime), DEC);
  Serial.print("/");
  Serial.print(month(nowTime), DEC);
  Serial.print("/");
  Serial.print(day(nowTime), DEC);
  Serial.print(" ");
  Serial.print(hour(nowTime), DEC);
  Serial.print(":");
  Serial.print(minute(nowTime), DEC);
  Serial.print(":");
  Serial.print(second(nowTime), DEC);
  Serial.print(",");
  #endif //ECHO_TO_SERIAL
    
  int sensorMoistureLvl = analogRead(soilMoisturePin);
  soilMoistureRaw = analogRead(soilMoisturePin)*(3.3/1024);
  delay(20);
  
  //Volumetric Water Content (piecewise function)
  if (soilMoistureRaw < 1.1) {
    soilMoisture = (10 * soilMoistureRaw) - 1;
  }
  else if (soilMoistureRaw < 1.3) {
    soilMoisture = (25 * soilMoistureRaw) - 17.5;
  }
  else if (soilMoistureRaw < 1.82) {
    soilMoisture = (48.08 * soilMoistureRaw) - 47.5;
  }
  else if (soilMoistureRaw < 2.2) {
    soilMoisture = (26.32 * soilMoistureRaw) - 7.89;
  }
  else {
    soilMoisture = (62.5 * soilMoistureRaw) - 87.5;
  }

  //Log variable
  logfile.print(soilMoisture);
 #if ECHO_TO_SERIAL
   Serial.print(soilMoisture);
 #endif


  if ((soilMoisture < wateringThreshold) && (wateredToday == false)) {
    //water the garden
    digitalWrite(waterLED, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(30000);                       // wait for a second
    digitalWrite(waterLED, LOW);    // turn the LED off by making the voltage LOW
    delay(600000);                       // wait for a second
    wateredToday = true;
  
    //record that we're watering
    logfile.print("TRUE");
  #if ECHO_TO_SERIAL
    Serial.print("TRUE");
  #endif
  
    wateredToday = true;
  }
 else {
    logfile.print("FALSE");
  #if ECHO_TO_SERIAL
    Serial.print("FALSE");
  #endif
  }

  
  logfile.println();
  #if ECHO_TO_SERIAL
  Serial.println();
  #endif
  delay(50);

  int chk = DHT.read11(DHT11_PIN);
  lcd.setCursor(0,0); 
  lcd.print("Temp: ");
  lcd.print(DHT.temperature);
  lcd.print((char)223);
  lcd.print("C");
  lcd.setCursor(0,1);
  lcd.print("Humidity: ");
  lcd.print(DHT.humidity);
  lcd.print("%");
  delay(1000);
  
  //Write to SD card
  logfile.flush();
  delay(5000);
} 
