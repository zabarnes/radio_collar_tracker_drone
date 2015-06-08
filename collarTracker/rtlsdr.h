#ifndef __RTLSDR_MOD__
#define __RTLSDR_MOD__
#include <rtl-sdr.h>

void setup_rtlsdr(rtlsdr_dev_t**, unsigned int, unsigned int, int*, int);
int test_rtlsdr(rtlsdr_dev_t*);
#endif //__RTLSDR_MOD__
