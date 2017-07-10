/*
//	sky(n)
	ifd_hisky.c
	This module provides IFD handling functions for SCI internal reader.
*/

#include "../globals.h"

#ifdef CARDREADER_INTERNAL_HISKY
#include "atr.h"
#include "ifd_hisky.h"
//
//
//
#if 1
	#define	MYSCI_TRACE		myprintf
#else
	#define	MYSCI_TRACE(...)
#endif
//
//
// 	future test...
#define __HIS_PPSNEGOTIATION__
//
//
//
#define HI_DEVICE_NAME	"hi_sci"
#define MAX_TX_TIMEOUT			5000
#define MAX_RX_TIMEOUT			12000
#define SUCCESS					0
#define OK							SUCCESS
#define ERROR						1
#define FAILURE 					2
#define _IS_CARD_INSERTED_(s)	(((s) <= HI_UNF_SCI_STATUS_NOCARD) ? 0 : 1)

#if 0
static pthread_mutex_t			CUR_Mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static int							SMC_Inserted  	= 0;
static HI_UNF_SCI_PROTOCOL_E	SMC_Protocol;
static int							SMC_Activated 	= FALSE;
static struct s_reader *		SMC_Readerp  	= NULL;

extern char  *strtoupper(char *s);
extern void   cs_sleep  (uint32_t msec);
extern void   cs_sleepms(uint32_t msec);
extern int    cs_save_cardinformation(struct s_reader *rdr);
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
uint16_t
HISCIAPI_ChkCardstatus(struct s_reader *reader)
{
	uint16_t	scstatus = 0;

	if (!reader) reader = SMC_Readerp;
	if ( reader)
	{
//		mycs_log("CARDSTATUS:(%p) {%d, %d} {%d}", reader,
//				reader->restarting, reader->card_status,
//				SMC_Inserted);
		if (reader->restarting)
		{
			scstatus = SMCSTATUS_RESTART;
		}
		else
		{
			switch (reader->card_status) {
				case NO_CARD: 			scstatus = SMCSTATUS_NOCARD; 	break;
				case CARD_NEED_INIT:	scstatus = SMCSTATUS_INIT; 	break;
				case CARD_FAILURE:  	scstatus = SMCSTATUS_FAILURE; break;
				case CARD_INSERTED:	scstatus = SMCSTATUS_OK;		break;
				case UNKNOWN:
				default:					scstatus = SMCSTATUS_RESTART;	break;
			}
		}
	}
	else
	{
//		mycs_log("Cardstatus:{%d}", SMC_Inserted);
		scstatus = (SMC_Inserted) ? SMCSTATUS_DEACTIVE : SMCSTATUS_NOCARD;
	}
	return (scstatus);
}

char *
HISCIAPI_ChkCardsystem(struct s_reader *reader)
{
	static char cards[1025] = {'\0'};

	strcpy(cards, "");
	if (!reader) reader = SMC_Readerp;
	if (!reader) return (cards);
	if ( reader->card_status == NO_CARD) {
		strcpy(cards, "No smartcard");
	}
	else if (reader->card_status == CARD_NEED_INIT) {
		strcpy(cards, "---");
	}
	else if (reader->csystem && reader->csystem->desc) {
		strcpy(cards, reader->csystem->desc);
		strtoupper(cards);
	}
	else {
		strcpy(cards, "Unknown card");
	}
	return (cards);
}

bool
HISCIAPI_SaveCardstatus(struct s_reader *reader)
{
	FILE *pf = NULL;
	char szFile[256];

	if (!reader) reader = SMC_Readerp;
	strcpy(szFile,"/var/smartcard");
	if ((pf = fopen(szFile, "w"))==NULL) return 0;
	fprintf(pf,"status=%d\n", HISCIAPI_ChkCardstatus(reader));
	fclose(pf);
	return 1;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
static void
HISCI_Msleep(int ms)
{
//	cs_sleepms(ms);
	cs_sleep(ms);
}

static int
HISCI_IsAvailable(struct s_reader *reader)
{
	HI_UNF_SCI_PORT_E enSciPort;

	if (!reader) return 0;
	if ( reader->handle < 0) return 0;
	enSciPort = reader->scinum;
	if (enSciPort >= HI_UNF_SCI_PORT_BUTT)
	{
		mycs_trace(D_ADB, "hisci:error scinum{%d}", enSciPort);
		return 0;
	}
	return 1;
}

static int
HISCI_ChkInserted(struct s_reader *reader)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_STATUS_S   SciStatus;
	HI_S32	Ret;
	int		bInserted;

	if (!HISCI_IsAvailable(reader)) return 0;
	enSciPort = reader->scinum;
	SciStatus.enSciPort = enSciPort;
	Ret = ioctl(reader->handle, CMD_SCI_GET_STATUS, &SciStatus);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.i{%08X}", Ret);
		bInserted = 0;
	}
	else
	{
	//	MYSCI_TRACE("mycard:getstatus{%d}\n", SciStatus.enSciStatus);
		bInserted = _IS_CARD_INSERTED_(SciStatus.enSciStatus);
	}
	SMC_Inserted = bInserted;
	return (bInserted);
}


int32_t
HISCI_Open(struct s_reader *reader, HI_UNF_SCI_PROTOCOL_E enSciProtocol, HI_U32 uFs)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_OPEN_S	SciOpen;
	SCI_LEVEL_S	SciLevel;
	HI_S32	Ret;

//	MYSCI_TRACE("mycard:open{%d,%d}\n", enSciProtocol, uFs);
	if (!HISCI_IsAvailable(reader)) return (ERROR);
	enSciPort = reader->scinum;
	SciOpen.enSciPort		= enSciPort;
	SciOpen.enSciProtocol= enSciProtocol;
	SciOpen.Frequency		= uFs;
	Ret = ioctl(reader->handle, CMD_SCI_OPEN, &SciOpen);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}
	//
	//
	//
