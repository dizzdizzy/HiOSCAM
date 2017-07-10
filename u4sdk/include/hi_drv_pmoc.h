#ifndef  __HI_DRV_PMOC_H__
#define  __HI_DRV_PMOC_H__

#include "hi_unf_pmoc.h"
#include "hi_debug.h"

/* add register macro */
#if defined (CHIP_TYPE_hi3716m) || defined (CHIP_TYPE_hi3716c) || defined (CHIP_TYPE_hi3716h)
#define C51_BASE             0x600b0000
#define C51_SIZE             0x8000
#define C51_DATA             0x7000
#define MCU_CODE_MAXSIZE     0x5000
#elif defined (CHIP_TYPE_hi3712)
#define C51_BASE             0x0
#define C51_SIZE             0x4000
#define C51_DATA             0x3e00
#define MCU_CODE_MAXSIZE     0x3000
#elif  defined(CHIP_TYPE_hi3716cv200)   \
    || defined(CHIP_TYPE_hi3716mv400)   \
    || defined(CHIP_TYPE_hi3718cv100)   \
    || defined(CHIP_TYPE_hi3719cv100)   \
    || defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3796cv100)   \
    || defined(CHIP_TYPE_hi3798cv100)   \
    || defined(CHIP_TYPE_hi3798cv200_a) \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)	\
    || defined(CHIP_TYPE_hi3716mv410)	\
    || defined(CHIP_TYPE_hi3716mv420)
#define C51_BASE             0xf8400000
#define C51_SIZE             0x8000  //32K
#define C51_DATA             0x7000  //28K
#define MCU_CODE_MAXSIZE     0x5000  //20K
#else
#error YOU MUST DEFINE  CHIP_TYPE!
#endif

#define DEFAULT_INIT_FREQ 1200000

#if    defined(CHIP_TYPE_hi3716cv200)   \
    || defined(CHIP_TYPE_hi3716mv400)   \
    || defined(CHIP_TYPE_hi3718cv100)   \
    || defined(CHIP_TYPE_hi3719cv100)
#define HI_VDD_MPU_OPP1_UV 1000
#define HI_VDD_MPU_OPP2_UV 1000
#define HI_VDD_MPU_OPP3_UV 1060
#define HI_VDD_MPU_OPP4_UV 1180
#define HI_VDD_MPU_OPP5_UV 1300
#define HI_VDD_MPU_OPP1_VMIN 1000
#define HI_VDD_MPU_OPP2_VMIN 1000
#define HI_VDD_MPU_OPP3_VMIN 1000
#define HI_VDD_MPU_OPP4_VMIN 1030
#define HI_VDD_MPU_OPP5_VMIN 1140
#define HI_VDD_MPU_OPP1_HPM 0x105
#define HI_VDD_MPU_OPP2_HPM 0x105
#define HI_VDD_MPU_OPP3_HPM 0x110
#define HI_VDD_MPU_OPP4_HPM 0x138
#define HI_VDD_MPU_OPP5_HPM 0x164
#elif defined(CHIP_TYPE_hi3719mv100)
#define HI_VDD_MPU_OPP1_UV 1000
#define HI_VDD_MPU_OPP2_UV 1000
#define HI_VDD_MPU_OPP3_UV 1000
#define HI_VDD_MPU_OPP4_UV 1110
#define HI_VDD_MPU_OPP5_UV 1220
#define HI_VDD_MPU_OPP6_UV 1350
#define HI_VDD_MPU_OPP1_VMIN 1000
#define HI_VDD_MPU_OPP2_VMIN 1000
#define HI_VDD_MPU_OPP3_VMIN 1000
#define HI_VDD_MPU_OPP4_VMIN 1030
#define HI_VDD_MPU_OPP5_VMIN 1070
#define HI_VDD_MPU_OPP1_HPM 0x105
#define HI_VDD_MPU_OPP2_HPM 0x105
#define HI_VDD_MPU_OPP3_HPM 0x110
#define HI_VDD_MPU_OPP4_HPM 0x115
#define HI_VDD_MPU_OPP5_HPM 0x150
#elif defined(CHIP_TYPE_hi3718mv100)
#define HI_VDD_MPU_OPP1_UV 1000
#define HI_VDD_MPU_OPP2_UV 1000
#define HI_VDD_MPU_OPP3_UV 1090
#define HI_VDD_MPU_OPP4_UV 1200
#define HI_VDD_MPU_OPP5_UV 1350
#define HI_VDD_MPU_OPP1_VMIN 1000
#define HI_VDD_MPU_OPP2_VMIN 1000
#define HI_VDD_MPU_OPP3_VMIN 1000
#define HI_VDD_MPU_OPP4_VMIN 1060
#define HI_VDD_MPU_OPP5_VMIN 1270
#define HI_VDD_MPU_OPP1_HPM 0x105
#define HI_VDD_MPU_OPP2_HPM 0x105
#define HI_VDD_MPU_OPP3_HPM 0x11a
#define HI_VDD_MPU_OPP4_HPM 0x157
#define HI_VDD_MPU_OPP5_HPM 0x195
#elif  defined(CHIP_TYPE_hi3798cv100)   \
    || defined(CHIP_TYPE_hi3796cv100)
