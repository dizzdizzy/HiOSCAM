/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : drv_vdec_ioctl.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/05/17
  Description   : 
  History       :
  1.Date        : 2006/05/17
    Author      : g45345
    Modification: Created file
  2.Date        : 2013/04/03
    Author      : l00185424
    Modification: Modify file name, hi_drv_vdec.h to drv_vdec_ioctl.h

******************************************************************************/
#ifndef __DRV_VDEC_IOCTL_H__
#define __DRV_VDEC_IOCTL_H__

#include "hi_unf_common.h"
#include "hi_unf_avplay.h"
#include "hi_mpi_vdec.h"
#include "hi_debug.h"
#include "hi_drv_vdec.h"
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef HI_DRV_VDEC_STREAM_BUF_S VDEC_CMD_CREATEBUF_S;

typedef struct hiVDEC_CMD_BUF_S
{
    HI_HANDLE           hHandle;
    VDEC_ES_BUF_S       stBuf;
}VDEC_CMD_BUF_S;

typedef struct hiVDEC_CMD_BUF_USERADDR_S
{
    HI_HANDLE           hHandle;
    HI_U32              u32UserAddr;
}VDEC_CMD_BUF_USERADDR_S;

typedef struct hiVDEC_CMD_BUF_STATUS_S
{
    HI_HANDLE               hHandle;
    HI_DRV_VDEC_STREAMBUF_STATUS_S stStatus;
}VDEC_CMD_BUF_STATUS_S;

typedef struct hiVDEC_CMD_ALLOC_S
{
    HI_HANDLE                   hHandle;
    HI_UNF_AVPLAY_OPEN_OPT_S    stOpenOpt;
    HI_U32                      u32DFSEnable;//l00273086
}VDEC_CMD_ALLOC_S;

typedef struct hiVDEC_CMD_RESET_S
{
    HI_HANDLE           hHandle;
    HI_DRV_VDEC_RESET_TYPE_E   enType;
}VDEC_CMD_RESET_S;

typedef struct hiVDEC_CMD_ATTR_S
{
    HI_HANDLE               hHandle;
    HI_UNF_VCODEC_ATTR_S    stAttr;
}VDEC_CMD_ATTR_S;

typedef struct hiVDEC_CMD_ATTACH_BUF_S
{
    HI_HANDLE               hHandle;
    HI_U32                  u32BufSize;
    HI_HANDLE               hDmxVidChn;
    HI_HANDLE               hStrmBuf;
}VDEC_CMD_ATTACH_BUF_S;

typedef struct hiVDEC_CMD_USERDATA_S
{
    HI_HANDLE               hHandle;
    HI_UNF_VIDEO_USERDATA_S stUserData;
}VDEC_CMD_USERDATA_S;

typedef struct hiVDEC_CMD_USERDATA_ACQMODE_S
{
    HI_HANDLE               hHandle;
    HI_UNF_VIDEO_USERDATA_S stUserData;
    HI_UNF_VIDEO_USERDATA_TYPE_E enType;
}VDEC_CMD_USERDATA_ACQMODE_S;

typedef struct hiVDEC_CMD_STATUS_S
{
    HI_HANDLE               hHandle;
    VDEC_STATUSINFO_S       stStatus;
}VDEC_CMD_STATUS_S;

typedef struct hiVDEC_CMD_STREAM_INFO_S
{
    HI_HANDLE                   hHandle;
    HI_UNF_VCODEC_STREAMINFO_S  stInfo;
}VDEC_CMD_STREAM_INFO_S;

typedef struct hiVDEC_CMD_EVENT_S
{
    HI_HANDLE               hHandle;
    VDEC_EVENT_S            stEvent;
} VDEC_CMD_EVENT_S;

typedef struct hiVDEC_CMD_FRAME_S
{
    HI_HANDLE               hHandle;
    HI_DRV_VIDEO_FRAME_S stFrame;
} VDEC_CMD_FRAME_S;

