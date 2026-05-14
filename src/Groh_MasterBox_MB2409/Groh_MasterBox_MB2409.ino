//*************************************************************************************************************************************************************************************************************************************
// COGИITIVA GROH MB-2409
// - ESP32-based
// - ST7735 1.8" Display
// - LoRa radio
// - Inteligent Client Recognition
// - Groh COGИET exclusive auto addressing
//*************************************************************************************************************************************************************************************************************************************//


//*************************************************************************************************************************************************************************************************************************************//
// Device definitions:
//*************************************************************************************************************************************************************************************************************************************//


#define GrohDeviceModel "MB2409"
#define GrohDeviceSerial 001


//*************************************************************************************************************************************************************************************************************************************//
// Libraries:
//*************************************************************************************************************************************************************************************************************************************//


#include <Preferences.h>
#include <SPI.h>
#include <LoRa.h>     // LoRa driver by Sandeep Mistry
#include <TFT_eSPI.h> // ST7735 TFT driver by Bodmer


//*************************************************************************************************************************************************************************************************************************************//
// Constants:
//*************************************************************************************************************************************************************************************************************************************//


// Comment to disable debug mode
#define _debug_mode 1

// Define pins used by LoRa on SPI mode:
#define LORA_NSS   27 // same as CS
#define LORA_DIO0  17 // data in
#define LORA_NRST  16 // same as RST

// Define pins used by ST7735 Display on HSPI mode:
#define TFT_MOSI 13  // SDA
#define TFT_SCLK 14  // SCL
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)

// Define status LED pin:
#define StatusLedPin 25

// TIme-Out limits:
#define ClientTimeOut 10000  // 10 seconds (in millisseconds)
#define LoRaTimeOut    3000  //  3 seconds (in millisseconds)


//*************************************************************************************************************************************************************************************************************************************//
// Global Variables:
//*************************************************************************************************************************************************************************************************************************************//


// New Client Detected indicator:
bool bNewClient    = false;
// LoRa timed out indicator:
bool bLoraTimedOut = false;

// Keyboard time-out variables:
unsigned long uCurrMillis = 0.0;
unsigned long uStopMillis = 0.0;

// Instantiate TFT_eSPI object:
TFT_eSPI TFT_Display = TFT_eSPI();


//*************************************************************************************************************************************************************************************************************************************//
// Forward declarations:
//*************************************************************************************************************************************************************************************************************************************//
void Get_LoRa_Packet();
bool Enable_LoRa();
void Lora_Has_Timed_Out();
void BlinkActivity(int times);


//*************************************************************************************************************************************************************************************************************************************//
// Initial setup:
//*************************************************************************************************************************************************************************************************************************************//
void setup() {
  // Initialize Serial Monitor and wait for it to become operative:
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Initializing TFT display");
  Serial.flush();

  // Initialize ST7735S 1.8" TFT display and show ID data:
  TFT_Display.init();
  TFT_Display.setCursor(0, 0);
  TFT_Display.fillScreen(TFT_BLACK);
  TFT_Display.setRotation(1);
  TFT_Display.setTextSize(2);
  TFT_Display.setTextColor(TFT_CYAN);
  TFT_Display.println("COGNITIVA IoT");
  TFT_Display.setTextSize(1);
  TFT_Display.setTextColor(TFT_WHITE);
  TFT_Display.println();
  TFT_Display.printf("%s SN: %d pronto!", GrohDeviceModel, GrohDeviceSerial);
  TFT_Display.println();

  Serial.println("TFT display OK!");
  Serial.flush();

  // Setup Status LED pin:
  pinMode     (StatusLedPin, OUTPUT);
  digitalWrite(StatusLedPin, LOW);

  //Setup LoRa pins:
  LoRa.setPins(LORA_NSS, LORA_NRST, LORA_DIO0);
  
  Serial.println("Initializing LoRa");

  // Prepare to use LoRa:
  bLoraTimedOut = false;
  uStopMillis   = -1;
  TFT_Display.println("Initializing LoRa radio...");

  // Try to enable LoRa's radio on 915MHz:
  while (!Enable_LoRa) {
    if (bLoraTimedOut) {
      Lora_Has_Timed_Out();
    }
    TFT_Display.print(".");
    delay(500);
  }
 
  // All set!
  TFT_Display.println();
  TFT_Display.println("LoRa OK!");
  Serial.println("LoRa OK!");
  Serial.flush();
  LoRa.end();
}


//*************************************************************************************************************************************************************************************************************************************//
// Main loop:
//*************************************************************************************************************************************************************************************************************************************//
void loop() {
  BlinkActivity(1);
  delay(500);

  Get_LoRa_Packet();
}


//*************************************************************************************************************************************************************************************************************************************//
// Additional functions:
//*************************************************************************************************************************************************************************************************************************************//


// Enable the LoRa radio:
// ----------------------
bool Enable_LoRa(){
  bool bResult = false;

  // Initialize timeout counter:
  if (uStopMillis == -1){
    uStopMillis = millis() + LoRaTimeOut;
  }

  bLoraTimedOut = millis() >= LoRaTimeOut;
  
  bResult = (LoRa.begin(915E6));
}


// Show error message and halt application due to LoRa time-out:
// -------------------------------------------------------------
void Lora_Has_Timed_Out(){
    // If the radio has timed-out, can't continue!
    TFT_Display.println();
    TFT_Display.setTextSize(2);
    TFT_Display.setTextColor(TFT_RED);
    TFT_Display.print("ERRO CRÍTICO 01: ");
    TFT_Display.setTextSize(2);
    TFT_Display.println("Rádio LoRa da unidade mestre (este equipamento) inoperante!");
    TFT_Display.println("Solicite manutenção ou substituição!");
    digitalWrite(StatusLedPin, HIGH);
    while (true){}
}


// Get a packet from the LoRa radio readout:
// -----------------------------------------
void Get_LoRa_Packet(){
  // Try to enable LoRa's radio on 915MHz:
  while (!Enable_LoRa) {
    if (bLoraTimedOut) {
      Lora_Has_Timed_Out();
    }
    TFT_Display.print(".");
    delay(500);
  }

  // All Cognitiva/Groh devices sync with 0x47 ('G'):
  LoRa.setSyncWord(0x47);
  
  // Try to get and parse a LoRa data packet:
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Get the packet string:
    while (LoRa.available()) {
      String LoRaData = LoRa.readString();
      TFT_Display.printf ("[%s]\n\r", LoRaData);
    }

    // print RSSI (Received Signal Strength Indicator) of the packet:
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
  LoRa.end();
}


// Blink the Status LED:
// ---------------------
void BlinkActivity(int times){
  for(int x=0;x<times;x++){
    digitalWrite (StatusLedPin, HIGH);	// turn on the LED
    delay(250);	// wait for half a second or 500 milliseconds
    digitalWrite (StatusLedPin, LOW);	// turn off the LED
    delay(250);	// wait for half a second or 500 milliseconds
  }
}