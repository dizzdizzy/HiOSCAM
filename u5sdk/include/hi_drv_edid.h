#ifndef __HI_DRV_EDID_H__
#define __HI_DRV_EDID_H__

#include "hi_type.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/* Sink capability of HDR and Dolby */
typedef struct hiHDMI_DOLBY_SINK_CAPS_S
{
    HI_BOOL     bYccQrangeSelectable;   /* Sink要求YCC量化范围是否满/限制 范围可选。HI_TRUE表示sink要求范围可选，HI_FALSE表示sink要求默认范围。 */
    HI_BOOL     bRgbQrangeSelectable;   /* Sink要求RGB量化范围是否满/限制 范围可选。HI_TRUE表示sink要求范围可选，HI_FALSE表示sink要求默认范围。 */
    HI_BOOL     bYUV420_12Bit;       
    HI_BOOL     b2160P60;            
    HI_U16      u16Red_X;            
    HI_U16      u16Red_Y;            
    HI_U16      u16Green_X;          
    HI_U16      u16Green_Y;          
    HI_U16      u16Blue_X;           
    HI_U16      u16Blue_Y;           
    HI_U16      u16White_X;          
    HI_U16      u16White_Y;          
    HI_U16      u16MinLuminance;     
    HI_U16      u16MaxLuminance;     
}HI_HDMI_DOLBY_SINK_CAPS_S;

typedef struct hiHDMI_HDR_EOTF_CAPS_S
{
    HI_BOOL bEotfSdr;
    HI_BOOL bEotfHdr;
    HI_BOOL bEotfSmpteSt2084;
    HI_BOOL bEotfFuture;
}HI_HDMI_HDR_EOTF_CAPS_S;

typedef struct hiHDMI_HDR_METADATA_CAPS_S
{
    HI_BOOL     bDescriptorType1;
}HI_HDMI_HDR_METADATA_CAPS_S;

typedef struct hiHDMI_HDR_SINK_CAPS_S
{
    HI_HDMI_HDR_EOTF_CAPS_S         stEotf;
    HI_HDMI_HDR_METADATA_CAPS_S     stMetadata;
    HI_U8                           u8MaxLuminance_CV;
    HI_U8                           u8AverageLumin_CV;
    HI_U8                           u8MinLuminance_CV;
}HI_HDMI_HDR_SINK_CAPS_S;

typedef struct 
{
    HI_BOOL    bxvYCC601;    
    HI_BOOL    bxvYCC709;
    HI_BOOL    bsYCC601;   
    HI_BOOL    bAdobleYCC601; 
    HI_BOOL    bAdobleRGB;         
    HI_BOOL    bBT2020cYCC;
	HI_BOOL    bBT2020YCC; 
	HI_BOOL    bBT2020RGB;
}HI_HDMI_COLORIMETRY_CAPS_S;    
typedef struct hiHDMI_VIDEO_CAPABILITY_S
{
    HI_BOOL                         bDolbySupport;          
    HI_BOOL                         bHdrSupport;            
    HI_HDMI_DOLBY_SINK_CAPS_S       stDolbyCaps;
    HI_HDMI_HDR_SINK_CAPS_S         stHdrCaps;
    HI_HDMI_COLORIMETRY_CAPS_S      stColorimetry;
}HI_DRV_HDMI_VIDEO_CAPABILITY_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

