/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;
volatile uint32_t counter_systick = 0;

#if defined (ENABLE_TICK_EVENT)
typedef void (*sys_pvTimeFunPtr)(void);   /* function pointer */
typedef struct timeEvent_t
{
    uint8_t             active;
    unsigned long       initTick;
    unsigned long       curTick;
    sys_pvTimeFunPtr    funPtr;
} TimeEvent_T;

#define TICKEVENTCOUNT                                 (8)                   
volatile  TimeEvent_T tTimerEvent[TICKEVENTCOUNT];
volatile uint8_t _sys_uTimerEventCount = 0;             /* Speed up interrupt reponse time if no callback function */
#endif


#define QSPI_TARGET_FREQ				(800000ul)
#define QSPI_TRANSMIT_LEN               (30)

#define QSPI_MASTER_TX_DMA_CH 			(14)
#define QSPI_MASTER_RX_DMA_CH 		    (15)
#define QSPI_MASTER_OPENED_CH   		((1 << QSPI_MASTER_TX_DMA_CH) | (1 << QSPI_MASTER_RX_DMA_CH))

#define QSPI_4_WIRE_READ_RX_CMD         (0x5A)
#define QSPI_4_WIRE_WRITE_RX_CMD        (0xA5)

#define QSPI_SINGLE_MODE_ENABLE(qspi)    {QSPI_DISABLE_DUAL_MODE(qspi);\
                                                QSPI_DISABLE_QUAD_MODE(qspi);}

#define QSPI_SINGLE_OUTPUT_MODE_ENABLE(qspi)    {QSPI_SINGLE_MODE_ENABLE(qspi);\
                                                    (qspi)->CTL |= QSPI_CTL_DATDIR_Msk;}

// #define QSPI_DMA_Complete_IRQHandler    (PDMA_IRQHandler)

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

uint32_t get_systick(void)
{
	return (counter_systick);
}

void set_systick(uint32_t t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    #if 1
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}
    #else
    if (memcmp(src, des, nBytes))
    {
        printf("\nMismatch!! - %d\n", nBytes);
        for (i = 0; i < nBytes; i++)
            printf("0x%02x    0x%02x\n", src[i], des[i]);
        return -1;
    }
    #endif

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}



#if defined (ENABLE_TICK_EVENT)
void TickCallback_processB(void)
{
    printf("%s test \r\n" , __FUNCTION__);
}

void TickCallback_processA(void)
{
    printf("%s test \r\n" , __FUNCTION__);
}

void TickClearTickEvent(uint8_t u8TimeEventID)
{
    if (u8TimeEventID > TICKEVENTCOUNT)
        return;

    if (tTimerEvent[u8TimeEventID].active == TRUE)
    {
        tTimerEvent[u8TimeEventID].active = FALSE;
        _sys_uTimerEventCount--;
    }
}

signed char TickSetTickEvent(unsigned long uTimeTick, void *pvFun)
{
    int  i;
    int u8TimeEventID = 0;

    for (i = 0; i < TICKEVENTCOUNT; i++)
    {
        if (tTimerEvent[i].active == FALSE)
        {
            tTimerEvent[i].active = TRUE;
            tTimerEvent[i].initTick = uTimeTick;
            tTimerEvent[i].curTick = uTimeTick;
            tTimerEvent[i].funPtr = (sys_pvTimeFunPtr)pvFun;
            u8TimeEventID = i;
            _sys_uTimerEventCount += 1;
            break;
        }
    }

    if (i == TICKEVENTCOUNT)
    {
        return -1;    /* -1 means invalid channel */
    }
    else
    {
        return u8TimeEventID;    /* Event ID start from 0*/
    }
}

void TickCheckTickEvent(void)
{
    uint8_t i = 0;

    if (_sys_uTimerEventCount)
    {
        for (i = 0; i < TICKEVENTCOUNT; i++)
        {
            if (tTimerEvent[i].active)
            {
                tTimerEvent[i].curTick--;

                if (tTimerEvent[i].curTick == 0)
                {
                    (*tTimerEvent[i].funPtr)();
                    tTimerEvent[i].curTick = tTimerEvent[i].initTick;
                }
            }
        }
    }
}