typedef struct hiVDEC_CMD_IFRAME_DEC_S
{
    HI_HANDLE               hHandle;
    HI_UNF_AVPLAY_I_FRAME_S stIFrame;
    HI_DRV_VIDEO_FRAME_S stVoFrameInfo;
    HI_BOOL                 bCapture;
}VDEC_CMD_IFRAME_DEC_S;

typedef struct hiVDEC_CMD_IFRAME_RLS_S
{
    HI_HANDLE               hHandle;
    HI_DRV_VIDEO_FRAME_S stVoFrameInfo;
}VDEC_CMD_IFRAME_RLS_S;

typedef struct hiVDEC_CMD_FRAME_RATE_S
{
    HI_HANDLE               hHandle;
    HI_UNF_AVPLAY_FRMRATE_PARAM_S stFrameRate;
}VDEC_CMD_FRAME_RATE_S;

typedef struct hiVDEC_CMD_GET_FRAME_S
{
    HI_HANDLE               hHandle;
    HI_DRV_VDEC_FRAME_BUF_S        stFrame;
}VDEC_CMD_GET_FRAME_S;

typedef struct hiVDEC_CMD_PUT_FRAME_S
{
    HI_HANDLE               hHandle;
    HI_DRV_VDEC_USR_FRAME_S        stFrame;
}VDEC_CMD_PUT_FRAME_S;

typedef struct hiVDEC_CMD_VO_FRAME_S
{
    HI_HANDLE               hHandle;
    HI_DRV_VIDEO_FRAME_S stFrame;
}VDEC_CMD_VO_FRAME_S;

typedef struct hiVDEC_CMD_DISCARD_FRAME_S
{
    HI_HANDLE               hHandle;
    VDEC_DISCARD_FRAME_S    stDiscardOpt;
}VDEC_CMD_DISCARD_FRAME_S;

typedef struct hiVDEC_CMD_ATTACHHDL_S
{
    HI_HANDLE               hHandle;
    HI_HANDLE               hVdec;
}VDEC_CMD_ATTACHHDL_S;

typedef struct hiVDEC_CMD_USERDATABUF_S
{
    HI_HANDLE           hHandle;        
    HI_DRV_VDEC_USERDATABUF_S  stBuf;
}VDEC_CMD_USERDATABUF_S;

typedef struct hiVDEC_CMD_SET_TPLAY_OPT_S
{
    HI_HANDLE               hHandle;
    HI_UNF_AVPLAY_TPLAY_OPT_S stTPlayOpt;
}VDEC_CMD_TRICKMODE_OPT_S;

typedef struct hiVDEC_CMD_SET_CTRL_INFO_S
{
    HI_HANDLE               hHandle;
    HI_UNF_AVPLAY_CONTROL_INFO_S stCtrlInfo;
}VDEC_CMD_SET_CTRL_INFO_S;

typedef struct hiVDEC_CMD_SET_PROGRESSIVE_S
{
    HI_HANDLE  hHandle;
	HI_BOOL    bProgressive;           
}VDEC_CMD_SET_PROGRESSIVE_S;

typedef struct hiVDEC_CMD_SET_DPBFULL_CTRL_S
{
    HI_HANDLE  hHandle;
	HI_BOOL    bDPBFullCtrl;           
}VDEC_CMD_SET_DPBFULL_CTRL_S;

typedef struct hiVDEC_CMD_SET_COLORSPACE_S
{
    HI_HANDLE  hHandle;
    HI_U32    u32ColorSpace;
}VDEC_CMD_SET_COLORSPACE_S;

typedef struct hiVDEC_CMD_SET_LOWDELAY_S
{
    HI_HANDLE  hHandle;
	HI_BOOL    bLowdelay;           
}VDEC_CMD_SET_LOWDELAY_S;
#ifdef HI_TVP_SUPPORT
typedef struct hiVDEC_CMD_SET_TVP_S
{
    HI_HANDLE  hHandle;
	HI_BOOL    bTVP;           
}VDEC_CMD_SET_TVP_S;
#endif
//add by l00225186
typedef struct tagVDEC_VPSS_PARAM_S
{
    HI_HANDLE* phVpss;
}VDEC_VPSS_PARAM_S;


