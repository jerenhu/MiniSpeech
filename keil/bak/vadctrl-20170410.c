#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Platform.h"
#include "recordctrl.h"
#include "vadctrl.h"
#include "flash.h"

typedef struct 
{
    int16_t  midd;       
    int16_t  nthr;       
    int16_t  zthr;       
    int32_t  sthr;       
} S_BACKGROUD_CTRL_HANDLE;  

typedef struct 
{
    uint32_t  u32BegAddr;
    uint32_t  u32EndAddr;
	uint32_t  u32CurAddr;
} S_VOICEADDR_CTRL_HANDLE;  

typedef struct 
{
    uint32_t  cmd;       
    uint32_t  frames;  	
    uint32_t  begaddr;  
    uint32_t  endaddr;	
} S_BLOCKADDR_CTRL_HANDLE;  

typedef struct 
{ 
    uint32_t  u32BegAddr;       
    uint32_t  u32EndAddr;             
} S_BOUNDADDR_CTRL_HANDLE;  

typedef struct
{
	float real;
	float img;
}complex;

typedef enum {
	E_DIRCT_RIGHT,
	E_DIRCT_UP,
	E_DIRCT_INCLINE,
    E_DIRCT_ERR
} DIRECTION;

volatile __align(4) int16_t   i16RecFrmData[VAD_FRAME_LEN];

volatile static S_BACKGROUD_CTRL_HANDLE s_sBackgroudCtrlHandle;
volatile static S_VOICEADDR_CTRL_HANDLE s_sVoiceaddrCtrlHandle;
volatile static S_BLOCKADDR_CTRL_HANDLE s_sBlockaddrCtrlHandle;
volatile static S_BOUNDADDR_CTRL_HANDLE s_sBoundaddrCtrlHandle;

/*256
const int16_t MFC_TRG_CENTER[]=
{
    2, 4, 6, 8, 11, 13, 16, 19, 23, 26, 30, 34, 38, 43, 48, 54, 60, 66, 73, 81, 89, 98, 107, 117
};
*/

///*512
const int16_t MFC_TRG_CENTER[]=
{
    4, 8, 12, 16, 21, 26, 32, 38, 45, 52, 59, 67, 76, 86, 96, 107, 119, 132, 146, 161, 177, 195, 214, 234
};
//*/

void procNoise(uint32_t u32Addr, uint32_t u32Cnt)
{
	int32_t sumval = 0;
	int32_t sumabs = 0;
    int32_t absval = 0;
    int32_t maxval = 0;
	
	int32_t i;
	int32_t j;
	
	int32_t u32BkAddr;
	int32_t u32BkData;
	
	memset((void*)&s_sVoiceaddrCtrlHandle, 0x0, sizeof(s_sVoiceaddrCtrlHandle));
	
	s_sVoiceaddrCtrlHandle.u32BegAddr = u32Addr;
	s_sVoiceaddrCtrlHandle.u32EndAddr = u32Addr + 4 * u32Cnt;
	s_sVoiceaddrCtrlHandle.u32CurAddr = u32Addr;
	
	s_sBackgroudCtrlHandle.midd = 0;
	s_sBackgroudCtrlHandle.nthr = 0;
	s_sBackgroudCtrlHandle.zthr = 0;
	s_sBackgroudCtrlHandle.sthr = 0;
	
	for(i = 0; i < VAD_BKGRO_CNT; i++)
    {
		u32BkAddr = s_sVoiceaddrCtrlHandle.u32CurAddr + 4 * i;
		u32BkData = Flash_Read(u32BkAddr);
        sumval += u32BkData;
    }

    s_sBackgroudCtrlHandle.midd = sumval / VAD_BKGRO_CNT;

    sumval = 0;
    for(i = 0; i < VAD_BKGRO_CNT; i += VAD_FRAME_LEN)
    {
    	for(j = 0; j < VAD_FRAME_LEN; j++)
	    {
		    u32BkAddr = s_sVoiceaddrCtrlHandle.u32CurAddr + 4 * i + 4 * j;
			u32BkData = Flash_Read(u32BkAddr);
			
			if(u32BkData > s_sBackgroudCtrlHandle.midd)
				absval = u32BkData - s_sBackgroudCtrlHandle.midd;
			else
                absval = s_sBackgroudCtrlHandle.midd - u32BkData;

			if(absval < 0)
			{
				printf("Error: procNoise midd=%d val=%d\n", s_sBackgroudCtrlHandle.midd, u32BkData);
			}
			
	        if(absval > maxval)
	            maxval = absval;
			
	        sumabs += absval;
	    }
	    sumval += maxval;
	}

    s_sBackgroudCtrlHandle.nthr = sumval / (VAD_BKGRO_CNT/VAD_FRAME_LEN) * VAD_NTHL_RATIO;
    s_sBackgroudCtrlHandle.sthr = sumabs / (VAD_BKGRO_CNT/VAD_FRAME_LEN) * VAD_STHL_RATIO;
    s_sBackgroudCtrlHandle.zthr = VAD_FRAME_LEN * VAD_ZTHL_RATIO / VAD_NTHL_RATIO;

    /*
	printf("ZHOUYANHUI: [BackGroud] Check\n");
    printf("s_sBackgroudCtrlHandle.midd = %d\n", s_sBackgroudCtrlHandle.midd);
    printf("s_sBackgroudCtrlHandle.nthr = %d\n", s_sBackgroudCtrlHandle.nthr);
    printf("s_sBackgroudCtrlHandle.sthr = %d\n", s_sBackgroudCtrlHandle.sthr);
    printf("s_sBackgroudCtrlHandle.zthr = %d\n", s_sBackgroudCtrlHandle.zthr);
    */
	
	s_sVoiceaddrCtrlHandle.u32CurAddr += 4 * VAD_BKGRO_CNT;
}