void TickInitTickEvent(void)
{
    uint8_t i = 0;

    _sys_uTimerEventCount = 0;

    /* Remove all callback function */
    for (i = 0; i < TICKEVENTCOUNT; i++)
        TickClearTickEvent(i);

    _sys_uTimerEventCount = 0;
}
#endif 

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned long delay)
{  
    
    uint32_t tickstart = get_systick(); 
    uint32_t wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

void QSPI_SS_SET_LOW(void)
{
    #if defined (ENABLE_QSPI_MANUAL_SS)
    QSPI_SET_SS_LOW(QSPI0); 
    #endif
}

void QSPI_SS_SET_HIGH(void)
{
    #if defined (ENABLE_QSPI_MANUAL_SS)
    QSPI_SET_SS_HIGH(QSPI0); 
    #endif
}

// void QSPI_DMA_Complete_IRQHandler(void)
void PDMA_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS(PDMA);
	
    if (status & PDMA_INTSTS_ABTIF_Msk)   /* abort */
    {
		#if 1
        PDMA_CLR_ABORT_FLAG(PDMA, PDMA_GET_ABORT_STS(PDMA));
		#else
        if (PDMA_GET_ABORT_STS(PDMA) & (1 << QSPI_MASTER_TX_DMA_CH))
        {

        }
        PDMA_CLR_ABORT_FLAG(PDMA, (1 << QSPI_MASTER_TX_DMA_CH));

        if (PDMA_GET_ABORT_STS(PDMA) & (1 << QSPI_MASTER_RX_DMA_CH))
        {

        }
        PDMA_CLR_ABORT_FLAG(PDMA, (1 << QSPI_MASTER_RX_DMA_CH));
		#endif
    }
    else if (status & PDMA_INTSTS_TDIF_Msk)     /* done */
    {
        if((PDMA_GET_TD_STS(PDMA) & (1 << QSPI_MASTER_TX_DMA_CH) ) == (1 << QSPI_MASTER_TX_DMA_CH)  )
        {
            /* Clear PDMA transfer done interrupt flag */
            PDMA_CLR_TD_FLAG(PDMA, (1 << QSPI_MASTER_TX_DMA_CH) );

			//insert process
            QSPI_DISABLE_TX_PDMA(QSPI0);
            set_flag(flag_qspi_tx_finish , ENABLE);
        }   

        if((PDMA_GET_TD_STS(PDMA) & (1 << QSPI_MASTER_RX_DMA_CH) ) == (1 << QSPI_MASTER_RX_DMA_CH)  )
        {
            /* Clear PDMA transfer done interrupt flag */
            PDMA_CLR_TD_FLAG(PDMA, (1 << QSPI_MASTER_RX_DMA_CH) );

			//insert process
            QSPI_DISABLE_RX_PDMA(QSPI0);
            set_flag(flag_qspi_rx_finish,ENABLE);
        }  

    }
    else if (status & (PDMA_INTSTS_REQTOF0_Msk | PDMA_INTSTS_REQTOF1_Msk))     /* Check the DMA time-out interrupt flag */
    {
        PDMA_CLR_TMOUT_FLAG(PDMA,QSPI_MASTER_TX_DMA_CH);
        PDMA_CLR_TMOUT_FLAG(PDMA,QSPI_MASTER_RX_DMA_CH);
    }
    else
    {

    }

}

