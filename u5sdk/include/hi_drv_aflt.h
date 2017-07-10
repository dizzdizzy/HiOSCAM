/******************************************************************************

Copyright (C), 2009-2014, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_drv_aflt.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2013-01-22
Last Modified :
Description   : audio dsp decoder
Function List :
History       :
* main\1    2013-01-22   zgjie     init.
******************************************************************************/
#ifndef __HI_DRV_AFLT_H__
#define __HI_DRV_AFLT_H__

#include "hi_type.h"
#include "hi_module.h"

#include "hi_audsp_aflt.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

/******************************* macro definition *****************************/
#define AFLT_CHNID_MASK 0xffff

#define CHECK_AFLT_INIT(AFLTDevFd) \
    do {\
        if (AFLTDevFd < 0)\
        {\
            HI_ERR_AFLT("AFLT is not init.\n"); \
            return HI_ERR_AFLT_NOT_INIT; \
        } \
    } while (0)


#define CHECK_AFLT_HANDLE(hAflt) \
    do {\
        if (((hAflt & 0xffff0000) != (HI_ID_AFLT << 16)) \
            || ((hAflt & AFLT_CHNID_MASK) >= AFLT_MAX_CHAN_NUM))\
        {\
            HI_ERR_AFLT("Invalid aflt handle:0x%x\n", hAflt); \
            return HI_ERR_AFLT_INVALID_HANDLE; \
        } \
    } while (0)

#define CHECK_AFLT_CREATE(u32AfltId) \
    do {\
        if (HI_NULL == s_stAfltDrv.astChanEntity[u32AfltId].pstChan) \
         {\
             HI_ERR_AFLT("aflt %d not create!\n", u32AfltId);\
             return HI_FAILURE;\
         }\
    } while (0)


     
#define HI_FATAL_AFLT(fmt...) \
                    HI_FATAL_PRINT(HI_ID_AFLT, fmt)
                
#define HI_ERR_AFLT(fmt...) \
                    HI_ERR_PRINT(HI_ID_AFLT, fmt)
                
#define HI_WARN_AFLT(fmt...) \
                    HI_WARN_PRINT(HI_ID_AFLT, fmt)
                
#define HI_INFO_AFLT(fmt...) \
                    HI_INFO_PRINT(HI_ID_AFLT, fmt)

/******************************* Struct definition *****************************/
typedef struct
{
    HI_U32      u32ParamIndex;
    HI_U32      u32ParamReserved;
    HI_U32      u32ParamStructSize;
    HI_VIRT_ADDR_T    tParamStruct;
} AFLT_PARAM_S;

typedef struct
{
    HI_U32      u32ConfigIndex;
    HI_U32      u32ConfigReserved;
    HI_U32      u32ConfigStructSize;
    HI_VIRT_ADDR_T    tConfigStruct;
} AFLT_CONFIG_S;

typedef enum 
{
    AFLT_DATA_TYPE_ES = 0, 
    AFLT_DATA_TYPE_PCM,
    AFLT_INDATA_TYPE_IEC61937_STANDARD,
    AFLT_INDATA_TYPE_IEC61937_NOSTANDARD,
    
} AFLT_DATA_TYPE_E;

typedef struct
{
    HI_U32      u32BufDataAddr;          /*buffer start addr*/
    HI_U32      u32BufSize;                 /*buffer length*/
    HI_U32      u32PtsBufBoundary;
    HI_U32      u32StreamReadPos;
} AFLT_INBUF_ATTR_S;

typedef struct
{
    HI_U32      u32BufDataAddr;
    HI_U32      u32BufSize;
    HI_U32      u32FrameBufDataSize;        //data size per frame
    HI_U32      u32FrameBufNumber;          /* 2/4/8 */
    HI_U32      u32FramePrivateInfoSize;    //private info size per frame
} AFLT_OUTBUF_ATTR_S;

typedef struct
{
    HI_U32   u32DataOffset1;        /**<Data pointer*/ /**<CNcomment: 数据偏移地址 */
    HI_U32   u32Size1;              /**<Data size*/ /**<CNcomment: 数据长度 */
    HI_U32   u32DataOffset2;        /**<Data pointer*/ /**<CNcomment: 数据偏移地址 */
    HI_U32   u32Size2;              /**<Data size*/ /**<CNcomment: 数据长度 */
} AFLT_STREAM_BUF_S;

typedef struct
{
    AFLT_COMPONENT_ID_E enAfltCompId;
    HI_BOOL  bMaster;
    HI_U32   u32MsgPoolSize;
    HI_U32   au32AuthKey[AFLT_CUSTOMER_AUTHKEY_SIZE];
    //HI_VOID *pPrivateStructure;
    //HI_U32   u32PrivateStructureSize;
} AFLT_CREATE_ATTR_S;

typedef struct
{
    HI_U32      u32FrameIndex;
    HI_U32      u32PtsReadPos;              /* wrap at u32BufBoundary */
    HI_U32      u32FrameDataOffsetAddr;     /*frame buf data offset addr*/
    HI_U32      u32FrameDataBytes;          /*frame buf data size*/
    HI_U32      u32FramePrivOffsetAddr;     /*frame buf priv info offset addr*/
    HI_U32      u32FramePrivBytes;          /*frame buf private info size*/
} AFLT_FRAME_INFO_S;

typedef struct
{
    HI_U32      u32InBufReadPos;
    HI_U32      u32InBufWritePos;

    HI_U32      u32OutBufReadIdx;
    HI_U32      u32OutBufReadWrap;
    HI_U32      u32OutBufWriteIdx;
    HI_U32      u32OutBufWriteWrap;

    HI_U32      u32TryExeCnt;
    HI_U32      u32FrameNum;
    HI_U32      u32ErrFrameNum;
    HI_BOOL     bEndOfFrame;

    HI_U32      u32ExeTimeOutCnt;
    HI_U32      u32ScheTimeOutCnt;

    HI_U32      u32StreamReadPos;
} AFLT_STATUS_INFO_S;

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_DRV_AFLT_H__ */
