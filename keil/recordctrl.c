#include <string.h>
#include <stdio.h>
#include "recordctrl.h"
#include "vadctrl.h"
#include "flash.h"

//S_KEYPAD_TGR_HANDLER
S_KEYPAD_TGR_HANDLER g_asTrgKeyHandler[] =  
{ 
	{KpHandler, SWB0_F, BIT0, KEYPAD_GPIOB, KEYPAD_FALLING}, 
	{KpHandler, SWB0_R, BIT0, KEYPAD_GPIOB, KEYPAD_RISING}, 
	{KpHandler, SWB0_P, BIT0, KEYPAD_GPIOB, KEYPAD_PRESSING}, 
	{KpHandler, SWB1_F, BIT1, KEYPAD_GPIOB, KEYPAD_FALLING}, 
	{KpHandler, SWB1_R, BIT1, KEYPAD_GPIOB, KEYPAD_RISING}, 
	{KpHandler, SWB1_P, BIT1, KEYPAD_GPIOB, KEYPAD_PRESSING},
	{KpHandler, SWB2_F, BIT2, KEYPAD_GPIOB, KEYPAD_FALLING}, 
	{KpHandler, SWB2_R, BIT2, KEYPAD_GPIOB, KEYPAD_RISING}, 
	{KpHandler, SWB2_P, BIT2, KEYPAD_GPIOB, KEYPAD_PRESSING}, 
	{KpHandler, SWB3_F, BIT3, KEYPAD_GPIOB, KEYPAD_FALLING}, 
	{KpHandler, SWB3_R, BIT3, KEYPAD_GPIOB, KEYPAD_RISING}, 
	{KpHandler, SWB3_P, BIT3, KEYPAD_GPIOB, KEYPAD_PRESSING},
	{KpHandler, SWB4_F, BIT6, KEYPAD_GPIOB, KEYPAD_FALLING}, 
	{KpHandler, SWB4_R, BIT6, KEYPAD_GPIOB, KEYPAD_RISING}, 
	{KpHandler, SWB4_P, BIT6, KEYPAD_GPIOB, KEYPAD_PRESSING},
	{KpHandler, SWB5_F, BIT7, KEYPAD_GPIOB, KEYPAD_FALLING}, 
	{KpHandler, SWB5_R, BIT7, KEYPAD_GPIOB, KEYPAD_RISING}, 
	{KpHandler, SWB5_P, BIT7, KEYPAD_GPIOB, KEYPAD_PRESSING},
	{0, 0, 0, 0, 0}
}; 

typedef struct
{
	uint16_t              u16TotCmd;
	uint16_t              u16CurCmd;
	uint32_t              u32RecBeg;
	uint32_t              u32RecEnd;
	uint32_t              u32CurBeg;
	uint32_t              u32RecCnt;  //Last Voice u32RecCnt Dots
	uint16_t              u16FullFg;
	uint16_t              u16StopFg;
	uint16_t              u16TmrCnt;
}S_RECORD_CTRL_HANDLE;

volatile static S_RECORD_CTRL_HANDLE s_sRecordCtrlHandle;


/*App Ctrl*/
//adc
volatile __align(4) int16_t gCtrlAdcConv;
volatile __align(4) int16_t gRecordDelay;
volatile __align(4) int16_t i16Buffer[BUFFER_LENGTH];

//time1
volatile __align(4) int16_t gCtrlTmr1Cnt;



