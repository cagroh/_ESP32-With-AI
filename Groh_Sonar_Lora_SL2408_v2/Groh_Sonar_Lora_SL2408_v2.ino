//*************************************************************************************************************************************************************************************************************************************
// COGИITIVA GROH SL-2408
// - ESP32-based
// - Contactless liquid level sensor (sonar)
// - LoRa radio
// - Recipient height adjustment
// - 7-segment display
// - Groh COGИET exclusive auto addressing
//*************************************************************************************************************************************************************************************************************************************//


//*************************************************************************************************************************************************************************************************************************************//
// Device definitions:
//*************************************************************************************************************************************************************************************************************************************//


#define GrohDeviceModel  "SL2408"
#define GrohDeviceSerial "24H001"


//*************************************************************************************************************************************************************************************************************************************//
// Libraries:
//*************************************************************************************************************************************************************************************************************************************//


#include <Preferences.h>
#include <SPI.h>
#include <LoRa.h>  // LoRa by Sandeep Mistry


//*************************************************************************************************************************************************************************************************************************************//
// Constants:
//*************************************************************************************************************************************************************************************************************************************//


// Comment to disable debug mode
#define _debug_mode   1

// Define pins used by LoRa on VSPI mode:
#define LoRa_NSS      27
#define LoRa_DIO0     26
#define LoRa_NRST     25
// Note that LoRa on VSPI also uses these default VSPI pins:
// #define LoRa_MOSI  23
// #define LoRa_MISO  19
// #define LoRa_SCLK  18

// Define reed switch pins:
#define ReedSwitchPin  4  // trigger for new client device anouncement

// Define status LED pin:
#define StatusLedPin   2

// Definir os pinos do sonar:
#define SonTrgrPin    33  // sonar trigger
#define SonEchoPin    32  // sonar echo

// Up and Dn keys ESP32 pins:
#define KeyUpPin      16  // increment height
#define KeyDnPin      17  // decrement height

// 74HC595N shift register control ESP32 pins:
#define DS_Pin        21  // shift register data pin // aka SerPin
#define OE_Pin        22  // Ouput-Enable pin (active LOW) - connected to GND
#define ST_Pin        12  // latch storage  clock    // aka LatchPin
#define SH_Pin        14  // shift register clock    // aka ClockPin
//      MR_Pin        VCC // Active low

// 7-Segment display - segment bit:
#define LED_a  0x02
#define LED_b  0x04
#define LED_c  0x08
#define LED_d  0x10
#define LED_e  0x20
#define LED_f  0x40
#define LED_g  0x80
//#define LED_dp 0x80

// TIme-Out limits:
#define KeyboardTimeOut 10000L  // 10 seconds (in millisseconds)
#define LoRaTimeOut      3000L  //  3 seconds (in millisseconds)
#define SonarTimeOut     3000L  //  3 seconds (in millisseconds)
#define ReadoutFreq     10000L//3600000L  // one reading per hour

// Liquid recipient boudaries:
/*
#define HeightMin  10.0L  // 10 cm
#define HeightMax 400.0L  // 4 meters (400 cm)

// Default gap between the top level of the recipient and the sonar transducer:
#define SonarGapDefault 10.0L  // 10cm
*/
constexpr int HeightMin       =  30;
constexpr int HeightMax       = 400;
constexpr int SonarGapDefault =  10;
constexpr int iIncrement      =  10;

//*************************************************************************************************************************************************************************************************************************************//
// Global Variables:
//*************************************************************************************************************************************************************************************************************************************//


// key-pressed indicator:
bool bKeyUp      = false;
bool bKeyDn      = false;
bool bKeyPressed = false;

// Reed switch indicator:
bool bReedSwitch = false;

// Water recipient's current height:
int iHeight   = HeightMax / 2;    // start with 2 meters
int iSonarGap = SonarGapDefault;  // gap between the top level of the recipient and the sonar transducer
int iVezes = 0;

// Keyboard time-out variables:
unsigned long uCurrMillis = 0.0;
unsigned long uStopMillis = 0.0;

// Readout frequency:
unsigned long uFreqCurrMillis = 0.0;
unsigned long uFreqStopMillis = 0.0;

// Instanciate Preferences object:
Preferences preferences;


//*************************************************************************************************************************************************************************************************************************************//
// Forward declarations:
//*************************************************************************************************************************************************************************************************************************************//


