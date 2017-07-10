#define MODULE_LOG_PREFIX "constcw"

//FIXME Not checked on threadsafety yet; after checking please remove this line
#include "globals.h"
#ifdef MODULE_CONSTCW
#include "oscam-conf.h"
#include "oscam-client.h"
#include "oscam-chk.h"
#include "oscam-ecm.h"
#include "oscam-net.h"
#include "oscam-string.h"
#include "oscam-array.h"
#include "module-dvbapi.h"
#include "module-constcw.h"
//
//
//
#if 1
	#define	MYCONST_TRACE	myprintf
#else
	#define	MYCONST_TRACE(...)
#endif
//
//
//
//
//
struct xconst_keydata
{
	int16_t	found;
	uint16_t	caid;
	uint32_t	ppid;
	uint32_t	freq;
	uint32_t	ecmpid;
	uint32_t	pmtpid;
	uint32_t	vpid;
	uint32_t	srvid;
	uint8_t	constcw[16];
};
struct xconst_keydata g_xconst[MAX_DEMUX];

static int32_t 	pserver;
static int16_t		cFTA_Maxes = 0;
static FTA_BISTAB	cFTA_Biss[MAX_FTABISS];


bool
CONSTCW_IsFtaBisses(struct s_reader *reader, uint16_t ccaid, uint32_t cpfid, uint16_t csrvid, uint16_t cvidpid, uint16_t cdgrid)
{
#if 0
	// CAID:PROVIDER:SID:VPID:ECMPID::XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
	// CAID:PROVIDER:SID:PMTPID:ECMPID:VPID:XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX

	FILE 	*fp;
	char 	 token[512];
	uint32_t caid, provid, srvid, vidpid, pmtpid, ecmpid;
	uint32_t cwiv[16];
	int32_t  cwfound = 0;
	int rcval;

	fp = open_config_file(reader->device);
	if (!fp) return 0;

	while (fgets(token, sizeof(token), fp))
	{

		if (token[0]=='#') continue;
		if (token[0]=='<') continue;
		if (token[0]==' ') continue;
		caid  = provid = srvid = vidpid = ecmpid = pmtpid = 0;
		memset(cwiv, 0, sizeof(cwiv));
		rcval = sscanf(token, "%4x:%6x:%4x:%4x:%4x::%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
							&caid, &provid, &srvid, &vidpid, &ecmpid,
							&cwiv[0], &cwiv[1], &cwiv[ 2], &cwiv[ 3], &cwiv[ 4], &cwiv[ 5], &cwiv[ 6], &cwiv[ 7],
							&cwiv[8], &cwiv[9], &cwiv[10], &cwiv[11], &cwiv[12], &cwiv[13], &cwiv[14], &cwiv[15]);

		if (rcval != 13 && rcval != 21) {
			rcval = sscanf(token, "%4x:%6x:%4x:%4x:%4x:%4x:%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
							&caid, &provid, &srvid, &pmtpid, &ecmpid, &vidpid,
				   		&cwiv[0], &cwiv[1], &cwiv[ 2], &cwiv[ 3], &cwiv[ 4], &cwiv[ 5], &cwiv[ 6], &cwiv[ 7],
				   		&cwiv[8], &cwiv[9], &cwiv[10], &cwiv[11], &cwiv[12], &cwiv[13], &cwiv[14], &cwiv[15]);
			if (rcval != 14 && rcval != 22) continue;
		}

		if ((ccaid == caid) &&
			 (csrvid== srvid)  &&
			 (cpfid ==provid) &&
			 (cdgrid ==ecmpid) &&
			 (cvidpid==vidpid))
		{
			MYCONST_TRACE("myconst:fta.constcw.%04X:%06X:%04X:%04X:%04X\n",
					ccaid, cpfid, csrvid, cvidpid, cdgrid);
			cwfound = 1;
			break;
		}
	}
	fclose(fp);
	return (cwfound);
#else
	int i;

	for (i=0; i<cFTA_Maxes; i++)
	{
		if ( cFTA_Biss[i].caid   != ccaid) continue;
		if ( cFTA_Biss[i].edgrid != cdgrid) continue;
		if (!cFTA_Biss[i].srvid  || cFTA_Biss[i].srvid != csrvid)  continue;
//		if (!cFTA_Biss[i].pfid   || cFTA_Biss[i].pfid  != cpfid)   continue;
		if (!cFTA_Biss[i].pfid   || cFTA_Biss[i].pfid   < cpfid-1 || cFTA_Biss[i].pfid > cpfid+1) continue;
		if ( cFTA_Biss[i].vidpid && cFTA_Biss[i].vidpid!= cvidpid) continue;
		return 1;
	}
	return 0;
#endif
}