#if defined(SDKV500) || defined(SDKV600)
	SCI_IO_OUTPUTTYPE_S SciClk;

	SciClk.enSciPort = enSciPort;
	SciClk.enIO = SCI_IO_CLK;
	SciClk.enOutputType = HI_UNF_SCI_MODE_CMOS;
	Ret = ioctl(reader->handle, CMD_SCI_CONF_MODE, &SciClk);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}

	SCI_IO_OUTPUTTYPE_S SciVcc;
	SciVcc.enSciPort = enSciPort;
	SciVcc.enIO = SCI_IO_VCC_EN;
	SciVcc.enOutputType = HI_UNF_SCI_MODE_CMOS;
//	SciVcc.enOutputType = HI_UNF_SCI_MODE_OD;
	Ret = ioctl(reader->handle, CMD_SCI_CONF_MODE, &SciVcc);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}

	SCI_IO_OUTPUTTYPE_S SciReset;
	SciReset.enSciPort = enSciPort;
	SciReset.enIO = SCI_IO_RESET;
	SciReset.enOutputType = HI_UNF_SCI_MODE_CMOS;
//	SciReset.enOutputType = HI_UNF_SCI_MODE_OD;
	Ret = ioctl(reader->handle, CMD_SCI_CONF_MODE, &SciReset);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}
#else
	SCI_CLK_S SciClk;
	SciClk.enSciPort = enSciPort;
//	SciClk.enClkMode = HI_UNF_SCI_CLK_MODE_OD;
	SciClk.enClkMode = HI_UNF_SCI_CLK_MODE_CMOS;
	Ret = ioctl(reader->handle, CMD_SCI_CONF_CLK_MODE, &SciClk);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}
#endif

	SciLevel.enSciPort	= enSciPort;
	SciLevel.enSciLevel 	= HI_UNF_SCI_LEVEL_LOW;
	Ret = ioctl(reader->handle, CMD_SCI_CONF_VCC, &SciLevel);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}

	SciLevel.enSciPort	= enSciPort;
	SciLevel.enSciLevel 	= HI_UNF_SCI_LEVEL_LOW;
//	SciLevel.enSciLevel 	= HI_UNF_SCI_LEVEL_HIGH;
	Ret = ioctl(reader->handle, CMD_SCI_CONF_DETECT, &SciLevel);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.o{%08X}!", Ret);
		return (ERROR);
	}
	SMC_Protocol = enSciProtocol;
	return (SUCCESS);
}


int32_t
HISCI_Close(struct s_reader *reader)
{
	HI_UNF_SCI_PORT_E enSciPort;
	HI_S32 Ret;;

//	MYSCI_TRACE("mycard:close\n");
	if (!HISCI_IsAvailable(reader)) return (ERROR);
	enSciPort = reader->scinum;
	SMC_Activated = FALSE;
	Ret = ioctl(reader->handle, CMD_SCI_CLOSE, &enSciPort);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.c{%08X}!", Ret);
	}
	return (Ret == HI_SUCCESS ? SUCCESS : ERROR);
}


int32_t
HISCI_Deactivate(struct s_reader *reader)
{
	HI_UNF_SCI_PORT_E enSciPort;
	HI_S32 Ret;

	MYSCI_TRACE("mycard:deactivate\n");
	if (!HISCI_IsAvailable(reader)) return (ERROR);
	enSciPort = reader->scinum;
	Ret = ioctl(reader->handle, CMD_SCI_DEACTIVE, &enSciPort);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.d{%08X}!", Ret);
	}
	return (Ret == HI_SUCCESS ? SUCCESS : ERROR);
}


#if 0
int32_t
HISCI_SetProtocol(struct s_reader *reader, HI_UNF_SCI_PROTOCOL_E enSciProtocol)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_PROTOCOL_S SciProtocol;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:setprotocol{%d}\n", enSciProtocol);
	enSciPort = reader->scinum;
	SciProtocol.enSciPort = enSciPort;
	SciProtocol.enSciProtocol = enSciProtocol;
	Ret = ioctl(reader->handle, CMD_SCI_SET_PROTOCOL, &SciProtocol);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.p{%08X.%d}!", Ret, SciProtocol.enSciProtocol);
	}
	return (Ret == HI_SUCCESS ? SUCCESS : ERROR);
}
#endif


static int32_t
HISCI_ChkAtrCompleted(struct s_reader *reader)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_STATUS_S	SciStatus;
	HI_S32	uResetTime;
	HI_S32	Ret;

	if (!HISCI_IsAvailable(reader)) return (ERROR);
	enSciPort  = reader->scinum;
	uResetTime = 0;
	HISCI_Msleep(500);
	while (1)
	{
		/*get SCI card status */
		SciStatus.enSciPort = enSciPort;
		Ret = ioctl(reader->handle, CMD_SCI_GET_STATUS, &SciStatus);
		if (Ret != HI_SUCCESS)
		{
			mycs_trace(D_ADB, "hisci:io error.a{%08X}!", Ret);
			return (ERROR);
		}
		if (!_IS_CARD_INSERTED_(SciStatus.enSciStatus))
		{
			mycs_trace(D_ADB, "hisci:no card{%d}!", SciStatus.enSciStatus);
			SMC_Inserted = 0;
			return (ERROR);
		}
		if (SciStatus.enSciStatus >= HI_UNF_SCI_STATUS_READY)
		{
			/*reset Success*/
			mycs_trace(D_ADB, "hisci:Atr success");
			break;
		}
//		MYSCI_TRACE("hisci:Atr waiting{%d,%d}...\n", SciStatus.enSciStatus, uResetTime);
		uResetTime += 1;

		if (uResetTime > 30)
		{
			mycs_trace(D_ADB, "hisci:Atr failure");
			return (FAILURE);
		}
		HISCI_Msleep(50);
	}
	HISCI_Msleep(100);
	return (SUCCESS);
}


int32_t
HISCI_CardReset(struct s_reader *reader, HI_BOOL bWarmReset)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_RESET_S	SciReset;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:resetcard\n");
	enSciPort = reader->scinum;
	SciReset.enSciPort	= enSciPort;
	SciReset.bWarmReset 	= bWarmReset;
	Ret = ioctl(reader->handle, CMD_SCI_RESET, &SciReset);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.r{%08X}!", Ret);
		return (ERROR);
	}
	Ret = HISCI_ChkAtrCompleted(reader);
	return (Ret);
}


