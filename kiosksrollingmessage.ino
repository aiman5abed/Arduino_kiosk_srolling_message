#include<Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>


#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 11
#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10
#define VERT_PIN A0
#define HORZ_PIN A1
#define SEL_PIN  2
#define SIG_PIN A5
#define RESET_SWITCH_PIN 3 
#define BUF_SIZE 100

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

int scrollSpeed = 30;
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "Hello! Enter new message     " }; // Added 5 spaces for smooth transition
bool newMessageAvailable = true;
bool freezeMessage = false;
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textEffect_t lastScrollEffect = PA_SCROLL_LEFT;

void readSerial(void)
{
  static char *cp = newMessage;
  while (Serial.available())
  {
    *cp = (char)Serial.read();
    if ((*cp == '\n') || (cp - newMessage >= BUF_SIZE - 2))
    {
      *cp = '\0';
      strcat(newMessage, "     "); // Add spaces for smooth transition
      cp = newMessage;
      newMessageAvailable = true;
    }
    else
      cp++;
  }
}

void checkJoystick()
{
  int horz = analogRead(HORZ_PIN);
  int vert = analogRead(VERT_PIN);
  int sel = digitalRead(SEL_PIN);
  static textEffect_t previousEffect = lastScrollEffect;

  if (sel == LOW)
  {
    freezeMessage = !freezeMessage; // Toggle freeze state
    delay(200);  // debounce delay for button press
  }
  
    // When frozen, check for joystick input to restart scrolling
    if (horz < 400)
      lastScrollEffect = PA_SCROLL_LEFT;
    else if (horz > 600)
      lastScrollEffect = PA_SCROLL_RIGHT;
    else if (vert < 400)
      lastScrollEffect = PA_SCROLL_UP;
    else if (vert > 600)
      lastScrollEffect = PA_SCROLL_DOWN;

    if (lastScrollEffect != previousEffect)
    {
      P.displayClear();
      scrollEffect = lastScrollEffect;
      P.setTextEffect(lastScrollEffect, scrollEffect);
      P.displayReset();  // restart scrolling with new direction
      freezeMessage = false;  // Unfreeze after direction change
    }

  previousEffect = lastScrollEffect;
}

void checkPot(){
  // Read the potentiometer
  int potValue = analogRead(SIG_PIN);
  scrollSpeed = map(potValue, 0, 1023, 10, 80);
  P.setSpeed(scrollSpeed);  // This updates the delay between frames.
}


// Function to reset the system to its default state.
void resetSystem() {
  // Reset scrolling parameters to defaults.
  freezeMessage = false;
  scrollEffect = PA_SCROLL_LEFT;
  lastScrollEffect = PA_SCROLL_LEFT;
  // Reset message buffers to the default message.
  strcpy(curMessage, "Hello! Enter new message     ");
  newMessageAvailable = false;  // Clear any pending new message
  P.displayClear();
  // Update the display with the default message.
  P.displayText(curMessage, PA_CENTER, scrollSpeed, scrollEffect, scrollEffect);
}

void checkResetSwitch() {
  if (digitalRead(RESET_SWITCH_PIN) == LOW) {  // When the switch is activated
    resetSystem(); 
    delay(200);  // Debounce delay for the slide switch
  }
}

void handleSerial(){
    if (!freezeMessage && P.displayAnimate())
  {
    // When the current text has finished animating, check if a new message is available
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
      // Set the new text with the updated scrollSpeed and effect
      P.displayText(curMessage, PA_CENTER, scrollSpeed, scrollEffect, scrollEffect);
    }
    else
    {
      // Continue scrolling the current text
      P.displayReset();
    }
  }
}

void setup()
{
  Serial.begin(57600);
  Serial.print("\nType a message for the scrolling display\nEnd message line with a newline");
  pinMode(SEL_PIN, INPUT_PULLUP);
  pinMode(RESET_SWITCH_PIN, INPUT_PULLUP); // Set the slide switch pin as input with pull-up
  P.begin();
  // Initialize curMessage with the default text so something appears
  strcpy(curMessage, newMessage);
  P.displayText(curMessage, PA_CENTER, scrollSpeed, scrollEffect, scrollEffect);
}

void loop()
{
  checkJoystick(); //use the joystick input to decide scolling direction and un/freeze scroll effect.
  checkPot();  // Updates scrollSpeed based on the potentiometer
  checkResetSwitch();   // Check if the slide switch is activated to reset the display
  handleSerial(); //handle scrolling effect after a freeze sequence has been initialized to ensure uncropped text preview
  readSerial(); //if user has new prompted message handle accordingly
}