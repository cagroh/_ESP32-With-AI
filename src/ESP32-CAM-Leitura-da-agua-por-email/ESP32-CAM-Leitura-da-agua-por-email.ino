/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-cam-projects-ebook/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <ESP_Mail_Client.h>
#include "esp_sleep.h"
#include <FS.h>
#include <WiFi.h>
//#include "NTPClient.h"
#include <WiFiUdp.h>
#include <time.h>

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Ap703-2.4G";
const char* password = "lili121212";

// Define NTP Client to get timeFLASH_LED
//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);
#define NTP_Server_List "a.ntp.br, b.ntp.br, c.ntp.br, pool.ntp.org,time.nist.gov"
#define gmtOffset_sec     -10800
#define daylightOffset_sec  0
#define deep_sleep_secs     (24*60*60) // timer to wake-up and take a photo at each interval of seconds

// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
// You need to create an email app password
#define emailSenderAccount    "cognitiva.iot@gmail.com"
#define emailSenderPassword   "ctqy wpdj osni zgng"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
//#define emailSubject          "Leitura da Água"
#define emailRecipient        "cesar.groh@gmail.com"

#define VOLTMETER_GPIO_     14 // Usado para medir a tensão da bateria
#define FLASH_LED_GPIO      12 // Usado para iluminar - trocar por IR quando camera IR (850nm)
#define WATCH_LED_GPIO      13 // Usado para sinalizar tarefas
#define ERROR_LED_GPIO      15 // Usado para sinalizar erros

#define CAMERA_MODEL_AI_THINKER
   
camera_config_t config;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

//void startCameraServer();

// Photo File Name to save in LittleFS
#define FILE_PHOTO "leitura.jpg"
#define FILE_PHOTO_PATH "/leitura.jpg"

int iErrorCode = 0;
int iVoltsRead = 0;



void setup() {
  iErrorCode = 0;
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  Serial.begin(115200);
  Serial.println();

#ifdef VOLTMETER_GPIO
  pinMode (VOLTMETER_GPIO, INPUT);
  iVoltsRead = analogRead(VOLTMETER_GPIO);
#endif

  ConnectReconnectWiFi();
  // // Connect to Wi-Fi
  // WiFi.begin(ssid, password);
  // Serial.print("Connecting to WiFi...");
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println();
  
  // // Print ESP32 Local IP Address
  // Serial.print("IP Address: http://");
  // Serial.println(WiFi.localIP());

  // Init filesystem
  if (!ESP_MAIL_DEFAULT_FLASH_FS.begin()){
    //SinalizarErroNoLED (1);
    Serial.println("Setup-Erro 01: Inicialização do LittleFS falhou");
    iErrorCode = 1;
    return;
  }
   
  // camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // // Initialize camera
  // esp_err_t err = esp_camera_init(&config);
  // if (err != ESP_OK) {
  //   Serial.printf("Camera init failed with error 0x%x", err);
  //   return;
  // }
  
  // capturePhotoSaveLittleFS();
  // sendPhoto();

  // // Let's sleep until next morning
  // delay(86400000);

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, "a.ntp.br", "b.ntp.br", "c.ntp.br"); //NTP_Server_List);

  pinMode (ERROR_LED_GPIO, OUTPUT);
  pinMode (FLASH_LED_GPIO, OUTPUT);
  pinMode (WATCH_LED_GPIO, OUTPUT);

  // Test LEDs:
  digitalWrite (FLASH_LED_GPIO, HIGH);
  digitalWrite (WATCH_LED_GPIO, HIGH);
  digitalWrite (ERROR_LED_GPIO, HIGH);
  delay(1000);
  digitalWrite (FLASH_LED_GPIO, HIGH);
  digitalWrite (WATCH_LED_GPIO, HIGH);
  digitalWrite (ERROR_LED_GPIO, LOW);
  delay(1000);
  digitalWrite (FLASH_LED_GPIO, HIGH);
  digitalWrite (WATCH_LED_GPIO, LOW);
  digitalWrite (ERROR_LED_GPIO, LOW);
  delay(1000);
  digitalWrite (FLASH_LED_GPIO, LOW);
  digitalWrite (WATCH_LED_GPIO, LOW);
  digitalWrite (ERROR_LED_GPIO, LOW);
  delay(500);
  digitalWrite (FLASH_LED_GPIO, LOW);
  digitalWrite (WATCH_LED_GPIO, LOW);
  digitalWrite (ERROR_LED_GPIO, HIGH);
  delay(500);
  digitalWrite (FLASH_LED_GPIO, LOW);
  digitalWrite (WATCH_LED_GPIO, HIGH);
  digitalWrite (ERROR_LED_GPIO, LOW);
  delay(500);
  digitalWrite (FLASH_LED_GPIO, HIGH);
  digitalWrite (WATCH_LED_GPIO, LOW);
  digitalWrite (ERROR_LED_GPIO, LOW);
  delay(500);
  digitalWrite (FLASH_LED_GPIO, LOW);
  digitalWrite (WATCH_LED_GPIO, LOW);
  digitalWrite (ERROR_LED_GPIO, LOW);
  delay(500);

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Setup-Erro 02: Inicialização da câmera falhou, erro 0x%x\n", err);
    iErrorCode = 2;
    return;
  }
  //startCameraServer();
  Serial.println ();
  Serial.println ("Ready!");
}