int32_t
HISCI_CardSwitch(struct s_reader *reader, HI_UNF_SCI_PROTOCOL_E enSciProtocol, HI_U32 uFs)
{
	HI_S32	Ret;

	MYSCI_TRACE("mycard:switchcard{%d,%d}\n", enSciProtocol, uFs);
	if (!HISCI_IsAvailable(reader)) return (ERROR);
	if (HI_UNF_SCI_PROTOCOL_T14 == enSciProtocol)
	{
		if ((uFs < 1000) || (uFs > 6000))
		{
			mycs_trace(D_ADB, "hisci:error para uFs is invalid{%d}", uFs);
			return (ERROR);
		}
	}
	else
	{
		if ((uFs < 1000) || (uFs > 5000))
		{
			mycs_trace(D_ADB, "hisci:error para uFs is invalid{%d}", uFs);
			return (ERROR);
		}
	}

#if 0
	//	CMD_SCI_SET_PROTOCOL
	SCI_OPEN_S	SciSwitch;
	HI_UNF_SCI_PORT_E enSciPort = reader->scinum;

	SciSwitch.enSciPort 	= enSciPort;
	SciSwitch.enSciProtocol = enSciProtocol;
	SciSwitch.Frequency 	= uFs;
	Ret = ioctl(reader->handle, CMD_SCI_SWITCH, &SciSwitch);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.s{%08X}!", Ret);
		return (ERROR);
	}
	Ret = HISCI_ChkAtrCompleted(reader);
	return (Ret);
#else
	Ret = HISCI_Close(reader);
	Ret = HISCI_Open(reader, enSciProtocol, uFs);
	if (Ret != SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:open(%d) error!", Ret);
		return (ERROR);
	}
	Ret = HISCI_CardReset(reader, HI_TRUE);
	return (Ret);
#endif
}



int32_t
HISCI_GetParameters(struct s_reader *reader, HI_UNF_SCI_PARAMS_S_PTR pParams)
{
	HI_UNF_SCI_PORT_E enSciPort;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:getparameters\n");
	enSciPort = reader->scinum;
	if (!pParams) return (ERROR);
	pParams->enSciPort = enSciPort;
	Ret = ioctl(reader->handle, CMD_SCI_GET_PARAM, pParams);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.g{%08X}!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}

int32_t
HISCI_SetBaudrates(struct s_reader *reader, HI_U32 uClkFactor, HI_U32 uBaudFactor)
{
	HI_UNF_SCI_PARAMS_S	SciParams;
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_EXT_BAUD_S	SciBaud;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:setbaudrate\n");
	enSciPort = reader->scinum;
	if (uClkFactor<372 || uClkFactor>2048)
	{
		mycs_trace(D_ADB, "hisci:u32ClkRate(%d) is invalid", uClkFactor);
		return (ERROR);
	}
	if ((uBaudFactor< 1) ||
		(uBaudFactor>32) ||
		(1!=uBaudFactor  && (uBaudFactor%2)!=0))
	{
		mycs_trace(D_ADB, "hisci:u32BitRate(%d) is invalid", uBaudFactor);
		return (ERROR);
	}

	Ret = HISCI_GetParameters(reader, &SciParams);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:getparameters error{%d}", Ret);
		return (ERROR);
	}
	if (SciParams.Fi == uClkFactor && SciParams.Di == uBaudFactor)
	{
		mycs_trace(D_ADB, "hisci:setparameters same");
		return (SUCCESS);
	}

	SciBaud.enSciPort = enSciPort;
	SciBaud.ClkRate   = uClkFactor;
	SciBaud.BitRate   = uBaudFactor;
	Ret = ioctl(reader->handle, CMD_SCI_SET_BAUD, &SciBaud);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.b{%08X}!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}

int32_t
HISCI_SetGuardTime(struct s_reader *reader, HI_U32 u32GuardTime)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_ADD_GUARD_S   GuardTime;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:setguardtime\n");
	enSciPort = reader->scinum;
	if (u32GuardTime > 254)
	{
		mycs_trace(D_ADB, "hisci:u32GuardTime(%d) is invalid", u32GuardTime);
		return (ERROR);
	}
	GuardTime.enSciPort = enSciPort;
	GuardTime.AddCharGuard = u32GuardTime;
	Ret = ioctl(reader->handle, CMD_SCI_SET_CHGUARD, &GuardTime);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.gu{%08X}!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}

int32_t
HISCI_SetCharTimeout(struct s_reader *reader, HI_U32 MaxCharTime)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_CHARTIMEOUT_S Chtimeout;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:setchartimeout\n");
	enSciPort = reader->scinum;
	if (HI_UNF_SCI_PROTOCOL_T1 == SMC_Protocol)
	{
		if (MaxCharTime<12 || MaxCharTime>32779)
		{
			mycs_trace(D_ADB, "hisci:MaxCharTime(%d) is invalid", MaxCharTime);
			return (ERROR);
		}
	}
	else
	{
		if (MaxCharTime<960 || MaxCharTime>244800)
		{
			mycs_trace(D_ADB, "hisci:MaxCharTime(%d) is invalid", MaxCharTime);
			return (ERROR);
		}
	}
	Chtimeout.enSciPort = enSciPort;
	Chtimeout.enSciProtocol = SMC_Protocol;
	Chtimeout.CharTimeouts	= MaxCharTime;
	Ret = ioctl(reader->handle, CMD_SCI_SET_CHARTIMEOUT, &Chtimeout);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.ct{%08X}!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}


int32_t
HISCI_SetBlockTimeout(struct s_reader *reader, HI_U32 MaxBlockTime)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_BLOCKTIMEOUT_S BlkTimeout;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:setblocktimeout\n");
	enSciPort = reader->scinum;
	if (MaxBlockTime<971 || MaxBlockTime>491531 )
	{
		mycs_trace(D_ADB, "hisci:MaxBlockTime(%d) is invalid", MaxBlockTime);
		return (ERROR);
	}
	BlkTimeout.enSciPort = enSciPort;
	BlkTimeout.BlockTimeouts = MaxBlockTime;
	Ret = ioctl(reader->handle, CMD_SCI_SET_BLOCKTIMEOUT, &BlkTimeout);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.bt{%08X}!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}

