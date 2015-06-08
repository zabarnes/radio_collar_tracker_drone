GCC_FLAGS = -c -std=gnu99 -g -I/usr/local/include
LD_FLAGS = `pkg-config --cflags --libs gtk+-2.0 librtlsdr`

.PHONY: clean install
testTracker: collarTracker.o benchgps.o rtlsdr.o
	gcc collarTracker.o benchgps.o rtlsdr.o $(LD_FLAGS) -o testTracker

collarTracker: collarTracker.o mavgps.o rtlsdr.o serialGPS.o
	gcc collarTracker.o mavgps.o serialGPS.o rtlsdr.o $(LD_FLAGS) -o collarTracker


serialGPS.o: serialGPS.c
	gcc -I mavlink/include/mavlink/v1.0 serialGPS.c $(GCC_FLAGS)

collarTracker.o: collarTracker.c gps.h rtlsdr.h
	gcc collarTracker.c $(GCC_FLAGS)

benchgps.o: gps.h benchgps.c gps_struct.h
	gcc benchgps.c $(GCC_FLAGS)

rtlsdr.o: rtlsdr.c rtlsdr.h
	gcc rtlsdr.c $(GCC_FLAGS)

clean:
	-rm *.o
	-rm *.out
	-rm collarTracker
	-rm testTracker

install:
	sudo sh -c 'echo blacklist dvb_usb_rtl28xxu > /etc/modprobe.d/rtlsdr.conf'
	sudo sh -c 'echo "sudo mount /dev/mmcblk0p1 /media/RAW_DATA -o dmask=000,fmask=111" > /etc/rc.local'
	make collarTracker
	cp collarTracker ~/collarTracker