void QSPIReadDataWithDMA(unsigned char *pBuffer , unsigned short size)
{
    #if defined (ENALBE_PDMA_POLLING)    
	uint32_t u32RegValue = 0;
	uint32_t u32Abort = 0;	
    #elif defined (ENALBE_QSPI_REGULAR_TRX)
    uint8_t i = 0;
    #endif

    uint8_t tBuffer[QSPI_TRANSMIT_LEN] = {0x00};

    #if defined (ENALBE_PDMA_IRQ) || defined (ENALBE_PDMA_POLLING)    
    set_flag(flag_qspi_tx_finish , DISABLE);
    set_flag(flag_qspi_rx_finish , DISABLE);    
    PDMA_SetTransferCnt(PDMA,QSPI_MASTER_TX_DMA_CH, PDMA_WIDTH_8, QSPI_TRANSMIT_LEN);
    PDMA_SetTransferAddr(PDMA,QSPI_MASTER_TX_DMA_CH, (uint32_t)tBuffer, PDMA_SAR_INC, (uint32_t)&QSPI0->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA,QSPI_MASTER_TX_DMA_CH, PDMA_QSPI0_TX, FALSE, 0);    

    PDMA_SetTransferCnt(PDMA,QSPI_MASTER_RX_DMA_CH, PDMA_WIDTH_8, size);
    PDMA_SetTransferAddr(PDMA,QSPI_MASTER_RX_DMA_CH, (uint32_t)&QSPI0->RX, PDMA_SAR_FIX, (uint32_t)pBuffer, PDMA_DAR_INC);
    PDMA_SetTransferMode(PDMA,QSPI_MASTER_RX_DMA_CH, PDMA_QSPI0_RX, FALSE, 0);   

    QSPI_TRIGGER_TX_PDMA(QSPI0);
    QSPI_TRIGGER_RX_PDMA(QSPI0);    
    #endif

    #if defined (ENALBE_PDMA_IRQ)
    while(!is_flag_set(flag_qspi_tx_finish));
    while(!is_flag_set(flag_qspi_rx_finish));    

    #elif defined (ENALBE_PDMA_POLLING)

    while(1)
    {
        /* Get interrupt status */
        u32RegValue = PDMA_GET_INT_STATUS(PDMA);
        /* Check the DMA transfer done interrupt flag */
        if(u32RegValue & PDMA_INTSTS_TDIF_Msk)
        {
            /* Check the PDMA transfer done interrupt flags */
            if((PDMA_GET_TD_STS(PDMA) & (1 << QSPI_MASTER_RX_DMA_CH)) == (1 << QSPI_MASTER_RX_DMA_CH))
            {
                /* Clear the DMA transfer done flags */
                PDMA_CLR_TD_FLAG(PDMA,1 << QSPI_MASTER_RX_DMA_CH);
                /* Disable SPI PDMA TX function */
                QSPI_DISABLE_RX_PDMA(QSPI0);
                break;
            }

            /* Check the DMA transfer abort interrupt flag */
            if(u32RegValue & PDMA_INTSTS_ABTIF_Msk)
            {
                /* Get the target abort flag */
                u32Abort = PDMA_GET_ABORT_STS(PDMA);
                /* Clear the target abort flag */
                PDMA_CLR_ABORT_FLAG(PDMA,u32Abort);
                break;
            }
        }
    }
    #elif defined (ENALBE_QSPI_REGULAR_TRX)

    for (i = 0 ; i < QSPI_TRANSMIT_LEN ; i++)
    {
        QSPI_WRITE_TX(QSPI0, 0x00);
        while(QSPI_IS_BUSY(QSPI0));
        pBuffer[i] = QSPI_READ_RX(QSPI0);                      
    }
    #endif

    while(QSPI_IS_BUSY(QSPI0)); 
    QSPI_SS_SET_HIGH();    
}

int ReadSlaveRxRegs(unsigned char ChipNo , 
                    unsigned char Bank , 
                    unsigned int StartAddr , 
                    unsigned char *pBuffer , 
                    unsigned short size , 
                    int bDMAMode)
{
    QSPI_SS_SET_LOW();
    QSPI_SINGLE_OUTPUT_MODE_ENABLE(QSPI0);
    QSPI_WRITE_TX(QSPI0,QSPI_4_WIRE_READ_RX_CMD);
    while(QSPI_IS_BUSY(QSPI0));

    QSPI_ENABLE_QUAD_OUTPUT_MODE(QSPI0);
    QSPI_WRITE_TX(QSPI0, Bank);
    QSPI_WRITE_TX(QSPI0, (StartAddr>>8)&0xFF);
    QSPI_WRITE_TX(QSPI0, StartAddr&0xFF);

    QSPI_WRITE_TX(QSPI0,0xFF);
    QSPI_WRITE_TX(QSPI0,0xFF);
    QSPI_WRITE_TX(QSPI0,0xFF);
    QSPI_WRITE_TX(QSPI0,0xFF);
    while(QSPI_IS_BUSY(QSPI0) || !QSPI_GET_TX_FIFO_EMPTY_FLAG(QSPI0));

    QSPI_ENABLE_QUAD_INPUT_MODE(QSPI0);
    QSPI_ClearRxFIFO(QSPI0);
    QSPIReadDataWithDMA(pBuffer,size);

    return 1;
}