typedef struct hiVDEC_CMD_VPSS_FRAME_S
{
    HI_HANDLE                       hHandle;
    VDEC_VPSS_PARAM_S               stVpssParam;
	HI_HANDLE                       hPort;
	VDEC_PORT_TYPE_E                enPortType;
    HI_DRV_VIDEO_FRAME_PACKAGE_S    stFrame;
	VDEC_PORT_PARAM_S               stPortParam;
	VDEC_FRMSTATUSINFO_S            stVdecFrmStatusInfo;    
	HI_UNF_VIDEO_FRAME_PACKING_TYPE_E   eFramePackType;
	VDEC_PORT_ABILITY_E             ePortAbility;
	HI_BOOL                         bAllPortComplete;
	HI_DRV_VPSS_PORT_CFG_S          stPortCfg;
    HI_DRV_VIDEO_FRAME_S            *pVideoFrame;
	VDEC_BUFFER_ATTR_S              stBufferAttr;
}VDEC_CMD_VPSS_FRAME_S;

typedef struct hiVDEC_CMD_SEEKPTS_S
{
    HI_HANDLE                       hHandle;
	HI_U32*                         pu32SeekPts;
	HI_U32                          u32Gap;
}VDEC_CMD_SEEK_PTS_S;

typedef struct hiVDEC_CMD_VPU_BUF_CREATE_S
{
	HI_U32 u32StartVirAddr;
    HI_U32 u32StartPhyAddr;
    HI_U32 u32Size;
}VDEC_CMD_VPU_BUF_CREATE_S;
typedef struct hiVDEC_CMD_VPU_GET_FRAME_S
{
    HI_HANDLE                   hHandle;
    HI_DRV_VDEC_FRAME_BUF_S     stFrame;
}VDEC_CMD_VPU_GET_FRAME_S;
typedef struct hiVDEC_CMD_VPU_PUT_FRAME_S
{
    HI_HANDLE                      hHandle;
    HI_DRV_VDEC_USR_FRAME_S        stFrame;
}VDEC_CMD_VPU_PUT_FRAME_S;
typedef struct hiVDEC_CMD_CREATE_FRAME_LIST_S
{
    HI_HANDLE                      hHandle;
}VDEC_CMD_CREATE_FRAME_LIST_S;
typedef struct hiVDEC_CMD_VPU_GET_VPSS_STATUSINFO_S
{
	HI_HANDLE					   hHandle;
	HI_BOOL                        bAllPortCompleteFrm;
}VDEC_CMD_VPU_GET_VPSS_STATUSINFO_S;
typedef struct hiVDEC_CMD_RELEASE_FRAME_LIST_S
{
    HI_HANDLE                      hHandle;
}VDEC_CMD_RELEASE_FRAME_LIST_S;
typedef struct hiVDEC_CMD_VPU_ATTR_S
{
    HI_HANDLE                      hHandle;
    VDEC_VPU_ATTR_S                stVPUAttr;
}VDEC_CMD_VPU_ATTR_S;
typedef struct hiVDEC_CMD_VPU_CHECK_RLSFRAME_S
{
    HI_HANDLE                      hHandle;
    HI_S32                         as32FrameID[HI_VDEC_MAX_VPU_FRAME_NUM];
	HI_S32                         s32Count;
}VDEC_CMD_VPU_CHECK_RLSFRAME_S;
typedef struct hiVDEC_CMD_SET_BUFFERMODE_S
{
    HI_HANDLE                       hHandle;
	VDEC_FRAMEBUFFER_MODE_E         enFrameBufferMode;
}VDEC_CMD_SET_BUFFERMODE_S;
typedef struct hiVDEC_CMD_CHECKANDDELBUFFER_S
{
    HI_HANDLE                       hHandle;
	VDEC_BUFFER_INFO_S              stBufInfo;
}VDEC_CMD_CHECKANDDELBUFFER_S;
typedef struct hiVDEC_CMD_SETEXTBUFFERSTATE_S
{
    HI_HANDLE                       hHandle;
	VDEC_EXTBUFFER_STATE_E          enExtBufferState;
}VDEC_CMD_SETEXTBUFFERTATE_S;

