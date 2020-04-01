#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__



void watchdog_init(uint32_t timeout_ms);
void watchdog_start();
void watchdog_refresh();
void watchdog_stop();



#endif  /* __WATCHDOG_H__ */
