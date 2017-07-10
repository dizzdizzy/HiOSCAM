/******************************************************************************

  Copyright (C), 2011-2014, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     :hi_drv_cipher.h
  Version       : Initial Draft
  Author        : Hisilicon hisecurity team
  Created       : 
  Last Modified :
  Description   : 
  Function List :
  History       :
******************************************************************************/

#ifndef __HI_DRV_CIPHER_H__
#define __HI_DRV_CIPHER_H__

#include "hi_type.h"
#include "hi_unf_cipher.h"
#include "hi_unf_hdcp.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#define CIPHER_SOFT_CHAN_NUM      CIPHER_CHAN_NUM
#define CIPHER_INVALID_CHN        (0xffffffff)

#define  HDCP_KEY_RAM_SIZE                      320
#define  HDCP_KEY_PRIME_SIZE                    320
#define  HDCP_KEY_TOOL_FILE_SIZE                384
#define  HDCP_KEY_CHIP_FILE_SIZE               (HDCP_KEY_RAM_SIZE + 12)

#define HI_FATAL_CIPHER(fmt...)             HI_FATAL_PRINT(HI_ID_CIPHER, fmt)
#define HI_ERR_CIPHER(fmt...)               HI_ERR_PRINT(HI_ID_CIPHER, fmt)
#define HI_WARN_CIPHER(fmt...)              HI_WARN_PRINT(HI_ID_CIPHER, fmt)
#define HI_INFO_CIPHER(fmt...)              HI_INFO_PRINT(HI_ID_CIPHER, fmt)
#define HI_DEBUG_CIPHER(fmt...)             HI_DBG_PRINT(HI_ID_CIPHER, fmt)
typedef struct
{
    HI_BOOL bIsUseOTPRootKey;
    HI_UNF_HDCP_HDCPKEY_S stHdcpKey;
    HI_U8 u8FlashEncryptedHdcpKey[HDCP_KEY_CHIP_FILE_SIZE];
}CIPHER_HDCP_KEY_TRANSFER_S;

typedef struct hiCIPHER_DATA_INFO_S
{
    HI_U32  u32src;
    HI_U32  u32dest;
    HI_U32  u32length;
    HI_BOOL bDecrypt;
    HI_U32  u32DataPkg[4];
}HI_DRV_CIPHER_DATA_INFO_S;

typedef struct hiCIPHER_TASK_S
{
    HI_DRV_CIPHER_DATA_INFO_S stData2Process;
    HI_U32 u32CallBackArg;
}HI_DRV_CIPHER_TASK_S;

typedef enum hiCIPHER_HDCP_MODE_E
{
    CIPHER_HDCP_MODE_NO_HDCP_KEY                = 0x0,
    CIPHER_HDCP_MODE_HDCP_KEY,
    CIPHER_HDCP_MODE_BUTT,
}HI_DRV_CIPHER_HDCP_KEY_MODE_E;

typedef enum hiCIPHER_HDCP_KEY_RAM_MODE_E
{
    CIPHER_HDCP_KEY_RAM_MODE_READ         		= 0x0,
    CIPHER_HDCP_KEY_RAM_MODE_WRITE,
    CIPHER_HDCP_KEY_RAM_MODE_BUTT,
}HI_DRV_CIPHER_HDCP_KEY_RAM_MODE_E;

typedef enum hiCIPHER_HDCP_KEY_RX_SELECT_E
{
    CIPHER_HDCP_KEY_SELECT_TX         		= 0x0,
    CIPHER_HDCP_KEY_SELECT_RX,
    CIPHER_HDCP_KEY_SELECT_BUTT,
}HI_DRV_CIPHER_HDCP_KEY_RX_SELECT_E;

typedef enum hiCIPHER_HDCP_KEY_RX_READ_E
{
    CIPHER_HDCP_KEY_WR_RX_RAM         		= 0x0,
    CIPHER_HDCP_KEY_RD_RX_RAM,
    CIPHER_HDCP_KEY_RX_READ_BUTT,
}HI_DRV_CIPHER_HDCP_KEY_RX_READ_E;


typedef enum hiCIPHER_HDCP_KEY_TYPE_E
{
    CIPHER_HDCP_KEY_TYPE_OTP_ROOT_KEY           = 0x0,
    CIPHER_HDCP_KEY_TYPE_HISI_DEFINED,
    CIPHER_HDCP_KEY_TYPE_HOST_ROOT_KEY,
    CIPHER_HDCP_KEY_TYPE_BUTT,
}HI_DRV_CIPHER_HDCP_ROOT_KEY_TYPE_E;

typedef enum
{
    HI_DRV_HDCPKEY_TX0 = 0x0,
    HI_DRV_HDCPKEY_TX1,

    HI_DRV_HDCPKEY_RX0 = 0x10,
    HI_DRV_HDCPKEY_RX1,

    HI_DRV_HDCPKEY_BUTT
}HI_DRV_HDCPKEY_TYPE_E;

typedef enum
{
    HI_DRV_HDCP_VERIOSN_1x = 0x0,
    HI_DRV_HDCP_VERIOSN_2x,

    HI_DRV_HDCP_VERSON_BUTT
}HI_DRV_HDCP_VERSION_E;

