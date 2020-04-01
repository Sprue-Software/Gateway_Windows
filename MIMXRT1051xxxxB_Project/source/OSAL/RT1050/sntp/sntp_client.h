#ifndef SNTP_CLIENT_H
#define SNTP_CLIENT_H



#include <time.h>
#include "sntp.h"



/* Realtek added for sntp update */
static unsigned int sntp_update_tick = 0;
static time_t sntp_update_sec = 0;
static time_t sntp_update_usec = 0;



void sntp_get_lasttime(long* sec, long* usec, long* tick);



#endif	// SNTP_CLIENT_H