uint16_t cal_zeros(uint32_t u32EndAddr)
{
	int16_t  v_zeros = 0;
	int32_t  u32Addr = u32EndAddr - 4 * VAD_FRAME_LEN;
	int32_t  u32Data1;
	int32_t  u32Data2;

	while(u32Addr < u32EndAddr)
	{
		u32Data1 = Flash_Read(u32Addr);
		u32Data2 = Flash_Read((u32Addr + 4));
		if((u32Data1 - (s_sBackgroudCtrlHandle.nthr + s_sBackgroudCtrlHandle.midd)) > 0 && (u32Data2 + (s_sBackgroudCtrlHandle.nthr - s_sBackgroudCtrlHandle.midd)) < 0)
		{
			v_zeros++;
		}

		if((u32Data1 + (s_sBackgroudCtrlHandle.nthr - s_sBackgroudCtrlHandle.midd)) < 0 && (u32Data2 - (s_sBackgroudCtrlHandle.nthr + s_sBackgroudCtrlHandle.midd)) > 0)
		{
			v_zeros++;
		}
		
		u32Addr += 4;
	}

	return v_zeros;
}

uint32_t cal_sumabs(uint32_t u32EndAddr)
{
	uint32_t  v_sumabs = 0;
	uint32_t  u32Addr = u32EndAddr - 4 * VAD_FRAME_LEN;
	int32_t   u32Data;
	int32_t   u32AbsVal;

	while(u32Addr < u32EndAddr)
	{
		u32Data = Flash_Read(u32Addr);
		if(u32Data > s_sBackgroudCtrlHandle.midd)
			u32AbsVal = u32Data - s_sBackgroudCtrlHandle.midd;
		else
			u32AbsVal = s_sBackgroudCtrlHandle.midd - u32Data;
		
		if(u32AbsVal < 0)
			printf("Error: cal_sumabs\n");
		
		v_sumabs += u32AbsVal;
		u32Addr += 4;
	}

	return v_sumabs;
}

