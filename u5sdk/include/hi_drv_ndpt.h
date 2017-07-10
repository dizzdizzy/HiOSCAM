/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : priv_ndpt.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/11/11
  Description   :
  History       :
  1.Date        : 2010/11/11
    Author      : f00172091
    Modification: Created file

******************************************************************************/

#ifndef __PRIV_NDPT_H__
#define __PRIV_NDPT_H__

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "hi_common.h"
#include "hi_module.h"
#include "hi_debug.h"

#define RTP_PKT_MINLEN 60
#define NDPT_MAX_CHANNEL_COUNT 20

#define IPV4_ADDR_LEN   4
#define IPV6_ADDR_LEN   16
#define NDPT_ETH_ALEN   6
#define NDPT_ETH_HLEN   14
#define NDPT_IP_TOS_DEFAULT     (46<<2)     //DSCP EF

/*extern interface*/
/*CNcomment:外部接口*/
typedef enum
{
    LOOPBACK_NONE   = 0,    
    SEND_BACK,	          /*send-circle,data is neither sent to remote nor received from remote*/ /*CNcomment:发送环回，数据不发送到远端，也不接收远端的数据*/
    SEND_BACK_AND_OUT,    /*send-circle,data is sent to remote,but refuse data from remote*/ /*CNcomment:发送环回，同时数据发送到远端，但不接收远端的数据*/
    REV_BACK,             /*receive-circle,host neither send data to remote nor receive from remote*/ /*CNcomment:接收环回，本机不接收远端数据，也不向远端发送数据*/
    REV_BACK_AND_IN,      /*receive-circle,host receive data from remote , but will not send data to remote*/ /*CNcomment:接收环回，本机接收远端数据，但不向远端发送数据*/
    ALL_BACK,             /*nocircle for both send and receive,REV_BACK and SEND_BACK will be executing at the same time*/ /*CNcomment:发送和接收都环回，即SEND_BACK 和REV_BACK同时执行*/
    LOOPBACK_MAX,
}NDPT_LOOPBACK_MODE_E;

/*change-flags for net parameter struct*/
/*CNcomment:网络参数结构变更标志位 */
typedef struct
{
   HI_U32   bit1SrcIP:1;    /*change-flag for source ip, 32SrcIPLen and u8SrcIP[16] have be changed*/ /*CNcomment:source ip变更标记, 32SrcIPLen和u8SrcIP[16]有更改*/ 
   HI_U32   bit1DstIP:1;    /*change-flag for dest ip, u32DstIPLen and u8DstIP[16] have be changed*/ /*CNcomment:dest ip变更标记, u32DstIPLen和u8DstIP[16]有更改*/ 
   HI_U32   bit1SrcPort:1;  /*change-flag for source port, u16SrcPort has be changed*/ /*CNcomment:source port 变更标记,u16SrcPort有更改*/ 
   HI_U32   bit1DstPort:1;	/*change-flag for dest port, u16DstPort has be changed*/ /*CNcomment:dest port变更标记,u16DstPort有更改*/ 
   HI_U32   bit1IPTos:1;	/*change-flag for IP service type,u32IPTos and u32Mask have be changed*/ /*CNcomment:IP服务类型变更标记,u32IPTos和u32Mask有更改*/ 
   HI_U32   bit1Vlan:1;		/*change-flag for Vlan, u32VlanEn,u32VlanPri,u32VlanPid have be changed, reserved*/ /*CNcomment:Vlan变更标记,u32VlanEn,u32VlanPri,u32VlanPid有更改，保留*/ 
   HI_U32   bit1Protocol:1;	/*change-flag for protocol type, only support IPV4, reserved*/ /*CNcomment:协议类型变更标记，目前只支持IPV4,保留*/ 
   HI_U32   bit25Resv:25;         
}NDPT_NET_CHANGE_FLAG_S;

