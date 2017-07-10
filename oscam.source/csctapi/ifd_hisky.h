#ifndef _ifd_hisky_h_
#define _ifd_hisky_h_

#if defined(SDKV600)
#define	CHIP_TYPE_hi3796mv100
#include "drv_sci_ioctl.h"
#elif defined(SDKV500)
#define	CHIP_TYPE_hi3716cv200
#include "drv_sci_ioctl.h"
#else
//
//
//	#define	__HISCI_IOCTL_AVAILABLE__
//
//
// $ANDROID_BUILD_TOP/device/hisilicon/godbox/driver/sdk/msp_base
// $ANDROID_BUILD_TOP/device/hisilicon/godbox/driver/sdk/msp_base/ecs/include
// $ANDROID_BUILD_TOP/device/hisilicon/godbox/driver/sdk/msp_base/common/include
typedef unsigned char       HI_U8;
typedef unsigned char       HI_UCHAR;
typedef unsigned short      HI_U16;
typedef unsigned int        HI_U32;

typedef char                HI_S8;
typedef short               HI_S16;
typedef int                 HI_S32;

typedef char                HI_CHAR;
typedef char*               HI_PCHAR;

typedef float               HI_FLOAT;
typedef double              HI_DOUBLE;
typedef void                HI_VOID;
typedef unsigned int	    	 HI_HANDLE;

typedef unsigned long       HI_SIZE_T;
typedef unsigned long       HI_LENGTH_T;

typedef int                 STATUS;

#ifndef WIN32
typedef unsigned long long  HI_U64;
typedef long long           HI_S64;
typedef unsigned long long  HI_PTS_TIME;
#else
typedef __int64             HI_U64;
typedef __int64             HI_S64;
typedef __int64             HI_PTS_TIME;
#endif

#if 0
#define _IOC_NRBITS 		8
#define _IOC_TYPEBITS 		8
#define _IOC_SIZEBITS 		14
#define _IOC_DIRBITS 		2

#define _IOC_NRMASK 		((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK 		((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK 		((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK 		((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT 		0
#define _IOC_TYPESHIFT 		(_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT 		(_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT 		(_IOC_SIZESHIFT+_IOC_SIZEBITS)

#define _IOC_NONE 			0U
#define _IOC_WRITE 			1U
#define _IOC_READ 			2U

#define _IOC(dir,type,nr,size)   (((dir) << _IOC_DIRSHIFT) | ((type) << _IOC_TYPESHIFT) | ((nr) << _IOC_NRSHIFT) |   ((size) << _IOC_SIZESHIFT))
#define _IOC_TYPECHECK(t)   	((sizeof(t) == sizeof(t[1]) && sizeof(t) < (1 << _IOC_SIZEBITS)) ?   sizeof(t) : __invalid_size_argument_for_IOC)

#define _IO(type,nr) 			_IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size) 		_IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size) 		_IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size) 	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOR_BAD(type,nr,size) 	_IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW_BAD(type,nr,size) 	_IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _IOWR_BAD(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))
#endif

typedef enum
{
    HI_FALSE    = 0,
    HI_TRUE     = 1
} HI_BOOL;

#define HI_SUCCESS 			0
#define HI_FAILURE      	(-1)


/* constants */
#define HI_ID_SCI				0x45

/**Output configuration of the smart card interface clock (SCICLK) pin*/
/**CNcomment:SCICLK????????????*/
typedef enum  hiUNF_SCI_CLK_MODE_E
{
    HI_UNF_SCI_CLK_MODE_CMOS = 0,   /**<Complementary metal-oxide semiconductor (CMOS) output*/   /**<CNcomment:CMOS????*/
    HI_UNF_SCI_CLK_MODE_OD,         /**<Open drain (OD) output*/                                  /**<CNcomment:OD????*/
    HI_UNF_SCI_CLK_MODE_BUTT
}HI_UNF_SCI_CLK_MODE_E;