void QSPIWriteDataWithDMA(unsigned char *pBuffer , unsigned short size)
{
    #if defined (ENALBE_PDMA_POLLING)    
	uint32_t u32RegValue = 0;
	uint32_t u32Abort = 0;
    #elif defined (ENALBE_QSPI_REGULAR_TRX)
    uint8_t i = 0;
    #endif

    #if defined (ENALBE_PDMA_IRQ) || defined (ENALBE_PDMA_POLLING)
    set_flag(flag_qspi_tx_finish , DISABLE);
    PDMA_SetTransferCnt(PDMA,QSPI_MASTER_TX_DMA_CH, PDMA_WIDTH_8, size);
    PDMA_SetTransferAddr(PDMA,QSPI_MASTER_TX_DMA_CH, (uint32_t)pBuffer, PDMA_SAR_INC, (uint32_t)&QSPI0->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA,QSPI_MASTER_TX_DMA_CH, PDMA_QSPI0_TX, FALSE, 0);    
    QSPI_TRIGGER_TX_PDMA(QSPI0);
    #endif

    #if defined (ENALBE_PDMA_IRQ)
    while(!is_flag_set(flag_qspi_tx_finish));
    #elif defined (ENALBE_PDMA_POLLING)   
    while(1)
    {
        /* Get interrupt status */
        u32RegValue = PDMA_GET_INT_STATUS(PDMA);
        /* Check the DMA transfer done interrupt flag */
        if(u32RegValue & PDMA_INTSTS_TDIF_Msk)
        {
            /* Check the PDMA transfer done interrupt flags */
            if((PDMA_GET_TD_STS(PDMA) & (1 << QSPI_MASTER_TX_DMA_CH)) == (1 << QSPI_MASTER_TX_DMA_CH))
            {
                /* Clear the DMA transfer done flags */
                PDMA_CLR_TD_FLAG(PDMA,1 << QSPI_MASTER_TX_DMA_CH);
                /* Disable SPI PDMA TX function */
                QSPI_DISABLE_TX_PDMA(QSPI0);
                break;
            }

            /* Check the DMA transfer abort interrupt flag */
            if(u32RegValue & PDMA_INTSTS_ABTIF_Msk)
            {
                /* Get the target abort flag */
                u32Abort = PDMA_GET_ABORT_STS(PDMA);
                /* Clear the target abort flag */
                PDMA_CLR_ABORT_FLAG(PDMA,u32Abort);
                break;
            }
        }
    }
    #elif defined (ENALBE_QSPI_REGULAR_TRX)

    for (i = 0 ; i < QSPI_TRANSMIT_LEN ; i++)
    {
        QSPI_WRITE_TX(QSPI0,pBuffer[i]);
        while(QSPI_IS_BUSY(QSPI0));      
    }
    #endif

    while(QSPI_IS_BUSY(QSPI0));
    QSPI_SS_SET_HIGH();   

}

int WriteSlaveRxRegs(unsigned char ChipNo , 
                    unsigned char Bank , 
                    unsigned int StartAddr , 
                    unsigned char *pBuffer , 
                    unsigned short size , 
                    int bDMAMode)
{
    QSPI_SS_SET_LOW();
    QSPI_SINGLE_OUTPUT_MODE_ENABLE(QSPI0);
    QSPI_WRITE_TX(QSPI0,QSPI_4_WIRE_WRITE_RX_CMD);
    while(QSPI_IS_BUSY(QSPI0));

    QSPI_ENABLE_QUAD_OUTPUT_MODE(QSPI0);
    QSPI_WRITE_TX(QSPI0, Bank);
    QSPI_WRITE_TX(QSPI0, (StartAddr>>8)&0xFF);
    QSPI_WRITE_TX(QSPI0, StartAddr&0xFF);
    while(QSPI_IS_BUSY(QSPI0) || !QSPI_GET_TX_FIFO_EMPTY_FLAG(QSPI0));

    QSPIWriteDataWithDMA(pBuffer,size);

    return 1;
}

