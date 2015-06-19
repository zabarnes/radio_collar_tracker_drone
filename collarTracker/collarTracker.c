#include <signal.h>	// Add signal handling
#include <stdio.h>	// Add i/o
#include <string.h>
#include <sys/time.h>	// Include interval timer
#include <stdlib.h>
#include "rtlsdr.h"	// rtlsdr module
#include "gps.h"	// gps module interface.  GPS option select object file
#include <stdint.h>
#include <string.h>

///////////////
// Filepaths //
///////////////
#define CONFIG_FILE_PATH	"/media/RAW_DATA/rct/cfg"
#define FILE_COUNTER_PATH	"/media/RAW_DATA/rct/fileCount"
#define RAW_DATA_PREFIX		"/media/RAW_DATA/rct/RAW_DATA_"
#define META_FILE_PREFIX	"/media/RAW_DATA/rct/RUN_META_"

///////////////////////////
// Function declarations //
///////////////////////////
void timerloop(int signum);
int main(int argc, char const *argv[]);
void load_config();
void sigint_handler(int);
void setup();
void set_run_number();
void compile_data(uint8_t*, gps_struct_t*, uint8_t*);
int store_data(int length, uint8_t* buffer);
int update_meta();
void adjust_gain();
void teardown();

///////////////////////////
// Constant Declarations //
///////////////////////////
#define EXTRA_DATA_SIZE 13
#define GPS_SERIAL_PERIOD 250

///////////////////////////
// Variable declarations //
///////////////////////////
/**
 * Run state.  1 is running, 0 is flag to stop
 */
int run_state = 1;
/**
 * Center frequency for collars in Hz
 */
unsigned int center_frequency = 172250000;
/**
 * SDR ADC bandwidth in Hz.
 */
unsigned int sampling_frequency = 2048000;
/**
 * Data recording period in ms.  Also frame length in ms.
 */
int record_period = 1500;
/**
 * SNR amplitude goal.
 */
int SNR_amplitude_goal = 96;
/**
 * gain constant for the autogain controller
 */
int controller_coef = 16;
/**
 * Number of record frames per file
 */
int frames_per_file = 4;

/**
 * Length of the sdr data buffer
 */
int time_buffer_len = 0;

/**
 * Length of the file buffer
 */
int raw_file_buffer_len = 0;

/**
 * RTL SDR device descriptor
 */
rtlsdr_dev_t* sdr = NULL;

/**
 * Valid SDR gain values
 */
int gain_val[] = {0, 9, 14, 27, 37, 77, 87, 125, 144, 157, 166, 197, 207,
                  229, 254, 280, 297, 328, 338, 364, 372, 386, 402, 421,
                  434, 439, 445, 480, 496
                 };
/**
 * Maximum gain index.  Alias for length of gain_val[].
 */
int max_gain_idx = 28;

/**
 * Current gain index
 */
int cur_gain_idx = 0;

/**
 * Current run number
 */
int currentRun = 0;

/**
 * Current frame.
 */
int frame_counter = 0;

/**
 * Current data file number
 */
int file_number = 1;

//////////////////////////
// Signal Buffer Arrays //
//////////////////////////
/**
 * Time domain buffer
 */
uint8_t *time_buffer = NULL;
/**
 * Data storage Time domain buffer
 */
uint8_t *raw_file_buffer = NULL;


/////////////////////////
// Functon Definitions //
/////////////////////////
/**
 * Main loop
 *
 * @param  argc argument count
 * @param  argv argument variable
 * @return      exit code
 */
int main(int argc, char const *argv[]) {

	setup();

	set_run_number();

	while (run_state) {
	}

	teardown();
	return 0;
}

/**
 * Sets the current run's run nuber and increments the run number on disk.
 */
void set_run_number() {
	FILE *fileStream = fopen(FILE_COUNTER_PATH, "r+");
	char fileBuffer[256];
	if (fgets(fileBuffer, 256, fileStream) == NULL) {
		fprintf(stderr, "ERROR Reading Data Config File!\n");
		exit(1);
	}
	int intAux = 0;
	while (fileBuffer[intAux] != ':') {
		intAux++;
	}
	if (intAux >= 256) {
		fprintf(stderr, "ERROR Reading Data Config File!\n");
		exit(1);
	}
	currentRun = atoi(fileBuffer + intAux + 1);
	currentRun++;
	rewind(fileStream);
	fprintf(fileStream, "currentRun: %d \n", currentRun);
	fclose(fileStream);
}

