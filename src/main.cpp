#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <Servo.h>

using namespace std;

// PWM output pins of ThinkerKit shield
#define SHIELD_O0 11 // FAN
#define SHIELD_O1 10 // LFO2
#define SHIELD_O2 9  // LFO1
#define SHIELD_O3 6  // SERVO 1
#define SHIELD_O4 5  // SERVO 2
#define SHIELD_O5 3  // SERVO 3



// ----------------------------SERIAL------------------------------------------
enum CommandWords {
  Component,
  ID,
  Instruction,
  Speed,
  Waveform,
  Rythm
};

enum class Components {T, S, F};

enum class Waveforms {S, C, T, I, N};
// ----------------------------------------------------------------------------



// ----------------------------MOZZI-------------------------------------------
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle_warm8192_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/brownnoise8192_int8.h>


// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSine(SIN2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> aSquare(SQUARE_NO_ALIAS_2048_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> aTriangle(TRIANGLE_WARM8192_DATA);
Oscil <SAW2048_NUM_CELLS, AUDIO_RATE> aSaw(SAW2048_DATA);
Oscil <BROWNNOISE8192_NUM_CELLS, AUDIO_RATE> aNoise(BROWNNOISE8192_DATA);

String waveformCode = "S";
// ----------------------------------------------------------------------------



// ------------------------------COMPONENTS------------------------------------
#define FAN_MIN_SPEED 0
#define FAN_MAX_SPEED 100
#define FAN_PIN SHIELD_O0


#define servo3_PIN SHIELD_O3
#define servo4_PIN SHIELD_O4
#define servo5_PIN SHIELD_O5
#define SERVO_MIN_POS 65
#define SERVO_MAX_POS 180
#define SERVO_MIN_INTERVAL 10
#define SERVO_MAX_INTERVAL 200
Servo servo3;
Servo servo4;
Servo servo5;
bool servo3State = false;
bool servo4State = false;
bool servo5State = false;
int servo3Interval = 0;
int servo4Interval = 0;
int servo5Interval = 0;
int servo3Pos = SERVO_MIN_POS;
int servo4Pos = SERVO_MIN_POS;
int servo5Pos = SERVO_MIN_POS;
bool servo3Clockwise = true;
bool servo4Clockwise = true;
bool servo5Clockwise = true;
unsigned long servo3LastUpdate = 0;
unsigned long servo4LastUpdate = 0;
unsigned long servo5LastUpdate = 0;
// ----------------------------------------------------------------------------


void setup() {
  pinMode(FAN_PIN, OUTPUT);
  servo3.attach(servo3_PIN);
  servo4.attach(servo4_PIN);
  servo5.attach(servo5_PIN);
  Serial.begin(115200);
  // while(!Serial) {};
  Serial.println("Serial connection initialized");
}


String parseCommand(String command, CommandWords commandWord) {
  int firstCharIndex = 0;
  int lastCharIndex = 0;

  switch (commandWord) {
    case Component:
      lastCharIndex = 1;
      break;
    case ID:
      firstCharIndex = 2;
      lastCharIndex = 3;
      break;
    case Instruction:
      firstCharIndex = 4;
      lastCharIndex = 5;
      break;
    case Speed:
      firstCharIndex = 6;
      lastCharIndex = 11;
      break;
    case Waveform:
      firstCharIndex = 12;
      lastCharIndex = 13;
      break;
    case Rythm:
      firstCharIndex = 14;
      lastCharIndex = 19;
      break;
  }

  return command.substring(firstCharIndex, lastCharIndex);
}


void operateTransistor(String command) {
  Serial.println("Transistor operation");
  String instructionCode = parseCommand(command, Instruction);
  // Serial.println("Instruction code : " + instructionCode);

  if (instructionCode == "0") {
    Serial.println("Stoping wave");
    stopMozzi();
  } else if (instructionCode == "1") {
    Serial.println("Starting wave");

    waveformCode = parseCommand(command, Waveform);
    Serial.println("Waveform : " + waveformCode);
    
    String freqCode = parseCommand(command, Speed);
    // Serial.println("Freq code : -" + freqCode + "-");
    int freq = freqCode.toInt();
    Serial.println("Frequency : " + String(freq));

    startMozzi();
    aSine.setFreq(freq);
    aSquare.setFreq(freq);
    aTriangle.setFreq(freq);
    aSaw.setFreq(freq);
    aNoise.setFreq(freq);
  } else {
    Serial.println("Wrong instruction code");
  }

}


