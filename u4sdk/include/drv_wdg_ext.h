/*
 *	Generic watchdog defines. Derived from..
 *
 * Berkshire PC Watchdog Defines
 * by Ken Hollis <khollis@bitgate.com>
 *
 */

#ifndef __DRV_WDG_EXT_H__
#define __DRV_WDG_EXT_H__

#include "hi_type.h"
#include "hi_drv_dev.h"

HI_S32 WDG_DRV_ModInit(HI_VOID);
HI_VOID WDG_DRV_ModExit(HI_VOID);

typedef HI_S32 (*FN_WDG_Suspend)(PM_BASEDEV_S* , pm_message_t );
typedef HI_S32 (*FN_WDG_Resume)(PM_BASEDEV_S* );

typedef struct
{
    FN_WDG_Suspend				   pfnWdgSuspend;
    FN_WDG_Resume				   pfnWdgResume;
} WDG_EXT_FUNC_S;

#endif  /* ifndef _DRV_WDG_EXT_H */
