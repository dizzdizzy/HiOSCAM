#ifndef __DRV_ADE_EXT_H__
#define __DRV_ADE_EXT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* __cplusplus */

#include "hi_type.h"

typedef union hiADE_VERSIONTYPE_U
{
    struct
    {
        HI_U8 u8VersionMajor;      /**< Major version */
        HI_U8 u8VersionMinor;      /**< Minor version */
        HI_U8 u8Revision;          /**< Revision version */
        HI_U8 u8Step;              /**< Step version */
    } s;
    HI_U32 u32Version;
} ADE_VERSIONTYPE_U;


typedef HI_S32 (*FN_ADE_todofunc)(HI_VOID);


typedef struct tagADE_EXPORT_FUNC_S
{
    FN_ADE_todofunc pfnADE_TodoA;
} ADE_EXPORT_FUNC_S;

HI_S32	ADE_DRV_ModInit(HI_VOID);
HI_VOID ADE_DRV_ModExit(HI_VOID);

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* __cplusplus */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DRV_VDEC_EXT_H__ */

