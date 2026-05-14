//***********
// Libraries: magno@magnomaciel.com // nlp.rocks (Michael)
//***********
//#include "wire.h"
//#include "esp_err.h"

//***********
// Constants:
//***********
// 7-Segment display - segment bit:
#define LED_a 0x01
#define LED_b 0x02
#define LED_c 0x04
#define LED_d 0x08
#define LED_e 0x10
#define LED_f 0x20
#define LED_g 0x40
//#define LED_p 0x80

// Up and Dn keys ESP32 pins:
#define KeyUpPin 19
#define KeyDnPin 18

// 74HC595N shift register control ESP32 pins:
#define DS_Pin   21 // shift register data pin // aka SerPin
#define ST_Pin   12 // latch storage  clock    // aka LatchPin
#define SH_Pin   14 // shift register clock    // aka ClockPin

// #define MR_Pin   15 // Master Reset pin (active LOW) - connected to +5V
#define OE_Pin    5 // Ouput-Enable pin (active LOW) - connected to GND

// PIN to handle decimal-point:
// #define LED_dp   15 // Controls decimal-point

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
int   iHeight = 20; // start with 2 meters
//float fHeight = 0; // to contain iHeight / 100
float CurrMillis  = 0.0;
float StopMillis  = 0.0;
// ALL THESE VARIABLES WHERE MENT TO BE LOCAL BUT WHERE
// DECLARED HERE DUE TO ACCESS VIOLATION ERRORS!
// Keep track of exhibitting digits:
// byte bDigit1    = 0x00;
// byte bDigit2    = 0x00;
// byte bDigit1Aux = 0x00;
// byte bDigit2Aux = 0x00;
// byte bDigit1Ant = 0x00;
// byte bDigit2Ant = 0x00;

int   i = 0;
int   TimeCount = 0;
// int   iParte1 = 0;
// int   iParte2 = 0;

//********************************
// Functions' forward declaration:
//********************************
/*
// THIS BLOCK WAS COMMENTED DUE TO REPEATED ACCESS VIOLATION ERRORS
// BEGIN OF BLOCK// Execute code for the key pressed:
void ExecuteKeyUp();
void ExecuteKeyDn();
void ShowHeight();
void DisplayDigits (int iDigit1, int iDigit2);
void ShowOnDisplay(byte aByte);
void ClearDisplay();
void BlinkDP();
int  Binarize (int iNumero);
*/

// Tratar interrupções (interrupt handling):
void IRAM_ATTR ISR_KeyUp();
void IRAM_ATTR ISR_KeyDn();

// Acertar indicador de tecla Dn pressionada (set Dn key as pressed):
void IRAM_ATTR ISR_KeyDn(){
  bKeyPressed = true;
  bKeyDn      = true;
  bKeyUp      = false;
}

// Acertar indicador de tecla Up pressionada (set Up key as pressed):
void IRAM_ATTR ISR_KeyUp(){
  bKeyPressed = true;
  bKeyUp      = true;
  bKeyDn      = false;
}
// END OF BLOCK
//*/


// Increase height:
void ExecuteKeyUp(){
  bKeyPressed = true;
  Serial.println("Up");
  iHeight++; //= iHeight + 10;
  if (iHeight > HeightMax) iHeight = HeightMax;
  // Serial.println(iHeight);
  // Serial.println();
  ShowHeight();
  bKeyUp = false;
}


// Decrease height:
void ExecuteKeyDn(){
  bKeyPressed = true;
  Serial.println("Dn");
  iHeight--; // = iHeight - 10;
  if (iHeight < HeightMin) iHeight = HeightMin;
  // Serial.println(iHeight);
  // Serial.println();
  ShowHeight();
  bKeyDn = false;
}


