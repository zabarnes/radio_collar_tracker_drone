#ifndef __MAVGPS__
#define __MAVGPS__

#include "gps_struct.h"

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

#endif	//__MAVGPS__
