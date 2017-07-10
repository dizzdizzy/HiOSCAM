/******************************************************************************

Copyright (C), 2009-2014, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_drv_ai.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2012/09/22
Last Modified :
Description   : ai
Function List :
History       :
* main\1    2012-09-22   z40717     init.
******************************************************************************/
#ifndef __HI_DRV_AI_H__
 #define __HI_DRV_AI_H__

#ifdef __cplusplus
#if __cplusplus
 extern "C"{
#endif
#endif /* __cplusplus */

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_unf_ai.h"

#define  AI_MAX_TOTAL_NUM          (4)
#define  AI_MAX_HANDLE_ID          ((HI_ID_AI << 16) | AI_MAX_TOTAL_NUM)
#define  AI_MIN_HANDLE_ID          (HI_ID_AI << 16)
#define  AI_CHNID_MASK             0xffff


/*Define Debug Level For HI_ID_AI                     */

 #define HI_FATAL_AI(fmt...) \
    HI_FATAL_PRINT(HI_ID_AI, fmt)

 #define HI_ERR_AI(fmt...) \
    HI_ERR_PRINT(HI_ID_AI, fmt)

 #define HI_WARN_AI(fmt...) \
    HI_WARN_PRINT(HI_ID_AI, fmt)

 #define HI_INFO_AI(fmt...) \
    HI_INFO_PRINT(HI_ID_AI, fmt)

#define CHECK_AI_NULL_PTR(p)                                \
    do {                                                    \
            if(HI_NULL == p)                                \
            {                                               \
                HI_ERR_AI("NULL pointer \n");               \
                return HI_ERR_AI_NULL_PTR;                   \
            }                                               \
         } while(0)
         
#define CHECK_AI_CREATE(state)                              \
    do                                                      \
    {                                                       \
        if (0 > state)                                      \
        {                                                   \
            HI_ERR_AI("AI  device not open!\n");           \
            return HI_ERR_AI_NOT_INIT;                \
        }                                                   \
    } while (0)

#if  1
#define CHECK_AI_PORT(port)                                  \
    do                                                          \
    {                                                           \
        if (HI_UNF_AI_BUTT <= port)                            \
        {                                                       \
            HI_WARN_AI(" Invalid Ai id %d\n", port);           \
            return HI_ERR_AI_INVALID_ID;                       \
        }                                                       \
    } while (0)
#endif

#define CHECK_AI_ID(AiHandle)                                  \
    do                                                          \
    {                                                           \
        if ((AI_MAX_HANDLE_ID <= AiHandle) || (AI_MIN_HANDLE_ID > AiHandle)) \
        {                                                       \
            HI_ERR_AI(" Invalid Ai id 0x%x\n", AiHandle);       \
            return HI_ERR_AI_INVALID_ID;                       \
        }                                                       \
    } while (0)

#define CHECK_AI_CHN_OPEN(AiHandle) \
    do                                                         \
    {                                                          \
        AiHandle &= AI_CHNID_MASK;                            \
        if (NULL == g_pstGlobalAIRS.pstAI_ATTR_S[AiHandle])   \
        {                                                       \
            HI_ERR_AI(" Invalid AI id %d(not open)\n", AiHandle);        \
            return HI_ERR_AI_INVALID_PARA;                       \
        }                                                       \
    } while (0)


#define CHECK_AI_SAMPLERATE(outrate )                   \
    do                                                  \
    {                                                   \
        switch (outrate)                                \
        {                                               \
        case  HI_UNF_SAMPLE_RATE_8K:                    \
        case  HI_UNF_SAMPLE_RATE_11K:                   \
        case  HI_UNF_SAMPLE_RATE_12K:                   \
        case  HI_UNF_SAMPLE_RATE_16K:                   \
        case  HI_UNF_SAMPLE_RATE_22K:                   \
        case  HI_UNF_SAMPLE_RATE_24K:                   \
        case  HI_UNF_SAMPLE_RATE_32K:                   \
        case  HI_UNF_SAMPLE_RATE_44K:                   \
        case  HI_UNF_SAMPLE_RATE_48K:                   \
        case  HI_UNF_SAMPLE_RATE_88K:                   \
        case  HI_UNF_SAMPLE_RATE_96K:                   \
        case  HI_UNF_SAMPLE_RATE_176K:                  \
        case  HI_UNF_SAMPLE_RATE_192K:                  \
            break;                                      \
        default:                                        \
            HI_WARN_AI("invalid sample out rate %d\n", outrate);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   


#define CHECK_AI_BCLKDIV(BclkDiv)                   \
    do                                                  \
    {                                                   \
        switch (BclkDiv)                                \
        {                                               \
        case  HI_UNF_I2S_BCLK_1_DIV:                    \
        case  HI_UNF_I2S_BCLK_2_DIV:                    \
        case  HI_UNF_I2S_BCLK_3_DIV:                    \
        case  HI_UNF_I2S_BCLK_4_DIV:                    \
        case  HI_UNF_I2S_BCLK_6_DIV:                    \
        case  HI_UNF_I2S_BCLK_8_DIV:                    \
        case  HI_UNF_I2S_BCLK_12_DIV:                   \
        case  HI_UNF_I2S_BCLK_24_DIV:                   \
        case  HI_UNF_I2S_BCLK_32_DIV:                   \
        case  HI_UNF_I2S_BCLK_48_DIV:                   \
        case  HI_UNF_I2S_BCLK_64_DIV:                   \
            break;                                      \
        default:                                        \
            HI_WARN_AI("invalid bclkDiv %d\n", BclkDiv);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   

#define CHECK_AI_MCLKDIV(MclkSel)                   \
    do                                                  \
    {                                                   \
        switch (MclkSel)                                \
        {                                               \
        case  HI_UNF_I2S_MCLK_128_FS:                    \
        case  HI_UNF_I2S_MCLK_256_FS:                    \
        case  HI_UNF_I2S_MCLK_384_FS:                    \
        case  HI_UNF_I2S_MCLK_512_FS:                    \
        case  HI_UNF_I2S_MCLK_768_FS:                    \
        case  HI_UNF_I2S_MCLK_1024_FS:                    \
            break;                                      \
        default:                                        \
            HI_WARN_AI("invalid mclk sel %d\n", MclkSel);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   

#define CHECK_AI_CHN(Chn)                   \
    do                                                  \
    {                                                   \
        switch (Chn)                                    \
        {                                               \
        case  HI_UNF_I2S_CHNUM_1:                       \
        case  HI_UNF_I2S_CHNUM_2:                       \
        case  HI_UNF_I2S_CHNUM_8:                       \
            break;                                      \
        default:                                        \
            HI_WARN_AI("invalid chn %d\n", Chn);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   


#define CHECK_AI_BITDEPTH(BitDepth )                   \
    do                                                  \
    {                                                   \
        switch (BitDepth)                               \
        {                                               \
        case  HI_UNF_I2S_BIT_DEPTH_16:                  \
        case  HI_UNF_I2S_BIT_DEPTH_24:                  \
            break;                                      \
        default:                                        \
            HI_WARN_AI("invalid bitDepth %d\n", BitDepth);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   


#define CHECK_AI_PCMDELAY(PcmDelayCycle)                   \
    do                                                  \
    {                                                   \
        switch (PcmDelayCycle)                          \
        {                                               \
        case  HI_UNF_I2S_PCM_0_DELAY:                   \
        case  HI_UNF_I2S_PCM_1_DELAY:                   \
        case  HI_UNF_I2S_PCM_8_DELAY:                   \
        case  HI_UNF_I2S_PCM_16_DELAY:                  \
        case  HI_UNF_I2S_PCM_17_DELAY:                  \
        case  HI_UNF_I2S_PCM_24_DELAY:                  \
        case  HI_UNF_I2S_PCM_32_DELAY:                  \
            break;                                      \
        default:                                        \
            HI_WARN_AI("invalid pcmDelayCycle %d\n", PcmDelayCycle);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   

#define CHECK_AI_HdmiDataFormat(HdmiDataFormat)         \
    do                                                  \
    {                                                   \
        switch (HdmiDataFormat)                               \
        {                                               \
        case  HI_UNF_AI_HDMI_FORMAT_LPCM:               \
        case  HI_UNF_AI_HDMI_FORMAT_LBR:                  \
        case  HI_UNF_AI_HDMI_FORMAT_HBR:                  \
            break;                                      \
        default:                                        \
            HI_ERR_AI("invalid Hdmi DataFormat %d\n", HdmiDataFormat);    \
            return HI_ERR_AI_INVALID_PARA;                        \
        }                                                       \
    } while (0)   

 #define HI_AI_LOCK(mutex) (void)pthread_mutex_lock(mutex);
 #define HI_AI_UNLOCK(mutex) (void)pthread_mutex_unlock(mutex);

 typedef struct hiAI_BUF_ATTR_S
 {
     HI_PHYS_ADDR_T tPhyBaseAddr;
     HI_U32      u32Read;
     HI_U32      u32Write;
     HI_U32      u32Size;
     /* user space virtual address */
     HI_VIRT_ADDR_T tUserVirBaseAddr;   
     /* kernel space virtual address */
     HI_VIRT_ADDR_T tKernelVirBaseAddr;
     //TO DO
     //MMZ Handle
     
 } AI_BUF_ATTR_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

 #endif