int32_t
HISCI_SetTxRetries(struct s_reader *reader, HI_U32 TxRetryTimes)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_TXRETRY_S TxRetry;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:settxretries\n");
	enSciPort = reader->scinum;
	if (TxRetryTimes > 7)
	{
		mycs_trace(D_ADB, "hisci:TxRetryTimes(%d) is invalid", TxRetryTimes);
		return (ERROR);
	}
	TxRetry.enSciPort = enSciPort;
	TxRetry.TxRetryTimes = TxRetryTimes;
	Ret = ioctl(reader->handle, CMD_SCI_SET_TXRETRY, &TxRetry);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.tr{%08X}!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}



int32_t
HISCI_ReadAtr(struct s_reader *reader, ATR *pAtr) // reads ATR on the fly: reading and some low levelchecking at the same time
{
	HI_UNF_SCI_PORT_E enSciPort;
	HI_S32	   	Ret;
	SCI_ATR_S  	SciAtr;
	HI_U8 		Atrbuf[256];
	int32_t statusreturn =0;

	MYSCI_TRACE("mycard:read_atr\n");
	enSciPort = reader->scinum;
	SciAtr.enSciPort 	= enSciPort;
	SciAtr.pAtrBuf	 	= Atrbuf;
	SciAtr.BufSize	 	= 255;
	Ret = ioctl(reader->handle, CMD_SCI_GET_ATR, &SciAtr);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.ra{%08X}!", Ret);
		return (ERROR);
	}

#if 0
	statusreturn = ATR_InitFromArray(pAtr, SciAtr.pAtrBuf, SciAtr.DataLen); // n should be same as atrlength but in case of atr read error its less so do not use atrlenght here!
	if (statusreturn == ATR_MALFORMED)
	{
		mycs_trace(D_ADB, "hisci:error ATR is malformed");
	}
	if (statusreturn == ERROR)
	{
		mycs_trace(D_ADB, "hisci:error ATR is invalid!");
		return (ERROR);
	}

	HI_UNF_SCI_PARAMS_S	SciParams;
	Ret = HISCI_GetParameters(reader, &SciParams);
	if (Ret == HI_SUCCESS)
	{
		if (SciParams.enProtocolType != SMC_Protocol)
		{
			Ret = HISCI_SetProtocol(reader, SciParams.enProtocolType);
		}
	}