void IRAM_ATTR ISR_KeyUp();
void IRAM_ATTR ISR_KeyDn();
void IRAM_ATTR ISR_Reed();

/*
void LoraTransmit(float fPercent);
void BlinkStatusLED();
void StatusLED(bool bState);
void halt();
*/

//*************************************************************************************************************************************************************************************************************************************//
// Initial setup:
//*************************************************************************************************************************************************************************************************************************************//
void setup() {
  // Initialize Serial Monitor and wait for it to become operative:
  Serial.begin(115200);
  while (!Serial);

  Serial.print("COGИITIVA ");
  Serial.print(GrohDeviceModel);
  Serial.printf(" SN: %s ready!\r\n", GrohDeviceSerial);
  Serial.printf("initial: H=%d  Hmin=%d  Hmax=%d  GAP=%d\r\n", iHeight, HeightMin, HeightMax, iSonarGap);
  Serial.println();
  Serial.flush();
 
  // Setup output pins for the shift-registers:
  pinMode     (DS_Pin, OUTPUT);     // data
  pinMode     (ST_Pin, OUTPUT);     // latch
  pinMode     (SH_Pin, OUTPUT);     // clock
  pinMode     (OE_Pin, OUTPUT);     // enables (low) or disables (high) display
  // digitalWrite(OE_Pin, HIGH);       // disables display
  digitalWrite(OE_Pin, LOW);       // disables display

  // Setup input pins for the keys and reed swith:
  pinMode(KeyUpPin,      INPUT_PULLUP);
  pinMode(KeyDnPin,      INPUT_PULLUP);
  pinMode(ReedSwitchPin, INPUT_PULLDOWN);

  // Setup sonar pins:
  pinMode     (SonTrgrPin, OUTPUT);
  pinMode     (SonEchoPin, INPUT);
  digitalWrite(SonTrgrPin, LOW);
  digitalWrite(SonEchoPin, LOW);

  // Setup Status LED pin:
  pinMode     (StatusLedPin, OUTPUT);
  digitalWrite(StatusLedPin, LOW);

  // Enable keys Up and Dn:
  attachInterrupt(KeyUpPin,      ISR_KeyUp, FALLING);
  attachInterrupt(KeyDnPin,      ISR_KeyDn, FALLING);
  attachInterrupt(ReedSwitchPin, ISR_Reed, RISING);

  // Initialize LoRa radio:
  LoraInit();

  // Recover stored limits from preferences:
  preferences.begin("Limits", false);
  iHeight   = preferences.getInt("Height",   HeightMax / 2);
  if (iHeight > HeightMax) iHeight = HeightMax;
  if (iHeight < HeightMin) iHeight = HeightMin;

  iSonarGap = preferences.getInt("SonarGap", SonarGapDefault);

  preferences.end();

  // On the first time, take the average from 10 sonar readings to calibrate the gap
  // between the sonar and the top of the tank (this must be done with a full tank):
  if (iSonarGap <= 0){
    MeasureAndRegisterSonarGap();
  }

  if (iSonarGap < SonarGapDefault) iSonarGap = SonarGapDefault;

  // Briefely show the current height:
  ShowHeight();
  delay(5000);

  // Disable the display:
  // digitalWrite(OE_Pin, HIGH);

  // Ensure Keyboard is initialized as false:
  bKeyUp      = false;
  bKeyDn      = false;
  bKeyPressed = false;

  // Disable reed swith:
  bReedSwitch = false;

  // Force a sonar reading on start-up:r
  uFreqStopMillis = 0;
  iVezes = 1;
  Serial.printf("after initial: H=%d  Hmin=%d  Hmax=%d  GAP=%d\r\n", iHeight, HeightMin, HeightMax, iSonarGap);
  Serial.println();
  Serial.flush();
}


