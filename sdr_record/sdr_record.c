/**
 * This program records broadband signal data from an RTL2832 based SDR to
 * assist detection of radio collars.
 * Copyright (C) 2015 by Nathan Hui <nthui@eng.ucsd.edu>
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtl-sdr.h>
#include <signal.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <stdbool.h>
#include "queue.h"
#include <stdint.h>

// Global constants
#define FILE_CAPTURE_DAEMON_SLEEP_PERIOD_MS	50
#define FRAMES_PER_FILE	80
#define META_PREFIX "META_"

// Typedefs
struct proc_queue_args {
	int run_num;
	int frame_len;
	int dev_id;
};

// Global variables
rtlsdr_dev_t** dev = NULL;
volatile int run = 1;
pthread_mutex_t lock;
queue* data_queue;
int counter = 0;
static volatile uint64_t num_samples = 0;
char* DATA_DIR = "/home/ntlhui/test/";
int* num_files;
int num_devices = 0;

// Function Prototypes
void sighandler(int signal);
int main(int argc, char** argv);
void* proc_queue(void* args);
static void rtlsdr_callback(unsigned char* buf, uint32_t len, void *ctx);
void lock_mutex();
void printUsage();
void* start_record(void* args);

/**
 * Prints the usage statement for this program.
 */
void printUsage() {
	printf("usage: sdr_record -r <run_number> [-h] [-g <gain>]\n\t"
	       "[-f <center_frequency>] [-s <sampling_frequency>] [-o <output_dir>]\n");
}

/**
 * Main function of this program.
 */
