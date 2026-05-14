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

#define _debug_mode 1

#include <EEPROM.h>
#include <SPI.h>
#include <LoRa.h>

#include <Groh_Display_7_segmentos.h>

// Definir os pinos usados pelo módulo LoRa no modo SPI:
#define CSS          25 //  5
#define DIO0         32 // 13
#define RST          33 // 14


// Definir pinos para os LEDs de erro e status:
#define ErrLedPin     2
#define StsLedPin    21

// Definir os pinos do sonar:
#define SonTrgrPin   13 // 12
#define SonEchoPin   15 // 13

// Definir o pino do botão de calibragem:
#define ButUpPin    32
#define ButDnPin    33

// Definir o modelo e o número de série:
#define device_model  "GSL-2408"
#define device_serial "27001"

// Definir o tempo de time-out:
#define time_out 10

// Definir tempo de espera antes de nova leitura (em milisegundos):
#define rest_time 2000 //(1000 * 60 * 5)

// Definir o gap default:
#define DefaultSonarGap 5

// Definir o tamanho do recipiente (em cm):
int   MaxDepth        = 400;
float fMaxDepth       = MaxDepth / 100;

// Definir variáveis auxiliares para interrupções por teclas:
bool Interrupted      = false;
bool InterruptedKeyDn = false;
bool InterruptedKeyUp = false;

// Variáveis globais:
// - Percentual de completude com base o valor medido / valor calibrado:
float Percentual = 0;

// Contador de time-out:
int   time_out_counter = 0;

// Distância default entre o sonar e o nível máximo do recipiente (em cm):
byte  SonarGap   = DefaultSonarGap;

// Auxiliares para computar tempo de botão pressionado:
unsigned long millisStarted;
unsigned long millisIncurred;

// Funções:
void DisplayMaxDepth();
void _debug(String literal);
void _debugln(String literal);
void reCalibrateMinDepth();
void EnviarDadosViaLoRa(float percent = 0);
void DisplayError(unsigned int ErrCode);

void IRAM_ATTR intButtonDn();
void IRAM_ATTR intButtonUp();

// *************************************************************************************************************************************************************************************************************************************
// Ajustes iniciais:
// *************************************************************************************************************************************************************************************************************************************
void setup() {
  Interrupted = false;

#ifdef _debug_mode
  Serial.begin(115200);
#else
#endif 

  pinMode(ButDnPin, INPUT_PULLUP);
  pinMode(ButUpPin, INPUT_PULLUP);
 
  attachInterrupt(ButDnPin, intButtonDn, FALLING);
  attachInterrupt(ButUpPin, intButtonUp, FALLING);

  // Initializar pinos:

  pinMode (StsLedPin,  OUTPUT);
  pinMode (ErrLedPin,  OUTPUT);
  // pinMode (SonTrgrPin, OUTPUT);
  // pinMode (SonEchoPin, INPUT);
  // pinMode (ButDnPin,   INPUT);
  // pinMode (ButUpPin,   INPUT);

  _debugln("Pinos inicializados");

  // Desligar todos os LEDs, display e botões:
  digitalWrite(StsLedPin,  HIGH);
  digitalWrite(ErrLedPin,  LOW);
  // digitalWrite(ButDnPin,   LOW);
  // digitalWrite(ButUpPin,   LOW);
  // digitalWrite(SonTrgrPin, LOW);
  // digitalWrite(SonEchoPin, LOW);
  clear_display();

  _debugln("LEDs inicializados");

  // Piscar o status 3 vezes para indicar que o dispositivo está inicializando:
  //BlinkActivity (3);

  // Obter o gap entre o sonar e o nível máximo do recipiente:
  SonarGap = EEPROM.read(0x00);

  _debug("Calibragem recuperada: ");
  _debugln(String(SonarGap));

  // Obter a altura do recipiente:
  MaxDepth  = EEPROM.read(0x01);
  fMaxDepth = MaxDepth / 100;

  _debug("Altura recuperada: ");
  _debugln(String(MaxDepth));

  // Apontar para o default caso a leitura seja abaixo do default:
  if (SonarGap < DefaultSonarGap) {
    SonarGap = DefaultSonarGap;

    // Recalibrar e salvar:
    reCalibrateMinDepth();   

    _debugln("Calibragem mudada para default");
  }

  // Apresentar a altura atual:
  DisplayMaxDepth();
  delay(5000);
  clear_display();

  // Zerar o contador do time-out:
  time_out_counter = 0;
  
  _debugln("Setup inicial concluído");

  millisStarted = millis();
  millisIncurred = -1;
}



