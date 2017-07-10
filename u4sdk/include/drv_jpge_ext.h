#ifndef __DRV_JPGE_EXT_H__
#define __DRV_JPGE_EXT_H__

#include "hi_drv_dev.h"
//#include "jpge_ext.h"
#include "hi_jpge_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef HI_VOID (*FN_JPGE_Init)(HI_VOID);
typedef HI_VOID  (*FN_JPGE_DeInit)(HI_VOID);
typedef HI_S32  (*FN_JPGE_EncodeFrame)(HI_U32 EncHandle, Venc2Jpge_EncIn_S * pEncIn, HI_BOOL *pBufferFull);
typedef HI_S32  (*FN_JPGE_CreateChn) (HI_U32 *pEncHandle, Jpge_EncCfg_S *pEncCfg );
typedef HI_S32  (*FN_JPGE_DestroyChn)( HI_U32   EncHandle );
typedef HI_S32  (*FN_JPGE_Suspend)(PM_BASEDEV_S *, pm_message_t);
typedef HI_S32  (*FN_JPGE_Resume)(PM_BASEDEV_S *);

typedef struct
{
	FN_JPGE_Init			      pfnJpgeInit;
	FN_JPGE_DeInit		      pfnJpgeDeInit;
	FN_JPGE_CreateChn		pfnJpgeCreateChn;
	FN_JPGE_DestroyChn		pfnJpgeDestroyChn;
	FN_JPGE_EncodeFrame		pfnJpgeEncodeFrame;
	FN_JPGE_Suspend			pfnJpgeSuspend;
	FN_JPGE_Resume			pfnJpgeResume;
} JPGE_EXPORT_FUNC_S;


int JPGE_DRV_ModInit(void);
void JPGE_DRV_ModExit(void);

/*************************************************************************************/


HI_VOID Jpge_Init( HI_VOID );
HI_VOID Jpge_DInit( HI_VOID );

/* Open & Close Hardware */
HI_S32 Jpge_Open ( HI_VOID );
HI_S32 Jpge_Close( HI_VOID );

/* Create & Destroy One Encoder Channel */
HI_S32 Jpge_Create ( HI_U32 *pEncHandle, Jpge_EncCfg_S *pEncCfg );
HI_S32 Jpge_Destroy( HI_U32   EncHandle );

/* Encode One Frame */
HI_S32 Jpge_Encode ( HI_U32 EncHandle, Jpge_EncIn_S *pEncIn, Jpge_EncOut_S *pEncOut );

/* for VENC*/
HI_S32 Jpge_Create_toVenc(HI_U32 * pEncHandle, Jpge_EncCfg_S * pEncCfg);
HI_S32 Jpge_Encode_toVenc( HI_U32 EncHandle, Venc2Jpge_EncIn_S *pEncIn,HI_BOOL *pBufferFull);
/*************************************************************************************/






#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__DRV_JPGE_EXT_H__*/
