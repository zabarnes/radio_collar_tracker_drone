#include <signal.h>	// Add signal handling
#include <stdio.h>	// Add i/o
#include <string.h>
#include <sys/time.h>	// Include interval timer
#include <stdlib.h>
#include "rtlsdr.h"	// rtlsdr module
#include "gps.h"	// gps module interface.  GPS option select object file

///////////////
// Filepaths //
///////////////
#define CONFIG_FILE_PATH	"/media/RAW_DATA/rct/cfg"
#define FILE_COUNTER_PATH	"/media/RAW_DATA/rct/fileCount"

///////////////////////////
// Function declarations //
///////////////////////////
void timerloop(int signum);
int main(int argc, char const *argv[]);
void load_config();
void sigint_handler(int);
void setup();
void set_run_number();

///////////////////////////
// Constant Declarations //
///////////////////////////
#define EXTRA_DATA_SIZE 13
#define GPS_SERIAL_PERIOD 250

///////////////////////////
// Variable declarations //
///////////////////////////
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
 *
 */
unsigned int time_buffer_len = 0;

/**
 *
 */
unsigned int raw_file_buffer_len = 0;

/**
 * Number of times we need to ping the gps per record frame to keep the gps
 * alive
 */
unsigned int gps_serial_mult = 0;

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

//////////////////////////
// Signal Buffer Arrays //
//////////////////////////
/**
 * Time domain buffer
 */
unsigned char *time_buffer = NULL;
/**
 * Data storage Time domain buffer
 */
unsigned char *raw_file_buffer = NULL;


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

	while (1) {
	}
	return 0;
}

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
	setup_rtlsdr(&sdr, sampling_frequency, center_frequency, gain_val,
	             cur_gain_idx);

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
	printf("Hello World!\n");
}

/**
 * Loads the configuration file specified in CONFIG_FILE_PATH
 */
void load_config() {
	// Temporaty custom parameters
	// Generate inferenced parameters
	// TODO: Why are these the values they are?
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
	exit(-1);
}
