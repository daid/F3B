#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "serialport.h"
#include "main.h"

#define MAX_SERIAL_PORT 3
int serialPort[MAX_SERIAL_PORT];
int serialPortSigns;
int inputState[16] = {0};
int signIdx = 0;
#define SIGN_COUNT 16*5
int signNum[SIGN_COUNT];
int outputStarted = 0;
int outputEndTime[32];

void setSignNr(int signNr, int nr)
{
    //printf("Set sign: %i %i\n", signNr, nr);
    if (signNr < 0 || signNr >= SIGN_COUNT)
        return;
    signNum[signNr] = nr;
}

void startSignal(int signalNum, int length)
{
	int now = getTicks();
	int output = signalNum;

	outputStarted |= (1 << output);
	outputEndTime[output] = now + length;
}

int getOutputs(int cardNum)
{
	int i;
	int now = getTicks();
	for(i=0;i<32;i++)
	{
		if ((outputStarted & (1 << i)))
		{
			if (now >= outputEndTime[i])
				outputStarted &=~(1 << i);
		}
	}
	if (cardNum > 0)
		return outputStarted >> 16;
	return outputStarted;
}

void handleButton(int cardNum, int input)
{
	//TODO: Map cardNum + input to buttons
	if (cardNum == 0)
		pushButtonEvent(input);
	else
		pushButtonEvent(input + 16);
}

int getButtonState(int num)
{
    return (inputState[num / 16] & (1 << (num % 16))) ? 1 : 0;
}

void processInputs(int cardNum, int inputs)
{
	int i;
	if (inputs != inputState[cardNum])
	{
		for(i=0;i<16;i++)
		{
			if ((inputs & (1 << i)) && !(inputState[cardNum] & (1 << i)))
			{
				handleButton(cardNum, i);
			}
		}
		inputState[cardNum] = inputs;
	}
}

void writeMsg(int ID, int code, char* buffer, int len)
{
    unsigned char c;
    unsigned char bcc = 0;
    int i, j;

    c = 0x80 | ID;
    bcc += c;
    for(j=0;j<MAX_SERIAL_PORT;j++)
        write(serialPort[j], &c, 1);
    c = ((code & 0x03) << 5) | (len & 0x1f);
    bcc += c;
    for(j=0;j<MAX_SERIAL_PORT;j++)
        write(serialPort[j], &c, 1);
    if (len > 0)
    {
        for(i=0;i<len;i++)
            bcc += buffer[i];
        for(j=0;j<MAX_SERIAL_PORT;j++)
            write(serialPort[j], buffer, len);
    }
    bcc = ~bcc & 0x7f;
    for(j=0;j<MAX_SERIAL_PORT;j++)
        write(serialPort[j], &bcc, 1);
}

#define EXPECT_HDR1                    0
#define EXPECT_HDR2                    1
#define EXPECT_DATA                    2
#define EXPECT_BCC                     3

int state = EXPECT_HDR1;
int header = 0;
int bcc = 0;
int code = 0;
int dataLen = 0;
char data[16];
int dataSize = 0;
int resetFlags = 0;

void processChar(unsigned char c)
{
    DEBUG(DEBUG_IO, "io_in: %02x\n", c);
    switch(state)
    {
    case EXPECT_HDR1:
        if (c & 0x80)
        {
            header = c;
            bcc = c;
            state = EXPECT_HDR2;
        }
        break;
    case EXPECT_HDR2:
        if (!(c & 0x80))
        {
            code = (c & 0x60) >> 5;
            dataLen = (c & 0x1F);
            bcc += c;
            if(dataLen == 0 || dataLen > 16)
            {
                state = EXPECT_BCC;
            }else{
                dataSize = 0;
                state = EXPECT_DATA;
            }
        }else{
            header = c;
            bcc = c;
            state = EXPECT_HDR2;
        }
        break;
    case EXPECT_DATA:
        bcc += c;
        data[dataSize] = c;
        dataSize ++;
        if (dataSize == dataLen)
            state = EXPECT_BCC;
        break;
    case EXPECT_BCC:
        bcc = ~bcc & 0x7F;
        if (bcc == c)
        {
            if (code == 0 && dataLen == 6)
            {
                if (data[0])
                {
                    DEBUG(DEBUG_IO, "Status: %02x Msg: %i\n", data[0], data[1]);
                    resetFlags = 1;
                }
            }else if (code == 0 && dataLen == 1 && data[0] == 0x06)
            {
                resetFlags = 0;
            }else if (code == 0 && dataLen == 2)
            {
				processInputs((header & 0x0f), (((unsigned int)data[1]) << 8) | ((unsigned int)data[0]));
            }else{
                int i;
                DEBUG(DEBUG_IO, "Unknown msg(%02x): %02x %02x", header, code, dataLen);
                for(i=0;i<dataLen;i++)
                {
                    DEBUG(DEBUG_IO, " %02x", data[i]);
                }
                DEBUG(DEBUG_IO, "\n");
            }
        }
        state = EXPECT_HDR1;
        break;
    }
}

