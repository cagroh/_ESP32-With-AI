/*************************************************************************************************************************************************************************************************************************************
COGИITIVA GROH SL-2408 Sonar with LoRa
Contactless liquid level sensor

File "pins_arduino.h" defaults:
-------------------------------
static const uint8_t TX   = 1;
static const uint8_t RX   = 3;

static const uint8_t SDA  = 21;
static const uint8_t SCL  = 22;

static const uint8_t SS   =  5;
static const uint8_t MOSI = 23;
static const uint8_t MISO = 19;
static const uint8_t SCK  = 18;

*************************************************************************************************************************************************************************************************************************************/
//***********
// Libraries: 
// magno@magnomaciel.com // nlp.rocks (Michael) // fernando.come@oracle.com sobre agentes
//***********
#include <EEPROM.h>
#include <SPI.h>
#include <LoRa.h>

//***********
// Constants:
//***********
// Comment to disable debug mode
#define _debug_mode 1

// Define pins used by LoRa on SPI mode:
#define CSS          25 //  5
#define DIO0         32 // 13
#define RST          33 // 14

// Define LED pins for error and status:
#define StsLedPin     2
#define ErrLedPin     4

// Definir os pinos do sonar:
#define SonTrgrPin   13 // 12
#define SonEchoPin   15 // 13

// 7-Segment display - segment bit:
#define LED_a 0x01
#define LED_b 0x02
#define LED_c 0x04
#define LED_d 0x08
#define LED_e 0x10
#define LED_f 0x20
#define LED_g 0x40
// #define LED_p 0x80 // for some reason, the d.p.s on this display (LDD5115) are not working

// Up and Dn keys ESP32 pins:
#define KeyUpPin 17
#define KeyDnPin 16

// 74HC595N shift register control ESP32 pins:
#define ST_Pin   12 // latch storage  clock    // aka LatchPin
#define SH_Pin   14 // shift register clock    // aka ClockPin
#define DS_Pin   26 // shift register data pin // aka SerPin
#define OE_Pin   27 // Ouput-Enable pin (active LOW) - connected to GND

// Keyboard time-out:
#define KeyboardTimeOut 10000 // 10 seconds (in millisseconds)

// boudaries:
#define HeightMin  1
#define HeightMax 40

//***********
// Variables:
//***********
// key-pressed indicator:
bool bKeyUp      = false;
bool bKeyDn      = false;
bool bKeyPressed = false;

// Water recipient's current height:
int   iHeight    = 20; // start with 2 meters

// Keyboard time-out variables:
float CurrMillis  = 0.0;
float StopMillis  = 0.0;

//*********************************
// Functions' forward declarations:
//*********************************
// Tratar interrupções (interrupt handling):
void IRAM_ATTR ISR_KeyUp();
void IRAM_ATTR ISR_KeyDn();

//*************************************************************************************************************************************************************
// Functions:
//*************************************************************************************************************************************************************
// Set KeyDn as pressed):
void IRAM_ATTR ISR_KeyDn(){
  bKeyPressed = true;
  bKeyDn      = true;
  bKeyUp      = false;
}

//*************************************************************************************************************************************************************
// Set KeyUp as pressed):
void IRAM_ATTR ISR_KeyUp(){
  bKeyPressed = true;
  bKeyUp      = true;
  bKeyDn      = false;
}

//*************************************************************************************************************************************************************
// Increase height on KeyUp:
void ExecuteKeyUp(){
  bKeyPressed = true;
  // Serial.println("Up");
  iHeight++; 
  if (iHeight > HeightMax) iHeight = HeightMax;
  ShowHeight();
  bKeyUp = false;
}

//*************************************************************************************************************************************************************
// Decrease height on KeyDn:
void ExecuteKeyDn(){
  bKeyPressed = true;
  // Serial.println("Dn");
  iHeight--;
  if (iHeight < HeightMin) iHeight = HeightMin;
  ShowHeight();
  bKeyDn = false;
}

//*************************************************************************************************************************************************************
// Display the height:
void ShowHeight(){
  int   iPart1   = 0;
  int   iPart2   = 0;

  String sHeight;

  sHeight = String(iHeight);

  if (iHeight < 10) {
    sHeight = "0" + sHeight;
  }

  iPart1 = sHeight[0] - 48; 
  iPart2 = sHeight[1] - 48; 

  iPart1 = Binarize(iPart1);
  iPart2 = Binarize(iPart2);

  DisplayDigits(iPart1, iPart2);
}

