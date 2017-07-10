#include "globals.h"
#if defined(MODULE_XCAS)
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-net.h"
#include "oscam-chk.h"
#include "oscam-string.h"
#include "cscrypt/des_ssl.h"
#include "cscrypt/aes_ctx.h"
#include "module-dvbapi.h"
#include "module-xcas.h"

#if defined(__XCAS_BISS__)
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
struct xbiss_keydata
{
	uint16_t	caid;
	uint32_t	ppid;
	uint32_t	freq;
	int16_t	found;
	uint8_t	constcw[16];

	int16_t	cwidx;
	int16_t	cwmax;
	uint8_t	cwmulkeys[MAXMULTIKEY<<1][8];
};
struct xbiss_keydata g_xbiss[MAX_DEMUX];

static int
XBISS_cwChkComparison(int dmuxid, int cwMaxes)
{
	struct xbiss_keydata *pxbiss = &g_xbiss[dmuxid];
	int counter = 0;
	int ignored;
	int i, j;

	if (cwMaxes < 2) return (cwMaxes);
	for (i=0; i<cwMaxes; i++)
	{
		ignored = 0;
		if (cs_Iszero(pxbiss->cwmulkeys[i],8)) ignored = 1;
		else {
			for (j=0; j<i; j++)
			{
				if (!memcmp(pxbiss->cwmulkeys[i], pxbiss->cwmulkeys[j], 8)) {
					ignored = 1;
					break;
				}
			}
		}
		if (!ignored) {
			if (i != counter) memcpy(pxbiss->cwmulkeys[counter], pxbiss->cwmulkeys[i], 8);
			counter++;
		}
	}
	return (counter);
}


int
XBISS_Process(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw)
{
	struct xbiss_keydata *pxbiss;
	uint32_t srvprid;
	uint8_t	*srvcws;
	uint8_t	*Lfscws;
	uint8_t  dcw[16];
	int dmuxid;
	int cwfound = 0;
	int LfsMax, srpMax;
	int cwdirection = 0;

	if (!er)  return 0;
	if (!rdr) return 0;
	dmuxid = er->dmuxid % MAX_DEMUX;
	pxbiss = &g_xbiss[dmuxid];

	cwdirection = er->constAfterwards;
	srvprid = (uint32_t)((er->srvid<<16)|er->pid);
	MYEMU_TRACE("xbiss:searching{%04X,%08X,%05d}{%04X,%08X,%05d}{%d.%d}\n",
			er->caid, srvprid,er->chSets.frequency,
			pxbiss->caid, pxbiss->ppid, pxbiss->freq,
			pxbiss->found,
			cwdirection);

	if (pxbiss->caid == er->caid &&
		 pxbiss->ppid == srvprid  &&
		 pxbiss->freq == er->chSets.frequency)
	{
		if (!pxbiss->found) return 0;
		if ( pxbiss->cwmax > 1)
		{
			if (cwdirection<0) pxbiss->cwidx = (pxbiss->cwidx + pxbiss->cwmax-1) % pxbiss->cwmax;
			else if (cwdirection>0) pxbiss->cwidx = (pxbiss->cwidx+1) % pxbiss->cwmax;
			else pxbiss->cwidx = pxbiss->cwidx % pxbiss->cwmax;
			memcpy(pxbiss->constcw, pxbiss->cwmulkeys[pxbiss->cwidx], 8);
			memcpy(pxbiss->constcw+8, pxbiss->cwmulkeys[pxbiss->cwidx], 8);
		}
		memcpy(cw, pxbiss->constcw, 16);
		MYEMU_TRACE("xbiss:cwfound{%02X...%02X}{%d/%d}\n", cw[0], cw[7], pxbiss->cwidx+1, pxbiss->cwmax);
		return 1;
	}

	pxbiss->cwidx = 0;
	Lfscws = pxbiss->cwmulkeys[0];
	LfsMax = XEMUKEY_ChLfsSearch(rdr, CASS_BISS, er->chSets.frequency, 0x0, Lfscws, 8);
	srvcws = pxbiss->cwmulkeys[LfsMax];
	srpMax = XEMUKEY_MultiSearch(rdr, CASS_CONSTANT, srvprid, 0x0, srvcws, 8);

	if (LfsMax)
	{
		memcpy(dcw, Lfscws, 8);
		memcpy(dcw+8, Lfscws, 8);
		cwfound = 1;
	}
	else
	if (srpMax)
	{
		memcpy(dcw, srvcws, 8);
		memcpy(dcw+8, srvcws, 8);
		cwfound = 1;
	}

	pxbiss->caid = er->caid;
	pxbiss->freq = er->chSets.frequency;
	pxbiss->ppid = srvprid;
	pxbiss->found = cwfound;
	pxbiss->cwmax = XBISS_cwChkComparison(dmuxid, srpMax+LfsMax);
	if (cwfound)
	{
		set_cw_checksum(dcw, 16);
		memcpy(cw, dcw, 16);
		memcpy(pxbiss->constcw, cw, 16);
		MYEMU_TRACE("xbiss:cwfirst{%02X...%02X}(%d/%d(%d_%d))\n",
				cw[0], cw[7], pxbiss->cwidx+1, pxbiss->cwmax, LfsMax, srpMax);
	}
	return (cwfound);
}


void
XBISS_Cleanup(void)
{
	MYEMU_TRACE("xbiss:clean\n");
	memset(&g_xbiss, 0, sizeof(g_xbiss));
}

#endif	// defined(__XCAS_BISS__)
#endif	// defined(MODULE_XCAS)

