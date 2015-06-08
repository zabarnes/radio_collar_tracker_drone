#ifndef serialGPS_H_
#define serialGPS_H_
//////////////
// Includes //
//////////////
#include <stdbool.h>

/**
 * Configures the compiled serial port, default /dev/ttyUSB0 at 57600 8N1.  If
 * unable to open the port, exits
 *
 * @return file descriptor for the serial port
 */
int init_serial();

/**
 * Returns the file descriptor on success or -1 on error.
 *
 * @param  port UNIX sysfs path of serial port
 * @return      file descriptor on success, or -1 on error
 */
int open_port(const char* port);

/**
 * Configures the specified serial port with the specified configuration
 *
 * @param  fd               File descriptor referring to the serial port
 * @param  baud             Baud rate
 * @param  data_bits        Number of data bits
 * @param  stop_bits        Number of stop bits
 * @param  parity           Parity Mode
 * @param  hardware_control CTS/DTR use mode
 * @return                  true if successful, false otherwise
 */
bool setup_port(int fd, int baud, int data_bits, int stop_bits, bool parity,
                bool hardware_control);

/**
 * This function blocks waiting for serial data in it's own thread and stores
 * the data once received.
 *
 * @param  fd File descriptor referring to the serial port
 */
void serial_read(int fd);

/**
 * Returns raw GPS longitude
 *
 * @return raw GPS longitude
 */
int return_long();

/**
 * Returnfs raw GPS latitude
 *
 * @return raw GPS latitude
 */
int return_lati();


/**
 * Returns raw GPS altitude
 *
 * @return raw GPS altitude
 */
int return_alti();

/**
 * Closes the current file descriptor
 *
 * @param fd file descriptor to close
 */
void close_port(int fd);

#endif