static bool
constcw_analyser(struct s_client *client)
{
	// CAID:PROVIDER:SID:VPID:ECMPID::XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
	// CAID:PROVIDER:SID:PMTPID:ECMPID:VPID:XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
	FILE 	*fp;
	char 	 token[512];
	uint32_t frequency, degree;
	uint32_t caid, provid, srvid, vidpid, pmtpid, ecmpid;
	uint32_t cwiv[16];
	int32_t  counter = 0;
	CAIDTAB_DATA xcts[CS_MAXCAIDTAB];
	int numcaids = 0;
	int rcval;
	int i;

	cFTA_Maxes = 0;
	caidtab_clear(&client->reader->ctab);
	fp = open_config_file(client->reader->device);
	if (!fp) return 0;

	while (fgets(token, sizeof(token), fp))
	{
		if (counter > MAX_FTABISS-1) break;
		if (token[0]=='#') continue;
		if (token[0]=='<') continue;
		if (token[0]==' ') continue;
		caid  = provid = srvid = vidpid = ecmpid = pmtpid = 0;
		memset(cwiv, 0, sizeof(cwiv));
		rcval = sscanf(token, "%4x:%6x:%4x:%4x:%4x::%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
							&caid, &provid, &srvid, &vidpid, &ecmpid,
							&cwiv[0], &cwiv[1], &cwiv[ 2], &cwiv[ 3], &cwiv[ 4], &cwiv[ 5], &cwiv[ 6], &cwiv[ 7],
							&cwiv[8], &cwiv[9], &cwiv[10], &cwiv[11], &cwiv[12], &cwiv[13], &cwiv[14], &cwiv[15]);

		if (rcval != 13 && rcval != 21) {
			rcval = sscanf(token, "%4x:%6x:%4x:%4x:%4x:%4x:%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
							&caid, &provid, &srvid, &pmtpid, &ecmpid, &vidpid,
				   		&cwiv[0], &cwiv[1], &cwiv[ 2], &cwiv[ 3], &cwiv[ 4], &cwiv[ 5], &cwiv[ 6], &cwiv[ 7],
				   		&cwiv[8], &cwiv[9], &cwiv[10], &cwiv[11], &cwiv[12], &cwiv[13], &cwiv[14], &cwiv[15]);
			if (rcval != 14 && rcval != 22) continue;
		}
		frequency = cs_BCD2i(provid);
		degree = cs_BCD2i(ecmpid);
		if ((caid_is_biss(caid)) &&
			 (srvid)  &&
			 (frequency && (frequency > 950 && frequency < 15000)) &&
			 (degree < 3600) &&
			 (vidpid && vidpid < 0x1fff))
		{
			MYCONST_TRACE("myconst:f.constcw.%04X:%06X:%04X:%04X:%04X\n",
					caid, provid, srvid, vidpid, ecmpid);
			cFTA_Biss[counter].caid   = caid;
			cFTA_Biss[counter].srvid  = srvid;
			cFTA_Biss[counter].pfid   = provid;
			cFTA_Biss[counter].vidpid = vidpid;
			cFTA_Biss[counter].edgrid = ecmpid;
			counter++;
		}

		for (i=0; i<numcaids; i++)	{
			if (xcts[i].caid == caid) break;
		}
		if (i==numcaids)
		{
			if (numcaids>(CS_MAXCAIDTAB-1)) {
				mycs_debug(D_ADB, "xcaskey:CA{%04X} CS_MAXCAIDTAB exceed\n", caid);
				break;
			}
			mycs_debug(D_ADB, "constcw:ctab.caid:%04X\n", caid);
			xcts[numcaids].mask = 0xffff;
			xcts[numcaids].cmap = 0x0;
			xcts[numcaids].caid = caid;
			caidtab_add(&client->reader->ctab, &xcts[numcaids]);
			numcaids++;
		}
	}
	fclose(fp);
	cFTA_Maxes = counter;
	return (counter);
}