//IRQ
void PDMA_IRQHandler(void)
{
	int rCnt = 0;	
	if(PDMA_GCR->GLOBALIF & (1 << CH))
	{
		if(gRecordDelay == 0 && s_sRecordCtrlHandle.u32RecCnt > ADC_SINGLE_DELAYS_CNT)
		{
			printf("Please Start Speech\n");
			gRecordDelay = 1;
		}
		
		if (PDMA->CHIF & (0x4 << PDMA_CHIF_WAIF_Pos)) //Current transfer half complete flag
		{	
			//DPWM_WriteFIFO(&i16Buffer[0], FRAME_SZIE);
			/*
			printf("Tx Half: ");
			for(rCnt = 0; rCnt < FRAME_SZIE; rCnt++)
			    printf("%hd ", i16Buffer[rCnt]);
			printf("\n");
			*/

			if (s_sRecordCtrlHandle.u16FullFg == 0 && s_sRecordCtrlHandle.u16StopFg == 0 && gCtrlAdcConv == 1)
			{
				//printf("Before£º s_sRecordCtrlHandle.u32CurBeg = %d\n", s_sRecordCtrlHandle.u32CurBeg);
				rCnt = Flash_Write(s_sRecordCtrlHandle.u32CurBeg, s_sRecordCtrlHandle.u32RecEnd, (int16_t*)&i16Buffer[0], FRAME_SZIE, (uint16_t*)&s_sRecordCtrlHandle.u16FullFg);
                s_sRecordCtrlHandle.u32RecCnt += rCnt;
			    s_sRecordCtrlHandle.u32CurBeg += (4 * rCnt);
			}
			
			PDMA->CHIF = (0x4 << PDMA_CHIF_WAIF_Pos); //Clear interrupt
		}
		else //Current transfer finished flag 
		{		
			//DPWM_WriteFIFO(&i16Buffer[FRAME_SZIE], FRAME_SZIE);
			//printf("Tx FINISH: ");
			//for(tmp = FRAME_SZIE; tmp < BUFFER_LENGTH; tmp++)
			//    printf("%hd\n", i16Buffer[tmp]);
			//printf("\n");
			
			if (s_sRecordCtrlHandle.u16FullFg == 0 && s_sRecordCtrlHandle.u16StopFg == 0 && gCtrlAdcConv == 1)
			{
		        rCnt = Flash_Write(s_sRecordCtrlHandle.u32CurBeg, s_sRecordCtrlHandle.u32RecEnd, (int16_t*)&i16Buffer[FRAME_SZIE], FRAME_SZIE, (uint16_t*)&s_sRecordCtrlHandle.u16FullFg);
                s_sRecordCtrlHandle.u32RecCnt += rCnt;
			    s_sRecordCtrlHandle.u32CurBeg += (4 * rCnt);
			}
			
			PDMA->CHIF = 0x1 << PDMA_CHIF_WAIF_Pos; //Clear interrupt
		}
	}	
}

void ADC_IRQHandler(void)
{
	if (ADC_GetIntFlag(ADC_CMP0_INT))
	{
		ADC_ClearIntFlag(ADC_CMP0_INT);
	}else if (ADC_GetIntFlag(ADC_CMP1_INT))
	{
		ADC_ClearIntFlag(ADC_CMP1_INT);
	}
}

void TMR1_IRQHandler()
{	
	if(gCtrlTmr1Cnt > 0)
	    gCtrlTmr1Cnt--;
	
	Keypad_ScanTgr();
	
	TIMER_ClearIntFlag(TIMER1);
	
	Keypad_TgrDecDebounceCounter();
}

// Init Sys
void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable External OSC49M */
    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk|CLK_PWRCTL_HIRCEN_Msk|CLK_PWRCTL_LIRCEN_Msk);
	
    /* Switch HCLK clock source to CLK2X a frequency doubled output of OSC48M */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKSEL0_HIRCFSEL_48M, CLK_CLKDIV0_HCLK(1));

	/* Set ADC divisor from HCLK */
    CLK_SetModuleClock(ADC_MODULE, MODULE_NoMsk, CLK_CLKDIV0_ADC(1));
	
	/* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Set GPG multi-function pins for UART0 RXD and TXD */
	SYS->GPA_MFP  = (SYS->GPA_MFP & (~SYS_GPA_MFP_PA8MFP_Msk) ) | SYS_GPA_MFP_PA8MFP_UART_TX;
	SYS->GPA_MFP  = (SYS->GPA_MFP & (~SYS_GPA_MFP_PA9MFP_Msk) ) | SYS_GPA_MFP_PA9MFP_UART_RX;
	
    /* Lock protected registers */
    SYS_LockReg();
}

void UART_Init(void)
{
    /* Reset IP */
	CLK_EnableModuleClock(UART_MODULE);
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate(115200) */
    UART_Open( UART0,115200 );
}

