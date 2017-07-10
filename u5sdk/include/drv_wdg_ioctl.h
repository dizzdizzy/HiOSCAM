/*
 *	Generic watchdog defines. Derived from..
 *
 * Berkshire PC Watchdog Defines
 * by Ken Hollis <khollis@bitgate.com>
 *
 */

#ifndef __DRV_WDG_IOCTL_H__
#define __DRV_WDG_IOCTL_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#include "hi_unf_wdg.h"
#include "hi_drv_wdg.h"

typedef struct _tagWDG_TIMEOUT_S
{
	HI_U32	u32WdgIndex;
	HI_S32	s32Timeout;
}WDG_TIMEOUT_S;

typedef struct _tagWDG_OPTION_S
{
	HI_U32	u32WdgIndex;
	HI_S32	s32Option;
}WDG_OPTION_S;

#define WATCHDOG_IOCTL_BASE 'W'

#define WDIOC_SET_OPTIONS _IOR(WATCHDOG_IOCTL_BASE, 4, WDG_OPTION_S)
#define WDIOC_KEEP_ALIVE _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_SET_TIMEOUT _IOWR(WATCHDOG_IOCTL_BASE, 6, WDG_TIMEOUT_S)
#define WDIOC_GET_TIMEOUT _IOR(WATCHDOG_IOCTL_BASE, 7, WDG_TIMEOUT_S)
#define WDIOF_UNKNOWN -1  /* Unknown flag error */
#define WDIOS_UNKNOWN -1  /* Unknown status error */


#define WDIOS_DISABLECARD 0x0001  /* Turn off the watchdog timer */
#define WDIOS_ENABLECARD 0x0002  /* Turn on the watchdog timer */
#define WDIOS_RESET_BOARD 0x0008  /* reset the board */



#endif  /* ifndef __DRV_WDG_IOCTL_H__ */
