#ifndef __DRV_AFLT_IOCTL_H__
#define __DRV_AFLT_IOCTL_H__

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

#include "hi_drv_aflt.h"

/***********************/
typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_CREATE_ATTR_S  stCreateAttr;
} AFLT_Create_Param_S, *AFLT_Create_Param_S_PTR;

typedef struct
{
    HI_HANDLE       hAflt;
    AFLT_PARAM_S    stParam;
} AFLT_Setting_Param_S, *AFLT_Setting_Param_S_PTR;

typedef struct
{
    HI_HANDLE       hAflt;
    AFLT_CONFIG_S   stConfig;
} AFLT_Setting_Config_S, *AFLT_Setting_Config_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_INBUF_ATTR_S   stInbuf;
} AFLT_Inbuf_Param_S, *AFLT_Inbuf_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_OUTBUF_ATTR_S  stOutbuf;
} AFLT_Outbuf_Param_S, *AFLT_Outbuf_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_STREAM_BUF_S   stStreambuf;
    HI_U32              u32RequestSize;
} AFLT_GetBuf_Param_S, *AFLT_GetBuf_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_STREAM_BUF_S   stStreambuf;
} AFLT_PutBuf_Param_S, *AFLT_PutBuf_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_FRAME_INFO_S   stFrameInfo;
} AFLT_AcquireFrame_Param_S, *AFLT_AcquireFrame_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    HI_U32              u32FrameIdx;
} AFLT_ReleaseFrame_Param_S, *AFLT_ReleaseFrame_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_STATUS_INFO_S  stStatusInfo;
} AFLT_StatusInfo_Param_S, *AFLT_StatusInfo_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    HI_BOOL             bEosFlag;
} AFLT_EOSFLAG_Param_S, *AFLT_EOSFLAG_Param_S_PTR;

typedef struct
{
    HI_HANDLE           hAflt;
    AFLT_STATUS_E       enCurnStatus;
} AFLT_Status_Param_S, *AFLT_Status_Param_S_PTR;

/*AFLT command code*/
#define CMD_AFLT_CREATE _IOWR (HI_ID_AFLT, 0x00, AFLT_Create_Param_S)
#define CMD_AFLT_DESTROY _IOW  (HI_ID_AFLT, 0x01, HI_HANDLE)
#define CMD_AFLT_START _IOW (HI_ID_AFLT, 0x02, HI_HANDLE)
#define CMD_AFLT_STOP _IOW (HI_ID_AFLT, 0x03, HI_HANDLE)
#define CMD_AFLT_SETPARAM _IOW (HI_ID_AFLT, 0x04, AFLT_Setting_Param_S)
#define CMD_AFLT_GETPARAM _IOWR (HI_ID_AFLT, 0x05, AFLT_Setting_Param_S)
#define CMD_AFLT_SETCONFIG _IOW (HI_ID_AFLT, 0x06, AFLT_Setting_Config_S)
#define CMD_AFLT_GETCONFIG _IOWR (HI_ID_AFLT, 0x07, AFLT_Setting_Config_S)
#define CMD_AFLT_GETSTATUS _IOWR (HI_ID_AFLT, 0x08, AFLT_Status_Param_S)

#define CMD_AFLT_INITINBUF _IOWR (HI_ID_AFLT, 0x10, AFLT_Inbuf_Param_S)
#define CMD_AFLT_DEINITINBUF _IOW (HI_ID_AFLT, 0x11, HI_HANDLE)
#define CMD_AFLT_INITOUTBUF _IOWR (HI_ID_AFLT, 0x12, AFLT_Outbuf_Param_S)
#define CMD_AFLT_DEINITOUTBUF _IOW (HI_ID_AFLT, 0x13, HI_HANDLE)

#define CMD_AFLT_GETBUF _IOWR (HI_ID_AFLT, 0x20, AFLT_GetBuf_Param_S)
#define CMD_AFLT_PUTBUF _IOW  (HI_ID_AFLT, 0x21, AFLT_PutBuf_Param_S)
#define CMD_AFLT_ACQUIREFRAME _IOWR (HI_ID_AFLT, 0x22, AFLT_AcquireFrame_Param_S)
#define CMD_AFLT_RELEASEFRAME _IOW (HI_ID_AFLT, 0x23, AFLT_ReleaseFrame_Param_S)

#define CMD_AFLT_SETEOSFLAG _IOW (HI_ID_AFLT, 0x24, AFLT_EOSFLAG_Param_S)

#define CMD_AFLT_GETSTATUSINFO _IOWR (HI_ID_AFLT, 0x25, AFLT_StatusInfo_Param_S)

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __DRV_AFLT_IOCTL_H__ */
