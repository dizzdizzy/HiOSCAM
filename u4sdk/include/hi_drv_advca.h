/******************************************************************************

  Copyright (C), 2011-2014, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_drv_advca.h
  Version       : Initial Draft
  Author        : Hisilicon hisecurity team
  Created       : 
  Last Modified :
  Description   : 
  Function List :
  History       :
******************************************************************************/
#ifndef __HI_DRV_ADVCA_H__
#define __HI_DRV_ADVCA_H__

#include "drv_advca_ext.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

#ifdef SDK_SECURITY_ARCH_VERSION_V3
#define HI_FATAL_CA(fmt...)    HI_FATAL_PRINT(HI_ID_CA, fmt)
#define HI_ERR_CA(fmt...)      HI_ERR_PRINT(HI_ID_CA, fmt)
#define HI_WARN_CA(fmt...)     HI_WARN_PRINT(HI_ID_CA, fmt)
#define HI_INFO_CA(fmt...)     HI_INFO_PRINT(HI_ID_CA, fmt)
#else
#ifdef SDK_SECURITY_ARCH_VERSION_V2
#ifdef ADVCA_RELEASE
#define HI_FATAL_CA(fmt...)
#define HI_ERR_CA(fmt...)
#define HI_WARN_CA(fmt...)
#define HI_INFO_CA(fmt...)
#else
#define HI_FATAL_CA(fmt...) \
    HI_FATAL_PRINT(CA, fmt)

#define HI_ERR_CA(fmt...) \
    HI_ERR_PRINT(CA, fmt)

#define HI_WARN_CA(fmt...) \
    HI_WARN_PRINT(CA, fmt)

#define HI_INFO_CA(fmt...) \
    HI_INFO_PRINT(CA, fmt)
#endif
#else   /* else #ifdef SDK_SECURITY_ARCH_VERSION_V2 */
#ifdef HI_ADVCA_FUNCTION_RELEASE
#define HI_FATAL_CA(fmt...)
#define HI_ERR_CA(fmt...)
#define HI_WARN_CA(fmt...)
#define HI_INFO_CA(fmt...)
#else
#define HI_FATAL_CA(fmt...) \
    HI_TRACE(HI_LOG_LEVEL_FATAL, HI_DEBUG_ID_CA, fmt)

#define HI_ERR_CA(fmt...) \
    HI_TRACE(HI_LOG_LEVEL_ERROR, HI_DEBUG_ID_CA, fmt)

#define HI_WARN_CA(fmt...) \
    HI_TRACE(HI_LOG_LEVEL_WARNING, HI_DEBUG_ID_CA, fmt)

#define HI_INFO_CA(fmt...) \
    HI_TRACE(HI_LOG_LEVEL_INFO, HI_DEBUG_ID_CA, fmt)
#endif
#endif  /* end #ifdef SDK_SECURITY_ARCH_VERSION_V2 */
#endif  /* end #ifdef SDK_SECURITY_ARCH_VERSION_V3 */

HI_S32 HI_DRV_ADVCA_Resume(HI_VOID);
HI_S32 HI_DRV_ADVCA_Suspend(HI_VOID);
HI_S32 HI_DRV_ADVCA_Crypto(DRV_ADVCA_EXTFUNC_PARAM_S stParam);

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_DRV_ADVCA_H__ */