// *************************************************************************************************************************************************************************************************************************************
// Loop principal:
// *************************************************************************************************************************************************************************************************************************************
void loop() {
  float distanciacm;

  if (Interrupted){
    // Exibir a profundidade do recipiente:
    DisplayMaxDepth();

    // Acender o LED de status:
    pinMode(StsLedPin, OUTPUT);
    digitalWrite(StsLedPin, HIGH); 
    
    // Aumentar ou diminuir MaxDepth de acordo com a tecla pressionada:
    if (InterruptedKeyDn){
      MaxDepth = MaxDepth - 10;
    }
    else {
      MaxDepth = MaxDepth + 10;
    }

    // Manter MaxDepth dentro dos limites:
    if (MaxDepth <  40) MaxDepth = 40;
    if (MaxDepth > 400) MaxDepth = 400;

    DisplayMaxDepth();

    // Validar tempo decorrido:
    millisIncurred = millis();
    if ((millisIncurred - millisStarted) > 3000){

      // Resetar tempo decorrido:
      millisIncurred = -1;
      millisStarted  =  0;

      // Desligar interrupção:
      Interrupted = false;

      // Apagar o LED de status:
      BlinkActivity (2); 

      // Recalibrar posição do sonar em relação ao recipiente cheio e salvar novas medidas do recipiente:
      reCalibrateMinDepth();
    }
  }
  else {
    _debugln("Acionando Sonar");

    // Medir:
    distanciacm = GetAverageMeasures(3);

    // Calcular o percentual:
    Percentual = (1 - ((distanciacm - SonarGap) / MaxDepth)) * 100;

    _debug("Percentual obtid0: ");
    _debugln(String(Percentual));

    // Piscar o LED de status:
    BlinkActivity (1);

    EnviarDadosViaLoRa(Percentual);

    // Zerar o contador do time-out:
    time_out_counter = 0;

    // Entrar em compasso de espera:
    while (time_out_counter < rest_time){
      // Piscar o LED de status:
      BlinkActivity (1); 

      // Manter apagado por 500ms:
      delay(500);

      // Somar 1:
      time_out_counter++;
    }
  }

  // Descansar alguns segundos:
  delay (5000);
}


void EnviarDadosViaLoRa(float percent){

  // Inicializar pinos do LoRa:
  LoRa.setPins(CSS, RST, DIO0);

  _debugln("Pinos LoRa inicializados ");
  
  // Zerar o contador do time-out:
  time_out_counter = 0;

  // Tentar conectar no módulo LoRa:
  while (!LoRa.begin(915E6)) {

    _debugln("LoRa Begin ");

    // Checar se estourou o time-out:
    if (time_out_counter >= time_out){

      // Acender LED de erro:
      digitalWrite (ErrLedPin, HIGH);
      DisplayError(1);

      _debugln("LoRa Timed-out!");

      // Entrar em loop:
      while (1){}
    }
    // Senão:
    else {

      // Somar 1:
      time_out_counter++;

      // Piscar o LED de Erro:
      BlinkError (1);
    }
  }

  // Todo dispositivo Groh deve ser setado para 0x47 ('G'):
  LoRa.setSyncWord(0x47);

  // Preparar para enviar pelo LoRa:
  LoRa.beginPacket();

  _debugln("Preparando LoRa para enviar ");

  // Enviar os dados:
  LoRa.print (device_model);

  _debugln("-Modelo enviado ");

  LoRa.print (device_serial);

  _debugln("-Serial enviado ");

  LoRa.print(percent);

  _debugln("-Percentual enviado");
  
  // Finalizar o pacote LoRa:
  LoRa.endPacket();

  // Finalizar o rádio LoRa:
  LoRa.end();

  _debugln("Envios finalizados");

}

// Obter a média de 'n' leituras do sonar:
float GetAverageMeasures(int times){
  // Variáveis locais:
  float prev  = -1;
  float curr  =  0;
  float total =  0;
  int   x;
  
  // Inicializar o contador:
  x = times;

  // Zerar o time-out:
  time_out_counter = 0;

  _debugln("Obtendo leituras do sonar ");

  // Loop para obter o número de leituras:
  while (x > 0){
    // Se estourou o timed-out, liga o LED de erro e fica em loop:
    if (time_out_counter >= time_out){

      // Acender o LED de erro:
      digitalWrite (ErrLedPin, HIGH);
      DisplayError(2);

      _debugln("Sonar timed-out!");

      // Ficar em loop:
      while (1){
      }
    }
    // Caso contrário:
    else {
      // Incrementar o contador de time-out:
      time_out_counter++;

      // Piscar o LED de Erro:
      BlinkError (1);
    }

    // Obter uma medida do sonar:
    curr = SonarPing();

    _debug("Valor lido:");
    _debugln(String(curr));

    // Leitura válida?
    if (curr > 0){

      // É a primeira leitura?
      if (prev < 0){
        // Equalizar primeira leitura:
        prev = curr;
      }

      // Ignorar leituras que variam muito (>20cm):
      if  (abs(curr -  prev) < 20){
        x--;
        prev  = curr;
        total = total + curr;
      }
    }
  }

  // Retornar a média das leituras:
  return (total / times);
}