typedef struct hiVDEC_CMD_SETRESOLUTION_S
{
    HI_HANDLE                       hHandle;
	VDEC_RESOLUTION_ATTR_S          stResolution;
}VDEC_CMD_SETRESOLUTION_S;
typedef struct hiVDEC_CMD_VPU_PROC_HANDLE_S
{
    HI_HANDLE                      hVdecHandle;
    HI_HANDLE                      hVpuHandle;
}VDEC_CMD_VPU_PROC_HANDLE_S;

typedef struct hiVDEC_CMD_VPU_PROC_STATUS_S
{
    HI_HANDLE                      hVdecHandle;
    HI_DRV_VDEC_VPU_STATUS_S       stVPUStatus;
}VDEC_CMD_VPU_PROC_STATUS_S;
/* 0x00 - 0x1F: For vdec global */
/* 0x20 - 0x3F: For stream buffer */
/* 0x40 - 0x5F: Reserve for frame buffer */
/* 0x60 - 0x7F: For vfmw codec instance basic operation */
/* 0x80 - 0x9F: For vfmw codec instance special operation */
/* 0xA0 - 0xFF: Reserve for use */
#define UMAPC_VDEC_GETCAP               _IOR (HI_ID_VDEC, 0x00, VDEC_CAP_S)
#define UMAPC_VDEC_ALLOCHANDLE          _IOR (HI_ID_VDEC, 0x01, HI_U32)
#define UMAPC_VDEC_FREEHANDLE           _IOW (HI_ID_VDEC, 0x02, HI_U32)

#define UMAPC_VDEC_CREATE_ESBUF         _IOWR(HI_ID_VDEC, 0x20, VDEC_CMD_CREATEBUF_S)
#define UMAPC_VDEC_DESTROY_ESBUF        _IOW (HI_ID_VDEC, 0x21, HI_U32)
#define UMAPC_VDEC_GETBUF 	            _IOWR(HI_ID_VDEC, 0x22, VDEC_CMD_BUF_S)
#define UMAPC_VDEC_PUTBUF 	            _IOW (HI_ID_VDEC, 0x23, VDEC_CMD_BUF_S)
#define UMAPC_VDEC_SETUSERADDR          _IOW (HI_ID_VDEC, 0x24, VDEC_CMD_BUF_USERADDR_S)
#define UMAPC_VDEC_RCVBUF               _IOWR(HI_ID_VDEC, 0x25, VDEC_CMD_BUF_S)
#define UMAPC_VDEC_RLSBUF               _IOWR(HI_ID_VDEC, 0x26, VDEC_CMD_BUF_S)
#define UMAPC_VDEC_RESET_ESBUF          _IOWR(HI_ID_VDEC, 0x27, HI_U32)
#define UMAPC_VDEC_GET_ESBUF_STATUS     _IOWR(HI_ID_VDEC, 0x28, VDEC_CMD_BUF_STATUS_S)