#define HI_VDD_MPU_OPP1_UV 1000
#define HI_VDD_MPU_OPP2_UV 1000
#define HI_VDD_MPU_OPP3_UV 1100
#define HI_VDD_MPU_OPP4_UV 1320
#define HI_VDD_MPU_OPP5_UV 1380
#define HI_VDD_MPU_OPP6_UV 1495
#define HI_VDD_MPU_OPP1_VMIN 1000
#define HI_VDD_MPU_OPP2_VMIN 1000
#define HI_VDD_MPU_OPP3_VMIN 1000
#define HI_VDD_MPU_OPP4_VMIN 1150
#define HI_VDD_MPU_OPP5_VMIN 1200
#define HI_VDD_MPU_OPP6_VMIN 1495
#define HI_VDD_MPU_OPP1_HPM 0x100
#define HI_VDD_MPU_OPP2_HPM 0x100
#define HI_VDD_MPU_OPP3_HPM 0x110
#define HI_VDD_MPU_OPP4_HPM 0x160
#define HI_VDD_MPU_OPP5_HPM 0x190
#define HI_VDD_MPU_OPP6_HPM 0x250
#elif defined(CHIP_TYPE_hi3716mv410)    \
    || defined(CHIP_TYPE_hi3716mv420)
#define HI_VDD_MPU_OPP1_UV 1380
#define HI_VDD_MPU_OPP2_UV 1380
#define HI_VDD_MPU_OPP3_UV 1380
#define HI_VDD_MPU_OPP4_UV 1380
#define HI_VDD_MPU_OPP5_UV 1380
#define HI_VDD_MPU_OPP11_UV 1380
#define HI_VDD_MPU_OPP12_UV 1380
#define HI_VDD_MPU_OPP1_VMIN 1300
#define HI_VDD_MPU_OPP2_VMIN 1300
#define HI_VDD_MPU_OPP3_VMIN 1300
#define HI_VDD_MPU_OPP4_VMIN 1300
#define HI_VDD_MPU_OPP5_VMIN 1300
#define HI_VDD_MPU_OPP11_VMIN 1300
#define HI_VDD_MPU_OPP12_VMIN 1300
#define HI_VDD_MPU_OPP1_HPM 0x100
#define HI_VDD_MPU_OPP2_HPM 0x100
#define HI_VDD_MPU_OPP3_HPM 0xc8
#define HI_VDD_MPU_OPP4_HPM 0xf6
#define HI_VDD_MPU_OPP5_HPM 0x120
#define HI_VDD_MPU_OPP11_HPM 0x100
#define HI_VDD_MPU_OPP12_HPM 0x100
#elif  defined(CHIP_TYPE_hi3798cv200_a)
#define HI_VDD_MPU_OPP1_UV 800
#define HI_VDD_MPU_OPP2_UV 800
#define HI_VDD_MPU_OPP3_UV 830
#define HI_VDD_MPU_OPP4_UV 890
#define HI_VDD_MPU_OPP5_UV 970
#define HI_VDD_MPU_OPP6_UV 1110
#define HI_VDD_MPU_OPP7_UV 1150
#define HI_VDD_MPU_OPP1_VMIN 800
#define HI_VDD_MPU_OPP2_VMIN 800
#define HI_VDD_MPU_OPP3_VMIN 800
#define HI_VDD_MPU_OPP4_VMIN 800
#define HI_VDD_MPU_OPP5_VMIN 860
#define HI_VDD_MPU_OPP6_VMIN 970
#define HI_VDD_MPU_OPP7_VMIN 1100
#define HI_VDD_MPU_OPP1_HPM 0x100
#define HI_VDD_MPU_OPP2_HPM 0x100
#define HI_VDD_MPU_OPP3_HPM 0xc8
#define HI_VDD_MPU_OPP4_HPM 0xf6
#define HI_VDD_MPU_OPP5_HPM 0x120
#define HI_VDD_MPU_OPP6_HPM 0x160
#define HI_VDD_MPU_OPP7_HPM 0x172
#elif  defined(CHIP_TYPE_hi3798mv100)    \
    || defined(CHIP_TYPE_hi3796mv100)	