void ADC_Init(void)
{
	uint32_t u32Div;
	
	/* Reset IP */
	CLK_EnableModuleClock(ADC_MODULE);
	CLK_EnableModuleClock(ANA_MODULE);
    SYS_ResetModule(EADC_RST);
	SYS_ResetModule(ANA_RST);
	
	/* Enable Analog block power */
	ADC_ENABLE_SIGNALPOWER(ADC,
	                       ADC_SIGCTL_ADCMOD_POWER|
						   ADC_SIGCTL_IBGEN_POWER|
	                       ADC_SIGCTL_BUFADC_POWER|
	                       ADC_SIGCTL_BUFPGA_POWER);
	
	/* PGA Setting */
	ADC_MUTEON_PGA(ADC, ADC_SIGCTL_MUTE_PGA);
	ADC_MUTEOFF_PGA(ADC, ADC_SIGCTL_MUTE_IPBOOST);
	ADC_ENABLE_PGA(ADC, ADC_PGACTL_REFSEL_VMID, ADC_PGACTL_BOSST_GAIN_26DB);
	ADC_SetPGAGaindB(PGA_GAIN); // 0dB
	
	/* MIC circuit configuration */
	ADC_ENABLE_VMID(ADC, ADC_VMID_HIRES_DISCONNECT, ADC_VMID_LORES_CONNECT);
	ADC_EnableMICBias(ADC_MICBSEL_90_VCCA);
	ADC_SetAMUX(ADC_MUXCTL_MIC_PATH, ADC_MUXCTL_POSINSEL_NONE, ADC_MUXCTL_NEGINSEL_NONE);
	
	/* Open ADC block */
	ADC_Open();
	ADC_SET_OSRATION(ADC, ADC_OSR_RATION_192);
	u32Div = CLK_GetHIRCFreq()/ADC_SAMPLE_RATE/192;
	ADC_SET_SDCLKDIV(ADC, u32Div);
	ADC_SET_FIFOINTLEVEL(ADC, 7);
	
	ADC_MUTEOFF_PGA(ADC, ADC_SIGCTL_MUTE_PGA);
	
}

void PDMA_Init(void)
{
	volatile int32_t i = 10;
	
	/* Reset IP */
	CLK_EnableModuleClock(PDMA_MODULE);
	SYS_ResetModule(PDMA_RST);

	
	PDMA_GCR->GLOCTL |= (1 << CH) << PDMA_GLOCTL_CHCKEN_Pos; //PDMA Controller Channel Clock Enable
			
	PDMA->DSCT_CTL |= PDMA_DSCT_CTL_SWRST_Msk;   //Writing 1 to this bit will reset the internal state machine and pointers
	PDMA->DSCT_CTL |= PDMA_DSCT_CTL_CHEN_Msk;    //Setting this bit to 1 enables PDMA assigned channel operation 
	while(i--);                                  //Need a delay to allow reset
	
	PDMA_GCR->SVCSEL &= 0xfffff0ff;  //DMA channel is connected to ADC peripheral transmit request.
	PDMA_GCR->SVCSEL |= CH << PDMA_SVCSEL_DPWMTXSEL_Pos;  //DMA channel is connected to DPWM peripheral transmit request.
	
	PDMA->DSCT_ENDSA = (uint32_t)&ADC->DAT;    //Set source address
	PDMA->DSCT_ENDDA = (uint32_t)i16Buffer;    //Set destination address
	
	PDMA->DSCT_CTL |= 0x2 << PDMA_DSCT_CTL_SASEL_Pos;    //Transfer Source address is fixed.
	PDMA->DSCT_CTL |= 0x3 << PDMA_DSCT_CTL_DASEL_Pos;    //Transfer Destination Address is wrapped.
	PDMA->DSCT_CTL |= 0x2 << PDMA_DSCT_CTL_TXWIDTH_Pos;  //One half-word (16 bits) is transferred for every PDMA operation
	PDMA->DSCT_CTL |= 0x1 << PDMA_DSCT_CTL_MODESEL_Pos;  //Memory to IP mode (APB-to-SRAM).
	PDMA->DSCT_CTL |= 0x5 << PDMA_DSCT_CTL_WAINTSEL_Pos; //Wrap Interrupt: Both half and end buffer.
	
	PDMA->TXBCCH = BUFFER_LENGTH*2;          // Audio array total length, unit: sample.
	
	PDMA->INTENCH = 0x1 << PDMA_INTENCH_WAINTEN_Pos;   //Wraparound Interrupt Enable
	
	ADC_ENABLE_PDMA(ADC);
	
	NVIC_ClearPendingIRQ(PDMA_IRQn);
	NVIC_EnableIRQ(PDMA_IRQn);
	PDMA->DSCT_CTL |= PDMA_DSCT_CTL_TXEN_Msk;    //Start PDMA transfer
}

