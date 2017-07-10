/******************************************************************************

Copyright (C), 2009-2014, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : drv_ao_ioctl.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2012/09/22
Last Modified :
Description   : ao
Function List :
History       :
* main\1    2012-09-22   z40717     init.
******************************************************************************/
#ifndef __DRV_AO_IOCTL_H__
 #define __DRV_AO_IOCTL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "hi_type.h"
#include "hi_unf_sound.h"
#include "hi_drv_ao.h"


typedef struct hiAO_SND_OpenDefault_Param_S
{
    HI_UNF_SND_E       enSound;
    HI_UNF_SND_ATTR_S  stAttr;
} AO_SND_OpenDefault_Param_S, *AO_SND_OpenDefault_Param_S_PTR;

typedef struct hiAO_SND_Open_Param_S
{
    HI_UNF_SND_E       enSound;
    HI_UNF_SND_ATTR_S  stAttr;
    HI_VOID                  *pAlsaPara;//HI_ALSA_I2S_ONLY_SUPPORT //0730
} AO_SND_Open_Param_S, *AO_SND_Open_Param_S_PTR;

typedef struct hiAO_SND_Mute_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E enOutPort;
    HI_BOOL                 bMute;
} AO_SND_Mute_Param_S, *AO_SND_Mute_Param_S_PTR;

typedef struct hiAO_SND_HdmiMode_Param_S
{
    HI_UNF_SND_E                        enSound;
    HI_UNF_SND_OUTPUTPORT_E             enOutPort;
    HI_UNF_SND_HDMI_MODE_E              enMode;
} AO_SND_HdmiMode_Param_S, *AO_SND_HdmiMode_Param_S_PTR;

typedef struct hiAO_SND_SpdifMode_Param_S
{
    HI_UNF_SND_E                        enSound;
    HI_UNF_SND_OUTPUTPORT_E             enOutPort;
    HI_UNF_SND_SPDIF_MODE_E             enMode;
} AO_SND_SpdifMode_Param_S, *AO_SND_SpdifMode_Param_S_PTR;

typedef struct hiAO_SND_SpdifSCMSMode_Param_S
{
    HI_UNF_SND_E                        enSound;
    HI_UNF_SND_OUTPUTPORT_E             enOutPort;
    HI_UNF_SND_SPDIF_SCMSMODE_E         enSCMSMode;
} AO_SND_SpdifSCMSMode_Param_S, *AO_SND_SpdifSCMSMode_Param_S_PTR;

typedef struct hiAO_SND_SpdifCategoryCode_Param_S
{
    HI_UNF_SND_E                        enSound;
    HI_UNF_SND_OUTPUTPORT_E             enOutPort;
    HI_UNF_SND_SPDIF_CATEGORYCODE_E     enCategoryCode;
} AO_SND_SpdifCategoryCode_Param_S, *AO_SND_SpdifCategoryCode_Param_S_PTR;
typedef struct hiAO_SND_Volume_Param_S
{
    HI_UNF_SND_E                        enSound;
    HI_UNF_SND_OUTPUTPORT_E             enOutPort;
    HI_UNF_SND_GAIN_ATTR_S stGain;
} AO_SND_Volume_Param_S, *AO_SND_Volume_Param_S_PTR;

typedef struct hiAO_SND_SampleRate_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E enOutPort;
    HI_UNF_SAMPLE_RATE_E    enSampleRate;
} AO_SND_SampleRate_Param_S, *AO_SND_SampleRate_Param_S_PTR;

typedef struct hiAO_SND_TrackMode_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E enOutPort;
    HI_UNF_TRACK_MODE_E     enMode;
} AO_SND_TrackMode_Param_S, *AO_SND_TrackMode_Param_S_PTR;

typedef struct hiAO_SND_AllTrackMute_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_BOOL                 bMute;
} AO_SND_AllTrackMute_Param_S, *AO_SND_AllTrackMute_Param_S_PTR;

