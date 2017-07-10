#ifndef __DRV_AFLT_EXT_H__
#define __DRV_AFLT_EXT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

HI_S32  AFLT_DRV_ModInit(HI_VOID);
HI_VOID AFLT_DRV_ModExit(HI_VOID);

#if defined(HI_MCE_SUPPORT)
HI_S32 HI_DRV_AFLT_Init(HI_VOID); //TODO   
HI_S32 HI_DRV_AFLT_DeInit(HI_VOID); //TODO    
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DRV_VDEC_EXT_H__ */