int main(int argc, char** argv) {
	// Set up varaibles
	// Options variable
	int opt;
	// SDR Gain
	int gain = 0;
	// Sampling Frequency for SDR in samples per second
	int samp_freq = 2048000;
	// Center Frequency for SDR in Hz
	int center_freq = 172464000;
	// Run number
	int run_num = -1;
	// Block size
	int block_size = 0;
	// Start Time
	struct timespec start_time;
	// Thread attributes
	pthread_attr_t* attr;
	// Thread ID
	pthread_t* thread_id;
	// proc_queue args
	struct proc_queue_args* pargs;
	// Start thread ID
	pthread_t* start_thread_id;
	// Start thread attributes
	pthread_attr_t* start_attr;

	// Get command line options
	// printf("Getting command line options\n");
	while ((opt = getopt(argc, argv, "hg:s:f:r:o:n:")) != -1) {
		switch (opt) {
			case 'h':
				printUsage();
				exit(0);
			case 'g':
				gain = (int)(atof(optarg) * 10);
				break;
			case 's':
				samp_freq = (int)(atof(optarg));
				break;
			case 'f':
				center_freq = (int)(atof(optarg));
				break;
			case 'r':
				run_num = (int)(atof(optarg));
				break;
			case 'o':
				DATA_DIR = optarg;
				break;
			case 'n':
				num_devices = atoi(optarg);
				break;

		}
	}
	if (run_num == -1) {
		// TODO: add usage notification here!
		fprintf(stderr, "ERROR: Bad RUN number!\n");
		printUsage();
		exit(-1);
	}
	// Set up *dev
	if(num_devices < 1){
		fprintf(stderr, "ERROR: Bad numer of radios!\n");
		printUsage();
		exit(-1);
	}

	printf("SDR_RECORD: Run Number %d\n", run_num);
	// printf("done\n");

	// Initialize environment
	// printf("Configuring environment\n");
	block_size = 262144;
	dev = malloc(num_devices * sizeof(rtlsdr_dev_t*));
	attr = malloc(num_devices * sizeof(pthread_attr_t));
	start_attr = malloc(num_devices * sizeof(pthread_attr_t));
	thread_id = malloc(num_devices * sizeof(pthread_t));
	pargs = malloc(num_devices * sizeof(struct proc_queue_args));
	data_queue = malloc(num_devices * sizeof(queue));
	start_thread_id = malloc(num_devices * sizeof(pthread_t));
	num_files = malloc(num_devices * sizeof(int));
	for(int i = 0; i < num_devices; i++){
		queue_init(&data_queue[i]);
	}
	// printf("Done configuring environment\n");

	// Open SDR
	// printf("Opening SDR\n");
	for(int dev_id = 0; dev_id < num_devices; dev_id++){
		if (rtlsdr_open(&(dev[dev_id]), dev_id)) {
			fprintf(stderr, "ERROR: Failed to open rtlsdr device %d\n", dev_id);
			exit(1);
		}
		// printf("SDR Opened\n");

		// Configure SDR
		// printf("Configuring SDR\n");
		if (rtlsdr_set_tuner_gain_mode(dev[dev_id], 1)) {
			fprintf(stderr, "ERROR: Failed to enable manual gain.\n");
			exit(1);
		}
		if (rtlsdr_set_tuner_gain(dev[dev_id], gain)) {
			fprintf(stderr, "ERROR: Failed to set tuner gain.\n");
			exit(1);
		}
		if (rtlsdr_set_center_freq(dev[dev_id], center_freq)) {
			fprintf(stderr, "ERROR: Failed to set center frequency.\n");
			exit(1);
		}
		if (rtlsdr_set_sample_rate(dev[dev_id], samp_freq)) {
			fprintf(stderr, "ERROR: Failed to set sampling frequency.\n");
			exit(1);
		}
		if (rtlsdr_reset_buffer(dev[dev_id])) {
			fprintf(stderr, "ERROR: Failed to reset buffers.\n");
			exit(1);
		}
		printf("SDR Configured.\n");
	}

	// Configure signal handlers
	// printf("Configuring signal handlers.\n");
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	// printf("Signal handlers configured\n");

	// Configure worker thread
	for(int dev_id = 0; dev_id < num_devices; dev_id++){
		// printf("Configuring worker thread\n");
		if (pthread_attr_init(&attr[dev_id])) {
			fprintf(stderr, "ERROR: Failed to create default thread attributes.\n");
			exit(1);
		}
		pargs[dev_id].run_num = run_num;
		pargs[dev_id].frame_len = block_size;
		pargs[dev_id].dev_id = dev_id;
		if (pthread_create(&thread_id[dev_id], &attr[dev_id], proc_queue, (void*) &pargs[dev_id])) {
			fprintf(stderr, "ERROR: Failed to create detached thread.\n");
			exit(1);
		}
		// printf("Worker thread configured\n");
	}

	// Get timestamp
	printf("Starting record\n");
	clock_gettime(CLOCK_REALTIME, &start_time);
	// begin recording
	for(int i = 0; i < num_devices; i++){
		// Thread attributes
		pthread_attr_init(&start_attr[i]);
		// Thread ID
		pthread_create(&start_thread_id[i], &start_attr[i], start_record, (void*) &pargs[i]);
	}
	// Add timing data
	char buf[256];
	snprintf(buf, sizeof(buf), "%s/%s%06d", DATA_DIR, META_PREFIX, run_num);
	FILE* timing_stream = fopen(buf, "w");
	fprintf(timing_stream, "start_time: %f\n",
	        start_time.tv_sec + (float)start_time.tv_nsec / 1.e9);
	fprintf(timing_stream, "center_freq: %d\n", center_freq);
	fprintf(timing_stream, "sampling_freq: %d\n", samp_freq);
	fprintf(timing_stream, "gain: %f\n", gain / 10.0);
	fflush(timing_stream);

	// Wait for record to finish
	for(int i = 0; i < num_devices; i++){
		pthread_join(start_thread_id[i], NULL);
	}

	// clean up
	printf("Stopping record\n");
	run = 0;
	fprintf(timing_stream, "num_files: %d\n", num_files[0]);
	fclose(timing_stream);
	printf("Queued %f seconds of data\n", num_samples / 2.0 / 2048000.0);
	for(int i = 0; i < num_devices; i++){
		pthread_join(thread_id[i], NULL);
		rtlsdr_close(dev[i]);
	}
}

