/******************************************************************************

Copyright (C), 2009-2014, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : drv_ade_ioctl.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2013-01-22
Last Modified :
Description   : audio dsp decoder
Function List :
History       :
* main\1    2013-01-22   zgjie     init.
******************************************************************************/
#ifndef __DRV_ADE_IOCTL_H__
#define __DRV_ADE_IOCTL_H__

#include "hi_drv_ade.h"
#include "hi_audsp_ade.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

/***********************/
typedef struct
{
    ADE_CHAN_ID_E enADEId;
    ADE_OPEN_DECODER_S   *pstOpenAttr;
} ADE_OPEN_DECODER_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_CLOSE_DECODER_Param_S;

typedef struct
{
    ADE_CHAN_ID_E     enADEId;
    ADE_INBUF_ATTR_S   *pstInbuf;
} ADE_ATTACH_INPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_DETACH_INPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E     enADEId;
    ADE_INBUF_ATTR_S   *pstInbuf;
} ADE_INIT_INPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_DEINIT_INPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E     enADEId;
    ADE_OUTBUF_ATTR_S   *pstInbuf;
} ADE_INIT_OUTPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_DEINIT_OUTPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_START_DECODER_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_STOP_DECODER_Param_S;

typedef struct
{
    ADE_CHAN_ID_E       enADEId;
    ADE_IOCTRL_PARAM_S *pstCtrlParams;
} ADE_IOCTL_DECODER_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
    HI_U32        u32CmdRetCode;
} ADE_CHECK_CMDDONE_Param_S;

typedef struct
{
    ADE_CHAN_ID_E    enADEId;
    ADE_INBUF_SYNC_S *pstSync;
} ADE_SYNC_INPUTBUF_Param_S;

typedef struct
{
    ADE_CHAN_ID_E    enADEId;
    ADE_INBUF_SYNC_WPOS_S *pstSync;
} ADE_SYNC_INPUTBUFWPOS_Param_S;

typedef struct
{
    ADE_CHAN_ID_E    enADEId;
    ADE_INBUF_SYNC_RPOS_S *pstSync;
} ADE_SYNC_INPUTBUFRPOS_Param_S;


typedef struct
{
    ADE_CHAN_ID_E enADEId;
    HI_U32        u32PtsReadPos;
} ADE_GET_PTSREADPOS_Param_S;

typedef struct
{
    ADE_CHAN_ID_E enADEId;
} ADE_SET_EOSFLAG_Param_S;

typedef struct
{
    ADE_CHAN_ID_E           enADEId;
    ADE_FRMAE_BUF_S        *pstAOut;
    HI_VOID *               pPrivate;
} ADE_RECEIVE_FRAME_Param_S;

typedef struct
{
    ADE_CHAN_ID_E           enADEId;
    HI_U32 u32FrameIdx;
} ADE_RELEASE_FRAME_Param_S;

/*ADE command code*/
#define CMD_ADE_OPEN_DECODER _IOWR (HI_ID_ADE, 0x00, ADE_OPEN_DECODER_Param_S)     //ade cmd
#define CMD_ADE_CLOSE_DECODER _IOW  (HI_ID_ADE, 0x01, ADE_CLOSE_DECODER_Param_S)   //ade cmd
#define CMD_ADE_START_DECODER _IOW  (HI_ID_ADE, 0x04, ADE_START_DECODER_Param_S) //ade cmd
#define CMD_ADE_STOP_DECODER _IOW  (HI_ID_ADE, 0x05, ADE_STOP_DECODER_Param_S) //ade cmd
#define CMD_ADE_IOCONTROL_DECODER _IOWR  (HI_ID_ADE, 0x06, ADE_IOCTL_DECODER_Param_S) //ade cmd
#define CMD_ADE_CHECK_CMDDONE _IOW  (HI_ID_ADE, 0x07, ADE_CHECK_CMDDONE_Param_S) //ade reg
#define CMD_ADE_SYNC_INPUTBUF _IOWR  (HI_ID_ADE, 0x08, ADE_SYNC_INPUTBUF_Param_S) //ade reg
#define CMD_ADE_GET_PTSREADPOS _IOWR  (HI_ID_ADE, 0x09, ADE_GET_PTSREADPOS_Param_S) //ade reg
#define CMD_ADE_SET_EOSFLAG _IOWR  (HI_ID_ADE, 0x0a, ADE_SET_EOSFLAG_Param_S) //ade reg
#define CMD_ADE_INIT_INPUTBUF _IOWR  (HI_ID_ADE, 0x10, ADE_INIT_INPUTBUF_Param_S) //ade cmd
#define CMD_ADE_DEINIT_INPUTBUF _IOWR  (HI_ID_ADE, 0x11, ADE_DEINIT_INPUTBUF_Param_S) //ade cmd
#define CMD_ADE_INIT_OUTPUTBUF _IOWR  (HI_ID_ADE, 0x12, ADE_INIT_OUTPUTBUF_Param_S) //ade cmd
#define CMD_ADE_DEINIT_OUTPUTBUF _IOWR  (HI_ID_ADE, 0x13, ADE_DEINIT_OUTPUTBUF_Param_S) //ade cmd
#define CMD_ADE_RECEIVE_FRAME _IOWR  (HI_ID_ADE, 0x14, ADE_RECEIVE_FRAME_Param_S) //ade cmd
#define CMD_ADE_RELEASE_FRAME _IOW  (HI_ID_ADE, 0x15, ADE_RELEASE_FRAME_Param_S) //ade cmd
#define CMD_ADE_SYNC_INPUTBUFWPOS _IOWR  (HI_ID_ADE, 0x16, ADE_SYNC_INPUTBUFWPOS_Param_S) //ade reg
#define CMD_ADE_SYNC_INPUTBUFRPOS _IOWR  (HI_ID_ADE, 0x17, ADE_SYNC_INPUTBUFRPOS_Param_S) //ade reg

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __DRV_VDEC_IOCTL_H__ */
