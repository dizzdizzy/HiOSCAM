#ifndef NDPT_EXT
#define NDPT_EXT

#include "hi_type.h"

#include "hi_drv_ndpt.h"

/*create link according to net parameter, used to receive and send RTP/RTCP data. CNcomment:根据网络参数创建链接，用于发送和接收RTP/RTCP数据。
input parameter:
     pNetPara:struct pointer of net communication protocol parameter,NDPT_NET_CONFIG_PARA_S CNcomment:RTP流网络通信协议参数结构体指针，NDPT_NET_CONFIG_PARA_S
return:
     0:failure CNcomment:失败
     other:handle of net adapter layer channel CNcomment:其他 网络适配层通道句柄
*/
extern HI_U32 HI_DRV_NDPT_CreateLink(HI_VOID *pNetCfgPara);

/*
modify parameter of net adapter layer channel CNcomment:修改网络适配通道参数。
input parameter:
      handle:handle of net adapter layer channel CNcomment:网络适配层通道句柄。
      pstNetPara:struct pointer of net parameter config, NDPT_NET_CONFIG_PARA_S CNcomment:网络参数配置结构体指针，NDPT_NET_CONFIG_PARA_S ；
return:
      0:success CNcomment:成功
      other:failure CNcomment:其他 失败
*/
extern HI_S32 HI_DRV_NDPT_ModifyNetPara(HI_U32 handle,HI_VOID * pNetCfgPara);

/*
cancellation link interface CNcomment:注销链路接口：
input parameter:
      handle:handle of net adapter layer channel CNcomment:网络适配层通道句柄。
return:
      0:success CNcomment:成功
      other:failure CNcomment:其他 失败
*/
extern HI_S32 HI_DRV_NDPT_DestroyLink(HI_U32 handle);


/*
sent-received callbacks function of register data from RTP to net adapter CNcomment:RTP向网络适配注册数据接收回调函数
input parameter:
      handle:handle of net adapter layer channel CNcomment:网络适配层通道句柄。
      TransId:RTP layer channel ID,return when net adapter call RTP layer's sent-received callbacks CNcomment:RTP层通道ID，网络适配调用RTP层接收回调函数时回传。
      funptr:receive callbacks function pointer CNcomment: CNcomment:接收回调函数指针；
return:
      0:success CNcomment:成功；
      other:failure CNcomment:其他 失败；
*/
extern HI_S32 HI_DRV_NDPT_RevFun(HI_U32 handle, HI_U32 TransId, ndpt_rtp_revfrom funptr);

/*
send RTP/RTCP data CNcomment:发送RTP/RTCP数据。
input parameter:
      handle:handle of net adapter layer channel CNcomment:网络适配层通道句柄。
      even_odd:port mode CNcomment:端口模式。
      data_buffer:data struct pointer CNcomment:数据结构体指针。
return:
      0:success CNcomment:成功；
      other:failure CNcomment:其他 失败；
*/
extern HI_S32 HI_DRV_NDPT_SendRtp(HI_U32 handle, HI_U32 even_odd, RTP_NET_BUFFER_STRU *data_buffer);


typedef HI_S32 (*FN_NDPT_RegRevfun)(HI_U32, HI_U32, ndpt_rtp_revfrom);
typedef HI_S32 (*FN_NDPT_RtpSendto)(HI_U32, HI_U32, RTP_NET_BUFFER_STRU *);
typedef HI_U32 (*FN_NDPT_CreateLink)(HI_VOID *);
typedef HI_S32 (*FN_NDPT_ModifyNetPara)(HI_U32, HI_VOID *);
typedef HI_S32 (*FN_NDPT_DestroyLink)(HI_U32);
	
typedef struct
{
    FN_NDPT_RegRevfun       pfnNdptRegRevfun;
    FN_NDPT_RtpSendto       pfnNdptRtpSendto;
    FN_NDPT_CreateLink      pfnNdptCreateLink;
    FN_NDPT_ModifyNetPara   pfnNdptModifyNetPara;
    FN_NDPT_DestroyLink     pfnNdptDestroyLink;
} NDPT_EXPORT_FUNC_S;

HI_S32 NDPT_ModeInit(HI_VOID);
HI_VOID NDPT_ModeExit(HI_VOID);
#endif


