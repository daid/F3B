#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define MY_ADDR    '0'
#define MY_SUBADDR '3'

#define START_BYTE (0xF0)
#define CMD_SET    '1'
#define CMD_CLR    '2'

#define SEG_UP      0x01
#define SEG_UL      0x02
#define SEG_UR      0x04
#define SEG_CE      0x08
#define SEG_DL      0x10
#define SEG_DR      0x20
#define SEG_DN      0x40

unsigned char segmentDef[10] = {
    SEG_UP | SEG_UL | SEG_UR | SEG_DL | SEG_DR | SEG_DN,            //0
    SEG_UR | SEG_DR,                                                //1
    SEG_UP | SEG_UR | SEG_CE | SEG_DL | SEG_DN,                     //2
    SEG_UP | SEG_UR | SEG_CE | SEG_DR | SEG_DN,                     //3
    SEG_UL | SEG_UR | SEG_CE | SEG_DR,                              //4
    SEG_UP | SEG_UL | SEG_CE | SEG_DR | SEG_DN,                     //5
    SEG_UL | SEG_CE | SEG_DL | SEG_DR | SEG_DN,                     //6
    SEG_UP | SEG_UR | SEG_DR,                                       //7
    SEG_UP | SEG_UL | SEG_UR | SEG_CE | SEG_DL | SEG_DR | SEG_DN,   //8
    SEG_UP | SEG_UL | SEG_UR | SEG_CE | SEG_DR | SEG_DN,            //9
};

#define PORT_UP_0 PORTD
#define BIT_UP_0 0x2
#define PORT_UP_1 PORTA
#define BIT_UP_1 0x2
#define PORT_UL_0 PORTA
#define BIT_UL_0 0x1
#define PORT_UL_1 PORTD
#define BIT_UL_1 0x4
#define PORT_UR_0 PORTD
#define BIT_UR_0 0x8
#define PORT_UR_1 PORTD
#define BIT_UR_1 0x10
#define PORT_CE_0 PORTD
#define BIT_CE_0 0x20
#define PORT_CE_1 PORTB
#define BIT_CE_1 0x1
#define PORT_DL_0 PORTB
#define BIT_DL_0 0x2
#define PORT_DL_1 PORTB
#define BIT_DL_1 0x4
#define PORT_DR_0 PORTB
#define BIT_DR_0 0x8
#define PORT_DR_1 PORTB
#define BIT_DR_1 0x10
#define PORT_DN_0 PORTB
#define BIT_DN_0 0x20
#define PORT_DN_1 PORTB
#define BIT_DN_1 0x40

#define CHECK(n) if (mask & SEG_ ## n ) { if (data & SEG_ ## n ) { PORT_ ## n ## _1 |= BIT_ ## n ## _1; } else { PORT_ ## n ## _0 |= BIT_ ## n ## _0; } }
void setOutputs(unsigned char data, unsigned char mask)
{
    PORTD = 0x00;
    PORTB = 0x00;
    PORTA = 0x00;
    CHECK(UP);
    CHECK(UL);
    CHECK(UR);
    CHECK(CE);
    CHECK(DL);
    CHECK(DR);
    CHECK(DN);
}

unsigned char recvState = 0;
unsigned char recvAddr, recvCmd, recvData;

volatile unsigned char currentOutput = 102;
volatile unsigned char nextOutput = 101;
volatile unsigned char updatePos = 0x01;

ISR(USART_RX_vect)
{
    unsigned char recv = UDR;
    
    switch(recvState)
    {
    case 0:
        if (recv == START_BYTE)
        {
            recvState = 1;
        }
        break;
    case 1:
        recvAddr = recv;
        recvState = 2;
        break;
    case 2:
        recvCmd = recv;
        recvState = 3;
        break;
    case 3:
        recvData = recv;
        recvState = 4;
        break;
    case 4:
        if (recvAddr == MY_ADDR /* && ((recvAddr + recvCmd + recvData) ^ 0xFF) == recv */ )
        {
            switch(recvCmd)
            {
            case CMD_SET:
                if (recvData == MY_SUBADDR)
                {
                    nextOutput = recv - '0';
                }
                break;
            case CMD_CLR:
                nextOutput = 101;
                break;
            }
        }
        recvState = 0;
        break;
    }
}

int __attribute__((noreturn)) main(void)
{
    unsigned char delayCnt;
    
    PORTA = 0x00;
    PORTB = 0x00;
    PORTD = 0x00;
    DDRA = 0x03;
    DDRB = 0xFF;
    DDRD = 0x3E;

    UCSRA = 0x00;
    UCSRB = _BV(RXEN) | _BV(RXCIE);
    UCSRC = _BV(UCSZ1) | _BV(UCSZ0);
    UBRRH = 0;
    UBRRL = 25;

    sei();
    
    for(delayCnt=0;delayCnt<10;delayCnt++)
    {
        setOutputs(segmentDef[delayCnt], 0xFF);
        _delay_ms(300);
        setOutputs(0, 0x00);
        _delay_ms(700);
    }
    
    for(;;)
    {
        if (currentOutput != nextOutput)
        {
            currentOutput = nextOutput;
            if (currentOutput < 10)
            {
                setOutputs(segmentDef[currentOutput], 0xFF);
            }else{
                setOutputs(0, 0xFF);
            }
            _delay_ms(300);
        }else{
            if (currentOutput < 10)
            {
                setOutputs(segmentDef[currentOutput], updatePos);
            }else{
                setOutputs(0, updatePos);
            }
            updatePos = updatePos << 1;
            if (!updatePos)
                updatePos = 0x01;
            for(delayCnt=0;delayCnt<30;delayCnt++)
            {
                _delay_ms(10);
                if (currentOutput != nextOutput)
                    break;
            }
        }
    }
}
