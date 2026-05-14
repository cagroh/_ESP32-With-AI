#include "esp32-hal-gpio.h"
/*
(c) 2024 by COGИITIVA and Cesar GROH
Functions for driving a 2-digit, 7-segment display
*/

// Esta é a pinagem padrão. Altere como julgar necessário
// These are the defaul pin numbers. You can re-define them as you wish
#define D1_a   4
#define D1_b   5
#define D1_c  15
#define D1_d  16
#define D1_e  17
#define D1_f  18
#define D1_g  19

#define D1_dp 21 // ponto decimal (decimal point)

#define D2_a  12
#define D2_b  14
#define D2_c  25
#define D2_d  26
#define D2_e  27
#define D2_f  32
#define D2_g  33

// Funções públicas (public functions)
void clear_display();
void clear_digit (int digit);
void display_number (int adigit, int anumber);
void decimal_on();
void decimal_off();

// Funções internas (internal functions)
void set_display_segment (int asegment, int astate);

// Matrizes de trabalho (working matrix)
int D1_Pins[7] = {D1_a, D1_b, D1_c, D1_d, D1_e, D1_f, D1_g};
int D2_Pins[7] = {D2_a, D2_b, D2_c, D2_d, D2_e, D2_f, D2_g};
//int Disp7Seg[2][7] = {{D1_Pins}, {D2_Pins}};

int Disp7Seg[2][7] = {{D1_a, D1_b, D1_c, D1_d, D1_e, D1_f, D1_g}, {D2_a, D2_b, D2_c, D2_d, D2_e, D2_f, D2_g}};

void setup_display_pins(){
  for (byte i = 0; i < 1; i++){
    for (byte j = 0; j < 7; j++){
      pinMode (Disp7Seg[i][j], OUTPUT);
    }  
  }
  pinMode(D1_dp, OUTPUT);
}

// Apagar o display todo (turn entrire display off)
void clear_display (){
  setup_display_pins();
  clear_digit (1);
  decimal_off();
  clear_digit (2);
}

// Apagar um digito (turn a digit off)
void clear_digit (int digit){
  setup_display_pins();
  for (byte i = 0; i < 7; i++){
    set_display_segment (Disp7Seg[digit - 1][i], LOW);
  }  
}

// Exibir número no dígito 1 ou 2 (show a number on digit 1 or 2)
void display_number (int adigit, int anumber){
  clear_digit (adigit);
  switch (anumber) {
    case 1:{
      set_display_segment (Disp7Seg[adigit - 1][1], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][2], HIGH);
      break;
    }
    case 2:{
      set_display_segment (Disp7Seg[adigit - 1][0], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][1], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][4], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][3], HIGH);
      break;
    }
    case 3:{
      set_display_segment (Disp7Seg[adigit - 1][0], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][1], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][2], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][3], HIGH);
      break;
    }
    case 4:{
      set_display_segment (Disp7Seg[adigit - 1][5], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][1], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][2], HIGH);
      break;
    }
    case 5:{
      set_display_segment (Disp7Seg[adigit - 1][0], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][1], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][4], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][3], HIGH);
      break;
    }
    case 6:{
      set_display_segment (Disp7Seg[adigit - 1][5], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][2], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][3], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][4], HIGH);
      break;
    }
    case 7:{
      set_display_segment (Disp7Seg[adigit - 1][0], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][1], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][2], HIGH);
      break;
    }
    case 8:{
      for (int i=0;i<7;i++){
        set_display_segment (Disp7Seg[adigit - 1][i], HIGH);
      }
      break;
    }
    case 9:{
      set_display_segment (Disp7Seg[adigit - 1][0], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][3], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][4], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][5], HIGH);
      set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
      break;
    }
    case 0:{
      for (int i=0;i<6;i++){
        set_display_segment (Disp7Seg[adigit - 1][i], HIGH);
      }
      break;
    }
    // Any other value display an "E" for error:
    default:
        set_display_segment (Disp7Seg[adigit - 1][0], HIGH);
        set_display_segment (Disp7Seg[adigit - 1][5], HIGH);
        set_display_segment (Disp7Seg[adigit - 1][6], HIGH);
        set_display_segment (Disp7Seg[adigit - 1][4], HIGH);
        set_display_segment (Disp7Seg[adigit - 1][3], HIGH);
      break;
  }
}

// Altera o estado de um segmento em um dígito (changes segment state in a digit)
void set_display_segment (int asegment, int astate){
  pinMode (asegment, OUTPUT);
  /*switch {astate) {
    0: astate = 0x00;
    1: astate = 0x01;
  }*/
  digitalWrite (asegment, astate);
}

void decimal_on(){
  set_display_segment (D1_dp, HIGH);
}

void decimal_off(){
  set_display_segment (D1_dp, LOW);
}