typedef struct hiAO_SND_SmartVolume_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E enOutPort;
    HI_BOOL                 bSmartVolume;
} AO_SND_SmartVolume_Param_S, *AO_SND_SmartVolume_Param_S_PTR;


typedef struct hiAO_Track_Create_Param_S
{
    HI_UNF_SND_E             enSound;
    HI_UNF_AUDIOTRACK_ATTR_S stAttr;
    HI_BOOL                  bAlsaTrack;      //if bAlsaTrack = HI_TRUE ALSA, or UNF
    AO_BUF_ATTR_S            stBuf;          //Only for compatible alsa
    HI_HANDLE                hTrack;
} AO_Track_Create_Param_S, *AO_Track_Create_Param_S_PTR;

typedef struct hiAO_SND_SetPeriodSize_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E enOutPort;
    HI_U32                  u32PeriodSize;
    HI_VOID                 (*Func)(HI_VOID);
} AO_SND_SetPeriodSize_Param_S;

typedef struct hiAO_Track_Attr_Param_S
{
    HI_HANDLE                    hTrack;
    HI_UNF_AUDIOTRACK_ATTR_S     stAttr;
} AO_Track_Attr_Param_S, *AO_Track_Attr_Param_S_PTR;

typedef struct hiAO_Track_Weight_Param_S
{
    HI_HANDLE               hTrack;
    HI_UNF_SND_GAIN_ATTR_S  stTrackGain;
} AO_Track_Weight_Param_S, *AO_Track_Weight_Param_S_PTR;


typedef struct hiAO_Track_AbsGain_Param_S
{
    HI_HANDLE               hTrack;
    HI_UNF_SND_ABSGAIN_ATTR_S  stTrackAbsGain;
} AO_Track_AbsGain_Param_S, *AO_Track_AbsGain_Param_S_PTR;

typedef struct hiAO_Track_Mute_Param_S
{
    HI_HANDLE               hTrack;
    HI_BOOL                 bMute;
} AO_Track_Mute_Param_S, *AO_Track_Mute_Param_S_PTR;

typedef struct hiAO_Track_ChannelMode_Param_S
{
    HI_HANDLE               hTrack;
    HI_UNF_TRACK_MODE_E     enMode;
} AO_Track_ChannelMode_Param_S, *AO_Track_ChannelMode_Param_S_PTR;

typedef struct hiAO_Track_SendData_Param_S
{
    HI_HANDLE              hTrack;
    HI_UNF_AO_FRAMEINFO_S  stAOFrame;
} AO_Track_SendData_Param_S, *AO_Track_SendData_Param_S_PTR;

typedef struct hiAO_Track_GetTrackMapInfo_Param_S
{
    HI_HANDLE                hTrack;
    HI_U32                   u32AipRegAddr;
    HI_TRACK_MmapAttr_S stTrackMmapAttr;
} AO_Track_GetTrackMapInfo_Param_S, *AO_Track_GetTrackMapInfo_Param_S_PTR;

typedef struct hiAO_Track_SpeedAdjust_Param_S
{
    HI_HANDLE                 hTrack;
    AO_SND_SPEEDADJUST_TYPE_E enType;
    HI_S32                    s32Speed;
} AO_Track_SpeedAdjust_Param_S, *AO_Track_SpeedAdjust_Param_S_PTR;

typedef struct hiAO_Track_DelayMs_Param_S
{
    HI_HANDLE hTrack;
    HI_U32    u32DelayMs;
} AO_Track_DelayMs_Param_S, *AO_Track_DelayMs_Param_S_PTR;

typedef struct hiAO_Track_BufEmpty_Param_S
{
    HI_HANDLE hTrack;
    HI_BOOL   bEmpty;
} AO_Track_BufEmpty_Param_S, *AO_Track_BufEmpty_Param_S_PTR;

typedef struct hiAO_Track_EosFlag_Param_S
{
    HI_HANDLE hTrack;
    HI_BOOL   bEosFlag;
} AO_Track_EosFlag_Param_S, *AO_Track_EosFlag_Param_S_PTR;