int16_t vad(void)
{
    uint32_t   u32Addr;
	uint32_t   zeros;
	uint32_t   sumabs;
	uint32_t   silence;
	int32_t    i32Ret = 0;
	
	s_sBoundaddrCtrlHandle.u32BegAddr = 0;
	s_sBoundaddrCtrlHandle.u32EndAddr = 0;
	
	u32Addr = s_sVoiceaddrCtrlHandle.u32CurAddr + 4 * VAD_FRAME_LEN;
	
	while(u32Addr < s_sVoiceaddrCtrlHandle.u32EndAddr)
	{
	    zeros  =  cal_zeros(u32Addr);
		sumabs =  cal_sumabs(u32Addr);
		
		if(zeros > s_sBackgroudCtrlHandle.zthr || sumabs > s_sBackgroudCtrlHandle.sthr)
		{
			if(!s_sBoundaddrCtrlHandle.u32BegAddr)
			{
				s_sBoundaddrCtrlHandle.u32BegAddr = u32Addr - 4 * VAD_FRAME_LEN;
			}
			silence = 0;
		}
		else
		{
			//printf("ZHOUYANHUI: i=%d zeros %d>%d sumabs %d>%d\n", i++, zeros, s_sBackgroudCtrlHandle.zthr, sumabs, s_sBackgroudCtrlHandle.sthr);
			if(s_sBoundaddrCtrlHandle.u32BegAddr)
			{
				if((u32Addr - s_sBoundaddrCtrlHandle.u32BegAddr) < 4 * (VAD_MINSP_CNT + VAD_FRAME_LEN))
				{
					s_sBoundaddrCtrlHandle.u32BegAddr  = 0;
					s_sBoundaddrCtrlHandle.u32EndAddr  = 0;
				}
				else
				{
					silence += VAD_FRAME_LEN;
					if(silence >= VAD_SILEN_CNT)
					{
						s_sBoundaddrCtrlHandle.u32EndAddr = u32Addr - 4 * silence;
						break;
					}
				}
			}
		}
	    u32Addr += 4 * VAD_FRAME_LEN;
	}
	/*
	printf("ZHOUYANHUI: [BackGroud] Check\n");
    printf("s_sBackgroudCtrlHandle.midd = %d\n", s_sBackgroudCtrlHandle.midd);
    printf("s_sBackgroudCtrlHandle.nthr = %d\n", s_sBackgroudCtrlHandle.nthr);
    printf("s_sBackgroudCtrlHandle.sthr = %d\n", s_sBackgroudCtrlHandle.sthr);
    printf("s_sBackgroudCtrlHandle.zthr = %d\n", s_sBackgroudCtrlHandle.zthr);
    */
	
	if(!s_sBoundaddrCtrlHandle.u32EndAddr)
	{
		printf("Vad Check Result: FAIL\n");
	    printf("BegAddr = %d\n", s_sBoundaddrCtrlHandle.u32BegAddr);
	    printf("EndAddr = %d\n", s_sBoundaddrCtrlHandle.u32EndAddr);
		i32Ret = -1;
	}
	else
	{
		printf("Vad Check Result: SUCCESS\n");
	    printf("BegAddr = %d\n", s_sBoundaddrCtrlHandle.u32BegAddr);
	    printf("EndAddr = %d\n", s_sBoundaddrCtrlHandle.u32EndAddr);
	}
		
	return i32Ret;
}

void add_hamm(int16_t *i16FrmData)
{
	int16_t  i;
	int32_t  temp;

	for(i=1; i < VAD_FRAME_LEN; i++)
	{
		temp = (i16FrmData[i] - i16FrmData[i-1]) * MFC_PRE_WTH;
		i16FrmData[i] = (int16_t)(temp * ((1 - 0.46f) - 0.46f * cos(2 * PI * i / (VAD_FRAME_LEN -1))) /** 10*/);
	}
}

complex add(complex a,complex b)
{
    complex c;
    c.real = a.real + b.real;
    c.img  = a.img  + b.img;

    return c;
}

complex sub(complex a,complex b)
{
    complex c;
    c.real = a.real - b.real;
    c.img  = a.img  - b.img;

    return c;
}

complex mul(complex a,complex b)
{
    complex c;
    c.real = a.real * b.real - a.img  * b.img;
    c.img  = a.img  * b.real + a.real * b.img;

    return c;
}

void Reverse(complex* data, int16_t N)
{
	int16_t i;
	int16_t j;
	int16_t k;
	int16_t t;
	int16_t m;
	int16_t rank = 0;
	complex tmp;

    for(m = 1; m < N; m <<= 1)
    {
    	rank++;
    }

    for(i = 0; i < N; i++)
    {
        k = i;
        j = 0;

        for(t = 0; t < rank; t++)
        {
            j <<= 1;
            j  |= (k&1);
            k >>= 1;
        }

        if(j > i)
        {
        	tmp.real = (data + i)->real;
        	tmp.img  = (data + i)->img;

        	(data + i)->real = (data + j)->real;
        	(data + i)->img  = (data + j)->img;
        	(data + j)->real = tmp.real;
        	(data + j)->img  = tmp.img;
        }
    }
}