static int32_t
constcw_available(struct s_client *client)
{
	FILE *fp;

	if (!client) return 0;
//	fp = fopen(client->reader->device, "r");
	fp = open_config_file(client->reader->device);
	if (!fp) return 0;
	fclose(fp);
	return 1;
}

static int32_t
constcw_search(struct s_client *client, ECM_REQUEST *er, uchar *dcw)
{
	// CAID:PROVIDER:SID:VPID:ECMPID::XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
	// CAID:PROVIDER:SID:PMTPID:ECMPID:VPID:XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
	struct xconst_keydata *pxconst;
	FILE 	*fp;
	char 	 token[512];
	uint32_t caid, provid, sid, vpid, pmtpid, ecmpid;
	uint32_t cwiv[16];
	int32_t  cwfound = 0;
	int32_t  dmuxid;

	if (!er) return 0;
	if (!client) return 0;
	MYCONST_TRACE("myconst:searching constcw.%04X:%06X:%04X:%04X:%04X\n",
			er->caid, er->prid, er->srvid, er->vpid, er->pid);
	dmuxid  = er->dmuxid % MAX_DEMUX;
	pxconst = &g_xconst[dmuxid];
	if (pxconst->caid == er->caid && pxconst->srvid == er->srvid && pxconst->ecmpid == er->pid && pxconst->vpid == er->vpid)
	{
		if (!pxconst->found) return 0;
		memcpy(dcw, pxconst->constcw, 16);
		MYCONST_TRACE("myconst:CONSTANT:%04X.%5d:%02X...%02X\n", er->caid, er->srvid, dcw[0], dcw[7]);
		return 1;
	}
	pxconst->caid   = er->caid;
	pxconst->ppid   = er->prid;
	pxconst->freq	 = er->chSets.frequency;
	pxconst->srvid  = er->srvid;
	pxconst->pmtpid = er->pmtpid;
	pxconst->ecmpid = er->pid;
	pxconst->vpid   = er->vpid;
	pxconst->found  = 0;

//	fp = fopen(cur_client()->reader->device, "r");
	fp = open_config_file(client->reader->device);
	if (!fp) return 0;

	while (fgets(token, sizeof(token), fp))
	{
		int rcval;
		int singly = 0;
		int i;

		if (token[0]=='#') continue;
		if (token[0]=='<') continue;
		if (token[0]==' ') continue;
		singly= 0;
		caid  = provid = sid = vpid = ecmpid = pmtpid = 0;
		memset(cwiv, 0, sizeof(cwiv));
		rcval = sscanf(token, "%4x:%6x:%4x:%4x:%4x::%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
							&caid, &provid, &sid, &vpid, &ecmpid,
							&cwiv[0], &cwiv[1], &cwiv[ 2], &cwiv[ 3], &cwiv[ 4], &cwiv[ 5], &cwiv[ 6], &cwiv[ 7],
							&cwiv[8], &cwiv[9], &cwiv[10], &cwiv[11], &cwiv[12], &cwiv[13], &cwiv[14], &cwiv[15]);
//		MYCONST_TRACE("rcval: %d\n", rcval);
		if (rcval == 13) singly = 1;
		if (rcval != 13 && rcval != 21) {
			rcval = sscanf(token, "%4x:%6x:%4x:%4x:%4x:%4x:%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
							&caid, &provid, &sid, &pmtpid, &ecmpid, &vpid,
				   		&cwiv[0], &cwiv[1], &cwiv[ 2], &cwiv[ 3], &cwiv[ 4], &cwiv[ 5], &cwiv[ 6], &cwiv[ 7],
				   		&cwiv[8], &cwiv[9], &cwiv[10], &cwiv[11], &cwiv[12], &cwiv[13], &cwiv[14], &cwiv[15]);
			if (rcval == 14) singly = 1;
			if (rcval != 14 && rcval != 22) continue;
		}

//		MYCONST_TRACE("Line found: %s", token);
		uint32_t freqid = cs_BCD2i(provid);
		if (( er->caid == caid) &&
			 ( er->srvid== sid)  &&
			 ( er->chSets.frequency==freqid || er->chSets.frequency==freqid-1 || er->chSets.frequency==freqid+1 || er->prid==provid) &&
		/*	 (!ecmpid || !er->pid || er->pid==ecmpid) && */
			 (!pmtpid || !er->pmtpid || er->pmtpid==pmtpid) &&
			 (!vpid   || !er->vpid || er->vpid==vpid))
		{
			for (i=0; i<16;i++) dcw[i] = cwiv[i];
			if (singly) memcpy(&dcw[8], &dcw[0], 8);
			cs_log("Entry found: %04X:%06X:%04X:%04X:%04X::%s",
					caid, provid, sid, vpid, ecmpid,
					cs_hexdump(1, dcw, 16, token, sizeof(token)));
			pxconst->found = 1;
			memcpy(pxconst->constcw, dcw, 16);
			MYCONST_TRACE("myconst:constcw.%04X:%06X:%04X:%04X:%04X::%02X...%02X:%02X...%02X\n",
					caid, provid, sid, vpid, ecmpid,
					dcw[0], dcw[7], dcw[8], dcw[15]);
			cwfound = 1;
			break;
		}
	}
	fclose(fp);
	return (cwfound);
}
//**********************************************************************
//* client/server common functions
//**********************************************************************
static int32_t
constcw_recv(struct s_client *client, uchar *buf, int32_t l)
{
	int32_t ret;

	MYCONST_TRACE("myconst:constcw_recv...\n");
	if (!client->udp_fd) return(-9);
	ret = read(client->udp_fd, buf, l);
	if (ret < 1) return -1;
	client->last = time(NULL);
	return (ret);
}