//*************************************************************************************************************************************************************************************************************************************//
// Main loop:
//*************************************************************************************************************************************************************************************************************************************//
void loop() {
  float fPercent = 0.0;

  // Check if a key was pressed:
  if (bKeyPressed) {
    if (bKeyUp) {
      // Increase and show height:
      ExecuteKeyUp();
    } else if (bKeyDn) {
      // Decrease and show height:
      ExecuteKeyDn();
    } else {
      // No key was pressed for a while, check for time-out:
      uCurrMillis = millis();
      if (uCurrMillis > uStopMillis) {
        // Store new limits on preferences:
        preferences.begin("Limits",    false);
        preferences.putInt("Height",   iHeight);
        preferences.putInt("SonarGap", iSonarGap);
        preferences.end();

        // Reset keypress indicators:
        bKeyPressed = false;
        bKeyUp = false;
        bKeyDn = false;

        // Turn display off;
        digitalWrite(OE_Pin, HIGH);
      }
      // Keyboard not timed-off, blink display:
      else
        BlinkDP();
    }
  } else {
    // Check if it's time for another sonar reading:
    uFreqCurrMillis = millis();
    if (uFreqCurrMillis >= uFreqStopMillis) {
      // Mark execution times:
      Serial.printf("Execution #%d\r\n", iVezes);
      iVezes++;

      // Get the new level as % of the total height:
      fPercent = GetSonarPercent();

      // Send it over LoRa:
      LoraTransmit(fPercent);

      // Reset timer:
      uFreqStopMillis = uFreqCurrMillis + ReadoutFreq;
    }
    // Use the idle time to check for the reed switch:
    // else {
      // // Send out the device ID while the reed switch is activated:
      // if (digitalRead(ReedSwitchPin) == HIGH){
      //   // Signal status:
      //   StatusLED(true);

      //   // Set reed swtich indicator:
      //   bReedSwitch = true;

      //   // Send the ID over LoRa:
      //   LoraTransmit(-1);

      //   // Give a break:
      //   delay(1000);
      // }
      // else {
        // Has there been a disconnection of the reed switch?
        // if (bReedSwitch == true) {
        //   // Reset sonar gap to force execution next time the device is turned on:
        //   preferences.begin("Limits",    false);
        //   preferences.putInt("Height",   iHeight);
        //   preferences.putInt("SonarGap", -1);
        //   preferences.end();
        //   bReedSwitch = false;
        //   StatusLED(false);
        // }
      // }
    // }
  }
  // Nothing to do, let's blink idle:
  BlinkStatusLED();
}


//*************************************************************************************************************************************************************************************************************************************//
// Additional functions:
//*************************************************************************************************************************************************************************************************************************************//


// Set KeyUp as pressed:
// ---------------------
void IRAM_ATTR ISR_KeyUp() {
  bKeyPressed = true;
  bKeyUp      = true;
  bKeyDn      = false;
}


// Set KeyDn as pressed:
// ---------------------
void IRAM_ATTR ISR_KeyDn() {
  bKeyPressed = true;
  bKeyDn      = true;
  bKeyUp      = false;
}


// Handle Reed Switch:
void IRAM_ATTR ISR_Reed() {
  // Prevent multiple calls:
  if (bReedSwitch) return;
  bReedSwitch = true;

  Serial.println("Reed switch detected!");

  // Send out the device ID while the reed switch is activated:
  if (digitalRead(ReedSwitchPin) == HIGH){
    // Signal status:
    StatusLED(true);

    // Reset sonar gap to force execution next time the device is turned on:
    preferences.begin("Limits",    false);
    preferences.putInt("Height",   iHeight);
    preferences.putInt("SonarGap", -1);
    preferences.end();
    //bReedSwitch = false;
    StatusLED(false);
  }

  Serial.printf("Saved data: Height=%d Gap=-1\r\n", iHeight);
  Serial.println("Entering Halt. Please RESET device!");
  Serial.flush();

  // Ask to reset device:
  DisplayError (3, 0);
  // Send the ID over LoRa:
  //LoraTransmit(-1);

  halt();

  // Give a break:
  //delay(1000);
  //StatusLED(false);
}



// Display the height:
// -------------------
void ShowHeight() {
  int iPart1 = 0;
  int iPart2 = 0;

  String sHeight;

  sHeight = String(iHeight / 10);

  if (iHeight < 10) {
    sHeight = "0" + sHeight;
  }

  iPart1 = sHeight[0] - 48;
  iPart2 = sHeight[1] - 48;

  iPart1 = Binarize(iPart1);
  iPart2 = Binarize(iPart2);

  DisplayDigits(iPart1, iPart2);
}


