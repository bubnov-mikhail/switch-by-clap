#include <Arduino.h>

// #define DEBUG true

#define MIC_PIN 3
#define BUTTON_PIN 2
#define SWITCHER_PIN 4
#define DEBOUNCE_MCS 50

enum SwitchStates
{
  OFF,
  ON
};

enum SwitchCtrlStates
{
  ALWAYS_ON,
  INTERACTIVE
};

SwitchStates switchState = OFF;
SwitchCtrlStates switchCtrlState = INTERACTIVE;

int clapGap[] = {250, 700, 250};
int clapGapJitter = 150;
int lastClapPatternMaxDelay = 1500;
int lastClapDelay = 1000;
int clapDebounce = 50;
bool hearingClap = false;
int clapsPointer = 0;
int clapsToSwitch = 4;
unsigned long lastClapHeared = 0;

void updateCtrlState();
bool hearingNewClap();

void setup()
{
  pinMode(MIC_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCHER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(SWITCHER_PIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  lastClapHeared = millis();
  #if defined(DEBUG)
  Serial.begin(9600);
  #endif
}

void loop()
{
  updateCtrlState();
  if (switchCtrlState == ALWAYS_ON)
  {
    return;
  }

  if (hearingNewClap())
  {
    if (millis() - lastClapHeared > lastClapPatternMaxDelay) {
      clapsPointer = 0;
    }

    if (clapsPointer == 0)
    {
      lastClapHeared = millis();
      clapsPointer++;
      #if defined(DEBUG)
      Serial.println("start new pattern.");
      #endif
      return;
    }

    unsigned long newClapHeared = millis();
    long betweenClaps = newClapHeared - lastClapHeared;
    lastClapHeared = newClapHeared;
    int expectedClapGap = clapGap[clapsPointer - 1];

    if (betweenClaps < clapDebounce)
    {
      return;
    }

    if (betweenClaps > expectedClapGap - clapGapJitter && betweenClaps < expectedClapGap + clapGapJitter)
    {
      // this is the correct next clap in the pattern
      #if defined(DEBUG)
      Serial.println("this is the correct next clap in the pattern.");
      #endif
      clapsPointer++;
      if (clapsPointer == clapsToSwitch)
      {
        #if defined(DEBUG)
        Serial.println("last clap. Checking if there will be no additional claps.");
        #endif
        // checking if there will be no additional claps after the correct pattern
        while (millis() - lastClapHeared < lastClapDelay)
        {
          if (hearingNewClap() && millis() - lastClapHeared > clapDebounce)
          {
            #if defined(DEBUG)
            Serial.println("pattern has been broken with a new clap.");
            Serial.print("diff: ");
            Serial.println(millis() - lastClapHeared);
            #endif
            // pattern has been broken with a new clap - let it be the new sequence
            clapsPointer = 1;
            return;
          }
        }

        // pattern has been finished with no additional clap after it.
        // lets switch the state and clear the clap pointer.
        clapsPointer = 0;
        if (switchState == OFF)
        {
          #if defined(DEBUG)
          Serial.println("switch on.");
          #endif
          digitalWrite(SWITCHER_PIN, LOW);
          digitalWrite(LED_BUILTIN, HIGH);
          switchState = ON;
        }
        else
        {
          #if defined(DEBUG)
          Serial.println("switch off.");
          #endif
          digitalWrite(SWITCHER_PIN, HIGH);
          digitalWrite(LED_BUILTIN, LOW);
          switchState = OFF;
        }
      }
    }
    else
    {
      #if defined(DEBUG)
      Serial.println("pattern has been broken - too late or too early.");
      Serial.print("diff: ");
      Serial.println(betweenClaps);
      #endif
      // pattern has been broken with a new clap - let it be the new sequence
      clapsPointer = 1;
    }
  }
}

// updateCtrlState checks the mode button
void updateCtrlState()
{
  bool buttonState;
  buttonState = digitalRead(BUTTON_PIN);
  if ((buttonState && switchCtrlState == ALWAYS_ON) || !buttonState && switchCtrlState == INTERACTIVE)
  {
    #if defined(DEBUG)
    Serial.println("button clicked");
    #endif
    delay(DEBOUNCE_MCS);
    buttonState = digitalRead(BUTTON_PIN);

    if (buttonState)
    {
      switchCtrlState = INTERACTIVE;
      if (switchState == ON)
      {
        #if defined(DEBUG)
        Serial.println("going to interactive mode.");
        #endif
        switchState = OFF;
        digitalWrite(SWITCHER_PIN, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    else
    {
      switchCtrlState = ALWAYS_ON;
      if (switchState == OFF)
      {
        #if defined(DEBUG)
        Serial.println("going to always on mode.");
        #endif
        switchState = ON;
        digitalWrite(SWITCHER_PIN, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
  }
}

// hearingNewClap checks if there is a new clap
bool hearingNewClap()
{
  if (digitalRead(MIC_PIN))
  {
    if (hearingClap)
    {
      return false;
    }
    hearingClap = true;
  }
  else
  {
    hearingClap = false;
  }

  return hearingClap;
}