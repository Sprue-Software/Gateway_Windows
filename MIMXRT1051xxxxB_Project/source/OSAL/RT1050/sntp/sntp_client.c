#include "sntp_client.h"



// Callback for lwIP's SNTP
void sntp_set_system_time_us(u32_t sec, u32_t usec)
{
    //PRINTF("SNTP time: %d\r\n", sec);

    sntp_update_tick = xTaskGetTickCount();
    sntp_update_sec = sec;
    sntp_update_usec = usec;
}




// We call this one (based on Ameba SDK)
void sntp_get_lasttime(long* sec, long* usec, long* tick)
{
    *sec = sntp_update_sec;
    *usec = sntp_update_usec;
    *tick = sntp_update_tick;
}