//**********************************************************************
//*       client functions
//**********************************************************************
static int32_t
constcw_client_init(struct s_client *client)
{
	int32_t fdp[2];

	MYCONST_TRACE("myconst:constcw_client_init...\n");
	client->pfd = 0;
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fdp))
	{
		cs_log("constcw: Socket creation failed (%s)", strerror(errno));
		return 1;
	}
	client->udp_fd =fdp[0];
	pserver = fdp[1];
	memset(&g_xconst,0, sizeof(g_xconst));
	memset((char *) &client->udp_sa, 0, sizeof(client->udp_sa));
	SIN_GET_FAMILY(client->udp_sa) = AF_INET;

	// Oscam has no reader.au in s_reader like ki's mpcs ;)
// reader[ridx].au = 0;
// cs_log("local reader: %s (file: %s) constant cw au=0", reader[ridx].label, reader[ridx].device);
	cs_log("local reader: %s (file: %s) constant cw", client->reader->label, client->reader->device);

	client->pfd = client->udp_fd;
	if (constcw_available(client))
	{
		constcw_analyser(client);
		client->reader->tcp_connected = 2;
		client->reader->card_status = CARD_INSERTED;
	}
	client->reader->tcp_ito = 0;
	client->reader->last_s = client->reader->last_g = time(NULL);
	return 0;
}