void Led_Enable(uint16_t u16Led)
{
    switch(u16Led)
	{
		case 0:
			GPIO_SET_OUT_DATA(LED0_PORT, ~LED0_PIN_MASK);
		    break;
		case 1:
			GPIO_SET_OUT_DATA(LED1_PORT, ~LED1_PIN_MASK);
		    break;
		case 2:
			GPIO_SET_OUT_DATA(LED2_PORT, ~LED2_PIN_MASK);
		    break;
		case 3:
			GPIO_SET_OUT_DATA(LED3_PORT, ~LED3_PIN_MASK);
		    break;
		case 4:
			GPIO_SET_OUT_DATA(LED4_PORT, ~LED4_PIN_MASK);
		    break;
		case 5:
			GPIO_SET_OUT_DATA(LED5_PORT, ~LED5_PIN_MASK);
		    break;
		default:
			GPIO_SET_OUT_DATA(LED0_PORT,(GPIO_GET_OUT_DATA(LED0_PORT)&~LED0_PIN_MASK));
		    GPIO_SET_OUT_DATA(LED1_PORT,(GPIO_GET_OUT_DATA(LED1_PORT)&~LED1_PIN_MASK));
		    GPIO_SET_OUT_DATA(LED2_PORT,(GPIO_GET_OUT_DATA(LED2_PORT)&~LED2_PIN_MASK));
		    GPIO_SET_OUT_DATA(LED3_PORT,(GPIO_GET_OUT_DATA(LED3_PORT)&~LED3_PIN_MASK));
		    GPIO_SET_OUT_DATA(LED4_PORT,(GPIO_GET_OUT_DATA(LED4_PORT)&~LED4_PIN_MASK));
		    GPIO_SET_OUT_DATA(LED5_PORT,(GPIO_GET_OUT_DATA(LED5_PORT)&~LED5_PIN_MASK));
	}
}

void Led_Disable()
{
	GPIO_SET_OUT_DATA(LED0_PORT,LED0_PIN_MASK);
	GPIO_SET_OUT_DATA(LED1_PORT,LED1_PIN_MASK);
    GPIO_SET_OUT_DATA(LED2_PORT,LED2_PIN_MASK);
	GPIO_SET_OUT_DATA(LED3_PORT,LED3_PIN_MASK);
	GPIO_SET_OUT_DATA(LED4_PORT,LED4_PIN_MASK);
	GPIO_SET_OUT_DATA(LED5_PORT,LED5_PIN_MASK);
}