typedef struct hiAO_Track_Status_Param_S
{
    HI_HANDLE hTrack;
    HI_VOID * pstStatus;
} AO_Track_Status_Param_S, *AO_Track_Status_Param_S_PTR;

typedef struct hiAO_Track_AttAi_Param_S
{
    HI_HANDLE hTrack;
    HI_HANDLE hAi;
} AO_Track_AttAi_Param_S, *AO_Track_AttAi_Param_S_PTR;

typedef struct hiAO_Cast_Mute_Param_S
{
    HI_HANDLE               hCast;
    HI_BOOL                 bMute;
} AO_Cast_Mute_Param_S, *AO_Cast_Mute_Param_S_PTR;

typedef struct hiAO_Cast_AbsGain_Param_S
{
    HI_HANDLE               hCast;
    HI_UNF_SND_ABSGAIN_ATTR_S  stCastAbsGain;
} AO_Cast_AbsGain_Param_S, *AO_Cast_AbsGain_Param_S_PTR;

typedef struct hiAO_Cast_Create_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_HANDLE               hCast;
    HI_U32                     u32ReqSize;
    HI_UNF_SND_CAST_ATTR_S stCastAttr;
} AO_Cast_Create_Param_S, *AO_Cast_Create_Param_S_PTR;
typedef struct hiAO_Cast_Info_Param_S
{
    HI_HANDLE          hCast;
    HI_U32                u32UserVirtAddr;
    HI_U32                u32KernelVirtAddr;
    HI_U32                u32PhyAddr;

    HI_U32                u32FrameBytes;
    HI_U32                u32FrameSamples;
    HI_U32                u32Channels;
    HI_S32                s32BitPerSample;
    
} AO_Cast_Info_Param_S, *AO_Cast_Info_Param_S_PTR;

typedef struct hiAO_Cast_Enable_Param_S
{
    HI_HANDLE          hCast;
    HI_BOOL              bCastEnable;
} AO_Cast_Enable_Param_S, *AO_Cast_Enable_Param_S_PTR;

typedef struct hiAO_Cast_Data_Param_S
{
    HI_HANDLE          hCast;
    HI_U32                u32FrameBytes;
    HI_U32                u32SampleBytes;
    
    HI_U32                u32DataOffset;
    HI_UNF_AO_FRAMEINFO_S   stAOFrame;
} AO_Cast_Data_Param_S, *AO_Cast_Data_Param_S_PTR;

typedef struct hiAO_SND_AttAef_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_U32                  u32AefId;
    HI_U32                  u32AefProcAddr;
} AO_SND_AttAef_Param_S, *AO_SND_AttAef_Param_S_PTR;

typedef struct hiAO_SND_AefBypass_Param_S
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E enOutPort;
    HI_BOOL                 bBypass;
} AO_SND_AefBypass_Param_S, *AO_SND_AefBypass_Param_S_PTR;

typedef struct hiAO_SND_GET_XRUN_S
{
    HI_UNF_SND_E            enSound;
    HI_U32                  u32Count;
} AO_SND_Get_Xrun_Param_S, *AO_SND_Get_Xrun_Param_S_PTR;


typedef struct hiAO_SND_SET_LOWLATENCY
{
    HI_UNF_SND_E            enSound;
    HI_UNF_SND_OUTPUTPORT_E eOutPort;
    HI_U32                  u32LatencyMs;
} AO_SND_Set_LowLatency_Param_S, *AO_SND_Set_LowLatency_Param_S_PTR;

typedef struct hiAO_Track_Priority_Param_S
{
    HI_HANDLE hTrack;
    HI_BOOL   bEnable;
} AO_Track_Priority_Param_S, *AO_Track_Priority_Param_S_PTR;

typedef struct hiAO_Track_FifoBypass_Param_S
{
    HI_HANDLE hTrack;
    HI_BOOL   bEnable;
} AO_Track_FifoBypass_Param_S, *AO_Track_FifoBypass_Param_S_PTR;

