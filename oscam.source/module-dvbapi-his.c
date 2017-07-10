/* Reversed from libhistream.so, this comes without any warranty */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "globals.h"

#if defined(WITH_HISILICON)

#define DVBAPI_LOG_PREFIX 		1
#include "module-dvbapi.h"
#include "module-dvbapi-his.h"
#include "oscam-string.h"
#include "oscam-client.h"
#include "oscam-time.h"

#undef	UNUSED
#include "hi_unf_demux.h"

#if 1
	#if (__ADB_TRACE__==1)
	#define HIPRT_FATAL	myprintf
	#else
	#define HIPRT_FATAL	mycs_log
	#endif
#else
	#define HIPRT_FATAL(...)
#endif

#if 1
	#if (__ADB_TRACE__==1)
	#define HIPRT_ERROR	myprintf
	#else
	#define HIPRT_ERROR	mycs_log
	#endif
#else
	#define HIPRT_ERROR(...)
#endif

#if 1
	#define HIPRT_WARN	myprintf
#else
	#define HIPRT_WARN(...)
#endif

#if 1
	#define HIPRT_INFO	myprintf
#else
	#define HIPRT_INFO(...)
#endif

#if 1
	#define HIPRT_TRACE	myprintf
#else
	#define HIPRT_TRACE(...)
#endif
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
/***************************** Macro Definition ************************/
#define TUNER0	0
#define TUNER1	1
#if defined(SDKV600)
	// demux id 0 to 6
	#define HIPORT_TUNER_ID 	HI_UNF_DMX_PORT_TSI_0
	#define HIPORT_INTERNAL_ID HI_UNF_DMX_PORT_TSI_1
	#define HIDEMUX_OSCAM_ID 	0
	#define HIDEMUX_OSCAM1_ID 	HIDEMUX_OSCAM_ID
	#define HIDEMUX_OSCAM2_ID 	5
#elif defined(SDKV500)
	// demux id 0 to 6
	#define HIPORT_TUNER_ID 	HI_UNF_DMX_PORT_TSI_0
	#define HIDEMUX_OSCAM_ID 	0
#else
	#define HIPORT_TUNER_ID 	1
	#define HIDEMUX_OSCAM_ID	0	// 3
#endif


#define NUM_HIREGION				3
#define NUM_HIPERREGION			32
#define MAX_HICHANNEL			(NUM_HIREGION*NUM_HIPERREGION)	// DMX_TOTALCHAN_CNT
#define MAX_HIFILTERS			(NUM_HIREGION*NUM_HIPERREGION)	// DMX_TOTALFILTER_CNT
#define NUM_HIFILACQUIRES		32		// 3
#define NUM_HIFLTDEPTH			16		// DMX_FILTER_MAX_DEPTH
#define DVB_HIFLTBYTES			8		// DMX_FILTER_MAX_DEPTH
#define MAX_HIBUFFERSIZE		(64 * 1024)
#define INVALID_HIFILTER		-1
#define INVALID_HIHANDLE		HI_INVALID_HANDLE

#define IS_INVALID_PID(x)			((x)>=0x1fff)
#define IS_INVALID_HIFILTERS(x)	((x)>MAX_HIFILTERS-1 || (x)<0)
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
#define HIAPI_ISDEMUXINIT() \
	if (g_DemuxIniz == HI_FALSE) { \
		HIPRT_FATAL("mydemux:demux none...\n"); \
		return HI_FAILURE; \
	}

#define HIAPI_CHECKFILTERID(id) \
	if ((id) >= MAX_HIFILTERS) { \
		HIPRT_FATAL("mydemux:invalid filterid(%d)\n", (id)); \
		return HI_FAILURE; \
	}


#define HIAPI_FILTERREGION(dmxid,region) \
	if ((dmxid) == 0)      { (region) = 0; } \
	else if ((dmxid) == 4) { (region) = 2; } \
	else                   { (region) = 1; }