#define HI_VDD_MPU_OPP1_UV 980
#define HI_VDD_MPU_OPP2_UV 1150
#define HI_VDD_MPU_OPP3_UV 1300
#define HI_VDD_MPU_OPP4_UV 1300
#define HI_VDD_MPU_OPP5_UV 1420
#define HI_VDD_MPU_OPP6_UV 1495
#define HI_VDD_MPU_OPP1_VMIN 980
#define HI_VDD_MPU_OPP2_VMIN 1000
#define HI_VDD_MPU_OPP3_VMIN 1060
#define HI_VDD_MPU_OPP4_VMIN 1050
#define HI_VDD_MPU_OPP5_VMIN 1180
#define HI_VDD_MPU_OPP6_VMIN 1495
#define HI_VDD_MPU_OPP1_HPM 0x100
#define HI_VDD_MPU_OPP2_HPM 0xd0
#define HI_VDD_MPU_OPP3_HPM 0x10b
#define HI_VDD_MPU_OPP4_HPM 0x100
#define HI_VDD_MPU_OPP5_HPM 0x135
#define HI_VDD_MPU_OPP6_HPM 0x200
#else
#error YOU MUST DEFINE  CHIP_TYPE!
#endif

typedef struct hiCPU_VF_S
{
    unsigned int freq;          /* unit: kHz */
    unsigned int hpmrecord;     /* hpm record */
    unsigned int vmin;          /* the minimum voltage of AVS */
    unsigned int vsetting;      /* the setting voltage of DVFS */
} CPU_VF_S;

#define MAX_FREQ_NUM 7
extern CPU_VF_S cpu_freq_hpm_table[MAX_FREQ_NUM];

#define HI_FATAL_PM(fmt...) HI_FATAL_PRINT(HI_ID_PM, fmt)

#define HI_ERR_PM(fmt...)   HI_ERR_PRINT(HI_ID_PM, fmt)

#define HI_WARN_PM(fmt...)  HI_WARN_PRINT(HI_ID_PM, fmt)

#define HI_INFO_PM(fmt...)  HI_INFO_PRINT(HI_ID_PM, fmt)

HI_S32 HI_DRV_PMOC_Init(HI_VOID);
HI_S32 HI_DRV_PMOC_DeInit(HI_VOID);
HI_S32 HI_DRV_PMOC_SwitchSystemMode(HI_UNF_PMOC_MODE_E enSystemMode, HI_UNF_PMOC_ACTUAL_WKUP_E * penWakeUpStatus);
HI_S32 HI_DRV_PMOC_SetWakeUpAttr(HI_UNF_PMOC_WKUP_S_PTR pstAttr);
HI_S32 HI_DRV_PMOC_SetStandbyDispMode(HI_UNF_PMOC_STANDBY_MODE_S_PTR pstStandbyMode);
HI_S32 HI_DRV_PMOC_SetScene(HI_UNF_PMOC_SCENE_E eScene);
HI_S32 HI_DRV_PMOC_SetDevType(HI_UNF_PMOC_DEV_TYPE_S_PTR pdevType);

#endif  /*  __HI_DRV_PMOC_H__ */

