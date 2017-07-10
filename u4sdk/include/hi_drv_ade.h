/******************************************************************************

Copyright (C), 2009-2014, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_drv_ade.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2013-01-22
Last Modified :
Description   : audio dsp decoder
Function List :
History       :
* main\1    2013-01-22   zgjie     init.
******************************************************************************/
#ifndef __HI_DRV_ADE_H__
#define __HI_DRV_ADE_H__

#include "hi_type.h"
#include "hi_module.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

/*Define Debug Level For HI_ID_ADE                     */
#define HI_FATAL_ADE(fmt...) \
    HI_FATAL_PRINT(HI_ID_ADE, fmt)

#define HI_ERR_ADE(fmt...) \
    HI_ERR_PRINT(HI_ID_ADE, fmt)

#define HI_WARN_ADE(fmt...) \
    HI_WARN_PRINT(HI_ID_ADE, fmt)

#define HI_INFO_ADE(fmt...) \
    HI_INFO_PRINT(HI_ID_ADE, fmt)

#define CHECK_ADE_OPEN(chan) \
    do                                                         \
    {                                                          \
        if (0 == atomic_read(&s_stAdeDrv.astChanEntity[chan].atmUseCnt))   \
        {                                                       \
            HI_WARN_ADE(" Invalid ade(%d)\n", chan);        \
            return HI_FAILURE;                       \
        }                                                       \
    } while (0)

typedef enum
{
    ADE_DEC_MODE_RAWPCM = 0, /**<PCM decoding mode*/ /**<CNcomment:PCM 解码模式 */
    ADE_DEC_MODE_THRU, /**<SPIDF61937 passthrough decoding mode only, such as AC3/DTS */ /**<CNcomment:透传解码模式 */
    ADE_DEC_MODE_SIMUL, /**<PCM and passthrough decoding mode*/ /**<CNcomment:PCM + 透传解码模式 */
    ADE_DEC_MODE_BUTT = 0x7FFFFFFF
} ADE_DEC_MODE_E;

typedef struct
{
    ADE_DEC_MODE_E   enDecMode;
    HI_VOID *        pCodecPrivateData;
    HI_U32           u32CodecPrivateParamsSize;
} ADE_OPEN_PARAM_S;


typedef struct
{
    HI_U32            u32CodecID;
    HI_U32            u32MsgPoolSize;
    ADE_OPEN_PARAM_S  stOpenParams;
} ADE_OPEN_DECODER_S;

typedef struct
{
    HI_U32   u32Cmd;
    HI_VOID* pPrivateSetConfigData;
    HI_U32   u32PrivateSetConfigDataSize;
} ADE_IOCTRL_PARAM_S;


typedef struct
{
    HI_U32    u32BufDataPhyAddr;          /*buffer physical addr*/
    HI_U32    u32BufDataVirAddr;          /*buffer virtual addr*/
    HI_U32    u32BufSize;                 /*buffer length*/
    HI_U32    u32Boundary;                /*pts read pointers wrap point */
} ADE_INBUF_ATTR_S;


typedef struct
{
    HI_U32 u32BufDataPhyAddr;
    HI_U32 u32BufDataVirAddr;          /*buffer virtual addr*/
    HI_U32 u32MaxFramePrivateInfoSize;
    HI_U32 u32MaxFrameBufSize;
    HI_U32 u32MaxFrameNumber;            /* 2/4/8 */
} ADE_OUTBUF_ATTR_S;

typedef struct
{
    HI_U32 u32PtsWritePos;
    HI_U32 u32WritePos;
    HI_U32 u32ReadPos;
} ADE_INBUF_SYNC_S;

typedef struct
{
    HI_U32 u32PtsWritePos;
    HI_U32 u32WritePos;
} ADE_INBUF_SYNC_WPOS_S;

typedef struct
{
    HI_U32 u32ReadPos;
} ADE_INBUF_SYNC_RPOS_S;

typedef struct
{
    /* input */
    HI_VOID *pOutbuf;  /* pcm -> lbr -> hbr  */
    HI_U32  u32OutbufSize;

    /* output */
    HI_U32 u32StreamSampleRate;
    HI_U32 u32StreamChannels;
    HI_U32 u32StreamBitRate;

    HI_U32 u32PcmSampleRate;
    HI_U32 u32LbrSampleRate;
    HI_U32 u32HbrSampleRate;

    HI_U32 u32PcmChannels;
    HI_U32 u32LbrChannels;/* 2 */
    HI_U32 u32HbrChannels;/* 2(DDP) or 8 */

    HI_U32 u32PcmBitDepth;/* 16 or 24 */
    HI_U32 u32LbrBitDepth;/* 16 */
    HI_U32 u32HbrBitDepth;/* 16 */

    HI_U32 u32PcmBytesPerFrame;
    HI_U32 u32LbrBytesPerFrame;
    HI_U32 u32HbrBytesPerFrame;

    HI_BOOL bAckEosFlag;

    HI_U32 u32CurnPtsReadPos; /* for PTS */
    HI_U32 u32FrameIndex;
} ADE_FRMAE_BUF_S;

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_DRV_ADE_H__ */
