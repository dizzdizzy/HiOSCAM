#define MODULE_LOG_PREFIX "xcas"
#include "globals.h"
#if defined(MODULE_XCAS)
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-chk.h"
#include "oscam-net.h"
#include "oscam-string.h"
#include "module-dvbapi.h"
#include "module-xcas.h"

extern struct s_client	*dvbApi_client;
static int32_t 			pserver;

// sky(powervu)
bool
xcas_IsAuAvailable(struct s_reader *rdr, uint16_t casid, uint32_t provid)
{
	#if defined(__XCAS_AUTOROLL__)
		if (IS_EMU_READERS(rdr)) {
		//	if (casid==0x500 && provid==0x030B00) return 1; /* dead */
			if (casid==0xE00) return 1;
		}
	#endif
	return 0;
}
// sky(powervu)
bool
xcas_IsEmmAvailable(struct s_reader *rdr, EMM_PACKET *ep)
{
	#if defined(__XCAS_AUTOROLL__)
		uint16_t casid = b2i(2, ep->caid);
		if (IS_EMU_READERS(rdr)) {
		//	if (casid==0x500 && provid==0x030B00) return 1; /* dead */
			if (casid==0xE00) return 1;
		}
	#endif
	return 0;
}
//**********************************************************************
//* client/server common functions
//**********************************************************************
static int32_t
xcas_recv(struct s_client *client, uchar *buf, int32_t l)
{
	int32_t ret;

	MYXCAS_TRACE("xcas:xcas_recv{%p}\n", client);
	if (!client->udp_fd) return -9;
	ret = read(client->udp_fd, buf, l);
	if (ret < 1) return(-1);
	client->last = time(NULL);
	return (ret);
}

//**********************************************************************
//*       client functions
//**********************************************************************
int32_t
xcas_clinit(struct s_client *client)
{
	int32_t fdp[2];

	MYXCAS_TRACE("xcas:xcas_clinit{%p}\n", client);
	client->pfd = 0;
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fdp))
	{
		cs_log("xcas:xcas_clinit failed (%s)", strerror(errno));
		return 1;
	}
	client->udp_fd = fdp[0];
	client->pfd = client->udp_fd;
	client->emu_casid = 0;
	pserver = fdp[1];

	memset((char *) &client->udp_sa, 0, sizeof(client->udp_sa));
	SIN_GET_FAMILY(client->udp_sa) = AF_INET;

	if (XEMUKEY_Initialize(client->reader))
	{
		mycs_log("xcas:file(%s)", client->reader->device);
		client->reader->card_status = CARD_INSERTED;
	}
	else
	{
		mycs_log("xcas:non file(%s)", client->reader->device);
		client->reader->card_status = CARD_FAILURE;
	}
	client->reader->tcp_ito = 0;
	client->reader->last_s 	= client->reader->last_g = time(NULL);
	return 0;
}

// return 1 if we are able to send requests
int32_t
xcas_clAvailable(struct s_reader *reader, int32_t checktype, ECM_REQUEST *er)
{
	if (!er) return 0;
	if (!reader) return 0;
	if ( reader->card_status != CARD_INSERTED) return 0;
	if (checktype == AVAIL_CHECK_CHANNEL || checktype == AVAIL_CHECK_ECM)
	{
		if (!XEMUKEY_IsAvailable(reader, er->caid, er->prid)) return 0;
	}
	return 1;
}


void
xcas_clidle(void)
{
	struct s_client *client = cur_client();
	time_t  now;
	int32_t time_diff;

	if (!client) return;
	if (!client->reader) return;
	if (!client->reader->enable) return;
	if (!IS_BISS(client->emu_casid)) return;
	if ( client->reader->ch_descramble) return;

	time(&now);
	time_diff = now - client->reader->last_s;
	if (time_diff > 12)
	{
	#if defined(WITH_HISILICON)
		if (chk_av_descrambling(client)) {
			client->reader->ch_descramble = 1;
			client->reader->tcp_ito = 0;
			mycs_debug(D_ADB, "xcas:descrambling");
		}
		else {
			mycs_debug(D_ADB, "xcas:scrambled");
			dvbapi_constcw_afterwards(R_XCAS, client->ch_ics.muxid, 1);
		}
	#endif
	}
}