complex get_Wn(int16_t k, int16_t N)
{
	complex wn;
	wn.real = cos(2 * PI / N * k);
	wn.img  = -sin(2 * PI / N * k);

	return wn;
}

void calc_fft(complex* data, int16_t N)
{
	int i;
	int j;
	int k;
	int l;
	int m;
	int16_t rank = 0;
	complex wn;
	complex wx;
	complex top;
	complex bot;

	for(m = 1; m < N; m <<= 1)
	{
		rank++;
	}

	Reverse(data, N);

	for(i = 0; i < rank; i++)
	{
		l = 1 << i;
		for(j = 0; j < N; j += 2 * l)
		{
			for(k = 0; k < l; k++)
			{
				wn  = get_Wn(k, 2 * l);
				wx  = mul(*(data + j + k + l), wn);
				top = add(*(data + j + k), wx);
				bot = sub(*(data + j + k), wx);

				(data + j + k)->real     = top.real;
				(data + j + k)->img      = top.img;
				(data + j + k + l)->real = bot.real;
				(data + j + k + l)->img  = bot.img;
			}
		}
	}
}

int32_t get_cent(uint32_t* v_buf, const int16_t low, const int16_t mid, const int16_t hig)
{
	int32_t i;
	int32_t pow_cen = 0;

	for(i = low; i < hig; i++)
	{
		if(mid - i > 0)
		{
			pow_cen += *(v_buf+i) * ((i - low) / (mid - low));
		}
		else if(mid - i < 0)
		{
			pow_cen += *(v_buf+i) * ((hig - i) / (hig - mid));
		}
		else
		{
			pow_cen += *(v_buf+i);
		}
	}
	//printf("%d ", pow_cen);

	return pow_cen;
}

void add_trig(uint32_t* v_fft, const int16_t* v_tricen, uint32_t* v_pow)
{
	uint16_t i;
	
	for(i=0; i< MFC_TRI_NUM; i++)
	{
		if(i == 0)
		{
			*(v_pow+i) = get_cent(v_fft, 0, *(v_tricen+i), *(v_tricen+i+1));
		}
		else if(i == MFC_TRI_NUM - 1)
		{
			*(v_pow+i) = get_cent(v_fft, *(v_tricen+i-1), *(v_tricen+i), MFC_FRQ_MAX);
		}
		else
		{
			*(v_pow+i) = get_cent(v_fft, *(v_tricen+i-1), *(v_tricen+i), *(v_tricen+i+1));
		}
	}
}

void get_loge(uint32_t* v_buf)
{
	uint16_t i;
	
	for(i=0; i < MFC_TRI_NUM; i++)
	{
		//printf("bef:%d ", *(v_buf+i));
		*(v_buf+i) = (uint32_t)(log(*(v_buf+i)) * 100);
		//printf("aft:%d ", *(v_buf+i));
	}
}

void get_coef(uint32_t* v_buf, uint32_t* v_cos)
{
	uint16_t i;
	uint16_t j;
	
	for(i = 0; i < MFC_ORD_NUM; i++)
	{
		for(j = 0; j < MFC_TRI_NUM; j++)
		{
			*(v_cos+i) += *(v_buf+j) * sqrt(2.0f/MFC_TRI_NUM) * cos((j-0.5f) * i * PI / MFC_TRI_NUM);
		}

		//printf(" mfcc:%d * ", *(v_cos+i));
	}
}