/*net parameter config*/
/*CNcomment:网络参数配置 */
typedef struct
{
    HI_U32 sip_len;      /*length of source ip address, IPV4:4, IPV6:16, other length is invalid*/ /*CNcomment:ip地址长度，4(IPV4)或16(IPV6)，其它值无效*/
    HI_U8 sip[16];       /*source ip, IPV4:4 bytes, IPV6:16 bytes,ip can't be string, eg: 4 bytes order is 192,168,1,1 for source ip 192.168.1.1*/
                         /*CNcomment:source ip，4字节(IPV4)或16字节(IPV6)，不是字符串，以192.168.1.1为例，4字节依次为192,168,1,1*/
    HI_U32 dip_len;      /*length of dest ip address, IPV4:4, IPV6:16, other length is invalid*/ /*CNcomment:ip地址长度，4(IPV4)或16(IPV6)，其它值无效*/
    HI_U8 dip[16];       /*dest ip, IPV4:4, IPV6:16, ip can't be string*/
                         /*CNcomment:dest ip，4字节(IPV4)或16字节(IPV6)，不是字符串*/
    HI_U16 sport;        /*port number of source RTP, it should be even, the releated RTCP port number is sport+1*/ /*CNcomment:source RTP 端口号，应为偶数；对应RTCP端口号为该数值+1 */
    HI_U16 dport;        /*port number of dest RTP, it should be even, the releated RTCP port number is dport+1*/ /*CNcomment:dest RTP 端口号，应为偶数；对应RTCP端口号为该数值+1 */

    HI_U32 mask;         /*bit0: u32IPTos valid;*/ /*CNcomment:bit0: u32IPTos valid;*/
    HI_U32 ip_tos;       /*8 bits for IP server type*/ /*CNcomment:IP服务类型,8bit*/
    HI_U32 vlan_en;      /*1:enable vlan, 0:disable vlan, reserved*/ /*CNcomment:vlan使能: 0--vlan 无效,1--vlan 有效,保留*/
    HI_U32 vlan_pri;     /*3 bits for priority of vlan, valid when u32VlanEn==1, reserved*/ /*CNcomment:vlan优先级,3bit, u32VlanEn为1时有效，保留*/
    HI_U32 vlan_pid;     /*vlan pid, 12bit, valid when u32VlanEn==1, reserved*/ /*CNcomment:vlan pid, 12bit, u32VlanEn为1时有效，保留*/
    HI_U32 protocol;     /*protocol type, 0x0800--IPV4, 0x86dd--IPV6, only support IPV4, reserved*/ /*CNcomment:协议类型,0x0800--IPV4, 0x86dd--IPV6，目前只支持IPV4，保留*/
}NDPT_NET_PARA_S;

/*config net parameter struct*/
/*CNcomment:配置网络参数结构 */
typedef struct hiNDPT_NET_CONFIG_PARA_S
{
    NDPT_NET_CHANGE_FLAG_S stChange;  /*chang-flag*/ /*CNcomment:变更标志位 */

    NDPT_NET_PARA_S stBody;           /*net parameter config*/ /*CNcomment:网络配置参数 */ 
}NDPT_NET_CONFIG_PARA_S;

typedef struct
{
    HI_U32  handle;
    NDPT_LOOPBACK_MODE_E    eLoopback;
}NDPT_LOOPBACK_S;

#define CMD_NDPT_CREATE_LINK              _IOW(HI_ID_NDPT,  0x1, NDPT_NET_PARA_S)
#define CMD_NDPT_DESTROY_LINK             _IOW(HI_ID_NDPT,  0x2, HI_U32)
#define CMD_NDPT_SET_LOOPBACK             _IOW(HI_ID_NDPT,  0x3, NDPT_LOOPBACK_S)

//-----------------------------------------------------------------------------------------------------

#define HI_FATAL_NDPT(fmt...) \
            HI_FATAL_PRINT(HI_ID_NDPT, fmt)

#define HI_ERR_NDPT(fmt...) \
            HI_ERR_PRINT(HI_ID_NDPT, fmt)

#define HI_WARN_NDPT(fmt...) \
            HI_WARN_PRINT(HI_ID_NDPT, fmt)

#define HI_INFO_NDPT(fmt...) \
            HI_INFO_PRINT(HI_ID_NDPT, fmt)


//#define RTP_BUFF_OFFSET   1

/*format definition of IP packet*/
/*CNcomment:IP数据包格式定义*/
typedef struct _HDR_IP_
{   
    HI_U8  ihl:4;  /*IP Header length in 32-bit*/
    HI_U8  ver:4;  /*IP protocol version */
    HI_U8  tos;    /*type of service*/
    HI_U16  len;   /*Length of  header and data in byte*/

    HI_U16  id;    /*Used for fragmentation*/
//  HI_U16  frag_offset:13;  /*Used for fragmentation*/
//  HI_U16  flag:3;  /*Used for fragmentation*/
    HI_U16 frag;

    HI_U8  ttl;       /*time to live*/
    HI_U8  protocol;  /*0x6 = TCP, 0x11 = UDP*/
    HI_U16  check;    /*Checksum of header only*/

    HI_U32  S_IP;     /*source IP address*/
    HI_U32  D_IP;     /*destination IP address*/
}HDR_IP;


