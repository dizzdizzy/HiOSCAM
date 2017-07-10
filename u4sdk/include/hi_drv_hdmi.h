/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_priv_hdmi.h
  Version       : Initial Draft
  Author        : q46153 l00168554
  Created       : 2010/6/15
  Last Modified :
  Description   : hdmi ioctl and HDMI API common data structs

******************************************************************************/

#ifndef  __HI_DRV_HDMI_H__
#define  __HI_DRV_HDMI_H__

//#include "hi_common_id.h"
#include "hi_module.h"
//#include "hi_common_log.h"
#include "hi_debug.h"

#include "hi_unf_hdmi.h"
#include "hi_error_mpi.h"
#include "hi_drv_disp.h"
#include "hi_drv_video.h"

#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif

/*
**HDMI Debug
*/
#ifndef HI_ADVCA_FUNCTION_RELEASE
#define HI_FATAL_HDMI(fmt...)       HI_FATAL_PRINT  (HI_ID_HDMI, fmt)
#define HI_ERR_HDMI(fmt...)         HI_ERR_PRINT    (HI_ID_HDMI, fmt)
#define HI_WARN_HDMI(fmt...)        HI_WARN_PRINT   (HI_ID_HDMI, fmt)
#define HI_INFO_HDMI(fmt...)        HI_INFO_PRINT   (HI_ID_HDMI, fmt)

#define debug_printk(fmt,args...) // printk(fmt,##args)
#else

#define HI_FATAL_HDMI(fmt...) 
#define HI_ERR_HDMI(fmt...) 
#define HI_WARN_HDMI(fmt...)
#define HI_INFO_HDMI(fmt...)
#define debug_printk(fmt,args...)  

#endif

/*hdmi audio interface */
typedef enum  hiHDMI_AUDIOINTERFACE_E
{
    HDMI_AUDIO_INTERFACE_I2S,
    HDMI_AUDIO_INTERFACE_SPDIF, 
    HDMI_AUDIO_INTERFACE_HBR, 
    HDMI_AUDIO_INTERFACE_BUTT
}HDMI_AUDIOINTERFACE_E;


typedef struct hiHDMI_AUDIO_ATTR_S
{
    HDMI_AUDIOINTERFACE_E   enSoundIntf;         /**<the origin of Sound,suggestion set HI_UNF_SND_INTERFACE_I2S,the parameter need consistent with Ao input *//**<CNcomment:HDMI音频来源, 建议HI_UNF_SND_INTERFACE_I2S,此参数需要与AO输入保持一致 */
    HI_BOOL                 bIsMultiChannel;     /**<set mutiChannel or stereo ;0:stereo,1:mutichannel fixup 8 channel *//**<CNcomment:多声道还是立体声，0:立体声，1:多声道固定为8声道 */
	HI_U32 					u32Channels;         //先channel和multy channel都保留，后续在内核态干掉multy channel
    HI_UNF_SAMPLE_RATE_E    enSampleRate;        /**<the samplerate of audio,this parameter consistent with AO config *//**<CNcomment:PCM音频采样率,此参数需要与AO的配置保持一致 */
    HI_U8                   u8DownSampleParm;    /**<PCM parameter of dowmsample,default 0*//**CNcomment:PCM音频向下downsample采样率的参数，默认为0 */
    
    HI_UNF_BIT_DEPTH_E      enBitDepth;          //目前默认配16bit    /**<the audio bit depth,defualt 16,this parameter consistent with AO config*//**<CNcomment:音频位宽，默认为16,此参数需要与AO的配置保持一致 */
    HI_U8                   u8I2SCtlVbit;        /**<reserve:config 0,I2S control(0x7A:0x1D)*//**CNcomment:保留，请配置为0, I2S control (0x7A:0x1D) */

    HI_UNF_EDID_AUDIO_FORMAT_CODE_E enAudioCode;
}HDMI_AUDIO_ATTR_S;

#if defined(HI_HDMI_SUPPORT_2_0)
typedef enum
{
	DRV_HDMI_HV_SYNC_POL_HPVP,
	DRV_HDMI_HV_SYNC_POL_HPVN,
	DRV_HDMI_HV_SYNC_POL_HNVP,
	DRV_HDMI_HV_SYNC_POL_HNVN
} HI_DRV_HDMI_HVSYNC_POLARITY_E;

typedef enum {
	DRV_HDMI_ACTIVE_ASPECT_16_9_TOP = 2,
	DRV_HDMI_ACTIVE_ASPECT_14_9_TOP = 3,
	DRV_HDMI_ACTIVE_ASPECT_16_9_CENTER = 4,
	DRV_HDMI_ACTIVE_ASPECT_PICTURE = 8,
	DRV_HDMI_ACTIVE_ASPECT_4_3 = 9,
	DRV_HDMI_ACTIVE_ASPECT_16_9 = 10,
	DRV_HDMI_ACTIVE_ASPECT_14_9 = 11,
	DRV_HDMI_ACTIVE_ASPECT_4_3_SP_14_9 = 13,
	DRV_HDMI_ACTIVE_ASPECT_16_9_SP_14_9 = 14,
	DRV_HDMI_ACTIVE_ASPECT_16_9_SP_4_3 = 15,
}HI_DRV_HDMI_ACTIVE_ASPECT_E;