// Display the height:
void ShowHeight(){
  int   iPart1   = 0;
  int   iPart2   = 0;

  String sHeight;
  String sPart1;
  String sPart2;

  /*
  int   iParteInt = 0.0;
  float fParteDec = 0.0;
  float fHeight   = 0.0;
  
  fHeight   = iHeight / 10.0;
  iParteInt = (fHeight);
  fParteDec = (fmod(fHeight, 1) * 10.0);

  iParte1 = Binarize(iParteInt);
  iParte2 = Binarize(int(fParteDec));
  */

  sHeight = String(iHeight);
  if (iHeight < 10) {
    sHeight = "0" + sHeight;
  }

  sPart1 = sHeight[0];
  sPart2 = sHeight[1];

  // Serial.println("Partes de sHeight:");
  // Serial.print(sPart1);
  // Serial.print("-");
  // Serial.print(sPart2);
  // Serial.println();

  iPart1 = sHeight[0] - 48; //atoi(*sPart1);
  iPart2 = sHeight[1] - 48; //atoi(*sPart2);

  // Serial.println("Partes inteiras:");
  // Serial.print(iPart1);
  // Serial.print("-");
  // Serial.print(iPart2);
  // Serial.println();

  iPart1 = Binarize(iPart1);
  iPart2 = Binarize(iPart2);

  // Serial.printf("sHeight=%s", sHeight);
  // Serial.println();
  // Serial.printf("Binary for D1=0x%02X  D2=0x%02X:", iPart1, iPart2);
  // Serial.println();
  // Serial.print("D1=");
  // Serial.print(iParte1, BIN);
  // Serial.print("  D2=");
  // Serial.print (iParte2, BIN);
  // Serial.println();
  DisplayDigits(iPart1, iPart2);
  //*/
}

// Exibe um número num dos lados do display:
int Binarize (int iNumero){
  int iResult = 0;

  iResult = 0;

  // Finds out the binary to mach each LED segment for numbers from 0 to 9. Any other number activates the ".":
  switch (iNumero) {
    case 1:{
      iResult = LED_b + LED_c;
      // Serial.println("   ");
      // Serial.println("  |");
      // Serial.println("   ");
      // Serial.println("  |");
      // Serial.println("   ");
      break;
    }
    case 2:{
      iResult = LED_a + LED_b + LED_g + LED_e + LED_d;
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println(" - ");
      // Serial.println("|  ");
      // Serial.println(" - ");
      break;
    }
    case 3:{
      iResult = LED_a + LED_b + LED_g + LED_c + LED_d;
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println(" - ");
      break;
    }
    case 4:{
      iResult = LED_f + LED_g + LED_b + LED_c;
      // Serial.println("   ");
      // Serial.println("| |");
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println("   ");
      break;
    }
    case 5:{
      iResult = LED_a + LED_f + LED_g + LED_c + LED_d;
      // Serial.println(" - ");
      // Serial.println("|  ");
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println(" - ");
     break;
    }
    case 6:{
      iResult = LED_f + LED_e + LED_d + LED_c + LED_g;
      // Serial.println("   ");
      // Serial.println("|  ");
      // Serial.println(" - ");
      // Serial.println("| |");
      // Serial.println(" - ");
     break;
    }
    case 7:{
      iResult = LED_a + LED_b + LED_c;
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println("   ");
      // Serial.println("  |");
      // Serial.println("   ");
      break;
    }
    case 8:{
      iResult = LED_a + LED_b + LED_c + LED_d + LED_e + LED_f + LED_g;
      // Serial.println(" - ");
      // Serial.println("| |");
      // Serial.println(" - ");
      // Serial.println("| |");
      // Serial.println(" - ");
      break;
    }
    case 9:{
      iResult = LED_g + LED_f + LED_a + LED_b + LED_c;
      // Serial.println(" - ");
      // Serial.println("| |");
      // Serial.println(" - ");
      // Serial.println("  |");
      // Serial.println("   ");
      break;
    }
    case 0:{
      iResult = LED_a + LED_b + LED_c + LED_d + LED_e + LED_f;
      // Serial.println(" _ ");
      // Serial.println("| |");
      // Serial.println("   ");
      // Serial.println("| |");
      // Serial.println(" - ");
     break;
    }
    // default:{
    //   iResult = LED_p;
    //   Serial.println("   ");
    //   Serial.println("   ");
    //   Serial.println("   ");
    //   Serial.println("   ");
    //   Serial.println("   .");
    //   break;
    // }
  }

  return iResult;
}

// Show digits on the dual 7-segment display:
void DisplayDigits (int iDigit1, int iDigit2){
byte bDigit1    = 0x00;
byte bDigit2    = 0x00;
  // Serial.print("D1=");
  // Serial.print(iDigit1, BIN);
  // Serial.print("  D2=");
  // Serial.print (iDigit2, BIN);
  // Serial.println();
  digitalWrite (OE_Pin, LOW);

  if (iDigit1 + iDigit2 == 0){
    ClearDisplay();
    return;
  }
  
  if (iDigit1 == 0){
    bDigit1  = iDigit2;
    bDigit2  = iDigit1;
    // digitalWrite(LED_dp, LOW);
  }
  else {
    bDigit2  = iDigit2;
    bDigit1  = iDigit1;
    // digitalWrite(LED_dp, HIGH);
  }

  // Since both shiftregisters are connected in series, we need to send digit 2 first, then digit 1:
  ShowOnDisplay (iDigit2);
  ShowOnDisplay (iDigit1);
  //Serial.println("Shown");
}

