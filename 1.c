#include <mega8.h>
#include <alcd.h>
#include <delay.h>
#include <stdio.h>
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)
#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<DOR)
#define RX_BUFFER_SIZE 8
unsigned char rx_buffer[RX_BUFFER_SIZE];
unsigned char lcd_buf[8];
unsigned char bat_buf[16];

#if RX_BUFFER_SIZE <= 256
unsigned char rx_wr_index=0,rx_rd_index=0;
#else
unsigned int rx_wr_index=0,rx_rd_index=0;
#endif

#if RX_BUFFER_SIZE < 256
unsigned char rx_counter=0;
#else
unsigned int rx_counter=0;
#endif
char status,data;

float volts[7];
// This flag is set on USART Receiver buffer overflow
bit rx_buffer_overflow;
bit READY = 0;
// USART Receiver interrupt service routine
interrupt [USART_RXC] void usart_rx_isr(void)
{READY = 0;
#asm("cli")

status=UCSRA;
data=UDR;
if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
   {
   rx_buffer[rx_wr_index++]=data;
#if RX_BUFFER_SIZE == 256
   // special case for receiver buffer size=256
   if (++rx_counter == 0) rx_buffer_overflow=1;
#else
   if (rx_wr_index == RX_BUFFER_SIZE) rx_wr_index=0;
   if (++rx_counter == RX_BUFFER_SIZE)
      {
      rx_counter=0;
      rx_buffer_overflow=1;
      }
#endif
   }
 #asm("sei")  
}

#ifndef _DEBUG_TERMINAL_IO_
// Get a character from the USART Receiver buffer
#define _ALTERNATE_GETCHAR_
#pragma used+
char getchar(void)
{
char data;
while (rx_counter==0);
data=rx_buffer[rx_rd_index++];
#if RX_BUFFER_SIZE != 256
if (rx_rd_index == RX_BUFFER_SIZE) rx_rd_index=0;
#endif
#asm("cli")
--rx_counter;
#asm("sei")
return data;
}
#pragma used-
#endif

// Standard Input/Output functions


void main(void)
{

// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: Off
// USART Mode: Asynchronous
// USART Baud Rate: 38400
UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
UCSRB=(1<<RXCIE) | (0<<TXCIE) | (0<<UDRIE) | (1<<RXEN) | (0<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);
UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
UBRRH=0x00;
UBRRL=0x0C;

lcd_init(16);
#asm("sei")

while (1)
      {
      
      if(rx_buffer[0]=='S' && rx_buffer[7]=='E')
         {
            #asm("cli")
            volts[0]=(rx_buffer[1]*5.0/256);
            volts[1]=((rx_buffer[2]*2-rx_buffer[1])*5.0/256);
            volts[2]=((rx_buffer[3]*3-rx_buffer[2]*2)*5.0/256);
            volts[3]=((rx_buffer[4]*4-rx_buffer[3]*3)*5.0/256);
            volts[4]=((rx_buffer[5]*5-rx_buffer[4]*4)*5.0/256);
            volts[5]=((rx_buffer[6]*6-rx_buffer[5]*5)*5.0/256);
            volts[6]=(rx_buffer[6]*30.0/256);
            #asm("sei")   
            lcd_gotoxy(0,0);
            sprintf(lcd_buf,"C1:%.2f ",volts[0]);
            lcd_puts(lcd_buf);
            sprintf(lcd_buf,"C2:%.2f ",volts[1]);
            lcd_puts(lcd_buf);
            sprintf(lcd_buf,"C3:%.2f ",volts[2]);
            lcd_puts(lcd_buf);
            sprintf(lcd_buf,"C4:%.2f ",volts[3]);
            lcd_puts(lcd_buf);
            delay_ms(1900);  
            lcd_clear();
            lcd_gotoxy(0,0);
            sprintf(lcd_buf,"C5:%.2f ",volts[4]);
            lcd_puts(lcd_buf);
            sprintf(lcd_buf,"C6:%.2f ",volts[5]);
            lcd_puts(lcd_buf);
            sprintf(bat_buf,"WHOLE BAT:%.2f ",volts[6]); 
            lcd_puts(bat_buf);
            delay_ms(1900);
        }
   }
}