typedef struct
{
    HI_U8 u8Key[HDCP_KEY_CHIP_FILE_SIZE];   // 332bytes for HDCP1.X, unknown for other.
    HI_U32 u32KeyLen;
    HI_DRV_HDCP_VERSION_E enHDCPVersion;
    HI_DRV_HDCPKEY_TYPE_E enHDCPKeyType;
}HI_DRV_CIPHER_FLASH_ENCRYPT_HDCPKEY_S;

typedef struct  hiCIPHER_HANDLE_S
{
    HI_HANDLE hCIHandle;
    HI_UNF_CIPHER_ATTS_S stCipherAtts;
}CIPHER_HANDLE_S;

typedef struct  hiCIPHER_DATA_S
{
    HI_HANDLE CIHandle;
    HI_U32 ScrPhyAddr;
    HI_U32 DestPhyAddr;
    HI_U32 u32DataLength;
}CIPHER_DATA_S;

typedef struct hiCIPHER_Config_CTRL
{
    HI_HANDLE CIHandle;
    HI_UNF_CIPHER_CTRL_S CIpstCtrl;
}CIPHER_Config_CTRL;

typedef enum
{
    HI_CIPHER_HMAC_KEY_FROM_CA  = 0,
    HI_CIPHER_HMAC_KEY_FROM_CPU = 1,
}CIPHER_HMAC_KEY_FROM_E;

typedef struct hiCipher_HASH_MMZ_BUFFER_S
{
    HI_U32 u32StartVirAddr;
    HI_U32 u32StartPhyAddr;
    HI_U32 u32Size;
}HASH_MMZ_BUFFER_S;

typedef struct hiCIPHER_HASH_DATA_S
{
    HI_HANDLE hHandle;
    HI_UNF_CIPHER_HASH_TYPE_E enShaType;
    HI_U32 u32TotalDataLen;
    HI_U8  *pu8InputData;
    HI_U32 u32InputDataLen;
    HI_U8  u8Padding[64+8];
    HI_U32 u32PaddingLen;
    HI_U8  *pu8HMACKey;
    HI_U32  u32HMACKeyLen;
    CIPHER_HMAC_KEY_FROM_E enHMACKeyFrom;
    HI_U8  *pu8Output;
	HASH_MMZ_BUFFER_S stMMZBuffer;
	HI_U32 u32Offset;
}CIPHER_HASH_DATA_S;

typedef struct  hiCIPHER_CBCMAC_DATA_S
{
    HI_U8 *pu8RefCbcMac;
    HI_U32 u32AppLen;
}CIPHER_CBCMAC_DATA_S;

typedef struct
{
    HI_U32 u32TimeOutUs;
	HI_U32 u32RNG;
}CIPHER_RNG_S;

HI_S32 HI_DRV_CIPHER_CreateHandle(CIPHER_HANDLE_S *pstCIHandle);
HI_S32 HI_DRV_CIPHER_ConfigChn(HI_U32 softChnId, HI_UNF_CIPHER_CTRL_S *pConfig);
HI_S32 HI_DRV_CIPHER_GetHandleConfig(CIPHER_Config_CTRL *pstCipherConfig);
HI_S32 HI_DRV_CIPHER_DestroyHandle(HI_HANDLE hCipherchn);
HI_S32 HI_DRV_CIPHER_Encrypt(CIPHER_DATA_S *pstCIData);
HI_S32 HI_DRV_CIPHER_Decrypt(CIPHER_DATA_S *pstCIData);
HI_S32 HI_DRV_CIPHER_EncryptMulti(CIPHER_DATA_S *pstCIData);
HI_S32 HI_DRV_CIPHER_DecryptMulti(CIPHER_DATA_S *pstCIData);
HI_S32 HI_DRV_CIPHER_GetRandomNumber(CIPHER_RNG_S *pstRNG);
HI_S32 HI_DRV_CIPHER_SoftReset(HI_VOID);
HI_S32 HI_DRV_CIPHER_LoadHdcpKey(HI_DRV_CIPHER_FLASH_ENCRYPT_HDCPKEY_S *pstFlashHdcpKey);
HI_S32 HI_DRV_CIPHER_CalcHashInit(CIPHER_HASH_DATA_S *pCipherHashData);
HI_S32 HI_DRV_CIPHER_CalcHashUpdate(CIPHER_HASH_DATA_S *pCipherHashData);
HI_S32 HI_DRV_CIPHER_CalcHashFinal(CIPHER_HASH_DATA_S *pCipherHashData);
HI_S32 HI_DRV_CIPHER_EncryptHDCPKey(CIPHER_HDCP_KEY_TRANSFER_S *pstHdcpKeyTransfer);
HI_S32 HI_DRV_CIPHER_CbcMacAuth(CIPHER_CBCMAC_DATA_S *pstParam);

HI_VOID HI_DRV_CIPHER_Suspend(HI_VOID);
HI_S32 HI_DRV_CIPHER_Resume(HI_VOID);
HI_VOID HI_DRV_CIPHER_SetHdmiReadClk(HI_DRV_HDCPKEY_TYPE_E enHDCPKeyType);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* End of #ifndef __HI_DRV_CIPHER_H__*/