/********************************************************
  AO command code definition
 *********************************************************/
/*AO SND command code*/
 #define CMD_AO_GETSNDDEFOPENATTR _IOWR  (HI_ID_AO, 0x00, AO_SND_OpenDefault_Param_S)
 #define CMD_AO_SND_OPEN _IOWR (HI_ID_AO, 0x01, AO_SND_Open_Param_S)
 #define CMD_AO_SND_CLOSE _IOW  (HI_ID_AO, 0x02, HI_UNF_SND_E)
 #define CMD_AO_SND_SETMUTE _IOW (HI_ID_AO, 0x03, AO_SND_Mute_Param_S)
 #define CMD_AO_SND_GETMUTE _IOWR (HI_ID_AO, 0x04, AO_SND_Mute_Param_S)
 #define CMD_AO_SND_SETHDMIMODE _IOW (HI_ID_AO, 0x05, AO_SND_HdmiMode_Param_S)
 #define CMD_AO_SND_SETSPDIFMODE _IOW (HI_ID_AO, 0x06, AO_SND_SpdifMode_Param_S)
 #define CMD_AO_SND_SETVOLUME _IOW (HI_ID_AO, 0x07, AO_SND_Volume_Param_S)
 #define CMD_AO_SND_GETVOLUME _IOWR (HI_ID_AO, 0x08, AO_SND_Volume_Param_S)
 #define CMD_AO_SND_SETSAMPLERATE _IOW (HI_ID_AO, 0x09, AO_SND_SampleRate_Param_S)
 #define CMD_AO_SND_GETSAMPLERATE _IOWR (HI_ID_AO, 0x0a, AO_SND_SampleRate_Param_S)
 #define CMD_AO_SND_SETTRACKMODE _IOW (HI_ID_AO, 0x0b, AO_SND_TrackMode_Param_S)
 #define CMD_AO_SND_GETTRACKMODE _IOWR (HI_ID_AO, 0x0c, AO_SND_TrackMode_Param_S)
 #define CMD_AO_SND_SETSMARTVOLUME _IOW (HI_ID_AO, 0x0d, AO_SND_SmartVolume_Param_S)
 #define CMD_AO_SND_GETSMARTVOLUME _IOWR (HI_ID_AO, 0x0e, AO_SND_SmartVolume_Param_S)
 #define CMD_AO_SND_SETCASTENABLE _IOW (HI_ID_AO, 0x0f, AO_SND_CastEnable_Param_S)  
 #define CMD_AO_SND_READCASTDATA _IOW (HI_ID_AO, 0x10, AO_SND_CastData_Param_S)  //_IOW verify
 #define CMD_AO_SND_GETHDMIMODE _IOWR (HI_ID_AO, 0x11, AO_SND_HdmiMode_Param_S)
 #define CMD_AO_SND_GETSPDIFMODE _IOWR (HI_ID_AO, 0x12, AO_SND_SpdifMode_Param_S)
 #define CMD_AO_SND_SETSPDIFSCMSMODE _IOW (HI_ID_AO, 0x13, AO_SND_SpdifSCMSMode_Param_S)
 #define CMD_AO_SND_GETSPDIFSCMSMODE _IOWR (HI_ID_AO, 0x14, AO_SND_SpdifSCMSMode_Param_S)
 #define CMD_AO_SND_SETSPDIFCATEGORYCODE _IOW (HI_ID_AO, 0x15, AO_SND_SpdifCategoryCode_Param_S)
 #define CMD_AO_SND_GETSPDIFCATEGORYCODE _IOWR (HI_ID_AO, 0x16, AO_SND_SpdifCategoryCode_Param_S)
 
 #define CMD_AO_SND_ATTACHTRACK _IOW (HI_ID_AO, 0x020, HI_HANDLE)
 #define CMD_AO_SND_DETACHTRACK _IOW  (HI_ID_AO, 0x21, HI_HANDLE)

 #define CMD_AO_SND_ATTACHAEF _IOWR (HI_ID_AO, 0x022, AO_SND_AttAef_Param_S)
 #define CMD_AO_SND_DETACHAEF _IOW  (HI_ID_AO, 0x23, AO_SND_AttAef_Param_S)
 #define CMD_AO_SND_SETAEFBYPASS _IOW (HI_ID_AO, 0x024, AO_SND_AefBypass_Param_S)
 #define CMD_AO_SND_GETAEFBYPASS _IOWR (HI_ID_AO, 0x25, AO_SND_AefBypass_Param_S)
 
 #define CMD_AO_SND_SETALLTRACKMUTE _IOW  (HI_ID_AO, 0x26, AO_SND_AllTrackMute_Param_S)
 #define CMD_AO_SND_GETALLTRACKMUTE _IOWR	(HI_ID_AO, 0x27, AO_SND_AllTrackMute_Param_S)
 #define CMD_AO_SND_GETXRUNCOUNT _IOWR	(HI_ID_AO, 0x28, AO_SND_Get_Xrun_Param_S)
 #define CMD_AO_SND_SETLOWLATENCY _IOW  (HI_ID_AO, 0x29, AO_SND_Set_LowLatency_Param_S)
 #define CMD_AO_SND_GETLOWLATENCY _IOWR  (HI_ID_AO, 0x2a, AO_SND_Set_LowLatency_Param_S)