//*************************************************************************************************************************************************************
// Exibe um número num dos lados do display:
int Binarize (int iNumero){
  int iResult = 0;

  iResult = 0;

  // Finds out the binary to mach each LED segment for numbers from 0 to 9. Any other number activates the ".":
  switch (iNumero) {
    case 1:{
      iResult = LED_b + LED_c;
      break;
    }
    case 2:{
      iResult = LED_a + LED_b + LED_g + LED_e + LED_d;
      break;
    }
    case 3:{
      iResult = LED_a + LED_b + LED_g + LED_c + LED_d;
      break;
    }
    case 4:{
      iResult = LED_f + LED_g + LED_b + LED_c;
      break;
    }
    case 5:{
      iResult = LED_a + LED_f + LED_g + LED_c + LED_d;
      break;
    }
    case 6:{
      iResult = LED_f + LED_e + LED_d + LED_c + LED_g;
      break;
    }
    case 7:{
      iResult = LED_a + LED_b + LED_c;
      break;
    }
    case 8:{
      iResult = LED_a + LED_b + LED_c + LED_d + LED_e + LED_f + LED_g;
      break;
    }
    case 9:{
      iResult = LED_g + LED_f + LED_a + LED_b + LED_c;
      break;
    }
    case 0:{
      iResult = LED_a + LED_b + LED_c + LED_d + LED_e + LED_f;
      break;
    }
  }

  return iResult;
}

//*************************************************************************************************************************************************************
// Show digits on the dual 7-segment display:
void DisplayDigits (int iDigit1, int iDigit2){
  byte bDigit1    = 0x00;
  byte bDigit2    = 0x00;

  digitalWrite (OE_Pin, LOW);

  if (iDigit1 + iDigit2 == 0){
    ClearDisplay();
    return;
  }
  
  if (iDigit1 == 0){
    bDigit1  = iDigit2;
    bDigit2  = iDigit1;
  }
  else {
    bDigit2  = iDigit2;
    bDigit1  = iDigit1;
  }

  // Since both shift registers are connected in series, we need to send digit 2 first, then digit 1:
  ShowOnDisplay (iDigit2);
  ShowOnDisplay (iDigit1);
}

//*************************************************************************************************************************************************************
// Performs a master rest on both shift registers:
void ClearDisplay(){
  digitalWrite(OE_Pin, HIGH);
}


//*************************************************************************************************************************************************************
// Blink the decimal point to signal idle:
void BlinkDP(){
  digitalWrite(OE_Pin, LOW);
  delay(500);
  
  digitalWrite(OE_Pin, HIGH);
  delay(500);
}

//*************************************************************************************************************************************************************
// Cycles the clock pins:
void LatchTick(){
  digitalWrite (ST_Pin, HIGH); // latchP // turns the latch on
  digitalWrite (SH_Pin, HIGH); // clockP // turns the clock on

  delayMicroseconds(10); // clock timing seems to be 10uSecs

  digitalWrite (ST_Pin, LOW);  // turns the latch off
  digitalWrite (SH_Pin, LOW);  // turns clock off

  delayMicroseconds(10);
}

//*************************************************************************************************************************************************************
// Writes the new data to the dataPin.
void WriteData(){
  digitalWrite (DS_Pin, HIGH);

  LatchTick();

  digitalWrite (DS_Pin, LOW);
}

//*************************************************************************************************************************************************************
// Displays a byte by commanding each bit:
void ShowOnDisplay(byte aByte){
  // loop to show each bit for the 7-segment display, HSB first:
  for (int x = 7; x >= 0; x--){
    int bit = bitRead(aByte, x);
    if(bit == 1){
      WriteData();
    } else {
      LatchTick();
    }
  }
}


//*************************************************************************************************************************************************************
// Initial setup:
//*************************************************************************************************************************************************************
void setup() {
  Serial.begin(115200);

  // Setup output pins for the shift-registers:
  pinMode(DS_Pin, OUTPUT); // data
  pinMode(ST_Pin, OUTPUT); // latch
  pinMode(SH_Pin, OUTPUT); // clock
  pinMode(OE_Pin, OUTPUT); // enables (low)/disables (high) display 
  digitalWrite (OE_Pin, HIGH); // disables display

  // Setup input pins for the keys:
  pinMode(KeyUpPin, INPUT_PULLUP);
  pinMode(KeyDnPin, INPUT_PULLUP);

  attachInterrupt(KeyUpPin, ISR_KeyUp, FALLING);
  attachInterrupt(KeyDnPin, ISR_KeyDn, FALLING);

  ShowHeight();
  delay(1000);
  digitalWrite (OE_Pin, HIGH); // disables display

  bKeyUp      = false;
  bKeyDn      = false;
  bKeyPressed = false;
  Serial.println("OK");
}


//*************************************************************************************************************************************************************
// Programa principal (main program):
//*************************************************************************************************************************************************************
void loop() {  
  if (bKeyPressed){
    if (bKeyUp){
      CurrMillis = millis();
      StopMillis  = CurrMillis + KeyboardTimeOut;

      digitalWrite(OE_Pin, LOW);
      bKeyUp = false;
      bKeyDn = false;
      Serial.println("Up");
      ExecuteKeyUp();
    }
    else if (bKeyDn){
      CurrMillis = millis();
      StopMillis  = CurrMillis + KeyboardTimeOut;

      digitalWrite(OE_Pin, LOW);
      bKeyUp = false;
      bKeyDn = false;
      Serial.println("Dn");
      ExecuteKeyDn();
    }
    else {
      CurrMillis = millis();
      if (CurrMillis > StopMillis){
        bKeyPressed = false;
        bKeyUp      = false;
        bKeyDn      = false;
        digitalWrite(OE_Pin, HIGH);
      }
      else BlinkDP();
    }
  }
}
















