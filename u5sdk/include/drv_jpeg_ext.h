/******************************************************************************
*
* Copyright (C) 2014 Hisilicon Technologies Co., Ltd.  All rights reserved. 
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon), 
* and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
******************************************************************************
File Name	    : drv_jpeg_ext.h
Version		    : Initial Draft
Author		    : 
Created		    : 2013/07/01
Description	    : 
Function List 	: 

			  		  
History       	:
Date				Author        		Modification
2013/07/01		    y00181162  		    Created file      	
******************************************************************************/
#ifndef __DRV_JPEG_EXT_H__
#define __DRV_JPEG_EXT_H__

/*********************************add include here******************************/

#if  defined(CHIP_TYPE_hi3712)      \
  || defined(CHIP_TYPE_hi3716m)     \
  || defined(CHIP_TYPE_hi3716mv310) \
  || defined(CHIP_TYPE_hi3110ev500) \
  || defined(CHIP_TYPE_hi3716mv320) \
  || defined(CHIP_TYPE_hi3716mv330)
	#include "drv_dev_ext.h"
#else
	#include "hi_drv_dev.h"
#endif
/*****************************************************************************/


/*****************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/***************************** Macro Definition ******************************/


/*************************** Structure Definition ****************************/
typedef enum hiDRV_JPEG_EXT_INTTYPE_E
{

	DRV_JPEG_EXT_INTTYPE_NONE       = 0,	   /** none halt happen	    **/
	DRV_JPEG_EXT_INTTYPE_CONTINUE,	       /** continue stream halt **/
	DRV_JPEG_EXT_INTTYPE_FINISH, 	           /** finish halt		    **/
	DRV_JPEG_EXT_INTTYPE_ERROR,		       /** error halt 		    **/
	DRV_JPEG_EXT_INTTYPE_BUTT
}DRV_JPEG_EXT_INTTYPE_E;

/** get halt state struct **/
typedef struct hiDRV_JPEG_EXT_GETINTTYPE_S
{

	DRV_JPEG_EXT_INTTYPE_E IntType;	  /** halt type **/
	unsigned int           TimeOut; 	  /** overtime	**/
	
}DRV_JPEG_EXT_GETINTTYPE_S;
/***************************  The enum of Jpeg image format  ******************/

/********************** Global Variable declaration **************************/


/******************************* API declaration *****************************/

typedef HI_S32  (*FN_JPEG_GetIntStatus)(DRV_JPEG_EXT_GETINTTYPE_S *pstIntType);
typedef HI_S32  (*FN_JPEG_GetDev)(HI_VOID);
typedef HI_S32  (*FN_JPEG_ReleaseDev)(HI_VOID);
typedef HI_VOID (*FN_JPEG_Reset)(HI_VOID);
typedef HI_S32  (*FN_JPEG_Suspend)(PM_BASEDEV_S *, pm_message_t);
typedef HI_S32  (*FN_JPEG_Resume)(PM_BASEDEV_S *);

typedef struct
{
	FN_JPEG_GetIntStatus    pfnJpegGetIntStatus;
	FN_JPEG_GetDev          pfnJpegGetDev;
	FN_JPEG_ReleaseDev      pfnJpegReleaseDev;
	FN_JPEG_Reset           pfnJpegReset;
	FN_JPEG_Suspend			pfnJpegSuspend;
	FN_JPEG_Resume			pfnJpegResume;
}JPEG_EXPORT_FUNC_S;


HI_VOID JPEG_DRV_ModExit(HI_VOID);

HI_S32 JPEG_DRV_ModInit(HI_VOID);

HI_S32 JPEG_DRV_K_ModInit(HI_VOID);

#ifdef __cplusplus
#if __cplusplus
}      
#endif
#endif /* __cplusplus */

#endif /*__DRV_JPEG_EXT_H__ */
