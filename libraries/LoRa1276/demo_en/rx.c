// This code run in the receiver
// The parameter is LORA,434MHz£,signal bandwidth=62.5kHz, spreading factor=9,coding rate=4/5
// The tx packet： “send_test”
#include "sfr_r81b.h"
#include "string.h"



// IO definition
#define  	LED_TX			p1_0	// output，led of tx
#define  	LED_RX			p1_1	// output，led of rx
#define 	MOSI       		p1_4 	// output，SPI MOSI
#define 	nCS   			p1_5	// output, SPI enable
#define 	SCK        		p1_6	// output，SPI clock
#define 	RF_RST			p1_7	// output，SX1276 reset
#define 	MISO       		p4_7 	// output，SPI MISO


#define EI();		asm("FSET I");
#define DI();		asm("FCLR I");


//SX1276 Internal registers Addres
#define LR_RegFifo                       0x00
#define LR_RegOpMode                     0x01
#define LR_RegBitrateMsb                 0x02
#define LR_RegBitrateLsb                 0x03
#define LR_RegFdevMsb                    0x04
#define LR_RegFdMsb                      0x05
#define LR_RegFrMsb                      0x06
#define LR_RegFrMid                      0x07
#define LR_RegFrLsb                      0x08
#define LR_RegPaConfig                   0x09
#define LR_RegPaRamp                     0x0A
#define LR_RegOcp                        0x0B
#define LR_RegLna                        0x0C
#define LR_RegFifoAddrPtr                0x0D
#define LR_RegFifoTxBaseAddr             0x0E
#define LR_RegFifoRxBaseAddr             0x0F
#define LR_RegFifoRxCurrentaddr          0x10
#define LR_RegIrqFlagsMask               0x11
#define LR_RegIrqFlags                   0x12
#define LR_RegRxNbBytes                  0x13
#define LR_RegRxHeaderCntValueMsb        0x14
#define LR_RegRxHeaderCntValueLsb        0x15
#define LR_RegRxPacketCntValueMsb        0x16
#define LR_RegRxPacketCntValueLsb        0x17
#define LR_RegModemStat                  0x18
#define LR_RegPktSnrValue                0x19
#define LR_RegPktRssiValue               0x1A
#define LR_RegRssiValue                  0x1B
#define LR_RegHopChannel                 0x1C
#define LR_RegModemConfig1               0x1D
#define LR_RegModemConfig2               0x1E
#define LR_RegSymbTimeoutLsb             0x1F
#define LR_RegPreambleMsb                0x20
#define LR_RegPreambleLsb                0x21
#define LR_RegPayloadLength              0x22
#define LR_RegMaxPayloadLength           0x23
#define LR_RegHopPeriod                  0x24
#define LR_RegFifoRxByteAddr             0x25
#define LR_RegModemConfig3               0x26
#define REG_LR_DIOMAPPING1               0x40
#define REG_LR_DIOMAPPING2               0x41
#define REG_LR_VERSION                   0x42
#define REG_LR_PLLHOP                    0x44
#define REG_LR_TCXO                      0x4B
#define REG_LR_PADAC                     0x4D
#define REG_LR_FORMERTEMP                0x5B
#define REG_LR_AGCREF                    0x61
#define REG_LR_AGCTHRESH1                0x62
#define REG_LR_AGCTHRESH2                0x63
#define REG_LR_AGCTHRESH3                0x64

// length of packet
#define payload_length    9

// rx packet
unsigned char rxbuf[30];


unsigned char count_tx=0;
unsigned char packet_size;
unsigned char rf_timeout;

typedef struct 
{
	unsigned char reach_tx			: 1;	
	unsigned char is_tx			: 1;
	unsigned char rf_reach_timeout		: 1;
}   FlagType;
FlagType          Flag;


void reset_sx1276(void);
void SX1276_Config(void);
void rx_init(void);
unsigned char SpiInOut (unsigned char data);
unsigned char SPIReadReg(unsigned char addr);
void SPIWriteReg(unsigned char addr, unsigned char value);
void SPIBurstRead(unsigned char addr, unsigned char *ptr, unsigned char len);
void SPIBurstWrite(unsigned char addr, unsigned char *ptr, unsigned char len);
void sysclk_cfg(void);
void port_init(void);
void delay_1ms(unsigned int delay_time);
void delay_10ms(void);
void delay_x10ms(unsigned int dx10ms);
void nop_10();
void timerx_init(void);