/*format definition of UDP packet*/
/*CNcomment:UDP包格式定义*/
typedef struct _HDR_UDP_
{   
    HI_U16  S_Port;    /*UDP source port*/
    HI_U16  D_Port;    /*UDP destination port*/
    HI_U16  len;       /*length of UDP header and data in bytes*/
    HI_U16  checkSum;  /*Checksum of UDP header and data*/
}HDR_UDP;



typedef struct _HDR_RTP_     /*format definition of RTP packet header*/ /*CNcomment:RTP数据包头格式定义*/
{   
    HI_U8  cc:4;       /*CSRC count */
    HI_U8  x:1;        /*header extension flag */
    HI_U8  p:1;        /*padding flag */
    HI_U8  ver:2;      /*protocol version */

    HI_U8  pt:7;       /*payload type */
    HI_U8  m:1;        /*marker bit */

    HI_U16  usSeq;     /*sequence number */

    HI_U32  uiTs;      /*4:timestamp */
    HI_U32  uiSsrc;    /*8:synchronization source */
}HDR_RTP;

typedef struct _HDR_RTCP_        /*RTCP packet header*/ /*CNcomment:RTCP数据包头 */
{
    HI_U8  RC:5;     /*numbers of received report or SDES struct*/ /*CNcomment:接收报告或者SDES结构的数目 */
    HI_U8  P:1;      /*extra data flag*/ /*CNcomment:附加数据标志 */
    HI_U8  ver:2;    /*protocol version*/ /*CNcomment:协议版本 */

    HI_U8  pt;       /*type of RTCP packet*/ /*CNcomment:RTCP数据包类型 */

    HI_U16  usLen;   /*length of message packet, unit: 32bit*/ /*CNcomment:消息包的长度，单位为32bit*/
}HDR_RTCP;


/*format definition of ARP packet*/
/*CNcomment:ARP包格式定义*/
typedef struct _HDR_IPV4_ARP_
{   
    HI_U16  hd_type;            /*hardware type*/
    HI_U16  protocol_type;      /*protocol type*/
    HI_U8   hd_adr_len;         /*hardware address length*/
    HI_U8   protocol_adr_len;   /*protocol address length*/
    HI_U16  op_type;            /*operation type*/
    HI_U8   s_mac[NDPT_ETH_ALEN];
    HI_U8   s_ip[4];
    HI_U8   d_mac[NDPT_ETH_ALEN];
    HI_U8   d_ip[4];
}HDR_IPV4_ARP;

/*Note: This structure must be same as RTP_NET_BUFFER_STRU in HME_NET_Device.h*/
#ifdef RTP_BUFF_OFFSET
typedef struct
{
    HI_U8    *pucBufferHdr;  /*start address of datagram header*/ /*CNcomment:报文头起始地址*/
    HI_U32    uiBufferLen;   /*length of buffer*/ /*CNcomment:buffer的长度*/
    HI_U32    uiOffset;      /*offset between datagram header and RTP header*/ /*CNcomment:报文头到RTP头的偏移*/
}RTP_NET_BUFFER_STRU;
#else
typedef struct
{
    HI_U8    *pucBufferHdr;  /*start address of RTP datagram header*/ /*CNcomment:RTP报文头起始地址*/
    HI_U32    uiBufferLen;   /*length of RTP data*/ /*CNcomment:RTP数据长度*/
}RTP_NET_BUFFER_STRU;
#endif

/*Note: This enum must be same as PORT_MODE in HME_NET_Device.h*/
typedef enum
{
    PORT_EVEN   = 0,     /*wraped by even port of datalink config, carry RTP stream*/ /*CNcomment:使用链路配置的偶数端口封装，承载RTP流*/
    PORT_ODD    = 1,	 /*wraped by odd port of datalink config, carry RTP stream*/ /*CNcomment:使用链路的奇数端口封装，承载RTCP流*/
    PORT_CONFIG = 2      /*wraped by port number of datalink config*/ /*CNcomment:使用链路配置的端口号封装*/
}PORT_MOD;

typedef enum
{
    NDPT_PROC_CMD_LOOKBACK  = 0,    /*set cicle-mode, parameter set refer to NDPT_LOOPBACK_MODE_E*/ /*CNcomment:设置环回模式，参数请参考NDPT_LOOPBACK_MODE_E*/
    NDPT_PROC_CMD_RESETCNT,	    /*statistic for RESET send and receive*/ /*CNcomment:RESET发送和接收统计*/
    NDPT_PROC_CMD_SETIPTOS,         /*set IP TOS*/ /*CNcomment:设置IP TOS*/
}NDPT_PROC_CMD;

