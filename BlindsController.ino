#include <stddef.h>
#include <EEPROM.h>
#include <Stepper.h>

// change this to the number of steps on your motor
#define STEPS_PER_REV 2038 

struct eeprom_config {
  int version;
  long fullStepCount;
  long currentStepCount;
};

const int sensorMin = 370;
const int sensorMax = 1020;
const int PHOTOCELL_PIN = A2;
const int BUTTON1_PIN = 2;
const int BUTTON2_PIN = 3;
const int LED_PIN = LED_BUILTIN; //6;

const int TRANSITION_LEVEL = 10;

#if 0
#define Sprint(a) (Serial.print(a))
#define Sprintln(a) (Serial.println(a))
#else
#define Sprint(a)
#define Sprintln(a)
#endif


// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS_PER_REV, 8, 10, 9, 11);
bool blindOverride = false;
int trendCount = 0;
bool stepCountSet = false;
long targetStepCount = 0;
int targetPercent = 0;

eeprom_config config;


enum MODE {
  SETUP,
  NORMAL
};
MODE mode = NORMAL;


void setup() {  
  Serial.begin(9600);

  stepper.setSpeed(13);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // load config & state from eeprom
  EEPROM.get( offsetof(eeprom_config, version), config.version );
  EEPROM.get( offsetof(eeprom_config, fullStepCount), config.fullStepCount);
  EEPROM.get( offsetof(eeprom_config, currentStepCount), config.currentStepCount);
  if (config.version == 1) {
    targetStepCount = config.currentStepCount;
    targetPercent = targetStepCount / config.fullStepCount * 100.0;
  } else {
    targetStepCount = config.currentStepCount = config.fullStepCount = 0;
    mode = SETUP;
  }


  // check if we need to enter setup mode or go straight into normal mode
  int button1 = digitalRead(BUTTON1_PIN);
  int button2 = digitalRead(BUTTON2_PIN);
  if (button1 == LOW || button2 == LOW) {
    mode = SETUP;
  }
}

void motorOff() {
  Sprintln("Motor off");
  digitalWrite(8,LOW);
  digitalWrite(9,LOW);
  digitalWrite(10,LOW);
  digitalWrite(11,LOW);
}

void setBlinds(int percentClosed) {
  // 0 - fully open
  // 100 - fully closed
  // set targetStepCount based on 0 steps being open and fullStepCount being closed.  
  // Note that fullStepCount may be positive or negative.

  targetPercent = percentClosed;
  if (targetPercent < 0) targetPercent = 0;
  if (targetPercent > 100) targetPercent = 100;
  targetStepCount = (long)(config.fullStepCount * targetPercent / 100.0);
}

bool blindsFullyOpen() {
  return (targetStepCount == 0);
}

bool blindsFullyClosed() {
  return (targetStepCount == config.fullStepCount);
}

void openBlinds() {
  Sprintln("Opening Blinds...");
  setBlinds(0);
  trendCount = 0;
}

void closeBlinds() {
  Sprintln("Closing Blinds...");
  setBlinds(100);
  trendCount = 0;
}


// direct control of blinds with the two buttons.
// Move blinds to the open position and press both buttons to reset step count.
// Move blinds to the closed position and press both buttons to set step count.
// Moves into mode2 after second button..
void setup_loop() {
  // read the button states:
  int button1 = digitalRead(BUTTON1_PIN);
  int button2 = digitalRead(BUTTON2_PIN);

  Sprint("MODE 1:  B1: ");
  Sprint(button1); Sprint("   B2: ");
  Sprint(button2); Sprint("   STEP_CT: ");
  Sprintln(config.fullStepCount);

  if (stepCountSet)
    digitalWrite(LED_PIN, (millis() / 500) % 2);
  else
    digitalWrite(LED_PIN, (millis() / 100) % 2);

  // set the motor speed:
  if (button1 == LOW && button2 == HIGH) {
    config.fullStepCount += (STEPS_PER_REV / 100);
    stepper.step(STEPS_PER_REV / 100);
  } else if (button1 == HIGH && button2 == LOW) {
    config.fullStepCount += (STEPS_PER_REV / -100);
    stepper.step(STEPS_PER_REV / -100);
  } else {
    motorOff();

    if (button1 == LOW && button2 == LOW) {
      if (!stepCountSet) {
        Sprintln("Step count reset");
        config.fullStepCount = 0;
        stepCountSet = true;
        delay(500);
      } else {
        Sprintln("Calibration complete.");
        targetStepCount = config.currentStepCount = config.fullStepCount;
        targetPercent = 100;
        EEPROM.put( offsetof(eeprom_config, version), 1);
        EEPROM.put( offsetof(eeprom_config, fullStepCount), config.fullStepCount);
        EEPROM.put( offsetof(eeprom_config, currentStepCount), config.currentStepCount);  

        mode = NORMAL;
        digitalWrite(LED_PIN, 0);
        delay(500);
      }
    }
    
    delay(10);
  }
}


