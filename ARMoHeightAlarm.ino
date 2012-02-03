#include <LiquidCrystal.h>

#define heightParam 39
#define gpsNorthParam 22
#define frameBlink 30
#define alarmCheckInterval 3000

int altAlarm = 70; // set this to 100m :-)

int infoPin = 10;
int info2Pin = 11;
int errPin = 12;
int buzzPin = 13;

boolean infoOn = false;
boolean info2On = false;
boolean errOn = false;
String aobV = "0.1";
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

byte f[7]; // buffer to hold a complete debug frame
int fc; // framecounter - reference to where the frame has been filled up
byte b=0; // last read byte from serial
int lastDebugParm=0;
long framesParsed = 0; // Number of total frames parsed
long debugValues[256]; // Contains ALL the parsed debug values
unsigned long running = 0; // The 'clock' in msec elapsed
unsigned long lastHeightFrame = 0;
unsigned long lastNoHeightFramesAlarm = 0;
unsigned long lastLCDUpdate = 0;
int heightFrameAlarm = 2000; // If no heightframe has been parsed for this amount of msec, sound the alarm
boolean noHeightFrames = false;
String line0 = "";
String line1 = "";

void setup() {
  pinMode(infoPin, OUTPUT);
  pinMode(info2Pin, OUTPUT);
  pinMode(errPin, OUTPUT);
  pinMode(buzzPin, OUTPUT);

  Serial.begin(38400); // 38400,8,N,1 default Arduino  
  setInitLEDS();

  lcd.begin(16, 2);
  lcd.clear(); 


  String s = "Arm-o-Beep";
  s.concat(" ");
  s.concat(aobV);
  Serial.println(s);
  print(0, s);

  // Initialize the debugvalues to 0xFF
  for(int i = 0; i < 256; i++) {
    debugValues[i] = 0xFF;
  }
  lastHeightFrame = millis();
}

void loop() {
  if(Serial.available()) {
    b = Serial.read();
    if(b == 0x1c) {
      fc=0;
      f[fc++] = b;
      readFrame();
      if(parseFrame()) {
        ledFeedback();
        checkHeight();
        checkGPSNorth();
      }
    }
  }
  running = millis();
  checkNoHeightFramesAlarms();
  updateLCD();
}

void checkNoHeightFramesAlarms() {
  if((running - lastHeightFrame) > heightFrameAlarm) {
    noHeightFrames = true;
  } 
  else {
    noHeightFrames = false;
  }

  if(noHeightFrames) {
    setAlarmLEDS();
    line1="No height frames";
    if((running - lastNoHeightFramesAlarm) > alarmCheckInterval) {
      tone(buzzPin, 4000, 100);
      lastNoHeightFramesAlarm = millis();
    }
  }
}

void updateLCD() {
  // Update LCD every sec
  if((running - lastLCDUpdate) > 500) {

    if(line0.length() == 0) {
      String s = "Frames: ";
      s.concat(framesParsed);
      line0=s;
    }

    print(0, line0);
    print(1, line1);
    lastLCDUpdate=millis();
    line0="";
    line1="";
  }
}

void ledFeedback() {
  if((framesParsed %  frameBlink) == 0) {
    digitalWrite(infoPin, (infoOn ? LOW : HIGH));
    infoOn = !infoOn;
  }
}

void readFrame() {
  while(fc < 7) {
    if(Serial.available() > 0) {
      b = Serial.read();
      f[fc++] = b;
    }
  }
}

boolean parseFrame() {
  long crc = f[1] ^ f[2] ^ f[3]^ f[4]^ f[5];
  if(crc==f[6]) {    
    if((f[1] >= 0) && (f[1] < 256)) {
      long val = 0;
      val = (val <<8)+ ((long)f[5]&0xff);
      val = (val <<8)+ ((long)f[4]&0xff);
      val = (val <<8)+ ((long)f[3]&0xff);
      val = (val <<8)+ ((long)f[2]&0xff);
      debugValues[f[1]] = val;
      framesParsed++;
      lastDebugParm = f[1];
      return true;
    } 
  }
  else {
    Serial.print("INV: ");
    printFrame();
  }
  return false;
}


void checkHeight() {
  if(lastDebugParm==heightParam) {
    lastHeightFrame = millis();
    if(debugValues[heightParam] != 0xFF) {
      int height =debugValues[heightParam] *20 / 32000;

      String s = "Alt: ";
      s.concat(height);
      s.concat("m");
      line1=s;

      digitalWrite(info2Pin, (info2On ? LOW : HIGH));
      info2On = !info2On;

      if(height >= altAlarm) {
        String s = "Alarm: >";
        s.concat(altAlarm);
        s.concat("m");
        line0=s;
        digitalWrite(errPin, HIGH);
        tone(buzzPin, 800, 100);        
        delay(50);
        tone(buzzPin, 6000, 100);
        delay(100);
        tone(buzzPin, 3000, 100);        
      } 
      else {
        digitalWrite(errPin, LOW);
      }

      Serial.print("**** Height: ");
      Serial.print(height);
      Serial.print (" (");
      Serial.print(debugValues[heightParam]);
      Serial.print (")\n");
    }
  }
}

void checkGPSNorth() {
  if(lastDebugParm==gpsNorthParam) {
    if(debugValues[gpsNorthParam] != 0xFF) {
      Serial.print("**** GPS North: ");
      Serial.println (debugValues[gpsNorthParam]);
    }
  }
}

void printFrame() {
  for(int i = 0; i < 7; i++) {
    Serial.print(f[i], BIN);
    Serial.print (" ");
  }
  Serial.println("");
}

void setInitLEDS() {
  digitalWrite(infoPin, HIGH);
  infoOn=true;
  digitalWrite(info2Pin, LOW);
  infoOn=false;
  digitalWrite(errPin, LOW);
  errOn= false;
}

void setAlarmLEDS() {
  digitalWrite(infoPin, LOW);
  infoOn=false;
  digitalWrite(info2Pin, LOW);
  infoOn=false;
  digitalWrite(errPin, HIGH);
  errOn= true;
}

String fill16(String s) {
  int sl = s.length();
  for(int i = 0; i < (16 - sl); i++) {
    s.concat(' ');
  }
  return s;
}

void print(int l, String s) {
  lcd.setCursor(0,l);
  lcd.print(fill16(s));
}
