/*AO Track command code*/
 #define CMD_AO_TRACK_GETDEFATTR _IOWR  (HI_ID_AO, 0x40, HI_UNF_AUDIOTRACK_ATTR_S)
 #define CMD_AO_TRACK_CREATE _IOWR (HI_ID_AO, 0x41, AO_Track_Create_Param_S)
 #define CMD_AO_TRACK_DESTROY _IOW  (HI_ID_AO, 0x42, HI_HANDLE)
 #define CMD_AO_TRACK_START _IOW  (HI_ID_AO, 0x43, HI_HANDLE)
 #define CMD_AO_TRACK_STOP _IOW  (HI_ID_AO, 0x44, HI_HANDLE)
 #define CMD_AO_TRACK_PAUSE _IOW  (HI_ID_AO, 0x45, HI_HANDLE)
 #define CMD_AO_TRACK_FLUSH _IOW  (HI_ID_AO, 0x46, HI_HANDLE)
 #define CMD_AO_TRACK_SENDDATA _IOW  (HI_ID_AO, 0x47, AO_Track_SendData_Param_S)
 #define CMD_AO_TRACK_SETWEITHT _IOW  (HI_ID_AO, 0x48, AO_Track_Weight_Param_S)
 #define CMD_AO_TRACK_GETWEITHT _IOWR  (HI_ID_AO, 0x49, AO_Track_Weight_Param_S)
 #define CMD_AO_TRACK_SETSPEEDADJUST _IOW  (HI_ID_AO, 0x4a, AO_Track_SpeedAdjust_Param_S)
 #define CMD_AO_TRACK_GETDELAYMS _IOWR  (HI_ID_AO, 0x4b, AO_Track_DelayMs_Param_S)
 #define CMD_AO_TRACK_ISBUFEMPTY _IOWR  (HI_ID_AO, 0x4c, AO_Track_BufEmpty_Param_S)
 #define CMD_AO_TRACK_SETEOSFLAG _IOW  (HI_ID_AO, 0x4d, AO_Track_Status_Param_S)
 #define CMD_AO_TRACK_GETATTR _IOWR  (HI_ID_AO, 0x4e, AO_Track_Attr_Param_S)
 #define CMD_AO_TRACK_SETATTR _IOW  (HI_ID_AO, 0x4f, AO_Track_Attr_Param_S)
 //+
 #define CMD_AO_TRACK_GETFREEBUfSIZE _IOWR  (HI_ID_AO, 0x50, AO_Track_Status_Param_S)
 #define CMD_AO_TRACK_ATTACHAI _IOW  (HI_ID_AO, 0x51, AO_Track_AttAi_Param_S)
 #define CMD_AO_TRACK_DETACHAI _IOW  (HI_ID_AO, 0x52, AO_Track_AttAi_Param_S)
 #define CMD_AO_TRACK_SETABSGAIN _IOW  (HI_ID_AO, 0x51, AO_Track_AbsGain_Param_S)
 #define CMD_AO_TRACK_GETABSGAIN _IOWR  (HI_ID_AO, 0x52, AO_Track_AbsGain_Param_S)
 #define CMD_AO_TRACK_SETMUTE _IOW  (HI_ID_AO, 0x53, AO_Track_Mute_Param_S)
 #define CMD_AO_TRACK_GETMUTE _IOWR  (HI_ID_AO, 0x54, AO_Track_Mute_Param_S)