#define UMAPC_VDEC_CHAN_ALLOC  	        _IOWR(HI_ID_VDEC, 0x60, VDEC_CMD_ALLOC_S)
#define UMAPC_VDEC_CHAN_FREE  	        _IOW (HI_ID_VDEC, 0x61, HI_U32)
#define UMAPC_VDEC_CHAN_START           _IOW (HI_ID_VDEC, 0x62, HI_U32)
#define UMAPC_VDEC_CHAN_STOP            _IOW (HI_ID_VDEC, 0x63, HI_U32)
#define UMAPC_VDEC_CHAN_RESET           _IOW (HI_ID_VDEC, 0x64, VDEC_CMD_RESET_S)
#define UMAPC_VDEC_CHAN_SETATTR         _IOW (HI_ID_VDEC, 0x65, VDEC_CMD_ATTR_S)
#define UMAPC_VDEC_CHAN_GETATTR         _IOWR(HI_ID_VDEC, 0x66, VDEC_CMD_ATTR_S)
#define UMAPC_VDEC_CHAN_ATTACHBUF       _IOW (HI_ID_VDEC, 0x67, VDEC_CMD_ATTACH_BUF_S)
#define UMAPC_VDEC_CHAN_DETACHBUF       _IOW (HI_ID_VDEC, 0x68, HI_U32)
#define UMAPC_VDEC_CHAN_SETEOSFLAG      _IOWR(HI_ID_VDEC, 0x69, HI_U32)
#define UMAPC_VDEC_CHAN_DISCARDFRM      _IOWR(HI_ID_VDEC, 0x6a, VDEC_CMD_DISCARD_FRAME_S)

#define UMAPC_VDEC_CHAN_USRDATA 	    _IOWR(HI_ID_VDEC, 0x80, VDEC_CMD_USERDATA_S)
#define UMAPC_VDEC_CHAN_STATUSINFO      _IOWR(HI_ID_VDEC, 0x81, VDEC_CMD_STATUS_S)
#define UMAPC_VDEC_CHAN_STREAMINFO      _IOWR(HI_ID_VDEC, 0x82, VDEC_CMD_STREAM_INFO_S)
#define UMAPC_VDEC_CHAN_CHECKEVT        _IOWR(HI_ID_VDEC, 0x83, VDEC_CMD_EVENT_S)
#define UMAPC_VDEC_CHAN_EVNET_NEWFRAME  _IOWR(HI_ID_VDEC, 0x84, VDEC_CMD_FRAME_S)
#define UMAPC_VDEC_CHAN_IFRMDECODE      _IOWR(HI_ID_VDEC, 0x85, VDEC_CMD_IFRAME_DEC_S)
#define UMAPC_VDEC_CHAN_IFRMRELEASE     _IOW (HI_ID_VDEC, 0x8c, VDEC_CMD_IFRAME_RLS_S)

#define UMAPC_VDEC_CHAN_SETFRMRATE      _IOW (HI_ID_VDEC, 0x86, VDEC_CMD_FRAME_RATE_S)
#define UMAPC_VDEC_CHAN_GETFRMRATE      _IOWR(HI_ID_VDEC, 0x87, VDEC_CMD_FRAME_RATE_S)
#define UMAPC_VDEC_CHAN_GETFRM          _IOWR(HI_ID_VDEC, 0x88, VDEC_CMD_GET_FRAME_S) 
#define UMAPC_VDEC_CHAN_PUTFRM          _IOWR(HI_ID_VDEC, 0x89, VDEC_CMD_PUT_FRAME_S)  
#define UMAPC_VDEC_CHAN_RLSFRM 	        _IOW (HI_ID_VDEC, 0x8a, VDEC_CMD_VO_FRAME_S)
#define UMAPC_VDEC_CHAN_RCVFRM 	        _IOWR(HI_ID_VDEC, 0x8b, VDEC_CMD_VO_FRAME_S)
/* 0x8c is UMAPC_VDEC_CHAN_IFRMRELEASE */
#define UMAPC_VDEC_CHAN_SETTRICKMODE    _IOW (HI_ID_VDEC, 0x8d, VDEC_CMD_TRICKMODE_OPT_S)

/* For CC acquire user data mode */
#define UMAPC_VDEC_CHAN_USERDATAINITBUF _IOWR(HI_ID_VDEC, 0x8e, VDEC_CMD_USERDATABUF_S)
#define UMAPC_VDEC_CHAN_USERDATASETBUFADDR  _IOW (HI_ID_VDEC, 0x8f, VDEC_CMD_BUF_USERADDR_S)
#define UMAPC_VDEC_CHAN_ACQUSERDATA     _IOWR(HI_ID_VDEC, 0x90, VDEC_CMD_USERDATA_ACQMODE_S)
#define UMAPC_VDEC_CHAN_RLSUSERDATA     _IOW (HI_ID_VDEC, 0x91, VDEC_CMD_USERDATA_S)

