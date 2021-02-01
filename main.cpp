// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include <RTClib.h>
#include <max6675.h>
#include <SerialCommand.h>

// RTC 
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};  

#define start_hour 18
#define start_minit 0

#define end_hour 6
#define end_minit 0 

#define LIGHT_PWM_PIN 12
#define LIGHT_PWM_PIN2 11 
#define LIGHT_PWM_PIN3 10
#define LIGHT_PWM_PIN4 9

#define FAN_PWM_PIN 8

// Light cooler thermocouple
#define thermoSO 49
#define thermoCS 51
#define thermoSCK 53

#define pump 22

const int AirValue = 615;   //you need to replace this value with Value_1
const int WaterValue = 570;  //you need to replace this value with Value_2
int soilMoistureValue = 0;
int soilmoisturepercent=0;

bool light = true;
bool fan = true;

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

SerialCommand sCmd;     // The demo SerialCommand object

#define arduinoLED 13   // Arduino LED on board

#define LIGHT_INTENSITY 255
#define LIGHT_INTENSITY2 255
#define LIGHT_INTENSITY3 255
#define LIGHT_INTENSITY4 255

#define FAN_SPEED 150


bool isRTCrunning = false; 

uint32_t frequency = 5; // in seconds 

uint32_t lastTimestamp = 0; 

void analogWrite25k(int value){
    OCR4C = value;
}

void setTimer4To25kHz(){ 
    TCCR4A = 0;
    TCCR4B = 0;
    TCNT4  = 0;

    // Mode 10: phase correct PWM with ICR4 as Top (= F_CPU/2/25000)
    // OC4C as Non-Inverted PWM output
    ICR4   = (F_CPU/25000)/2;
    OCR4C  = ICR4/2;                    // default: about 50:50
    TCCR4A = _BV(COM4C1) | _BV(WGM41);  
    TCCR4B = _BV(WGM43) | _BV(CS40);  
}


void setRTC(){ 
  if (! rtc.begin()) {
      Serial.println("Couldn't find RTC");
    }else{
      Serial.println("RTC found");
      printTime(); 
    }

    if (!rtc.isrunning()) {
      Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }else { 
      isRTCrunning = true;
      printTime(); 
    }
}

void setLight(){
      // Set the PWM pin as output.
    pinMode(FAN_PWM_PIN, OUTPUT); // Fanspeed 

    pinMode(LIGHT_PWM_PIN, OUTPUT); // Light Intensity 
    pinMode(LIGHT_PWM_PIN2, OUTPUT); // Light Intensity 
    pinMode(LIGHT_PWM_PIN3, OUTPUT); // Light Intensity 
    pinMode(LIGHT_PWM_PIN4, OUTPUT); // Light Intensity 

    analogWrite25k(FAN_SPEED);
    analogWrite(LIGHT_PWM_PIN, LIGHT_INTENSITY);  
    analogWrite(LIGHT_PWM_PIN2, LIGHT_INTENSITY2);  
    analogWrite(LIGHT_PWM_PIN3, LIGHT_INTENSITY3);  
    analogWrite(LIGHT_PWM_PIN4, LIGHT_INTENSITY4);  
}

void setLightOff(){
    analogWrite(LIGHT_PWM_PIN, 0);  
    analogWrite(LIGHT_PWM_PIN2, 0);  
    analogWrite(LIGHT_PWM_PIN3, 0);  
    analogWrite(LIGHT_PWM_PIN4, 0);  
}

void setLightOn(){
    analogWrite(LIGHT_PWM_PIN, LIGHT_INTENSITY);  
    analogWrite(LIGHT_PWM_PIN2, LIGHT_INTENSITY2);  
    analogWrite(LIGHT_PWM_PIN3, LIGHT_INTENSITY3);  
    analogWrite(LIGHT_PWM_PIN4, LIGHT_INTENSITY4); 
}

