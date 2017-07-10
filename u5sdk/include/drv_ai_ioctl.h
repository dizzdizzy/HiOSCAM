#ifndef __DRV_AI_IOCTL_H__
 #define __DRV_AI_IOCTL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "hi_type.h"
#include "hi_unf_ai.h"
#include "hi_drv_ao.h"

typedef struct hiAI_GetDfAttr_Param_S
{
    HI_UNF_AI_E              enAiPort;
    HI_UNF_AI_ATTR_S         stAttr;
} AI_GetDfAttr_Param_S, *AI_GetDfAttr_Param_S_PTR;

typedef struct hiAI_Create_Param_S
{
    HI_UNF_AI_E              enAiPort;
    HI_UNF_AI_ATTR_S         stAttr;
    HI_HANDLE                hAi;
//#ifdef HI_ALSA_AI_SUPPORT
    HI_BOOL                  bAlsaUse;      //if bAlsaTrack = HI_TRUE ALSA, or UNF
//#endif 
} AI_Create_Param_S, *AI_Create_Param_S_PTR;

typedef struct hiAI_Enable_Param_S
{
    HI_HANDLE          hAi;
    HI_BOOL            bAiEnable;
} AI_Enable_Param_S, *AI_Enable_Param_S_PTR;

typedef struct hiAI_GetFrame_Param_S
{
    HI_HANDLE          hAi;
    AO_FRAMEINFO_S   stAiFrame;
} AI_Frame_Param_S, *AI_Frame_Param_S_PTR;

typedef struct hiAI_Attr_Param_S
{
    HI_HANDLE                hAi;
    HI_UNF_AI_ATTR_S         stAttr;
} AI_Attr_Param_S, *AI_Attr_Param_S_PTR;

typedef struct hiAI_Buf_Param_S
{
    HI_HANDLE                hAi;
    AI_BUF_ATTR_S            stAiBuf;
} AI_Buf_Param_S, *AI_Buf_Param_S_PTR;

typedef struct hiAI_DelayComps_Param_S
{
    HI_HANDLE                       hAi;
    HI_UNF_AI_DELAY_S               stDelayComps;
} AI_DelayComps_Param_S, *AI_DelayComps_Param_S_PTR;

#if 0
typedef struct hiAI_StreamType_Param_S
{
    HI_HANDLE                       hAi;
    HI_UNF_AI_HDMI_DATA_TYPE_E      enStreamType;
} AI_StreamType_Param_S, *AI_StreamType_Param_S_PTR;
#endif

/*AI Device command code*/
#define CMD_AI_GEtDEFAULTATTR _IOWR  (HI_ID_AI, 0x00, AI_GetDfAttr_Param_S)
#define CMD_AI_CREATE _IOWR  (HI_ID_AI, 0x01, AI_Create_Param_S)
#define CMD_AI_DESTROY _IOW  (HI_ID_AI, 0x02, HI_HANDLE)
#define CMD_AI_SETENABLE _IOW  (HI_ID_AI, 0x03, AI_Enable_Param_S)
#define CMD_AI_GETENABLE _IOWR  (HI_ID_AI, 0x04, AI_Enable_Param_S)
#define CMD_AI_ACQUIREFRAME _IOWR  (HI_ID_AI, 0x05, AI_Frame_Param_S)
#define CMD_AI_RELEASEFRAME _IOW  (HI_ID_AI, 0x06, AI_Frame_Param_S)
#define CMD_AI_SETATTR _IOW  (HI_ID_AI, 0x07, AI_Attr_Param_S)
#define CMD_AI_GETATTR _IOWR  (HI_ID_AI, 0x08, AI_Attr_Param_S)
#define CMD_AI_GETBUFINFO _IOWR  (HI_ID_AI, 0x09, AI_Buf_Param_S)
#define CMD_AI_SETBUFINFO _IOW  (HI_ID_AI, 0x0a, AI_Buf_Param_S)
#define CMD_AI_SETDELAYCOMPS _IOW  (HI_ID_AI, 0x0b, AI_DelayComps_Param_S)
#define CMD_AI_GETDELAYCOMPS _IOWR  (HI_ID_AI, 0x0c, AI_DelayComps_Param_S)
#if 0
#define CMD_AI_GETSTREAMTYPE _IOWR  (HI_ID_AI, 0x0d, AI_StreamType_Param_S)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif 
 