int16_t mfcc(uint16_t u16Cmd)
{
	uint32_t    u32Addr;
	uint32_t    u32FrmAddr;
	uint32_t    u32FrmData;
	uint32_t    u32FrqData;
	uint32_t    u32MfcAddr;
	
	complex*    complxFftArr = 0;
	uint32_t*   u32FrqArr    = 0;
	
	uint32_t    u32TRGCenArr[MFC_TRI_NUM];
	uint32_t    u32MFCCFgArr[MFC_ORD_NUM];
	
	int16_t     i;
	int16_t     i16Ret = 0;
	
	memset((void*)&s_sBlockaddrCtrlHandle, 0x0, sizeof(s_sBlockaddrCtrlHandle));
	s_sBlockaddrCtrlHandle.cmd     = u16Cmd;
	s_sBlockaddrCtrlHandle.frames  = 0;
	s_sBlockaddrCtrlHandle.begaddr = DATA_FLASH_MFCCTG_BEG + s_sBlockaddrCtrlHandle.cmd * DATA_FLASH_VALBUF_SIZ;
	s_sBlockaddrCtrlHandle.endaddr = s_sBlockaddrCtrlHandle.begaddr + DATA_FLASH_VALBUF_SIZ;
	u32MfcAddr = s_sBlockaddrCtrlHandle.begaddr + 8;
	
	Flash_Erase(s_sBlockaddrCtrlHandle.begaddr, s_sBlockaddrCtrlHandle.endaddr);
	
	u32Addr = s_sBoundaddrCtrlHandle.u32BegAddr + 4 * VAD_FRAME_LEN;
	while(u32Addr < s_sBoundaddrCtrlHandle.u32EndAddr)
	{
		memset((void*)i16RecFrmData, 0x0, sizeof(i16RecFrmData));
		for(i = 0; i < VAD_FRAME_LEN; i++)
		{
			u32FrmAddr = u32Addr - 4 * (VAD_FRAME_LEN - i);
			u32FrmData = Flash_Read(u32FrmAddr);
			i16RecFrmData[i] = u32FrmData;
			//printf("Before hamm: %d \n", i16RecFrmData[i]);
		}
		
		add_hamm(i16RecFrmData);
		
		complxFftArr = (complex*)FFT_MEM_BEG;
	    memset((void*)complxFftArr, 0x0, sizeof(complex) * MFC_FFT_PRC);
		
		for(i=0; i < VAD_FRAME_LEN; i++)
        {
    	    complxFftArr[i].real = (float)i16RecFrmData[i];
    	    complxFftArr[i].img  = 0;
        }
		
		calc_fft(complxFftArr, MFC_FFT_PRC);

		u32FrqArr = (uint32_t*)FRQ_MEM_BEG;
		memset((void*)u32FrqArr, 0x0, sizeof(uint32_t) * MFC_FRQ_MAX);
		
		for(i=0; i < MFC_FRQ_MAX; i++)
        {
			u32FrqData  = pow(complxFftArr[i].real, 2) + pow(complxFftArr[i].img, 2);
    	    //optimize[u32FrqArr] u32FrqArr[i] = (uint32_t)sqrt(u32FrqData);
			//optimize[u32FrqArr]
			u32FrqArr[i] = (uint32_t)u32FrqData; 
        }
		
		//optimize[memset complxFftArr 0x0]
	    //memset((void*)complxFftArr, 0x0, sizeof(complex) * MFC_FFT_PRC);
		complxFftArr = 0;
		
		/* optimize[u32FrqArr]
		for(i=0; i < MFC_FRQ_MAX; i++)
        {
    	    u32FrqArr[i] *= u32FrqArr[i];
        }
		*/
		
		memset((void*)u32TRGCenArr, 0x0, sizeof(u32TRGCenArr));
		add_trig(u32FrqArr, MFC_TRG_CENTER, u32TRGCenArr);
		
		//optimize[memset u32FrqArr 0x0]
		//memset((void*)u32FrqArr, 0x0, sizeof(uint32_t) * MFC_FRQ_MAX);
		u32FrqArr = 0;
		
		get_loge(u32TRGCenArr);
		
		memset((void*)u32MFCCFgArr, 0x0, sizeof(uint32_t) * MFC_ORD_NUM);
		get_coef(u32TRGCenArr, u32MFCCFgArr);
		
		/*check Mfcc
		printf("MFCC Check Frame[%d]\n", s_sBlockaddrCtrlHandle.frames);
		for(i = 0; i < MFC_ORD_NUM; i++)
		    printf("%d ", u32MFCCFgArr[i]);
		printf("\n");
		*/

		u32Addr += 4 * VAD_FRAME_MOV;
		
		//Flash 
		s_sBlockaddrCtrlHandle.frames++;
		for(i = 0; i < MFC_ORD_NUM; i++)
		{
			u32MfcAddr += i * 4;
			if(u32MfcAddr < s_sBlockaddrCtrlHandle.endaddr)
			{
				Flash_Write4B(u32MfcAddr, u32MFCCFgArr[i]);
			}
			else
			{
				i16Ret = -1;
				break;
			}
		}
	}	
	
	if(i16Ret < 0)
	{
		Flash_Erase(s_sBlockaddrCtrlHandle.begaddr, s_sBlockaddrCtrlHandle.endaddr);
		printf("Cmd[%d] mfcc storage overflow\n", s_sBlockaddrCtrlHandle.cmd);
		printf("Cmd[%d] mfcc storage frames %d\n", s_sBlockaddrCtrlHandle.frames);
		printf("Cmd[%d] mfcc storage need %d B\n", s_sBlockaddrCtrlHandle.cmd, (s_sBlockaddrCtrlHandle.frames * MFC_ORD_NUM * 4));
	}
	else
	{
	    Flash_Write4B(s_sBlockaddrCtrlHandle.begaddr, s_sBlockaddrCtrlHandle.cmd);
	    Flash_Write4B((s_sBlockaddrCtrlHandle.begaddr + 4), s_sBlockaddrCtrlHandle.frames);
		printf("Cmd[%d] mfcc process SUCCESS\n", s_sBlockaddrCtrlHandle.cmd);
	}
	
	return i16Ret;
}