/**SCI port*/
/**CNcomment:SCI ?˿? */
typedef enum hiUNF_SCI_PORT_E
{
    HI_UNF_SCI_PORT0,      /**< SCI port 0*/  /**<CNcomment:SCI?˿?0*/
    HI_UNF_SCI_PORT1,      /**< SCI port 1*/  /**<CNcomment:SCI?˿?1*/
    HI_UNF_SCI_PORT_BUTT
}HI_UNF_SCI_PORT_E;

/**Status of the SCI card*/
/**CNcomment:???ܿ?״̬ */
typedef enum hiUNF_SCI_STATUS_E
{
    HI_UNF_SCI_STATUS_UNINIT     = 0,   /**<The SCI card is not initialized.*/               /**<CNcomment: SCIδ??ʼ?? */
    HI_UNF_SCI_STATUS_FIRSTINIT,        /**<The SCI card is being initialized.*/             /**<CNcomment:SCI??ʼ????????*/
    HI_UNF_SCI_STATUS_NOCARD,           /**<There is no SCI card.*/                          /**<CNcomment:?޿? */
    HI_UNF_SCI_STATUS_INACTIVECARD,     /**<The SCI card is not activated (unavailable).*/   /**<CNcomment:??δ???ɼ??????Ч?? */
 //   HI_UNF_SCI_STATUS_CARDFAULT,		  /**<The SCI card is faulty.*/                        /**<CNcomment:??????*/
    HI_UNF_SCI_STATUS_WAITATR,          /**<The SCI card is waiting for the ATR data.*/      /**<CNcomment:?ȴ?ATR*/
    HI_UNF_SCI_STATUS_READATR,          /**<The SCI card is receiving the ATR data.*/        /**<CNcomment:???ڽ???ATR*/
    HI_UNF_SCI_STATUS_READY,            /**<The SCI card is available (activated).*/         /**<CNcomment:??????ʹ?ã?????? */
    HI_UNF_SCI_STATUS_RX,               /**<The SCI card is busy receiving data.*/           /**<CNcomment:??æ???????????У? */
    HI_UNF_SCI_STATUS_TX                /**<The SCI card is busy transmitting data.*/        /**<CNcomment:??æ???????????У? */
} HI_UNF_SCI_STATUS_E;

/**SCI protocol*/
/**CNcomment:SCI Э?? */
typedef enum hiUNF_SCI_PROTOCOL_E
{
    HI_UNF_SCI_PROTOCOL_T0,    /**<7816 T0 protocol*/   /**<CNcomment:7816 T0 Э?? */
    HI_UNF_SCI_PROTOCOL_T1,    /**<7816 T1 protocol*/   /**<CNcomment:7816 T1 Э?? */
    HI_UNF_SCI_PROTOCOL_T14 ,  /**<7816 T14 protocol*/  /**<CNcomment:7816 T14 Э?? */
    HI_UNF_SCI_PROTOCOL_BUTT
}HI_UNF_SCI_PROTOCOL_E;

/**SCI active level*/
/**CNcomment:SCI??Ч??ƽ*/
typedef enum hiUNF_SCI_LEVEL_E
{
    HI_UNF_SCI_LEVEL_LOW,    /**<Active low*/   /**<CNcomment:?͵?ƽ??Ч */
    HI_UNF_SCI_LEVEL_HIGH,   /**<Active high*/  /**<CNcomment:?ߵ?ƽ??Ч */
    HI_UNF_SCI_LEVEL_BUTT
}HI_UNF_SCI_LEVEL_E ;