/*In order to extern ,so we define struct*/
typedef struct hiHDMI_VIDEO_ATTR_S
{

    HI_DRV_DISP_FMT_E             enVideoFmt;          /**<video fromat ,the format must consistent with display  config*//**<CNcomment:视频制式,此参数需要与Display配置的制式保持一致 */
    HI_U32                        u32ClkFs; 
    HI_U32                        u32PixelRepeat;       
    HI_UNF_EDID_3D_TYPE_E         enStereoMode;
    HI_DRV_HDMI_HVSYNC_POLARITY_E enHvSyncPol;
    HI_UNF_HDMI_DEEP_COLOR_E      enInBitDepth;
    HI_U32                        u32ColorSpace;
    HI_U32                        u32DispRatioWidth;
    HI_U32                        u32DispRatioHeight;    
/*
    HI_UNF_HDMI_VIDEO_MODE_E      enInColorSpace; 
    HI_UNF_HDMI_COLORSPACE_E      enColorimetry;
    HI_UNF_HDMI_RGB_QUAN_RAGE_E   enRGBQuantization;
    HI_UNF_HDMI_YCC_QUAN_RAGE_E   enYCCQuantization;

    HI_UNF_HDMI_ASPECT_RATIO_E    enPictureAspect;
    HI_DRV_HDMI_ACTIVE_ASPECT_E   enActiveAspect; 
    HI_UNF_HDMI_PICTURE_SCALING_E enPictureScaling;
*/
    
  
}HDMI_VIDEO_ATTR_S;
#else
/*In order to extern ,so we define struct*/
typedef struct hiHDMI_VIDEO_ATTR_S
{
    HI_DRV_DISP_FMT_E       enVideoFmt;          /**<video fromat ,the format must consistent with display  config*//**<CNcomment:视频制式,此参数需要与Display配置的制式保持一致 */
    HI_BOOL                 b3DEnable;           /**<0:disable 3d,1,enable 3d mode*//**<CNcomment:< 0:3D不激活，1:3D模式打开 */
    HI_U32                  u83DParam;           /**<3D Parameter,defualt HI_FALSE*//**<CNcomment:< 3D Parameter, 默认为HI_FALSE */
}HDMI_VIDEO_ATTR_S;

/*In order to extern ,so we define struct*/
typedef struct hiHDMI_APP_ATTR_S
{
	HI_BOOL                 bEnableHdmi;         /**<force to HDMI or DVI,the value must set before HI_UNF_HDMI_Start or behind HI_UNF_HDMI_Stop*//**<CNcomment:是否强制HDMI,否则为DVI.该值必须在 HI_UNF_HDMI_Start之前或者HI_UNF_HDMI_Stop之后设置  */
    HI_BOOL                 bEnableVideo;        /**<parameter must set HI_TRUE,or the HDMI diver will force to set HI_TRUE*//**<CNcomment:必须是HI_TRUE, 如果是HI_FALSE:HDMI驱动会强制设置为HI_TRUE */
    HI_BOOL                 bEnableAudio;        /**<Enable flag of Audio*//**CNcomment:是否Enable音频 */

    HI_UNF_HDMI_VIDEO_MODE_E enVidOutMode;       /**<HDMI output vedio mode VIDEO_MODE_YCBCR,VIDEO_MODE_YCBCR444，VIDEO_MODE_YCBCR422，VIDEO_MODE_RGB444 *//**<CNcomment:HDMI输出视频模式，VIDEO_MODE_YCBCR444，VIDEO_MODE_YCBCR422，VIDEO_MODE_RGB444 */
    HI_UNF_HDMI_DEEP_COLOR_E enDeepColorMode;    /**<Deep Color output mode,defualt: HI_UNF_HDMI_DEEP_COLOR_24BIT *//**<CNcomment:DeepColor输出模式, 默认为HI_UNF_HDMI_DEEP_COLOR_24BIT */
    HI_BOOL                 bxvYCCMode;          /**<the xvYCC output mode,default:HI_FALSE*//**<CNcomment:< xvYCC输出模式，默认为HI_FALSE */

    HI_BOOL                 bEnableAviInfoFrame; /**<Enable flag of AVI InfoFrame,suggestion:enable *//**<CNcomment:是否使能 AVI InfoFrame，建议使能 */
    HI_BOOL                 bEnableSpdInfoFrame; /**<Enable flag of SPD info frame,suggestion:disable*//**<CNcomment:是否使能 SPD InfoFrame， 建议关闭 */
    HI_BOOL                 bEnableMpegInfoFrame;/**<Enable flag of MPEG info frame,suggestion:disable*//**<CNcomment:是否使能 MPEG InfoFrame， 建议关闭 */
    HI_BOOL                 bEnableAudInfoFrame; /**<Enable flag of Audio InfoFrame,suggestion:enable*//**<CNcomment:是否使能 AUDIO InfoFrame，建议使能 */

    HI_U32                  bDebugFlag;          /**<the flag of hdmi dubug,suggestion:disable*//**<CNcomment:< 是否使能 打开hdmi内部debug信息， 建议关闭 */
    HI_BOOL                 bHDCPEnable;         /**<0:HDCP disable mode,1:eable HDCP mode*//**<CNcomment:< 0:HDCP不激活，1:HDCP模式打开 */
}HDMI_APP_ATTR_S;



