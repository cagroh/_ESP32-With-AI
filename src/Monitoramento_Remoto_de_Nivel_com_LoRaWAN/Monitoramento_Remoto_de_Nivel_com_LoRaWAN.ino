// https://www.newtoncbraga.com.br/iot/19965-monitoramento-remoto-de-nivel-com-lorawan-mic572.html
#include <Ultrasonic.h>

/* Definições - sensor ultrassônico */ 
#define TRIGGER_PIN  26
#define ECHO_PIN     25

/* Definições - serial para módulo LoRaWAN */
#define UART_LORAWAN_BAUDRATE           9600
#define UART_LORAWAN_RX                 14
#define UART_LORAWAN_TX                 13
#define TAM_NWSKEY_APPSKEY              60
#define TAM_APPEUI                      30
#define TAM_DEVADDR                     15

 
/* Definições - parâmetros de medição de distância */ 
#define NUM_MEDIDAS_DISTANCIA           10
#define ALTURA_TOTAL_SENSOR_ATE_FUNDO   200 //cm
#define DISTANCIA_SENSOR_RESERVATORIO   30 //cm
 
/* Definições - debug */
#define MOSTRA_DEBUG
#define NUM_LINHAS_PARA_PULAS   80

/* Credenciais LoRaWAN */
const char nwskey[TAM_NWSKEY_APPSKEY] = "NNNNNNNNNN\0";   // Network session key
const char appskey[TAM_NWSKEY_APPSKEY] = "AAAAAAAAAA\0";  // Application session key
const char appeui[TAM_APPEUI] = "AAAAAAAAAA\0";           // Application EUI
const char devaddress[TAM_DEVADDR] = "EEEEEEEEEE\0";      // Device Address

//variaveis e objetos globais
Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
unsigned long timestamp_envio = 0;

//prototypes
int mede_distancia_centimetros(void);
int calcula_media_distancias(void);
 
/*
 * Implementações
 */
 
 /*
  * Função: Média de medidas de distância
  * Parâmetros: nenhum
  * Retorno: média das medidas de distância
  */
int calcula_media_distancias(void)
{
    int i;
    int soma_medidas;
    int media;
 
    soma_medidas = 0.0;
    for(i=0; i<NUM_MEDIDAS_DISTANCIA; i++)
        soma_medidas = soma_medidas + mede_distancia_centimetros();     
 
    media = (soma_medidas / NUM_MEDIDAS_DISTANCIA);    
    return media;    
}
  
 /*
  * Função: mede distancia em centímetros
  * Parametros: nenhum
  * Retorno: distancia (cm)
 */
int mede_distancia_centimetros(void) 
{
    int cmMsec;
    long microsec = ultrasonic.timing();
 
    cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);
    
    #ifdef MOSTRA_DEBUG
        Serial.println("[DADOS DO SENSOR]");
        Serial.print("Tempo(ms): ");
        Serial.print(microsec);
        Serial.print(", Distancia(cm): ");
        Serial.print(cmMsec);
    #endif

    return cmMsec; 
}

 /*
  * Função: envia comando AT para módulo LoRaWAN
  * Parametros: ponteiro para comando AT a ser enviado e tamanho
  *             do comando
  * Retorno: nenhum
 */
void envia_comando_at(char * pt_cmd_at, int tamanho)
{
    Serial2.write(pt_cmd_at, tamanho);
    delay(1000);

    Serial.print("Comando enviado: ");
    Serial.println(pt_cmd_at);
    Serial.println("Resposta: ");
    
    while(Serial2.available())
    {
        Serial.print(Serial2.read());
    }

    Serial.println("");
}
 
void setup()
{
    char comando_at[200] = {0};
    int tam_comando;

    Serial.begin(115200);
    Serial.println("Dispositivo iniciado");

    /* Configura módulo LoRaWAN */
    Serial.println("Configurando modulo LoRaWAN...");
    Serial2.begin(UART_LORAWAN_BAUDRATE, SERIAL_8N1, UART_LORAWAN_RX, UART_LORAWAN_TX);

    /* Envio do channel mask */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+CHMASK=00FF:0000:0000:0000:0000:0000\n\r");
    envia_comando_at(comando_at, strlen(comando_at));

    /* Envio do join mode */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+NJM=0\n\r");
    envia_comando_at(comando_at, strlen(comando_at));

    /* Envio do device address */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+DADDR=%s\n\r", devaddress);
    envia_comando_at(comando_at, strlen(comando_at));

    /* Envio do application EUI */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+APPEUI=%s\n\r", appeui);
    envia_comando_at(comando_at, strlen(comando_at));

    /* Envio do application session key */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+APPSKEY=%s\n\r", appskey);
    envia_comando_at(comando_at, strlen(comando_at));

    /* Envio do network session key */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+NWKSKEY=%s\n\r", nwskey);
    envia_comando_at(comando_at, strlen(comando_at));

    /* Desabilita ADR (Automatic Data Rate) */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+ADR=0\n\r");
    envia_comando_at(comando_at, strlen(comando_at));

    /* Configura Data Rate e Spread Factor para máximo alcance 
       e menor payload. Aqui, o consumo do módulo é o maior 
       possível, porém a chance do payload chegar ao gateway é
       significativamente maior. 
    */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+DR=0\n\r");
    envia_comando_at(comando_at, strlen(comando_at));

    /* Configura classe A para o LoRaWAN */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+CLASS=A\n\r");
    envia_comando_at(comando_at, strlen(comando_at));

    /* Liga confirmação de envio */
    memset(comando_at, 0x00, sizeof(comando_at));
    snprintf(comando_at, sizeof(comando_at), "AT+CFM=1\n\r");
    envia_comando_at(comando_at, strlen(comando_at));

    Serial.println("Modulo LoRaWAN configurado!");
    timestamp_envio = millis();
}
 
void loop()
{
    int distancia_cm;
    int nivel_calculado;
    char comando_at[200] = {0};
    int tam_comando;
    int i;

    /* faz as mmedição da distância (considerando a media) e 
       calcula nível
    */
    distancia_cm = calcula_media_distancias();
    nivel_calculado = ALTURA_TOTAL_SENSOR_ATE_FUNDO - (distancia_cm - DISTANCIA_SENSOR_RESERVATORIO);

    for (i=0; i<NUM_LINHAS_PARA_PULAS; i++)
        Serial.println(""); 
      
    #ifdef MOSTRA_DEBUG  
        Serial.println("[DADOS DA MEDICAO]");
        Serial.print("Medida: ");
        Serial.print(distancia_cm);
        Serial.println("cm");
        Serial.print("Nuvel: ");
        Serial.print(nivel_calculado);
        Serial.print("cm");
    #endif

    /* Envia, de hora em hora, o nivel medido */
    if ( (millis() - timestamp_envio) >= 3600000 )
    {
        Serial.println("Enviando nivel...");
        memset(comando_at, 0x00, sizeof(comando_at));
        snprintf(comando_at, sizeof(comando_at), "AT+SEND=%d:%d\n\r", nivel_calculado);
        envia_comando_at(comando_at, strlen(comando_at));
        Serial.println("Nivel enviado!");
        timestamp_envio = millis();
    }
        
    delay(5000);
}

