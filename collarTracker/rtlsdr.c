#include <rtl-sdr.h>
#include <stdlib.h>
#include <stdio.h>

void setup_rtlsdr(rtlsdr_dev_t** dev, unsigned int sampling_frequency,
                  unsigned int center_frequency, int* gain_val, int cur_gain_idx) {
	// setup rtlsdr
	if (!rtlsdr_get_device_count()) {
		fprintf(stderr, "No supported devices found! Exiting...\n");
		exit(1);
	}

	int dev_index = 0;	// get 1st device
	if (rtlsdr_open(dev, dev_index) == dev_index) {
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

	if (rtlsdr_set_tuner_gain(*dev, gain_val[cur_gain_idx])) {
		fprintf(stderr, "WARMING: Failed to set fixed gain!\n");
	}

	if (rtlsdr_reset_buffer(*dev)) {
		fprintf(stderr, "WARNING: Failed to reset buffers!\n");
	}
}

int test_rtlsdr(rtlsdr_dev_t* dev) {
	int n_read;
	int* time_buffer = malloc(256);
	if (!time_buffer) {
		return -1;
	}
	int time_buffer_len = 64;
	// TODO: set short delay of tens of samples
	rtlsdr_read_sync(dev, time_buffer, time_buffer_len, &n_read);
	if (n_read == 0) {
		fprintf(stderr, "ERROR: SDR not recording! Exiting...\n");
		return -1;
	}
	return 0;
}