static int32_t
xcas_send_ecm(struct s_client *client, ECM_REQUEST *er, uchar *UNUSED(msgbuf))
{
	CWEXTENTION cwEx;
	uint8_t cw[16];
	time_t  now;
	int cwisextend = 0;
	int cwfound = 0;

	if (!er) return 0;
	if (!client->reader) return 0;
	if ( client->reader->card_status != CARD_INSERTED) return 0;
	client->reader->tcp_ito = 0;
	client->reader->audisabled = 1;
	if (xcas_IsAuAvailable(client->reader, er->caid, er->prid)) client->reader->audisabled = 0;
	client->emu_casid = er->caid;
	switch (er->caid & 0xff00)
	{
	#if defined(__XCAS_SEKA__)
		case 0x0100:
			mycs_debug(D_ADB, "xcas:send_ecm.seka{%04X}", er->caid);
			cwfound = XSEKA_Process(client->reader, er, cw);
			break;
	#endif
	#if defined(__XCAS_VIACESS__)
		case 0x0500:
			mycs_debug(D_ADB, "xcas:send_ecm.viacess{%04X}", er->caid);
			cwfound = XVIACESS_Process(client->reader, er, cw);
			break;
	#endif
	#if defined(__XCAS_BISS__)
		case 0x2600:
			mycs_debug(D_ADB, "xcas:send_ecm.biss{%04X.%d}", er->caid, er->constAfterwards);
			cwfound = XBISS_Process(client->reader, er, cw);
			client->reader->tcp_ito = 10;
			break;
	#endif
	#if defined(__XCAS_CRYPTOWORKS__)
		case 0x0D00:
			mycs_debug(D_ADB, "xcas:send_ecm.cryptoworks{%04X}", er->caid);
			cwfound = XCRYPTOWORKS_Process(client->reader, er, cw);
			break;
	#endif
	#if defined(__XCAS_POWERVU__)
		case 0x0E00:
			mycs_debug(D_ADB, "xcas:send_ecm.powervu{%04X}", er->caid);
			cwisextend = 1;
			cwfound = XPVU_Process(client->reader, er, cw, &cwEx);
			break;
	#endif
		default:
			mycs_debug(D_ADB, "xcas:send_ecm.failed{%04X}", er->caid);
			break;
	}

	if (cwfound > 0)
	{
		write_ecm_answer(client->reader, er, E_FOUND, 0, cw,
					(cwisextend) ? &cwEx : NULL, NULL);
	}
	else if (cwfound == 0)
	{
		write_ecm_answer(client->reader, er, E_NOTFOUND, (E1_READER<<4 | E2_SID), NULL, NULL, NULL);
	}
	now = time(NULL);
	client->last = now;
	client->reader->last_s = client->reader->last_g = now;
	return 0;
}

static int32_t
xcas_send_emm(struct emm_packet_t *emm)
{
	int found = 0;
	#if defined(__XCAS_AUTOROLL__)
		uint16_t caid   = b2i(2, emm->caid);
		uint32_t provid = b2i(4, emm->provid);

		mycs_debug(D_ADB, "xcas:send_emm{%02X.%06X}", caid, provid);
		switch (caid)
		{
		#if defined(__XCAS_VIACESS__)
			case 0x0500:
				found = XVIACESS_EmmProcess(emm->client->reader, emm->emm, emm->emmlen, provid);
				break;
		#endif
		#if defined(__XCAS_POWERVU__)
			case 0x0E00:
				found = XPVU_EmmProcess(emm->client->reader, emm->emm, emm->emmlen);
				break;
		#endif
			default:
				break;
		}
	#endif
	return found;
}

static int32_t
xcas_recv_chk(struct s_client *UNUSED(client), uchar *UNUSED(dcw), int32_t *rc, uchar *UNUSED(buf), int32_t UNUSED(n))
{
	MYXCAS_TRACE("xcas:recv_chk\n");
	*rc = 0;
	return -1;
}

static int32_t
xcas_ChCloser(struct s_client *client, int muxid)
{
	MYXCAS_TRACE("xcas:chstop\n");
	if (!client) return -1;
	if (!client->reader) return -1;
	client->emu_casid = 0;
	client->reader->tcp_ito = 0;
	client->reader->ch_descramble = 0;
	XEMUKEY_Cleanup(client->reader);
	return 1;
}

static int32_t
xcas_Cleanup(struct s_client *client)
{
	MYXCAS_TRACE("xcas:cleanup\n");
	if (!client) return -1;
	if (!client->reader) return -1;
	if (XEMUKEY_Initialize(client->reader))
	{
		mycs_log("xcas:file(%s)", client->reader->device);
		client->reader->card_status = CARD_INSERTED;
	}
	else
	{
		mycs_log("xcas:non file(%s)", client->reader->device);
		client->reader->card_status = CARD_FAILURE;
	}
	return 1;
}

void
MODULE_xcas(struct s_module *ph)
{
	MYXCAS_TRACE("MODULE_xcas...\n");
	ph->desc 		 	= "xcas";
	ph->type 		 	= MOD_NO_CONN;
	ph->listenertype	= LIS_XCAS;
	ph->c_available 	= xcas_clAvailable;
	ph->c_init 		 	= xcas_clinit;
	ph->c_idle			= xcas_clidle;
	ph->recv 		 	= xcas_recv;
	ph->c_recv_chk  	= xcas_recv_chk;
	ph->c_send_ecm  	= xcas_send_ecm;
	ph->c_send_emm  	= xcas_send_emm;
	ph->c_ChCloser		= xcas_ChCloser;
	ph->c_Cleanup		= xcas_Cleanup;
	ph->num			 	= R_XCAS;
	ph->large_ecm_support = 1;
}

#endif	// #if defined(MODULE_XCAS)