int16_t Record_Process(uint32_t u32RecAddr, uint16_t u16RecCnt, uint16_t u16Cmd)
{
	int16_t  i16Ret = 0;
	
    // Noise Process
	procNoise(u32RecAddr, u16RecCnt);
				
	// Vad Process
	i16Ret = vad();
	if(i16Ret < 0)
	{
		printf("Cmd[%d] Record Process vad fail\n", u16Cmd);
		return -1;
	}
	
	// Mfcc Process
	i16Ret = mfcc(u16Cmd);
	if(i16Ret < 0)
	{
		printf("Cmd[%d] Record Process mfcc fail", u16Cmd);
		return -1;
	}
	
	return i16Ret;
}

int32_t get_frmdis(uint32_t v_frm1, uint32_t v_frm2)
{
	int32_t dis = 0;
	int32_t dif = 0;
	int32_t i;
	
	uint32_t u32FrmAddr1 = v_frm1;
	uint32_t u32FrmAddr2 = v_frm2;
	uint32_t u32FrmData1;
	uint32_t u32FrmData2;

	for (i = 0; i < MFC_ORD_NUM; i++)
	{
		u32FrmAddr1 += 4 * i;
		u32FrmAddr2 += 4 * i;
		
		u32FrmData1 = Flash_Read(u32FrmAddr1);
		u32FrmData2 = Flash_Read(u32FrmAddr2);
		
		dif  = u32FrmData1 - u32FrmData2;
		dis += pow(dif, 2);
	}

	dis = sqrt(dis);
	return dis;
}

int32_t get_dtwdis(uint32_t u32SmpBlkAddr, uint32_t u32TemBlkAddr)
{
	int32_t dtwdis = 0;
	int16_t i      = 1;
	int16_t j      = 1;
	int16_t k;

	DIRECTION dir;
	int32_t   dis[3];
	int32_t   min;
	
	uint32_t  u32SmpAddr = u32SmpBlkAddr;
	uint32_t  u32TemAddr = u32TemBlkAddr;
	uint32_t  u32SmpFrms = Flash_Read((u32SmpAddr + 4));
	uint32_t  u32TemFrms = Flash_Read((u32TemAddr + 4));
	
	uint32_t  u32SmpFArr0 = u32SmpAddr + 8;
	uint32_t  u32TemFArr0 = u32TemAddr + 8;
	uint32_t  u32SmpFArr1 = u32SmpAddr + 8;
	uint32_t  u32TemFArr1 = u32TemAddr + 8;

	dtwdis = get_frmdis(u32SmpFArr0, u32TemFArr0);
	while(i < u32SmpFrms && j < u32TemFrms)
	{
		u32SmpFArr0 += (i - 1) * MFC_ORD_NUM * 4;
		u32TemFArr0 += (j - 1) * MFC_ORD_NUM * 4;
		u32SmpFArr1 += i * MFC_ORD_NUM * 4;
		u32TemFArr1 += j * MFC_ORD_NUM * 4;
		
		dis[E_DIRCT_RIGHT]   = get_frmdis(u32SmpFArr1, u32TemFArr0);
		dis[E_DIRCT_UP]      = get_frmdis(u32SmpFArr0, u32TemFArr1);
		dis[E_DIRCT_INCLINE] = get_frmdis(u32SmpFArr1, u32TemFArr1);

		min = dis[E_DIRCT_RIGHT];
		dir = E_DIRCT_RIGHT;

		for(k = 1; k < 3; k++)
		{
			if(min > dis[k])
			{
				min = dis[k];
				if(k == 1)
				    dir = E_DIRCT_UP;
				else
					dir = E_DIRCT_INCLINE;
			}
		}

		switch(dir)
		{
		    case E_DIRCT_RIGHT:
			    i++;
			    break;
		    case E_DIRCT_UP:
			    j++;
			    break;
		    case E_DIRCT_INCLINE:
			    i++;
			    j++;
			    break;
		    default:
			    break;
		}

		dtwdis += min;
	}

	while(i < u32SmpFrms)
	{
		u32SmpFArr0 += (i - 1) * MFC_ORD_NUM * 4;
		u32TemFArr0 += (j - 1) * MFC_ORD_NUM * 4;
		u32SmpFArr1 += i * MFC_ORD_NUM * 4;
		u32TemFArr1 += j * MFC_ORD_NUM * 4;
		
		dtwdis += get_frmdis(u32SmpFArr1, u32TemFArr0);
		i++;
	}

	while(j < u32TemFrms)
	{
		u32SmpFArr0 += (i - 1) * MFC_ORD_NUM * 4;
		u32TemFArr0 += (j - 1) * MFC_ORD_NUM * 4;
		u32SmpFArr1 += i * MFC_ORD_NUM * 4;
		u32TemFArr1 += j * MFC_ORD_NUM * 4;
		
		dtwdis += get_frmdis(u32SmpFArr0, u32TemFArr1);
		j++;
	}

	return dtwdis;
}

