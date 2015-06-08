#include "gps.h"
#include <time.h>
#include <stdlib.h>

int setup_GPS() {
	return 0;
}

void update_GPS() {
	return;
}

gps_struct_t* get_GPS() {
	gps_struct_t* gps = malloc(sizeof(gps_struct_t));
	gps->lat = 32.7150;
	gps->lon = 117.1625;
	gps->alt = 20;
	gps->time = time(NULL);
}