/**
 * Configures the environment.  Sets up the signal handler, configuration params
 * timing loop
 */
void setup() {
	// Register SIGINT handler
	signal(SIGINT, sigint_handler);

	// Load configuration file
	load_config();

	// run rtlsdr setup
	setup_rtlsdr(&sdr, sampling_frequency, center_frequency,
	             gain_val[cur_gain_idx]);

	// run gps setup
	setup_GPS();

	// Validate setup
	update_GPS();	// blocks until fix achieved
	if (test_rtlsdr(sdr)) {
		exit(-1);
	}
	printf("GPS and SDR operating!\n");

	// Allocate memory for buffers
	time_buffer = malloc(time_buffer_len * sizeof(unsigned char));
	raw_file_buffer = malloc(raw_file_buffer_len * sizeof(unsigned char));

	// Configure timing loop
	struct itimerval tv;
	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = 100000;	// reset to 100 ms
	tv.it_value.tv_sec = 0;
	tv.it_value.tv_usec = 100000;	// set initial to 100 ms
	setitimer(ITIMER_REAL, &tv, NULL);
	signal(SIGALRM, timerloop);

}

/**
 * Timer loop.  This function runs each time the timer is called.
 *
 * @param signum signal input
 */
void timerloop(int signum) {
	// Get rtlsdr data
	if (read_rtlsdr(sdr, time_buffer, time_buffer_len)) {
		// problem with read!
		exit(-1);
	}

	gps_struct_t* gps;
	gps = get_GPS();

	compile_data(time_buffer, gps, raw_file_buffer);

	printf("Current Frame: %03d Gain: %03d\n", frame_counter,
	       gain_val[(int)cur_gain_idx]);
	frame_counter++;
	if (frame_counter >= frames_per_file) {
		printf("FILE: %06d\n", file_number);
		if (store_data(raw_file_buffer_len, raw_file_buffer)) {
			fprintf(stderr, "ERROR: Failed to write to raw file!\n");
			exit(-1);
		}
		frame_counter = 0;
		if (update_meta()) {
			fprintf(stderr, "ERROR: Failed to write to meta file!\n");
			exit(-1);
		}
		file_number++;
	}
	adjust_gain();
}

/**
 * Compiles the given buffers of data into the raw file buffer.
 *
 * @param time_buffer     SDR data buffer
 * @param gps_data        GPS data struct
 * @param raw_file_buffer Raw data buffer to fill
 */
void compile_data(uint8_t* time_buffer, gps_struct_t* gps_data,
                  uint8_t* raw_file_buffer) {
	for (int i = 0; i < time_buffer_len / 2; i++) {
		raw_file_buffer[frame_counter * (time_buffer_len + EXTRA_DATA_SIZE) + 2 * i] =
		    time_buffer[2 * i];
		raw_file_buffer[frame_counter * (time_buffer_len + EXTRA_DATA_SIZE) + 2 * i + 1]
		    = time_buffer[2 * i + 1];
	}

	for (int i = 0; i < 4; i++) {
		raw_file_buffer[frame_counter * (time_buffer_len + EXTRA_DATA_SIZE) +
		                time_buffer_len + 1] = ((int)(gps_data->lat * 10000000) >> 8 * i) & 0xFF;
		raw_file_buffer[frame_counter * (time_buffer_len + EXTRA_DATA_SIZE) +
		                time_buffer_len + 4 + i] = ((int)(gps_data->lon * 10000000) >> 8 * i) & 0xFF;
		raw_file_buffer[frame_counter * (time_buffer_len + EXTRA_DATA_SIZE) +
		                time_buffer_len + 8 + i] = ((int)(gps_data->alt * 1000) >> 8 * i) & 0xFF;
	}

	raw_file_buffer[frame_counter * (time_buffer_len + EXTRA_DATA_SIZE) +
	                time_buffer_len + EXTRA_DATA_SIZE - 1] = cur_gain_idx;
}

/**
 * Loads the configuration file specified in CONFIG_FILE_PATH
 */
