/*****************************************************************
 * Include files                                                 *
 *****************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#ifndef WIN32
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdarg.h>
#include <signal.h>

#include "cpmuart.h"
#endif

#include "serialport.h"
#include "main.h"

struct seradm
{
	int			serialfd;
	char		lockname[64];
	struct seradm	*next;
};

#ifndef WIN32
static struct seradm	*serlist = 0;
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*!
 * \brief	Close a serial port
 *
 *		The lock file is removed and the serial port is closed.
 *
 * \param	fd: the file descriptor as returned by \ref openSerialPort
 * \returns	-1 on error, 0 on success
 */
int closeSerialPort(int fd)
{
#ifdef WIN32
	return -1;
#else
	struct seradm	*prev = 0;
	struct seradm	*scan;
	int			retval = -1;
	
	if (fd < 0)
		return -1;
		
	if (pthread_mutex_lock(&mutex) != 0)
		return -1;
		
	for (scan = serlist; scan != 0; prev = scan, scan = scan->next)
	{
		if (scan->serialfd == fd)
		{
			close(scan->serialfd);
			unlink(scan->lockname);
			if (prev)
				prev->next = scan->next;
			else
				serlist = scan->next;
			free(scan);
			retval = 0;
			break;
		}
	}
	
	pthread_mutex_unlock(&mutex);
	
	return retval;
#endif
}

/*!
 * \brief	Open a serial port
 *
 *		The serial port is only opened if it was not locked. Locking is
 *		achieved by creating a file in /var/lock named after the serial port:
 *		LCK..<name> where <name> is the basename of the device path.
 *
 *		The lock file contains a string representation of the process id of the
 *		process which has opened the file.
 *
 *		The opened serial port is set to 8 bits, no parity, 1 stop bit and raw mode.
 *		Hardware flow control is enabled.
 *
 * \param	dev: a device name, as in open()
 * \param	flags: the open flags, as in open()
 * \returns	-1 on error, an open file descriptor on success
 */
int openSerialPort(const char *dev, int flags)
{
#ifdef WIN32
	return -1;
#else
	const char		*locktail = dev;
	char		tmpname[64];
	char		tmp[64];
	int			fd;
	int			len;
	struct termios	options;
	int			maxtries = 10;
	struct seradm	*newser = calloc(1, sizeof(struct seradm));
	
	if (!newser)
		return -1;
		
	// get the lock in /var/lock
	while (*locktail)
		locktail++;
	while (locktail > dev && *locktail != '/')
		locktail--;
	if (*locktail == '/')
		locktail++;
	sprintf(newser->lockname,"/var/lock/LCK..%s", locktail);
	strcpy(tmpname, "/var/lock/LCK..TMXXXXXX");
	if ((fd = mkstemp(tmpname)) < 0)
	{
		free(newser);
		return -1;
	}
	
	chmod(tmpname, 0644);
	len = sprintf(tmp, "%10d\n", getpid());
	if (write(fd, tmp,len) != len)
	{
		close(fd);
		unlink(tmpname);
		free(newser);
		return -1;
	}
	close(fd);
	
	while (link(tmpname, newser->lockname) < 0)
	{
		// lock taken by someone else?
		int	lockedpid;
		
		if (errno != EEXIST)
		{
			// some strange error, bail out
			unlink(tmpname);
			free(newser);
			return -1;
		}
		
		if (  ( (fd = open(newser->lockname, O_RDONLY)) < 0)
				|| ( (len = read(fd, tmp, sizeof(tmp)-1)) < 0))
		{
			// cannot open or read lock file, bail out
			unlink(tmpname);
			if (fd >= 0)
				close(fd);
			free(newser);
			return -1;
		}
		close(fd);
		tmp[len] = 0;
		
		if (sscanf(tmp, " %d", &lockedpid) != 1)
		{
			unlink(tmpname);
			free(newser);
			return -1;	// no pid in lockfile?
		}
		
		if (lockedpid == getpid())
			break;	// lock already taken by us, probably double open
			
		if (kill(lockedpid, 0) < 0 && errno == ESRCH)
		{
			// stale lock file, remove it
			if (unlink(newser->lockname) < 0 && errno != EINTR && errno != ENOENT)
			{
				// cannot remove the stale lockfile, give up
				unlink(tmpname);
				free(newser);
				return -1;
			}
			syslog(LOG_INFO, "removed stale lock file %s\n", newser->lockname);
		}
		
		if (--maxtries < 0)
		{
			unlink(tmpname);
			free(newser);
			return -1;
		}
		
		DoSleep(100);	// sleep 100ms
	}
	unlink(tmpname);
	
	// now the lockfile is written, really open the device
	
	if ((fd = open(dev, flags)) < 0)
	{
		unlink(newser->lockname);
		free(newser);
		return -1;
	}
	
	// get the parameters
	tcgetattr(fd, &options);
	
	cfmakeraw(&options);
	
	// Enable the receiver
	options.c_cflag |= CREAD;
	
	// No parity (8N1):
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	
	// enable hardware flow control (CNEW_RTCCTS)
	options.c_cflag |= CRTSCTS;
	
	// Set the new options for the port...
	tcsetattr(fd, TCSANOW, &options);
	
	newser->serialfd = fd;
	
	if (pthread_mutex_lock(&mutex) != 0)
	{
		close(fd);
		unlink(newser->lockname);
		free(newser);
		return -1;
	}
	
	newser->next = serlist;
	serlist = newser;
	
	pthread_mutex_unlock(&mutex);
	
	return fd;
#endif
}

