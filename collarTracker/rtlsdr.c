#include <rtl-sdr.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
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
                  uint32_t center_frequency, int gain) {
	// setup rtlsdr
	if (!rtlsdr_get_device_count()) {
		fprintf(stderr, "No supported devices found! Exiting...\n");
		exit(1);
	}

	int dev_index = 0;	// get 1st device
	if (rtlsdr_open(dev, dev_index)) {
		fprintf(stderr, "Failed to open rtlsdr device #%d! Exiting...\n", dev_index);
		exit(1);
	}

	if (rtlsdr_set_sample_rate(*dev, sampling_frequency)) {
		fprintf(stderr, "WARNING: Failed to set sample rate!\n");
	}

	if (rtlsdr_set_center_freq(*dev, center_frequency)) {
		fprintf(stderr, "WARNING: Failed to set center frequency!\n");
	}

	if (rtlsdr_set_tuner_gain_mode(*dev, 1)) {
		fprintf(stderr, "WARNING: Failed to enable manual gain!\n");
	}

	if (rtlsdr_set_tuner_gain(*dev, gain) {
	fprintf(stderr, "WARMING: Failed to set fixed gain!\n");
	}

	if (rtlsdr_reset_buffer(*dev)) {
	fprintf(stderr, "WARNING: Failed to reset buffers!\n");
	}
}

/**
 * Fills the given buffer with data from the given RTLSDR device.
 *
 * @param  dev             RTL SDR device
 * @param  time_buffer     buffer to fill with data
 * @param  time_buffer_len length of buffer
 * @return                 0 on success, -1 otherwise
 */
int read_rtlsdr(rtlsdr_dev_t* dev, uint8_t* time_buffer, int time_buffer_len) {
	int n_read = 0;

	if (rtlsdr_read_sync(dev, time_buffer, time_buffer_len, &n_read)) {
		fprintf(stderr, "WARNING: sync read failed!\n");
		return -1;
	}

	if (n_read < time_buffer_len) {
		fprintf(stderr, "WARNING: samples lost!\n");
		return -1;
	}
	return 0;
}

/**
 * Tests the given RTL SDR for reads.
 *
 * @param  dev Device to test
 * @return     0 on success, -1 otherwise.
 */
int test_rtlsdr(rtlsdr_dev_t* dev) {
	int n_read;
	uint8_t* time_buffer = malloc(256);
	if (!time_buffer) {
		return -1;
	}
	int time_buffer_len = 256;
	// TODO: set short delay of tens of samples
	rtlsdr_read_sync(dev, time_buffer, time_buffer_len, &n_read);
	if (n_read == 0) {
		fprintf(stderr, "ERROR: SDR not recording! Exiting...\n");
		return -1;
	}
	return 0;
}