void loop() {
  String sResult;
  //const int deep_sleep_sec = 10;
  //ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
  // esp_deep_sleep(1000000LL * deep_sleep_sec);
  if (WiFi.status() != WL_CONNECTED) {
    ConnectReconnectWiFi();
  }

  sResult = capturePhotoSaveLittleFS();
  sendMail(sResult);
  if (sResult != "OK") {
    Serial.println("Loop-Erro 03: Captura da foto falhou");
    iErrorCode = 3;
    return;
    // delay(1000);
    // ESP.restart();
  }

  if (iErrorCode > 0) {
    SinalizarErroNoLED (iErrorCode);
  }

// Let's sleep until it's time for the next picture:
  digitalWrite (FLASH_LED_GPIO, LOW);
  digitalWrite (WATCH_LED_GPIO, LOW);
  digitalWrite (ERROR_LED_GPIO, LOW);
  Serial.printf("Indo dormir profundamente por %d segundos\n", deep_sleep_secs);
  esp_deep_sleep(1000000LL * deep_sleep_secs);  
}




void SinalizarErroNoLED(int iError){
  //pinMode (ERROR_LED_GPIO, OUTPUT);
  for (int i=0; i < iError; i++) {
    PiscaLED();
  }
  delay (2000);
}



void PiscaLED(){
    digitalWrite(ERROR_LED_GPIO, HIGH);
    delay (500);
    digitalWrite(ERROR_LED_GPIO, LOW);
    delay (500);
}



void ConnectReconnectWiFi(){
  // Connect to Wi-Fi
  unsigned int milissegundos = 5000;

  Serial.println();
  Serial.print("Connecting to WiFi...");

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WATCH_LED_GPIO, HIGH);
    delay(milissegundos);
    digitalWrite(WATCH_LED_GPIO, LOW);
    delay(500);
    milissegundos = milissegundos * 3;
    if (milissegundos>10935000) {
      milissegundos = 5000;
    }
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  
  // Print ESP32 Local IP Address:
  //Serial.print("IP Address: http://");
  //Serial.println(WiFi.localIP());
}




