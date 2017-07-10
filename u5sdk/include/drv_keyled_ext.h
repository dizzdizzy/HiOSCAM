#ifndef __DRV_KEYLED_EXT_H__
#define __DRV_KEYLED_EXT_H__

#include "hi_type.h"

HI_S32 KEYLED_DRV_ModInit(HI_VOID);
HI_VOID KEYLED_DRV_ModExit(HI_VOID);

// sky(A)
#define PM_SMARTSTANDBY_FLAG   0x5FFFFFFF	//TODO
typedef HI_S32 (*FN_KEYLED_Suspend)(PM_BASEDEV_S* , pm_message_t );
typedef HI_S32 (*FN_KEYLED_Resume)(PM_BASEDEV_S* );
typedef struct
{
    FN_KEYLED_Suspend	pfnKeyledSuspend;
    FN_KEYLED_Resume	pfnKeyledResume;
} KEYLED_EXT_FUNC_S;

#endif /* __DRV_KEYLED_EXT_H__ */