#if 0
#define HIAPI_CHECKERROR(hiReturn,FUNC)	do {} while (0);
#else
#define HIAPI_CHECKERROR(hiReturn,FUNC)	\
	if (hiReturn != HI_SUCCESS) { \
		HIPRT_ERROR("mydemux:%s.%s.error(%x)\n",__FUNCTION__,#FUNC, hiReturn); \
	}
#endif

#define HIFLT_REVERSE_MASK(mask) 	(HI_U8)(~(mask))
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
/*************************** Structure Definition **********************/
/* Transparent transmission mode, only the data to reach the internal buffer of events and timeout events, not a copy of the data does not appear to copy data to an external buffer event
	In copy mode, in addition to the data reaches the internal buffer of events and timeout events, there will be a copy of the data to the external buffer events.
	However, in the copy mode, when the reach the data of the external buffer event, the user via the callback function's return value to confirm whether to continue copying, in order to discard the unwanted duplicate data.
	At this point, the return value of 0 means copy a copy, and direct release.
*/

typedef struct hidemux_attrib
{
	HI_S32		dmuxid;
	HI_S32		fltnum;

	HI_U32		u32TUNERID;				/* TUNER ID ,0-1*/
	HI_U32		u32DMXID;				/* DMX ID ,0-4*/
	HI_U32		u32PID;					/* TS PID */

	HI_S32		u32Type; 				/* task type 1 - TYPE_ECM  2- TYPE_EMM */
	HI_U32		u32FilterType; 		/* section type 0 - SECTION  1- PES  2 - ECM/EMM */
	HI_U32		u32CrcFlag; 			/* crc check flag,0 - close crc check 1-force crc check 2-crc check by sytax*/
	HI_U32		u32TimeOutMs;			/* time out in ms,0 stand for not timeout,otherwise when timeout,the user can receive the envent*/

	HI_U32		u32FilterDepth;		/* Filter Depth*/
	HI_U8			u8Match	[NUM_HIFLTDEPTH];
	HI_U8			u8Mask	[NUM_HIFLTDEPTH];
	HI_U8			u8Negate	[NUM_HIFLTDEPTH];
} HIDEMUX_ATTRIB;

typedef struct hidemux_filters
{
	HIDEMUX_ATTRIB fltAttr;
	HI_S32		uFiltype;
	HI_U32		u32UseFlag;				/*use flag :0- available,1-busy*/
	HI_U32		u32EnableFlag;			/*enable flag :0- enable,1-disable*/

	HI_HANDLE	hChannel;				/*corresponding  channel of filter*/
	HI_HANDLE	hFilter;					/*corresponding  hander of filter*/

	HI_U32		u32TimerCount;			/*current value in timer of (ms)filter */
} HIDEMUX_FILTERS;

typedef struct hidemux_inform {
	int32_t		adapter;
	int32_t		dmuxid;
	int32_t		flnum;
	int32_t		hifltnum;
	int32_t		type;
	int32_t		pid;
	bool			activated;
} HIDEMUX_INFORM;


struct hidemux_thread_param
{
	int32_t id;
//	int32_t flnum;
	struct s_client *cli;
};

/********************** Global Variable declaration ********************/
extern struct s_client		*dvbApi_client;

static HI_BOOL					g_DemuxIniz 	= 0;
static HI_BOOL 				g_DemuxRuning 	= HI_TRUE;
static pthread_mutex_t		g_FltMutex;
static pthread_mutex_t		g_FltLocks;
static pthread_t				g_FltEcmThread;
static pthread_t				g_FltEmmThread;

static HIDEMUX_FILTERS		g_HIFilters[MAX_HIFILTERS];
static HIDEMUX_INFORM		g_dmuxes[MAX_DEMUX][MAX_FILTER];
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//feedback data to filter by callback
static HI_BOOL
hifilter_SectionCallbacks(HIDEMUX_FILTERS *ChFilter, HI_U8 *pSection, HI_U32 uSecLen)
{
	HIDEMUX_ATTRIB *pFltAttr;
	int k = 0;

	if (!uSecLen ) return 0;
	if (!pSection) return 0;
	if (ChFilter->u32UseFlag == 0) return 0;
	if (ChFilter->u32EnableFlag == 0) return 0;
	pFltAttr = &(ChFilter->fltAttr);
	if ((0 == pFltAttr->u32FilterType || 2 == pFltAttr->u32FilterType) && pFltAttr->u32FilterDepth != 0) /*section data,filter data by software*/
	{
		if (pFltAttr->u8Negate[0])
		{
			if ((pFltAttr->u8Match[0] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[0])) ==
			  	 (pSection[0] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[0])))
			{
			  	return 0;
			}
		}
		else
		{
			if ((pFltAttr->u8Match[0] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[0])) !=
			  	 (pSection[0] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[0])))
			{
			  	return 0;
			}
		}

		for (k = 1; k < (int)(pFltAttr->u32FilterDepth); k++)
		{
			if (pFltAttr->u8Negate[k])
			{
			  	if ((pFltAttr->u8Match[k] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[k])) ==
					 (pSection[2+k] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[k])))
			  	{
					break;
			  	}
			}
			else
			{
			  	if ((pFltAttr->u8Match[k] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[k])) !=
					 (pSection[2+k] & HIFLT_REVERSE_MASK(pFltAttr->u8Mask[k])))
			  	{
					break;
			  	}
			}
		}
	}
	else
	{
		k = pFltAttr->u32FilterDepth; /*do not filter pes*/
	}

	if (k == (int)(pFltAttr->u32FilterDepth)) /*get matched filter condition */
	{
		ChFilter->u32TimerCount = 0;
//		if (ChFilter->uFiltype==TYPE_EMM) {
//			myprintf("FILTER:%d, %d\n", uFilNum, i);
//		}
		dvbapi_process_input(pFltAttr->dmuxid, pFltAttr->fltnum, (void *)pSection, uSecLen);
		return 1;
	}
	return 0;
}


static void *
hifilter_ThreadModules(void *param)
{
	struct hidemux_thread_param *para = (struct hidemux_thread_param *)param;
	HI_UNF_DMX_DATA_S ChRcvBuf[NUM_HIFILACQUIRES];
	HI_UNF_DMX_CHAN_STATUS_S channelstatus;
//	HI_UNF_DMX_DATA_TYPE_E enDataType;
//	HI_UNF_DMX_CHAN_ATTR_S stChAttr;
	HIDEMUX_FILTERS *ChFilter;
	HI_HANDLE ChHandle;
	HI_S32 	 ChTYPE;
//	HI_U32 	 u32HandleNum = MAX_HICHANNEL;
//	HI_HANDLE u32ChHandle[MAX_HICHANNEL];
	HI_U32 u32AcqNum 	= 0;
	HI_U32 u32BufLen 	= 0;
	HI_U8  *pRecvBuff	= 0;
	HI_S32 hiReturn = HI_FAILURE;
	int f, ki = 0;

	ChTYPE = para->id;
	pthread_setspecific(getclient, para->cli);
	while (g_DemuxRuning)
	{
//		usleep(10 * 1000);
		cs_sleepms(20);
//		memset((void *)u32ChHandle, 0, sizeof(HI_HANDLE) * MAX_HICHANNEL);
//		u32HandleNum = MAX_HICHANNEL;
//		hiReturn = HI_UNF_DMX_GetDataHandle(u32ChHandle, &u32HandleNum, 1000);	/* timeout 1000ms */
//		if ((HI_SUCCESS != hiReturn) || (u32HandleNum == 0)) continue;
		for (f = 0; f < MAX_HIFILTERS; f++)
		{
		  	ChFilter = &(g_HIFilters[f]);
			if ( ChFilter->uFiltype != ChTYPE) continue;
			if (!ChFilter->u32UseFlag || !ChFilter->u32EnableFlag) continue;
			#if defined(SDKV500) || defined(SDKV600)
				if (ChFilter->hChannel == 0) continue;
			#else
				if (ChFilter->hChannel < 0x150000 || ChFilter->hChannel > 0x200000) continue;
			#endif

			ChHandle = ChFilter->hChannel;
			hiReturn = HI_UNF_DMX_GetChannelStatus(ChHandle, &channelstatus);
			if ((HI_SUCCESS != hiReturn) || (HI_UNF_DMX_CHAN_CLOSE == channelstatus.enChanStatus)) continue;

			u32AcqNum = 0;
			hiReturn  = HI_UNF_DMX_AcquireBuf(ChHandle, NUM_HIFILACQUIRES, &u32AcqNum, ChRcvBuf, 100);
			if (hiReturn == HI_ERR_DMX_TIMEOUT ||
				 hiReturn == HI_ERR_DMX_NOAVAILABLE_DATA ||
				 u32AcqNum== 0) {
				cs_sleep(10);
				continue;
			}
			if (hiReturn == HI_FAILURE) break;
			if (HI_SUCCESS != hiReturn)
			{
			 	HIPRT_WARN("mydemux:hi_unf_dmx_acquirebuf.error(%x)\n", (HI_U32)hiReturn);
			 	break;
			}
//			hiReturn = HI_UNF_DMX_GetChannelAttr(ChHandle, &stChAttr);
//			if (HI_SUCCESS != hiReturn) continue;
			if (u32AcqNum==NUM_HIFILACQUIRES)
			{
			 	HIPRT_WARN("mydemux:max Acquire(%d)\n", u32AcqNum);
			}
//			else
//			{
//			 	HIPRT_WARN("mydemux:acquire\n");
//			}
			for (ki = 0; ki < (int)u32AcqNum; ki++) /*process data package gradually*/
			{
				pRecvBuff  = ChRcvBuf[ki].pu8Data;
				u32BufLen  = ChRcvBuf[ki].u32Size;
// 			enDataType = ChRcvBuf[ki].enDataType;
//				if (stChAttr.enChannelType == HI_UNF_DMX_CHAN_TYPE_POST)
//				{
//					pRecvBuff += 5;
//					hifilter_SectionCallbacks(ChFilter, pRecvBuff, u32BufLen);
//				}
//				else
				{
				  	hifilter_SectionCallbacks(ChFilter, pRecvBuff, u32BufLen);
				}
			}

			/*release message*/
			hiReturn = HI_UNF_DMX_GetChannelStatus(ChHandle, &channelstatus);
			if ((HI_SUCCESS != hiReturn) ||
				((HI_UNF_DMX_CHAN_CLOSE == channelstatus.enChanStatus) && (HI_SUCCESS == hiReturn)))
			{
			  	/*avoid reference ChRcvBuf after closing channel */
			  	break;
			}
//				hiReturn = HI_UNF_DMX_GetChannelStatus(ChHandle, &channelstatus);
//				if ((HI_SUCCESS != hiReturn) || (HI_UNF_DMX_CHAN_CLOSE == channelstatus.enChanStatus)) continue;
			hiReturn = HI_UNF_DMX_ReleaseBuf(ChHandle, u32AcqNum, ChRcvBuf);
//				HIAPI_CHECKERROR(hiReturn,hi_unf_dmx_releasebuf);
			cs_sleep(10);
		}
	}
	return NULL;
}