#else
	unsigned char buf[ATR_MAX_SIZE];
	int32_t n = 0;
	uint8_t recvlen = SciAtr.DataLen;
	memcpy(buf, Atrbuf, ATR_MAX_SIZE);
	rdr_log_dbg(reader, D_IFD, "received ATR length = %d", recvlen);

	if (recvlen < 2)
	{
		rdr_log_dbg(reader, D_IFD, "ERROR: only 1 character found in ATR");
		return ERROR;
	}

	if (buf[0] == 0x3F) // 3F: card is using inverse convention, 3B = card is using direct convention
		rdr_log_dbg(reader, D_IFD, "This card uses inverse convention");
	else
		rdr_log_dbg(reader, D_IFD, "This card uses direct convention");
	n++;

	int32_t T0= buf[n];
	int32_t historicalbytes = T0&0x0F; // num of historical bytes in lower nibble of T0 byte
	rdr_log_dbg(reader, D_ATR, "ATR historicalbytes should be: %d", historicalbytes);
	rdr_log_dbg(reader, D_ATR, "Fetching global interface characters for protocol T0"); // protocol T0 always aboard!
	n++;

	int32_t protocols=1, tck = 0, protocol, protocolnumber;	// protocols = total protocols on card, tck = checksum byte present, protocol = mandatory protocol
	int32_t D = 0;												// protocolnumber = TDi uses protocolnumber
	int32_t TDi = T0; // place T0 char into TDi for looped parsing.
	while (n < recvlen)
	{
		if (TDi&0x10)
		{  //TA Present: 							   //The value of TA(i) is always interpreted as XI || UI if i > 2 and T = 15 ='F'in TD(i?)
			//if (IO_Serial_Read(reader, 0, timeout, 1, buf+n)) break;  //In this case, TA(i) contains the clock stop indicator XI, which indicates the logical
																  //state the clockline must assume when the clock is stopped, and the class indicator UI,
			rdr_log_dbg(reader, D_ATR, "TA%d: %02X",protocols,buf[n]);      //which specifies the supply voltage class.
			if ((protocols >2) && ((TDi&0x0F)==0x0F)) {  // Protocol T15 does not exists, it means mandatory on all ATRs
				if ((buf[n]&0xC0) == 0xC0) rdr_log_dbg(reader, D_ATR, "Clockline low or high on clockstop");
				if ((buf[n]&0xC0) == 0x00) rdr_log_dbg(reader, D_ATR, "Clockline not supported on clockstop");
				if ((buf[n]&0xC0) == 0x40) rdr_log_dbg(reader, D_ATR, "Clockline should be low on clockstop");
				if ((buf[n]&0xC0) == 0x80) rdr_log_dbg(reader, D_ATR, "Clockline should be high on clockstop");
				if ((buf[n]&0x3F) == 0x01) rdr_log_dbg(reader, D_ATR, "Voltage class A 4.5~5.5V");
				if ((buf[n]&0x3F) == 0x02) rdr_log_dbg(reader, D_ATR, "Voltage class B 2.7~3.3V");
				if ((buf[n]&0x3F) == 0x03) rdr_log_dbg(reader, D_ATR, "Voltage class A 4.5~5.5V and class B 2.7~3.3V");
				if ((buf[n]&0x3F) == 0x04) rdr_log_dbg(reader, D_ATR, "Voltage RFU");
			}
			if ((protocols >2) && ((TDi&0x0F)==0x01)) {  // Protocol T1 specfic (There is always an obsolete T0 protocol!)
				int32_t ifsc = buf[n];
				if (ifsc == 0x00) ifsc = 32; //default is 32
				rdr_log_dbg(reader, D_ATR, "Maximum information field length this card can receive is %d bytes (IFSC)", ifsc);
			}

			if (protocols < 2) {
				int32_t FI = (buf[n]>>4); // FI is high nibble                  ***** work ETU = (1/D)*(Frequencydivider/cardfrequency) (in seconds!)
				int32_t F  = atr_f_table[FI]; // lookup the frequency divider
				float fmax = atr_fs_table[FI]; // lookup the max frequency      ***** initial ETU = 372 / initial frequency during atr  (in seconds!)

				int32_t DI = (buf[n]&0x0F); // DI is low nibble
				D = atr_d_table[DI]; // lookup the bitrate adjustment (yeah there are floats in it, but in iso only integers!?)
				rdr_log_dbg(reader, D_ATR, "Advertised max cardfrequency is %.2f (Fmax), frequency divider is %d (F)", fmax/1000000L, F); // High nibble TA1 contains cardspeed
				rdr_log_dbg(reader, D_ATR, "Bitrate adjustment is %d (D)", D); // Low nibble TA1 contains Bitrateadjustment
				rdr_log_dbg(reader, D_ATR, "Work ETU = %.2f us", (double) ((1/(double)D)*((double)F/(double)fmax)*1000000)); // And display it...
				rdr_log_dbg(reader, D_ATR, "Initial ETU = %.2f us", (double)372/(double)fmax*1000000); // And display it... since D=1 and frequency during ATR fetch might be different!
			}
			if (protocols > 1 && protocols <3) {
				if ((buf[n]&0x80)==0x80) rdr_log_dbg(reader, D_ATR, "Switching between negotiable mode and specific mode is not possible");
				else {
					rdr_log_dbg(reader, D_ATR, "Switching between negotiable mode and specific mode is possible");
					// int32_t PPS = 1; Stupid compiler, will need it later on eventually
				}
				if ((buf[n]&0x01)==0x01) rdr_log_dbg(reader, D_ATR, "Transmission parameters implicitly defined in the interface characters.");
				else rdr_log_dbg(reader, D_ATR, "Transmission parameters explicitly defined in the interface characters.");

				protocol = buf[n]&0x0F;
				if (protocol) rdr_log_dbg(reader, D_ATR, "Protocol T = %d is to be used!", protocol);
			}
			n++; // next interface character
		}
		if (TDi&0x20)
		{	 //TB Present
			//if (IO_Serial_Read(reader, 0, timeout, 1, buf+n)) break;
			rdr_log_dbg(reader, D_ATR, "TB%d: %02X",protocols,buf[n]);
			if ((protocols >2) && ((TDi&0x0F)==0x01)) {  // Protocol T1 specfic (There is always an obsolete T0 protocol!)
				int32_t CWI = (buf[n]&0x0F); // low nibble contains CWI code for the character waiting time CWT
				int32_t BWI = (buf[n]>>4); // high nibble contains BWI code for the block waiting time BWT
				int32_t CWT = (1<<CWI) + 11; // in work etu  *** 2^CWI + 11 work etu ***
				rdr_log_dbg(reader, D_ATR, "Protocol T1: Character waiting time is %d work etu (CWT)", CWT);
				int32_t BWT = (int) ((1<<BWI) * 960 * 372); // divided by frequency and add with 11 work etu *** 2^BWI*960*372/f + 11 work etu ***
				rdr_log_dbg(reader, D_ATR, "Protocol T1: Block waiting time is %d divided by actual cardfrequency + 11 work etu (BWI)", BWI);
				rdr_log_dbg(reader, D_ATR, "Protocol T1: BWT: %d", BWT);
			}

			n++; // next interface character
		}
		if (TDi&0x40)
		{	 //TC Present
			//if (IO_Serial_Read(reader, 0, timeout, 1, buf+n)) break;
			rdr_log_dbg(reader, D_ATR, "TC%d: %02X",protocols, buf[n]);
			if ((protocols > 1) && ((TDi&0x0F)==0x00)) {
				int32_t WI = buf[n];
				rdr_log_dbg(reader, D_ATR, "Protocol T0: work wait time is %d work etu (WWT)", (int) (960*D*WI));
			}
			if ((protocols > 1) && ((TDi&0x0F)==0x01)) {
				if (buf[n]&0x01) rdr_log_dbg(reader, D_ATR, "Protocol T1: CRC is used to compute the error detection code");
				else rdr_log_dbg(reader, D_ATR, "Protocol T1: LRC is used to compute the error detection code");
			}
			if ((protocols < 2) && (buf[n]<0xFF)) rdr_log_dbg(reader, D_ATR, "Extra guardtime of %d ETU (N)", (int) buf[n]);
			if ((protocols < 2) && (buf[n]==0xFF)) rdr_log_dbg(reader, D_ATR, "Protocol T1: Standard 2 ETU guardtime is lowered to 1 ETU");

			n++; // next interface character
		}
		if (TDi&0x80)
		{	//TD Present? Get next TDi there will be a next protocol
			//if (IO_Serial_Read(reader, 0, timeout, 1, buf+n)) break;
			rdr_log_dbg(reader, D_ATR, "TD%d %02X",protocols,buf[n]);
			TDi = buf[n];
			protocolnumber = TDi&0x0F;
			if (protocolnumber == 0x00) tck = 0; // T0 protocol do not use tck byte  (TCK = checksum byte!)
			if (protocolnumber == 0x0E) tck = 1; // T14 protocol tck byte should be present
			if (protocolnumber == 0x01) tck = 1; // T1 protocol tck byte is mandatory, BTW: this code doesnt calculate if the TCK is valid jet...
			rdr_log_dbg(reader, D_ATR, "Fetching global interface characters for protocol T%d:", (TDi&0x0F)); // lower nibble contains protocol number
			protocols++; // there is always 1 protocol T0 in every ATR as per iso defined, max is 16 (numbered 0..15)

			n++; // next interface character
		}
		else break;
	}
	int32_t atrlength = 0;
	atrlength += n;
	atrlength += historicalbytes;
	rdr_log_dbg(reader, D_ATR, "Total ATR Length including %d historical bytes should be %d",historicalbytes,atrlength);
	if (T0&0x80) protocols--;	// if bit 8 set there was a TD1 and also more protocols, otherwise this is a T0 card: substract 1 from total protocols
	rdr_log_dbg(reader, D_ATR, "Total protocols in this ATR is %d",protocols);

	while (n < atrlength + tck)
	{ // read all the rest and mandatory tck byte if other protocol than T0 is used.
		//if (IO_Serial_Read(reader, 0, timeout, 1, buf+n)) break;
		n++;
	}

	if (n!=atrlength+tck) cs_log("Warning reader %s: Total ATR characters received is: %d instead of expected %d", reader->label, n, atrlength+tck);

	if ((buf[0] != 0x3B) && (buf[0] != 0x3F) && (n>9 && !memcmp(buf+4, "IRDETO", 6))) //irdeto S02 reports FD as first byte on dreambox SCI, not sure about SH4 or phoenix
		  buf[0]  = 0x3B;

	statusreturn = ATR_InitFromArray (pAtr, buf, n); // n should be same as atrlength but in case of atr read error its less so do not use atrlenght here!

	if (statusreturn == ATR_MALFORMED) cs_log("Warning reader %s: ATR is malformed, you better inspect it with a -d2 log!", reader->label);

	if (statusreturn == ERROR) {
		cs_log("Warning reader %s: ATR is invalid!", reader->label);
		return ERROR;
	}