int16_t dtw(float* mindtw)
{
	uint32_t u32SmpAddr;
	uint32_t u32TemAddr;
	int32_t  i32TemComd;
	uint32_t u32TemFrms;
	
	uint16_t i;
	int16_t  i16RecCmd;
	int16_t  i16RetCmd;
	
    float    u32DtwArr[DATA_FLASH_MFCCTG_MAX];
	float    u32DtwMin;
	
	memset((void*)u32DtwArr, 0x0, sizeof(u32DtwArr));
	u32SmpAddr = DATA_FLASH_MFCCRG_BEG;
	
	for(i = 0; i < DATA_FLASH_MFCCTG_MAX; i++)
	{
		u32TemAddr = DATA_FLASH_MFCCTG_BEG + i * DATA_FLASH_VALBUF_SIZ;
		i32TemComd = Flash_Read(u32TemAddr);
		u32TemFrms = Flash_Read((u32TemAddr + 4));
		
		if(i32TemComd < 0)
		{
			i16RecCmd = i;
			break;
		}
		
		u32DtwArr[i] = get_dtwdis(u32SmpAddr, u32TemAddr) / u32TemFrms;
	}
	
	u32DtwMin = MFC_MAX_DIS;
	i16RetCmd = MFC_INV_CMD;
	for(i = 0; i < i16RecCmd; i++)
	{
		if(u32DtwArr[i] == MFC_MAX_DIS)
		{
			continue;
		}

		if(u32DtwMin == MFC_MAX_DIS)
		{
			u32DtwMin = u32DtwArr[i];
			i16RetCmd = i;
		}

		if(u32DtwMin > u32DtwArr[i])
		{
			u32DtwMin = u32DtwArr[i];
			i16RetCmd = i;
		}

		//printf("%f \n", u32DtwArr[i]);
	}
	*mindtw = u32DtwMin;

	//printf("dtwmin = %d\n", dtwmin);
	return i16RetCmd;
}
	
int16_t Recognize_Process(uint32_t u32RecAddr, uint16_t u16RecCnt, uint16_t u16Cmd)
{
	int16_t  i16Ret;
	float    dtwLen;
	
    // Noise Process
	procNoise(u32RecAddr, u16RecCnt);
				
	// Vad Process
	i16Ret = vad();
	if(i16Ret < 0)
	{
		printf("Cmd[u16Cmd] Record Process vad fail\n");
		return -1;
	}
	
	// Mfcc Process
	i16Ret = mfcc(u16Cmd);
	if(i16Ret < 0)
	{
		printf("Cmd[u16Cmd] Record Process mfcc fail");
		return -1;
	}
	
	i16Ret = dtw(&dtwLen);
	printf("Cmd[%d]  ", i16Ret);
	printf("Dtw[%f]\n", dtwLen);
	
	return i16Ret;
}























