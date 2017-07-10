/******************************************************************************

Copyright (C), 2009-2014, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_drv_amp.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2013-01-22
Last Modified :
Description   : audio dsp decoder
Function List :
History       :
* main\1    2013-01-22   zgjie     init.
******************************************************************************/
#ifndef __HI_DRV_AMP_H__
#define __HI_DRV_AMP_H__

#include "hi_type.h"
#include "hi_module.h"


#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

/******************************* macro definition *****************************/
#define HI_FATAL_AMP(fmt...) \
                    HI_FATAL_PRINT(HI_ID_AMP, fmt)
                
#define HI_ERR_AMP(fmt...) \
                    HI_ERR_PRINT(HI_ID_AMP, fmt)
                
#define HI_WARN_AMP(fmt...) \
                    HI_WARN_PRINT(HI_ID_AMP, fmt)
                
#define HI_INFO_AMP(fmt...) \
                    HI_INFO_PRINT(HI_ID_AMP, fmt)

#define CHECK_AMP_INIT(AMPDevFd) \
    do {\
        if (AMPDevFd < 0)\
        {\
            HI_ERR_AMP("AMP is not init.\n"); \
            return -1; \
        } \
    } while (0)

#define CHECK_AMP_NULL_PTR(p)                                \
    do {                                                    \
            if(HI_NULL == p)                                \
            {                                               \
                HI_ERR_AMP("NULL pointer \n");               \
                return HI_FAILURE;                          \
            }                                               \
         } while(0)        

/******************************* Struct definition *****************************/

typedef struct
{
    HI_U32      u32todo;
} AMP_XXXX_S;

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_DRV_AMP_H__ */
