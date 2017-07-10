/******************************************************************************

  Copyright (C), 2012-2014, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_drv_ademod.h
  Version       : Initial Draft
  Author        : Hisilicon audio software group
  Created       : 2012/09/14
  Description   : 
  History       :
  1.Date        : 2012/09/14
    Author      : m00196336
    Modification: Created file
******************************************************************************/
#ifndef __HI_DRV_SIF_H__
#define __HI_DRV_SIF_H__

#include <linux/ioctl.h>

#include "hi_unf_sif.h"
#include "hi_module.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define HI_FATAL_SIF(fmt...) HI_FATAL_PRINT(HI_ID_SIF, fmt)
#define HI_ERR_SIF(fmt...)   HI_ERR_PRINT(HI_ID_SIF, fmt)
#define HI_WARN_SIF(fmt...)  HI_WARN_PRINT(HI_ID_SIF, fmt)
#define HI_INFO_SIF(fmt...)  HI_INFO_PRINT(HI_ID_SIF, fmt)
#define HI_DEBUG_SIF(fmt...) HI_DBG_PRINT(HI_ID_SIF, fmt)

typedef enum SIF_OVER_DEVIATION_E 
{
    SIF_OVER_DEVIATION_50K  = 0x0,           /**<Over Devaiton 50K*/  /**<CNcomment:1y|：????? 50K*/
    SIF_OVER_DEVIATION_100K,                 /**<Over Devaiton 100K*/ /**<CNcomment:1y|：????? 100K*/
    SIF_OVER_DEVIATION_384K,                 /**<Over Devaiton 384K*/ /**<CNcomment:1y|：????? 384K*/
    SIF_OVER_DEVIATION_540K,                 /**<Over Devaiton 500K*/ /**<CNcomment:1y|：????? 500K*/
    SIF_OVER_DEVIATION_200K,                 /**<Over Devaiton 200K*/ /**<CNcomment:1y|：????? 200K*/
    SIF_OVER_DEVIATION_BUTT
} SIF_OVER_DEVIATION_E;

typedef struct hiSIF_OPEN_PARAM_S
{
    HI_UNF_SIF_ID_E         enSifId;
    HI_UNF_SIF_OPENPARMS_S  stSifOpenParam;
} SIF_OPEN_PARAM_S, *SIF_OPEN_PARAM_S_PTR;

typedef struct hiSIF_ATTR_S
{
    HI_UNF_SIF_ID_E    enSifId;
    HI_UNF_SIF_ATTR_S  stSifAttr;
} SIF_ATTR_S, *SIF_ATTR_S_PTR;

typedef struct hiSIF_STANDARD_TYPE_S
{
    HI_UNF_SIF_ID_E             enSifId;
    HI_UNF_SIF_STANDARD_TYPE_E  enStand;
} SIF_STANDARD_TYPE_S, *SIF_STANDARD_TYPE_S_PTR;

typedef struct hiSIF_CARRIER_ATTR_S
{
    HI_UNF_SIF_ID_E             enSifId;
    HI_UNF_SIF_CARRIER_ATTR_S   enCarrierAttr;
} SIF_CARRIER_ATTR_S, *SIF_CARRIER_ATTR_S_PTR;

typedef struct hiSIF_SYSCTL_S
{
    HI_UNF_SIF_ID_E      enSifId;
    HI_UNF_SIF_SYSCTL_E  enSysCtl;
} SIF_SYSCTL_S, *SIF_SYSCTL_S_PTR;

typedef struct hiSIF_ASD_COMPLETE_S
{
    HI_UNF_SIF_ID_E      enSifId;
    HI_BOOL              bAsdComplete;
} SIF_ASD_COMPLETE_S,SIF_ASD_COMPLETE_S_PTR;

typedef struct hiSIF_OUTMODE_S
{
    HI_UNF_SIF_ID_E       enSifId;
    HI_UNF_SIF_OUTMODE_E  enOutMode;
} SIF_OUTMODE_S, *SIF_OUTMODE_S_PTR;

typedef struct hiSIF_OVER_DEVIATION_S
{
    HI_UNF_SIF_ID_E              enSifId;
    SIF_OVER_DEVIATION_E         enOverMode;
} SIF_OVER_DEVIATION_S, *SIF_OVER_DEVIATION_S_PTR;

typedef struct hiSIF_CARRISHIFT_S
{
    HI_UNF_SIF_ID_E      enSifId;
    HI_U32               u32CarriShift;
} SIF_CARRISHIFT_S, *SIF_CARRISHIFT_S_PTR;

typedef struct hiSIF_AAOS_MODE_S
{
    HI_UNF_SIF_ID_E        enSifId;
    HI_UNF_SIF_AAOS_MODE_E enAAOSMode;
} SIF_AAOS_MODE_S, *SIF_AAOS_MODE_S_PTR;

#define CMD_SIF_OPEN            _IOW(HI_ID_SIF, 0x00, SIF_OPEN_PARAM_S)
#define CMD_SIF_CLOSE           _IOW(HI_ID_SIF,  0x01, HI_UNF_SIF_ID_E)
#define CMD_SIF_SET_ATTR        _IOW(HI_ID_SIF,  0x02, SIF_ATTR_S)
#define CMD_SIF_GET_ATTR        _IOWR(HI_ID_SIF,  0x03, SIF_ATTR_S)
#define CMD_SIF_SET_START       _IOW(HI_ID_SIF,  0x04, HI_UNF_SIF_ID_E)
#define CMD_SIF_SET_STOP        _IOW(HI_ID_SIF,  0x05, HI_UNF_SIF_ID_E)
#define CMD_SIF_SET_STANDDARD   _IOW(HI_ID_SIF,  0x06, SIF_STANDARD_TYPE_S)
#define CMD_SIF_START_ASD       _IOW(HI_ID_SIF,  0x07, SIF_SYSCTL_S)
#define CMD_SIF_GET_ASD_CMPL    _IOWR(HI_ID_SIF,  0x08, SIF_ASD_COMPLETE_S)
#define CMD_SIF_GET_STANDARD    _IOWR(HI_ID_SIF,  0x09, SIF_STANDARD_TYPE_S)
#define CMD_SIF_GET_ASD_RESULT  _IOWR(HI_ID_SIF,  0x0a, SIF_STANDARD_TYPE_S)
#define CMD_SIF_SET_OUTMODE     _IOW(HI_ID_SIF,  0x0b, SIF_OUTMODE_S)
#define CMD_SIF_GET_OUTMODE     _IOWR(HI_ID_SIF,  0x0c, SIF_AAOS_MODE_S)
#define CMD_SIF_SET_OVER_DEV    _IOW(HI_ID_SIF,  0x0d, SIF_OVER_DEVIATION_S)
#define CMD_SIF_GET_OVER_DEV    _IOW(HI_ID_SIF,  0x0e, SIF_OVER_DEVIATION_S)
#define CMD_SIF_SET_CARRI_SHIFT _IOW(HI_ID_SIF,  0x0f, SIF_CARRISHIFT_S)
#define CMD_SIF_TRY_STANDARD    _IOWR(HI_ID_SIF,  0x10, SIF_CARRIER_ATTR_S)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif/* __HI_DRV_SIF_H__ */