#endif
	return (SUCCESS); // return (OK) but atr might be softfailing!
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
int32_t
HISCIAPI_Initialize(struct s_reader *reader)
{
	HI_UNF_SCI_PROTOCOL_E enSciProtocol;
	HI_U32	uFs;
	HI_S32	Ret;

	MYSCI_TRACE("mycard:initialize(%s)\n", HI_DEVICE_NAME);
	reader->card_status	= NO_CARD;
	reader->use_gpio 		= 0;
	reader->resetcounter = 0;
	reader->mhz 			= 9600;
	SMC_Readerp = reader;
	HISCIAPI_SaveCardstatus(reader);
	cs_save_cardinformation(reader);

	reader->scinum = HI_UNF_SCI_PORT0;
	reader->handle = open("/dev/"HI_DEVICE_NAME, O_RDWR, 0);
	if (reader->handle < 0)
	{
		mycs_trace(D_ADB, "hisci:error:%s", HI_DEVICE_NAME);
		return (ERROR);
	}
	//
	//
	//
	#if 1
	uFs = 3570;
//	uFs = 3692;
	enSciProtocol = HI_UNF_SCI_PROTOCOL_T0;
	#else
	uFs = 6000;
	enSciProtocol = HI_UNF_SCI_PROTOCOL_T14;
	#endif
	Ret = HISCI_Open(reader, enSciProtocol, uFs);
	if (Ret != SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:open(%d) error!", Ret);
		return (ERROR);
	}
	return (SUCCESS);
}


int32_t
HISCIAPI_Terminates(struct s_reader *reader)
{
	HI_S32 Ret;

	MYSCI_TRACE("mycard:terminate(%s)\n", HI_DEVICE_NAME);
	HISCIAPI_SaveCardstatus(reader);
	cs_save_cardinformation(reader);
	if (reader->handle < 0) return (ERROR);
	HISCI_Close(reader);

	Ret = close(reader->handle);
//	pthread_mutex_destroy(&CUR_Mutex);
	if (HI_SUCCESS != Ret)
	{
		mycs_trace(D_ADB, "hisci:close error{%d}!", Ret);
		return (ERROR);
	}
	reader->handle = -1;
	SMC_Readerp = NULL;
	return (SUCCESS);
}


int32_t
HISCIAPI_GetStatus(struct s_reader *reader, int32_t *status)
{
	*status = 0;
	if (HISCI_ChkInserted(reader)) *status = 1;
	return (SUCCESS);
}


int32_t
HISCIAPI_DoReset(struct s_reader *UNUSED(reader), struct s_ATR *UNUSED(pAtr),
	int32_t (*rdr_activate_card) (struct s_reader *, struct s_ATR *, uint16_t deprecated),
	int32_t (*rdr_get_cardsystem)(struct s_reader *, struct s_ATR *))
{
	MYSCI_TRACE("mycard:doreset\n");
	SMC_Activated = FALSE;
	if (rdr_activate_card)
	{
	}
	if (rdr_get_cardsystem)
	{
	}
	return (SUCCESS);
}

#define	MAX_SWITCH_TRIES	4	// 2

int32_t
HISCIAPI_Activate(struct s_reader *reader, ATR *pAtr)
{
	HI_U32	uFs;
	HI_S32	Ret;
	HI_S32	uTries = 0;

	MYSCI_TRACE("mycard:activate{%d,%d}\n", SMC_Protocol, SMC_Activated);
	if (!HISCI_ChkInserted(reader))
	{
		mycs_trace(D_ADB, "hisci:card none");
		return (ERROR);
	}
	if (reader->ins7e11_fast_reset)
	{
		rdr_log(reader, "Doing fast reset");
		Ret = HISCI_CardReset(reader, HI_TRUE);
		if (Ret == SUCCESS)
		{
			Ret = HISCI_ReadAtr(reader, pAtr);
		}
	}
	else
	if (!SMC_Activated)
	{
		Ret = HISCI_CardReset(reader, HI_FALSE);
		if (Ret == FAILURE)
		{
			while (uTries++ < MAX_SWITCH_TRIES)
			{
				//	(T0,T1),T14
				HISCI_Msleep(100);
				SMC_Protocol = (SMC_Protocol == HI_UNF_SCI_PROTOCOL_T14) ? HI_UNF_SCI_PROTOCOL_T0 : HI_UNF_SCI_PROTOCOL_T14;
				uFs = (SMC_Protocol == HI_UNF_SCI_PROTOCOL_T14) ? 6000 : 3570;
				Ret = HISCI_CardSwitch(reader, SMC_Protocol, uFs);
				if (Ret == SUCCESS) break;
			}
		}

		if (Ret == SUCCESS)
		{
			Ret = HISCI_ReadAtr(reader, pAtr);
		}
		SMC_Activated = TRUE;
	}
	else
	{
		Ret = HISCI_CardReset(reader, HI_FALSE);
		if (Ret == SUCCESS)
		{
			Ret = HISCI_ReadAtr(reader, pAtr);
		}
	}
	return (Ret);
}