//KeyHandler
void KpHandler(uint32_t u32Param)
{
/*
	UINT32 u32Data;
	UINT32 u32Index;
	UINT32 u32Addr;
*/
	
	INT32  i32Ret;
	UINT32 u32LParam = u32Param;

	if(u32LParam == SWB0_P)
	{
	    if (gCtrlTmr1Cnt == 0 || s_sRecordCtrlHandle.u16FullFg > 0)
	    {
		    u32LParam = SWB0_R;
	    }
    }
		
	switch(u32LParam)
	{
		case SWB0_F:  
            printf ("[SWB0] Recorder init  sampling....\n"); 
		
		    if(s_sRecordCtrlHandle.u16TotCmd <= s_sRecordCtrlHandle.u16CurCmd)
	        {
		        printf("Cmd Buf have been Full, Can't Allow to continue Record \n");
				return;
	        }
			
	        // Init ADC 
	        ADC_Init();
	
	        // Init PDMA
	        PDMA_Init();	
			
			// Init Flash
			open_Flash();
		
		    gCtrlAdcConv = 0;
			gRecordDelay = 0;
			gCtrlTmr1Cnt = TIMER1_MAXPRSCNT;
		
			s_sRecordCtrlHandle.u32CurBeg = s_sRecordCtrlHandle.u32RecBeg;
		    s_sRecordCtrlHandle.u32RecCnt = 0;
		    s_sRecordCtrlHandle.u16FullFg = 0;
			s_sRecordCtrlHandle.u16StopFg = 0;
			s_sRecordCtrlHandle.u16TmrCnt = 0;
			
			// Init Flash
			Flash_Erase(s_sRecordCtrlHandle.u32RecBeg, s_sRecordCtrlHandle.u32RecEnd);	
		   
		    break;
			
	    case SWB0_R:  
			if(s_sRecordCtrlHandle.u16StopFg > 0)
			    break;
			else
				s_sRecordCtrlHandle.u16StopFg = 1;
			
			printf ("[SWB0] Recorder stop  sampling....\n");  
		
			//Clear ADC
		    PDMA->INTENCH = 0x1 << PDMA_INTENCH_WAINTEN_Pos;
			ADC_DISABLE_PDMA(ADC);
            ADC_STOP_CONV(ADC);
	
	        NVIC_ClearPendingIRQ(PDMA_IRQn);
	        NVIC_DisableIRQ(PDMA_IRQn);	
			
			s_sRecordCtrlHandle.u16TmrCnt = (TIMER1_MAXPRSCNT - gCtrlTmr1Cnt);
			
		    //check result
		    //printf ("s_sRecordCtrlHandle.u16CurCmd %hd\n", s_sRecordCtrlHandle.u16CurCmd); 
            //printf ("s_sRecordCtrlHandle.u32RecCnt %d\n",  s_sRecordCtrlHandle.u32RecCnt); 
			//printf ("s_sRecordCtrlHandle.u16TmrCnt %d\n",  s_sRecordCtrlHandle.u16TmrCnt); 
            //printf ("s_sRecordCtrlHandle.u32CurBeg %d\n",  s_sRecordCtrlHandle.u32CurBeg);  
			//printf ("s_sRecordCtrlHandle.u32RecBeg %d\n",  s_sRecordCtrlHandle.u32RecBeg); 
			//printf ("s_sRecordCtrlHandle.u32RecEnd %d\n",  s_sRecordCtrlHandle.u32RecEnd); 
			
			//u32Addr = s_sRecordCtrlHandle.u32RecBeg;
            //for	(u32Index = 0; u32Index < s_sRecordCtrlHandle.u32RecCnt; u32Index++)
            //{
			//	u32Addr += 4;
			//	u32Data = Flash_Read(u32Addr);
				
			//	printf("u32Addr=%d u32Data=%d\n", u32Addr, u32Data);
			//}			

			
			//Adc Ctrl
            if(gCtrlAdcConv > 0)
			{
				if(s_sRecordCtrlHandle.u32RecCnt > RECORD_MINADCCNT)
			    {
                    i32Ret = Record_Process(s_sRecordCtrlHandle.u32RecBeg, s_sRecordCtrlHandle.u32RecCnt, s_sRecordCtrlHandle.u16CurCmd);
			
			        if(i32Ret == 0)
			        {
				        s_sRecordCtrlHandle.u16CurCmd++;
			        }
		        }
			}  
			
			stop_Flash();

		    break;
		
		case SWB0_P:       			
            if(gCtrlAdcConv == 0)	
			{
				printf ("[SWB0] Recorder Adc init sampling....\n");  
						
			    ADC_START_CONV(ADC);
				
				gCtrlAdcConv = 1;
			}
			
		    break;
			
		case SWB1_F:  
			printf ("PB0(SWB1) is falling state.\n"); 
		
			if(s_sRecordCtrlHandle.u16CurCmd > 0)
			    s_sRecordCtrlHandle.u16CurCmd--;
		   
		    break;
	    case SWB1_R:   printf ("PB0(SWB1) is rising state.\n");     break;
		case SWB1_P:   printf ("PB0(SWB1) is pressing state.\n");   break;
		
		
		case SWB2_F:  
            printf ("[SWB2] Recorder init  sampling....\n"); 
			
	        // Init ADC 
	        ADC_Init();
	
	        // Init PDMA
	        PDMA_Init();

            // Init Flash
            open_Flash();		
		
		    gCtrlAdcConv = 0;
		    gRecordDelay = 0;
			gCtrlTmr1Cnt = TIMER1_MAXPRSCNT;
		
			s_sRecordCtrlHandle.u32CurBeg = s_sRecordCtrlHandle.u32RecBeg;
		    s_sRecordCtrlHandle.u32RecCnt = 0;
		    s_sRecordCtrlHandle.u16FullFg = 0;
			s_sRecordCtrlHandle.u16StopFg = 0;
			s_sRecordCtrlHandle.u16TmrCnt = 0;
			
			// Init Flash
			Flash_Erase(s_sRecordCtrlHandle.u32RecBeg, s_sRecordCtrlHandle.u32RecEnd);	
		   
		    break;
			
	    case SWB2_R:  
			if(s_sRecordCtrlHandle.u16StopFg > 0)
			    break;
			
			printf ("[SWB2] Recorder stop  sampling....\n");  
		
			//Clear ADC
		    PDMA->INTENCH = 0x1 << PDMA_INTENCH_WAINTEN_Pos;
			ADC_DISABLE_PDMA(ADC);
            ADC_STOP_CONV(ADC);
	
	        NVIC_ClearPendingIRQ(PDMA_IRQn);
	        NVIC_DisableIRQ(PDMA_IRQn);	
			
			s_sRecordCtrlHandle.u16TmrCnt = (TIMER1_MAXPRSCNT - gCtrlTmr1Cnt);
			
		    //check result
		    //printf ("s_sRecordCtrlHandle.u16CurCmd %hd\n", s_sRecordCtrlHandle.u16CurCmd); 
            //printf ("s_sRecordCtrlHandle.u32RecCnt %d\n",  s_sRecordCtrlHandle.u32RecCnt); 
			//printf ("s_sRecordCtrlHandle.u16TmrCnt %d\n",  s_sRecordCtrlHandle.u16TmrCnt); 
            //printf ("s_sRecordCtrlHandle.u32CurBeg %d\n",  s_sRecordCtrlHandle.u32CurBeg);  
			//printf ("s_sRecordCtrlHandle.u32RecBeg %d\n",  s_sRecordCtrlHandle.u32RecBeg); 
			//printf ("s_sRecordCtrlHandle.u32RecEnd %d\n",  s_sRecordCtrlHandle.u32RecEnd); 
			
			//u32Addr = s_sRecordCtrlHandle.u32RecBeg;
            //for	(u32Index = 0; u32Index < s_sRecordCtrlHandle.u32RecCnt; u32Index++)
            //{
			//	u32Addr += 4;
			//	u32Data = Flash_Read(u32Addr);
				
			//	printf("u32Addr=%d u32Data=%d\n", u32Addr, u32Data);
			//}			
			
			//Adc Ctrl
            if(gCtrlAdcConv > 0)
			{
				s_sRecordCtrlHandle.u16StopFg++;
				
				if(s_sRecordCtrlHandle.u32RecCnt > RECORD_MINADCCNT)
			    {
                    i32Ret = Recognize_Process(s_sRecordCtrlHandle.u32RecBeg, s_sRecordCtrlHandle.u32RecCnt, DATA_FLASH_MFCCTG_MAX);
					if(i32Ret > 0)
					{
					    //Led_Enable(i32Ret);
						printf("Result Cmd[%d]\n", i32Ret);
					}
					else
					{
						printf("Result Cmd fail\n");
					}
		        }
			}  

			stop_Flash();
		    break;
		
		case SWB2_P: 
            if(gCtrlAdcConv == 0)	
			{
				printf ("[SWB2] Recorder Adc init sampling....\n");  
						
			    ADC_START_CONV(ADC);
				
				gCtrlAdcConv = 1;
			}
			
		    break;
			
		case SWB3_F:
			printf ("[SWB3] Reset System: starting...\n");   
			s_sRecordCtrlHandle.u16TotCmd = DATA_FLASH_MFCCTG_MAX;
   		    s_sRecordCtrlHandle.u32RecBeg = DATA_FLASH_RECORD_BEG;
			s_sRecordCtrlHandle.u32RecEnd = DATA_FLASH_RECORD_END;
			s_sRecordCtrlHandle.u32CurBeg = DATA_FLASH_RECORD_BEG;
			s_sRecordCtrlHandle.u16CurCmd = 0;
		    s_sRecordCtrlHandle.u32RecCnt = 0;
		    s_sRecordCtrlHandle.u16FullFg = 0;
		    s_sRecordCtrlHandle.u16StopFg = 0;
		    s_sRecordCtrlHandle.u16TmrCnt = 0;
		
		    Flash_Erase(DATA_FLASH_RECORD_BEG, DATA_FLASH_MFCCRG_END);
 
		    break;
	    case SWB3_R:   
			printf ("[SWB3] Reset System: complete...\n");  
		
		    break;
		case SWB3_P:
			printf ("[SWB3] Reset System: processing...\n");  
		
		    break;
		case SWB4_F:
			//Led_Enable(s_sRecordCtrlHandle.u16CurCmd);
			break;
		case SWB4_R:
			break;
		case SWB4_P:
			break;
		case SWB5_F:
			Led_Disable();
			break;
		case SWB5_R:
			break;
		case SWB5_P:
			break;
		default:
			printf ("Please press key: \n");
     		printf ("Record         --> SWB0 \n");
	    	printf ("BackStep       --> SWB1 \n");
		    printf ("SpeechReco     --> SWB2 \n");
		    printf ("Reset System   --> SWB3 \n");
		    printf ("Enable  RecLed --> SWB4 \n");
		    printf ("Disable RecLed --> SWB5 \n");
	}
	
}

