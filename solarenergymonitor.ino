

#define BLYNK_TEMPLATE_ID   "ur template id"
#define BLYNK_TEMPLATE_NAME "Solar Energy Management"
#define BLYNK_AUTH_TOKEN    "Your auth token"

char ssid[] = "your wifi ssid";
char pass[] = "your wifi pass";

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensorPin = 34;
const float mVperAmp = 185.0;
float panelVoltage = 4.20;    //  Change if needed
float zeroCurrent = 0.0;

// for daily energy variable.......
static unsigned long lastEnergyTime = 0;
static float dailyEnergyWh = 0.0;
float previousPower = 0.0;
unsigned long todayMidnight = 0;

BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Solar IoT");
  lcd.setCursor(0,1); lcd.print("Connecting...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  
  delay(10000);
  zeroCurrent = measureRawCurrent();
  Serial.print("Zero offset: "); Serial.println(zeroCurrent, 4);

  lastEnergyTime = millis();
  todayMidnight = (millis() / 86400000UL) * 86400000UL;  // 

  lcd.clear();
  lcd.print("Connected!");

  timer.setInterval(2000L, sendToBlynk); 
}

void loop() {
  Blynk.run();
  timer.run();
  updateLCD();
  calculateDailyEnergy();
}

void sendToBlynk() {
  float raw = measureRawCurrent();
  float current = raw - zeroCurrent;
  if (current < 0.008) current = 0.000;

  float power = current * panelVoltage;

//Guyzzz u need to define the variables for the datastream in the blynk dashboard from the app or website and use them accordingly

  Blynk.virtualWrite(V0, panelVoltage);   
  Blynk.virtualWrite(V1, current);        
  Blynk.virtualWrite(V2, power);           
  Blynk.virtualWrite(V3, dailyEnergyWh);  
}

void updateLCD() {
  float current = measureRawCurrent() - zeroCurrent;
  if (current < 0.008) current = 0.000;
  float power = current * panelVoltage;

  lcd.setCursor(0,0);
  lcd.print("I: "); lcd.print(current, 3); lcd.print("A  ");

  lcd.setCursor(0,1);
  lcd.print("E: "); lcd.print(dailyEnergyWh, 2); lcd.print("Wh ");
}

void calculateDailyEnergy() {
  unsigned long now = millis();
  float hoursPassed = (now - lastEnergyTime) / 3600000.0;
  dailyEnergyWh += previousPower * hoursPassed;
  lastEnergyTime = now;
  previousPower = (measureRawCurrent() - zeroCurrent);
  if (previousPower < 0) previousPower = 0;
  previousPower *= panelVoltage;

 
  unsigned long currentMidnight = (now / 86400000UL) * 86400000UL;
  if (currentMidnight != todayMidnight) {
    dailyEnergyWh = 0.0;
    todayMidnight = currentMidnight;
    Serial.println("Daily energy reset at midnight!");
  }
}

//measure the raw current
float measureRawCurrent() {
  float sum = 0.0;
  for (int i = 0; i < 1000; i++) {
    int adc = analogRead(sensorPin);
    float vMeasured = adc * 3.3 / 4096.0;
    float vSensor = vMeasured * 1.5;  
    sum += (vSensor - 2.5) * 1000.0 / mVperAmp;
    delayMicroseconds(400);
  }
  return sum / 1000.0;
}