// Capture Photo and Save it to LittleFS
String capturePhotoSaveLittleFS(){
  String sResult = "";
  unsigned int iBytesWritten = 0;

 // Turn LEDs ON:
 //pinMode (WATCH_LED_GPIO, OUTPUT);
  digitalWrite(FLASH_LED_GPIO,   HIGH);
  //digitalWrite(WATCH_LED_GPIO, HIGH);
  //WATCH_LED_GPIO)

  // Dispose first pictures because of bad quality
  camera_fb_t *fb = NULL;

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  //s->set_vflip(s, 1); // flip it back
  s->set_denoise(s, 5);
  s->set_brightness(s,  3); // up the brightness just a bit
  s->set_saturation(s, -2); // lower the saturation

  // Skip first 3 frames (increase/decrease number as needed).
  for (int i = 0; i < 3; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
    
  // Take a new photo
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if (!fb) {
    sResult = "CapturePhoto-Erro 04: Captura da foto falhou";
    iErrorCode = 4;
    Serial.println(sResult);
    return sResult;
    //delay(1000);
    //ESP.restart();
  }  

  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

  // Insert the data in the photo file
  if (!file) {
    sResult = "CapturePhoto-Erro 05: Falha ao abrir o arquivo para escrita";
    Serial.println(sResult);
    iErrorCode = 5;
    return sResult;
  }
  else {
    iBytesWritten = file.write(fb->buf, fb->len); // payload (image), payload length
    if (iBytesWritten != fb->len) {
      sResult = "CapturePhoto-Erro 06: Falha ao gravar a foto";
      Serial.println(sResult);
      iErrorCode = 6;
      return sResult;
    }
    // Serial.print("The picture has been saved in ");
    // Serial.print(FILE_PHOTO_PATH);
    // Serial.print(" - Size: ");
    // Serial.print(fb->len);
    // Serial.println(" bytes");
  }
  
    // GPIO_MODE_DISABLE          /*!< GPIO mode : disable input and output             */
    // GPIO_MODE_INPUT            /*!< GPIO mode : input only                           */
    // GPIO_MODE_OUTPUT           /*!< GPIO mode : output only mode                     */
    // GPIO_MODE_OUTPUT_OD        /*!< GPIO mode : output only with open-drain mode     */
    // GPIO_MODE_INPUT_OUTPUT_OD  /*!< GPIO mode : output and input with open-drain mode*/
    // GPIO_MODE_INPUT_OUTPUT     /*!< GPIO mode : output and input mode                */

  // Turn LEDs OFF:
  digitalWrite(FLASH_LED_GPIO,   LOW);
  //digitalWrite(WATCH_LED_GPIO, LOW);
  //pinMode(WATCH_LED_GPIO, GPIO_MODE_DISABLE);
  //pinMode(FLASH_LED_GPIO,   GPIO_MODE_DISABLE);

  // Close the file:
  file.close();

  // Free the buffer:
  esp_camera_fb_return(fb);
  return "OK";
}




void sendMail(String sMessage) {
  struct tm timeinfo;
  String emailSubject = "";
  String sDateTime    = "";
  String htmlMsg = "";

  digitalWrite (WATCH_LED_GPIO, HIGH);

  Serial.println("Enviando email:");
  if (sMessage == "OK") {
    emailSubject = "Leitura da Água";
    htmlMsg      = "<h2>Foto do hidrômetro anexada à esta mensagem.</h2><br><br><br><h3>IP: " + String(WiFi.localIP()) + "<br>AP: " + String(ssid) + "<br></h3>";
  }
  else {
    emailSubject = "ERRO ao tentar fazer a Leitura da Água";
    htmlMsg      = "<h2>" + sMessage + "</h2><br><br><br><h3>IP: " + String(WiFi.localIP()) + "<br>AP: " + String(ssid) + "<br></h3>";
  }

#ifdef VOLTMETER_GPIO
  //htmlMsg = htmlMsg + "<br><br>Battery reads " + String(iVoltsRead) + " (please divide by 1240 to obtain proportion of 3V3)";
  //htmlMsg = htmlMsg + "<br><br>Battery reads " + String(iVoltsRead) + " (" + String(double(iVoltsRead)/1240.0) + "V)";
  //htmlMsg = htmlMsg + "<br><br>Battery reads " + String(iVoltsRead) + " (" + String(map(float(iVoltsRead), 0.0f, 4095.0f, 0, 100)) + "V)";
#endif

  Serial.println(sMessage);
  Serial.println(emailSubject);

  if (!getLocalTime(&timeinfo)) {
    sDateTime = "NTP-Err ";
    //return;
  }
  else {
    sDateTime = String (timeinfo.tm_mday) + "/" + String (timeinfo.tm_mon) + "/" + String (timeinfo.tm_year) + " " + String (timeinfo.tm_hour) + ":" + String (timeinfo.tm_min) + ":" + String (timeinfo.tm_sec) + " ";
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); 

  /** Enable the debug via Serial port
   * none  or 0 = no debug
   * basic or 1 = basic debug
  */
  smtp.debug(0);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  Session_Config config;
  
  /*Set the NTP config time
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  config.time.ntp_server = F(NTP_Server_List); //"a.ntp.br, b.ntp.br, c.ntp.br, pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 15;
  config.time.day_light_offset = 0;

  /* Set the session config */
  config.server.host_name = smtpServer;
  config.server.port = smtpServerPort;
  config.login.email = emailSenderAccount;
  config.login.password = emailSenderPassword;
  config.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Enable the chunked data transfer with pipelining for large message if server supported */
  message.enable.chunking = true;

  /* Set the message headers */
  message.sender.name = "Cognitiva IoT Device";
  message.sender.email = emailSenderAccount;
  emailSubject = emailSubject + " em " + sDateTime;
  message.subject = MB_String(emailSubject);
  message.addRecipient("Assinante", emailRecipient);

  // String htmlMsg = "<h2>Photo captured with ESP32-CAM and attached in this email.</h2>";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* The attachment data item */
  SMTP_Attachment att;

  /** Set the attachment info e.g. 
   * file name, MIME type, file path, file storage type,
   * transfer encoding and content encoding
  */
  att.descr.filename = FILE_PHOTO;
  att.descr.mime = "image/png"; 
  att.file.path = FILE_PHOTO_PATH;
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add attachment to the message */
  message.addAttachment(att);

  /* Connect to server with the session config */
  if (!smtp.connect(&config)) {
    Serial.println("SendMail-Erro 07: Erro ao conectar ao servidor de Email: " + smtp.errorReason());
    iErrorCode = 7;
    return;
  }

  /* Start sending the Email and close the session */
  if (!MailClient.sendMail(&smtp, &message, true)) {
    Serial.println("SendMail-Erro 08: Erro ao enviar o Email: " + smtp.errorReason());
    iErrorCode = 8;
    return;
  }
  
  digitalWrite (WATCH_LED_GPIO, LOW);
}



// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  //Serial.println(status.info());

  digitalWrite (WATCH_LED_GPIO, HIGH);

  /* Print the sending result */
  if (status.success()) {
    // Serial.println("----------------");
    // Serial.printf("Message sent success: %d\n", status.completedCount());
    // Serial.printf("Message sent failled: %d\n", status.failedCount());
    // Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      // ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      // ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      // ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      // ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      // ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    //Serial.println("----------------\n");

   // You need to clear sending result as the memory usage will grow up.
   smtp.sendingResult.clear();
   digitalWrite (WATCH_LED_GPIO, LOW);
  }
  // else {
  //   Serial.println("SendMail-Erro 09: Erro indefinido ao enviar o Email: " + String(status.info()));
  //   iErrorCode = 9;
  //   return;
  // }
}

