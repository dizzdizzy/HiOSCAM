#ifndef __DRV_PMOC_IOCTL_H__
#define __DRV_PMOC_IOCTL_H__

#include "hi_unf_pmoc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//#define MCU_IRKEY_MAXNUM 6

typedef struct hiC51_PmocDisp_S
{
    unsigned int mode;
    unsigned int value;
}C51_PMOC_DISP_S;

typedef struct hiC51_PmocVal_S
{
    unsigned int  irnum;
    unsigned int  irlVal[PMOC_WKUP_IRKEY_MAXNUM];
    unsigned int  irhVal[PMOC_WKUP_IRKEY_MAXNUM];
    unsigned int  keyVal;
    unsigned int  timeVal;
    unsigned char usbWakeupMask;
    unsigned char ethWakeupFlag;
    unsigned char resumeResetFlag;
    unsigned int  gpioPort;
}C51_PMOC_VAL_S;

typedef enum
{
    C51_PMOC_STAT_NORMAL = 0,   /**<NORMAL*/
    C51_PMOC_STAT_SLOW,         /**<SLOW*/
    C51_PMOC_STAT_DOZE,         /**<DOZE*/
    C51_PMOC_STAT_SLEEP         /**<SLEEP*/
}C51_PMOC_STAT_E;

typedef enum
{
    C51_PMOC_WAKEUP_IR = 0,
    C51_PMOC_WAKEUP_KEY,
    C51_PMOC_WAKEUP_TIME,
    C51_PMOC_WAKEUP_ETH,
    C51_PMOC_WAKEUP_USB,
    C51_PMOC_WAKEUP_GPIO,
    C51_PMOC_WAKEUP_BUTT
}C51_PMOC_WAKEUP_E;


typedef struct hiC51_PmocDev_S
{
    unsigned int irtype;  // 0: necsample, 4: raw

#ifdef HI_KEYLED_SUPPORT
    unsigned int kltype;  // 0: std,  1: 6961
#endif

}C51_PMOC_DEV_S;

/* Ioctl definitions */
#define HI_PMOC_STANDBY_CMD             _IOW(HI_ID_PM, 0x01, C51_PMOC_DISP_S)
#define HI_PMOC_SET_WAKEUP_CMD          _IOW(HI_ID_PM, 0x02, C51_PMOC_VAL_S)
#define HI_PMOC_WAKEUP_READY_CMD        _IO (HI_ID_PM, 0x03  )
#define HI_PMOC_WAKEUP_FINISH_CMD       _IOR(HI_ID_PM, 0x04, C51_PMOC_WAKEUP_E)
#define HI_PMOC_WAKEUP_SETDEV_CMD       _IOW(HI_ID_PM, 0x05, C51_PMOC_DEV_S)
#define HI_PMOC_LOAD_MCUCODE_CMD        _IOW(HI_ID_PM, 0x06, HI_U32)
#define HI_PMOC_SET_SCENE_CMD           _IOW(HI_ID_PM, 0x07, HI_UNF_PMOC_SCENE_E)
#define HI_PMOC_SET_PWROFFGPIO_CMD      _IOW(HI_ID_PM, 0x08, HI_U32)
#define HI_PMOC_GET_PERIOD_CMD          _IOW(HI_ID_PM, 0x09, HI_U32)
#define HI_PMOC_ENTER_SMARTSTANDBY_CMD  _IOW(HI_ID_PM, 0x0A, HI_U32)
#define HI_PMOC_QUIT_SMARTSTANDBY_CMD   _IO (HI_ID_PM, 0x0B)
#define HI_PMOC_GET_WAKEUP_CMD          _IOR(HI_ID_PM, 0x0C, C51_PMOC_VAL_S)
#define HI_PMOC_BOOST_CPU               _IO (HI_ID_PM, 0x0D)
// sky(stby.futures)
#define HI_PMOC_ENTER_SLEEPSTANDBY_CMD 	_IOW(HI_ID_PM, 0x0D, HI_U32)
#define HI_PMOC_QUIT_SLEEPSTANDBY_CMD  	_IO (HI_ID_PM, 0x0E)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __DRV_PMOC_IOCTL_H__ */