// Generate a binary representation of the digits 0-9:
// ---------------------------------------------------
int Binarize(int iNumero) {
  int iResult = 0;

  iResult = 0;

  // Finds out the binary to mach each LED segment for numbers from 0 to 9. Any other number shows "E":
  switch (iNumero) {
    case 1: {
      iResult = LED_b + LED_c;
      break;
    }
    case 2: {
      iResult = LED_a + LED_b + LED_g + LED_e + LED_d;
      break;
    }
    case 3: {
      iResult = LED_a + LED_b + LED_g + LED_c + LED_d;
      break;
    }
    case 4: {
      iResult = LED_f + LED_g + LED_b + LED_c;
      break;
    }
    case 5: {
      iResult = LED_a + LED_f + LED_g + LED_c + LED_d;
      break;
    }
    case 6: {
      iResult = LED_f + LED_e + LED_d + LED_c + LED_g;
      break;
    }
    case 7: {
      iResult = LED_a + LED_b + LED_c;
      break;
    }
    case 8: {
      iResult = LED_a + LED_b + LED_c + LED_d + LED_e + LED_f + LED_g;
      break;
    }
    case 9: {
      iResult = LED_g + LED_f + LED_a + LED_b + LED_c;
      break;
    }
    case 0: {
      iResult = LED_a + LED_b + LED_c + LED_d + LED_e + LED_f;
      break;
    }
    case 'S': {
      iResult = LED_a + LED_f + LED_g + LED_c + LED_d;
      break;
    }
    case 'f': {
      iResult = LED_a + LED_b + LED_c + LED_g;
      break;
    }
    case 'L': {
      iResult = LED_d + LED_e + LED_f;
      break;
    }
    case 'o': {
      iResult = LED_c + LED_d + LED_e + LED_g;
      break;
    }
  }
  return iResult;
}


// Show digits on the dual 7-segment display:
// ------------------------------------------
void DisplayDigits(int iDigit1, int iDigit2) {
  byte bDigit1 = 0x00;
  byte bDigit2 = 0x00;

  // setting OE as LOW enables the display:
  digitalWrite(OE_Pin, LOW);

  // If both digits are zero, turn display off:
  if (iDigit1 + iDigit2 == 0) {
    ClearDisplay();
    return;
  }

  // If first digit is zero, invert:
  if (iDigit1 == 0) {
    bDigit1 = iDigit2;
    bDigit2 = iDigit1;
  } else {
    bDigit2 = iDigit2;
    bDigit1 = iDigit1;
  }

  // Since both shift registers are connected in series, we need to send digit 2 first, then digit 1:
  ShowOnDisplay(iDigit2);
  ShowOnDisplay(iDigit1);
}


// Setting OW as HIGH disables the display:
// ----------------------------------------
void ClearDisplay() {
  digitalWrite(OE_Pin, HIGH);
}


// Blink the decimal point/display to signal idle:
// -----------------------------------------------
void BlinkDP() {
  digitalWrite(OE_Pin, LOW);
  delay(500);

  digitalWrite(OE_Pin, HIGH);
  delay(500);
}


// Cycles clock pins:
// ------------------
void LatchTick() {
  // Turn clock pins on:
  digitalWrite(ST_Pin, HIGH);  // latchP // turns the latch on
  digitalWrite(SH_Pin, HIGH);  // clockP // turns the clock on

  // Clock timing seems to be 10uSecs:
  delayMicroseconds(10);

  // Turn clock pins off:
  digitalWrite(ST_Pin, LOW);  // turns the latch off
  digitalWrite(SH_Pin, LOW);  // turns clock off

  // Clock timing seems to be 10uSecs:
  delayMicroseconds(10);
}


// Writes the new data to the data Pin:
// ------------------------------------
void WriteData() {
  // Write the bit:
  digitalWrite(DS_Pin, HIGH);

  // Cycle clocks:
  LatchTick();

  // Get bit down to zero again:
  digitalWrite(DS_Pin, LOW);
}


// Display a byte one bit at a time:
// ---------------------------------
void ShowOnDisplay(byte aByte) {
  // loop to show each bit for the 7-segment display, HSB first:
  for (int x = 7; x >= 0; x--) {
    int bit = bitRead(aByte, x);
    if (bit == 1) {
      WriteData();
    } else {
      LatchTick();
    }
  }
}


// Increase and show height on KeyUp:
// ----------------------------------
void ExecuteKeyUp() {
  // Unset which key was pressed, but signal a key was pressed:
  bKeyUp = false;
  bKeyPressed = true;

  // Reinitialize time-out at each keystroke:
  uCurrMillis = millis();
  uStopMillis = uCurrMillis + KeyboardTimeOut;

  // Enable display:
  digitalWrite(OE_Pin, LOW);

  // Increment height and show new value:
  iHeight = iHeight + iIncrement;
  if (iHeight > HeightMax) iHeight = HeightMax;
  ShowHeight();
}