void main(void)
{
	unsigned char  i, j, temp;
	
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	DI();
	
	sysclk_cfg();		// internal 8M osc
	nop_10();
	port_init();		// IO definition
	nop_10();
	
	delay_x10ms(50);	// wait for MCU ready
	LED_TX = 0;
	LED_RX = 0;
	
	timerx_init();  	// timer of 30hz
	EI();
	asm("nop"); 

	reset_sx1276();		// reset RF module
 	SX1276_Config();  	// initialize RF module

	rx_init();		// enter rx mode
	while(1)
	{		
		if(!Flag.is_tx)							// RF in receive mode
		{				
			temp=SPIReadReg(LR_RegIrqFlags);	// read interrupt
			if(temp&0x40)						// wait for rxdone
			{ 
				SPIWriteReg(LR_RegIrqFlags,0xff);		// clear interrupt
				temp = SPIReadReg(LR_RegFifoRxCurrentaddr);	// read RxCurrentaddr
				SPIWriteReg(LR_RegFifoAddrPtr,temp);		// RxCurrentaddr -> FiFoAddrPtr

				packet_size = SPIReadReg(LR_RegRxNbBytes);	// read length of packet  

				SPIBurstRead(0x00, rxbuf, packet_size);		// read from fifo
			
				if(strstr(rxbuf,"send_test")!=NULL) 		// check the packet
				{
					LED_RX=LED_RX^1;			// data is correct then flash the LED
					rx_init();
				}
				else
				{
					rx_init();				// restart Rx when data is not correct.
				}
				
		    }	
		}	
	}
}
void reset_sx1276(void)
{
	RF_RST=0;
	delay_x10ms(1);		// delay 10ms
	RF_RST=1;
	delay_x10ms(2);		// delay 20ms
}   
void SX1276_Config(void)
{
	// In setting mode, RF module should turn to sleep mode
	SPIWriteReg(LR_RegOpMode,0x08);		// low frequency mode，sleep mode
	nop_10();

	SPIWriteReg(REG_LR_TCXO,0x09);		// external Crystal
	SPIWriteReg(LR_RegOpMode,0x88);		// lora mode

	SPIWriteReg(LR_RegFrMsb,0x6c);
	SPIWriteReg(LR_RegFrMid,0x80);
	SPIWriteReg(LR_RegFrLsb,0x00);		// frequency：434Mhz					 

	SPIWriteReg(LR_RegPaConfig,0xff);	// max output power

	SPIWriteReg(LR_RegOcp,0x0B);		// close ocp
	SPIWriteReg(LR_RegLna,0x23);		// enable LNA

	SPIWriteReg(LR_RegModemConfig1,0x62);	// BW=62.5kHz,CR = 4/5,explicit Header mode
	SPIWriteReg(LR_RegModemConfig2,0x97);	// SF=9
	SPIWriteReg(LR_RegModemConfig3,0x00);	// LowDataRateOptimize disabled. if (2^SF)/BW>0.016,this register should be set to 0x08
	  
	SPIWriteReg(LR_RegSymbTimeoutLsb,0xFF); // max rx timeout

	SPIWriteReg(LR_RegPreambleMsb,0x00);
	SPIWriteReg(LR_RegPreambleLsb,16);      // preamble 16 bytes  	

	SPIWriteReg(REG_LR_PADAC,0x87);         // tx power 20dBm
	SPIWriteReg(LR_RegHopPeriod,0x00);      // no hopping

	SPIWriteReg(REG_LR_DIOMAPPING2,0x01);   // DIO5=ModeReady,DIO4=CadDetected
	SPIWriteReg(LR_RegOpMode,0x09);         // standby mode
}
void rx_init(void)
{
	unsigned char addr; 
	
	Flag.is_tx = 0;
	    
	SPIWriteReg(REG_LR_DIOMAPPING1,0x01);			//DIO0=00, DIO1=00, DIO2=00, DIO3=01  DIO0=00--RXDONE
	  
	SPIWriteReg(LR_RegIrqFlagsMask,0x3f);			// enable rxdone and rxtimeout
	SPIWriteReg(LR_RegIrqFlags,0xff);			// clear interrupt

	addr = SPIReadReg(LR_RegFifoRxBaseAddr);		// read RxBaseAddr
	SPIWriteReg(LR_RegFifoAddrPtr,addr);			// RxBaseAddr->FifoAddrPtr
	SPIWriteReg(LR_RegOpMode,0x0d);				// enter rx continuous mode
}
// spi read and write one byte
unsigned char SpiInOut(unsigned char data) 
{
	unsigned char i;
	
	for (i = 0; i < 8; i++)		
	{				
		if (data & 0x80)
			MOSI = 1;
		else
			MOSI = 0;
			
		data <<= 1;
		SCK = 1;
		
		if (MISO)
			data |= 0x01;
		else
			data &= 0xfe;
			
		SCK = 0;
	}	
	return (data);	
}
// SPI read register
unsigned char SPIReadReg(unsigned char addr)
{
	unsigned char data; 
	
	MOSI=0;
	SCK=0;
	
	nCS=0;
	SpiInOut(addr);			// write register address
	data = SpiInOut(0);		// read register value
	nCS=1;
	return(data);
}

