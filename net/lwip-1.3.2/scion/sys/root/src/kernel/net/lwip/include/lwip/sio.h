
/*
 * This is the interface to the platform specific serial IO module
 * It needs to be implemented by those platforms which need SLIP or PPP
 */

#ifndef __SIO_H__
#define __SIO_H__

#include "lwip/arch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If you want to define sio_fd_t elsewhere or differently,
   define this in your cc.h file. */
#ifndef __sio_fd_t_defined
typedef void * sio_fd_t;
#endif

/* The following functions can be defined to something else in your cc.h file
   or be implemented in your custom sio.c file. */

#ifndef sio_open
/**
 * Opens a serial device for communication.
 * 
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum);
#endif

#ifndef sio_send
/**
 * Sends a single character to the serial device.
 * 
 * @param c character to send
 * @param fd serial device handle
 * 
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd);
#endif

#ifndef sio_recv
/**
 * Receives a single character from the serial device.
 * 
 * @param fd serial device handle
 * 
 * @note This function will block until a character is received.
 */
u8_t sio_recv(sio_fd_t fd);
#endif

#ifndef sio_read
/**
 * Reads from the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 * 
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len);
#endif

#ifndef sio_tryread
/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len);
#endif

#ifndef sio_write
/**
 * Writes to the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 * 
 * @note This function will block until all data can be sent.
 */
u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len);
#endif

#ifndef sio_read_abort
/**
 * Aborts a blocking sio_read() call.
 * 
 * @param fd serial device handle
 */
void sio_read_abort(sio_fd_t fd);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SIO_H__ */
