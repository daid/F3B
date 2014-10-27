#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

//Fuse high: 0xDF
//Fuse low: 0xE4

#define MY_ADDR    '4'

#define START_BYTE (0xF0)
#define CMD_SET    '1'
#define CMD_CLR    '2'

#define DELAY() asm("nop")

#define B(n) (1L << (n))
#define DN   B(28)
#define DR   B(29)
#define DL   B(27)
#define CE   B(25)
#define UR   B(30)
#define UL   B(26)
#define UP   B(31)
#define DOTS (B(3)|B(24))

uint32_t segmentDef[17] = {
    //0123456789
    UP | UL | UR | DL | DR | DN |      B(5)|B(8)|B(11)|B(12)|B(14)|B(15)|B(18)|B(21)|B(23),
    UR | DR |                          B(4)|B(5)|B(6)|B(7)|B(8)|B(9)|B(10)|B(11)|B(12)|B(14)|B(18),
    UP | UR | CE | DL | DN |           B(6)|B(7)|B(9)|B(12)|B(13)|B(17)|B(19)|B(20)|B(21)|B(23),
    UP | UR | CE | DR | DN |           B(7)|B(9)|B(14)|B(15)|B(16)|B(18)|B(19)|B(20)|B(21)|B(23),
    UL | UR | CE | DR |                B(7)|B(9)|B(10)|B(11)|B(12)|B(14)|B(16)|B(18)|B(19)|B(20),
    UP | UL | CE | DR | DN |           B(5)|B(7)|B(9)|B(10)|B(14)|B(15)|B(16)|B(17)|B(19)|B(21),
    UL | CE | DL | DR | DN |           B(6)|B(8)|B(9)|B(10)|B(12)|B(14)|B(15)|B(16)|B(17)|B(19)|B(21)|B(22)|B(23),
    UP | UR | DR |                     B(6)|B(7)|B(8)|B(9)|B(10)|B(11)|B(12)|B(14)|B(18)|B(23),
    UP | UL | UR | CE | DL | DR | DN | B(5)|B(6)|B(8)|B(9)|B(10)|B(12)|B(14)|B(15)|B(16)|B(18)|B(19)|B(20)|B(21)|B(23),
    UP | UL | UR | CE | DR | DN |      B(5)|B(7)|B(9)|B(10)|B(14)|B(15)|B(16)|B(18)|B(19)|B(20)|B(21)|B(23),
    //Empty
    B(4)|B(5)|B(6)|B(7)|B(8)|B(9)|B(10)|B(11)|B(12)|B(13)|B(14)|B(15)|B(16)|B(17)|B(18)|B(19)|B(20)|B(21)|B(22)|B(23),
    //ABCDEF
    UP | UL | UR | CE | DL | DR |      B(5)|B(6)|B(8)|B(9)|B(10)|B(13)|B(16)|B(18)|B(19)|B(20)|B(21)|B(23),
    UP | UL | UR | CE | DL | DR | DN | B(8)|B(9)|B(13)|B(15)|B(17)|B(18)|B(21)|B(23),
    UP | UL | DL | DN |                B(5)|B(8)|B(12)|B(13)|B(16)|B(17)|B(18)|B(19)|B(20)|B(21),
    UP | UL | UR | DL | DR | DN |      B(8)|B(13)|B(15)|B(18)|B(21)|B(23),
    UP | UL | CE | DL | DN |           B(5)|B(6)|B(8)|B(9)|B(10)|B(12)|B(13)|B(16)|B(17)|B(18)|B(20)|B(21),
    UP | UL | CE | DL |                B(5)|B(6)|B(8)|B(9)|B(10)|B(13)|B(14)|B(15)|B(16)|B(17)|B(18)|B(20)|B(21),
};

// PB6 = Data
// PB5 = Clock
// PB4 = Strobe1
// PB3 = Strobe2
// PB2 = Strobe3
// PB1 = Strobe4
// PD1 = Strobe5
// PA1 = Strobe6
// PA0 = Strobe7
// PD2 = Strobe8
// PD3 = Strobe9
// PD4 = Strobe10

