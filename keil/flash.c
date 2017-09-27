#include <stdio.h>
#include "flash.h"

/*
static int set_data_flash_base(uint32_t u32DFBA)
{
    uint32_t   au32Config[4];

    if (FMC_ReadConfig(au32Config, 4) < 0) {
        printf("\nRead User Config failed!\n");
        return -1;
    }

    if ((!(au32Config[0] & 0x1)) && (au32Config[1] == u32DFBA))
        return 0;

    FMC_EnableConfigUpdate();

    au32Config[0] &= ~0x1;
    au32Config[1] = u32DFBA;

    if (FMC_WriteConfig(au32Config, 4) < 0)
        return -1;

    // Perform chip reset to make new User Config take effect
    SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
    return 0;
}
*/

void open_Flash(void)
{
	SYS_UnlockReg();
    FMC_Open();
    FMC_EnableLDUpdate();
}

void stop_Flash(void)
{
	FMC_DisableLDUpdate();
    FMC_Close();
    SYS_LockReg();
}
	
void Flash_Erase(uint32_t u32BegAddr, uint32_t u32EndAddr)
{
	uint32_t u32Addr;
	
	/*
	set_data_flash_base(u32BegAddr);
	
	SYS_UnlockReg();
    FMC_Open();
    FMC_EnableLDUpdate();
	*/

	for(u32Addr = u32BegAddr; u32Addr < u32EndAddr; u32Addr += FMC_FLASH_PAGE_SIZE)
	{	
        FMC_Erase(u32Addr);	
	}
	
	//FMC_DisableLDUpdate();
    //FMC_Close();
    //SYS_LockReg();  
}
	
int32_t  Flash_Write(uint32_t u32BegAddr, uint32_t u32EndAddr, int16_t* u16Data, uint16_t u32DataCnt, uint16_t* u16Full)
{
	uint32_t u32Cnt;
	uint32_t u32WAddr;
	uint32_t u32Data;
	
	/*
	set_data_flash_base(u32BegAddr);
	
    SYS_UnlockReg();
    FMC_Open();
	FMC_EnableLDUpdate();
	*/
	
	//fill data
	for (u32Cnt = 0; u32Cnt < u32DataCnt; u32Cnt++) 
	{
		u32WAddr = u32BegAddr + 4 * u32Cnt;
		if(u32WAddr < u32EndAddr)
		{	
			u32Data = *(u16Data + u32Cnt);
            FMC_Write(u32WAddr, u32Data);
			//printf("u32WriteAdd = %d u32Data=%d \n", u32WAddr, u32Data);
		}
		else
		{
			*u16Full = 1;
			break;
		}
    }
	
	/*
	FMC_DisableLDUpdate();
    FMC_Close();
    SYS_LockReg();
	*/
	
	return u32Cnt;
}

int32_t  Flash_Read(uint32_t u32Addr)
{
	uint32_t u32Data;
	
	/*
	set_data_flash_base(u32Addr);
	
    SYS_UnlockReg();
    FMC_Open();
	FMC_EnableLDUpdate();
	*/
	
    u32Data = FMC_Read(u32Addr);
	
	/*
	FMC_DisableLDUpdate();
    FMC_Close();
    SYS_LockReg();
	*/
	
	return u32Data;
}

void  Flash_Write4B(uint32_t u32Addr, uint32_t u32Data)
{	
	/*
	set_data_flash_base(u32Addr);
	
    SYS_UnlockReg();
    FMC_Open();
	FMC_EnableLDUpdate();
	*/
	
	FMC_Write(u32Addr, u32Data);
	
	/*
	FMC_DisableLDUpdate();
    FMC_Close();
    SYS_LockReg();
	*/
}


















