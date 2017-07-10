#ifndef DMAC_EXT
#define DMAC_EXT

#include "hi_type.h"


typedef HI_S32 (*FN_DMAC_ChannelAllocate)(void *);
typedef HI_S32 (*FN_DMAC_ChannelFree)(HI_U32);
typedef HI_S32 (*FN_DMAC_AllocateDmalliSpace)(HI_U32 *, HI_U32);
typedef HI_S32 (*FN_DMAC_FreeDmalliSpace)(HI_U32 *, HI_U32);
typedef HI_S32 (*FN_DMAC_ChannelStart)(HI_U32);
typedef HI_S32 (*FN_DMAC_ChannelClose)(HI_U32);
typedef HI_S32 (*FN_DMAC_StartLlim2p)(HI_U32, HI_U32 *, HI_U32);
typedef HI_S32 (*FN_DMAC_StartM2p)(HI_U32, HI_U32, HI_U32, HI_U32, HI_U32);
typedef HI_S32 (*FN_DMAC_Buildllim2p)(HI_U32 *, HI_U32 *, HI_U32, HI_U32, HI_U32, HI_U32);
	
typedef struct
{
    FN_DMAC_ChannelAllocate      pfnDmacChannelAllocate;
    FN_DMAC_ChannelFree          pfnDmacChannelFree;
    FN_DMAC_AllocateDmalliSpace  pfnDmacAllocateDmalliSpace;
    FN_DMAC_FreeDmalliSpace      pfnDmacFreeDmalliSpace;
	FN_DMAC_ChannelStart         pfnDmacChannelStart;
	FN_DMAC_ChannelClose         pfnDmacChannelClose;
	FN_DMAC_StartLlim2p          pfnDmacStartLlim2p;
	FN_DMAC_StartM2p             pfnDmacStartM2p;
	FN_DMAC_Buildllim2p          pfnDmacBuildllim2p;
} DMAC_EXPORT_FUNC_S;

#endif

