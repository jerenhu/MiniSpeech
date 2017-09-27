#ifndef __VADCTRL_H__
#define __VADCTRL_H__

//#define VAD_BKGRO_CNT     2400
#define VAD_BKGRO_CNT     2400
#define VAD_SILEN_CNT     1200
#define VAD_MINSP_CNT     640
#define VAD_FRAME_LEN     160
#define VAD_FRAME_MOV     80

#define VAD_NTHL_RATIO    1
#define VAD_STHL_RATIO    11/10    /* 11/10 */
#define VAD_ZTHL_RATIO    2/160    /* 2/160 */

#define FFT_MEM_END       0x20003000
#define FFT_MEM_BEG       0x20002000
#define FRQ_MEM_END       0x20002000
#define FRQ_MEM_BEG       0x20001C00

#define MFC_PRE_WTH 	  95/100			
#define MFC_FFT_PRC       512             
#define MFC_FRQ_MAX		  MFC_FFT_PRC/2     
#define HAMM_TOP	      10000			
#define	TRI_TOP		      1000			
#define MFC_TRI_NUM		  24				
#define MFC_ORD_NUM	      12				
#define PI                3.14159f    

#define MFC_MAX_DIS       -1 
#define MFC_INV_CMD       -1  

int16_t Record_Process(uint32_t u32RecAddr, uint16_t u16RecCnt, uint16_t u16Cmd);
int16_t Recognize_Process(uint32_t u32RecAddr, uint16_t u16RecCnt, uint16_t u16Cmd);

#endif