#define UMAPC_VDEC_CHAN_SETCTRLINFO     _IOW (HI_ID_VDEC, 0x92, VDEC_CMD_SET_CTRL_INFO_S)
#define UMAPC_VDEC_CHAN_PROGRSSIVE      _IOW (HI_ID_VDEC, 0x93, VDEC_CMD_SET_PROGRESSIVE_S)
#define UMAPC_VDEC_CHAN_DPBFULL      _IOWR (HI_ID_VDEC, 0x94, VDEC_CMD_SET_DPBFULL_CTRL_S)
#define UMAPC_VDEC_CHAN_SETCOLORSPACE      _IOWR (HI_ID_VDEC, 0x95, VDEC_CMD_SET_COLORSPACE_S)

//add by l00225186
#define UMAPC_VDEC_ALLOCCHANENTITY      _IOWR(HI_ID_VDEC, 0xa0, HI_U32)
#define UMAPC_VDEC_CHAN_RCVVPSSFRM      _IOWR(HI_ID_VDEC, 0xa1, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_CREATEPORT      _IOWR(HI_ID_VDEC, 0xa2, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_DESTROYPORT     _IOWR(HI_ID_VDEC, 0xa3, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_GETPORTPARAM    _IOWR(HI_ID_VDEC, 0xa4, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_CREATEVPSS      _IOWR(HI_ID_VDEC, 0xa5, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_ENABLEPORT      _IOWR(HI_ID_VDEC, 0xa6, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_DISABLEPORT     _IOWR(HI_ID_VDEC, 0xa7, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_RESETVPSS       _IOWR(HI_ID_VDEC, 0xa8, HI_U32)
#define UMAPC_VDEC_CHAN_GETFRMSTATUSINFO  _IOWR(HI_ID_VDEC, 0xa9, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_SETPORTTYPE      _IOWR(HI_ID_VDEC, 0xaa, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_CANCLEMAINPORT     _IOWR(HI_ID_VDEC, 0xab, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_SETFRMPACKTYPE    _IOWR(HI_ID_VDEC, 0xac, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_GETFRMPACKTYPE    _IOWR(HI_ID_VDEC, 0xad, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_SENDEOS            _IOWR(HI_ID_VDEC, 0xae, HI_U32)
#define UMAPC_VDEC_CHAN_GETPORTSTATE      _IOWR(HI_ID_VDEC, 0xaf, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_DESTORYVPSS      _IOWR(HI_ID_VDEC, 0xb0, VDEC_CMD_VPSS_FRAME_S)

#define UMAPC_VDEC_CHAN_GETPORTATTR      _IOWR(HI_ID_VDEC, 0xb1, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_SETPORTATTR      _IOWR(HI_ID_VDEC, 0xb2, VDEC_CMD_VPSS_FRAME_S)