/*hdmi struct */
typedef struct hiHDMI_ATTR_S
{
	HDMI_AUDIO_ATTR_S  stAudioAttr;
    HDMI_VIDEO_ATTR_S  stVideoAttr;
    HDMI_APP_ATTR_S    stAppAttr;
}HDMI_ATTR_S;
#endif

typedef struct hiHDMI_AUDIO_CAPABILITY_S
{
    HI_BOOL             bAudioFmtSupported[HI_UNF_EDID_AUDIO_FORMAT_CODE_BUTT]; /**<Audio capability,reference EIA-CEA-861-D,table 37,HI_TRUE:support this Audio type;HI_FALSE,nonsupport this Audio type*//**<CNcomment:音频能力集, 请参考EIA-CEA-861-D 表37;HI_TRUE表示支持这种显示格式，HI_FALSE表示不支持 */
    HI_U32              u32AudioSampleRateSupported[MAX_SAMPE_RATE_NUM]; /**<PCM smprate capability,0: illegal value,other is support PCM smprate *//**<CNcomment:PCM音频采样率能力集，0为非法值，其他为支持的音频采样率 */
    HI_U32              u32MaxPcmChannels;        /**<Audio max PCM Channels number*//**CNcomment:音频最大的PCM通道数 */
}HI_DRV_HDMI_AUDIO_CAPABILITY_S;



HI_S32  HI_DRV_HDMI_Init(HI_VOID);
HI_VOID  HI_DRV_HDMI_Deinit(HI_VOID);
HI_S32 HI_DRV_HDMI_Open(HI_UNF_HDMI_ID_E enHdmi);
HI_S32 HI_DRV_HDMI_Close(HI_UNF_HDMI_ID_E enHdmi);

HI_S32 HI_DRV_HDMI_GetPlayStatus(HI_UNF_HDMI_ID_E enHdmi, HI_U32 *pu32Status);
HI_S32 HI_DRV_HDMI_GetAOAttr(HI_UNF_HDMI_ID_E enHdmi, HDMI_AUDIO_ATTR_S *pstHDMIAOAttr);
HI_S32 HI_DRV_HDMI_SetAOAttr(HI_UNF_HDMI_ID_E enHdmi, HDMI_AUDIO_ATTR_S *pstHDMIAOAttr);
HI_S32 HI_DRV_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_E enHdmi, HI_UNF_EDID_BASE_INFO_S *pstSinkCap);
HI_S32 HI_DRV_HDMI_GetAudioCapability(HI_UNF_HDMI_ID_E enHdmi, HI_DRV_HDMI_AUDIO_CAPABILITY_S *pstAudCap);
HI_S32 HI_DRV_HDMI_SetAudioMute(HI_UNF_HDMI_ID_E enHdmi);
HI_S32 HI_DRV_HDMI_SetAudioUnMute(HI_UNF_HDMI_ID_E enHdmi);

HI_S32 HI_DRV_HDMI_Detach(HI_UNF_HDMI_ID_E enHdmi);
#if defined(HI_HDMI_SUPPORT_2_0)
HI_S32 HI_DRV_HDMI_PreFormat(HI_UNF_HDMI_ID_E enHdmi);
HI_S32 HI_DRV_HDMI_SetFormat(HI_UNF_HDMI_ID_E enHdmi, HDMI_VIDEO_ATTR_S *pstVideoAttr);
HI_S32 HI_DRV_HDMI_Attach(HI_UNF_HDMI_ID_E enHdmi, HDMI_VIDEO_ATTR_S *pstVideoAttr);
#else
HI_S32 HI_DRV_HDMI_PreFormat(HI_UNF_HDMI_ID_E enHdmi, HI_DRV_DISP_FMT_E enEncodingFormat);
HI_S32 HI_DRV_HDMI_SetFormat(HI_UNF_HDMI_ID_E enHdmi, HI_DRV_DISP_FMT_E enFmt, HI_DRV_DISP_STEREO_E enStereo);
HI_S32 HI_DRV_HDMI_Attach(HI_UNF_HDMI_ID_E enHdmi,HI_DRV_DISP_FMT_E enFmt, HI_DRV_DISP_STEREO_E enStereo);
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
/*--------------------------END-------------------------------*/
