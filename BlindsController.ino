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

#define TRANSITION_LEVEL 10


#define Sprint(a) //(Serial.print(a))
#define Sprintln(a) //(Serial.println(a))

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS_PER_REV, 8, 10, 9, 11);
bool blindOverride = false;
int trendCount = 0;
bool stepCountSet = false;
long targetStepCount = 0;

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

bool blindsOpen() {
  return (targetStepCount == 0);
}

void openBlinds() {
  Sprintln("Opening Blinds...");
  targetStepCount = 0;
  trendCount = 0;
}

void closeBlinds() {
  Sprintln("Closing Blinds...");
  targetStepCount = config.fullStepCount;
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


unsigned long input_lockout_time = 0;
void handle_input() {
  if (millis() - input_lockout_time > 2000ul) {
    int button1 = digitalRead(BUTTON1_PIN);
    int button2 = digitalRead(BUTTON2_PIN);

    if (button1 == LOW || button2 == LOW) {
      blindOverride = !blindOverride;
      input_lockout_time = millis();
      digitalWrite(LED_PIN, blindOverride);
      
      if (blindsOpen())
        closeBlinds();
      else
        openBlinds();
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
        if (blindsOpen())
          Sprint("OPEN  ");
        else
          Sprint("CLOSED  ");
        Sprint("  Light Level: "); Sprint(analogValue);  Sprint("  Adjusted: ");
        Sprint(value);  Sprint("   Trend: ");
        Sprintln(trendCount);
      
        if (blindsOpen()) {
          if (value < TRANSITION_LEVEL)
            trendCount ++;
          else
            trendCount = 0;
      
          if (trendCount >= 5) {
            closeBlinds();
          }
        } else {
          if (value > TRANSITION_LEVEL)
            trendCount ++;
          else
            trendCount = 0;
      
          if (trendCount >= 5) {
            openBlinds();
          }
        }

    }
}

bool do_steps() {
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
    Sprint("STEPPING ");
    Sprintln(steps);
    stepper.step(steps);
    config.currentStepCount += steps;

    if (config.currentStepCount == targetStepCount) {
        Sprintln("At target position.  Saving...");
        motorOff();
        EEPROM.put( offsetof(eeprom_config, currentStepCount), config.currentStepCount);
    }
    return true;
  }
  return false;
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
