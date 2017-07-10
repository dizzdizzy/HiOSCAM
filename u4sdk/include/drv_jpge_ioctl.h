/*Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
 File Name     : hi_tde_ioctl.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2006/7/10
Last Modified :
Description   : 
Function List :
History       :
 ******************************************************************************/

#ifndef __HI_JPGE_IOCTL_H__
#define __HI_JPGE_IOCTL_H__

#include <linux/ioctl.h>
#include "hi_jpge_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/* Use 'j' as magic number */
#define JPGE_IOC_MAGIC 'j'

#define JPGE_CREATE_CMD _IOWR(JPGE_IOC_MAGIC, 100, Jpge_EncCfgInfo_S)
#define JPGE_ENCODE_CMD _IOWR(JPGE_IOC_MAGIC, 101, Jpge_EncInfo_S)
#define JPGE_DESTROY_CMD _IOW(JPGE_IOC_MAGIC, 102, JPGE_HANDLE)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __HI_JPGE_IOCTL_H__ */

