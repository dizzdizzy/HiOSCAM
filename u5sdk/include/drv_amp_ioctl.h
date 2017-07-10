#ifndef __DRV_AMP_IOCTL_H__
#define __DRV_AMP_IOCTL_H__

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

#include "hi_drv_amp.h"
#include "hi_board.h"

/***********************/

typedef struct
{
    HI_U32      u32RegAddr;
    HI_U32      u32ByteSize;
    HI_U8       *pu8Value;
} AMP_REG_OPERATE_PARAM_S, *AMP_REG_OPERATE_PARAM_S_PTR;

typedef struct AmpInfo{
    HI_U8 u8DeviceType;
    HI_U8 u8DeviceAddr;
    HI_U8 u8GPIOOutputPolarity;
    HI_U8 u8ResetGPIONum;
    HI_U8 u8ResetPolarity;
    HI_U8 u8HWMuteGPIONum;
    HI_U8 u8HWMutePolarity;
    HI_U8 u8I2CNum;
}AMP_INFO_S;

/* AMP custom command code*/
#define CMD_AMP_SETMUTE _IOW (HI_ID_AMP, 0x00, HI_BOOL)
#define CMD_AMP_GETMUTE _IOWR (HI_ID_AMP, 0x01, HI_BOOL) 
#define CMD_AMP_SETSUBWOOFERVOL _IOW (HI_ID_AMP, 0x02, HI_U32) 
#define CMD_AMP_GETSUBWOOFERVOL _IOWR (HI_ID_AMP, 0x03, HI_U32) 
#define CMD_AMP_WRITEREG _IOW (HI_ID_AMP, 0x04, AMP_REG_OPERATE_PARAM_S) 
#define CMD_AMP_READREG _IOWR (HI_ID_AMP, 0x05, AMP_REG_OPERATE_PARAM_S) 
#define CMD_AMP_INIT _IOWR (HI_ID_AMP, 0x06, AMP_INFO_S)


#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __DRV_AMP_IOCTL_H__ */