// Performs a master rest on both shift registers:
void ClearDisplay(){
  digitalWrite(OE_Pin, HIGH);
  // digitalWrite(MR_Pin, LOW);
  // Serial.println("Display cleared");
}


//*************************************************************************************************************************************************************
// Blink the decimal point to signal idle:
void BlinkDP(){
  // bDigit1Ant = bDigit1;
  // bDigit2Ant = bDigit2;

  // bDigit2Aux = bDigit2 + LED_p;
  // bDigit1Aux = bDigit1;

  // ShowOnDisplay (bDigit2Aux);
  // ShowOnDisplay (bDigit1Aux);
  digitalWrite(OE_Pin, LOW);
  // digitalWrite(LED_dp, HIGH);
  delay(500);

  // bDigit1 = bDigit1Ant;
  // bDigit2 = bDigit2Ant;

  // ShowOnDisplay (bDigit1Ant);
  // ShowOnDisplay (bDigit2Ant);
  digitalWrite(OE_Pin, HIGH);
  // digitalWrite(LED_dp, LOW);
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
// writeData writes the new data to the dataPin.
void WriteData(){
  digitalWrite (DS_Pin, HIGH);

  LatchTick();

  digitalWrite (DS_Pin, LOW);
}

//*************************************************************************************************************************************************************
// Displays a byte by commanding each bit:
void ShowOnDisplay(byte aByte){
  // Serial.print("aByte=");
  // Serial.print(aByte, BIN);
  // Serial.printf("(0x%02X)", aByte);
  // Serial.println();

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

  // Keeps both shift registers active:
  //digitalWrite(MR_Pin, HIGH);

  ShowHeight();
  delay(1000);
  digitalWrite (OE_Pin, HIGH); // disables display

  // iHeight = HeightMax / 2;
  //i = 0;
  bKeyUp      = false;
  bKeyDn      = false;
  bKeyPressed = false;

  // bDigit1    = 0x00;
  // bDigit2    = 0x00;
  // bDigit1Ant = 0x00;
  // bDigit2Ant = 0x00;  
  // bDigit1Aux = 0x00;
  // bDigit2Aux = 0x00;

  // fHeight = 0;
  // iParte1 = 0;
  // iParte2 = 0;
  // TimeCount   = 0;
  Serial.println("OK");
}


//*************************************************************************************************************************************************************
// Programa principal (main program):
//*************************************************************************************************************************************************************
void loop() {  
  // if (digitalRead(KeyUpPin) == LOW && !bKeyUp){
  //   digitalWrite(OE_Pin, LOW);
  //   bKeyUp = true;
  //   bKeyDn = false;
  //   Serial.println("Up");
  //   ExecuteKeyUp();
  //   TimeCount = KeyboardTimeOut;
  //   delay(250);
  // }
  // else if (digitalRead(KeyDnPin) == LOW && !bKeyDn){
  //   digitalWrite(OE_Pin, LOW);
  //   bKeyDn = true;
  //   bKeyUp = false;
  //   Serial.println("Dn");
  //   ExecuteKeyDn();
  //   TimeCount = KeyboardTimeOut;
  //   delay(250);
  // }
  // else 
  if (bKeyPressed){
    if (bKeyUp){
      CurrMillis = millis();
      StopMillis  = CurrMillis + KeyboardTimeOut;

      digitalWrite(OE_Pin, LOW);
      bKeyUp = false;
      bKeyDn = false;
      Serial.println("Up");
      ExecuteKeyUp();
      // TimeCount = KeyboardTimeOut;
    }
    else if (bKeyDn){
      CurrMillis = millis();
      StopMillis  = CurrMillis + KeyboardTimeOut;

      digitalWrite(OE_Pin, LOW);
      bKeyUp = false;
      bKeyDn = false;
      Serial.println("Dn");
      ExecuteKeyDn();
      // TimeCount = KeyboardTimeOut;
    }
    else {
      CurrMillis = millis();
      if (CurrMillis > StopMillis){
        bKeyPressed = false;
        bKeyUp      = false;
        bKeyDn      = false;
        digitalWrite(OE_Pin, HIGH);
        // TimeCount = KeyboardTimeOut;
      }
      else BlinkDP();
    }
  }
  else {
    // BlinkDP();
    // Serial.print(".");    
    // i++;
    // if (i>100) {
    //   Serial.println();
    //   i=0;
    // }
  }
  //delay(250);
}
