/**
 * Starts the asynchronous recording.
 *
 * @param dev_id Device ID
 */
void* start_record(void* args){
	struct proc_queue_args* pargs = (struct proc_queue_args*) args;
	int frame_len = pargs->frame_len;
	int dev_id = pargs->dev_id;
	rtlsdr_read_async(dev[dev_id], rtlsdr_callback, (void*) &data_queue[dev_id], 0, frame_len);
	return NULL;
}

/**
 * Signal handler for this program.  Cancels the asynchronous read for the SDR
 * and notifies all threads to finish execution.
 */
void sighandler(int signal) {
	for(int i = 0; i < num_devices; i++){
		rtlsdr_cancel_async(dev[i]);
	}
	printf("Signal caught, exiting\n");
}

/**
 * Worker thread function.  Reads information from the queue and writes it to
 * the disk.
 */
void* proc_queue(void* args) {
	struct proc_queue_args* pargs = (struct proc_queue_args*) args;
	int run_num = pargs->run_num;
	int frame_len = pargs->frame_len;
	int dev_id = pargs->dev_id;
	// printf("Started thread %d\n", dev_id);
	FILE* data_stream;
	char buff[256];
	int frame_num;
	uint64_t num_samples = 0;
	num_files[dev_id] = 0;

	uint8_t data_buf[frame_len];

	frame_num = 0;
	// Lock mutex
	bool empty = true;
	pthread_mutex_lock(&lock);
	empty = queue_isEmpty(&data_queue[dev_id]);
	pthread_mutex_unlock(&lock);


	while (!empty || run) {
		// printf("DEV: %d\tRUN: %d\tLENGTH: %d\n", dev_id, run, data_queue[dev_id].length);
		lock_mutex();
		empty = queue_isEmpty(&data_queue[dev_id]);
		pthread_mutex_unlock(&lock);

		if (!empty) {
			// Process queue
			if (frame_num / FRAMES_PER_FILE + 1 != num_files[dev_id]) {
				if (data_stream) {
					fclose(data_stream);
				}
				snprintf(buff, sizeof(buff),
				         "%s/RAW_DATA_%06d_%06d_%02d", DATA_DIR, run_num,
				         frame_num / FRAMES_PER_FILE + 1, dev_id);
				// printf("DEV: %d\tFile: %s\n", dev_id, buff);
				num_files[dev_id]++;
				data_stream = fopen(buff, "wb");
			}
			lock_mutex();
			unsigned char* data_ptr = NULL;
			data_ptr = (unsigned char*) queue_pop(&data_queue[dev_id]);
			pthread_mutex_unlock(&lock);

			for (int i = 0; i < frame_len; i++) {
				data_buf[i] = (uint8_t)data_ptr[i];
			}
			fwrite(data_buf, sizeof(uint8_t), frame_len, data_stream);

			free(data_ptr);
			frame_num++;
			num_samples += frame_len / 2;
		} else {
			usleep(FILE_CAPTURE_DAEMON_SLEEP_PERIOD_MS * 1000);
		}
		lock_mutex();
		empty = queue_isEmpty(&data_queue[dev_id]);
		pthread_mutex_unlock(&lock);
	}
	printf("DEV: %d\tRecorded %f seconds of data to disk\n", dev_id, num_samples / 2048000.0);
	printf("DEV: %d\tQueue length at end: %d.\n",dev_id, data_queue[dev_id].length);
	fclose(data_stream);
	return NULL;
}

static void rtlsdr_callback(unsigned char* buf, uint32_t len, void *ctx) {
	counter++;
	if (counter > 1000) {
	}
	if (!run) {
		return;
	}
	num_samples += len / 2;
	unsigned char* newframe = malloc(len * sizeof(char));
	for (int i = 0; i < len; i++) {
		newframe[i] = buf[i];
	}
	pthread_mutex_lock(&lock);
	queue_push((queue*)ctx, (void*) newframe);
	pthread_mutex_unlock(&lock);
}

void lock_mutex() {
	pthread_mutex_lock(&lock);
}
