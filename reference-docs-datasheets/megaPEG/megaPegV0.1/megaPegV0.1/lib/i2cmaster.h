/*************************************************************************
* Title:    C include file for the I2C master interface (i2cmaster.S)
* Author:   Peter Fleury <pfleury@gmx.ch>
* Date:     15-Jul-2000
* Note:     Adapt the file i2cmaster.S to your target !
**************************************************************************/
#ifndef _I2CMASTER_H
#define _I2CMASTER_H   1

/* define the data direction in i2c_start(),i2c_rep_start() */
#define I2C_READ    1
#define I2C_WRITE   0
#define I2C_ACK     1
#define I2C_NACK    0

/* initialize the I2C master interace. Need to be called only once */
extern void i2c_init(void);

/* Terminates the data transfer and releases the I2C bus */
extern void i2c_stop(void);

/* Issues a start condition and sends address and transfer direction */
/* return 0 = device accessible, 1= failed to access device          */
extern unsigned char i2c_start(unsigned char addr);

/* Issues a repeated start condition and sends address and transfer direction */
/* return 0 = device accessible, 1 = failed to access device                  */
extern unsigned char i2c_rep_start(unsigned char addr);

/* Issues a start condition and sends address and transfer direction */
/* if device is busy, use ack polling to wait until device ready     */
extern void i2c_start_wait(unsigned char addr);

/* Send one byte to I2C device                   */
/* return 0 = write successful, 1 = write failed */
extern unsigned char i2c_write(unsigned char data);

/* read one byte from the I2C device                     */
/* ack=1: send ack, request more data from device        */
/* ack=0: send nak, read is followed by a stop condition */
extern unsigned char i2c_read(unsigned char ack);
extern unsigned char i2c_readAck(void);
extern unsigned char i2c_readNak(void);

#define i2c_read(ack)  (ack) ? i2c_readAck() : i2c_readNak();


#endif