/**SCI system parameters*/
/**CNcomment:SCI ϵͳ???? */
typedef struct hiUNF_SCI_PARAMS_S
{
	HI_UNF_SCI_PORT_E enSciPort;		      /**<SCI port ID*/                                          	/**<CNcomment:SCI ?˿ں? */
	HI_UNF_SCI_PROTOCOL_E enProtocolType; /**<Used protocol type*/                                    /**<CNcomment:ʹ?õ?Э?????? */
	HI_U32 ActalClkRate;	                /**<Actual clock rate conversion factor F*/	                /**<CNcomment:ʵ?ʵ?F ֵʱ??ת?????? */
	HI_U32 ActalBitRate;                	/**<Actual bit rate conversion factor D*/	                  /**<CNcomment:ʵ?ʵ?D ֵ??????ת?????? */
	HI_U32 Fi;			                      /**<Clock factor returned by the answer to reset (ATR)*/  	/**<CNcomment:ATR ???ص?ʱ?????? */
	HI_U32 Di;			                      /**<Bit rate factor returned by the ATR*/                 	/**<CNcomment:ATR ???صı????????? */
	HI_U32 GuardDelay;	                  /**<Extra guard time N*/	                                  /**<CNcomment:N ֵ???????ӵı???ʱ??*/
	HI_U32 CharTimeouts;	                /**<Character timeout of T0 or T1*/                       	/**<CNcomment:T0 ??T1???ַ???ʱʱ??*/
	HI_U32 BlockTimeouts;	                /**<Block timeout of T1 */                                  /**<CNcomment:T1?Ŀ鳬ʱʱ??*/
	HI_U32 TxRetries;			                /**<Number of transmission retries*/                      	/**<CNcomment:???????Դ???*/
}HI_UNF_SCI_PARAMS_S,*HI_UNF_SCI_PARAMS_S_PTR;



typedef struct hiSCI_OPEN_S
{
    HI_UNF_SCI_PORT_E      enSciPort;
    HI_UNF_SCI_PROTOCOL_E  enSciProtocol;
    HI_U32                 Frequency;
}SCI_OPEN_S;

typedef struct hiSCI_RESET_S
{
    HI_UNF_SCI_PORT_E    enSciPort;
    HI_BOOL              bWarmReset;
}SCI_RESET_S;

typedef struct hiSCI_ATR_S
{
    HI_UNF_SCI_PORT_E  enSciPort;
	HI_U8              *pAtrBuf;
	HI_U32             BufSize;
	HI_U8              DataLen;
}SCI_ATR_S;

typedef struct hiSCI_STATUS_S
{
    HI_UNF_SCI_PORT_E    enSciPort;
	HI_UNF_SCI_STATUS_E  enSciStatus;
}SCI_STATUS_S;

typedef struct hiSCI_DATA_S
{
    HI_UNF_SCI_PORT_E    enSciPort;
	HI_U8                *pDataBuf;
    HI_U32               BufSize;
    HI_U32               DataLen;
    HI_U32               TimeoutMs;
}SCI_DATA_S;

typedef struct hiSCI_LEVEL_S
{
    HI_UNF_SCI_PORT_E    enSciPort;
	HI_UNF_SCI_LEVEL_E   enSciLevel;
}SCI_LEVEL_S;

typedef struct hiSCI_CLK_S
{
    HI_UNF_SCI_PORT_E      enSciPort;
    HI_UNF_SCI_CLK_MODE_E  enClkMode;
}SCI_CLK_S;

typedef struct hiSCI_DEV_STATE_S
{
    HI_BOOL         bSci0;
    HI_BOOL         bSci1;
}SCI_DEV_STATE_S;

typedef struct hiSCI_EXT_BAUD_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_U32 ClkRate;
	HI_U32 BitRate;
}SCI_EXT_BAUD_S;

typedef struct hiSCI_ADD_GUARD_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_U32	AddCharGuard;
}SCI_ADD_GUARD_S;

typedef struct hiSCI_PPS_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_U8  	Send[6];
	HI_U8  	Receive[6];
	HI_U32  SendLen;
	HI_U32  ReceiveLen;
	HI_U32	RecTimeouts;
}SCI_PPS_S;

typedef struct hiSCI_CHARTIMEOUT_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_UNF_SCI_PROTOCOL_E  enSciProtocol;
	HI_U32	CharTimeouts;
}SCI_CHARTIMEOUT_S;

typedef struct hiSCI_BLOCKTIMEOUT_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_U32	BlockTimeouts;
}SCI_BLOCKTIMEOUT_S;

typedef struct hiSCI_TXRETRY_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_U32	TxRetryTimes;
}SCI_TXRETRY_S;

// sky(n)
typedef struct hiSCI_PROTOCOL_S
{
	HI_UNF_SCI_PORT_E	   enSciPort;
	HI_UNF_SCI_PROTOCOL_E  enSciProtocol;
}SCI_PROTOCOL_S;


