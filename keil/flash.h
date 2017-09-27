#ifndef __FLASH_H__
#define __FLASH_H__
#include "Platform.h"

void    Flash_Erase(uint32_t u32BegAddr, uint32_t u32EndAddr);
int32_t Flash_Write(uint32_t u32BegAddr, uint32_t u32EndAddr, int16_t* u16Data, uint16_t u32DataCnt, uint16_t* u16Full);
int32_t Flash_Read(uint32_t u32Addr);
void    Flash_Write4B(uint32_t u32Addr, uint32_t u32Data);
void    open_Flash(void);
void    stop_Flash(void);

#endif