/*!
 * \brief	Configure a serial port using a config structure.
 *
 *		The config structure can be read from a config file with \ref readSerialConfig.
 *
 *		The serial port is set to raw mode, and to the settings in the config structure.
 *
 * \param	fd: an open file descriptor, e.g. as returned by open() or \ref openSerialPort
 * \param	config: a config structure, e.g. read with \ref readSerialConfig
 * \returns	-1 on error, 0 on success
 */
int configSerialConfig(int fd, struct SERIAL_CONF *config)
{
#ifdef WIN32
	return -1;
#else
	struct termios	options;
	speed_t		speed;
	int			devicenr=-1;
	
	// get the parameters
	if (tcgetattr(fd, &options) < 0)
		return -1;
		
	cfmakeraw(&options);
	
	// Enable the receiver
	options.c_cflag |= CREAD;
	
	// Clear handshake, parity, stopbits and size
	options.c_cflag &= ~CLOCAL;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	
	switch (config->baudrate)
	{
	default:
	case 0:
		speed = B0;
		break;
	case 50:
		speed = B50;
		break;
	case 75:
		speed = B75;
		break;
	case 110:
		speed = B110;
		break;
	case 134:
		speed = B134;
		break;
	case 150:
		speed = B150;
		break;
	case 200:
		speed = B200;
		break;
	case 300:
		speed = B300;
		break;
	case 600:
		speed = B600;
		break;
	case 1200:
		speed = B1200;
		break;
	case 1800:
		speed = B1800;
		break;
	case 2400:
		speed = B2400;
		break;
	case 4800:
		speed = B4800;
		break;
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
		speed = B115200;
		break;
	case 230400:
		speed = B230400;
		break;
	};
	if (cfsetispeed(&options, speed) < 0)
		return -1;
	if (cfsetospeed(&options, speed) < 0)
		return -1;
	
	if (strncmp(config->devpath, "/dev/tts/", strlen("/dev/tts/")) == 0)
	{
		if (sscanf(config->devpath, "/dev/tts/%d", &devicenr) != 1)
		{
			devicenr=-1;
		}
		
		/* RS485 auto transmitter enable and echo suppress features are only
		   supported by scc type hardware */
		if ((devicenr >= 2) && (devicenr <= 4))
		{
			switch (config->type)
			{
			default:
			case SERIAL_RS232:	/* 1 + 2 wire full duplex */
				ioctl(fd, RS485_TRANSMITTER_ON, 0);
				ioctl(fd, RS485_NO_ECHO_SUPPRESS, 0);
				break;
			case SERIAL_RS422:	/* 1 + 4 wire differential full duplex */
				ioctl(fd, RS485_TRANSMITTER_AUTO, 0);
				ioctl(fd, RS485_NO_ECHO_SUPPRESS, 0);
				break;
			case SERIAL_RS485:	/* 1 + 2 wire differential half duplex */
				ioctl(fd, RS485_TRANSMITTER_AUTO, 0);
				ioctl(fd, RS485_ECHO_SUPPRESS, 0);
				break;
			}
		}
		else
		{
			ioctl(fd, RS485_TRANSMITTER_ON, 0);
		}
	}
	
	switch (config->stopbits)
	{
	default:
	case 1:
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	}
	
	switch (config->databits)
	{
	case 5:
		options.c_cflag |= CS5;
		break;
	case 6:
		options.c_cflag |= CS6;
		break;
	case 7:
		options.c_cflag |= CS7;
		break;
	default:
	case 8:
		options.c_cflag |= CS8;
		break;
	}
	
	switch (config->parity)
	{
	default:
	case SERIAL_NO:
		break;
	case SERIAL_ODD:
		options.c_cflag |= PARENB | PARODD;
		break;
	case SERIAL_EVEN:
		options.c_cflag |= PARENB;
		break;
	}
	
	switch (config->handshake)
	{
	case SERIAL_NONE:
		options.c_cflag |= CLOCAL;
		break;
	case SERIAL_HW:
		options.c_cflag |= CRTSCTS;
		break;
	case SERIAL_XONXOFF:
		options.c_cflag |= CLOCAL;	/* no hardware handshake */
		options.c_iflag |= IXON | IXOFF;
		break;
	}
	
	// Set the new options for the port...
	if (tcsetattr(fd, TCSANOW, &options) < 0)
		return -1;
		
	return 0;
#endif
}