typedef struct _NDPT_SEND_ERR_CNT_S
{
   HI_U32   bit1DstIP:1;	/*unable to reach dest IP address*/ /*CNcomment:目标IP地址无法到达*/
   HI_U32   bit1SameIP:1;       /*source and dest ip are the same*/ /*CNcomment:source 与dest ip相同*/
   HI_U32   bit1Long:1;         /*extra-packet*/ /*CNcomment:超长包*/
   HI_U32   bit1Skb:1;	        /*malloc skb failure*/ /*CNcomment:分配skb失败*/
   HI_U32   bit1MacHd:1;	/*create mac header failure*/ /*CNcomment:创建mac头失败*/
   HI_U32   bit1Xmit:1;	        /*dev_queue_xmit failure*/ /*CNcomment:dev_queue_xmit失败*/
   HI_U32   bit1Para:1;         /*invalid parameter for ndpt_rtp_sendto()*/ /*CNcomment:HI_DRV_NDPT_SendRtp()函数参数错误*/
   HI_U32   bit1NotReady:1;     /*channel is not ready to receive and send data*/ /*CNcomment:通道未准备好收发数据*/
}NDPT_SEND_ERR_FLAG_S;

#define NDPT_REV_INTERVAL_NUM   1000   /*statistic of received RTP packets gap*/ /*CNcomment:接收到的RTP包间隔统计个数*/

typedef struct _NDPT_PACKET_INTERVAL_S
{
    struct _NDPT_PACKET_INTERVAL_S *next;
    HI_U32  u32Interval;                       /*gaps between received RTP packets, unit is us*/ /*CNcomment:接收到的RTP包间隔,us为单位*/
}NDPT_PACKET_INTERVAL_S;

/*Note: This type define must be same as ndpt_rtp_revfrom in HME_NET_Device.h*/
typedef HI_S32 (*ndpt_rtp_revfrom)(HI_U32 TransId, PORT_MOD even_odd, RTP_NET_BUFFER_STRU *data_buffer);

/*module NDPT channel control struct*/ /*CNcomment:NDPT模块通道控制结构*/
typedef struct _NDPT_CH_S
{
    struct _NDPT_CH_S *prev;
    struct _NDPT_CH_S *next;
    
    HI_U32  handle;	        /*handle of net adapter channel*/ /*CNcomment:网络适配通道句柄*/
    NDPT_NET_PARA_S stNetPara;  /*net config parameter*/ /*CNcomment:网络配置参数*/
    NDPT_NET_CHANGE_FLAG_S  stNetFlag;  /*flag of net parameter config*/ /*CNcomment:网络参数设置标记*/
    
    HI_U8 ucSrc_mac[NDPT_ETH_ALEN];
    struct net_device *dev;
    HI_U8 ucDst_mac[NDPT_ETH_ALEN];

    NDPT_LOOPBACK_MODE_E    eLoopback;

    HI_U32 TransId;      /*ID for AV module channel*/ /*CNcomment:音视频模块通道ID*/
    ndpt_rtp_revfrom revfunc;    /*RTP data received function pointer of AV module*/ /*CNcomment:音视频模块RTP数据接收函数指针*/
    
    NDPT_SEND_ERR_FLAG_S stSendErrFlag;
    HI_U32 uiSendTry;
    HI_U32 uiSendOkCnt;
    HI_U32 uiRevRtpCnt;
    HI_U16 usRtpSendSeq;
    HI_U16 usRevFlag;       /*0:not begin to count*/ /*CNcomment:0--未开始计数*/
    HI_U32 uiRevSeqMin;
    HI_U32 uiRevSeqMax;
    HI_U32 uiRevSeqCnt;
    struct timeval stArriveLast;    /*previous RTP datagram arrived time*/ /*CNcomment:上一个RTP报文到达时刻*/
    struct _NDPT_PACKET_INTERVAL_S  *pstRevInterval;    /*interval array pointer of received RTP packets*/ /*CNcomment:接收到的RTP包间隔数组指针*/
    struct _NDPT_PACKET_INTERVAL_S  *pstRevIntervalHead;
    struct _NDPT_PACKET_INTERVAL_S  *pstRevIntervalTail;
    HI_U32  u32RevIntervalCnt;
    HI_U32  u32RevIntervalTotal;
}NDPT_CH_S;

/*
set circle mode for net adapter layer CNcomment:设置网络适配层通道环回模式。
input parameter:
      handle:handle of net adapter layer channel CNcomment:网络适配层通道句柄。
      eLoopbackMode:circle mode CNcomment:环回模式。
return:
      0:success CNcomment:成功
      other:failure CNcomment:其它 失败
*/    
extern HI_S32 ndpt_set_loopback(HI_U32 handle,NDPT_LOOPBACK_MODE_E eLoopback);

#endif