bool btn1Pressed = false;
bool btn2Pressed = false;
void handle_input() {
  int button1 = digitalRead(BUTTON1_PIN);
  if (button1 == LOW && !btn1Pressed) {
    btn1Pressed = true;
    blindOverride = true;
    digitalWrite(LED_PIN, blindOverride);
    setBlinds(targetPercent + 25);
    Sprint("target step: "); Sprint(targetStepCount); Sprint(" / "); Sprintln(config.fullStepCount);
  } else if (button1 == HIGH && btn1Pressed) {
    btn1Pressed = false;
  }
  
  int button2 = digitalRead(BUTTON2_PIN);
  if (button2 == LOW && !btn2Pressed) {
    btn2Pressed = true;
    blindOverride = true;
    digitalWrite(LED_PIN, blindOverride);
    setBlinds(targetPercent - 25);
    Sprint("target step: "); Sprint(targetStepCount); Sprint(" / "); Sprintln(config.fullStepCount);
  } else if (button2 == HIGH && btn2Pressed) {
    btn2Pressed = false;
  }

  if (blindOverride) {
    if (button1 == LOW && button2 == LOW) {
      blindOverride = false;
      digitalWrite(LED_PIN, blindOverride);
      Sprint("target step: "); Sprint(targetStepCount); Sprint(" / "); Sprintln(config.fullStepCount);
    }
  }
}

unsigned long last_sensor_time = 0;
void check_sensors() {
    if (blindOverride)
      return;

    if(millis() - last_sensor_time > 1000ul) {
        last_sensor_time = millis();

        int analogValue = analogRead(PHOTOCELL_PIN);
        int value = map(analogValue, sensorMin, sensorMax, 0, 100);

        Sprint("MODE 2:  ");
        if (blindsFullyOpen())
          Sprint("OPEN  ");
        else if (blindsFullyClosed())
          Sprint("CLOSED  ");
        else
          Sprint("CUSTOM");
        Sprint("  Light Level: "); Sprint(analogValue);  Sprint("  Adjusted: ");
        Sprint(value);  Sprint("   Trend: ");
        Sprintln(trendCount);
      
        if (blindsFullyOpen()) {
          if (value < TRANSITION_LEVEL)
            trendCount ++;
          else
            trendCount = 0;
      
          if (trendCount >= 5) {
            closeBlinds();
          }
        } else if (blindsFullyClosed()) {
          if (value > TRANSITION_LEVEL)
            trendCount ++;
          else
            trendCount = 0;
      
          if (trendCount >= 5) {
            openBlinds();
          }
        } else {
          if (value < TRANSITION_LEVEL)
            closeBlinds();
          else
            openBlinds();
        }

    }
}

void do_steps() {
  int steps = 0;
  if (config.currentStepCount < targetStepCount) {
    steps = STEPS_PER_REV / 100;
    if (config.currentStepCount + steps > targetStepCount)
      steps = targetStepCount - config.currentStepCount;
  } else if (config.currentStepCount > targetStepCount) {
    steps = STEPS_PER_REV / -100;
    if (config.currentStepCount + steps < targetStepCount)
      steps = targetStepCount - config.currentStepCount;
  }

  if (steps != 0) {
    //Sprint("STEPPING ");
    //Sprintln(steps);
    stepper.step(steps);
    config.currentStepCount += steps;

    if (config.currentStepCount == targetStepCount) {
        Sprintln("At target position.  Saving...");
        motorOff();
        EEPROM.put( offsetof(eeprom_config, currentStepCount), config.currentStepCount);
    }
  }
}


void normal_loop() {
  // check manual input
  // if we are past the lockout delay, check for input.
  handle_input();

  // check sensor input
  // if it has been 1 second since the last check, poll new values.   1 second delay is important for trending.
  check_sensors();

  // perform steps
  // if we are not at the target yet, take an incremental step.
  do_steps();
}

void loop() {
  switch (mode) {
    case SETUP:
      setup_loop();
      break;
    case NORMAL:
      normal_loop();
      break;
  }
}