void InitQSPIDMA(void)
{
    PDMA_Open(PDMA, QSPI_MASTER_OPENED_CH);

    PDMA_SetBurstType(PDMA, QSPI_MASTER_TX_DMA_CH , PDMA_REQ_SINGLE, 0);   
    PDMA_SetBurstType(PDMA, QSPI_MASTER_RX_DMA_CH , PDMA_REQ_SINGLE, 0);  

    PDMA->DSCT[QSPI_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
    PDMA->DSCT[QSPI_MASTER_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    QSPI_DISABLE_TX_RX_PDMA(QSPI0);

    #if defined (ENALBE_PDMA_IRQ)
    PDMA_EnableInt(PDMA, QSPI_MASTER_TX_DMA_CH, PDMA_INT_TRANS_DONE);
    PDMA_EnableInt(PDMA, QSPI_MASTER_RX_DMA_CH, PDMA_INT_TRANS_DONE);

    NVIC_SetPriority(PDMA_IRQn,0);
    NVIC_ClearPendingIRQ(PDMA_IRQn);
    NVIC_EnableIRQ(PDMA_IRQn);
    #endif

}

void InitQSPI(void)
{
    uint32_t clk = 0;
    
    SYS_UnlockReg();
    CLK_EnableModuleClock(QSPI0_MODULE);
    CLK_EnableModuleClock(PDMA_MODULE);    
    CLK_SetModuleClock(QSPI0_MODULE, CLK_CLKSEL2_QSPI0SEL_PCLK0, MODULE_NoMsk);
    SYS_LockReg();

    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA2MFP_QSPI0_CLK | SYS_GPA_MFPL_PA5MFP_QSPI0_MISO1);
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH9MFP_Msk | SYS_GPH_MFPH_PH11MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH9MFP_QSPI0_SS | SYS_GPH_MFPH_PH11MFP_QSPI0_MOSI1);
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC0MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC0MFP_QSPI0_MOSI0);
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE1MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE1MFP_QSPI0_MISO0);

    /* Enable QSPI0 clock pin (PA2) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
    PA->SMTEN |= GPIO_SMTEN_SMTEN5_Msk;  
    PH->SMTEN |= GPIO_SMTEN_SMTEN9_Msk;  
    PH->SMTEN |= GPIO_SMTEN_SMTEN11_Msk;  
    PC->SMTEN |= GPIO_SMTEN_SMTEN0_Msk;   
    PE->SMTEN |= GPIO_SMTEN_SMTEN1_Msk;           

    GPIO_SetSlewCtl(PA, BIT2 | BIT5 , GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PH, BIT9 | BIT11 , GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PC, BIT0 , GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PE, BIT1 , GPIO_SLEWCTL_FAST);

    clk = QSPI_Open(QSPI0, QSPI_MASTER, QSPI_MODE_0, 8, QSPI_TARGET_FREQ);
    // printf("clk = %8d\r\n", clk);

    #if defined (ENABLE_QSPI_MANUAL_SS)
    QSPI_DisableAutoSS(QSPI0);
    QSPI_SS_SET_HIGH();
    #else
    QSPI_EnableAutoSS(QSPI0, SPI_SS, SPI_SS_ACTIVE_LOW);
    #endif

    QSPI_SET_SUSPEND_CYCLE(QSPI0,0);
    // QSPI_SetFIFO(QSPI0, 4, 4);

    InitQSPIDMA();
}

void TestQSPIFlow(void)
{
    unsigned char WriteBuf[QSPI_TRANSMIT_LEN] = {0};
    unsigned char ReadBuf[QSPI_TRANSMIT_LEN] = {0};   
    unsigned char i = 0;

     for( i = 0 ; i < QSPI_TRANSMIT_LEN ; i++)
     {
        WriteBuf[i] = i;
     }

     WriteSlaveRxRegs(0 , 0 , 0 , &WriteBuf[0] , QSPI_TRANSMIT_LEN , 1 );
     ReadSlaveRxRegs(0 , 0 , 0 , &ReadBuf[0] , QSPI_TRANSMIT_LEN , 1 );
}


void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            set_flag(flag_timer_period_1000ms ,ENABLE);
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void loop(void)
{
	static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (is_flag_set(flag_timer_period_1000ms))
    {
        set_flag(flag_timer_period_1000ms ,DISABLE);

        printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        PH0 ^= 1;             
    }
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}

void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

    #if defined (ENALBE_UART_IRQ)
	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);
    #endif

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif
}

void Custom_Init(void)
{
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);

	//EVM LED
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    
    // CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    // CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    // CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
    // CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

    // CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
    // CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV1 | CLK_PCLKDIV_APB1DIV_DIV1);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	Custom_Init();
	// UART0_Init();
	// TIMER1_Init();

    // SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    InitQSPI();
    TestQSPIFlow();

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
