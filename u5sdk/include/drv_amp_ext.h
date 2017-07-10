#ifndef __DRV_AMP_EXT_H__
#define __DRV_AMP_EXT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */
#include "hi_type.h"
typedef HI_S32 (*FN_AMP_Init)(HI_VOID);

typedef HI_S32 (*FN_AMP_DeInit)(HI_BOOL bSuspend);

typedef HI_S32 (*FN_AMP_SetMute)(HI_BOOL bMute);

typedef HI_VOID (*FN_AMP_ShowInfo)(HI_VOID);


typedef struct tagAMP_EXPORT_FUNC_S
{
    FN_AMP_Init pfnAMP_Init;
    FN_AMP_DeInit pfnAMP_DeInit;
    FN_AMP_SetMute pfnAMP_SetMute;
    FN_AMP_ShowInfo pfnAMP_ShowInfo;
} AMP_EXPORT_FUNC_S;

HI_S32  AMP_DRV_ModInit(HI_VOID);
HI_VOID AMP_DRV_ModExit(HI_VOID);
HI_S32  HI_DRV_AMP_Init(HI_U32 uSampelRate, HI_BOOL bResume);
HI_VOID HI_DRV_AMP_DeInit(HI_BOOL bSuspend);
HI_VOID HI_DRV_AMP_SetMute(HI_BOOL bMute);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DRV_AMP_EXT_H__ */

