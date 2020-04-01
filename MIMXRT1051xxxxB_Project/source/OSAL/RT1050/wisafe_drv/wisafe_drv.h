#ifndef WISAFE_DRV_H
#define WISAFE_DRV_H



/*******************************************************************************
 * Declarations
 ******************************************************************************/

void wisafedrv_init();
void wisafedrv_read(uint8_t* bytes, uint32_t* count);
void wisafedrv_write(uint8_t* bytes, uint32_t count);
void wisafedrv_close();



#endif	// WISAFE_DRV_H