// Decrement and show height on KeyDn:
// ----------------------------------
void ExecuteKeyDn() {
  // Unset which key was pressed, but signal a key was pressed:
  bKeyDn = false;
  bKeyPressed = true;

  // Reinitialize time-out at each keystroke:
  uCurrMillis = millis();
  uStopMillis = uCurrMillis + KeyboardTimeOut;

  // Enable display:
  digitalWrite(OE_Pin, LOW);

  // Increment height and show new value:
  iHeight = iHeight - iIncrement;
  if (iHeight > HeightMin) iHeight = HeightMin;
  ShowHeight();
}


// Get the "percent full" from sonar:
// ----------------------------------
float GetSonarPercent() {
  float fDistance = 0.0;
  float fPercent  = 0.0;


  Serial.printf("Using GAP=%d and HEIGHT=%d\r\n", iSonarGap, iHeight);

  // Get the average of 3 measures:
  fDistance = GetSonarAverageMeasures(3, 1);

  // Try to adjust height as it was read as too low from last sonar reading:
  if (fDistance > (iHeight + iSonarGap)){
    Serial.println("-Got Height discrepancie - trying to fix it");
    fDistance = GetSonarAverageMeasures(10, 1);
    iHeight = trunc(fDistance + iSonarGap);
    Serial.printf(" -New Height=%d\r\n", iHeight);
  }

  Serial.printf("Got fDistance=%3.1f\r\n", fDistance);
  Serial.printf("fPercent=(1.0 - ((%3.1f - %d) / %d))\r\n", fDistance, iSonarGap, iHeight);

  // Compute percent:
  fPercent = (1.0 - ((fDistance - iSonarGap) / iHeight)) * 100.0;

  Serial.printf("Computed \%=%3.1f\r\n", fPercent);
  Serial.flush();

  // Return percent:
  return fPercent;
}


// Initialize LoRa radio:
// ----------------------
void LoraInit(){
  // Setup LoRa pins:
  LoRa.setPins(LoRa_NSS, LoRa_NRST, LoRa_DIO0);

  // Initialize time-out:
  uStopMillis = millis() + LoRaTimeOut;

  Serial.println("Enabling LoRa");
  Serial.flush();

  // Try to connect to LoRa module:
  while (!LoRa.begin(915E6)) {
    // Wait a bit:
    delay(500);

   // Check for time-out:
    uCurrMillis = millis();
    if (uCurrMillis > uStopMillis) {
      // Initialize time-out:
      uStopMillis = millis() + LoRaTimeOut;

      // Error 1: LoRa timed-out!
      DisplayError(1, 1);

      // Freeze:
      halt();
    }
  }
}

// Send data using LoRa radio:
// ---------------------------
void LoraTransmit(float fPercent) {
  //int iTimeOutCounter = 0;

  // Signal Status LED:
  StatusLED(true);

  // All Cognitiva/Groh devices sync with 0x47 ('G'):
  LoRa.setSyncWord(0x47);

  // Prepare to send:
  LoRa.beginPacket();

  Serial.println("Sending LoRa packet");

  // Send device model and serial:
  Serial.printf("Sending %s\r\n", GrohDeviceModel);
  LoRa.println(GrohDeviceModel);
  Serial.printf("Sending S/N: %s\r\n", GrohDeviceSerial);
  LoRa.println(GrohDeviceSerial);

  // Send data and close LoRa:
  Serial.printf("Sending percent= %3.2f\r\n", fPercent);
  Serial.flush();

  LoRa.println(fPercent);
  LoRa.endPacket();
  //LoRa.end();
  //LoRa.flush();
  Serial.printf("All sent. Going to sleep %d milisseconds.\r\n", ReadoutFreq);
  Serial.flush();

  //Serial.println("Releasing LoRa");

  // Turn Status LED off:
  StatusLED(false);
}