void load_config() {
	// Temporaty custom parameters
	FILE* cfg_file = fopen(CONFIG_FILE_PATH, "r");
	if (!cfg_file) {
		fprintf(stderr, "ERROR: Failed to open the configuration file! Exiting"
		        "...\n");
		exit(1);
	}
	// 6 lines in the config file
	char* buffer = calloc(sizeof(char), 256);
	if (fscanf(cfg_file, "%s: %d", buffer, &center_frequency) != 2) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (strcmp(buffer, "center_freq")) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (fscanf(cfg_file, "%s: %d", buffer, &sampling_frequency) != 2) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (strcmp(buffer, "samp_rate")) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (fscanf(cfg_file, "%s: %d", buffer, &record_period) != 2) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (strcmp(buffer, "timeout_interrupt")) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (fscanf(cfg_file, "%s: %d", buffer, &SNR_amplitude_goal) != 2) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (strcmp(buffer, "goal_signal_amplitude")) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (fscanf(cfg_file, "%s: %d", buffer, &controller_coef) != 2) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (strcmp(buffer, "controller_coef")) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (fscanf(cfg_file, "%s: %d", buffer, &frames_per_file) != 2) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	if (strcmp(buffer, "number_frames_per_file")) {
		fprintf(stderr, "ERROR: Failed to properly read the configuration file!"
		        " Exiting...\n");
		exit(1);
	}
	// TODO: Why are these the values they are?
	// Generate inferenced parameters
	time_buffer_len = (int)(record_period * sampling_frequency /
	                        500.0);
	raw_file_buffer_len = frames_per_file * (time_buffer_len +
	                      EXTRA_DATA_SIZE);
}

/**
 * Handler for SIGINT.  This function prints out "Got SIGINT!!!" to the console
 * to notify the external parser of signal receipt, then exits with an exit code
 * of -1.
 *
 * @param signal Signal type
 */
void sigint_handler(int signal) {
	printf("Got SIGINT!!!\n");
	setitimer(ITIMER_REAL, NULL, NULL);
	run_state = 0;
}

/**
 * Stores length bytes from buffer into the file specified in the global
 * variables.
 *
 * @param  length Number of bytes to store
 * @param  buffer Buffer of data to store
 * @return        0 if successful, nonzero otherwise.
 */
int store_data(int length, uint8_t* buffer) {
	char filename[256];
	sprintf(filename, "%s%06d_%06d", RAW_DATA_PREFIX, currentRun, file_number);
	FILE* fileStream = fopen(filename, "wb");
	if (!fileStream) {
		return -1;
	}
	fwrite(raw_file_buffer, sizeof(uint8_t), raw_file_buffer_len * sizeof(uint8_t),
	       fileStream);
	fclose(fileStream);
	return 0;
}

/**
 * Updates the meta file on disk with data from this run.
 *
 * @return 0 if successful, nonzero otherwise.
 */
int update_meta() {
	char filename[256];
	sprintf(filename, "%s%06d", META_FILE_PREFIX, currentRun);
	FILE* fileStream = fopen(filename, "wb");
	if (!fileStream) {
		return -1;
	}
	fprintf(fileStream,
	        "center_freq: %d \n"
	        "samp_rate: %d \n"
	        "timeout_interrupt: %d \n"
	        "goal_signal_amplitude: %d \n"
	        "controller_coef: %d \n"
	        "number_frames_per_file: %d \n"
	        "currentRunFile: %d \n ",
	        center_frequency,
	        sampling_frequency,
	        record_period,
	        SNR_amplitude_goal,
	        controller_coef,
	        frames_per_file,
	        file_number);
	fclose(fileStream);
	return 0;
}

/**
 * Automatically adjusts the gain of the SDR.
 */
void adjust_gain() {
	int max = 0;
	for (int i = 0; i < time_buffer_len; i += 2) {
		if (time_buffer[i] > max) {
			max = time_buffer[i];
		}
	}
	max -= 128;

	cur_gain_idx += (SNR_amplitude_goal - abs(max)) / controller_coef;

	if (cur_gain_idx > max_gain_idx) {
		cur_gain_idx = max_gain_idx;
	} else if (cur_gain_idx < 0) {
		cur_gain_idx = 0;
	}

	if (set_gain_rtlsdr(sdr, gain_val[cur_gain_idx])) {
		fprintf(stderr, "WARNING: Failed to set gain!\n");
	}
}

/**
 * Cleanup function
 */
void teardown() {
	// close GPS
	teardown_gps();

	free(time_buffer);
	free(raw_file_buffer);
}
