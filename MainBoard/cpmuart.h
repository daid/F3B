#ifndef _LINUX_CPMUART_H
#define _LINUX_CPMUART_H

#include <linux/ioctl.h>

#define	CPMUART_IOCTL_BASE	'R'

/*! Enable the RS485 transmitter permanently (as long as the device is open). */
#define RS485_TRANSMITTER_ON	_IO(CPMUART_IOCTL_BASE, 0x20)

/*! Disable the RS485 transmitter permanently (as long as the device is open). */
#define RS485_TRANSMITTER_OFF	_IO(CPMUART_IOCTL_BASE, 0x21)

/*! Enable the RS485 tranmitter enveloping the data sent (as long as the device is open). */
#define RS485_TRANSMITTER_AUTO	_IO(CPMUART_IOCTL_BASE, 0x22)

/*! Disable the RS485 receiver while data is sent */
#define	RS485_ECHO_SUPPRESS	_IO(CPMUART_IOCTL_BASE, 0x23)

/*! Enable the RS485 receiver while data is sent */
#define	RS485_NO_ECHO_SUPPRESS	_IO(CPMUART_IOCTL_BASE, 0x24)

#define SET_BAUD_RATE		_IOW(CPMUART_IOCTL_BASE, 0x25, int)

#define SET_MAX_RCV_CHARS	_IOW(CPMUART_IOCTL_BASE, 0x26, int)

#endif  /* ifndef _LINUX_CPMUART_H */