static HI_S32
hifilter_CheckFltAttr(HIDEMUX_ATTRIB *pFltAttr)
{
	if (!pFltAttr) return -1;
	if ((pFltAttr->u32FilterType > 2))
	{
	  	HIPRT_ERROR("mydemux:hifilter_checkfltattr.error.type(%d)\n", pFltAttr->u32FilterType);
	  	return -1;
	}

	if (pFltAttr->u32CrcFlag > 2)
	{
	  	HIPRT_ERROR("mydemux:hifilter_checkfltattr.error.crc(%d)\n", pFltAttr->u32CrcFlag);
	  	return -1;
	}

	if (pFltAttr->u32FilterDepth > NUM_HIFLTDEPTH)
	{
	  	HIPRT_ERROR("mydemux:hifilter_checkfltattr.error.depth(%d)\n", pFltAttr->u32FilterDepth);
	  	return -1;
	}

	return 0;
}

/*pu32FltId's value scope is 0-95 */
static HI_S32
hifilter_GetFreeFltId(HI_U32 u32DmxID, HI_U32 *pu32FltId)
{
	HIDEMUX_FILTERS *ChFilter;
	HI_U32 uRegionNum;
	int i;

	HIAPI_FILTERREGION(u32DmxID, uRegionNum);
	for (i = 0; i < NUM_HIPERREGION; i++)
	{
		ChFilter = &(g_HIFilters[(uRegionNum * NUM_HIPERREGION) + i]);
		if (ChFilter->u32UseFlag == 0)
		{
			*pu32FltId = (uRegionNum * NUM_HIPERREGION) + i;
			return 0;
		}
	}

	return -1;
}

static HI_U32
hifilter_GetChnFltNum(HI_S32 u32DmxID, HI_HANDLE uChHandle)
{
	HIDEMUX_FILTERS *ChFilter;
	HI_U32 uRegionNum;
	HI_U32 uFltNum = 0;
	int i = 0;

	HIAPI_FILTERREGION(u32DmxID, uRegionNum);
	for (i = 0; i < NUM_HIPERREGION; i++)
	{
		ChFilter = &(g_HIFilters[(uRegionNum * NUM_HIPERREGION) + i]);
		if (!ChFilter->u32UseFlag) continue;
		if ( ChFilter->hChannel == 0 || ChFilter->hChannel == INVALID_HIHANDLE) continue;
		if ( ChFilter->hChannel == uChHandle) uFltNum++;
	}

	return uFltNum;
}

/*get filter amount which channel is enable */
static HI_U32
hifilter_GetChnEnFltNum(HI_S32 u32DmxID, HI_HANDLE uChHandle)
{
	HIDEMUX_FILTERS *ChFilter;
	HI_U32 uRegionNum;
	HI_U32 uFltNum = 0;
	int i = 0;

	HIAPI_FILTERREGION(u32DmxID, uRegionNum);
	for (i = 0; i < NUM_HIPERREGION; i++)
	{
		ChFilter = &(g_HIFilters[(uRegionNum * NUM_HIPERREGION) + i]);
		if (!ChFilter->u32UseFlag || !ChFilter->u32EnableFlag) continue;
		if ( ChFilter->hChannel == 0 || ChFilter->hChannel == INVALID_HIHANDLE) continue;
		if ( ChFilter->hChannel == uChHandle) uFltNum++;
	}

	return uFltNum;
}

