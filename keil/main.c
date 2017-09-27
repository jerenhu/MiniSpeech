/******************************************************************************
 * @file     main.c
 * @version  V20170316
 * @author   yanhui
 * @brief    Speechreco
 * @note
 * Copyright (C) 2017 Yanhui. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "N575.h"
#include "recordctrl.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Main function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
	// Lock protected registers 
    if(SYS->REGLCTL == 1)
        SYS_LockReg();

    // Init System, IP clock and multi-function I/O 
    SYS_Init(); 
	
	// Enable LDO3.3V.
	CLK_EnableLDO(CLK_LDOSEL_3_3V);	
	
    // Init UART for printf 
    UART_Init();
	
	// LDO¡¡[KeyPad]
	CLK_EnableLDO(CLK_LDOSEL_3_3V);

	// Init input key [KeyPad]
	Init_InKey();
	
	while(1);
	
}