uint8_t values[10];

#define CLOCK_BITS(_v) { \
        if (values[_v] != 10) data = segmentDef[values[_v]] | DOTS; else data = segmentDef[values[_v]];\
        PORTB = 0xFF; \
        for(n=0; n<32; n++) { \
            if (data & (1L << n)) PORTB &=~_BV(6); else PORTB |= _BV(6); \
            PORTB &=~_BV(5); DELAY(); PORTB |= _BV(5); DELAY(); \
        }}

void setNumbers()
{
    uint32_t data;
    uint8_t n;
    
    CLOCK_BITS(0);
    PORTB &=~_BV(4); DELAY(); PORTB |= _BV(4); DELAY();

    CLOCK_BITS(1);
    PORTB &=~_BV(3); DELAY(); PORTB |= _BV(3); DELAY();

    CLOCK_BITS(2);
    PORTB &=~_BV(2); DELAY(); PORTB |= _BV(2); DELAY();

    CLOCK_BITS(3);
    PORTB &=~_BV(1); DELAY(); PORTB |= _BV(1); DELAY();

    CLOCK_BITS(4);
    PORTD &=~_BV(1); DELAY(); PORTD |= _BV(1); DELAY();

    CLOCK_BITS(5);
    PORTA &=~_BV(1); DELAY(); PORTA |= _BV(1); DELAY();

    CLOCK_BITS(6);
    PORTA &=~_BV(0); DELAY(); PORTA |= _BV(0); DELAY();

    CLOCK_BITS(7);
    PORTD &=~_BV(2); DELAY(); PORTD |= _BV(2); DELAY();

    CLOCK_BITS(8);
    PORTD &=~_BV(3); DELAY(); PORTD |= _BV(3); DELAY();

    CLOCK_BITS(9);
    PORTD &=~_BV(4); DELAY(); PORTD |= _BV(4); DELAY();
}

unsigned char recvState = 0;
unsigned char recvAddr, recvCmd, recvData;

volatile unsigned char currentOutput = 102;
volatile unsigned char nextOutput = 101;
volatile unsigned char updatePos = 0x01;

volatile unsigned char timeout = 0;

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
        if (recvAddr == MY_ADDR)
        {
            switch(recvCmd)
            {
            case CMD_SET:
                values[recvData - '0'] = recv - '0';
                break;
            case CMD_CLR:
                for(recvData=0;recvData<9;recvData++)
                    values[recvData] = 10;
                break;
            }
            timeout = 0;
        }
        recvState = 0;
        break;
    }
}

int __attribute__((noreturn)) main(void)
{
    unsigned char delayCnt;
    
    PORTA = 0x03;
    PORTB = 0xFF;
    PORTD = 0xFE;
    DDRA = 0x03;
    DDRB = 0xFF;
    DDRD = 0xFE;

    UCSRA = 0x00;
    UCSRB = _BV(RXEN) | _BV(RXCIE);
    UCSRC = _BV(UCSZ1) | _BV(UCSZ0);
    UBRRH = 0;
    UBRRL = 25;

    sei();

    values[0] = MY_ADDR - '1';
    values[1] = 11;
    values[2] = 12;
    values[3] = 13;
    values[4] = 14;
    values[5] = 15;
    values[6] = 16;
    values[7] = 10;
    values[8] = 10;
    values[9] = 10;
    setNumbers();
    _delay_ms(2000);
    delayCnt = 0;
    for(;;)
    {
        uint8_t n;
        if (timeout > 20)
        {
            delayCnt = delayCnt + 1;
            if (delayCnt == 17)
                delayCnt = 0;
            for(n=0;n<10;n++)
                values[n] = delayCnt;
            _delay_ms(900);
        }else{
            timeout ++;
        }
        _delay_ms(100);
        setNumbers();
    }
}