// Pingar o sonar e medir o tempo do echo e a distância percorrida (em cm):
float SonarPing(){
  
  _debugln("Efetuando ping ");

  // Variáveis locais:
  long  duracao   = 0;
  float distancia = 0;

  // Desligar o trigger:
  digitalWrite(SonTrgrPin, LOW); 

  // Aguardar 2 microsegundos:   
  delayMicroseconds(2);          
 
  // Acionar o trigger: 
  digitalWrite(SonTrgrPin, HIGH);  

  // Aguardar 10 microsegundos: 
  delayMicroseconds(10);

  // Desligar o trigger:         
  digitalWrite(SonTrgrPin, LOW);    
   
  // Ler o echo e retornar a onda de som em microsegundos:
  duracao = pulseIn(SonEchoPin, HIGH);

  _debug("eco obtido: ");
  _debugln(String(duracao));

  // Calcular a distância com base na fórmula da velocidade do som (340m/s):
  distancia = duracao * 0.034 / 2;

  _debug("DISTÂNCIA: ");
  _debugln(String(distancia));
  // devolver o valor obtido:
  return distancia;
}


// Piscar o LED de atividade (cada piscada do LED dura 500ms):
void BlinkActivity(int times){
  // Setar o modo do pino do LED de status:
  pinMode (StsLedPin,  OUTPUT);

  // Lopp executado 'times' vezes:
  for(int x=0;x<times;x++){

    // Acender o LED de status:
    digitalWrite (StsLedPin, HIGH);

    // Manter aceso por 250ms:
    delay(250);

    // Apagar o LED de status:
    digitalWrite (StsLedPin, LOW);

    // Manter apagado por 250ms:
    delay(250);
  }
}

// Piscar o LED de ERRO (cada piscada do LED dura 500ms):
void BlinkError(int times){
  // Setar o modo do pino do LED de status:
  pinMode (ErrLedPin,  OUTPUT);

  // Lopp executado 'times' vezes:
  for(int x=0;x<times;x++){

    // Acender o LED de status:
    digitalWrite (ErrLedPin, HIGH);

    // Manter aceso por 250ms:
    delay(250);

    // Apagar o LED de status:
    digitalWrite (ErrLedPin, LOW);

    // Manter apagado por 250ms:
    delay(250);
  }
}

void DisplayError(unsigned int ErrCode){
  display_number(1, 10);
  decimal_off();
  display_number(2, ErrCode);
}

void _debug(String literal){
#ifdef _debug_mode
  Serial.print(literal);
#else
#endif
}

void _debugln(String literal){
#ifdef _debug_mode
  Serial.println(literal);
#else
#endif
}

void DisplayMaxDepth(){
  int inteiro;
  int decimal;
  fMaxDepth = MaxDepth / 100;

  // if (fMaxDepth >= 1){
  //   inteiro = int(fMaxDepth);
  //   decimal = int ((fMaxDepth - inteiro) * 10);
  // }

  inteiro = int(fMaxDepth);
  decimal = int ((fMaxDepth - inteiro) * 10);

  display_number(1, inteiro);
  decimal_on();
  display_number(2, decimal);
}


void IRAM_ATTR intButtonDn(){
  if (millisIncurred == -1){
    millisStarted  = millis();
    millisIncurred = millisStarted;
  }
  InterruptedKeyDn = true;
  InterruptedKeyUp = false;
  Interrupted      = true;
}

void IRAM_ATTR intButtonUp(){
  if (millisIncurred == -1){
    millisStarted  = millis();
    millisIncurred = millisStarted;
  }
  InterruptedKeyDn = false;
  InterruptedKeyUp = true;
  Interrupted      = true;
}


void reCalibrateMinDepth(){
  _debugln("Obtendo calibragem");

  // Obter a média de 4 leituras:
  SonarGap = GetAverageMeasures(4);

  _debug("Calibragem obtida: ");
  _debugln(String(SonarGap));

  // Gravar o valor do gap na EEPROM:
  EEPROM.write(0x00, SonarGap);

  // Gravar o valor de MaxDepth na EEPROM:
  EEPROM.write(0x01, MaxDepth);

  // Salvar:
  EEPROM.commit();

  _debugln("Calibragem salva");

}