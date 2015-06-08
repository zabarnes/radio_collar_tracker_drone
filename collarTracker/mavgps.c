#include <serialGPS.h>
#include <gps.h>

int fd;

int setup_GPS() {
	fd = init_serial();
}

void update_GPS() {
	serial_read(fd);
}