static void
constcw_clidle(void)
{
	struct s_client *client = cur_client();
	time_t  now;
	int32_t time_diff;

	if (!client) return;
	if (!client->reader) return;
	if (!client->reader->enable) return;
	if ( client->reader->ch_descramble) return;

	time(&now);
	time_diff = now - client->reader->last_s;
	if (time_diff > 12)
	{
#if defined(WITH_HISILICON)
		if (chk_av_descrambling(client)) {
			client->reader->ch_descramble = 1;
			client->reader->tcp_ito = 0;
			mycs_debug(D_ADB, "constcw:descrambling");
		}
		else {
			mycs_debug(D_ADB, "constcw:scrambled");
			dvbapi_constcw_afterwards(R_CONSTCW, client->ch_ics.muxid, 1);
		}
#endif
	}
}

static int32_t
constcw_send_ecm(struct s_client *client, ECM_REQUEST *er, uchar *UNUSED(msgbuf))
{
	uint8_t cw[16];
	int cwfound = 0;

	if (!er) return 0;
	if (!client->reader) return 0;
	if ( client->reader->card_status != CARD_INSERTED) return 0;
	MYCONST_TRACE("myconst:constcw_send_ecm...\n");
	cs_log("searching constcw %04X:%06X:%04X:%04X", er->caid, er->prid, er->srvid, er->vpid);
	// Check if DCW exist in the files
	cwfound = constcw_search(client, er, cw);
	if (cwfound)
	{
		er->ecm_bypass = 1;
		set_cw_checksum(cw, 16);
		write_ecm_answer(client->reader, er, E_FOUND, 0, cw, NULL, NULL);
	}
	else
	{
		MYCONST_TRACE("myconst:constcw.none\n");
	//	sky(!)
	//	write_ecm_answer(client->reader, er, E_NOTFOUND, (E1_READER<<4 | E2_SID), NULL, NULL, NULL);
	}

	time_t now = time(NULL);
	client->last = now;
	client->reader->last_s  = client->reader->last_g = now;
	client->reader->tcp_ito = 10;
	return 0;
}

static int32_t
constcw_recv_chk(struct s_client *UNUSED(client), uchar *UNUSED(dcw), int32_t *rc, uchar *UNUSED(buf), int32_t UNUSED(n))
{
	MYCONST_TRACE("myconst:constcw_recv_chk...\n");
//	dcw = dcw;
//	n 	 = n;
//	buf = buf;
	*rc = 0;
	return -1;
}

static int32_t
constcw_ChCloser(struct s_client *client, int muxid)
{
	MYCONST_TRACE("constcw:closer\n");
	if (!client) return -1;
	if (!client->reader) return -1;
	client->reader->tcp_ito = 0;
	client->reader->ch_descramble = 0;
	return 1;
}

static int32_t
constcw_Cleanup(struct s_client *client)
{
	MYCONST_TRACE("constcw:cleanup\n");
	if (!client) return -1;
	if (!client->reader) return -1;
	client->reader->tcp_ito = 0;
	memset(&g_xconst,0, sizeof(g_xconst));
	return 1;
}

void module_constcw(struct s_module *ph)
{
	MYCONST_TRACE("MODULE_constcw...\n");
	ph->desc 		 	= "constcw";
	ph->type 		 	= MOD_NO_CONN;
	ph->listenertype 	= LIS_CONSTCW;
	ph->recv 		 	= constcw_recv;
	ph->c_init 		 	= constcw_client_init;
	ph->c_idle			= constcw_clidle;
	ph->c_recv_chk 	= constcw_recv_chk;
	ph->c_send_ecm 	= constcw_send_ecm;
	ph->c_Cleanup		= constcw_Cleanup;
	ph->c_ChCloser		= constcw_ChCloser;
	ph->num			 	= R_CONSTCW;
}
#endif	// #ifdef MODULE_CONSTCW

