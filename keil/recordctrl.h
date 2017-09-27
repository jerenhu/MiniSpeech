#ifndef __RECORDCTRL_H__
#define __RECORDCTRL_H__
#include "Platform.h"

/*ADC*/
#define CH                          0
#define PDMA                        PDMA0
#define PGA_GAIN                    0        //default
#define ADC_SAMPLE_RATE             (4000)   //default
#define FRAME_SZIE                  (8)
#define BUFFER_LENGTH               (FRAME_SZIE*2)

/*SINGLE RECORD MAX SEC*/
#define ADC_SINGLE_RECORD_MAX       5   
#define ADC_SINGLE_RECORD_MIN       1   
#define ADC_SINGLE_DELAYS_SEC       300         //ms 
#define ADC_SINGLE_DELAYS_CNT       (ADC_SINGLE_DELAYS_SEC * ADC_SAMPLE_RATE / 1000)

/*KeyPad*/
#define KEY_DEBOUNCE_TIME		    (0.1000)	// unit:s
#define KEY_PRESS_TIME			    (0.1000)	// unit:s
#define TRIGGER_PORTA_PINS_MASK		(0)
#define TRIGGER_PORTB_PINS_MASK		(BIT0|BIT1|BIT2|BIT3|BIT6|BIT7)
#define TIMER1_FREQUENCY      		(200)
#define TIMER1_MAXPRSCNT            (ADC_SINGLE_RECORD_MAX * TIMER1_FREQUENCY)
#define RECORD_MINADCCNT            (ADC_SINGLE_RECORD_MIN * ADC_SAMPLE_RATE)
#define SWB0_F			            0x10000001
#define SWB0_R			            0x20000001
#define SWB0_P			            0x40000001
#define SWB1_F			            0x10000002
#define SWB1_R		            	0x20000002
#define SWB1_P		            	0x40000002
#define SWB2_F		            	0x10000003
#define SWB2_R		            	0x20000003
#define SWB2_P		            	0x40000003
#define SWB3_F		            	0x10000004
#define SWB3_R		            	0x20000004
#define SWB3_P		            	0x40000004
#define SWB4_F		            	0x10000005
#define SWB4_R		            	0x20000005
#define SWB4_P		            	0x40000005
#define SWB5_F		            	0x10000006
#define SWB5_R		            	0x20000006
#define SWB5_P		            	0x40000006

//LED
#define LED0_PORT  				    PA
#define LED0_PIN_MASK			    BIT12
#define LED1_PORT			    	PA
#define LED1_PIN_MASK			    BIT13
#define LED2_PORT				    PA
#define LED2_PIN_MASK			    BIT14
#define LED3_PORT				    PA
#define LED3_PIN_MASK			    BIT15
#define LED4_PORT			    	PB
#define LED4_PIN_MASK			    BIT4
#define LED5_PORT			    	PB
#define LED5_PIN_MASK		    	BIT5

/*RECORD COMMAND MAX CNT*/
#define DATA_FLASH_MFCCTG_MAX       6

/*FLASH DATA SPACE DISTRIBUTION*/
#define DATA_FLASH_MFCCRG_END       0x23400
#define DATA_FLASH_MFCCRG_BEG       0x22400
#define DATA_FLASH_MFCCTG_END       0x22400
#define DATA_FLASH_MFCCTG_BEG       0x1C400
#define DATA_FLASH_RECORD_END       0x1C400
#define DATA_FLASH_RECORD_BEG       0x07800
#define DATA_FLASH_VALBUF_SIZ       0x01000

void PDMA_IRQHandler(void);
void ADC_IRQHandler(void);
void TMR1_IRQHandler(void);
void SYS_Init(void);
void UART_Init(void);
void ADC_Init(void);
void PDMA_Init(void);
void KpHandler(uint32_t u32Param);
void Init_InKey(void);



#endif