void operateServo(String command) {
  Serial.println("Servo operation");

  String idCode = parseCommand(command, ID);
  int id = idCode.toInt();
  Serial.println("Servo ID : " + idCode);

  bool servoState;
  String instructionCode = parseCommand(command, Instruction);
  if (instructionCode == "1") {
    servoState = true;
  } else {
    servoState = false;
  }

  String speedCode = parseCommand(command, Speed);
  int speed = speedCode.toInt();
  int interval = map(speed, 1, 100, SERVO_MAX_INTERVAL, SERVO_MIN_INTERVAL);

  switch (id) {
    case 3:
      servo3State = servoState;
      servo3Interval = interval;
      break;
    case 4:
      servo4State = servoState;
      servo4Interval = interval;
      break;
    case 5:
      servo5State = servoState;
      servo5Interval = interval;
      break;
    default:
      Serial.println("Component #" + idCode + " is not a servo");
      break;
  }

  Serial.println("State : " + String(servoState));
  Serial.println("Speed : " + String(speed));
}


void operateFan(String command) {
  Serial.println("Fan operation");
  String speedCode = parseCommand(command, Speed);
  int speed = speedCode.toInt();
  Serial.println("Speed : " + String(speed));
  speed = map(speed, FAN_MIN_SPEED, FAN_MAX_SPEED, 0, 255);
  analogWrite(FAN_PIN, speed);
}


void parseComponent(String command) {
  String componentCode = parseCommand(command, Component);

  if (componentCode == "T") {
    operateTransistor(command);
  }
  else if (componentCode == "S") {
    operateServo(command);
  }
  else if (componentCode == "F") {
    operateFan(command);
  }
  else {
    Serial.println("Wrong component code");
  }
}


void updateControl() {
}


AudioOutput_t updateAudio(){
  if (waveformCode == "S") {
      return MonoOutput::from8Bit(aSine.next()); // 8 bit * 8 bit gives 16 bits value
    } else if (waveformCode == "C") {
      return MonoOutput::from16Bit(aSquare.next());
    } else if (waveformCode == "T") {
      return MonoOutput::from16Bit(aTriangle.next());
    } else if (waveformCode == "I") {
      return MonoOutput::from16Bit(aSaw.next());
    } else if (waveformCode == "N") {
      return MonoOutput::from16Bit(aNoise.next());
    } else {
      Serial.println("Wrong waveform code");
      return;
    }
}


void updateServos() {
  unsigned long currentMillis = millis();

  if (servo3State && (currentMillis - servo3LastUpdate >= servo3Interval)) {
    servo3LastUpdate = currentMillis;

    if (servo3Pos < SERVO_MIN_POS) {
      servo3Clockwise = true;
    } else if (servo3Pos > SERVO_MAX_POS) {
      servo3Clockwise = false;
    }

    if (servo3Clockwise) {
      servo3Pos++;
    } else {
      servo3Pos--;
    }

    servo3.write(servo3Pos);
  }

  if (servo4State && (currentMillis - servo4LastUpdate >= servo4Interval)) {
    servo4LastUpdate = currentMillis;

    if (servo4Pos < SERVO_MIN_POS) {
      servo4Clockwise = true;
    } else if (servo4Pos > SERVO_MAX_POS) {
      servo4Clockwise = false;
    }

    if (servo4Clockwise) {
      servo4Pos++;
    } else {
      servo4Pos--;
    }

    servo4.write(servo4Pos);
  }

  if (servo5State && (currentMillis - servo5LastUpdate >= servo5Interval)) {
    servo5LastUpdate = currentMillis;

    if (servo5Pos < SERVO_MIN_POS) {
      servo5Clockwise = true;
    } else if (servo5Pos > SERVO_MAX_POS) {
      servo5Clockwise = false;
    }

    if (servo5Clockwise) {
      servo5Pos++;
    } else {
      servo5Pos--;
    }

    servo5.write(servo5Pos);
  }
}


void loop()
{
  updateServos();
  audioHook();

  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    String msg = parseCommand(command, Component);
    parseComponent(command);
  }

}
