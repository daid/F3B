#ifndef _SERIALPORT_H_
#define _SERIALPORT_H_

#define	SERIAL_MAXPATH	64

typedef enum 
{
    SERIAL_RS232 = 0,
    SERIAL_RS422,
    SERIAL_RS485
} SERIAL_TYPE;

typedef enum 
{
    SERIAL_NO = 0,
    SERIAL_ODD,
    SERIAL_EVEN
} SERIAL_PARITY;

typedef enum 
{
    SERIAL_NONE = 0,
    SERIAL_HW,
    SERIAL_XONXOFF
} SERIAL_HSHAKE;

struct SERIAL_CONF
{
    char		devpath[SERIAL_MAXPATH];
    int			baudrate;
    SERIAL_TYPE		type;
    int			stopbits;
    int			databits;
    SERIAL_PARITY	parity;
    SERIAL_HSHAKE	handshake;
};

extern int configSerialConfig(int fd, struct SERIAL_CONF *config);

extern int openSerialPort(const char *dev, int flags);
extern int closeSerialPort(int fd);

#endif




