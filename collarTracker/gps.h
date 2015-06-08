#ifndef __MAVGPS__
#define __MAVGPS__

typedef struct gps_struct {
	double lat;
	double lon;
	double alt;
	long time;
} gps_struct_t;

/**
 * Sets up the GPS for use
 *
 * @return 0 if successful, 1 otherwise
 */
int setup_GPS();

/**
 * Blocks until a fix has been achieved.
 */
void update_GPS();

/**
 * Returns a struct containing the most recent GPS fix. Fix is formatted
 * as lat/lon/alt (m).
 *
 * @return gps_struct_t containing GPS fix.
 */
gps_struct_t* get_GPS();

#endif	//__MAVGPS__