#if 0
int32_t
HISCIAPI_WriteSettings3(struct s_reader *reader,
	uint32_t ETU, uint32_t WWT, uint32_t I)
{
	MYSCI_TRACE("mycard:write_settings3{%d,%d,%d}\n", ETU, WWT, I);
	return (SUCCESS);
}

int32_t
HISCIAPI_WriteSettings2(struct s_reader *reader,
	uint16_t F, uint8_t D, uint32_t WWT, uint32_t EGT, uint32_t BGT)
{
	MYSCI_TRACE("mycard:write_settings2{%d,%d}\n", EGT, BGT);
	return (SUCCESS);
}
#endif


int32_t
HISCIAPI_WriteSettings(struct s_reader *reader, struct s_cardreader_settings *s)
{
	if (reader->scideprecated)
	{
		// future test.
		MYSCI_TRACE("mycard:writesettings(deprecated)\n");
		return (SUCCESS);
	}
//	MYSCI_TRACE("mycard:writesettings{%d,%d} {%d,%d,%d, %d,%d}\n",
//			Fi, Di, ETU, EGT, P, I, Ni);
	mycs_trace(D_ADB, "mycard:writesettings{%d,%d}", s->Fi, s->Di);
	if (s->Fi == 372) return (SUCCESS);
	if (s->Fi == 612) return (SUCCESS);
	HISCI_SetBaudrates(reader, s->Fi, s->Di);
	return (SUCCESS);
}


int32_t
HISCIAPI_Transmit(struct s_reader *reader, unsigned char *buffer, uint32_t size, uint32_t UNUSED(expectedlen), uint32_t UNUSED(delay), uint32_t timeout)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_STATUS_S	SciStatus;
	SCI_DATA_S		SciData;
	HI_U32	u32ActLen;
	HI_S32	Ret;

//	MYSCI_TRACE("mycard:transmit{%3d,%d}\n", size, timeout);
	if (!HISCI_IsAvailable(reader)) return (ERROR);
   if (timeout < MAX_TX_TIMEOUT) timeout = MAX_TX_TIMEOUT;
	enSciPort = reader->scinum;
	if (!buffer)
	{
		mycs_trace(D_ADB, "hisci:error transmit buffer!");
		return (ERROR);
	}

	if (!size)
	{
		mycs_trace(D_ADB, "hisci:transmit size?{%d}", size);
		return (ERROR);
	}

	SciStatus.enSciPort = enSciPort;
	Ret = ioctl(reader->handle, CMD_SCI_GET_STATUS, &SciStatus);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.wstatus{%08X}!", Ret);
		return (ERROR);
	}

	if (SciStatus.enSciStatus < HI_UNF_SCI_STATUS_READY)
	{
		if (!_IS_CARD_INSERTED_(SciStatus.enSciStatus)) SMC_Inserted = 0;
		mycs_trace(D_ADB, "hisci:current state cann't execute send opertaion");
		return (ERROR);
	}
	SciData.enSciPort = enSciPort;
	SciData.pDataBuf  = buffer;
	SciData.BufSize   = size;
	SciData.TimeoutMs = timeout;
	Ret = ioctl(reader->handle, CMD_SCI_SEND_DATA, &SciData);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.w{%d.%08X, %d}!", size, Ret, timeout);
		return (ERROR);
	}
	u32ActLen = SciData.DataLen;
	if (u32ActLen < size)
	{
		mycs_trace(D_ADB, "hisci:transmit size error{%d,%d}!", u32ActLen, size);
		return (ERROR);
	}
	return (SUCCESS);
}


int32_t
HISCIAPI_Receive(struct s_reader *reader, unsigned char *buffer, uint32_t size, uint32_t UNUSED(delay), uint32_t timeout)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_STATUS_S	SciStatus;
	SCI_DATA_S		SciData;
	HI_U32	u32ActLen;
	HI_S32	Ret;

//	MYSCI_TRACE("mycard:receive.{%3d,%d}\n", size, timeout);
	if (!HISCI_IsAvailable(reader)) return (ERROR);
	timeout = MAX_RX_TIMEOUT;
	enSciPort = reader->scinum;
	if (!buffer)
	{
		mycs_trace(D_ADB, "hisci:error receive buffer!");
		return (ERROR);
	}
	if (!size)
	{
		mycs_trace(D_ADB, "hisci:receive size?{%d}", size);
		return (SUCCESS);
	}

	SciStatus.enSciPort = enSciPort;
	Ret = ioctl(reader->handle, CMD_SCI_GET_STATUS, &SciStatus);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.rstatus{%08X}!", Ret);
		return (ERROR);
	}

	if (SciStatus.enSciStatus < HI_UNF_SCI_STATUS_READATR)
	{
		if (!_IS_CARD_INSERTED_(SciStatus.enSciStatus)) SMC_Inserted = 0;
		mycs_trace(D_ADB, "hisci:current state cann't execute send opertaion");
		return (ERROR);
	}

	SciData.enSciPort = enSciPort;
	SciData.pDataBuf  = buffer;
	SciData.BufSize   = size;
	SciData.TimeoutMs = timeout;
	Ret = ioctl(reader->handle, CMD_SCI_RECEIVE_DATA, &SciData);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.r{%08X}!", Ret);
		return (ERROR);
	}

	u32ActLen = SciData.DataLen;
	if (u32ActLen < size)
	{
		mycs_trace(D_ADB, "hisci:error receive size{%d,%d}!", u32ActLen, size);
		return (ERROR);
	}
	return Ret;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