void* ioLoop(void* data)
{
    struct SERIAL_CONF config;
    struct SERIAL_CONF configSigns;
    unsigned char c;
    int outputs;
    char buffer[32];
    char *ptr;
    int card = 0, j;
    
    for(j=0;j<SIGN_COUNT;j++)
        signNum[j] = 255;

    strcpy(config.devpath, "/dev/tts/3");
    config.baudrate = 38400;
    config.type = SERIAL_RS422;
    config.stopbits = 1;
    config.databits = 8;
    config.parity = SERIAL_NO;
    config.handshake = SERIAL_NONE;

    strcpy(configSigns.devpath, "/dev/tts/1");
    configSigns.baudrate = 19200;
    configSigns.type = SERIAL_RS232;
    configSigns.stopbits = 1;
    configSigns.databits = 8;
    configSigns.parity = SERIAL_NO;
    configSigns.handshake = SERIAL_NONE;

#ifndef WIN32
    mkdir("/var/lock", 0777);
#else
#define O_NDELAY 0
#endif

    serialPort[0] = openSerialPort("/dev/tts/2", O_RDWR | O_NDELAY);
    serialPort[1] = openSerialPort("/dev/tts/3", O_RDWR | O_NDELAY);
    serialPort[2] = openSerialPort("/dev/tts/4", O_RDWR | O_NDELAY);
    for(j = 0; j < MAX_SERIAL_PORT; j++)
        configSerialConfig(serialPort[j], &config);

    serialPortSigns = openSerialPort(configSigns.devpath, O_RDWR | O_NDELAY);
    configSerialConfig(serialPortSigns, &configSigns);

    while(1)
    {
        if (resetFlags)
        {
            //Clear flags
            writeMsg(32, 0, "\x02\x07", 2);
        }else{
            //Get status
            //writeMsg(32, 1, NULL, 0);
            outputs = getOutputs(card);
            buffer[0] = 0;
            buffer[1] = outputs & 0xFF;
            buffer[2] = outputs >> 8;
            writeMsg(32 + card, 0, buffer, 3);
            card ^= 1;
        }
        
        ptr = buffer;
        for(c=0;c<4;c++)
        {
            while((signIdx & 0x0F) >= 10) signIdx++;
            if (signIdx == SIGN_COUNT)
                signIdx = 0;
            
            *ptr++ = 0xF0;
            *ptr++ = (signIdx >> 4) + '0';
            if (signNum[signIdx] >= 0 && signNum[signIdx] < 100)
            {
                *ptr++ = '1';
                *ptr++ = '0' + (signIdx & 0x0F);
                *ptr++ = '0' + signNum[signIdx];
            }else{
                *ptr++ = '1';
                *ptr++ = '0' + (signIdx & 0x0F);
                *ptr++ = '0' + 10;
            }
            signIdx++;
        }
        if (serialPortSigns != -1)
            write(serialPortSigns, buffer, (int)(ptr - buffer));

        for(j = 0; j < MAX_SERIAL_PORT; j++)
        {
            if (serialPort[j] != -1)
            {
                while(read(serialPort[j], &c, 1) > 0)
                {
                    processChar(c);
                }
            }
        }
        DoSleep(10);
    }

    return 0;
}