static HI_S32
hifilter_Create(HIDEMUX_ATTRIB *pFltAttr, HI_S32 *ps32FilterID)
{
	HI_UNF_DMX_FILTER_ATTR_S sFilterAttr;
	HI_UNF_DMX_CHAN_ATTR_S stChAttr;
	HIDEMUX_FILTERS *ChFilter;
	HI_HANDLE hFilter;
	HI_HANDLE hChannel;
	HI_U32 u32FltID;
	int hiReturn;

	HIPRT_INFO("mydemux:call hifilter_create\n");
	HIAPI_ISDEMUXINIT();
	if (hifilter_CheckFltAttr(pFltAttr) != 0)
	{
		HIPRT_ERROR("mydemux:hifilter_checkfltattr.invalid param\n");
		return HI_FAILURE;
	}

	if (hifilter_GetFreeFltId(pFltAttr->u32DMXID, &u32FltID) != 0)
	{
		HIPRT_ERROR("mydemux:hifilter_getfreefltid.no free\n");
		return HI_FAILURE;
	}

	memset(&sFilterAttr, 0, sizeof(HI_UNF_DMX_FILTER_ATTR_S));
	sFilterAttr.u32FilterDepth =  pFltAttr->u32FilterDepth;
	memcpy(sFilterAttr.au8Mask, 	pFltAttr->u8Mask,  NUM_HIFLTDEPTH);
	memcpy(sFilterAttr.au8Match, 	pFltAttr->u8Match, NUM_HIFLTDEPTH);
	memcpy(sFilterAttr.au8Negate, pFltAttr->u8Negate,NUM_HIFLTDEPTH);
	hiReturn = HI_UNF_DMX_CreateFilter(pFltAttr->u32DMXID, &sFilterAttr, &hFilter);
	if (HI_SUCCESS != hiReturn)
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_createfilter.error(%x)\n", hiReturn);
		return HI_FAILURE;
	}

	hiReturn = HI_UNF_DMX_GetChannelHandle(pFltAttr->u32DMXID, pFltAttr->u32PID, &hChannel);
	if ((HI_SUCCESS != hiReturn) && (HI_ERR_DMX_UNMATCH_CHAN != hiReturn))
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_getchannelhandle.error(%x)\n", hiReturn);
		HI_UNF_DMX_DestroyFilter(hFilter);
		return HI_FAILURE;
	}

	if ((HI_ERR_DMX_UNMATCH_CHAN == hiReturn) || (INVALID_HIHANDLE == hChannel))
	{
		/*do have channel for this PID,it need to create new channel */
		hiReturn = HI_UNF_DMX_GetChannelDefaultAttr(&stChAttr);
		if (HI_SUCCESS != hiReturn)
		{
			HIPRT_ERROR("mydemux:hi_unf_dmx_getchanneldefaultattr.error(%x)\n", hiReturn);
			HI_UNF_DMX_DestroyFilter(hFilter);
			return HI_FAILURE;
		}

		if (pFltAttr->u32CrcFlag == 1)
		{
			stChAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_FORCE_AND_DISCARD;
		}
		else
		if (pFltAttr->u32CrcFlag == 2)
		{
			stChAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
		}
		else
		{
			stChAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
		}

		if (pFltAttr->u32FilterType == 1)
		{
			stChAttr.enChannelType = HI_UNF_DMX_CHAN_TYPE_PES;
		}
		else
		if (pFltAttr->u32FilterType == 2)
		{
			stChAttr.enChannelType = HI_UNF_DMX_CHAN_TYPE_ECM_EMM;
		//	stChAttr.enChannelType = HI_UNF_DMX_CHAN_TYPE_POST;
		}
		else
		{
			stChAttr.enChannelType = HI_UNF_DMX_CHAN_TYPE_SEC;
		}

		stChAttr.enOutputMode = HI_UNF_DMX_CHAN_OUTPUT_MODE_PLAY;
		stChAttr.u32BufSize 	 = MAX_HIBUFFERSIZE; // not used, don't care

		HIPRT_INFO("mydemux:call hi_unf_dmx_createchannel\n");
		hiReturn = HI_UNF_DMX_CreateChannel(pFltAttr->u32DMXID, &stChAttr, &hChannel);
		if (HI_SUCCESS != hiReturn)
		{
			HI_UNF_DMX_DestroyFilter(hFilter);
			HIPRT_ERROR("mydemux:hi_unf_dmx_createchannel.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}

		hiReturn = HI_UNF_DMX_SetChannelPID(hChannel, pFltAttr->u32PID);
		if (HI_SUCCESS != hiReturn)
		{
			HI_UNF_DMX_DestroyFilter(hFilter);
			HI_UNF_DMX_DestroyChannel(hChannel);
			HIPRT_ERROR("mydemux:hi_unf_dmx_setchannelpid.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}
	}

	ChFilter = &g_HIFilters[u32FltID];
	memset(ChFilter, 0, sizeof(HIDEMUX_FILTERS));
	ChFilter->u32EnableFlag = 0;
	ChFilter->u32TimerCount = 0;
	ChFilter->hFilter  		= hFilter;
	ChFilter->hChannel 		= hChannel;
	ChFilter->u32UseFlag 	= 1;
	ChFilter->uFiltype  		= pFltAttr->u32Type;
	memcpy(&ChFilter->fltAttr, pFltAttr, sizeof(HIDEMUX_ATTRIB));

	*ps32FilterID = u32FltID;
	return HI_SUCCESS;
}

static HI_S32
hifilter_Destroy(HI_S32 s32FilterID)
{
	HIDEMUX_FILTERS *ChFilter;
	int hiReturn;

	HIPRT_INFO("mydemux:call hifilter_destroy\n");
	HIAPI_ISDEMUXINIT();
	HIAPI_CHECKFILTERID(s32FilterID);

	ChFilter = &g_HIFilters[s32FilterID];
	if ((INVALID_HIHANDLE == ChFilter->hFilter) || (INVALID_HIHANDLE == ChFilter->hChannel))
	{
		HIPRT_ERROR("mydemux:hifilter_destroy.error\n");
		return HI_FAILURE;
	}

	if (ChFilter->u32EnableFlag)
	{
		hiReturn = HI_UNF_DMX_DetachFilter(ChFilter->hFilter, ChFilter->hChannel);
		if (HI_SUCCESS != hiReturn)
		{
			HIPRT_ERROR("mydemux:hi_unf_dmx_detachfilter.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}

		ChFilter->u32EnableFlag = 0;
	}

	hiReturn = HI_UNF_DMX_DestroyFilter(ChFilter->hFilter);
	if (HI_SUCCESS != hiReturn)
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_destroyfilter.error(%x)\n", hiReturn);
		return HI_FAILURE;
	}

	ChFilter->u32UseFlag = 0;
	ChFilter->uFiltype   = 0;
	/*if the channel did not attach filter,destroy the channel*/
	if (!hifilter_GetChnFltNum(ChFilter->fltAttr.u32DMXID, ChFilter->hChannel))
	{
		HIPRT_INFO("mydemux:call hi_unf_dmx_destroychannel\n");
		hiReturn = HI_UNF_DMX_DestroyChannel(ChFilter->hChannel);
		if (HI_SUCCESS != hiReturn)
		{
			HIPRT_ERROR("mydemux:hi_unf_dmx_destroychannel.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}
	}

	ChFilter->hChannel = INVALID_HIHANDLE;
	ChFilter->hFilter  = INVALID_HIHANDLE;
	ChFilter->u32TimerCount = 0;
	return HI_SUCCESS;
}

static HI_S32
hifilter_SetAttribute(HI_S32 s32FilterID, HIDEMUX_ATTRIB *pFltAttr)
{
	HI_UNF_DMX_FILTER_ATTR_S sFilterAttr;
	HI_UNF_DMX_CHAN_ATTR_S sChanAttr;
	HIDEMUX_FILTERS *ChFilter;
	int hiReturn;

	HIAPI_ISDEMUXINIT();
	HIAPI_CHECKFILTERID(s32FilterID);
	if (hifilter_CheckFltAttr(pFltAttr) != 0)
	{
		HIPRT_ERROR("mydemux:hifilter_checkfltattr.invalid param\n");
		return HI_FAILURE;
	}

	ChFilter = &g_HIFilters[s32FilterID];
	if (pFltAttr->u32FilterType != ChFilter->fltAttr.u32FilterType)
	{
		HIPRT_ERROR("mydemux:hifilter_setattr.error type\n");
		return HI_FAILURE;
	}

	if ((INVALID_HIHANDLE == ChFilter->hFilter) || (INVALID_HIHANDLE == ChFilter->hChannel))
	{
		HIPRT_ERROR("mydemux:hifilter_setattr.error\n");
		return HI_FAILURE;
	}

	memset(&sFilterAttr, 0, sizeof(HI_UNF_DMX_FILTER_ATTR_S));
	sFilterAttr.u32FilterDepth = pFltAttr->u32FilterDepth;
	memcpy(sFilterAttr.au8Mask,  pFltAttr->u8Mask,   NUM_HIFLTDEPTH);
	memcpy(sFilterAttr.au8Match, pFltAttr->u8Match,  NUM_HIFLTDEPTH);
	memcpy(sFilterAttr.au8Negate,pFltAttr->u8Negate, NUM_HIFLTDEPTH);
	hiReturn = HI_UNF_DMX_SetFilterAttr(ChFilter->hFilter, &sFilterAttr);
	if (HI_SUCCESS != hiReturn)
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_setfilterattr.error(%x)\n", hiReturn);
		return HI_FAILURE;
	}

	if (ChFilter->fltAttr.u32CrcFlag != pFltAttr->u32CrcFlag)
	{
		hiReturn = HI_UNF_DMX_GetChannelAttr(ChFilter->hChannel, &sChanAttr);
		if (HI_SUCCESS != hiReturn)
		{
			HIPRT_ERROR("mydemux:hi_unf_dmx_getchannelattr.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}

		if (sChanAttr.enChannelType == HI_UNF_DMX_CHAN_TYPE_SEC ||
			 sChanAttr.enChannelType == HI_UNF_DMX_CHAN_TYPE_ECM_EMM)
		{
			if ((ChFilter->fltAttr.u32CrcFlag == 1)
				&& (sChanAttr.enCRCMode != HI_UNF_DMX_CHAN_CRC_MODE_FORCE_AND_DISCARD))
			{
				sChanAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_FORCE_AND_DISCARD;
				hiReturn  = HI_UNF_DMX_CloseChannel(ChFilter->hChannel);
				hiReturn |= HI_UNF_DMX_SetChannelAttr(ChFilter->hChannel, &sChanAttr);
				hiReturn |= HI_UNF_DMX_OpenChannel(ChFilter->hChannel);
			}
			else
			if ((ChFilter->fltAttr.u32CrcFlag == 2) && (sChanAttr.enCRCMode != HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD))
			{
				sChanAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
				hiReturn  = HI_UNF_DMX_CloseChannel(ChFilter->hChannel);
				hiReturn |= HI_UNF_DMX_SetChannelAttr(ChFilter->hChannel, &sChanAttr);
				hiReturn |= HI_UNF_DMX_OpenChannel(ChFilter->hChannel);
			}
			else
			if ((ChFilter->fltAttr.u32CrcFlag == 0) && (sChanAttr.enCRCMode != HI_UNF_DMX_CHAN_CRC_MODE_FORBID))
			{
				sChanAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
				hiReturn  = HI_UNF_DMX_CloseChannel(ChFilter->hChannel);
				hiReturn |= HI_UNF_DMX_SetChannelAttr(ChFilter->hChannel, &sChanAttr);
				hiReturn |= HI_UNF_DMX_OpenChannel(ChFilter->hChannel);
			}

			if (HI_SUCCESS != hiReturn)
			{
				 HIPRT_ERROR("mydemux:hifilter_setattr.crc.error(%x)\n", hiReturn);
				 return HI_FAILURE;
			}
		}
	}

	memcpy(&ChFilter->fltAttr, pFltAttr, sizeof(HIDEMUX_ATTRIB));
	return HI_SUCCESS;
}

static HI_S32
hifilter_GetAttr(HI_S32 s32FilterID, HIDEMUX_ATTRIB *pFltAttr)
{
	HIDEMUX_FILTERS *ChFilter;

	HIAPI_ISDEMUXINIT();
	HIAPI_CHECKFILTERID(s32FilterID);
	if (!pFltAttr)
	{
		HIPRT_ERROR("mydemux:hifilter_getattr.null\n");
		return HI_FAILURE;
	}

	ChFilter = &g_HIFilters[s32FilterID];
	memcpy(pFltAttr, &ChFilter->fltAttr, sizeof(HIDEMUX_ATTRIB));
	return HI_SUCCESS;
}

static HI_S32
hifilter_Start(HI_S32 s32FilterID)
{
	HI_UNF_DMX_CHAN_STATUS_S stStatus;
	HIDEMUX_FILTERS *ChFilter;
	int hiReturn;

	HIPRT_INFO("mydemux:call hifilter_start\n");
	HIAPI_ISDEMUXINIT();
	HIAPI_CHECKFILTERID(s32FilterID);

	ChFilter = &g_HIFilters[s32FilterID];
	if ((INVALID_HIHANDLE == ChFilter->hFilter) || (INVALID_HIHANDLE == ChFilter->hChannel))
	{
		HIPRT_ERROR("mydemux:hifilter_start.error\n");
		return HI_FAILURE;
	}

	if (ChFilter->u32EnableFlag)
	{
		HIPRT_ERROR("mydemux:hifilter_start.already\n");
		return HI_SUCCESS;
	}

	hiReturn = HI_UNF_DMX_AttachFilter(ChFilter->hFilter, ChFilter->hChannel);
	if (HI_SUCCESS != hiReturn)
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_attachfilter.error(%x)\n", hiReturn);
		return HI_FAILURE;
	}

	ChFilter->u32EnableFlag = 1;
	ChFilter->u32TimerCount = 0;
	hiReturn = HI_UNF_DMX_GetChannelStatus(ChFilter->hChannel, &stStatus);
	if (HI_SUCCESS != hiReturn)
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_getchannelstatus.error(%x)\n", hiReturn);
		return HI_FAILURE;
	}

	if (HI_UNF_DMX_CHAN_CLOSE == stStatus.enChanStatus)
	{
		hiReturn = HI_UNF_DMX_OpenChannel(ChFilter->hChannel);
		if (HI_SUCCESS != hiReturn)
		{
			HIPRT_ERROR("mydemux:hi_unf_dmx_openchannel.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}
	}
	return HI_SUCCESS;
}

static HI_S32
hifilter_Stop(HI_S32 s32FilterID)
{
	HI_UNF_DMX_CHAN_STATUS_S stStatus;
	HIDEMUX_FILTERS *ChFilter;
	int hiReturn;

	HIPRT_INFO("mydemux:call hifilter_stop!\n");
	HIAPI_ISDEMUXINIT();
	HIAPI_CHECKFILTERID(s32FilterID);

	ChFilter = &g_HIFilters[s32FilterID];
	if ((INVALID_HIHANDLE == ChFilter->hFilter) || (INVALID_HIHANDLE == ChFilter->hChannel))
	{
		HIPRT_ERROR("mydemux:hifilter_stop.error\n");
		return HI_FAILURE;
	}

	if (!ChFilter->u32EnableFlag)
	{
		HIPRT_ERROR("mydemux:hifilter_stop.already\n");
		return HI_SUCCESS;
	}

	hiReturn = HI_UNF_DMX_DetachFilter(ChFilter->hFilter, ChFilter->hChannel);
	if (HI_SUCCESS != hiReturn)
	{
		HIPRT_ERROR("mydemux:hi_unf_dmx_detachfilter.error(%x)\n", hiReturn);
		return HI_FAILURE;
	}

	ChFilter->u32EnableFlag = 0;
	ChFilter->u32TimerCount = 0;
	/*if the channel did not attach filter,close the channel*/
	if (!hifilter_GetChnEnFltNum(ChFilter->fltAttr.u32DMXID, ChFilter->hChannel))
	{
		HIPRT_INFO("mydemux:call hi_unf_dmx_closechannel\n");
		hiReturn = HI_UNF_DMX_GetChannelStatus(ChFilter->hChannel, &stStatus);
		if (HI_SUCCESS != hiReturn)
		{
			HIPRT_ERROR("mydemux:hi_unf_dmx_getchannelstatus.error(%x)\n", hiReturn);
			return HI_FAILURE;
		}

		if (HI_UNF_DMX_CHAN_CLOSE != stStatus.enChanStatus)
		{
			hiReturn = HI_UNF_DMX_CloseChannel(ChFilter->hChannel);
			if (HI_SUCCESS != hiReturn)
			{
				HIPRT_ERROR("mydemux:hi_unf_dmx_closechannel.error(%x)\n", hiReturn);
				return HI_FAILURE;
			}
		}
	}
	return HI_SUCCESS;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
static int32_t
hidemux_Start(int32_t adapter, int32_t dmuxid, int32_t type, int32_t n, int32_t pid, unsigned char *fltr, unsigned char *mask)
{
	HIDEMUX_ATTRIB FltAttr;
	HI_S32 hifltnum = INVALID_HIFILTER;
	int hiReturn;
	int i;

	FltAttr.dmuxid				= dmuxid;
	FltAttr.fltnum				= n;
	FltAttr.u32TUNERID		= TUNER0;
	FltAttr.u32DMXID			= HIDEMUX_OSCAM_ID;
	#if defined(SDKV600)
		if (adapter==DEMUX_SECONDADAPTER) {
			FltAttr.u32TUNERID= TUNER1;
			FltAttr.u32DMXID	= HIDEMUX_OSCAM2_ID;
		}
	#endif
	FltAttr.u32Type			= type;
	FltAttr.u32FilterType 	= 0;	// Section/ECM.EMM
	FltAttr.u32FilterType 	= (pid == CAT_PID) ? 0 : 2;
	FltAttr.u32PID				= pid;
	FltAttr.u32CrcFlag 		= 0;
	FltAttr.u32TimeOutMs		= 9000;

	memset(FltAttr.u8Match, 0x00,NUM_HIFLTDEPTH);
	memset(FltAttr.u8Mask,  0xFF,NUM_HIFLTDEPTH);
	memset(FltAttr.u8Negate,0x00,NUM_HIFLTDEPTH);
	FltAttr.u8Match[0] 		= fltr[0];
	FltAttr.u8Mask [0] 		= HIFLT_REVERSE_MASK(mask[0]);
	FltAttr.u32FilterDepth	= DVB_HIFLTBYTES;
	for (i=1; i<(int)FltAttr.u32FilterDepth; i++)
	{
		FltAttr.u8Match[i] 	= fltr[i];
		FltAttr.u8Mask [i] 	= HIFLT_REVERSE_MASK(mask[i]);
	}

	cs_log_dbg(D_DVBAPI, "hifilter %d(%d) start(pid:%04X)", n, type, pid);
	if (!IS_INVALID_PID(pid))
	{
		hiReturn = hifilter_Create(&FltAttr, &hifltnum);
		if (HI_SUCCESS != hiReturn)
		{
			g_dmuxes[dmuxid][n].activated = 0;
			HIPRT_ERROR("mydemux:hifilter %d(%d) failure", n, type);
		}
		else
		{
			hiReturn = hifilter_Start(hifltnum);
			if (HI_SUCCESS != hiReturn)
			{
				HIPRT_ERROR("mydemux:hifilter %d(%d) failure", n, type);
			}
			g_dmuxes[dmuxid][n].hifltnum  = hifltnum;
			g_dmuxes[dmuxid][n].activated = 1;
		}
	}
	HIPRT_INFO("mydemux:hifilter_start{%d:%d:%d,%04X}{%2d:%02X%02X%02X%02X%02X%02X%02X%02X}\n",
				dmuxid,
				type, n, pid,
				g_dmuxes[dmuxid][n].hifltnum,
				fltr[0],fltr[1],fltr[2],fltr[3],fltr[4],fltr[5],fltr[6],fltr[7]);
	g_dmuxes[dmuxid][n].adapter= adapter;
	g_dmuxes[dmuxid][n].dmuxid = dmuxid;
	g_dmuxes[dmuxid][n].pid	 	= pid;
	g_dmuxes[dmuxid][n].type	= type;
	g_dmuxes[dmuxid][n].flnum 	= n;
	return (n+1);
}

static int32_t
hidemux_Stop(int32_t adapter, int32_t dmuxid, int32_t type, int32_t n, int32_t pid)
{
	int hiReturn;

	if (n > MAX_FILTER-1) return 0;
	if (g_dmuxes[dmuxid][n].activated)
	{
		HIPRT_INFO("mydemux:hifilter_stop{%d:%d:%d,%04x}{%d}\n",
					dmuxid,
					type, n, pid,
					g_dmuxes[dmuxid][n].hifltnum);
		g_dmuxes[dmuxid][n].activated = 0;
//		if (g_dmuxes[dmuxid][n].dmuxid != dmuxid)
//		{
//			HIPRT_TRACE("mydemux:hidemux_filterclose dmuxid{%x,%x} invalid\n", g_dmuxes[dmuxid][n].dmuxid, dmuxid);
//			return 0;
//		}
//		if (pid  && g_dmuxes[dmuxid][n].pid != pid)
//		{
//			HIPRT_TRACE("mydemux:hidemux_filterclose pid{%x,%x} invalid\n", g_dmuxes[dmuxid][n].pid, pid);
//			return 0;
//		}
//		if (type && g_dmuxes[dmuxid][n].type != type)
//		{
//			HIPRT_TRACE("mydemux:hidemux_filterclose pid{%x,%x} invalid\n", g_dmuxes[dmuxid][n].pid, pid);
//			return 0;
//		}
		if (!IS_INVALID_HIFILTERS(g_dmuxes[dmuxid][n].hifltnum))
		{
			cs_log_dbg(D_DVBAPI, "hifilter %d(%d) stop(pid:%04X)", n, type, pid);
			hiReturn = hifilter_Stop(g_dmuxes[dmuxid][n].hifltnum);
			if (HI_SUCCESS != hiReturn)
			{
				HIPRT_INFO("hifilter_stop.failed\n");
				cs_log_dbg(D_DVBAPI, "hifilter %d(%d) failure{%x}", n, type, hiReturn);
			}

			hiReturn = hifilter_Destroy(g_dmuxes[dmuxid][n].hifltnum);
			if (HI_SUCCESS != hiReturn)
			{
				HIPRT_INFO("hifilter_destroy.failed\n");
				cs_log_dbg(D_DVBAPI, "hifilter %d(%d) failure{%x}", n, type, hiReturn);
			}
		}
	}
	g_dmuxes[dmuxid][n].adapter	= -1;
	g_dmuxes[dmuxid][n].dmuxid 	= -1;
	g_dmuxes[dmuxid][n].type		=  0;
	g_dmuxes[dmuxid][n].pid	 		=  0;
	g_dmuxes[dmuxid][n].flnum  	= -1;
	g_dmuxes[dmuxid][n].hifltnum 	= INVALID_HIFILTER;
	return 1;
}


static int
hidemux_Init(void)
{
	struct hidemux_thread_param *ecmpara, *emmpara;
	pthread_attr_t attr;
	int hiReturn;
	int ret;
	int dmuxid, n, i;

//	HIPRT_TRACE("mydemux:hidemux_init{%p,%p)\n", dvbApi_client, cur_client());
	HIPRT_TRACE("mydemux:hidemux_init{%d)\n", MAX_HIFILTERS);

	if (g_DemuxIniz) return 1;

	hiReturn = HI_UNF_DMX_Init();
	HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_Init);

	#if defined(SDKV600)
		#if 0
			// use tvheadend...
			/* 0,4 is PLAY DMX */
			hiReturn = HI_UNF_DMX_AttachTSPort(HIDEMUX_OSCAM1_ID/*demux id*/, HIPORT_TUNER_ID/*port id*/); //for section
			HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_AttachTSPort);
			// TUNER 1
			/* 0,4 is PLAY DMX */
			hiReturn = HI_UNF_DMX_AttachTSPort(HIDEMUX_OSCAM2_ID/*demux id*/, HIPORT_INTERNAL_ID/*port id*/); //for section
			HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_AttachTSPort);
		#endif
	#elif defined(SDKV500)
		// tvheadend
		#if 0
			HI_UNF_DMX_PORT_ATTR_S PortAttr;
			// TUNER 0
			hiReturn = HI_UNF_DMX_GetTSPortAttr(HIPORT_TUNER_ID, &PortAttr);
			HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_GetTSPortAttr);
			PortAttr.enPortType = HI_UNF_DMX_PORT_TYPE_SERIAL;
			#if defined(Hi3719MV100)
				PortAttr.u32SerialBitSelector = 1; //D0
			#else
				PortAttr.u32SerialBitSelector = 0; //D7
			#endif
			hiReturn = HI_UNF_DMX_SetTSPortAttr(HIPORT_TUNER_ID, &PortAttr);
			HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_SetTSPortAttr);
		#endif

		/* 0,4 is PLAY DMX */
		hiReturn = HI_UNF_DMX_AttachTSPort(HIDEMUX_OSCAM_ID/*demux id*/, HIPORT_TUNER_ID/*port id*/); //for section
		HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_AttachTSPort);
		// TUNER 1
		//	hiReturn = HI_UNF_DMX_GetTSPortAttr(HI_UNF_DMX_PORT_TSI_3, &PortAttr);
		//	HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_GetTSPortAttr);
		//	PortAttr.enPortType = HI_UNF_DMX_PORT_TYPE_SERIAL;
		//	PortAttr.u32SerialBitSelector = 0; //D7
		//	hiReturn = HI_UNF_DMX_SetTSPortAttr(HI_UNF_DMX_PORT_TSI_3, &PortAttr);
		//	HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_SetTSPortAttr);

		/* 0,4 is PLAY DMX */
	//	hiReturn = HI_UNF_DMX_AttachTSPort(HIDEMUX_DMX_ID2/*demux id*/, HI_UNF_DMX_PORT_TSI_3/*port id*/); //for section
	//	HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_AttachTSPort);
	#else
		hiReturn = HI_UNF_DMX_AttachTSPort(HIDEMUX_OSCAM_ID/*demux id*/, HIPORT_TUNER_ID/*port id*/); //for section
		HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_AttachTSPort);
	#endif

	for (dmuxid=0; dmuxid<MAX_DEMUX; dmuxid++)
	{
		for (n=0; n<MAX_FILTER; n++)
		{
			g_dmuxes[dmuxid][n].dmuxid		= -1;
			g_dmuxes[dmuxid][n].adapter	= -1;
			g_dmuxes[dmuxid][n].type	 	= 0;
			g_dmuxes[dmuxid][n].pid 	 	= 0;
			g_dmuxes[dmuxid][n].flnum 	 	= -1;
			g_dmuxes[dmuxid][n].hifltnum	= INVALID_HIFILTER;
			g_dmuxes[dmuxid][n].activated = 0;
		}
	}
	for (i=0; i<MAX_HIFILTERS; i++)
	{
		memset(&g_HIFilters[i], 0, sizeof(HIDEMUX_FILTERS));
		g_HIFilters[i].hChannel = INVALID_HIHANDLE;
		g_HIFilters[i].hFilter  = INVALID_HIHANDLE;
	}

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);

	if (!cs_malloc(&ecmpara, sizeof(struct hidemux_thread_param))) return 0;
//	ecmpara->flnum = HIDEMUX_OSCAM_ID;
	ecmpara->id  	 = TYPE_ECM;
	ecmpara->cli 	 = cur_client();
	ret = pthread_create(&g_FltEcmThread, NULL, hifilter_ThreadModules, (void *)ecmpara);
	if (ret) {
		pthread_attr_destroy(&attr);
		HIPRT_TRACE("hifilter_Init thread.errno(%d.%s)", ret, strerror(ret));
		return 0;
	}
	else {
		pthread_detach(g_FltEcmThread);
	}

	if (!cs_malloc(&emmpara, sizeof(struct hidemux_thread_param))) return 0;
	emmpara->id  = TYPE_EMM;
	emmpara->cli = cur_client();
	ret = pthread_create(&g_FltEmmThread, NULL, hifilter_ThreadModules, (void *)emmpara);
	if (ret) {
		pthread_attr_destroy(&attr);
		HIPRT_TRACE("hifilter_Init thread.errno(%d.%s)", ret, strerror(ret));
		return 0;
	}
	else {
		pthread_detach(g_FltEmmThread);
	}
	pthread_attr_destroy(&attr);
	g_DemuxRuning = HI_TRUE;
	if (hiReturn == HI_SUCCESS) g_DemuxIniz = 1;
	return (g_DemuxIniz);
}


