/*
(c) 2024 by COGИITIVA and Cesar GROH
Example of driving a 2-digit, 7-segment display
using two 74HC595N shift registers 
*/

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
#define LED_a  0x02 //0x01
#define LED_b  0x04 //0x02
#define LED_c  0x08 //0x04
#define LED_d  0x10 //0x08
#define LED_e  0x20 //0x10
#define LED_f  0x40 //0x20
#define LED_g  0x80 //0x40
//#define LED_dp 0x80

void IRAM_ATTR ISR_KeyUp();
void IRAM_ATTR ISR_KeyDn();

int SegmentDisplay[] = {LED_a, LED_b, LED_c, LED_d, LED_e, LED_f, LED_g}; //, LED_dp}; 
String LEDs[] = {"LED_a", "LED_b", "LED_c", "LED_d", "LED_e", "LED_f", "LED_g"}; //, "LED_dp"};

int iSegIndex    = 0;
bool bKeyPressed = false;
bool bKeyUp = false;
bool bKeyDn = false;

void setup(){
  // Initialize Serial Monitor and wait for it to become operative:
  Serial.begin(115200);
  while (!Serial);
 
  // Setup output pins for the shift-registers:
  pinMode     (DS_Pin, OUTPUT);     // data
  pinMode     (ST_Pin, OUTPUT);     // latch
  pinMode     (SH_Pin, OUTPUT);     // clock
  pinMode     (OE_Pin, OUTPUT);     // enables (low) or disables (high) display
  digitalWrite(OE_Pin, LOW);        // enables display

  // Setup input pins for the keys and reed swith:
  pinMode(KeyUpPin,      INPUT_PULLUP);
  pinMode(KeyDnPin,      INPUT_PULLUP);

  // Enable keys Up and Dn:
  attachInterrupt(KeyUpPin, ISR_KeyUp, FALLING);
  attachInterrupt(KeyDnPin, ISR_KeyDn, FALLING);

  bKeyPressed = false;
  bKeyUp = false;
  bKeyDn = false;
  iSegIndex = -1;
}

void loop() {
  byte iValue = 0;
  if (bKeyUp){
    // Enable display:
    digitalWrite(OE_Pin, LOW);

    // Increment height and show new value:
    iSegIndex++;
    if (iSegIndex>6) {
      iSegIndex=8;
      ActivateSegment(LED_a + LED_b + LED_c + LED_d + LED_e + LED_f + LED_g); // + LED_dp);
      Serial.println("Segment=8. (all)");
      iSegIndex=-1;
    }
    else {
      iValue = SegmentDisplay[iSegIndex];
      ActivateSegment(SegmentDisplay[iSegIndex]);
      Serial.printf("Segment[%d]=0x%.2X\r\n", iSegIndex, iValue);
      //Serial.println(iValue, HEX); // Serial.println(c, HEX);
    }
    delay(500);
    bKeyPressed = false;
    bKeyUp      = false;
  }
  if (bKeyDn){
    // Enable display:
    digitalWrite(OE_Pin, LOW);

    // Increment height and show new value:
    iSegIndex--;
    if (iSegIndex<0) {
      iSegIndex=8;
      ActivateSegment(LED_a + LED_b + LED_c + LED_d + LED_e + LED_f + LED_g); // + LED_dp);
      Serial.println("Segment=8. (all)");
      iSegIndex=-1;
    }
    else {
      iValue = SegmentDisplay[iSegIndex];
      ActivateSegment(SegmentDisplay[iSegIndex]);
      Serial.printf("Segment[%d]=0x%.2X\r\n", iSegIndex, iValue);
      //Serial.println(iValue, HEX); // Serial.println(c, HEX);
    }
    delay(500);
    bKeyPressed = false;
    bKeyDn      = false;
  }
}

// Activate next segment:
// ---------------------
void IRAM_ATTR ISR_KeyUp() {
  // Avoid bouncing:
  if (bKeyPressed) return;
  bKeyPressed = true;
  bKeyDn = false;
  bKeyUp = true;
}


// Activate previous segment:
// ---------------------
void IRAM_ATTR ISR_KeyDn() {
  // Avoid bouncing:
  if (bKeyPressed) return;
  bKeyPressed = true;
  bKeyDn = true;
  bKeyUp = false;
}

void ActivateSegment(byte iSegNum){
  byte iDigit = 0;

  // setting OE as LOW enables the display:
  digitalWrite(OE_Pin, LOW);

  ShowOnDisplay(iSegNum);

  iDigit = Binarize (iSegIndex);
  ShowOnDisplay(iDigit);
}

int Binarize(byte iNumero) {
  byte iResult = 0;

  iResult = 0;

  // Finds out the binary to mach each LED segment for numbers from 0 to 9. Any other number shows "E":
  switch (iNumero) {
    case 1: {
      iResult = LED_b + LED_c;
      break;
    }
    case 2: {
      iResult = LED_a + LED_b + LED_g + LED_e + LED_d;
      //iResult = LED_b + LED_c + LED_g + LED_f + LED_e;
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
  Serial.printf("iNumero=%d iResult=%d\r\n", iNumero, iResult);
  return iResult;
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


// Activate one display segment:
// -----------------------------
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