// Obtain average of 'n' readings from sonar:
// ------------------------------------------
float GetSonarAverageMeasures(int iTimes, int iCall) {
  float fPrev  = -1;
  float fCurr  =  0;
  float fTotal =  0;
  int   iCount =  0;

  // Signal status:
  StatusLED(true);

  // Initialize counter:
  iCount = 0;

  // Initialize time-out:
  uCurrMillis = millis();
  uStopMillis = uCurrMillis + SonarTimeOut;

  // Get iTimes readings from sonar:
  while (iCount < iTimes) {
    // Check for time-out:
    uCurrMillis = millis();
    if (uCurrMillis > uStopMillis) {
      // Initialize time-out:
      uStopMillis = millis() + LoRaTimeOut;

      // Error 2: Sonar timed-out!
      DisplayError(2, iCall);

      // Freeze:
      halt();
    }

    // Get a measure from sonar:
    Serial.println("pinging sonar...");
    fCurr = SonarPing();
    Serial.printf("Got %3.1f \n\r", fCurr);

    // Check if it's valid:
    if (fCurr > 0) {

      // Is it the first one?
      if (fPrev < 0) {
        // Equalize:
        fPrev = fCurr;
      }

      // Only use readings that don't differ too much (~20cm):
      if (abs(fCurr - fPrev) <= 20) {
        // Increment counter:
        iCount++;

        // Save current read:
        fPrev = fCurr;

        // Add it to total:
        fTotal = fTotal + fCurr;

        // Reset time-out:
        uCurrMillis = millis();
        uStopMillis = uCurrMillis + SonarTimeOut;
      }
    }
    else { 
      delay(500);
    }
  }

  // Turn status LED off:
  StatusLED(false);
  delay(250);

  // Return the average:
  Serial.printf("Avg= %3.1f \n\r", (fTotal / iTimes));
  Serial.flush();
  return (fTotal / iTimes);
}


// Ping sonar and return distance measured from target:
// ----------------------------------------------------
float SonarPing() {
  long  iDuration = 0;
  float fDistance = 0;

  // Turn sonar trigger off and wait 2us:
  digitalWrite(SonTrgrPin, LOW);
  delayMicroseconds(2);

  // Trigger the sonar and wait 10Us:
  digitalWrite(SonTrgrPin, HIGH);
  delayMicroseconds(10);

  // Turn sonar trigger off:
  digitalWrite(SonTrgrPin, LOW);

  // Get sonar echo:
  iDuration = pulseIn(SonEchoPin, HIGH);

  // Compute distance as a fraction of the speed of sound (340m/s):
  fDistance = iDuration * 0.034 / 2;

  // Return the distance measured:
  return fDistance;
}


// Show "E" + ErrCode on the 7-segment display:
// --------------------------------------------
void DisplayError(int iErrCode, int iCall) {
  int iPart1 = 0;
  int iPart2 = 0;

  Serial.printf ("Error %d - caller instance=%d", iErrCode, iCall);
  Serial.println();
  Serial.flush();

  // Turn Status LED on:
  StatusLED(true);

  // Make sure the 7-segment display is on:
  digitalWrite(OE_Pin, LOW);

  switch (iErrCode) {
    case 1: {
      // LoRa error:
      iPart1 = Binarize('L');
      iPart2 = Binarize('o');
    }
    case 2: {
      // Sonar error:
      iPart1 = Binarize('S');
      iPart2 = Binarize('o');
    }
    case 3: {
      // Must turn device off:
      iPart1 = Binarize('o');
      iPart2 = Binarize('f');
    }
  }

  // Present error:
  DisplayDigits(iPart1, iPart2);
}


// Measure and register the sonar gap:
// -----------------------------------
void MeasureAndRegisterSonarGap(){
  //long iDistance = 0;

  // Get the average of 10 measures:
  iSonarGap = trunc(GetSonarAverageMeasures(10, 2));

  Serial.printf("Got iSonarGap=%3.1f\r\n", iSonarGap);

  // Store new limits on preferences:
  preferences.begin("Limits",    false);
  preferences.putInt("Height",   iHeight);
  preferences.putInt("SonarGap", iSonarGap);
  preferences.end();
}


// Blink status LED:
// -----------------
void BlinkStatusLED(){
  StatusLED(true);
  delay (500);

  StatusLED(false);
  delay (500);
}


// Turns Status LED on/off:
// ------------------------
void StatusLED(bool bState){
  if (bState) {
    digitalWrite (StatusLedPin, HIGH);
  }
  else  {
    digitalWrite (StatusLedPin,  LOW);
  }
}


// Halt any further execution:
// ---------------------------
void halt(){
  //while (1) {}
}