#define UMAPC_VDEC_CHAN_RLSPORTFRM      _IOWR(HI_ID_VDEC, 0xb3, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_LOWDELAY      _IOW (HI_ID_VDEC, 0xb4, VDEC_CMD_SET_LOWDELAY_S)
#define UMAPC_VDEC_CHAN_SEEKPTS       _IOWR (HI_ID_VDEC, 0xb5, VDEC_CMD_SEEK_PTS_S)
#define UMAPC_VDEC_CHAN_CREATE_VPU_BUF   _IOWR (HI_ID_VDEC, 0xb6, VDEC_CMD_VPU_BUF_CREATE_S) 
#define UMAPC_VDEC_CHAN_VPU_REVERT_FRAME_BUF   _IOWR(HI_ID_VDEC, 0xb7, HI_U32*) 
#define UMAPC_VDEC_CHAN_VPU_CREATE_FRAMELIST   _IOWR(HI_ID_VDEC, 0xb8, VDEC_CMD_CREATE_FRAME_LIST_S) 
#define UMAPC_VDEC_CHAN_VPU_RELEASE_FRAMELIST   _IOWR(HI_ID_VDEC, 0xb9, VDEC_CMD_RELEASE_FRAME_LIST_S) 
#define UMAPC_VDEC_CHAN_VPU_PUT_FRAME   _IOWR(HI_ID_VDEC, 0xba, VDEC_CMD_VPU_PUT_FRAME_S) 
#define UMAPC_VDEC_CHAN_VPU_ATTR   _IOWR(HI_ID_VDEC, 0xbb, VDEC_CMD_VPU_ATTR_S)
#define UMAPC_VDEC_CHAN_VPU_CHECK_RLSFRM   _IOWR(HI_ID_VDEC, 0xbc, VDEC_CMD_VPU_CHECK_RLSFRAME_S)

#define UMAPC_VDEC_CHAN_VPU_START           _IOW (HI_ID_VDEC, 0xbd, HI_U32)
#define UMAPC_VDEC_CHAN_VPU_STOP            _IOW (HI_ID_VDEC, 0xbe, HI_U32)
#define UMAPC_VDEC_VPU_PTS_Alloc           _IOW (HI_ID_VDEC, 0xbf, VDEC_CMD_VPU_PROC_HANDLE_S) // modified by w00278582
#define UMAPC_VDEC_VPU_PTS_Free            _IOW (HI_ID_VDEC, 0xc0, VDEC_CMD_VPU_PROC_HANDLE_S) // modified by w00278582
#define UMAPC_VDEC_VPU_PTS_Start           _IOW (HI_ID_VDEC, 0xc1, HI_U32)
#define UMAPC_VDEC_VPU_PTS_Stop            _IOW (HI_ID_VDEC, 0xc2, HI_U32)
#define UMAPC_VDEC_VPU_PTS_Reset           _IOW (HI_ID_VDEC, 0xc3, HI_U32)
#define UMAPC_VDEC_VPU_GET_FRAME_RATE      _IOW (HI_ID_VDEC, 0xc4, VDEC_CMD_VO_FRAME_S)
#define UMAPC_VDEC_VPU_GET_VPSS_STATUSINFO    _IOWR (HI_ID_VDEC, 0xc5, VDEC_CMD_VPU_GET_VPSS_STATUSINFO_S)
// add by w00278582
#define UMAPC_VDEC_VPU_PROC                 _IOWR(HI_ID_VDEC, 0xc6, VDEC_CMD_VPU_PROC_STATUS_S)

#define UMAPC_VDEC_CHAN_SETEXTBUFFER      _IOWR(HI_ID_VDEC, 0xd0, VDEC_CMD_VPSS_FRAME_S)
#define UMAPC_VDEC_CHAN_SETBUFFERMODE     _IOWR(HI_ID_VDEC, 0xd1, VDEC_CMD_SET_BUFFERMODE_S)
#define UMAPC_VDEC_CHAN_CHECKANDDELBUFFER     _IOWR(HI_ID_VDEC, 0xd2, VDEC_CMD_CHECKANDDELBUFFER_S)
#define UMAPC_VDEC_CHAN_SETEXTBUFFERSTATE     _IOWR(HI_ID_VDEC, 0xd3, VDEC_CMD_SETEXTBUFFERTATE_S)
#define UMAPC_VDEC_CHAN_SETRESOLUTION     _IOWR(HI_ID_VDEC, 0xd4, VDEC_CMD_SETRESOLUTION_S)

#ifdef HI_TVP_SUPPORT
#define UMAPC_VDEC_CHAN_TVP           _IOW (HI_ID_VDEC, 0xb6, VDEC_CMD_SET_TVP_S)
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __DRV_VDEC_IOCTL_H__ */
