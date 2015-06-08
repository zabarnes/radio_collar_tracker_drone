#ifndef __RTLSDR_MOD__
#define __RTLSDR_MOD__
#include <rtl-sdr.h>
/**
 * Sets up the RTL SDR with the given parameters.  Accepts a pointer to a local
 * variable of type rtlsdr_dev_t*, and will populate that variable with the
 * address of the rtlsdr_dev_t struct.  Accepts as unsigned 32-bit integers the
 * sampling frequency and center frequency.  Accepts as a signed 32-bit integer
 * the initial gain.
 *
 * @param dev                device variable
 * @param sampling_frequency Sampling frequency as an unsigned 32-bit int
 * @param center_frequency   Center frequency as an unsigned 32-bit int
 * @param gain           initial gain as a signed 32-bit int
 */
void setup_rtlsdr(rtlsdr_dev_t** dev, uint32_t sampling_frequency,
                  uint32_t center_frequency, int gain);

/**
 * Tests the given RTL SDR for reads.
 *
 * @param  dev Device to test
 * @return     0 on success, -1 otherwise.
 */
int test_rtlsdr(rtlsdr_dev_t* dev);

/**
 * Fills the given buffer with data from the given RTLSDR device.
 *
 * @param  dev             RTL SDR device
 * @param  time_buffer     buffer to fill with data
 * @param  time_buffer_len length of buffer
 * @return                 0 on success, -1 otherwise
 */
int read_rtlsdr(rtlsdr_dev_t* dev, uint8_t* time_buffer,
                int time_buffer_len);
#endif //__RTLSDR_MOD__