#define CMD_SCI_OPEN              	_IOW(HI_ID_SCI,  0x1, SCI_OPEN_S)
#define CMD_SCI_CLOSE             	_IOW(HI_ID_SCI,  0x2, HI_UNF_SCI_PORT_E)
#define CMD_SCI_RESET             	_IOW(HI_ID_SCI,  0x3, SCI_RESET_S)
#define CMD_SCI_DEACTIVE         	_IOW(HI_ID_SCI,  0x4, HI_UNF_SCI_PORT_E)
#define CMD_SCI_GET_ATR           	_IOWR(HI_ID_SCI, 0x5, SCI_ATR_S)
#define CMD_SCI_GET_STATUS        	_IOWR(HI_ID_SCI, 0x6, SCI_STATUS_S)
#define CMD_SCI_CONF_VCC         	_IOW(HI_ID_SCI,  0x7, SCI_LEVEL_S)
#define CMD_SCI_CONF_DETECT       	_IOW(HI_ID_SCI,  0x8, SCI_LEVEL_S)
#define CMD_SCI_CONF_CLK_MODE     	_IOW(HI_ID_SCI,  0x9, SCI_CLK_S)
#define CMD_SCI_SEND_DATA         	_IOWR(HI_ID_SCI, 0xa, SCI_DATA_S)
#define CMD_SCI_RECEIVE_DATA      	_IOWR(HI_ID_SCI, 0xb, SCI_DATA_S)
#define CMD_SCI_SWITCH            	_IOW(HI_ID_SCI,  0xc, SCI_OPEN_S)
#define CMD_SCI_SET_BAUD          	_IOW(HI_ID_SCI,  0xd, SCI_EXT_BAUD_S)
#define CMD_SCI_SET_CHGUARD       	_IOW(HI_ID_SCI,  0xe, SCI_ADD_GUARD_S)
#define CMD_SCI_SEND_PPS_DATA     	_IOW(HI_ID_SCI,  0xF, SCI_PPS_S)
#define CMD_SCI_GET_PPS_DATA	  		_IOWR(HI_ID_SCI, 0x10, SCI_PPS_S)
#define CMD_SCI_GET_PARAM				_IOWR(HI_ID_SCI, 0x11, HI_UNF_SCI_PARAMS_S)
#define CMD_SCI_SET_CHARTIMEOUT     _IOW(HI_ID_SCI,  0x12, SCI_CHARTIMEOUT_S)
#define CMD_SCI_SET_BLOCKTIMEOUT    _IOW(HI_ID_SCI,  0x13, SCI_BLOCKTIMEOUT_S)
#define CMD_SCI_SET_TXRETRY       	_IOW(HI_ID_SCI,  0x14, SCI_TXRETRY_S)
#define CMD_SCI_SET_PROTOCOL        _IOW(HI_ID_SCI,  0x15, SCI_PROTOCOL_S)

#define HI_ERR_SCI_OPEN_ERR     			(HI_S32)(0x80450001)
#define HI_ERR_SCI_CLOSE_ERR    			(HI_S32)(0x80450002)
#define HI_ERR_SCI_NOT_INIT       		(HI_S32)(0x80450003)
#define HI_ERR_SCI_INVALID_PARA     	(HI_S32)(0x80450004)
#define HI_ERR_SCI_NULL_PTR           	(HI_S32)(0x80450005)
#define HI_ERR_SCI_INVALID_OPT     		(HI_S32)(0x80450006)
#define HI_ERR_SCI_SEND_ERR      		(HI_S32)(0x80450007)
#define HI_ERR_SCI_RECEIVE_ERR        	(HI_S32)(0x80450008)
#define HI_ERR_SCI_NO_ATR           	(HI_S32)(0x80450009)
#define HI_ERR_SCI_PPS_PTYPE_ERR     	(HI_S32)(0x8045000A)
#define HI_ERR_SCI_PPS_FACTOR_ERR    	(HI_S32)(0x8045000B)
#define HI_ERR_SCI_PPS_NOTSUPPORT_ERR	(HI_S32)(0x8045000C)

#endif // defined(SDKV500)
#endif /* End of #ifndef _ifd_hisky_h_*/