// SPI write register
void SPIWriteReg(unsigned char addr, unsigned char value)                
{                                                       
	addr |= 0x80;			// write register,MSB=1

	SCK=0;
	nCS=0;

	SpiInOut(addr);			// write register address
	SpiInOut(value);		// write register value

	SCK=0;
	MOSI=1;
	nCS=1;
}
// spi burst write
void SPIBurstRead(unsigned char addr, unsigned char *ptr, unsigned char len)
{
	unsigned char i;
	if(len<=1)			// length>1,use burst mode
		return;
	else
	{
		SCK=0; 
		nCS=0;
		SpiInOut(addr);
		for(i=0;i<len;i++)
			ptr[i] = SpiInOut(0);
		nCS=1;  
	}
}
// spi burst read
void SPIBurstWrite(unsigned char addr, unsigned char *ptr, unsigned char len)
{ 
	unsigned char i;
	
	addr |= 0x80;  

	if(len<=1)			// length>1,use burst mode
		return;
	else  
	{   
		SCK=0;
		nCS=0;        
		SpiInOut(addr);
		for(i=0;i<len;i++)
			SpiInOut(ptr[i]);
		nCS=1;  
	}
}
// system clock setting
void sysclk_cfg(void)			// internal 8M osc
{   	
    prc0 = 1;
    prc1 = 1;
    prc3 = 1;
    
    cm02 = 0;
    cm05 = 1;
    cm06 = 0;

    cm13 = 0;
    cm15 = 1;
    cm16 = 0;
    cm17 = 0;
    cm10 = 0;	
    
    hra00 = 1;
    
    ocd0 = 0;
    ocd1 = 0;
    ocd2 = 1;
    ocd3 = 0;
    hra01 = 1;       
    cm14 = 1;
    prc0 = 0; 
}
// IO definition
void port_init(void)
{   	
    p1 = 0x00;
    pd1 = 0b11111011;
    p3 = 0xff;
    pd3 = 0b10111000;
    p4 = 0xff;
    pd4 = 0b00100000;
    
    
    pur0 = 0xcc;
    pur1 = 0x02;
    drr = 0xff;
    u0mr=0x00;
    
    tzic = 0x00;
    txic = 0x00;
    tcic = 0x00;
    adic = 0x00;
    s0tic = 0x00;
    s0ric = 0x00;
    s1tic = 0x00;
    s1ric = 0x00;	
    			
}
void delay_1ms(unsigned int delay_time)
{
	unsigned int i;
	while(delay_time !=0)
	{
		for (i =380; i!=0; i--)
		{
			asm("NOP");
			asm("NOP");
		}	
		delay_time--;
	}	
}	

void delay_10ms(void)
{
	int i;
	
	for(i = 0; i<2472; i++)
	{
		;
	}	
	// add watchdog			
	wdtr = 0;
	wdtr = 0xff;
	// add watchdog
}
	
//-------------------------------------------
void delay_x10ms(unsigned int dx10ms)
{
	unsigned int j;
	
	for(j = 0; j<dx10ms; j++)
		delay_10ms();	
}

void nop_10()
{
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
	asm("NOP");	
}	
// 30Hz timer
void timerx_init(void)
{
	txmr = 0x00;
	tcss = 0x01;			
	prex = 0xff;
	tx = 129;	
	txic = 0x04;
	txs = 1;	
}
// interrupt of timer
// timer X			(software int 22)
#pragma interrupt	_timer_x(vect=22)
void _timer_x(void);
void _timer_x(void)
{
	// tx timeout
	rf_timeout++;
	if(rf_timeout == 120)
	{
		rf_timeout=0;
		Flag.rf_reach_timeout = 1;
	}	
}