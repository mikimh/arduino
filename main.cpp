// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include <RTClib.h>
#include <max6675.h>
#include <SerialCommand.h>

// RTC 
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};  

int start_hour = 8; 
int start_minit = 0; 

int end_hour = 22; 
int end_minit = 0; 

int LIGHT_PWM_PIN = 9; 
int FAN_PWM_PIN = 8;  

// Light cooler thermocouple
int thermoSO = 49;
int thermoCS = 51;
int thermoSCK = 53;

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

SerialCommand sCmd;     // The demo SerialCommand object

#define arduinoLED 13   // Arduino LED on board

bool isRTCrunning = false; 

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
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    if (! rtc.isrunning()) {
      Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }else { 
      printTime(); 
    }
}

void setup(){    
    Serial.begin(9600);
    
    pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
    digitalWrite(arduinoLED, LOW);    // default to LED off

    // Setup callbacks for SerialCommand commands
    sCmd.addCommand("ON",    LED_on);          // Turns LED on
    sCmd.addCommand("OFF",   LED_off);         // Turns LED off
    sCmd.addCommand("time", cmd_get_time);
    sCmd.addCommand("temp", cmd_temp);
    sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")

    setTimer4To25kHz(); 

    //setRTC(); 
    
    // Set the PWM pin as output.
    pinMode(FAN_PWM_PIN, OUTPUT); // Fanspeed 
    pinMode(LIGHT_PWM_PIN, OUTPUT); // Light Intensity 

    analogWrite25k(110);
    analogWrite(LIGHT_PWM_PIN, 90);  
    Serial.println("Ready"); 
}

void loop(){ 
    sCmd.readSerial();     // We don't do much, just process serial commands

    /*int w = Serial.parseInt();*/ 
    /*if (w>0) {
        analogWrite25k(w);
        Serial.println(w);
        printTime();
    } */

  

    if(isRTCrunning) {
      DateTime now = rtc.now();
    
      if(now.hour() < start_hour || now.hour() > end_hour) { 
        analogWrite(LIGHT_PWM_PIN, 0); 
      }
    }
}
void cmd_get_time(){
  printTime(); 
}

void cmd_temp(){ 
   Serial.print("C = "); 
   Serial.println(thermocouple.readCelsius());
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
  Serial.print('Starting printTime'); 
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