static int
hidemux_DeInit(void)
{
	int hiReturn;
	int dmuxid;
	int n;

	HIPRT_TRACE("mydemux:hidemux_close\n");
	if (!g_DemuxIniz) return 1;

	for (dmuxid=0;dmuxid<MAX_DEMUX;dmuxid++)
	{
		for (n=0;n<MAX_FILTER;n++)
		{
			hidemux_Stop(-1, dmuxid, 0, n, 0);
		}
	}
	g_DemuxIniz = 0;
	g_DemuxRuning = HI_FALSE;
	usleep(100 * 1000);
	pthread_join(g_FltEcmThread, 0);
	pthread_join(g_FltEmmThread, 0);

//	hiReturn = HI_UNF_DMX_DetachTSPort(HIPORT_TUNER_ID/*port id*/);
//	HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_AttachTSPort);

	hiReturn = HI_UNF_DMX_DeInit();
	HIAPI_CHECKERROR(hiReturn,HI_UNF_DMX_DeInit);
	return 1;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
int32_t
hidemuxapi_AddFilters(int32_t adapter, int32_t dmuxid, int32_t type, int32_t n, int32_t pid, uchar *flt, uchar *mask)
{
	int status;

	if (!g_DemuxIniz) return -1;
	pthread_mutex_lock(&g_FltLocks);
	status = hidemux_Start(adapter, dmuxid, type, n, pid, flt, mask);
	pthread_mutex_unlock(&g_FltLocks);
	return status;
}

int32_t
hidemuxapi_RemoveFilters(int32_t adapter, int32_t dmuxid, int32_t type, int32_t n, int32_t pid)
{
	int status;

	if (!g_DemuxIniz) return -1;
	pthread_mutex_lock(&g_FltLocks);
	status = hidemux_Stop(adapter, dmuxid, type, n, pid);
	pthread_mutex_unlock(&g_FltLocks);
	return status;
}



/* write cw to all demuxes in mask with passed index */
int32_t
hidemuxapi_Init(void)
{
	pthread_mutex_init(&g_FltLocks, NULL);
	pthread_mutex_init(&g_FltMutex, NULL);
	//
	//
	// link option -lhi_common
	// for disable debug message
	HI_SYS_Init();
	#if defined(SDKV500) || defined(SDKV600)
		HI_SYS_SetLogLevel(HI_ID_DEMUX,HI_LOG_LEVEL_ERROR);
	#else
		HI_SYS_SetLogLevel(HI_DEBUG_ID_DEMUX,HI_LOG_LEVEL_ERROR);
	#endif
	//
	//
	//
	hidemux_Init();
	atexit(hidemuxapi_Deinit);
	return 1;
}

void
hidemuxapi_Deinit(void)
{
	hidemux_DeInit();
	//
	//
	//
	HI_SYS_DeInit();
	//
	//
	//
	pthread_mutex_destroy(&g_FltLocks);
	pthread_mutex_destroy(&g_FltMutex);
}

#endif	// #if defined(WITH_HISILICON)
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