void setup(){    
    Serial.begin(9600);
    setTimer4To25kHz(); 
    
    pinMode(pump, OUTPUT);
    digitalWrite(pump, HIGH);    // default to off

    pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
    digitalWrite(arduinoLED, LOW);    // default to LED off

    setLight(); 
    
    // Setup callbacks for SerialCommand commands
    sCmd.addCommand("ON",    LED_on);          // Turns LED on
    sCmd.addCommand("OFF",   LED_off);         // Turns LED off
    sCmd.addCommand("time", cmd_get_time);
    sCmd.addCommand("temp", cmd_temp);
    sCmd.addCommand("hum", cmd_get_hum);
    sCmd.addCommand("pump_on", cmd_pump_on);
    sCmd.addCommand("pump_off", cmd_pump_off);
    sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")

    setRTC(); 
    if(isRTCrunning) {
      lastTimestamp = rtc.now().unixtime(); 
    }
    
    Serial.println("Ready"); 
}



void loop(){ 
    sCmd.readSerial();     // We don't do much, just process serial commands

    if(isRTCrunning) {
      /*
      if(lastTimestamp != 0 && ((rtc.now().unixtime() - lastTimestamp) > frequency) ){ 
        Serial.println(lastTimestamp); 
        Serial.println(rtc.now().unixtime()); 
        soilMoistureValue = analogRead(A0);  //put Sensor insert into soil
        Serial.println(soilMoistureValue);
        soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
        Serial.print(soilmoisturepercent);
        Serial.println("%");
        lastTimestamp = rtc.now().unixtime();
      }*/

      /*
            if( (hour > end_hour && hour < start_hour && start_hour > end_hour && light) || 
          (hour > end_hour && start_hour < end_hour && light) )  
        */
      DateTime now = rtc.now();
    
      if((now.hour() < start_hour && light) || 
         (now.hour() >= end_hour && light && end_hour > start_hour) ||
         (now.hour() >= end_hour && light && now.hour() < start_hour )) 
     {
        Serial.println("OFF");
        light = false; 
        fan = false; 
        setLightOff(); 
        analogWrite(FAN_PWM_PIN, 0);
         
      }else if(((!light && now.hour() >= start_hour && end_hour > start_hour) && 
               ( !light && now.hour() < end_hour && end_hour > start_hour) ) ||
               (((!light && end_hour < start_hour) && !(now.hour() > end_hour && now.hour() < start_hour))))
      {
        Serial.println("ON");
        light = true; 
        fan = true; 
        setLightOn();
        analogWrite(FAN_PWM_PIN, FAN_SPEED); 
      }
    }
}

void cmd_get_time(){
  printTime(); 
}

void cmd_get_hum(){
                soilMoistureValue = analogRead(A0);  //put Sensor insert into soil
        Serial.println(soilMoistureValue);
        soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
        Serial.print(soilmoisturepercent);
        Serial.println("%");
}


void cmd_temp(){ 
   Serial.print("C = "); 
   Serial.println(thermocouple.readCelsius());
}

void cmd_pump_off(){ 
  digitalWrite(pump, LOW);    
}

void cmd_pump_on(){ 
  digitalWrite(pump, HIGH);    
}

void LED_on() {
  Serial.println("LED on");
  digitalWrite(arduinoLED, HIGH);
}

void LED_off() {
  Serial.println("LED off");
  digitalWrite(arduinoLED, LOW);
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("What?");
}

void printTime(){
   //Serial.print("Starting printTime"); 
   DateTime now = rtc.now();
   Serial.print(now.year(), DEC);
   Serial.print('/');
   Serial.print(now.month(), DEC);
   Serial.print('/');
   Serial.print(now.day(), DEC);
   Serial.print(" (");
   Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
   Serial.print(") ");
   Serial.print(now.hour(), DEC);
   Serial.print(':');
   Serial.print(now.minute(), DEC);
   Serial.print(':');
   Serial.print(now.second(), DEC);
   Serial.println();
   /*Serial.print(" since midnight 1/1/1970 = ");
   Serial.print(now.unixtime());
   Serial.print("s = ");
   Serial.print(now.unixtime() / 86400L);
   Serial.println("d");*/ 
   
}