//Init_InKey
void Init_InKey(void)
{
	uint32_t u32Addr;
	int32_t  i32Data;
	uint32_t i;
	
	// GPIO pin input&output initiate.
	GPIO_SetMode(PA, TRIGGER_PORTA_PINS_MASK, GPIO_MODE_QUASI);	
	GPIO_SetMode(PB, TRIGGER_PORTB_PINS_MASK, GPIO_MODE_QUASI);		
	GPIO_SetMode(PA, 0x0, GPIO_MODE_OUTPUT);	
	GPIO_SetMode(PA, 0x0, GPIO_MODE_OUTPUT);	
	
	// TMR1_MODULE
    CLK_EnableModuleClock(TMR1_MODULE);
	SYS_ResetModule(TMR1_RST);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_LIRC, NULL);
	TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, TIMER1_FREQUENCY);
	TIMER_EnableInt(TIMER1);
	NVIC_EnableIRQ(TMR1_IRQn);	
	TIMER_Start(TIMER1);

	//Change debounce time unit to second.
	Keypad_InitKeypad(KEY_DEBOUNCE_TIME*TIMER_GetWorkingFreq(TIMER1),KEY_PRESS_TIME*TIMER_GetWorkingFreq(TIMER1));
	
	// Default input state.
	Keypad_InitTgr(TRIGGER_PORTA_PINS_MASK, TRIGGER_PORTB_PINS_MASK, g_asTrgKeyHandler);
	
	// Init s_sRecordCtrlHandle
	s_sRecordCtrlHandle.u16TotCmd = DATA_FLASH_MFCCTG_MAX;
    s_sRecordCtrlHandle.u32RecBeg = DATA_FLASH_RECORD_BEG;
	s_sRecordCtrlHandle.u32RecEnd = DATA_FLASH_RECORD_END;
	s_sRecordCtrlHandle.u32CurBeg = DATA_FLASH_RECORD_BEG;
	s_sRecordCtrlHandle.u16CurCmd = 0;
	
	//Init CurCmd
	for(i = 0; i < DATA_FLASH_MFCCTG_MAX; i++)
	{
		u32Addr = DATA_FLASH_MFCCTG_BEG + i * DATA_FLASH_VALBUF_SIZ;
		i32Data = Flash_Read(u32Addr);
		
		if(i32Data < 0)
		{
			s_sRecordCtrlHandle.u16CurCmd = i + 1;
			break;
		}
	}
}