#if defined(__HIS_PPSNEGOTIATION__)
int32_t
HISCIAPI_PpsExchange(struct s_reader *reader, unsigned char *params, unsigned *length, uint32_t lenrequest)
{
	HI_UNF_SCI_PORT_E enSciPort;
	SCI_STATUS_S	SciStatus;
	HI_S32	Ret;

	if (!HISCI_IsAvailable(reader)) return (ERROR);
	enSciPort = reader->scinum;
	SciStatus.enSciPort = enSciPort;
	Ret = ioctl(reader->handle, CMD_SCI_GET_STATUS, &SciStatus);
	if (Ret != HI_SUCCESS)
	{
		mycs_trace(D_ADB, "hisci:io error.e{%08X}!", Ret);
		return (ERROR);
	}
	if (!params) return (ERROR);
	if (!length) return (ERROR);
	if (!lenrequest) return (ERROR);

	#if 0
	{
		SCI_PPS_S	SciPPS;

		MYSCI_TRACE("mycard:ppsexchange{%d}\n", lenrequest);
		memcpy((HI_U8 *)SciPPS.Send, params, lenrequest);
		SciPPS.enSciPort= enSciPort;
		SciPPS.SendLen	= lenrequest;
		SciPPS.RecTimeouts = MAX_RX_TIMEOUT;
		Ret = ioctl(reader->handle, CMD_SCI_SEND_PPS_DATA, &SciPPS);
		if (Ret != HI_SUCCESS)
		{
			mycs_trace(D_ADB, "hisci:io error.e{%08X}!", Ret);
			return (ERROR);
		}

		SciPPS.enSciPort = enSciPort;
		Ret = ioctl(reader->handle, CMD_SCI_GET_PPS_DATA, &SciPPS);
		if (Ret != HI_SUCCESS)
		{
			mycs_trace(D_ADB, "hisci:io error.e{%08X}!", Ret);
			return (ERROR);
		}
		memcpy(params,(HI_U8 *)SciPPS.Receive, SciPPS.ReceiveLen);
		*length = SciPPS.ReceiveLen;
		if ((lenrequest != SciPPS.ReceiveLen) || (memcmp (params, SciPPS.Receive, lenrequest)))
		{
			mycs_trace(D_ADB, "hisci:pps error{%d,%d}!", lenrequest, SciPPS.ReceiveLen);
			return (ERROR);
		}
		mycs_trace(D_ADB, "hisci:pts succesfull:FI=%d, DI=%d", params[2] >> 4, params[2] & 0x0F);
	}
	#else
		MYSCI_TRACE("mycard:ppsexchange{none}\n");
	#endif
	return (SUCCESS);
}
#endif	// defined(__HIS_PPSNEGOTIATION__)

void
HISCIAPI_DisplayMessages(struct s_reader *reader, char *messages)
{
	if (!HISCI_IsAvailable(reader)) return;
	if (!messages) return;
	myprintf("mycard:displaymessages{%d}{%04X,%s}......\n",
				 reader->restarting,
 				 reader->caid,
 				 messages);
	reader->restarting = 0;
}


#if 0
int32_t
HISCIAPI_InitLocks(struct s_reader *reader)
{
//	UNAVAILABLE
//	pthread_mutex_init(&CUR_Mutex, NULL);
//	MYSCI_TRACE("mycard:initlocks......\n");
	return (SUCCESS);
}


void
HISCIAPI_Lock(struct s_reader *reader)
{
//	UNAVAILABLE
//	MYSCI_TRACE("mycard:lock......\n");
//	pthread_mutex_lock(&CUR_Mutex);
}


void
HISCIAPI_Unlock(struct s_reader *reader)
{
//	UNAVAILABLE
//	MYSCI_TRACE("mycard:unlock......\n");
//	pthread_mutex_unlock(&CUR_Mutex);
}


int32_t
HISCIAPI_SetParity(struct s_reader *reader, uchar parity)
{
//	UNAVAILABLE
	MYSCI_TRACE("mycard:setparity{%d}......\n", parity);
	return (SUCCESS);
}

int32_t
HISCIAPI_SetBaudrate(struct s_reader *reader, uint32_t baudrate)
{
//	UNAVAILABLE
	MYSCI_TRACE("mycard:setbaudrate{%d}......\n", baudrate);
	return (SUCCESS);
}

void
HISCIAPI_SetTransmitTimeout(struct s_reader *reader)
{
//	UNAVAILABLE
	MYSCI_TRACE("mycard:settransmittimeout......\n");
}


bool
HISCIAPI_SetDtsRts(struct s_reader *reader, int32_t *dtr, int32_t *rts)
{
	// UNAVAILABLE
	MYSCI_TRACE("mycard:setdtsrts{%d,%d}......\n", *dtr, *rts);
	return (SUCCESS);
}
#endif
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
const struct s_cardreader cardreader_internal_hisky =
{
//	MYSCI_TRACE("CARDREADER_INTERNAL_HISKY......\n");
	.desc					= "internal",
	.typ					= R_INTERNAL,
	.flush 				= 0,
	.max_clock_speed	= 1,
	.need_inverse		= 0,
//	.timings_in_etu	= 1,
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	.reader_init		= HISCIAPI_Initialize,
	.get_status			= HISCIAPI_GetStatus,
	.activate			= HISCIAPI_Activate,
	.transmit			= HISCIAPI_Transmit,
	.receive				= HISCIAPI_Receive,
	.close 				= HISCIAPI_Terminates,
	.write_settings	= HISCIAPI_WriteSettings,
//	.write_settings2	= HISCIAPI_WriteSettings2,
//	.write_settings3	= HISCIAPI_WriteSettings3,
	.do_reset			= HISCIAPI_DoReset,
	#if defined(__HIS_PPSNEGOTIATION__)
	.set_protocol		= HISCIAPI_PpsExchange,
	#endif	// defined(__HIS_PPSNEGOTIATION__)
	//------------------------------------------------------------------
	//------------------------------------------------------------------
//	.lock_init 			= HISCIAPI_InitLocks,
//	.lock					= HISCIAPI_Lock,
//	.unlock				= HISCIAPI_Unlock,
//	.set_parity			= HISCIAPI_SetParity,
//	.set_baudrate		= HISCIAPI_SetBaudrate,
//	.set_transmit_timeout	= HISCIAPI_SetTransmitTimeout,
//	.set_DTS_RTS		= HISCIAPI_SetDtsRts,
	.display_msg		= HISCIAPI_DisplayMessages,
};

#endif