#define CMD_AO_TRACK_SETCHANNELMODE _IOW  (HI_ID_AO, 0x55, AO_Track_ChannelMode_Param_S)
#define CMD_AO_TRACK_GETCHANNELMODE _IOWR  (HI_ID_AO, 0x56, AO_Track_ChannelMode_Param_S)
 #define CMD_AO_TRACK_SETFIFOBYPASS _IOW  (HI_ID_AO, 0x57, AO_Track_FifoBypass_Param_S)
 #define CMD_AO_TRACK_GETFIFOBYPASS _IOWR  (HI_ID_AO, 0x58, AO_Track_FifoBypass_Param_S)
 #define CMD_AO_TRACK_MMAPTRACKATTR _IOWR  (HI_ID_AO, 0x59, AO_Track_GetTrackMapInfo_Param_S)
 #define CMD_AO_TRACK_SETPRIORITY _IOW  (HI_ID_AO, 0x5a, AO_Track_Priority_Param_S)
 #define CMD_AO_TRACK_GETPRIORITY _IOWR  (HI_ID_AO, 0x5b, AO_Track_Priority_Param_S)
/*AO Cast command code*/
 #define CMD_AO_CAST_GETDEFATTR _IOWR  (HI_ID_AO, 0x60, HI_UNF_SND_CAST_ATTR_S)
 #define CMD_AO_CAST_CREATE _IOWR (HI_ID_AO, 0x61, AO_Cast_Create_Param_S)
 #define CMD_AO_CAST_DESTROY _IOW  (HI_ID_AO, 0x62, HI_HANDLE)
 #define CMD_AO_CAST_SETENABLE _IOW  (HI_ID_AO, 0x63, AO_Cast_Enable_Param_S)
 #define CMD_AO_CAST_GETENABLE _IOWR  (HI_ID_AO, 0x64, AO_Cast_Enable_Param_S)
 #define CMD_AO_CAST_GETINFO  _IOWR  (HI_ID_AO, 0x65, AO_Cast_Info_Param_S)
 #define CMD_AO_CAST_SETINFO  _IOW  (HI_ID_AO, 0x66, AO_Cast_Info_Param_S)
 #define CMD_AO_CAST_ACQUIREFRAME  _IOWR  (HI_ID_AO, 0x67, AO_Cast_Data_Param_S)
 #define CMD_AO_CAST_RELEASEFRAME  _IOW  (HI_ID_AO, 0x68, AO_Cast_Data_Param_S)
 #define CMD_AO_CAST_SETABSGAIN _IOW  (HI_ID_AO, 0x69, AO_Cast_AbsGain_Param_S)
 #define CMD_AO_CAST_GETABSGAIN _IOWR  (HI_ID_AO, 0x6A, AO_Cast_AbsGain_Param_S)
 #define CMD_AO_CAST_SETMUTE _IOW  (HI_ID_AO, 0x6B, AO_Cast_Mute_Param_S)
 #define CMD_AO_CAST_GETMUTE _IOWR  (HI_ID_AO, 0x6C, AO_Cast_Mute_Param_S)
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
 
 #endif
