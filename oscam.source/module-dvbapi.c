#define MODULE_LOG_PREFIX "dvbapi"

#include "globals.h"

#ifdef HAVE_DVBAPI

#include "module-dvbapi.h"
#include "module-cacheex.h"
#include "module-dvbapi-azbox.h"
#include "module-dvbapi-mca.h"
#include "module-dvbapi-coolapi.h"
#include "module-dvbapi-stapi.h"
#include "module-dvbapi-chancache.h"
#if defined(WITH_HISILICON)
#include "module-dvbapi-his.h"
#endif
#include "module-stat.h"
#include "oscam-chk.h"
#include "oscam-client.h"
#include "oscam-config.h"
#include "oscam-ecm.h"
#include "oscam-emm.h"
#include "oscam-files.h"
#include "oscam-net.h"
#include "oscam-reader.h"
#include "oscam-string.h"
#include "oscam-time.h"
#include "oscam-work.h"
#include "reader-irdeto.h"
#if defined(MODULE_XCAS)
#include "module-xcas.h"
#endif
// sky(quad)
#if 1
	#define	MYDVB_TRACE	mystrace
#else
	#define	MYDVB_TRACE(...)
#endif
#if defined (__CYGWIN__)
#define F_NOTIFY 	0
#define F_SETSIG 	0
#define DN_MODIFY 0
#define DN_CREATE 0
#define DN_DELETE 0
#define DN_MULTISHOT 0
#endif

static int  is_samygo = 0;

void flush_read_fd(int32_t demux_index, int32_t num, int fd)
{
	if(!cfg.dvbapi_listenport && cfg.dvbapi_boxtype != BOXTYPE_PC_NODMX)
	{
		cs_log_dbg(D_DVBAPI,"Demuxer %d flushing stale input data of filter %d (fd:%d)", demux_index, num + 1, fd);
		fd_set rd;
		struct timeval t;
		char buff[100];
		t.tv_sec=0;
		t.tv_usec=0;
		FD_ZERO(&rd);
		FD_SET(fd,&rd);
		while(select(fd+1,&rd,NULL,NULL,&t) > 0)
		{
			 if (read(fd,buff,100)){;}
		}
	}
}

static int dvbapi_ioctl(int fd, uint32_t request, ...)
{
	int ret = 0;
	va_list args;
	va_start(args, request);
	if (!is_samygo)
 	{
		void *param = va_arg(args, void *);
		ret = ioctl(fd, request, param);
	}
	else
	{
		switch(request) {
		   case DMX_SET_FILTER:
		   {
			   struct dmxSctFilterParams *sFP = va_arg(args, struct dmxSctFilterParams *);
			   // prepare packet
			   unsigned char packet[sizeof(request) + sizeof(struct dmxSctFilterParams)];
			   memcpy(&packet, &request, sizeof(request));
			   memcpy(&packet[sizeof(request)], sFP, sizeof(struct dmxSctFilterParams));
			   ret = send(fd, packet, sizeof(packet), 0);
			   break;
		   }
		   case DMX_SET_FILTER1:
		   {
			   struct dmx_sct_filter_params *sFP = va_arg(args, struct dmx_sct_filter_params *);
			   ret = send(fd, sFP, sizeof(struct dmx_sct_filter_params), 0);
			   break;
		   }
		   case DMX_STOP:
		   {
			   ret = send(fd, &request, sizeof(request), 0);
			   ret = 1;
			   break;
		   }
		   case CA_SET_PID:
		   {
			   ret = 1;
			   break;
		   }
		   case CA_SET_DESCR:
		   {
			   ret = 1;
			   break;
		   }
		}
		if (ret > 0) // send() may return larger than 1
			ret = 1;
	}
#if defined(__powerpc__)
	// Old dm500 boxes (ppc old) are using broken kernel, se we need some fixups
	switch (request)
	{
		case DMX_STOP:
		case CA_SET_DESCR:
		case CA_SET_PID:
		ret = 1;
	}
#endif
	// FIXME: Workaround for su980 bug
	// See: http://www.streamboard.tv/wbb2/thread.php?postid=533940
	if (boxtype_is("su980")) ret = 1;
	va_end(args);
	return ret;
}

// tunemm_caid_map
#define FROM_TO 0
#define TO_FROM 1

DEMUXTYPE 		 demux[MAX_DEMUX];
struct s_dvbapi_priority 	*dvbApi_priority;
struct s_client 				*dvbApi_client;

const char *boxtype_desc[] = {"none",
										"dreambox",
										"duckbox",
										"ufs910",
										"dbox2",
										"ipbox",
										"ipbox-pmt",
										"dm7000",
										"qboxhd",
										"coolstream",
										"neumo",
										"pc",
										"hisky", // WITH_HISILICON
										"pc-nodmx",
									};

static const struct box_devices devices[BOX_COUNT] = {
	/* QboxHD(dvb-api-3)*/	{ "/tmp/virtual_adapter/", "ca%d",			"demux%d",		"/tmp/camd.socket", DVBAPI_3  },
	/* dreambox(dvb-api-3)*/{ "/dev/dvb/adapter%d/",	"ca%d", 			"demux%d",		"/tmp/camd.socket", DVBAPI_3 },
	/* dreambox(dvb-api-1)*/{ "/dev/dvb/card%d/",		"ca%d",			"demux%d",		"/tmp/camd.socket", DVBAPI_1 },
	/* neumo(dvb-api-1)*/	{ "/dev/",						"demuxapi",		"demuxapi",		"/tmp/camd.socket", DVBAPI_1 },
	/* sh4(stapi)*/			{ "/dev/stapi/", 				"stpti4_ioctl","stpti4_ioctl","/tmp/camd.socket", STAPI },
	/* coolstream*/			{ "/dev/cnxt/", 				"null",			"null",			"/tmp/camd.socket", COOLAPI },
	// sky(!).
	/* HiSilicon*/				{ "/dev/", 						"null",			"null",			"/var/camd.socket", HISILICONAPI },
};

static int32_t 	selected_box = -1;
static int32_t 	selected_api = -1;
static int32_t 	maxfilter = MAX_FILTER;
static int32_t 	dir_fd = -1;
static char   		*client_name = "hisilicon";
static uint16_t 	client_proto_version = 0;

static int32_t 	pausecam = 0, pmt_stopmarking = 0, pmt_handling = 0;
int32_t				disable_pmt_files = 0;
static int32_t 	ca_fd[MAX_DEMUX]; // holds fd handle of each ca device 0 = not in use
static LLIST * 	ll_activestreampids; // list of all enabled streampids on ca devices

static int32_t 	unassoc_fd[MAX_DEMUX];

bool is_dvbapi_usr(char *usr) {
	return streq(cfg.dvbapi_usr, usr);
}

struct s_emm_filter {
	int32_t 	demux_id;
	uchar 	filter[32];
	uint16_t caid;
	uint32_t	provid;
	uint16_t	pid;
	uint32_t num;
	struct timeb time_started;
};

static LLIST *ll_emm_active_filter;
static LLIST *ll_emm_inactive_filter;
static LLIST *ll_emm_pending_filter;

int32_t add_emmfilter_to_list(int32_t demux_id, uchar *filter, uint16_t caid, uint32_t provid, uint16_t emmpid, int32_t num, bool enable)
{
	if (!ll_emm_active_filter)
		{ ll_emm_active_filter = ll_create("ll_emm_active_filter"); }

	if (!ll_emm_inactive_filter)
		{ ll_emm_inactive_filter = ll_create("ll_emm_inactive_filter"); }

	if (!ll_emm_pending_filter)
		{ ll_emm_pending_filter = ll_create("ll_emm_pending_filter"); }

	struct s_emm_filter *filter_item;
	if (!cs_malloc(&filter_item, sizeof(struct s_emm_filter)))
		{ return 0; }

	filter_item->demux_id= demux_id;
	memcpy(filter_item->filter, filter, 32);
	filter_item->caid		= caid;
	filter_item->provid	= provid;
	filter_item->pid		= emmpid;
	filter_item->num		= num;
	if (enable)
	{
		cs_ftime(&filter_item->time_started);
	}
	else
	{
		memset(&filter_item->time_started, 0, sizeof(filter_item->time_started));
	}

	if (num > 0)
	{
		ll_append(ll_emm_active_filter, filter_item);
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d added to active emmfilters (CAID %04X PROVID %06X EMMPID %04X)",
			filter_item->demux_id, filter_item->num, filter_item->caid, filter_item->provid, filter_item->pid);
	}
	else if (num < 0)
	{
		ll_append(ll_emm_pending_filter, filter_item);
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter added to pending emmfilters (CAID %04X PROVID %06X EMMPID %04X)",
					  filter_item->demux_id, filter_item->caid, filter_item->provid, filter_item->pid);
	}
	else
	{
		ll_append(ll_emm_inactive_filter, filter_item);
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter added to inactive emmfilters (CAID %04X PROVID %06X EMMPID %04X)",
					  filter_item->demux_id, filter_item->caid, filter_item->provid, filter_item->pid);
	}
	return 1;
}

int32_t is_emmfilter_in_list_internal(LLIST *ll, uchar *filter, uint16_t emmpid, uint32_t provid, uint16_t caid)
{
	struct s_emm_filter *filter_item;
	LL_ITER itr;
	if (ll_count(ll) > 0)
	{
		itr = ll_iter_create(ll);
		while((filter_item = ll_iter_next(&itr)) != NULL)
		{
			if (!memcmp(filter_item->filter, filter, 32) && (filter_item->pid == emmpid) && (filter_item->provid == provid) && (filter_item->caid == caid))
				{ return 1; }
		}
	}
	return 0;
}

int32_t is_emmfilter_in_list(uchar *filter, uint16_t emmpid, uint32_t provid, uint16_t caid)
{
	if (!ll_emm_active_filter)
		{ ll_emm_active_filter = ll_create("ll_emm_active_filter"); }

	if (!ll_emm_inactive_filter)
		{ ll_emm_inactive_filter = ll_create("ll_emm_inactive_filter"); }

	if (!ll_emm_pending_filter)
		{ ll_emm_pending_filter = ll_create("ll_emm_pending_filter"); }

	if (is_emmfilter_in_list_internal(ll_emm_active_filter, filter, emmpid, provid, caid))
		{ return 1; }
	if (is_emmfilter_in_list_internal(ll_emm_inactive_filter, filter, emmpid, provid, caid))
		{ return 1; }
	if (is_emmfilter_in_list_internal(ll_emm_pending_filter, filter, emmpid, provid, caid))
		{ return 1; }

	return 0;
}

struct s_emm_filter *get_emmfilter_by_filternum_internal(LLIST *ll, int32_t demux_id, uint32_t num)
{
	struct s_emm_filter *filter;
	LL_ITER itr;

//	MYDVB_TRACE("mydvb:get_emmfilter_by_filternum_internal\n");
	if (ll_count(ll) > 0)
	{
		itr = ll_iter_create(ll);
		while ((filter=ll_iter_next(&itr)))
		{
			if (filter->demux_id == demux_id && filter->num == num)
				{ return filter; }
		}
	}
	return NULL;
}

struct s_emm_filter *get_emmfilter_by_filternum(int32_t demux_id, uint32_t num)
{
//	MYDVB_TRACE("mydvb:get_emmfilter_by_filternum\n");
	if (!ll_emm_active_filter)
		{ ll_emm_active_filter = ll_create("ll_emm_active_filter"); }

	if (!ll_emm_inactive_filter)
		{ ll_emm_inactive_filter = ll_create("ll_emm_inactive_filter"); }

	if (!ll_emm_pending_filter)
		{ ll_emm_pending_filter = ll_create("ll_emm_pending_filter"); }

	struct s_emm_filter *emm_filter = NULL;
	emm_filter = get_emmfilter_by_filternum_internal(ll_emm_active_filter, demux_id, num);
	if (emm_filter) return emm_filter;

	emm_filter = get_emmfilter_by_filternum_internal(ll_emm_inactive_filter, demux_id, num);
	if (emm_filter) return emm_filter;

	emm_filter = get_emmfilter_by_filternum_internal(ll_emm_pending_filter, demux_id, num);
	if (emm_filter) return emm_filter;

	return NULL;
}

int8_t remove_emmfilter_from_list_internal(LLIST *ll, int32_t demux_id, uint16_t caid, uint32_t provid, uint16_t pid, uint32_t num)
{
	struct s_emm_filter *filter;
	LL_ITER itr;
	MYDVB_TRACE("mydvb:remove_emmfilter_from_list_internal\n");
	if (ll_count(ll) > 0)
	{
		itr = ll_iter_create(ll);
		while ((filter=ll_iter_next(&itr)))
		{
			if (filter->demux_id == demux_id && filter->caid == caid && filter->provid == provid && filter->pid == pid && filter->num == num)
			{
				ll_iter_remove_data(&itr);
				return 1;
			}
		}
	}
	return 0;
}

void remove_emmfilter_from_list(int32_t demux_id, uint16_t caid, uint32_t provid, uint16_t pid, uint32_t num)
{
	MYDVB_TRACE("mydvb:remove_emmfilter_from_list\n");
	if (ll_emm_active_filter   && remove_emmfilter_from_list_internal(ll_emm_active_filter, demux_id, caid, provid, pid, num))
		{ return; }
	if (ll_emm_inactive_filter && remove_emmfilter_from_list_internal(ll_emm_inactive_filter, demux_id, caid, provid, pid, num))
		{ return; }
	if (ll_emm_pending_filter  && remove_emmfilter_from_list_internal(ll_emm_pending_filter, demux_id, caid, provid, pid, num))
		{ return; }
}

void dvbapi_net_add_str(unsigned char *packet, int *size, const char *str)
{
	unsigned char *str_len = &packet[*size];                            //string length
	*size += 1;

	*str_len = snprintf((char *) &packet[*size], DVBAPI_MAX_PACKET_SIZE - *size, "%s", str);
	*size += *str_len;
}

int32_t dvbapi_net_send(uint32_t request, int32_t socket_fd, int32_t demux_index, uint32_t filter_number, unsigned char *data, struct s_client *client, ECM_REQUEST *er)
{
	unsigned char packet[DVBAPI_MAX_PACKET_SIZE];                       //maximum possible packet size
	int32_t size = 0;

	// not connected?
	if (socket_fd <= 0)
		return 0;

	// preparing packet - header
	// in old protocol client expect this first byte as adapter index, changed in the new protocol
	// to be always after request type (opcode)
	if (client_proto_version <= 0)
		packet[size++] = demux[demux_index].adapter_index;          //adapter index - 1 byte

	// type of request
	uint32_t req = request;
	if (client_proto_version >= 1)
		req = htonl(req);
	memcpy(&packet[size], &req, 4);                                     //request - 4 bytes
	size += 4;

	// preparing packet - adapter index for proto >= 1
	if ((request != DVBAPI_SERVER_INFO) && client_proto_version >= 1)
		packet[size++] = demux[demux_index].adapter_index;          //adapter index - 1 byte

	// struct with data
	switch (request)
	{
		case DVBAPI_SERVER_INFO:
		{
			int16_t proto_version = htons(DVBAPI_PROTOCOL_VERSION);           //our protocol version
			memcpy(&packet[size], &proto_version, 2);
			size += 2;

			unsigned char *info_len = &packet[size];   //info string length
			size += 1;

			*info_len = snprintf((char *) &packet[size], sizeof(packet) - size, "OSCam v%s, build r%s (%s)", CS_VERSION, CS_SVN_VERSION, CS_TARGET);
			size += *info_len;
			break;
		}
		case DVBAPI_ECM_INFO:
		{
			if (er->rc >= E_NOTFOUND)
				return 0;

			int8_t hops = 0;

			uint16_t sid = htons(er->srvid);                                  //service ID (program number)
			memcpy(&packet[size], &sid, 2);
			size += 2;

			uint16_t caid = htons(er->caid);                                  //CAID
			memcpy(&packet[size], &caid, 2);
			size += 2;

			uint16_t pid = htons(er->pid);                                    //PID
			memcpy(&packet[size], &pid, 2);
			size += 2;

			uint32_t prid = htonl(er->prid);                                  //Provider ID
			memcpy(&packet[size], &prid, 4);
			size += 4;

			uint32_t ecmtime = htonl(client->cwlastresptime);                 //ECM time
			memcpy(&packet[size], &ecmtime, 4);
			size += 4;

			dvbapi_net_add_str(packet, &size, get_cardsystem_desc_by_caid(er->caid));  //cardsystem name

			switch (er->rc)
			{
				case E_FOUND:
					if (er->selected_reader)
					{
						dvbapi_net_add_str(packet, &size, er->selected_reader->label);                   //reader
						if (is_network_reader(er->selected_reader))
							dvbapi_net_add_str(packet, &size, er->selected_reader->device);          //from
						else
							dvbapi_net_add_str(packet, &size, "local");                              //from
						dvbapi_net_add_str(packet, &size, reader_get_type_desc(er->selected_reader, 1)); //protocol
						hops = er->selected_reader->currenthops;
					}
					break;

				case E_CACHE1:
					dvbapi_net_add_str(packet, &size, "Cache");       //reader
					dvbapi_net_add_str(packet, &size, "cache1");      //from
					dvbapi_net_add_str(packet, &size, "none");        //protocol
					break;

				case E_CACHE2:
					dvbapi_net_add_str(packet, &size, "Cache");       //reader
					dvbapi_net_add_str(packet, &size, "cache2");      //from
					dvbapi_net_add_str(packet, &size, "none");        //protocol
					break;

				case E_CACHEEX:
					dvbapi_net_add_str(packet, &size, "Cache");       //reader
					dvbapi_net_add_str(packet, &size, "cache3");      //from
					dvbapi_net_add_str(packet, &size, "none");        //protocol
					break;
			}

			packet[size++] = hops;                                  //hops

			break;
		}
		case DVBAPI_CA_SET_PID:
		{
			int sct_capid_size = sizeof(ca_pid_t);

			if (client_proto_version >= 1)
			{
				ca_pid_t *capid = (ca_pid_t *) data;
				capid->pid = htonl(capid->pid);
				capid->index = htonl(capid->index);
			}
			memcpy(&packet[size], data, sct_capid_size);

			size += sct_capid_size;
			break;
		}
		case DVBAPI_CA_SET_DESCR:
		{
			int sct_cadescr_size = sizeof(ca_descr_t);

			if (client_proto_version >= 1)
			{
				ca_descr_t *cadesc = (ca_descr_t *) data;
				cadesc->index = htonl(cadesc->index);
				cadesc->parity = htonl(cadesc->parity);
			}
			memcpy(&packet[size], data, sct_cadescr_size);

			size += sct_cadescr_size;
			break;
		}
		case DVBAPI_DMX_SET_FILTER:
		case DVBAPI_DMX_STOP:
		{
			int32_t sct_filter_size = sizeof(struct dmx_sct_filter_params);
			packet[size++] = demux_index;                               //demux index - 1 byte
			packet[size++] = filter_number;                             //filter number - 1 byte

			if (data)       // filter data when starting
			{
				if (client_proto_version >= 1)
				{
					struct dmx_sct_filter_params *fp = (struct dmx_sct_filter_params *) data;

					// adding all dmx_sct_filter_params structure fields
					// one by one to avoid padding problems
					uint16_t pid = htons(fp->pid);
					memcpy(&packet[size], &pid, 2);
					size += 2;

					memcpy(&packet[size], fp->filter.filt, 16);
					size += 16;
					memcpy(&packet[size], fp->filter.mask, 16);
					size += 16;
					memcpy(&packet[size], fp->filter.mode, 16);
					size += 16;

					uint32_t timeout = htonl(fp->timeout);
					memcpy(&packet[size], &timeout, 4);
					size += 4;

					uint32_t flags = htonl(fp->flags);
					memcpy(&packet[size], &flags, 4);
					size += 4;
				}
				else
				{
					memcpy(&packet[size], data, sct_filter_size);       //dmx_sct_filter_params struct
					size += sct_filter_size;
				}
			}
			else            // pid when stopping
			{
				if (client_proto_version >= 1)
				{
					uint16_t pid = htons(demux[demux_index].demux_fd[filter_number].pid);
					memcpy(&packet[size], &pid, 2);
					size += 2;
				}
				else
				{
					uint16_t pid = demux[demux_index].demux_fd[filter_number].pid;
					packet[size++] = pid >> 8;
					packet[size++] = pid & 0xff;
				}
			}
			break;
		}
		default:  //unknown request
		{
			cs_log("ERROR: dvbapi_net_send: invalid request");
			return 0;
		}
	}

	// sending
	cs_log_dump_dbg(D_DVBAPI, packet, size, "Sending packet to dvbapi client (fd=%d):", socket_fd);
	send(socket_fd, &packet, size, MSG_DONTWAIT);

	// always returning success as the client could close socket
	return 0;
}

int32_t dvbapi_set_filter(int32_t demux_id, int32_t api, uint16_t pid, uint16_t caid, uint32_t provid, uchar *filt, uchar *mask, int32_t timeout, int32_t pidx, int32_t type,
					int8_t add_to_emm_list)
{
#if defined WITH_AZBOX || defined WITH_MCA
	openxcas_set_caid(demux[demux_id].ECMpids[pidindex].CAID);
	openxcas_set_ecm_pid(pid);
	if (USE_OPENXCAS) return 1;
#endif

	int32_t ret=-1,n=-1,i;

	for (i=0; i<maxfilter && demux[demux_id].demux_fd[i].fd>0; i++);

	MYDVB_TRACE("mydvb:dvbapi_set_filter{%d:%d, %d,%04X,%06X:%04X}\n", demux_id, i, type, caid, provid, pid);
	if (i>=maxfilter) {
		cs_log_dbg(D_DVBAPI, "no free filter");
		return -1;
	}
	n = i;
// sky(move)
//	demux[demux_id].demux_fd[n].pidindex = pidx;
//	demux[demux_id].demux_fd[n].pid      = pid;
//	demux[demux_id].demux_fd[n].caid     = caid;
//	demux[demux_id].demux_fd[n].provid   = provid;
//	demux[demux_id].demux_fd[n].type     = type;

	switch (api)
	{
		case DVBAPI_3:
		if (cfg.dvbapi_listenport || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
				ret = demux[demux_id].demux_fd[n].fd = DUMMY_FD;
			else
				ret = demux[demux_id].demux_fd[n].fd = dvbapi_open_device(0, demux[demux_id].demux_index, demux[demux_id].adapter_index);
			if (ret < 0) { return ret; }  // return if device cant be opened!
			struct dmx_sct_filter_params sFP2;

			memset(&sFP2,0,sizeof(sFP2));

			sFP2.pid	 	 = pid;
			sFP2.timeout = timeout;
			sFP2.flags   = DMX_IMMEDIATE_START;
			if (cfg.dvbapi_boxtype == BOXTYPE_NEUMO) {
				//DeepThought: on dgs/cubestation and neumo images, perhaps others
				//the following code is needed to descramble
				sFP2.filter.filt[0]=filt[0];
				sFP2.filter.mask[0]=mask[0];
				sFP2.filter.filt[1]=0;
				sFP2.filter.mask[1]=0;
				sFP2.filter.filt[2]=0;
				sFP2.filter.mask[2]=0;
				memcpy(sFP2.filter.filt+3,filt+1,16-3);
				memcpy(sFP2.filter.mask+3,mask+1,16-3);
				//DeepThought: in the drivers of the dgs/cubestation and neumo images,
				//dvbapi 1 and 3 are somehow mixed. In the kernel drivers, the DMX_SET_FILTER
				//ioctl expects to receive a dmx_sct_filter_params structure (DVBAPI 3) but
				//due to a bug its sets the "positive mask" wrongly (they should be all 0).
				//On the other hand, the DMX_SET_FILTER1 ioctl also uses the dmx_sct_filter_params
				//structure, which is incorrect (it should be  dmxSctFilterParams).
				//The only way to get it right is to call DMX_SET_FILTER1 with the argument
				//expected by DMX_SET_FILTER. Otherwise, the timeout parameter is not passed correctly.
				ret = dvbapi_ioctl(demux[demux_id].demux_fd[n].fd, DMX_SET_FILTER1, &sFP2);
			}
			else {
				memcpy(sFP2.filter.filt,filt,16);
				memcpy(sFP2.filter.mask,mask,16);
				if (cfg.dvbapi_listenport || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
				ret = dvbapi_net_send(DVBAPI_DMX_SET_FILTER, demux[demux_id].socket_fd, demux_id, n, (unsigned char *) &sFP2, NULL, NULL);
				else
					ret = dvbapi_ioctl(demux[demux_id].demux_fd[n].fd, DMX_SET_FILTER, &sFP2);
			}
			break;

		case DVBAPI_1:
			ret = demux[demux_id].demux_fd[n].fd = dvbapi_open_device(0, demux[demux_id].demux_index, demux[demux_id].adapter_index);
			if (ret < 0) { return ret; }  // return if device cant be opened!
			struct dmxSctFilterParams sFP1;

			memset(&sFP1,0,sizeof(sFP1));

			sFP1.pid	 	 = pid;
			sFP1.timeout = timeout;
			sFP1.flags	 = DMX_IMMEDIATE_START;
			memcpy(sFP1.filter.filt,filt,16);
			memcpy(sFP1.filter.mask,mask,16);
			ret = dvbapi_ioctl(demux[demux_id].demux_fd[n].fd, DMX_SET_FILTER1, &sFP1);

			break;

#if defined(WITH_HISILICON)
		case HISILICONAPI:
			//	demux[demux_id].demux_fd[n].fd = 1;
			// sky(quad)
			ret = hidemuxapi_AddFilters(demux[demux_id].adapter_index, demux[demux_id].demux_index, type, n, pid, filt, mask);
			if (ret > 0) {
				demux[demux_id].demux_fd[n].fd = ret;
			}
			else {
				ret = -1; // error setting filter!
			}
			break;
#endif

#ifdef WITH_STAPI
		case STAPI:
			ret = stapi_set_filter(demux_id, pid, filt, mask, n, demux[demux_id].pmt_file);
			if (ret != 0)
				{ demux[demux_id].demux_fd[n].fd = ret; }
			else
				{ ret = -1; } // error setting filter!
			break;
#endif
#ifdef WITH_COOLAPI
		case COOLAPI:
			demux[demux_id].demux_fd[n].fd = coolapi_open_device(demux[demux_id].demux_index, demux_id);
			if (demux[demux_id].demux_fd[n].fd > 0)
				{ ret = coolapi_set_filter(demux[demux_id].demux_fd[n].fd, n, pid, filt, mask, type); }
			break;
#endif
		default:
			break;
	}
	if (ret < 0)
	{
		cs_log("ERROR: Could not start demux filter (api: %d errno=%d %s)", selected_api, errno, strerror(errno));
		return (ret);
	}
	// sky(move)
	demux[demux_id].demux_fd[n].pidindex = pidx;
	demux[demux_id].demux_fd[n].pid      = pid;
	demux[demux_id].demux_fd[n].caid     = caid;
	demux[demux_id].demux_fd[n].provid   = provid;
	demux[demux_id].demux_fd[n].type     = type;

	cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d started successfully (caid %04X provid %06X pid %04X)", demux_id, n + 1, caid, provid, pid);
	if (type == TYPE_EMM && add_to_emm_list) {
		add_emmfilter_to_list(demux_id, filt, caid, provid, pid, n+1, true);
	}
	return (n);
}

static int32_t dvbapi_detect_api(void)
{
#if defined(WITH_HISILICON)
	selected_api		 = HISILICONAPI;
	selected_box 		 = 6;
	disable_pmt_files  = 1;
	cfg.dvbapi_pmtmode = 1;
	cfg.dvbapi_boxtype = BOXTYPE_HISILICON;
	cfg.dvbapi_listenport = 0; // TCP port to listen instead of camd.socket
	if (hidemuxapi_Init() == 0)
	{
		cs_log("ERROR: dvbiapi: setting up hisky failed.");
		return 0;
	}
	cs_log("Detected Hisilicon{%d}.", cfg.dvbapi_pmtmode);
	MYDVB_TRACE("mydvb:Hisilicon\n");
	return 1;
#elif defined(WITH_COOLAPI)
	selected_api = COOLAPI;
	selected_box = 5;
	disable_pmt_files = 1;
	cfg.dvbapi_listenport = 0;
	cs_log("Detected Coolstream API");
	return 1;
#else
	if (cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX || cfg.dvbapi_boxtype == BOXTYPE_PC ) {
		selected_api = DVBAPI_3;
		selected_box = 1;
	   if (cfg.dvbapi_listenport)
	   {
			cs_log("Using TCP listen socket, API forced to DVBAPIv3 (%d), userconfig boxtype: %d", selected_api, cfg.dvbapi_boxtype);
	   }
		else
		{
			cs_log("Using %s listen socket, API forced to DVBAPIv3 (%d), userconfig boxtype: %d", devices[selected_box].cam_socket_path, selected_api, cfg.dvbapi_boxtype);
		}
		return 1;
	}
	else
	{
		cfg.dvbapi_listenport = 0;
	}

	int32_t i = 0, n = 0, devnum = -1, dmx_fd = 0, boxnum = sizeof(devices) / sizeof(struct box_devices);
	char device_path[128], device_path2[128];

	while (i < boxnum)
	{
		snprintf(device_path2, sizeof(device_path2), devices[i].demux_device, 0);
		snprintf(device_path,  sizeof(device_path),  devices[i].path, n);
		strncat(device_path, device_path2, sizeof(device_path) - strlen(device_path)-1);
		// FIXME: *THIS SAMYGO CHECK IS UNTESTED*
		// FIXME: Detect samygo, checking if default DVBAPI_3 device paths are sockets
		if (i == 1) { // We need boxnum 1 only
			struct stat sb;
			if (stat(device_path, &sb) > 0 && S_ISSOCK(sb.st_mode)) {
				selected_box = 0;
				disable_pmt_files = 1;
				is_samygo = 1;
				devnum = i;
				break;
			}
		}
		if ((dmx_fd = open(device_path, O_RDWR | O_NONBLOCK)) > 0)
		{
			devnum=i;
			int32_t ret = close(dmx_fd);
			if (ret < 0) { cs_log("ERROR: Could not close demuxer fd (errno=%d %s)", errno, strerror(errno)); }
			break;
		}
		/* try at least 8 adapters */
		if ((strchr(devices[i].path, '%') != NULL) && (n < 8)) n++; else { n = 0; i++; }
	}

	if (devnum == -1) { return 0; }
	selected_box = devnum;
	if (selected_box > -1)
		{ selected_api = devices[selected_box].api; }

#ifdef WITH_STAPI
	if (devnum == 4 && stapi_open() == 0)
	{
		cs_log("ERROR: stapi: setting up stapi failed.");
		return 0;
	}
#endif
	if (cfg.dvbapi_boxtype == BOXTYPE_NEUMO)
	{
		selected_api = DVBAPI_3; //DeepThought
	}
	cs_log("Detected %s Api: %d, userconfig boxtype: %d", device_path, selected_api, cfg.dvbapi_boxtype);
#endif
	return 1;
}

static int32_t dvbapi_read_device(int32_t dmx_fd, unsigned char *buf, int32_t length)
{
	int32_t len, rc;
	struct pollfd pfd[1];

	pfd[0].fd = dmx_fd;
	pfd[0].events = (POLLIN | POLLPRI);

	rc = poll(pfd, 1, 7000);
	if (rc < 1)
	{
		cs_log("ERROR: Read on %d timed out (errno=%d %s)", dmx_fd, errno, strerror(errno));
		return -1;
	}

	len = read(dmx_fd, buf, length);

	if (len<1)
	{
		if (errno == EOVERFLOW)
		{
			cs_log("fd %d no valid data present since receiver reported an internal bufferoverflow!", dmx_fd);
			return 0;
		}
		else if (errno != EBADF && errno !=EINVAL) // dont throw errors on invalid fd or invalid argument
		{
			cs_log("ERROR: Read error on fd %d (errno=%d %s)", dmx_fd, errno, strerror(errno));
		}
	}
	else { cs_log_dump_dbg(D_TRACE, buf, len, "Readed:"); }
	return len;
}

int32_t dvbapi_open_device(int32_t type, int32_t num, int32_t adapter)
{
	int32_t dmx_fd;
	int32_t ca_offset = 0;
	int32_t ret = 0;
	char device_path[128], device_path2[128];

	if (cfg.dvbapi_listenport || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
		return DUMMY_FD;

	if (type == 0)
	{
		snprintf(device_path2, sizeof(device_path2), devices[selected_box].demux_device, num);
		snprintf(device_path,  sizeof(device_path),  devices[selected_box].path, adapter);

		strncat(device_path, device_path2, sizeof(device_path)-strlen(device_path)-1);
	}
	else
	{
		if (cfg.dvbapi_boxtype==BOXTYPE_DUCKBOX || cfg.dvbapi_boxtype==BOXTYPE_DBOX2 || cfg.dvbapi_boxtype==BOXTYPE_UFS910)
			{ ca_offset = 1; }

		if (cfg.dvbapi_boxtype==BOXTYPE_QBOXHD)
			{ num = 0; }

		if (cfg.dvbapi_boxtype==BOXTYPE_PC)
			{ num = 0; }

#if defined(WITH_HISILICON)
		if (cfg.dvbapi_boxtype==BOXTYPE_HISILICON) num = 0;
#endif

		snprintf(device_path2, sizeof(device_path2), devices[selected_box].ca_device, num+ca_offset);
		snprintf(device_path,  sizeof(device_path),  devices[selected_box].path, adapter);

		strncat(device_path, device_path2, sizeof(device_path)-strlen(device_path)-1);
	}

#if defined(WITH_HISILICON)
	if (cfg.dvbapi_boxtype==BOXTYPE_HISILICON) {
	   struct sockaddr_un saddr;
 	   memset(&saddr, 0, sizeof(saddr));
	   saddr.sun_family = AF_UNIX;
	   strncpy(saddr.sun_path, device_path, sizeof(saddr.sun_path) - 1);
      dmx_fd = socket(AF_UNIX, SOCK_STREAM, 0);
		ret = connect(dmx_fd, (struct sockaddr *)&saddr, sizeof(saddr));
		if (ret < 0) close(dmx_fd);
	}
	else
#endif
	if (is_samygo) {
	   struct sockaddr_un saddr;
 	   memset(&saddr, 0, sizeof(saddr));
	   saddr.sun_family = AF_UNIX;
	   strncpy(saddr.sun_path, device_path, sizeof(saddr.sun_path) - 1);
      dmx_fd = socket(AF_UNIX, SOCK_STREAM, 0);
		ret = connect(dmx_fd, (struct sockaddr *)&saddr, sizeof(saddr));
		if (ret < 0)
			close(dmx_fd);
	}
	else {
	   MYDVB_TRACE("mydvb:dvbapi_open_device{%s}\n", device_path);
		dmx_fd = ret = open(device_path, O_RDWR | O_NONBLOCK);
   }

	if (ret < 0)
	{
		cs_log("ERROR: Can't open device %s (errno=%d %s)", device_path, errno, strerror(errno));
		return -1;
	}

	cs_log_dbg(D_DVBAPI, "Open device %s (fd %d)", device_path, dmx_fd);

	return dmx_fd;
}

int32_t dvbapi_open_netdevice(int32_t UNUSED(type), int32_t UNUSED(num), int32_t adapter)
{
	int32_t socketfd;

	socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketfd == -1) {
		cs_log("ERROR: Failed create socket (%d %s)", errno, strerror(errno));
	}
	else {
		struct sockaddr_in saddr;
		set_nonblock(socketfd, true);
		bzero(&saddr, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(PORT + adapter); // port = PORT + adapter number
		saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		int32_t r = connect(socketfd, (struct sockaddr *) &saddr, sizeof(saddr));
		if (r<0) {
			cs_log("ERROR: Failed to connect socket (%d %s), at localhost, port=%d", errno, strerror(errno), PORT + adapter);
			int32_t ret = close(socketfd);
			if (ret < 0) cs_log("ERROR: Could not close socket fd (errno=%d %s)", errno, strerror(errno));
			socketfd = -1;
		}
	}

	MYDVB_TRACE("mydvb:netdevice(port:%d)(fd.%d)\n", PORT + adapter, socketfd);
	cs_log_dbg(D_DVBAPI, "NET DEVICE open (port = %d) fd %d", PORT + adapter, socketfd);
	return socketfd;
}

uint16_t tunemm_caid_map(uint8_t direct, uint16_t caid, uint16_t srvid)
{
	int32_t i;
	struct s_client *cl = cur_client();
	TUNTAB *ttab = &cl->ttab;

	if (!ttab->ttnum)
		return caid;

	if (direct)
	{
		for(i = 0; i < ttab->ttnum; i++)
		{
			if (caid == ttab->ttdata[i].bt_caidto
					&& (srvid == ttab->ttdata[i].bt_srvid || ttab->ttdata[i].bt_srvid == 0xFFFF || !ttab->ttdata[i].bt_srvid))
				{ return ttab->ttdata[i].bt_caidfrom; }
		}
	}
	else
	{
		for(i = 0; i < ttab->ttnum; i++)
		{
			if (caid == ttab->ttdata[i].bt_caidfrom
					&& (srvid == ttab->ttdata[i].bt_srvid || ttab->ttdata[i].bt_srvid == 0xFFFF || !ttab->ttdata[i].bt_srvid))
				{ return ttab->ttdata[i].bt_caidto; }
		}
	}
	return caid;
}

int32_t dvbapi_stop_filter(int32_t demux_index, int32_t type)
{
	int32_t g, ret = -1;

	for(g = 0; g < MAX_FILTER; g++) // just stop them all, we dont want to risk leaving any stale filters running due to lowering of maxfilters
	{
		if (demux[demux_index].demux_fd[g].type==type)
		{
			MYDVB_TRACE("mydvb:dvbapi_stop_filter{%s.%d}\n", (type==TYPE_ECM) ? "ecm":"emm", g);
			ret = dvbapi_stop_filternum(demux_index, g);
		}
	}
	if (ret == -1) return 0; // on error return 0
	else return 1;
}

int32_t dvbapi_stop_filternum(int32_t demux_index, int32_t num)
{
	int32_t retfilter = -1, retfd = -1, fd = demux[demux_index].demux_fd[num].fd;
	if (fd > 0)
	{
		cs_log_dbg(D_DVBAPI, "Demuxer %d stop Filter %d (fd: %d api: %d, caid: %04X, provid: %06X, %spid: %04X)",
				demux_index, num+1, fd, selected_api, demux[demux_index].demux_fd[num].caid, demux[demux_index].demux_fd[num].provid,
				(demux[demux_index].demux_fd[num].type==TYPE_ECM ?"ecm":"emm"), demux[demux_index].demux_fd[num].pid);

		switch (selected_api)
		{
			case DVBAPI_3:
				if (cfg.dvbapi_listenport || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
					retfilter = dvbapi_net_send(DVBAPI_DMX_STOP, demux[demux_index].socket_fd, demux_index, num, NULL, NULL, NULL);
				else
					retfilter = dvbapi_ioctl(fd, DMX_STOP, NULL);
				break;

			case DVBAPI_1:
				retfilter = dvbapi_ioctl(fd, DMX_STOP, NULL);
				break;

#ifdef WITH_STAPI
			case STAPI:
				retfilter = stapi_remove_filter(demux_index, num, demux[demux_index].pmt_file);
				if (retfilter != 1)   // stapi returns 0 for error, 1 for all ok
				{
					retfilter = -1;
				}
				break;
#endif
#ifdef WITH_COOLAPI
			case COOLAPI:
				retfilter = coolapi_remove_filter(fd, num);
				retfd = coolapi_close_device(fd);
				break;
#endif
#if defined(WITH_HISILICON)
			case HISILICONAPI:
		      MYDVB_TRACE("mydvb:dvbapi_stop_filternum{%s:%d.%04X}\n",
		      		(demux[demux_index].demux_fd[num].type==TYPE_ECM) ? "ecm":"emm" ,
		      		num, demux[demux_index].demux_fd[num].pid);
		      retfilter = hidemuxapi_RemoveFilters(demux[demux_index].adapter_index,
				      				demux_index,
								      demux[demux_index].demux_fd[num].type,
								      num,
								      demux[demux_index].demux_fd[num].pid);
				if (retfilter < 1) retfilter = -1;
				retfd = 0;
				break;
#endif
			default:
				break;
		}
		if (retfilter < 0)
		{
			cs_log("ERROR: Demuxer %d could not stop Filter %d (fd:%d api:%d errno=%d %s)", demux_index, num + 1, fd, selected_api, errno, strerror(errno));
		}
#if defined(WITH_HISILICON)
#else
#ifndef WITH_COOLAPI // no fd close for coolapi and stapi, all others do close fd!
		if (!cfg.dvbapi_listenport && cfg.dvbapi_boxtype != BOXTYPE_PC_NODMX)
		{
			if (selected_api == STAPI) { retfd = 0; }  // stapi closes its own filter fd!
			else
			{
				flush_read_fd(demux_index, num, fd); // flush filter input buffer in attempt to avoid overflow receivers internal buffer
			 	retfd = close(fd);
			 	if (errno == 9) { retfd = 0; }  // no error on bad file descriptor
			}
		}
		else
			retfd = 0;
#endif
		if (retfd)
		{
			cs_log("ERROR: Demuxer %d could not close fd of Filter %d (fd=%d api:%d errno=%d %s)", demux_index, num + 1, fd,
				selected_api, errno, strerror(errno));
		}
#endif

		if (demux[demux_index].demux_fd[num].type == TYPE_ECM) // ecm filter stopped: reset index!
		{
// sky(n)
#if defined(__DVCCRC_AVAILABLE__)
			DVBCRC_Remove(demux_index, demux[demux_index].demux_fd[num].pid);
#endif

#if defined(WITH_HISILICON)
			demux[demux_index].ECMpids[demux[demux_index].demux_fd[num].pidindex].index = 0;
#else
			int32_t oldpid = demux[demux_index].demux_fd[num].pidindex;
			int32_t curpid = demux[demux_index].pidindex;
			int32_t idx = demux[demux_index].ECMpids[oldpid].index;
			demux[demux_index].ECMpids[oldpid].index = 0;
			if(idx) // if in use
			{
				int32_t i;
				for(i = 0; i < demux[demux_index].STREAMpidcount; i++)
				{
					int8_t match = 0;
					// check streams of old disabled ecmpid
					if(!demux[demux_index].ECMpids[oldpid].streams || ((demux[demux_index].ECMpids[oldpid].streams & (1 << i)) == (uint) (1 << i)))
					{
						// check if new ecmpid is using same streams
						if(curpid != -1 && (!demux[demux_index].ECMpids[curpid].streams || ((demux[demux_index].ECMpids[curpid].streams & (1 << i)) == (uint) (1 << i))))
						{
							continue; // found same stream on old and new ecmpid -> skip! (and leave it enabled!)
						}
						int32_t pidtobestopped = demux[demux_index].STREAMpids[i];
						int32_t j, k, otherdemuxpid, otherdemuxidx;

						for(j = 0; j < MAX_DEMUX; j++) // check other demuxers for same streampid with same index
						{
							if(demux[j].program_number == 0) { continue; }  					// skip empty demuxers
							if(demux_index == j) { continue; } 									// skip same demuxer
							if(demux[j].ca_mask != demux[demux_index].ca_mask) { continue;}		// skip streampid running on other ca device

							otherdemuxpid = demux[j].pidindex;
							if(otherdemuxpid == -1) { continue; }          						// Other demuxer not descrambling yet

							otherdemuxidx = demux[j].ECMpids[otherdemuxpid].index;
							if(!otherdemuxidx || otherdemuxidx != idx) { continue; } 			// Other demuxer has no index yet, or index is different

							for(k = 0; k < demux[j].STREAMpidcount; k++)
							{
								if(!demux[j].ECMpids[otherdemuxpid].streams || ((demux[j].ECMpids[otherdemuxpid].streams & (1 << k)) == (uint) (1 << k)))
								{
									if(demux[j].STREAMpids[k] == pidtobestopped)
									{
										continue; // found same streampid enabled with same index on one or more other demuxers -> skip! (and leave it enabled!)
									}
								}
								match = 1; // matching stream found
							}
						}

						if(!match)
						{
							dvbapi_set_pid(demux_index, i, idx - 1, false); // disable streampid since its not used by this pid (or by the new ecmpid or any other demuxer!)
						}
					}
				}
			}
#endif
		}

		if (demux[demux_index].demux_fd[num].type == TYPE_EMM)   // If emm type remove from emm filterlist
		{
			remove_emmfilter_from_list(demux_index, demux[demux_index].demux_fd[num].caid, demux[demux_index].demux_fd[num].provid, demux[demux_index].demux_fd[num].pid, num+1);
		}
		demux[demux_index].demux_fd[num].fd   = 0;
		demux[demux_index].demux_fd[num].type = 0;
	}
	if (retfilter < 0) { return retfilter; }  // error on remove filter
	if (retfd < 0) { return retfd; }  // error on close filter fd
	return 1; // all ok!
}

int32_t dvbapi_start_filter(int32_t demux_id, int32_t pidx, uint16_t pid, uint16_t caid, uint32_t provid, uchar table, uchar mask, int32_t timeout, int32_t type)
{
	uchar filter[32];
	int32_t filnum;
	int32_t o;

//	MYDVB_TRACE("mydvb:dvbapi_start_filter{%d}{%04X.%06X,%04x}\n", type, caid, provid, pid);
	for(o = 0; o < maxfilter; o++)    // check if ecmfilter is in use & stop all ecmfilters of lower status pids
	{
		if(demux[demux_id].demux_fd[o].fd > 0 &&
			demux[demux_id].demux_fd[o].pid == pid &&
			demux[demux_id].demux_fd[o].type == type && type == TYPE_ECM)
		{
			return demux[demux_id].cs_filtnum;
		}
	}

	memset(filter,0,32);
	filter[ 0]= table;
	filter[16]= mask;

	cs_log_dbg(D_DVBAPI, "Demuxer %d try to start new filter for caid: %04X, provid: %06X, pid: %04X", demux_id, caid, provid, pid);
	filnum = dvbapi_set_filter(demux_id,
						selected_api,
						pid,
						caid,
						provid,
						filter,
						filter+16,
						timeout,
						pidx,
						type,
						0);
	demux[demux_id].cs_filtnum = filnum;
	return (filnum);
}

int32_t dvbapi_start_emm_filter(int32_t demux_index)
{
	if (!demux[demux_index].EMMpidcount) return 0;
//	if ( demux[demux_index].emm_filter)  return 0;

	struct s_csystem_emm_filter *dmx_filter = NULL;
	unsigned int filter_count = 0;
	uint16_t caid, ncaid;
	uint32_t provid;

	struct s_reader *rdr = NULL;
	struct s_client *cl  = cur_client();

	if (!cl) return 0;
	if (!cl->aureader_list)
	{
		mycs_trace(D_ADB,"mydvb:dvbapi_start_emm_filter(%c.aureader_list none)", cl->typ);
		return 0;
	}

	LL_ITER itr = ll_iter_create(cl->aureader_list);
	while ((rdr = ll_iter_next(&itr)))
	{
		if (rdr->audisabled || !rdr->enable || (!is_network_reader(rdr) && rdr->card_status != CARD_INSERTED))
			{ continue; }
		if (IS_ICS_READERS(rdr)) continue;
		// sky(powervu)
		if (IS_CONSTCW_READERS(rdr)) continue;

		const struct s_cardsystem *csystem;
		uint16_t c, match;
		cs_log_dbg(D_DVBAPI, "Demuxer %d matching reader %s against available emmpids -> START!", demux_index, rdr->label);
		for (c = 0; c < demux[demux_index].EMMpidcount; c++)
		{
			caid = ncaid = demux[demux_index].EMMpids[c].CAID;
			if (!caid) continue;
			// sky(powervu)
			if (IS_XCAS_READERS(rdr)) {
				if (!xcas_IsAuAvailable(rdr, caid, 0)) continue;
			}

			if (chk_is_betatunnel_caid(caid) == 2)
			{
				ncaid = tunemm_caid_map(FROM_TO, caid, demux[demux_index].program_number);
			}
			provid = demux[demux_index].EMMpids[c].PROVID;
			if (caid == ncaid)
			{
				match = emm_reader_match(rdr, caid, provid);
			}
			else
			{
				match = emm_reader_match(rdr, ncaid, provid);
			}
			if (match)
			{
				csystem = get_cardsystem_by_caid(caid);
				if (csystem)
				{
					if (caid != ncaid)
				   {
						csystem = get_cardsystem_by_caid(ncaid);
						if (csystem && csystem->get_tunemm_filter)
				   	{
							csystem->get_tunemm_filter(rdr, &dmx_filter, &filter_count);
							cs_log_dbg(D_DVBAPI, "Demuxer %d setting emm filter for betatunnel: %04X -> %04X", demux_index, ncaid, caid);
						}
						else
						{
							cs_log_dbg(D_DVBAPI, "Demuxer %d cardsystem for emm filter for caid %04X of reader %s not found", demux_index, ncaid, rdr->label);
							continue;
						}
				   }
					else if (csystem->get_emm_filter)
				   {
						csystem->get_emm_filter(rdr, &dmx_filter, &filter_count);
				   }
				}
				else
				{
					cs_log_dbg(D_DVBAPI, "Demuxer %d cardsystem for emm filter for caid %04X of reader %s not found", demux_index, caid, rdr->label);
					continue;
				}

				int j;
			   for (j = 0; j < (int)filter_count; j++)
			   {
				   if (dmx_filter[j].enabled == 0) continue;

			      uchar filter[32];
			      memset(filter, 0, sizeof(filter)); // reset filter
			      uint32_t usefilterbytes = 16; // default use all filters
			      memcpy(filter, dmx_filter[j].filter, usefilterbytes);
			      memcpy(filter + 16, dmx_filter[j].mask, usefilterbytes);
			      int32_t emmtype = dmx_filter[j].type;

					if (filter[0] && (((1 << (filter[0] % 0x80)) & rdr->b_nano) && !((1 << (filter[0] % 0x80)) & rdr->s_nano)))
					{
						cs_log_dbg(D_DVBAPI, "Demuxer %d reader %s emmfilter %d/%d blocked by userconfig -> SKIP!", demux_index, rdr->label, j+1, filter_count);
						continue;
					}

				   if ((rdr->blockemm & emmtype) && !(((1<<(filter[0] % 0x80)) & rdr->s_nano) || (rdr->saveemm & emmtype)))
					{
						cs_log_dbg(D_DVBAPI, "Demuxer %d reader %s emmfilter %d/%d blocked by userconfig -> SKIP!", demux_index, rdr->label, j+1, filter_count);
						continue;
					}

					if (demux[demux_index].EMMpids[c].type & emmtype)
					{
						cs_log_dbg(D_DVBAPI, "Demuxer %d reader %s emmfilter %d/%d type match -> ENABLE!", demux_index, rdr->label, j+1, filter_count);
						check_add_emmpid(demux_index, filter, c, emmtype);
					}
					else
					{
						cs_log_dbg(D_DVBAPI, "Demuxer %d reader %s emmfilter %d/%d type mismatch -> SKIP!", demux_index, rdr->label, j+1, filter_count);
					}
				}

				// dmx_filter not use below this point;
			   NULLFREE(dmx_filter);
			}
		}
		cs_log_dbg(D_DVBAPI, "Demuxer %d matching reader %s against available emmpids -> DONE!", demux_index, rdr->label);
	}
	if (demux[demux_index].emm_filter == -1) // first run -1
	{
		demux[demux_index].emm_filter = 0;
	}
	cs_log_dbg(D_DVBAPI, "Demuxer %d handles %i emm filters", demux_index, demux[demux_index].emm_filter);
	return (demux[demux_index].emm_filter);
}

void dvbapi_add_ecmpid_int(int32_t demux_id, uint16_t caid, uint16_t ecmpid, uint32_t provid)
{
	int32_t n,added=0;

	if (demux[demux_id].ECMpidcount >= ECM_PIDS)
		{ return; }

	int32_t stream = demux[demux_id].STREAMpidcount-1;
	for (n=0; n<demux[demux_id].ECMpidcount; n++)
	{
		if (stream > -1 && demux[demux_id].ECMpids[n].CAID == caid && demux[demux_id].ECMpids[n].ECM_PID == ecmpid && demux[demux_id].ECMpids[n].PROVID == provid)
		{
			if (!demux[demux_id].ECMpids[n].streams)
			{
				//we already got this caid/ecmpid as global, no need to add the single stream
				cs_log("Demuxer %d skipped stream CAID: %04X ECM_PID: %04X PROVID: %06X (Same as ECMPID %d)", demux_id, caid, ecmpid, provid, n);
				continue;
			}
			added = 1;
			demux[demux_id].ECMpids[n].streams |= (1 << stream);
			cs_log("Demuxer %d added stream to ecmpid %d CAID: %04X ECM_PID: %04X PROVID: %06X", demux_id, n, caid, ecmpid, provid);
		}
	}

	if (added == 1) return;
	for (n=0;n<demux[demux_id].ECMpidcount;n++) // check for existing pid
	{
		if (demux[demux_id].ECMpids[n].CAID == caid && demux[demux_id].ECMpids[n].ECM_PID == ecmpid && demux[demux_id].ECMpids[n].PROVID == provid)
			{ return; } // found same pid -> skip
	}

	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].table	 = 0;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].tableid = 0;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].ECM_PID = ecmpid;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].CAID    = caid;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].PROVID  = provid;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].CHID 	 = 0x10000; 		// reset CHID
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].checked = 0;
//	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].index 	 = 0;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].status  = 0;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].tries 	 = 0xfe;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].streams = 0; 				// reset streams!
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].irdeto_curindex = 0xfe; 	// reset
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].irdeto_maxindex = 0; 		// reset
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].irdeto_cycle		= 0xfe; 	// reset
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].irdeto_cycling	= 0;
#if defined(MODULE_XCAMD)
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].iks_irdeto_pi	= 0xfe;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].iks_irdeto_chid	= 0x10000;
#endif
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].constcw = (caid_is_biss(caid)) ? 1 : 0;
	demux[demux_id].ECMpids[demux[demux_id].ECMpidcount].crcprevious = 0; /* sky(dvn) */

	cs_log("Demuxer %d ecmpid %d CAID: %04X ECM_PID: %04X PROVID: %06X", demux_id, demux[demux_id].ECMpidcount, caid, ecmpid, provid);
	if (caid_is_irdeto(caid)) { demux[demux_id].emmstart.time = 1; }  // marker to fetch emms early irdeto needs them!

	demux[demux_id].ECMpidcount++;
}

void dvbapi_add_ecmpid(int32_t demux_id, uint16_t caid, uint16_t ecmpid, uint32_t provid)
{
	MYDVB_TRACE("mydvb:dvbapi_add_ecmpid{%04X:%06X, %04X}\n", caid, provid, ecmpid);
	dvbapi_add_ecmpid_int(demux_id, caid, ecmpid, provid);
	struct s_dvbapi_priority *joinentry;

	for (joinentry=dvbApi_priority; joinentry != NULL; joinentry=joinentry->next)
	{
		if ((joinentry->type != 'j')
			|| (joinentry->caid   && joinentry->caid   != caid)
			|| (joinentry->provid && joinentry->provid != provid)
			|| (joinentry->ecmpid && joinentry->ecmpid != ecmpid)
			|| (joinentry->srvid  && joinentry->srvid  != demux[demux_id].program_number))
			{ continue; }
		cs_log_dbg(D_DVBAPI, "Join ecmpid %04X:%06X:%04X to %04X:%06X:%04X",
				caid, provid, ecmpid, joinentry->mapcaid, joinentry->mapprovid, joinentry->mapecmpid);
		dvbapi_add_ecmpid_int(demux_id, joinentry->mapcaid, joinentry->mapecmpid, joinentry->mapprovid);
	}
}

void dvbapi_add_emmpid(struct s_reader *aurdr, int32_t demux_id, uint16_t caid, uint16_t emmpid, uint32_t provid, uint8_t type)
{
	char typetext[40];
	cs_strncpy(typetext, ":", sizeof(typetext));

	if (type & 0x01) { strcat(typetext, "UNIQUE:"); }
	if (type & 0x02) { strcat(typetext, "SHARED:"); }
	if (type & 0x04) { strcat(typetext, "GLOBAL:"); }
	if (type & 0xF8) { strcat(typetext, "UNKNOWN:"); }

	uint16_t i;
	for (i = 0; i < demux[demux_id].EMMpidcount; i++)
	{
		if (demux[demux_id].EMMpids[i].PID == emmpid && demux[demux_id].EMMpids[i].CAID == caid && demux[demux_id].EMMpids[i].PROVID == provid)
		{
			if (!(demux[demux_id].EMMpids[i].type&type)) {
				demux[demux_id].EMMpids[i].type |= type; // register this emm kind to this emmpid
				cs_log_dbg(D_DVBAPI, "Added to existing emmpid %d additional emmtype %s", demux[demux_id].EMMpidcount - 1, typetext);
			}
			return;
		}
	}
	demux[demux_id].EMMpids[demux[demux_id].EMMpidcount].PID 	= emmpid;
	demux[demux_id].EMMpids[demux[demux_id].EMMpidcount].CAID 	= caid;
	demux[demux_id].EMMpids[demux[demux_id].EMMpidcount].PROVID = provid;
	demux[demux_id].EMMpids[demux[demux_id].EMMpidcount++].type = type;
	cs_log_dbg(D_DVBAPI, "Added new emmpid %d CAID: %04X EMM_PID: %04X PROVID: %06X TYPE %s", demux[demux_id].EMMpidcount - 1, caid, emmpid, provid, typetext);
}

void dvbapi_parse_cat(int32_t demux_id, uchar *buffer, int32_t bufsize)
{
#if defined(WITH_HISILICON)
	demux[demux_id].max_emmfilters = MAX_FILTER-1;
#elif defined(WITH_COOLAPI)
	// driver sometimes reports error if too many emm filter
	// but adding more ecm filter is no problem
	// ... so ifdef here instead of limiting MAX_FILTER
	demux[demux_id].max_emmfilters = 14;
#else
	if (cfg.dvbapi_requestmode == 1)
	{
		uint16_t ecm_filter_needed=0,n;
		for (n=0; n<demux[demux_id].ECMpidcount; n++)
		{
			if (demux[demux_id].ECMpids[n].status > -1)
				ecm_filter_needed++;
		}
		if (maxfilter-ecm_filter_needed<=0)
			demux[demux_id].max_emmfilters = 0;
		else
			demux[demux_id].max_emmfilters = maxfilter-ecm_filter_needed;
	}
	else {
		demux[demux_id].max_emmfilters = maxfilter-1;
	}
#endif
	uint16_t i, k;
	uint16_t sctlen = SCT_DATLEN(buffer);
	if (bufsize != sctlen + 3) { // invalid CAT length
		cs_log_dbg(D_DVBAPI, "[DVBAPI] Received an CAT with invalid length!");
		return;
	}

 	MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ CAT\n");
	cs_log_dump_dbg(D_DVBAPI, buffer, bufsize, "cat:");
	struct s_client *cl = cur_client();
	if (!cl) return;
	if (!cl->aureader_list)
	{
		mycs_trace(D_ADB,"mydvb:dvbapi_parse_cat(%c.aureader_list none)", cl->typ);
		return;
	}

	struct s_reader *aurdr = NULL;
	LL_ITER itr = ll_iter_create(cl->aureader_list);
	while ((aurdr = ll_iter_next(&itr))) // make a list of all readers
	{
		if (!aurdr->client
			|| ( aurdr->audisabled)
			|| (!aurdr->enable)
			|| (!is_network_reader(aurdr) && aurdr->card_status != CARD_INSERTED))
		{
		//	MYDVB_TRACE("mydvb:Reader %12s au disabled or not enabled-> skip!\n", aurdr->label); // only parse au enabled readers that are enabled
			cs_log_dbg(D_DVBAPI,"Reader %12s au disabled or not enabled-> skip!", aurdr->label); // only parse au enabled readers that are enabled
			continue;
		}
		// sky(powervu)
		if (IS_XCAMD_READERS(aurdr)) continue;
		if (IS_MORECAM_READERS(aurdr)) continue;
		if (IS_CONSTCW_READERS(aurdr)) continue;

		MYDVB_TRACE("mydvb:Reader %s au enabled.\n", aurdr->label);
		cs_log_dbg(D_DVBAPI,"Reader %s au enabled -> parsing cat for emm pids!", aurdr->label);

		for (i = 8; i < sctlen - 1; i += buffer[i+1] + 2)
		{
			if (buffer[i] != 0x09) continue;
			if (demux[demux_id].EMMpidcount >= EMM_PIDS) break;

			uint16_t caid = b2i(2, buffer + i + 2);
			uint16_t emm_pid = b2i(2, buffer + i +4)&0x1FFF;
			uint32_t emm_provider = 0;

//			MYDVB_TRACE("mydvb:emm-casysid: %04X,%04X\n", caid, emm_pid);
			switch (caid >> 8)
			{
				case 0x01:
// sky(powervu)
#if defined(MODULE_XCAS)
					if (IS_XCAS_READERS(aurdr)) break;
#endif
					dvbapi_add_emmpid(aurdr, demux_id, caid, emm_pid, 0, EMM_UNIQUE|EMM_GLOBAL);
					for (k = i+7; k < i+buffer[i+1]+2; k += 4)
					{
					   emm_provider = b2i(2, buffer + k + 2);
					   emm_pid = b2i(2, buffer + k)&0xFFF;
						dvbapi_add_emmpid(aurdr, demux_id, caid, emm_pid, emm_provider, EMM_SHARED);
					}
					break;
				case 0x05:
// sky(powervu)
#if defined(MODULE_XCAS)
					if (IS_XCAS_READERS(aurdr)) break;
#endif
					for (k = i+6; k < i+buffer[i+1]+2; k += buffer[k+1]+2)
					{
						if (buffer[k] == 0x14)
						{
							emm_provider = (b2i(3, buffer + k + 2) & 0xFFFFF0); // viaccess fixup last digit is a dont care!
							dvbapi_add_emmpid(aurdr, demux_id, caid, emm_pid, emm_provider, EMM_UNIQUE | EMM_SHARED | EMM_GLOBAL);
						}
					}
					break;
				case 0x18:
// sky(powervu)
#if defined(MODULE_XCAS)
					if (IS_XCAS_READERS(aurdr)) break;
#endif
					if (buffer[i + 1] == 0x07 || buffer[i + 1] == 0x0B)
					{
						for(k = i + 7; k < i + 7 + buffer[i + 6]; k += 2)
						{
							emm_provider = b2i(2, buffer + k);
							dvbapi_add_emmpid(aurdr, demux_id, caid, emm_pid, emm_provider, EMM_UNIQUE | EMM_SHARED | EMM_GLOBAL);
						}
					}
					else
					{
						dvbapi_add_emmpid(aurdr, demux_id, caid, emm_pid, emm_provider, EMM_UNIQUE|EMM_SHARED|EMM_GLOBAL);
					}
					break;
				default:
				// sky(DRE3)
				//	if (aurdr->DRE_3s)
					{
						if (((caid & 0xfffe)==0x4AE0) || ((caid & 0xfffe)==0x7BE0) || (caid ==0x2710)) {
							if (buffer[i+1] > 4) emm_provider = buffer[i+6];
						}
					}
// sky(powervu)
#if defined(MODULE_XCAS)
					if (IS_XCAS_READERS(aurdr)) {
						if (!xcas_IsAuAvailable(aurdr, caid, emm_provider)) break;
					}
#endif
					dvbapi_add_emmpid(aurdr, demux_id, caid, emm_pid, emm_provider, EMM_UNIQUE|EMM_SHARED|EMM_GLOBAL);
					break;
			}
		}
	}
	return;
}

static pthread_mutex_t lockindex;
int32_t dvbapi_get_descindex(int32_t demux_index)
{
	int32_t i,j,idx=1,fail=1;
	if (cfg.dvbapi_boxtype == BOXTYPE_NEUMO)
	{
		idx=0;
		sscanf(demux[demux_index].pmt_file, "pmt%3d.tmp", &idx);
		idx++; // fixup
		return idx;
	}
	pthread_mutex_lock(&lockindex); // to avoid race when readers become responsive!
	while (fail)
	{
		fail=0;
		for(i = 0; i < MAX_DEMUX && !fail; i++)
		{
			for(j = 0; j < demux[i].ECMpidcount && !fail; j++)
			{
				if (demux[i].ECMpids[j].index == idx)
				{
					fail = 1;
					idx++;
				}
			}
		}
		cs_sleepms(1);
	}
	pthread_mutex_unlock(&lockindex); // and release it!
	return idx;
}

void dvbapi_set_pid(int32_t demux_id, int32_t num, int32_t idx, bool enable)
{
	int32_t i, currentfd;

//	MYDVB_TRACE("mydvb:dvbapi_set_pid{%d, %d.%04X}\n", demux[demux_id].pidindex, num, demux[demux_id].STREAMpids[num]);
	if (demux[demux_id].pidindex == -1 && enable) return; // no current pid on enable? --> exit

	switch (selected_api)
	{
#ifdef WITH_STAPI
		case STAPI:
			if (!enable) idx = -1;
			stapi_set_pid(demux_id, num, idx, demux[demux_id].STREAMpids[num], demux[demux_id].pmt_file); // only used to disable pids!!!
			break;
#endif
#ifdef WITH_COOLAPI
		case COOLAPI:
			break;
#endif

#if defined(WITH_HISILICON)
// unnecessariness(tvheadend)....
		case HISILICONAPI:
			break;
#endif

		default:
			for (i=0;i<MAX_DEMUX;i++)
			{
				if (((demux[demux_id].ca_mask & (1 << i)) == (uint) (1 << i)))
				{
				   int8_t action = 0;
				   if (enable) {
					   action = update_streampid_list(i, demux[demux_id].STREAMpids[num], idx);
				   }
					if (!enable){
					   action = remove_streampid_from_list(i, demux[demux_id].STREAMpids[num], idx);
				   }

					if (action != NO_STREAMPID_LISTED && action != FOUND_STREAMPID_INDEX && action != ADDED_STREAMPID_INDEX)
					{
						ca_pid_t ca_pid2;
						memset(&ca_pid2,0,sizeof(ca_pid2));
						ca_pid2.pid   = demux[demux_id].STREAMpids[num];

					   // removed last of this streampid on ca? -> disable this pid with -1 on this ca
					   if ((action == REMOVED_STREAMPID_LASTINDEX) && (is_ca_used(i, ca_pid2.pid) == CA_IS_CLEAR)) idx = -1;

					   // removed index of streampid that is used to decode on ca -> get a fresh one
					   if (action == REMOVED_DECODING_STREAMPID_INDEX)
					   {
						   idx = is_ca_used(i, demux[demux_id].STREAMpids[num]); // get an active index for this pid and enable it on ca device
						   enable = 1;
					   }

						ca_pid2.index = idx;
						cs_log_dbg(D_DVBAPI, "Demuxer %d %s stream %d pid=0x%04x index=%d on ca%d", demux_id,
						   	(enable ? "enable" : "disable"), num + 1, ca_pid2.pid, ca_pid2.index, i);

						if (cfg.dvbapi_boxtype == BOXTYPE_PC || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
							dvbapi_net_send(DVBAPI_CA_SET_PID, demux[demux_id].socket_fd, demux_id, -1 /*unused*/, (unsigned char *) &ca_pid2, NULL, NULL);
						else
						{
						   currentfd = ca_fd[i];
						   if (currentfd <= 0)
						   {
							   currentfd = dvbapi_open_device(1, i, demux[demux_id].adapter_index);
							   ca_fd[i] = currentfd; // save fd of this ca
						   }
						   if (currentfd > 0)
						   {
							   if (dvbapi_ioctl(currentfd, CA_SET_PID, &ca_pid2) == -1)
								   cs_log_dbg(D_TRACE | D_DVBAPI,"CA_SET_PID ioctl error (errno=%d %s)", errno, strerror(errno));
								int8_t result = is_ca_used(i,0); // check if in use by any pid
							   if (!enable && result == CA_IS_CLEAR) {
									cs_log_dbg(D_DVBAPI, "Demuxer %d close now unused CA%d device", demux_id, i);
							      int32_t ret = close(currentfd);
							      if (ret < 0) { cs_log("ERROR: Could not close demuxer fd (errno=%d %s)", errno, strerror(errno)); }
							      currentfd = ca_fd[i] = 0;
						      }
						   }
					   }
				   }
			   }
		   }
		   break;
	}
	return;
}

void dvbapi_stop_all_descrambling(void)
{
	int32_t j;
	for(j = 0; j < MAX_DEMUX; j++)
	{
		if (demux[j].program_number == 0) { continue; }
		dvbapi_stop_descrambling(j);
	}
}

void dvbapi_stop_descrambling(int32_t demux_id)
{
	int32_t i;

 	MYDVB_TRACE("mydvb:SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS STOP.\n");
	MYDVB_TRACE("mydvb:dvbapi_stop_descrambling{%d,%d}\n", demux_id, demux[demux_id].program_number);
	if (demux[demux_id].program_number==0) return;

	char channame[32];
	i = demux[demux_id].pidindex;
	if (i< 0) i = 0;
	int32_t idx = demux[demux_id].ECMpids[i].index;
	get_servicename(dvbApi_client, demux[demux_id].program_number, demux[demux_id].ECMpidcount>0 ? demux[demux_id].ECMpids[i].CAID : 0, channame);
	cs_log_dbg(D_DVBAPI, "Demuxer %d stop descrambling program number %04X (%s)", demux_id, demux[demux_id].program_number, channame);
	dvbapi_stop_filter(demux_id, TYPE_EMM);
	if (demux[demux_id].ECMpidcount>0)
	{
	   dvbapi_stop_filter(demux_id, TYPE_ECM);
		demux[demux_id].cs_cwidx = -1;
		demux[demux_id].pidindex = -1;
		demux[demux_id].curindex = -1;
		for (i=0; i<demux[demux_id].STREAMpidcount; i++) {
			dvbapi_set_pid(demux_id, i, idx - 1, false); // disable all streampids for this index!
		}
	}
// sky(powervu)
#if defined(MODULE_XCAS) || defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	DVBICS_ChCloser(demux_id);
#endif

	memset(&demux[demux_id], 0, sizeof(DEMUXTYPE));
	demux[demux_id].pidindex = -1;
	demux[demux_id].curindex = -1;
	demux[demux_id].cs_cwidx = -1;
	// sky(his)
	if (!cfg.dvbapi_listenport && cfg.dvbapi_boxtype != BOXTYPE_PC_NODMX && cfg.dvbapi_boxtype != BOXTYPE_HISILICON) {
		unlink(ECMINFO_FILE);
	}
	return;
}

// sky(n)
#if defined(MODULE_XCAS)
int32_t dvbapi_constcw_afterwards(int32_t typ, int32_t demux_id, int16_t direction)
{
	ECM_REQUEST *er;
	int16_t idx = demux[demux_id].curindex;

	mycs_debug(D_DVBAPI, "dvbapi_constcw_afterwards(%x){%d, %d}\n", typ, idx, direction);
	if (!demux[demux_id].ECMpids[idx].constcw) return 0;

	if (!(er = get_ecmtask())) return 0;
	er->dmuxid = demux_id;// sky(powervu)
	er->srvid  = demux[demux_id].program_number;
	er->caid   = demux[demux_id].ECMpids[idx].CAID;
	er->pid    = demux[demux_id].ECMpids[idx].ECM_PID;
	er->prid   = demux[demux_id].ECMpids[idx].PROVID;
	er->vpid   = demux[demux_id].ECMpids[idx].VPID;
	er->ecmlen = 13;
	er->ecm[0] = 0x80;
	er->ecm[1] = 0;
	er->ecm[2] = 10;
	i2b_buf(2, er->srvid, er->ecm+3);
	i2b_buf(2, er->vpid,  er->ecm+5);
	i2b_buf(2, demux[demux_id].cs_degree, er->ecm+7);
	i2b_buf(2, demux[demux_id].cs_frequency, er->ecm+9);
	i2b_buf(2, demux[demux_id].cs_subsequence, er->ecm+11);
	if (direction) er->constAfterwards++;
	else er->constAfterwards = 0;
	er->chSets.muxid	 	= demux_id;
	er->chSets.srvid  	= demux[demux_id].program_number;
	er->chSets.degree		= demux[demux_id].cs_degree;
	er->chSets.frequency = demux[demux_id].cs_frequency;
	er->chSets.vpid	 	= demux[demux_id].cs_vidpid;

	demux[demux_id].cs_subsequence++;
	dvbapi_request_cw(dvbApi_client, er, demux_id, demux[demux_id].cs_filtnum, 0);
	return 1;
}
#endif

int32_t dvbapi_start_descrambling(int32_t demux_id, int32_t pidx, int8_t checked)
{
	struct s_reader *rdr;
	ECM_REQUEST *er;
	int32_t started = 0; // in case ecmfilter started = 1
//	int32_t ecmfake = 0;

	if (!(er = get_ecmtask())) return started;
	er->dmuxid = demux_id;// sky(powervu)
	demux[demux_id].ECMpids[pidx].checked = checked + 1; // mark this pidx as checked!
#if defined(MODULE_XCAMD)
	demux[demux_id].ECMpids[pidx].iks_irdeto_pi = 0xfe;
	demux[demux_id].ECMpids[pidx].iks_irdeto_chid = 0x10000;
#endif

	MYDVB_TRACE("mydvb:dvbapi_start_descrambling{%d, %d}\n", pidx, demux[demux_id].curindex);
	struct s_dvbapi_priority *p;
	for (p = dvbApi_priority; p != NULL ; p = p->next)
	{
		if ((p->type != 'p')
			|| (p->caid   && p->caid   != demux[demux_id].ECMpids[pidx].CAID)
			|| (p->provid && p->provid != demux[demux_id].ECMpids[pidx].PROVID)
			|| (p->ecmpid && p->ecmpid != demux[demux_id].ECMpids[pidx].ECM_PID)
			|| (p->srvid  && p->srvid  != demux[demux_id].program_number)
			|| (p->pidx   && p->pidx-1 != pidx))
			{ continue; }
		// if found chid and first run apply chid filter, on forced pids always apply!
		if (p->type == 'p' && p->chid <0x10000 && (demux[demux_id].ECMpids[pidx].checked == 1 || (p && p->force)))
		{
			if (demux[demux_id].ECMpids[pidx].CHID < 0x10000) // channelcache delivered chid
			{
				er->chid = demux[demux_id].ECMpids[pidx].CHID;
			}
			else
			{
				er->chid = p->chid; // no channelcache or no chid in use, so use prio chid
				demux[demux_id].ECMpids[pidx].CHID = p->chid;
			}
		//	cs_log("********* CHID %04X **************", demux[demux_id].ECMpids[pidx].CHID);
			break; // we only accept one!
		}
		else
		{
			if (demux[demux_id].ECMpids[pidx].CHID < 0x10000) // channelcache delivered chid
			{
				er->chid = demux[demux_id].ECMpids[pidx].CHID;
			}
			else // no channelcache or no chid in use
			{
				er->chid = 0;
				demux[demux_id].ECMpids[pidx].CHID = 0x10000;
			}
		}
	}
	er->onid   = demux[demux_id].onid;
	er->srvid  = demux[demux_id].program_number;
	er->caid   = demux[demux_id].ECMpids[pidx].CAID;
	er->pid    = demux[demux_id].ECMpids[pidx].ECM_PID;
	er->prid   = demux[demux_id].ECMpids[pidx].PROVID;
	er->vpid   = demux[demux_id].ECMpids[pidx].VPID;
	er->pmtpid = demux[demux_id].pmtpid;
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM) || defined(MODULE_XCAS)
	er->chSets.muxid	 	= demux_id;
	er->chSets.srvid  	= demux[demux_id].program_number;
	er->chSets.degree		= demux[demux_id].cs_degree;
	er->chSets.frequency = demux[demux_id].cs_frequency;
	er->chSets.vpid	 	= demux[demux_id].cs_vidpid;
#endif
#if defined(MODULE_AVAMGCAMD)
	er->exprid = er->prid;
	if (IS_IRDETO(er->caid)) {
		er->exprid = (((er->onid>>8)&0xff) ^ ((er->onid)&0xff));
	}
#endif
#if defined(__DVCCRC_AVAILABLE__)
	DVBCRC_Clean(&(demux[demux_id].ECMpids[pidx]));
#endif

	struct timeb now;
	cs_ftime(&now);
	for (rdr=first_active_reader; rdr != NULL; rdr=rdr->next)
	{
		int8_t match = matching_reader(er, rdr, 1); // check for matching reader
		int64_t gone = comp_timeb(&now, &rdr->emm_last);
		if (gone > 3600*1000 && rdr->needsemmfirst && caid_is_irdeto(er->caid))
		{
			cs_log("Warning reader %s received no emms for the last %d seconds -> skip, this reader needs emms first!", rdr->label,
				   (int)(gone/1000));
			continue; // skip this card needs to process emms first before it can be used for descramble
		}
		if (p && p->force) { match = 1; }  // forced pid always started!

		if (!match) // if this reader does not match, check betatunnel for it
			match = lb_check_auto_betatunnel(er, rdr);

		if (!match && chk_is_betatunnel_caid(er->caid)) // these caids might be tunneled invisible by peers
			{ match = 1; } // so make it a match to try it!

		if (config_enabled(CS_CACHEEX) && (!match && (cacheex_is_match_alias(dvbApi_client, er))))   // check if cache-ex is matching
		{
			match = 1; // so make it a match to try it!
		}

		// BISS or FAKE CAID
		// ecm stream pid is fake, so send out one fake ecm request
		// special treatment: if we asked the cw first without starting a filter the cw request will be killed due to no ecmfilter started
		if (caid_is_fake(demux[demux_id].ECMpids[pidx].CAID) || caid_is_biss(demux[demux_id].ECMpids[pidx].CAID))
		{
			MYDVB_TRACE("mydvb:biss channel\n");
			er->ecmlen = 13;
			er->ecm[0] = 0x80;
			er->ecm[1] = 0;
			er->ecm[2] = 10;
			i2b_buf(2, er->srvid, er->ecm+3);
			i2b_buf(2, er->vpid,  er->ecm+5);
			i2b_buf(2, demux[demux_id].cs_degree, er->ecm+7);
			i2b_buf(2, demux[demux_id].cs_frequency, er->ecm+9);
			i2b_buf(2, demux[demux_id].cs_subsequence, er->ecm+11);
			demux[demux_id].cs_subsequence++;
#if defined(MODULE_XCAS)
			er->constAfterwards = 0;
#endif

			cs_log("Demuxer %d trying to descramble PID %d CAID %04X PROVID %06X ECMPID %04X ANY CHID PMTPID %04X VPID %04X", demux_id, pidx,
				   demux[demux_id].ECMpids[pidx].CAID, demux[demux_id].ECMpids[pidx].PROVID, demux[demux_id].ECMpids[pidx].ECM_PID,
				   demux[demux_id].pmtpid, demux[demux_id].ECMpids[pidx].VPID);

			demux[demux_id].curindex = pidx; // set current pidx to the fresh started one

			int32_t filnum;
			filnum = dvbapi_start_filter(demux_id,
								pidx,
								demux[demux_id].ECMpids[pidx].ECM_PID,
								demux[demux_id].ECMpids[pidx].CAID,
								demux[demux_id].ECMpids[pidx].PROVID,
								0x80,
								0xF0,
								3000,
								TYPE_ECM);
			started = 1;

			dvbapi_request_cw(dvbApi_client, er, demux_id, filnum, 0); // do not register ecm since this try!
//			ecmfake = 1;
			break; // we started an ecmfilter so stop looking for next matching reader!
		}

		if (match) // if matching reader found check for irdeto cas if local irdeto card check if it received emms in last 60 minutes
		{

			if (caid_is_irdeto(er->caid))   // irdeto cas init irdeto_curindex to wait for first index (00)
			{
				if (demux[demux_id].ECMpids[pidx].irdeto_curindex==0xfe) demux[demux_id].ECMpids[pidx].irdeto_curindex = 0x00;
			}

			if (p && p->chid<0x10000) { // do we prio a certain chid?
				cs_log("Demuxer %d trying to descramble PID %d CAID %04X PROVID %06X ECMPID %04X CHID %04X PMTPID %04X VPID %04X", demux_id, pidx,
						demux[demux_id].ECMpids[pidx].CAID, demux[demux_id].ECMpids[pidx].PROVID, demux[demux_id].ECMpids[pidx].ECM_PID,
						demux[demux_id].ECMpids[pidx].CHID, demux[demux_id].pmtpid, demux[demux_id].ECMpids[pidx].VPID);
			}
			else {
				cs_log("Demuxer %d trying to descramble PID %d CAID %04X PROVID %06X ECMPID %04X ANY CHID PMTPID %04X VPID %04X", demux_id, pidx,
						demux[demux_id].ECMpids[pidx].CAID, demux[demux_id].ECMpids[pidx].PROVID, demux[demux_id].ECMpids[pidx].ECM_PID,
					   demux[demux_id].pmtpid, demux[demux_id].ECMpids[pidx].VPID);
			}

			demux[demux_id].curindex = pidx; // set current pidx to the fresh started one

			dvbapi_start_filter(demux_id,
							pidx,
							demux[demux_id].ECMpids[pidx].ECM_PID,
							demux[demux_id].ECMpids[pidx].CAID,
							demux[demux_id].ECMpids[pidx].PROVID,
							0x00,
							0x00,
							3000,
							TYPE_ECM);
			started = 1;
			break; // we started an ecmfilter so stop looking for next matching reader!
		}
	}
	if (demux[demux_id].curindex != pidx)
	{
		cs_log("Demuxer %d impossible to descramble PID %d CAID %04X PROVID %06X ECMPID %04X PMTPID %04X (NO MATCHING READER)", demux_id, pidx,
				demux[demux_id].ECMpids[pidx].CAID, demux[demux_id].ECMpids[pidx].PROVID, demux[demux_id].ECMpids[pidx].ECM_PID, demux[demux_id].pmtpid);
		demux[demux_id].ECMpids[pidx].checked =  4; // flag this pid as checked
		demux[demux_id].ECMpids[pidx].status  = -1; // flag this pid as unusable
		dvbapi_edit_channel_cache(demux_id, pidx, 0); 	  // remove this pid from channelcache
	}
// sky(?)
//	if (!ecmfake) NULLFREE(er);
	return started;
}

struct s_dvbapi_priority *dvbapi_check_prio_match_emmpid(int32_t demux_id, uint16_t caid, uint32_t provid, char type)
{
	struct s_dvbapi_priority *p;
	int32_t i;

	uint16_t ecm_pid=0;
//	MYDVB_TRACE("mydvb:dvbapi_check_prio_match_emmpid\n");
	for (i=0; i<demux[demux_id].ECMpidcount; i++)
	{
		if ((demux[demux_id].ECMpids[i].CAID==caid) && (demux[demux_id].ECMpids[i].PROVID==provid))
		{
			ecm_pid = demux[demux_id].ECMpids[i].ECM_PID;
			break;
		}
	}

	if (!ecm_pid)
		return NULL;

	for(p = dvbApi_priority; p != NULL; p = p->next)
	{
		if (p->type != type
			|| (p->caid   && p->caid   != caid)
			|| (p->provid && p->provid != provid)
			|| (p->ecmpid && p->ecmpid != ecm_pid)
			|| (p->srvid  && p->srvid  != demux[demux_id].program_number)
			|| (p->pidx   && p->pidx-1 != i)
			|| (p->type == 'i' && (p->chid < 0x10000)))
			continue;
		return p;
	}
	return NULL;
}

struct s_dvbapi_priority *dvbapi_check_prio_match(int32_t demux_id, int32_t pidx, char type)
{
	struct s_dvbapi_priority *p;
	struct s_ecmpids *ecmpid = &demux[demux_id].ECMpids[pidx];

	for(p = dvbApi_priority; p != NULL; p = p->next)
	{
		if (p->type != type
			|| (p->caid   && p->caid   != ecmpid->CAID)
			|| (p->provid && p->provid != ecmpid->PROVID)
			|| (p->ecmpid && p->ecmpid != ecmpid->ECM_PID)
			|| (p->srvid  && p->srvid  != demux[demux_id].program_number)
		//	|| (p->type == 'i' && (p->chid > -1)))  // ????
			|| (p->pidx   && p->pidx-1 != pidx)
			|| (p->chid<0x10000 && p->chid != ecmpid->CHID))
		{
			continue;
		}
		MYDVB_TRACE("mydvb:check_prio_matching\n");
		return p;
	}
	return NULL;
}


void dvbapi_process_emm(int32_t demux_index, int32_t filter_num, unsigned char *buffer, uint32_t bufsize)
{
	EMM_PACKET emmpkt_s;
	int32_t emmsento = 0;

	cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d fetched emm data", demux_index, filter_num + 1); // emm shown with -d64

	struct s_emm_filter *filter = get_emmfilter_by_filternum(demux_index, filter_num+1);

	if (!filter) {
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d no filter matches -> SKIP!", demux_index, filter_num+1);
		return;
	}

	uint32_t provider = filter->provid;
	uint16_t caid = filter->caid;

	struct s_dvbapi_priority *mapentry = dvbapi_check_prio_match_emmpid(filter->demux_id, filter->caid, filter->provid, 'm');
	if (mapentry)
	{
		cs_log_dbg(D_DVBAPI, "Demuxer %d mapping EMM from %04X:%06X to %04X:%06X", demux_index, caid, provider, mapentry->mapcaid,
					  mapentry->mapprovid);
		caid = mapentry->mapcaid;
		provider = mapentry->mapprovid;
	}
	#if (__ADB_TRACE__)
		if (IS_CRYPTOWORKS(caid)) {
			if (buffer[0] == 0x86) {
				MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ EMM{%02X}{%04X:%06X}{%02X.%d}\n", buffer[0], caid, provider, buffer[8], bufsize);
			}
			else {
				MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ EMM{%02X}{%04X:%06X}{%2d.%d}\n", buffer[0], caid, provider, filter_num, bufsize);
				myprdump("@@@ EMM", buffer, bufsize);
			}
		}
		else {
			MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ EMM{%02X}{%04X:%06X}{%d.%d}\n", buffer[0], caid, provider, filter_num, bufsize);
			myprdump("@@@ EMM", buffer, bufsize);
		}
	#endif

	memset(&emmpkt_s, 0, sizeof(emmpkt_s));

	i2b_buf(2, caid, emmpkt_s.caid);
	i2b_buf(4, provider, emmpkt_s.provid);

//	emmpkt_s.emmlen = bufsize;
	emmpkt_s.emmlen = bufsize > sizeof(emmpkt_s.emm) ? sizeof(emmpkt_s.emm) : bufsize;
	memcpy(emmpkt_s.emm, buffer, emmpkt_s.emmlen);

	if (config_enabled(READER_IRDETO) && chk_is_betatunnel_caid(caid) == 2)
	{
		uint16_t ncaid = tunemm_caid_map(FROM_TO, caid, demux[demux_index].program_number);
		if (caid != ncaid) {
			irdeto_add_emm_header(&emmpkt_s);
			i2b_buf(2, ncaid, emmpkt_s.caid);
		}
	}

	// sky(a!)
	emmsento = do_emm(dvbApi_client, &emmpkt_s);
	if (!emmsento)	{
		if (dvbApi_client->emmsento) {
			mycs_trace(D_ADB, "mydvb:emm.stop");
		//	dvbapi_stop_filter(demux_index, TYPE_EMM); // stop all emm filters
		}
	}
	dvbApi_client->emmsento = emmsento;
}

void dvbapi_read_priority(void)
{
	FILE *fp;
	char token[128], str1[128];
	char type;
	int32_t i, ret, count=0;

	const char *cs_prio="oscam.dvbapi";

	fp = fopen(get_config_filename(token, sizeof(token), cs_prio), "r");
	if (!fp) {
		cs_log_dbg(D_DVBAPI, "ERROR: Can't open priority file %s", token);
		return;
	}

	if (dvbApi_priority)
	{
		cs_log_dbg(D_DVBAPI, "reread priority file %s", cs_prio);
		struct s_dvbapi_priority *o, *p;
		for (p = dvbApi_priority; p != NULL; p = o) {
			o = p->next;
			NULLFREE(p);
		}
		dvbApi_priority = NULL;
	}

	while (fgets(token, sizeof(token), fp))
	{
		// Ignore comments and empty lines
		if (token[0]=='#' || token[0]=='/' || token[0]=='\n' || token[0]=='\r' || token[0]=='\0')
			continue;
		if (strlen(token)>100) continue;

		memset(str1, 0, 128);

		for (i=0; i<(int)strlen(token) && token[i]==' '; i++);
		if (i  == (int)strlen(token) - 1) //empty line or all spaces
			continue;

		for (i=0;i<(int)strlen(token);i++)
		{
			if ((token[i]==':' || token[i]==' ') && token[i+1]==':') { 	// if "::" or " :"
				memmove(token+i+2, token+i+1, strlen(token)-i+1); //insert extra position
				token[i+1]='0';		//and fill it with NULL
			}
			if (token[i]=='#' || token[i]=='/') {
				token[i]='\0';
				break;
			}
		}

		type = 0;
#ifdef WITH_STAPI
		uint32_t disablefilter = 0;
		ret  = sscanf(trim(token), "%c: %63s %63s %d", &type, str1, str1+64, &disablefilter);
#else
		ret  = sscanf(trim(token), "%c: %63s %63s", &type, str1, str1+64);
#endif
		type = tolower((uchar)type);

		if (ret < 1 ||(type != 'p' && type != 'i' && type != 'm' && type != 'd' && type != 's' && type != 'l'
					   && type != 'j' && type != 'a' && type != 'x'))
		{
			//fprintf(stderr, "Warning: line containing %s in %s not recognized, ignoring line\n", token, cs_prio);
			//fprintf would issue the warning to the command line, which is more consistent with other config warnings
			//however it takes OSCam a long time (>4 seconds) to reach this part of the program, so the warnings are reaching tty rather late
			//which leads to confusion. So send the warnings to log file instead
			cs_log_dbg(D_DVBAPI, "WARN: line containing %s in %s not recognized, ignoring line\n", token, cs_prio);
			continue;
		}

		struct s_dvbapi_priority *entry;
		if (!cs_malloc(&entry, sizeof(struct s_dvbapi_priority)))
		{
			int32_t retstatus = fclose(fp);
			if (retstatus < 0) cs_log("ERROR: Could not close oscam.dvbapi fd (errno=%d %s)", errno, strerror(errno));
			return;
		}

		entry->type=type;
		entry->next=NULL;

		count++;

#ifdef WITH_STAPI
		if (type=='s')
		{
			strncpy(entry->devname, str1, 29);
			strncpy(entry->pmtfile, str1+64, 29);

			entry->disablefilter=disablefilter;

			cs_log_dbg(D_DVBAPI, "stapi prio: ret=%d | %c: %s %s | disable %d",
				ret, type, entry->devname, entry->pmtfile, disablefilter);

			if (!dvbApi_priority) {
				dvbApi_priority=entry;
			}
			else {
 				struct s_dvbapi_priority *p;
				for (p = dvbApi_priority; p->next != NULL; p = p->next);
				p->next = entry;
			}
			continue;
		}
#endif

		char c_srvid[34];
		c_srvid[0]='\0';
		uint32_t caid=0, provid=0, srvid=0, ecmpid=0;
		uint32_t chid=0x10000; //chid=0 is a valid chid
		ret = sscanf(str1, "%4x:%6x:%33[^:]:%4x:%4x"SCNx16, &caid, &provid, c_srvid, &ecmpid, &chid);
		if (ret < 1)
		{
			cs_log("Error in oscam.dvbapi: ret=%d | %c: %04X %06X %s %04X %04X",
					ret, type, caid, provid, c_srvid, ecmpid, chid);
			continue; // skip this entry!
		}
		else
		{
			cs_log_dbg(D_DVBAPI, "Parsing rule: ret=%d | %c: %04X %06X %s %04X %04X",
					ret, type, caid, provid, c_srvid, ecmpid, chid);
		}

		entry->caid	  = caid;
		entry->provid = provid;
		entry->ecmpid = ecmpid;
		entry->chid	  = chid;

		uint32_t delay = 0, force = 0, mapcaid = 0, mapprovid = 0, mapecmpid = 0, pidx = 0;
		switch(type)
		{
			case 'i':
				ret = sscanf(str1 + 64, "%1d", &pidx);
				entry->pidx = pidx+1;
				if (ret < 1) entry->pidx = 0;
				break;
			case 'd':
				sscanf(str1+64, "%4d", &delay);
				entry->delay=delay;
				break;
			case 'l':
				entry->delay = dyn_word_atob(str1+64);
				if (entry->delay == -1) { entry->delay = 0; }
				break;
			case 'p':
				ret = sscanf(str1 + 64, "%1d:%1d", &force, &pidx);
				entry->force= force;
				entry->pidx = pidx+1;
				if (ret < 2) entry->pidx = 0;
				break;
			case 'm':
				sscanf(str1+64, "%4x:%6x", &mapcaid, &mapprovid);
				if (!mapcaid) { mapcaid = 0xFFFF; }
				entry->mapcaid=mapcaid;
				entry->mapprovid=mapprovid;
				break;
			case 'a':
			case 'j':
				sscanf(str1+64, "%4x:%6x:%4x", &mapcaid, &mapprovid, &mapecmpid);
				if (!mapcaid) { mapcaid = 0xFFFF; }
				entry->mapcaid=mapcaid;
				entry->mapprovid=mapprovid;
				entry->mapecmpid=mapecmpid;
				break;
		}

		if (c_srvid[0]=='=')
		{
			struct s_srvid *this;

			for (i=0;i<16;i++)
			for (this = cfg.srvid[i]; this != NULL; this = this->next)
			{
				if (strcmp(this->prov, c_srvid+1)==0)
				{
					struct s_dvbapi_priority *entry2;
					if (!cs_malloc(&entry2,sizeof(struct s_dvbapi_priority))) continue;
					memcpy(entry2, entry, sizeof(struct s_dvbapi_priority));

					entry2->srvid=this->srvid;

					cs_log_dbg(D_DVBAPI, "prio srvid: ret=%d | %c: %04X %06X %04X %04X %04X -> map %04X %06X %04X | prio %d | delay %d",
							ret, entry2->type, entry2->caid, entry2->provid, entry2->srvid, entry2->ecmpid, entry2->chid,
							entry2->mapcaid, entry2->mapprovid, entry2->mapecmpid, entry2->force, entry2->delay);

					if (!dvbApi_priority) {
						dvbApi_priority = entry2;
					}
					else {
 						struct s_dvbapi_priority *p;
						for (p = dvbApi_priority; p->next != NULL; p = p->next);
						p->next = entry2;
					}
				}
			}
			NULLFREE(entry);
			continue;
		}
		else {
			sscanf(c_srvid, "%4x", &srvid);
			entry->srvid=srvid;
		}

		cs_log_dbg(D_DVBAPI, "prio: ret=%d | %c: %04X %06X %04X %04X %04X -> map %04X %06X %04X | prio %d | delay %d",
				ret, entry->type, entry->caid, entry->provid, entry->srvid, entry->ecmpid, entry->chid, entry->mapcaid,
				entry->mapprovid, entry->mapecmpid, entry->force, entry->delay);

		if (!dvbApi_priority) {
			dvbApi_priority = entry;
		}
		else {
 			struct s_dvbapi_priority *p;
			for (p = dvbApi_priority; p->next != NULL; p = p->next);
			p->next = entry;
		}
	}

	cs_log_dbg(D_DVBAPI, "Read %d entries from %s", count, cs_prio);

	int32_t retstatus = fclose(fp);
	if (retstatus < 0) cs_log("ERROR: Could not close oscam.dvbapi fd (errno=%d %s)", errno, strerror(errno));
	return;
}

void dvbapi_resort_ecmpids(int32_t demux_index)
{
	int32_t n, cache=0, prio=1, highest_prio=0, matching_done=0, found = -1;
	uint16_t btun_caid=0;
	struct timeb start,end;
	cs_ftime(&start);

	MYDVB_TRACE("mydvb:dvbapi_resort_ecmpids{%d}.\n", demux[demux_index].ECMpidcount);
	for (n=0; n<demux[demux_index].ECMpidcount; n++)
	{
		demux[demux_index].ECMpids[n].status  = 0;
		demux[demux_index].ECMpids[n].checked = 0;
	}

	demux[demux_index].max_status= 0;
	demux[demux_index].curindex  = -1;
	demux[demux_index].pidindex  = -1;
	demux[demux_index].cs_cwidx  = -1;

	struct s_channel_cache *c = NULL;

	for(n = 0; n < demux[demux_index].ECMpidcount; n++)
	{
		c = dvbapi_find_channel_cache(demux_index, n, 0); // find exact channel match
		if (c != NULL)
		{
			found = n;
			cache = 2; //found cache entry with higher priority
			demux[demux_index].ECMpids[n].status = prio * 2; // prioritize CAIDs which already decoded same caid:provid:srvid
			if (c->chid < 0x10000) { demux[demux_index].ECMpids[n].CHID = c->chid; } // if chid registered in cache -> use it!
			cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X (found caid/provid/srvid in cache - weight: %d)", demux_index, n,
						demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID, demux[demux_index].ECMpids[n].ECM_PID, demux[demux_index].ECMpids[n].status);
			break;
		}
	}

	if (found == -1)
	{
		// prioritize CAIDs which already decoded same caid:provid
		for (n = 0; n < demux[demux_index].ECMpidcount; n++)
		{
			c = dvbapi_find_channel_cache(demux_index, n, 1);
			if (c != NULL)
			{
				cache=1; //found cache entry
				demux[demux_index].ECMpids[n].status = prio;
				cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X (found caid/provid in cache - weight: %d)", demux_index, n,
					demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID, demux[demux_index].ECMpids[n].ECM_PID, demux[demux_index].ECMpids[n].status);
			}
		}
	}

#if defined(MODULE_XCAMD)
	DVBICS_ChkEcmReaders(demux_index);
#endif

	// prioritize & ignore according to oscam.dvbapi and cfg.preferlocalcards
//	if (!dvbApi_priority) {
//		cs_log_dbg(D_DVBAPI,"[DVBAPI] No oscam.dvbapi found or no valid rules are parsed!");
//	}
	if ( dvbApi_priority)
	{
		struct s_reader *rdr;
		ECM_REQUEST *er;
		if (!cs_malloc(&er, sizeof(ECM_REQUEST))) return;

		int32_t add_prio=0; // make sure that p: values overrule cache
		if (cache==1)
			{ add_prio = prio; }
		else if (cache==2)
			{ add_prio = prio * 2; }

		// reverse order! makes sure that user defined p: values are in the right order
		int32_t p_order = demux[demux_index].ECMpidcount;

		highest_prio = (prio * demux[demux_index].ECMpidcount) + p_order;

		struct s_dvbapi_priority *p;
		for (p = dvbApi_priority; p != NULL; p = p->next)
		{
			if (p->type != 'p' && p->type != 'i')
				{ continue; }
			for (n = 0; n < demux[demux_index].ECMpidcount; n++)
			{
				if (!cache && demux[demux_index].ECMpids[n].status != 0)
					{ continue; }
				else if (cache==1 && (demux[demux_index].ECMpids[n].status < 0 || demux[demux_index].ECMpids[n].status > prio))
					{ continue; }
				else if (cache==2 && (demux[demux_index].ECMpids[n].status < 0 || demux[demux_index].ECMpids[n].status > prio*2))
					{ continue; }

				er->dmuxid = demux_index;// sky(powervu)
				er->caid   = er->ocaid = demux[demux_index].ECMpids[n].CAID;
				er->prid   = demux[demux_index].ECMpids[n].PROVID;
				er->pid    = demux[demux_index].ECMpids[n].ECM_PID;
				er->srvid  = demux[demux_index].program_number;
				er->onid   = demux[demux_index].onid;
#if defined(MODULE_AVAMGCAMD)
				er->exprid = er->prid;
				if (IS_IRDETO(er->caid)) {
					er->exprid = (((er->onid>>8)&0xff) ^ ((er->onid)&0xff));
				}
#endif
				er->client = cur_client();

				btun_caid  = chk_on_btun(SRVID_MASK, er->client, er);
				if (p->type == 'p' && btun_caid)
					{ er->caid = btun_caid; }

				if (p->caid   && p->caid   != er->caid)
					{ continue; }
				if (p->provid && p->provid != er->prid)
					{ continue; }
				if (p->ecmpid && p->ecmpid != er->pid)
					{ continue; }
				if (p->srvid  && p->srvid  != er->srvid)
					{ continue; }
				if (p->pidx && p->pidx-1 != n)
					{ continue; }

				if (p->type == 'i') { // check if ignored by dvbapi
					if (p->chid == 0x10000) { // ignore all? disable pid
						demux[demux_index].ECMpids[n].status = -1;
					}
					cs_log_dbg(D_DVBAPI, "Demuxer %d ignore ecmpid %d %04X:%06X:%04X:%04X (file)", demux_index, n, demux[demux_index].ECMpids[n].CAID,
							demux[demux_index].ECMpids[n].PROVID,demux[demux_index].ECMpids[n].ECM_PID, (uint16_t)p->chid);
					continue;
				}

				if (p->type == 'p')
				{
					if (demux[demux_index].ECMpids[n].status == -1) // skip ignores
						{ continue; }

					matching_done = 1;
					for (rdr=first_active_reader; rdr ; rdr=rdr->next)
					{
						if (cfg.preferlocalcards && !is_network_reader(rdr)
							&& rdr->card_status == CARD_INSERTED) // cfg.preferlocalcards = 1 local reader
						{

							if (matching_reader(er, rdr, 0))
							{
								if (cache==2 && demux[demux_index].ECMpids[n].status==1)
									{ demux[demux_index].ECMpids[n].status++; }
								else if (cache && !demux[demux_index].ECMpids[n].status)
									{ demux[demux_index].ECMpids[n].status += add_prio; }
								//priority*ECMpidcount should overrule network reader
								demux[demux_index].ECMpids[n].status += (prio * demux[demux_index].ECMpidcount) + (p_order--);
								cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X:%04X (localrdr: %s weight: %d)", demux_index,
										n, demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID,
										demux[demux_index].ECMpids[n].ECM_PID, (uint16_t)p->chid, rdr->label,
										demux[demux_index].ECMpids[n].status);
								break;
							}
						}
						else // cfg.preferlocalcards = 0 or cfg.preferlocalcards = 1 and no local reader
						{
							if (matching_reader(er, rdr, 0))
							{
								if (cache==2 && demux[demux_index].ECMpids[n].status==1)
									{ demux[demux_index].ECMpids[n].status++; }
								else if (cache && !demux[demux_index].ECMpids[n].status)
									{ demux[demux_index].ECMpids[n].status += add_prio; }
								demux[demux_index].ECMpids[n].status += prio + (p_order--);
								cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X:%04X (rdr: %s weight: %d)", demux_index,
										n, demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID,
										demux[demux_index].ECMpids[n].ECM_PID, (uint16_t)p->chid, rdr->label,
										demux[demux_index].ECMpids[n].status);
								break;
							}
						}
					}
				}
			}
		}
		NULLFREE(er);
	}

	if (!matching_done) // works if there is no oscam.dvbapi or if there is oscam.dvbapi but not p rules in it
	{
		if (dvbApi_priority && !matching_done)
			{ cs_log_dbg(D_DVBAPI, "Demuxer %d no prio rules in oscam.dvbapi matches!", demux_index); }

		struct s_reader *rdr;
		ECM_REQUEST *er;
		if (!cs_malloc(&er, sizeof(ECM_REQUEST))) return;

		highest_prio = prio*2;

		for (n=0; n<demux[demux_index].ECMpidcount; n++)
		{
			if (demux[demux_index].ECMpids[n].status == -1)	//skip ignores
				{ continue; }

			er->dmuxid = demux_index;// sky(powervu)
			er->onid   = demux[demux_index].onid;
			er->caid   = er->ocaid = demux[demux_index].ECMpids[n].CAID;
			er->prid   = demux[demux_index].ECMpids[n].PROVID;
			er->pid    = demux[demux_index].ECMpids[n].ECM_PID;
			er->srvid  = demux[demux_index].program_number;
#if defined(MODULE_AVAMGCAMD)
			er->exprid = er->prid;
			if (IS_IRDETO(er->caid)) {
				er->exprid = (((er->onid>>8)&0xff) ^ ((er->onid)&0xff));
			}
#endif
			er->client = cur_client();
			MYDVB_TRACE("mydvb:matching_check...{%04X.%06X, srvid.%5d}, ecmpid.%04X\n",
					er->caid, er->prid, er->srvid, er->pid);

			btun_caid  = chk_on_btun(SRVID_MASK, er->client, er);
			if (btun_caid)
				{ er->caid = btun_caid; }

			for (rdr=first_active_reader; rdr; rdr=rdr->next)
			{
				if (cfg.preferlocalcards
					&& !is_network_reader(rdr)
					&& rdr->card_status==CARD_INSERTED) // cfg.preferlocalcards = 1 local reader
				{
					if (matching_reader(er, rdr, 0))
					{
						demux[demux_index].ECMpids[n].status += prio*2;
						cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X (localrdr: %s weight: %d)", demux_index,
								n, demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID,
								demux[demux_index].ECMpids[n].ECM_PID, rdr->label,
								demux[demux_index].ECMpids[n].status);
						break;
					}
				}
				else // cfg.preferlocalcards = 0 or cfg.preferlocalcards = 1 and no local reader
				{
					if (matching_reader(er, rdr, 0))
					{
						demux[demux_index].ECMpids[n].status += prio;
						cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X (rdr: %s weight: %d)", demux_index,
								n, demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID,
								demux[demux_index].ECMpids[n].ECM_PID, rdr->label, demux[demux_index].ECMpids[n].status);
						break;
					}
				}
			}
		}
		NULLFREE(er);
	}

	if (cache==1)
		{ highest_prio += prio; }
	else if (cache==2)
		{ highest_prio += prio * 2; };

	highest_prio++;

	for (n=0; n<demux[demux_index].ECMpidcount; n++)
	{
		int32_t nr;
		SIDTAB  *sidtab;
		ECM_REQUEST er;

		er.dmuxid = demux_index;// sky(powervu)
		er.caid   = demux[demux_index].ECMpids[n].CAID;
		er.prid   = demux[demux_index].ECMpids[n].PROVID;
		er.srvid  = demux[demux_index].program_number;
		er.onid   = demux[demux_index].onid;
#if defined(MODULE_AVAMGCAMD)
		er.exprid = er.prid;
		if (IS_IRDETO(er.caid)) {
			er.exprid = (((er.onid>>8)&0xff) ^ ((er.onid)&0xff));
		}
#endif

		for (nr=0, sidtab=cfg.sidtab; sidtab; sidtab=sidtab->next, nr++)
		{
			if (sidtab->num_caid | sidtab->num_provid | sidtab->num_srvid)
			{
				if ((cfg.dvbapi_sidtabs.no&((SIDTABBITS)1<<nr)) && (chk_srvid_match(&er, sidtab)))
				{
					demux[demux_index].ECMpids[n].status = -1; //ignore
					cs_log_dbg(D_DVBAPI, "Demuxer %d ignore ecmpid %d %04X:%06X:%04X (service %s) pos %d", demux_index,
							n, demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID,
							demux[demux_index].ECMpids[n].ECM_PID, sidtab->label, nr);
				}
				if ((cfg.dvbapi_sidtabs.ok&((SIDTABBITS)1<<nr)) && (chk_srvid_match(&er, sidtab)))
				{
					demux[demux_index].ECMpids[n].status = highest_prio++; //priority
					cs_log_dbg(D_DVBAPI, "Demuxer %d prio ecmpid %d %04X:%06X:%04X (service: %s position: %d)", demux_index,
							n, demux[demux_index].ECMpids[n].CAID, demux[demux_index].ECMpids[n].PROVID,
							demux[demux_index].ECMpids[n].ECM_PID, sidtab->label,
							demux[demux_index].ECMpids[n].status);
				}
			}
		}
	}

	struct s_reader *rdr;
	ECM_REQUEST *er;
	if (!cs_malloc(&er, sizeof(ECM_REQUEST)))
		{ return; }

	for(n = 0; n < demux[demux_index].ECMpidcount; n++)
	{
		er->dmuxid = demux_index;// sky(powervu)
		er->caid   = er->ocaid = demux[demux_index].ECMpids[n].CAID;
		er->prid   = demux[demux_index].ECMpids[n].PROVID;
		er->pid    = demux[demux_index].ECMpids[n].ECM_PID;
		er->srvid  = demux[demux_index].program_number;
		er->onid   = demux[demux_index].onid;
		er->client = cur_client();
		btun_caid = chk_on_btun(SRVID_MASK, er->client, er);
		if (btun_caid)
		{
			er->caid = btun_caid;
		}

		int32_t match = 0;
		for(rdr = first_active_reader; rdr ; rdr = rdr->next)
		{
			if (matching_reader(er, rdr, 0))
			{
				match++;
			}
		}
		if (match == 0)
		{
			cs_log_dbg(D_DVBAPI, "Demuxer %d ignore ecmpid %d %04X:%06X:%04X:%04X (no matching reader)", demux_index, n, demux[demux_index].ECMpids[n].CAID,
						demux[demux_index].ECMpids[n].PROVID, demux[demux_index].ECMpids[n].ECM_PID, demux[demux_index].ECMpids[n].CHID);
			demux[demux_index].ECMpids[n].status = -1;
		}
	}
	NULLFREE(er);

	highest_prio = 0;
	int32_t highest_priopid = -1;
	for (n=0; n<demux[demux_index].ECMpidcount; n++)
	{
		if (demux[demux_index].ECMpids[n].status > highest_prio) // find highest prio pid
		{
			highest_prio = demux[demux_index].ECMpids[n].status;
			highest_priopid = n;
		}
		if (demux[demux_index].ECMpids[n].status == 0) { demux[demux_index].ECMpids[n].checked = 2; }  // set pids with no status to no prio run
	}

	struct s_dvbapi_priority *match;
	for (match = dvbApi_priority; match != NULL; match = match->next)
	{
		if (match->type != 'p')
			{ continue; }
		if (!match || !match->force) // only evaluate forced prio's
			{ continue; }
		for (n=0; n<demux[demux_index].ECMpidcount; n++)
		{
			if (match->caid   && match->caid   != demux[demux_index].ECMpids[n].CAID) 	 continue;
			if (match->provid && match->provid != demux[demux_index].ECMpids[n].PROVID) continue;
			if (match->srvid  && match->srvid  != demux[demux_index].program_number) 	 continue;
			if (match->ecmpid && match->ecmpid != demux[demux_index].ECMpids[n].ECM_PID)continue;
			if (match->pidx   && match->pidx-1 != n) { continue; }
			if (match->chid < 0x10000) demux[demux_index].ECMpids[n].CHID = match->chid;
			demux[demux_index].ECMpids[n].status = ++highest_prio;
			cs_log_dbg(D_DVBAPI, "Demuxer %d forced ecmpid %d %04X:%06X:%04X:%04X", demux_index, n, demux[demux_index].ECMpids[n].CAID,
					demux[demux_index].ECMpids[n].PROVID,demux[demux_index].ECMpids[n].ECM_PID, (uint16_t) match->chid);
			demux[demux_index].max_status = highest_prio;	// register maxstatus
			demux[demux_index].ECMpids[n].checked = 0; 		// set forced pid to prio run
			return; // we only accept one forced pid!
		}
	}
	demux[demux_index].max_status = highest_prio; // register maxstatus
	if (highest_priopid != -1 && found == highest_priopid && cache == 2) // Found entry in channelcache that is valid and has exact match on srvid
	{
		for(n = 0; n < demux[demux_index].ECMpidcount; n++)
		{
			if (n != found)
			{
				// disable non matching pid
				demux[demux_index].ECMpids[n].status = -1;
			}
			else
			{
				demux[demux_index].ECMpids[n].status = 1;
			}
		}
		demux[demux_index].max_emmfilters = maxfilter - 1;
		demux[demux_index].max_status = 1;
		cs_log("Demuxer %d found channel in cache and matching prio -> start descrambling ecmpid %d ", demux_index, found);
	}
	cs_ftime(&end);
	int64_t gone = comp_timeb(&end, &start);
	cs_log_dbg(D_DVBAPI, "Demuxer %d sorting the ecmpids took %"PRId64" ms", demux_index, gone);
	return;
}

void dvbapi_parse_descriptor(int32_t demux_id, uint32_t info_length, unsigned char *buffer)
{
// int32_t  ca_pmt_cmd_id = buffer[i + 5];
	uint32_t descriptor_length=0;
	uint32_t j,u;

	if (info_length<1)
		{ return; }

	if (!(buffer[0] == 0x09 || buffer[0] == 0x81)) // skip input we have no interest in: accept only 0x09 ecmpid descriptor and 0x81 enigma private descriptor
	{
		buffer++;
		info_length--;
	}

	for (j = 0; j < info_length; j += descriptor_length + 2)
	{
		descriptor_length = buffer[j+1];

		if (buffer[j] == 0x81 && descriptor_length == 8) // CAPMT_DESC_PRIVATE:private descriptor of length 8, assume enigma/tvh
		{
			demux[demux_id].enigma_namespace = b2i(4, buffer + j + 2);
			demux[demux_id].tsid = b2i(2, buffer + j + 6);
			demux[demux_id].onid = b2i(2, buffer + j + 8);
			cs_log_dbg(D_DVBAPI, "Demuxer %d found pmt type: %02x length: %d (assuming enigma private descriptor: namespace %04x tsid %02x onid %02x)", demux_id,
					buffer[j], descriptor_length, demux[demux_id].enigma_namespace, demux[demux_id].tsid, demux[demux_id].onid);
		}
		else if (descriptor_length !=0)
		{
			cs_log_dbg(D_TRACE, "Demuxer %d found pmt type: %02x length: %d", demux_id, buffer[j], descriptor_length);
		}

		if (buffer[j] != 0x09) continue;
		if (demux[demux_id].ECMpidcount >= ECM_PIDS) break;

		int32_t descriptor_ca_system_id = b2i(2, buffer + j + 2);
		int32_t descriptor_ca_pid = b2i(2, buffer + j + 4)&0x1FFF;
		int32_t descriptor_ca_provider = 0;

		if ((descriptor_ca_system_id>>8) == 0x01)
		{
			for (u=2; u<descriptor_length; u+=15)
			{
				descriptor_ca_pid = b2i(2, buffer + j + u + 2)&0x1FFF;
				descriptor_ca_provider = b2i(2, buffer + j + u + 4);
				dvbapi_add_ecmpid(demux_id, descriptor_ca_system_id, descriptor_ca_pid, descriptor_ca_provider);
			}
		}
		else
		{
			if (caid_is_viaccess(descriptor_ca_system_id) && descriptor_length == 0x0F && buffer[j + 12] == 0x14)
				{ descriptor_ca_provider = b2i(3, buffer + j + 14) &0xFFFFF0; }

			else if (caid_is_nagra(descriptor_ca_system_id) && descriptor_length == 0x07)
				{ descriptor_ca_provider = b2i(2, buffer + j + 7); }
			// sky(DRE3)
			else if (((descriptor_ca_system_id & 0xfffe)==0x7BE0||
						 (descriptor_ca_system_id & 0xfffe)==0x2710||
						 (descriptor_ca_system_id & 0xfffe)==0x4AE0) && descriptor_length > 4)
				{ descriptor_ca_provider = buffer[j + 6]; }
			else if ( descriptor_ca_system_id == 0x4A02) { /* sky(tongfang) */ }
			else if ((descriptor_ca_system_id>>8) == 0x4A && descriptor_length == 0x05)
				{ descriptor_ca_provider = buffer[j + 6]; }

			dvbapi_add_ecmpid(demux_id, descriptor_ca_system_id, descriptor_ca_pid, descriptor_ca_provider);

		}
	}

	// Apply mapping:
	if (dvbApi_priority)
	{
		struct s_dvbapi_priority *mapentry;
		for (j = 0; (int32_t)j < demux[demux_id].ECMpidcount; j++)
		{
			mapentry = dvbapi_check_prio_match(demux_id, j, 'm');
			if (mapentry)
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d mapping ecmpid %d from %04X:%06X to %04X:%06X", demux_id, j,
						demux[demux_id].ECMpids[j].CAID, demux[demux_id].ECMpids[j].PROVID,
						mapentry->mapcaid, mapentry->mapprovid);
				demux[demux_id].ECMpids[j].CAID   = mapentry->mapcaid;
				demux[demux_id].ECMpids[j].PROVID = mapentry->mapprovid;
			}
		}
	}
}

void dvbapi_request_cw(struct s_client *client, ECM_REQUEST *er, int32_t demux_id, int32_t filnum, uint8_t delayed_ecm_check)
{
	int32_t filternum = dvbapi_set_section_filter(demux_id, er, -1); // set ecm filter to odd -> even and visaversa

	if (!er) return;
//	myprdump("ECM", er->ecm, er->ecmlen);
 	MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ECM{%02X.%d} {%02X:%06X,%04X(%02X.%04X)}(f.%d.%d)\n",
					er->ecm[0],
					er->ecmlen,
					er->caid,
					er->prid,
					er->pid,
					er->ecm[4],
					er->chid,
					filnum, demux[demux_id].demux_fd[filnum].pidindex);

//	{
//		uint8_t cw[16];
//		XVIACESS_Process(CSREADER_GetReaders(R_XCAS), er, cw);
//	}

	filternum = filnum;
#if defined WITH_AZBOX || defined WITH_MCA
#elif defined(WITH_HISILICON)
#else
	if (filternum < 0 || filternum > MAX_FILTER-1) {
		cs_log_dbg(D_DVBAPI, "Demuxer %d not requesting cw -> ecm filter was killed!", demux_id);
		return;
	}
#endif

	if (!get_cw(client, er)) return;

#if defined WITH_AZBOX || defined WITH_MCA
#else
	if (delayed_ecm_check) memcpy(demux[demux_id].demux_fd[filternum].ecmd5, er->ecmd5, CS_ECMSTORESIZE); // register this ecm as latest request for this filter
	else memset(demux[demux_id].demux_fd[filternum].ecmd5, 0, CS_ECMSTORESIZE); // zero out ecmcheck!
#endif

#ifdef WITH_DEBUG
	if (D_DVBAPI & cs_dblevel) {
		char buf[ECM_FMT_LEN];
		format_ecm(er, buf, ECM_FMT_LEN);
		cs_log_dbg(D_DVBAPI, "Demuxer %d request controlword for ecm %s", demux_id, buf);
	}
#endif
}

void dvbapi_try_next_caid(int32_t demux_id, int8_t checked, int32_t atfirst)
{
	int32_t n, j, found = -1, started = 0;
	int32_t status = demux[demux_id].max_status;

	MYDVB_TRACE("mydvb:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TRY{%d(%d)}{%d}\n",
			checked,
			demux_id,
			demux[demux_id].max_status);

	for (j = status; j >= 0; j--) // largest status first!
	{
		for (n=0; n<demux[demux_id].ECMpidcount; n++)
		{
			//cs_log_dbg(D_DVBAPI,"Demuxer %d PID %d checked = %d status = %d (searching for pid with status = %d)", demux_id, n,
			//			demux[demux_id].ECMpids[n].checked, demux[demux_id].ECMpids[n].status, j);
			if (demux[demux_id].ECMpids[n].checked == checked && demux[demux_id].ECMpids[n].status == j)
			{
				found = n;
#if defined WITH_AZBOX || defined WITH_MCA
				openxcas_set_provid(demux[demux_id].ECMpids[found].PROVID);
				openxcas_set_caid(demux[demux_id].ECMpids[found].CAID);
				openxcas_set_ecm_pid(demux[demux_id].ECMpids[found].ECM_PID);
#endif

				// fixup for cas that need emm first!
				if (caid_is_irdeto(demux[demux_id].ECMpids[found].CAID)) { demux[demux_id].emmstart.time = 0; }
				started = dvbapi_start_descrambling(demux_id, found, checked);
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
				DVBICS_ChSettings(demux_id, (atfirst) ? CHCS_CASFIRST : CHCS_CASFOWARD);
#endif
				if (cfg.dvbapi_requestmode == 0 && started == 1) { return; } // in requestmode 0 we only start 1 ecm request at the time
			}
		}
	}

	if (found == -1 && demux[demux_id].pidindex == -1)
	{
		cs_log("Demuxer %d no suitable readers found that can be used for decoding!", demux_id);
		return;
	}
}

// sky(n)
void dvbapi_try_goto_caid(int32_t demux_id, int8_t checked, int32_t chknum)
{

	int32_t n, j, found = -1, started = 0;

	int32_t status = demux[demux_id].max_status;

	MYDVB_TRACE("mydvb:gggggggggggggggggggggggggggggggggggggggg TRY{%d(%d)}{%d, %d}\n",
			checked,
			demux_id,
			demux[demux_id].max_status, chknum);

	for (j = status; j >= 0; j--) // largest status first!
	{
		for (n=0; n<demux[demux_id].ECMpidcount; n++)
		{
			//	cs_log_dbg(D_DVBAPI,"[DVBAPI] Demuxer %d PID %d checked = %d status = %d (searching for pid with status = %d)", demux_id, n,
			//			demux[demux_id].ECMpids[n].checked, demux[demux_id].ECMpids[n].status, j);
			if (chknum==n)
			{
				found = n;
				// fixup for cas that need emm first!
				if (caid_is_irdeto(demux[demux_id].ECMpids[found].CAID)) { demux[demux_id].emmstart.time = 0; }
				started = dvbapi_start_descrambling(demux_id, found, checked);

#if defined(MODULE_XCAMD)
				DVBICS_ChSettings(demux_id, CHCS_CAS2FAKE);
#endif
			   if (cfg.dvbapi_requestmode == 0 && started == 1) return; // in requestmode 0 we only start 1 ecm request at the time
			}
		}
	}

	if (found == -1 && demux[demux_id].pidindex == -1)
	{
		cs_log("Demuxer %d no suitable readers found that can be used for decoding!", demux_id);
		return;
	}
}

void dvbapi_try_stop_caid(int32_t demux_id, int8_t checked)
{
	if (demux[demux_id].ECMpidcount == 1) return;
	MYDVB_TRACE("mydvb:nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn STP{%d(%d)}\n", checked, demux_id);
	if (checked==1) {
		dvbapi_try_next_caid(demux_id, 0, 0);
	}
	else {
		dvbapi_stop_filter(demux_id, TYPE_ECM);
	}
}


static void getDemuxOptions(int32_t demux_id, unsigned char *buffer, uint16_t *ca_mask, uint16_t *demux_index, uint16_t *adapter_index, uint16_t *pmtpid)
{
	*ca_mask=0x01, *demux_index=0x00, *adapter_index=0x00, *pmtpid = 0x00;

	if (buffer[17]==0x82 && buffer[18]==0x02)
	{
		// CAPMT_DESC_DEMUX:enigma2
		*ca_mask     = buffer[19];
		uint32_t demuxidx = buffer[20];
		if (demuxidx == 0xff) demuxidx = 0; // tryfix prismcube (0xff -> "demux-1" = error! )
		*demux_index = demuxidx;
	//	if (buffer[21]==0x84 && buffer[22]==0x02) *pmtpid = (buffer[23] << 8) | buffer[24];
		if (buffer[25]==0x83 && buffer[26]==0x01) *adapter_index=buffer[27]; // from code cahandler.cpp 0x83 index of adapter
	}

	if (cfg.dvbapi_boxtype == BOXTYPE_IPBOX_PMT) {
		*ca_mask     = demux_id + 1;
		*demux_index = demux_id;
	}

	if (cfg.dvbapi_boxtype == BOXTYPE_QBOXHD && buffer[17]==0x82 && buffer[18]==0x03)
	{
	//  ca_mask 		= buffer[19];		 // with STONE 1.0.4 always 0x01
		*demux_index   = buffer[20]; 		 // with STONE 1.0.4 always 0x00
		*adapter_index = buffer[21]; 		 // with STONE 1.0.4 adapter index can be 0,1,2
		*ca_mask = (1 << *adapter_index); // use adapter_index as ca_mask (used as index for ca_fd[] array)
	}

	if ((cfg.dvbapi_boxtype == BOXTYPE_PC || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX) && buffer[7]==0x82 && buffer[8]==0x02)
	{
		*demux_index   = buffer[ 9]; 		 // it is always 0 but you never know
		*adapter_index = buffer[10]; 		 // adapter index can be 0,1,2
		*ca_mask = (1 << *adapter_index); // use adapter_index as ca_mask (used as index for ca_fd[] array)
	}

#if defined(WITH_HISILICON)
	// tvheadend
	// CAPMT_DESC_DEMUX
	if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON && buffer[7]==0x82 && buffer[8]==0x02)
	{
		*demux_index   = buffer[ 9]; 		 // it is always 0 but you never know
		*adapter_index = buffer[10]; 		 // adapter index can be 0,1,2
		*ca_mask = (1 << *adapter_index); // use adapter_index as ca_mask (used as index for ca_fd[] array)
	}
#endif
}

static void dvbapi_capmt_notify(struct demux_s *dmx)
{
	struct s_client *cl;
	for (cl = first_client->next; cl ; cl = cl->next)
	{
		if ((cl->typ == 'p' || cl->typ == 'r') && cl->reader && cl->reader->ph.c_capmt)
		{
			struct demux_s *curdemux;
			if (cs_malloc(&curdemux, sizeof(struct demux_s)))
			{
				memcpy(curdemux, dmx, sizeof(struct demux_s));
				add_job(cl, ACTION_READER_CAPMT_NOTIFY, curdemux, sizeof(struct demux_s));
			}
		}
	}
}

#if defined(WITH_HISILICON)
void
DVBICS_csdescriptor_parse(int32_t demux_id, uint32_t pgminfosize, unsigned char *buffer)
{
	uint32_t descriptor_len=0;
	uint32_t i;
	short		longitude;
	// tvheadend(WITH_HISILICON)
	// 03 00 02 13 00 19 01
	//	82 02 00 03
	//	81 08 00 00 00 00 00 02 00 01
	//	84 02 00 4E
	//	09 04 06 11 E0 4E
	// 01 00 85 00 06
	if (pgminfosize<1) return;
	if (buffer[0]==0x01)
	{
		buffer = buffer+1;
		pgminfosize--;
	}

	for (i=0; i<pgminfosize; i+=(descriptor_len+2))
	{
		descriptor_len = buffer[i+1];
		switch (buffer[i]) {
			case 0x83:	// CAPMT_DESC_INFO
				longitude = (short)(buffer[i+ 2] << 8 | buffer[i+ 3]);
				if (longitude < 0) longitude = 3600 + (short)longitude;
				demux[demux_id].cs_degree	  		= longitude;
				demux[demux_id].cs_frequency 		= (buffer[i+ 4] << 8 | buffer[i+ 5]);
				demux[demux_id].cs_vidpid 			= (buffer[i+ 6] << 8 | buffer[i+ 7]);
				demux[demux_id].cs_symbolrate 	= (buffer[i+ 8] << 8 | buffer[i+ 9]);
				demux[demux_id].cs_polarisation 	= (buffer[i+10] << 8 | buffer[i+11]);
				mycs_trace(D_ADB, "mypmt:--- chiks:%d, %d.%d,%d, vpid:%04x",
							demux[demux_id].cs_degree,
							demux[demux_id].cs_frequency,
							demux[demux_id].cs_symbolrate,
							demux[demux_id].cs_polarisation,
							demux[demux_id].cs_vidpid);
				break;

			case 0x84:	// CAPMT_DESC_PID
				demux[demux_id].pmtpid = (buffer[i+2] << 8 | buffer[i+3]);
				mycs_trace(D_ADB, "mypmt:--- pmtpid:%04x", demux[demux_id].pmtpid);
				break;

			case 0x81:	// CAPMT_DESC_PRIVATE
				// parse private descriptor as used by enigma (4 bytes namespace, 2 tsid, 2 onid)
				demux[demux_id].enigma_namespace=(buffer[i+2] << 24 | buffer[i+3] << 16 | buffer[i+4] << 8 | buffer[i+5]);
				demux[demux_id].tsid = (buffer[i+6] << 8 | buffer[i+7]);
				demux[demux_id].onid = (buffer[i+8] << 8 | buffer[i+9]);
				mycs_trace(D_ADB, "mypmt:--- tsid:%04X, onid=%04X", demux[demux_id].tsid, demux[demux_id].tsid);
				break;

			case 0x09: return;
			case 0x00: return;
			default:
				mycs_trace(D_ADB, "mypmt:--- type:%02X, len{%d}", buffer[i], descriptor_len);
				break;
		}
	}
}
#endif // #if defined(WITH_HISILICON)

#define CAPMT_LIST_MORE   				0x00	// append a 'MORE' CAPMT object the list and start receiving the next object
#define CAPMT_LIST_FIRST  				0x01	// clear the list when a 'FIRST' CAPMT object is received, and start receiving the next object
#define CAPMT_LIST_LAST   				0x02	// append a 'LAST' CAPMT object to the list and start working with the list
#define CAPMT_LIST_ONLY   				0x03 	// clear the list when an 'ONLY' CAPMT object is received, and start working with the object
#define CAPMT_LIST_ADD    				0x04	// append an 'ADD' CAPMT object to the current list and start working with the updated list
#define CAPMT_LIST_UPDATE 				0x05 	// replace an entry in the list with an 'UPDATE' CAPMT object, and start working with the updated list
// ca_pmt_cmd_id values:
#define CAPMT_CMD_OK_DESCRAMBLING	0x01  // start descrambling the service in this CAPMT object as soon as the list of CAPMT objects is complete
#define CAPMT_CMD_OK_MMI            0x02  //
#define CAPMT_CMD_QUERY             0x03  //
#define CAPMT_CMD_NOT_SELECTED      0x04
// ca_pmt_descriptor types
#define CAPMT_DESC_PRIVATE 			0x81
#define CAPMT_DESC_DEMUX   			0x82
#define CAPMT_DESC_PID     			0x84

#define LIST_MORE 	CAPMT_LIST_MORE
#define LIST_FIRST 	CAPMT_LIST_FIRST
#define LIST_LAST 	CAPMT_LIST_LAST
#define LIST_ONLY 	CAPMT_LIST_ONLY
#define LIST_ADD 		CAPMT_LIST_ADD
#define LIST_UPDATE 	CAPMT_LIST_UPDATE


int32_t dvbapi_parse_capmt(unsigned char *buffer, uint32_t length, int32_t connfd, char *pmtfile)
{
	uint32_t i = 0, start_descrambling = 0;
	int32_t  j = 0;
	int32_t  demux_id = -1;
	uint16_t ca_mask, demux_index, adapter_index, pmtpid = 0;
#if WITH_COOLAPI
	int32_t  ca_pmt_list_management = LIST_ONLY;
#else
	int32_t  ca_pmt_list_management = buffer[0];
#endif
	uint32_t program_number = b2i(2, buffer + 1);
	uint32_t program_info_length = b2i(2, buffer + 4) &0xFFF;

 	MYDVB_TRACE("mydvb:PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP CAPMT{%d}\n", program_number);
	MYDVB_TRACE("mydvb:sends PMT(%d) command %d for channel %d\n", ca_pmt_list_management, cfg.dvbapi_pmtmode, program_number);
	cs_log_dump_dbg(D_DVBAPI, buffer, length, "capmt:");
	cs_log_dbg(D_DVBAPI, "Receiver sends PMT command %d for channel %04X", ca_pmt_list_management, program_number);
	if ((ca_pmt_list_management == LIST_FIRST || ca_pmt_list_management == LIST_ONLY))
	{
		for (i = 0; i < MAX_DEMUX; i++)
		{
			if (cfg.dvbapi_pmtmode == 6 && pmt_stopmarking == 1) { continue; } // already marked -> skip!
			if (demux[i].program_number == 0) { continue; }  // skip empty demuxers
			if (demux[i].ECMpidcount != 0 && demux[i].pidindex != -1 ) { demux[i].running = 1; }  // running channel changes from scrambled to fta
			if (demux[i].socket_fd != connfd) { continue; }  // skip demuxers belonging to other ca pmt connection
			if (cfg.dvbapi_pmtmode == 6)
			{
			  demux[i].stopdescramble = 1; // Mark for deletion if not used again by following pmt objects.
			  cs_log_dbg(D_DVBAPI, "Marked demuxer %d/%d (srvid = %04X fd = %d) to stop decoding", i, MAX_DEMUX, demux[i].program_number, connfd);
		  }
	  	}
		pmt_stopmarking = 1;
	}
	for (i = 0; i < MAX_DEMUX; i++) // search current demuxers for running the same program as the one we received in this PMT object
	{
		if (demux[i].program_number == 0) { continue; }
		if (cfg.dvbapi_boxtype == BOXTYPE_IPBOX_PMT) demux_index = i; // fixup for ipbox

		bool full_check = 1, matched = 0;
		if (config_enabled(WITH_COOLAPI) || is_samygo) full_check = 0;

		if (full_check)
			matched = (connfd > 0 && demux[i].socket_fd == connfd) && demux[i].program_number == program_number;
		else
			matched =  connfd > 0 && demux[i].program_number == program_number;

		if (matched)
		{
			MYDVB_TRACE("mydvb:capmt same.....\n");
			getDemuxOptions(i, buffer, &ca_mask, &demux_index, &adapter_index, &pmtpid);
			if (full_check) {
			   if (demux[i].adapter_index != adapter_index) continue; // perhaps next demuxer matches?
			   if (demux[i].ca_mask != ca_mask) continue; // perhaps next demuxer matches?
			   if (demux[i].demux_index != demux_index) continue; // perhaps next demuxer matches?
			}
			if (ca_pmt_list_management == LIST_UPDATE) {
				cs_log("Demuxer %d PMT update for decoding of SRVID %04X! ", i, program_number);
			}

			demux_id = i;

#if defined(WITH_HISILICON)
			dvbapi_stop_descrambling(i);
#endif
			cs_log("Demuxer %d continue decoding of SRVID %04X", i, demux[i].program_number);

#if defined WITH_AZBOX || defined WITH_MCA
			openxcas_set_sid(program_number);
#endif

			demux[i].stopdescramble = 0; // dont stop current demuxer!
			break; // no need to explore other demuxers since we have a found!
		}
	}

	// stop descramble old demuxers from this ca pmt connection that arent used anymore
	if ((ca_pmt_list_management == LIST_LAST) || (ca_pmt_list_management == LIST_ONLY))
	{
		for (j = 0; j < MAX_DEMUX; j++)
		{
			if (demux[j].program_number == 0) { continue; }
			if (demux[j].stopdescramble == 1) { dvbapi_stop_descrambling(j); }  // Stop descrambling and remove all demuxer entries not in new PMT.
		}
		start_descrambling = 1; // flag that demuxer descrambling is to be executed!
	}

	if (demux_id==-1)
	{
		for(demux_id=0; demux_id<MAX_DEMUX && demux[demux_id].program_number>0; demux_id++);
#if defined(WITH_HISILICON)
// multi-demux simulation
		if (demux_id) {
			MYDVB_TRACE("mydvb:demux_id strange{%d}.......\n", demux_id);
		// sky(quad)
		//	demux_id = 0;
		}
#endif
	}

	if (demux_id>=MAX_DEMUX)
 	{
		MYDVB_TRACE("mydvb:No free id (MAX_DEMUX).......\n");
		cs_log("ERROR: No free id (MAX_DEMUX)");
		return -1;
	}
	myprintf("mydvb:start emmyyy{%d}{%d,%d}\n", demux_id, demux[demux_id].running, (int)demux[demux_id].emmstart.time);
	demux[demux_id].program_number = program_number; // do this early since some prio items use them!
	demux[demux_id].pmtpid = pmtpid;
	// CAPMT_DESC_PRIVATE
	if (buffer[7]==0x81 && buffer[8]==0x08) {
		// parse private descriptor as used by enigma (4 bytes namespace, 2 tsid, 2 onid)
		demux[demux_id].enigma_namespace=(buffer[9] << 24 | buffer[10] << 16 | buffer[11] << 8 | buffer[12]);
		demux[demux_id].tsid = (buffer[13] << 8 | buffer[14]);
		demux[demux_id].onid = (buffer[15] << 8 | buffer[16]);
	}
	else {
		demux[demux_id].enigma_namespace=0;
		demux[demux_id].tsid = 0;
		demux[demux_id].onid = 0;
	}

#if defined(WITH_HISILICON)
	DVBICS_csdescriptor_parse(demux_id, program_info_length-1, buffer+7);
#endif

	if (pmtfile)
	{
		cs_strncpy(demux[demux_id].pmt_file, pmtfile, sizeof(demux[demux_id].pmt_file));
	}

	for(j = 0; j < demux[demux_id].ECMpidcount; j++) // cleanout demuxer from possible stale info
	{
		demux[demux_id].ECMpids[j].streams = 0; // reset streams of each ecmpid!
	}
	demux[demux_id].STREAMpidcount = 0; // reset number of streams
	demux[demux_id].ECMpidcount = 0; // reset number of ecmpids

	if (program_info_length > 1 && program_info_length < length)
	{
		dvbapi_parse_descriptor(demux_id, program_info_length - 1, buffer + 6);
	}

	uint32_t es_info_length = 0, vpid = 0;
	struct s_dvbapi_priority *addentry;

#if defined(WITH_HISILICON)
	vpid = demux[demux_id].cs_vidpid;
#endif
	MYDVB_TRACE("mydvb:program_info(%d, %d)\n", program_info_length, length);
	for (i = program_info_length + 6; i < length; i += es_info_length + 5)
	{
		int32_t  stream_type = buffer[i];
		uint16_t elementary_pid = b2i(2, buffer + i + 1)&0x1FFF;
		es_info_length = b2i(2, buffer + i +3)&0x0FFF;
		mycs_trace(D_ADB, "mypmt:--- elementary:%02X,%04X{%d}", stream_type, elementary_pid, es_info_length);
		cs_log_dbg(D_DVBAPI, "Found stream_type: %02x pid: %04x length: %d", stream_type, elementary_pid, es_info_length);

		if (demux[demux_id].STREAMpidcount >= ECM_PIDS) break;
		demux[demux_id].STREAMpids[demux[demux_id].STREAMpidcount++] = elementary_pid;
#if defined(WITH_HISILICON)
		// elementary_pid: tvheadend capmt sequence number.
#else
		// find and register videopid
		if (!vpid && (stream_type == 01 || stream_type == 02 || stream_type == 0x10 || stream_type == 0x1B)) {
			vpid = elementary_pid;
		}
#endif
		// sky(tvheadend)
		if (es_info_length != 0 && (i+5+es_info_length) < length) {
			dvbapi_parse_descriptor(demux_id, es_info_length, buffer+i+5);
		}
		else {
			for (addentry=dvbApi_priority; addentry != NULL; addentry=addentry->next)
			{
				if (addentry->type != 'a'
						|| (addentry->ecmpid &&  pmtpid && addentry->ecmpid != pmtpid) // ecmpid is misused to hold pmtpid in case of A: rule
						|| (addentry->ecmpid && !pmtpid && addentry->ecmpid != vpid) // some receivers dont forward pmtpid, use vpid instead
						|| (addentry->srvid != demux[demux_id].program_number))
					{ continue; }
				cs_log_dbg(D_DVBAPI, "Demuxer %d fake ecmpid %04X:%06x:%04x for unencrypted stream on srvid %04X", demux_id, addentry->mapcaid, addentry->mapprovid,
							addentry->mapecmpid, demux[demux_id].program_number);
				dvbapi_add_ecmpid(demux_id, addentry->mapcaid, addentry->mapecmpid, addentry->mapprovid);
				break;
			}
		}
	}
	for (j = 0; j < demux[demux_id].ECMpidcount; j++)
	{
		demux[demux_id].ECMpids[j].VPID = vpid; // register found vpid on all ecmpids of this demuxer
	}
	cs_log("Demuxer %d found %d ECMpids and %d STREAMpids in PMT", demux_id, demux[demux_id].ECMpidcount, demux[demux_id].STREAMpidcount);

	getDemuxOptions(demux_id, buffer, &ca_mask, &demux_index, &adapter_index, &pmtpid);
	char channame[32];
	get_servicename(dvbApi_client, demux[demux_id].program_number, demux[demux_id].ECMpidcount > 0 ? demux[demux_id].ECMpids[0].CAID : NO_CAID_VALUE, channame);
	cs_log("Demuxer %d serving srvid %04X (%s) on adapter %04X camask %04X index %04X pmtpid %04X", demux_id,
		   demux[demux_id].program_number, channame, adapter_index, ca_mask, demux_index, pmtpid);

	demux[demux_id].adapter_index = adapter_index;
	demux[demux_id].ca_mask = ca_mask;
	demux[demux_id].rdr = NULL;
	// sky(powervu)
	#if defined(WITH_HISILICON)
		demux[demux_id].demux_index = demux_id;
	#else
		demux[demux_id].demux_index = demux_index;
	#endif
	demux[demux_id].socket_fd = connfd;
	demux[demux_id].stopdescramble = 0; // remove deletion mark!

	// remove from unassoc_fd when necessary
	for (j = 0; j < MAX_DEMUX; j++)
		if (unassoc_fd[j] == connfd)
			unassoc_fd[j] = 0;

	demux[demux_id].scrambled_counter = 0;
	demux[demux_id].cs_soleMatch 	 	 = 0;
	demux[demux_id].cs_ecmRequisite 	 = -1;
	demux[demux_id].cs_filtnum     	 = 0;
	demux[demux_id].cs_subsequence    = 0;
	demux[demux_id].constcw_fbiss     = 0;
	MYDVB_TRACE("mydvb:capmt{%d,%d,%d}{%d, %dEA}\n", demux_id, adapter_index, demux_index, program_number, demux[demux_id].ECMpidcount);

	dvbapi_capmt_notify(&demux[demux_id]);

// sky.futures(cas noinfromation)
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM) || defined(MODULE_CONSTCW)
	DVBICS_ChDescrambled(demux_id, false);
	#if defined(MODULE_CONSTCW)
	if (demux[demux_id].ECMpidcount==0)
	{
		if (CSREADER_IsFTAbiss(demux[demux_id].cs_degree,
				demux[demux_id].cs_frequency,
				demux[demux_id].program_number,
				vpid))
		{
			mycs_log("mydvb:fta{%d}", program_number);
			demux[demux_id].ECMpids[0].CAID    = 0x2600;
			demux[demux_id].ECMpids[0].PROVID  = cs_i2BCD(demux[demux_id].cs_frequency);
			demux[demux_id].ECMpids[0].ECM_PID = cs_i2BCD(demux[demux_id].cs_degree);
			demux[demux_id].ECMpids[0].VPID    = vpid;
			demux[demux_id].ECMpids[0].constcw = 1;
			demux[demux_id].ECMpidcount   = 1;
			demux[demux_id].constcw_fbiss = 9;
		}
	}
	#endif
#endif

	struct s_dvbapi_priority *xtraentry;
	int32_t k, l, m, xtra_demux_id;

	for (xtraentry=dvbApi_priority; xtraentry != NULL; xtraentry=xtraentry->next)
	{
		if (xtraentry->type != 'x') { continue; }

		for (j = 0; j <= demux[demux_id].ECMpidcount; ++j)
		{
			if ((xtraentry->caid && xtraentry->caid != demux[demux_id].ECMpids[j].CAID)
				|| (xtraentry->provid && xtraentry->provid != demux[demux_id].ECMpids[j].PROVID)
				|| (xtraentry->ecmpid && xtraentry->ecmpid != demux[demux_id].ECMpids[j].ECM_PID)
				|| (xtraentry->srvid  && xtraentry->srvid  != demux[demux_id].program_number))
				{ continue; }

			cs_log("Mapping ecmpid %04X:%06X:%04X:%04X to xtra demuxer/ca-devices", xtraentry->caid, xtraentry->provid, xtraentry->ecmpid, xtraentry->srvid);

			for (xtra_demux_id=0; xtra_demux_id<MAX_DEMUX && demux[xtra_demux_id].program_number>0; xtra_demux_id++)
				{ ; }

			if (xtra_demux_id >= MAX_DEMUX)
			{
				cs_log("Found no free demux device for xtra streams.");
				continue;
			}
			// copy to new demuxer
			getDemuxOptions(demux_id, buffer, &ca_mask, &demux_index, &adapter_index, &pmtpid);
			demux[xtra_demux_id].ECMpids[0] 		= demux[demux_id].ECMpids[j];
			demux[xtra_demux_id].ECMpidcount 	= 1;
			demux[xtra_demux_id].STREAMpidcount = 0;
			demux[xtra_demux_id].program_number = demux[demux_id].program_number;
			demux[xtra_demux_id].pmtpid 			= demux[demux_id].pmtpid;
			// sky(powervu)
			#if defined(WITH_HISILICON)
				demux[xtra_demux_id].demux_index = xtra_demux_id;
			#else
				demux[xtra_demux_id].demux_index = demux_index;
			#endif
			demux[xtra_demux_id].adapter_index 	= adapter_index;
			demux[xtra_demux_id].ca_mask 			= ca_mask;
			demux[xtra_demux_id].socket_fd 		= connfd;
			demux[xtra_demux_id].stopdescramble = 0; // remove deletion mark!
			demux[xtra_demux_id].rdr 				= NULL;
			demux[xtra_demux_id].curindex 		= -1;
			demux[xtra_demux_id].cs_cwidx 		= -1;

			// add streams to xtra demux
			for (k = 0; k < demux[demux_id].STREAMpidcount; ++k)
			{
				if (!demux[demux_id].ECMpids[j].streams || demux[demux_id].ECMpids[j].streams & (1 << k))
				{
					demux[xtra_demux_id].ECMpids[0].streams |= (1 << demux[xtra_demux_id].STREAMpidcount);
					demux[xtra_demux_id].STREAMpids[demux[xtra_demux_id].STREAMpidcount] = demux[demux_id].STREAMpids[k];
					++demux[xtra_demux_id].STREAMpidcount;

					// shift stream associations in normal demux because we will remove the stream entirely
					for (l = 0; l < demux[demux_id].ECMpidcount; ++l)
					{
						for (m = k; m < demux[demux_id].STREAMpidcount-1;  ++m)
						{
							if(demux[demux_id].ECMpids[l].streams & (1 << (m + 1)))
							{
								demux[demux_id].ECMpids[l].streams |=  (1 <<  m);
							}
							else
							{
								demux[demux_id].ECMpids[l].streams &= ~(1 <<  m);
							}
						}
					}

					// remove stream association from normal demux device
					for(l = k; l < demux[demux_id].STREAMpidcount - 1; ++l)
					{
						demux[demux_id].STREAMpids[l] = demux[demux_id].STREAMpids[l+1];
					}
					--demux[demux_id].STREAMpidcount;
					--k;
				}
			}

			// remove ecmpid from normal demuxer
			for (k = j; k < demux[demux_id].ECMpidcount; ++k)
			{
				demux[demux_id].ECMpids[k] = demux[demux_id].ECMpids[k+1];
			}
			--demux[demux_id].ECMpidcount;
			--j;

			if (demux[xtra_demux_id].STREAMpidcount <= 0)
			{
				cs_log("Found no streams for xtra demuxer. Not starting additional decoding on it.");
				demux[xtra_demux_id].program_number = 0;
				demux[xtra_demux_id].stopdescramble = 1;
			}

			if (demux[demux_id].STREAMpidcount < 1)
			{
				cs_log("Found no streams for normal demuxer. Not starting additional decoding on it.");
			}
		}
	}

	myprintf("mydvb:start emmxxx{%d,%d}\n", demux[demux_id].running, (int)demux[demux_id].emmstart.time);
	if (cfg.dvbapi_au > 0 && demux[demux_id].EMMpidcount == 0) // only do emm setup if au enabled and not running!
	{
	   demux[demux_id].emm_filter = -1; // to register first run emmfilter start
	   if (demux[demux_id].emmstart.time == 1)   // irdeto fetch emm cat direct!
	   {
		   cs_ftime(&demux[demux_id].emmstart); // trick to let emm fetching start after 30 seconds to speed up zapping
			myprintf("mydvb:start CAT\n");
		   dvbapi_start_filter(demux_id,
					   demux[demux_id].pidindex,
					   CAT_PID,
					   0x001,
					   0x01,
					   0x01,
					   0xFF,
					   0,
					   TYPE_EMM); // CAT
	   }
		else { cs_ftime(&demux[demux_id].emmstart); } // for all other caids delayed start!
	}

	if (start_descrambling)
	{
		for (j = 0; j < MAX_DEMUX; j++)
		{
			if (demux[j].program_number == 0) { continue; }

			if (demux[j].running) disable_unused_streampids(j); // disable all streampids not in use anymore

			if (demux[j].running == 0 && demux[j].ECMpidcount != 0 )   // only start demuxer if it wasnt running
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d/%d lets start descrambling (srvid = %04X fd = %d ecmpids = %d)", j, MAX_DEMUX,
					demux[j].program_number, connfd, demux[j].ECMpidcount);
				demux[j].running = 1;  // mark channel as running
			//	openxcas_set_sid(demux[j].program_number);
				demux[j].decodingtries = -1;
				dvbapi_resort_ecmpids(j);
				dvbapi_try_next_caid(j, 0, 1);
				cs_sleepms(1);
			}
			else if (demux[j].ECMpidcount == 0) //fta do logging and part of ecmhandler since there will be no ecms asked!
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d/%d no descrambling needed (srvid = %04X fd = %d ecmpids = %d)", j, MAX_DEMUX,
					demux[j].program_number, connfd, demux[j].ECMpidcount);
				demux[j].running = 0; // reset running flag
				demux[demux_id].pidindex = -1; // reset ecmpid used for descrambling
				dvbapi_stop_filter(j, TYPE_ECM);
				if (cfg.usrfileflag) { cs_statistics(dvbApi_client);} // add to user log previous channel + time on channel
				dvbApi_client->last_srvid = demux[demux_id].program_number; // set new channel srvid
				dvbApi_client->last_caid  = NO_CAID_VALUE; // FTA channels have no caid!
				dvbApi_client->lastswitch = dvbApi_client->last = time((time_t *)0); // reset idle-Time & last switch
			}
		}
	}
	return demux_id;
}


void dvbapi_handlesockmsg(unsigned char *buffer, uint32_t len, int32_t connfd)
{
	uint32_t val=0, size=0, i, k;

	for (k = 0; k < len; k += (3 + size + val))
	{
		if (buffer[0+k] != 0x9F || buffer[1+k] != 0x80)
		{
			cs_log_dbg(D_DVBAPI,"Received unknown PMT command: %02x", buffer[0+k]);
			break;
		}

		if (k > 0) {
			cs_log_dump_dbg(D_DVBAPI, buffer+k, len-k,"Parsing next PMT object:");
		}
		if (buffer[3+k] & 0x80) {
			val  = 0;
			size = buffer[3+k] & 0x7F;
			for (i = 0; i < size; i++)
				val = (val << 8) | buffer[i + 1 + 3 + k];
			size++;
		}
		else {
			val  = buffer[3+k] & 0x7F;
			size = 1;
		}
		switch (buffer[2+k]) {
			case 0x32:
				MYDVB_TRACE("mydvb:capmt{k=%d}\n", k);
			 	MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			 	MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			 	MYDVB_TRACE("mydvb:~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
				dvbapi_parse_capmt(buffer + size + 3 + k, val, connfd, NULL);
				break;
			case 0x3f:
				MYDVB_TRACE("mydvb:capmt.3f{k=%d}\n", k);
				// 9F 80 3f 04 83 02 00 <demux index>
				cs_log_dump_dbg(D_DVBAPI, buffer, len, "capmt 3f:");
				// ipbox fix
			   if (cfg.dvbapi_boxtype == BOXTYPE_IPBOX || cfg.dvbapi_listenport)
			   {
					int32_t demux_index = buffer[7+k];
				   for(i = 0; i < MAX_DEMUX; i++)
				   {
					   // 0xff demux_index is a wildcard => close all related demuxers
					   if (demux_index == 0xff)
					   {
						   if (demux[i].socket_fd == connfd)
							   dvbapi_stop_descrambling(i);
					   }
					   else if (demux[i].demux_index == demux_index)
					   {
							dvbapi_stop_descrambling(i);
							break;
						}
					}
				   if (cfg.dvbapi_boxtype == BOXTYPE_IPBOX)
				   {
					   // check do we have any demux running on this fd
					   int16_t execlose = 1;
					   for(i = 0; i < MAX_DEMUX; i++)
					   {
						   if (demux[i].socket_fd == connfd)
						   {
							    execlose = 0;
							    break;
						   }
					   }
					   if (execlose)
					   {
						   int32_t ret = close(connfd);
						   if (ret < 0) { cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno)); }
					   }
					}
				}
			   else
			   {
				   if (cfg.dvbapi_pmtmode != 6)
				   {
					   int32_t ret = close(connfd);
					   if (ret < 0) { cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno)); }
					}
				}
				break;
			default:
				cs_log_dbg(D_DVBAPI,"handlesockmsg() unknown command");
				cs_log_dump(buffer, len, "unknown command:");
				break;
		}
	}
}

int32_t dvbapi_init_listenfd(void)
{
	struct sockaddr_un servaddr;
	int32_t clilen, listenfd;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	cs_strncpy(servaddr.sun_path, devices[selected_box].cam_socket_path, sizeof(servaddr.sun_path));
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((unlink(devices[selected_box].cam_socket_path) < 0) && (errno != ENOENT))
		{ return 0; }
	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		{ return 0; }
	if (bind(listenfd, (struct sockaddr *)&servaddr, clilen) < 0)
		{ return 0; }
	if (listen(listenfd, 5) < 0)
		{ return 0; }

	// change the access right on the camd.socket
	// this will allow oscam to run as root if needed
	// and still allow non root client to connect to the socket
	chmod(devices[selected_box].cam_socket_path, S_IRWXU | S_IRWXG | S_IRWXO);
	MYDVB_TRACE("camsocket:%s\n", devices[selected_box].cam_socket_path);

	return (listenfd);
}

int32_t dvbapi_net_init_listenfd(void)
{
	struct SOCKADDR servaddr;
	int32_t listenfd;

	memset(&servaddr, 0, sizeof(servaddr));
	SIN_GET_FAMILY(servaddr) = DEFAULT_AF;
	SIN_GET_ADDR(servaddr)   = ADDR_ANY;
	SIN_GET_PORT(servaddr)   = htons((uint16_t)cfg.dvbapi_listenport);

	if ((listenfd = socket(DEFAULT_AF, SOCK_STREAM, 0)) < 0)
		{ return 0; }

	int32_t opt = 0;
#ifdef IPV6SUPPORT
	// set the server socket option to listen on IPv4 and IPv6 simultaneously
	setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&opt, sizeof(opt));
#endif

	opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));
	set_so_reuseport(listenfd);

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		{ return 0; }
	if (listen(listenfd, 5) < 0)
		{ return 0; }

	return listenfd;
}

static pthread_mutex_t 	event_handler_lock;

void event_handler(int32_t UNUSED(signal))
{
	struct stat pmt_info;
	char dest[1024];
	DIR *dirp;
	struct dirent entry, *dp = NULL;
	int32_t i, pmt_fd;
	int32_t retstatus;
	uchar mbuf[2048];	// dirty fix: larger buffer needed for CA PMT mode 6 with many parallel channels to decode

//	MYDVB_TRACE("mydvb:event_handler\n");
	if (dvbApi_client != cur_client()) return;

	pthread_mutex_lock(&event_handler_lock);

	if (cfg.dvbapi_boxtype == BOXTYPE_PC || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
		pausecam = 0;
#if defined(WITH_HISILICON)
	else if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON)
		pausecam = 0;
#endif
	else {
		int32_t standby_fd = open(STANDBY_FILE, O_RDONLY);
		pausecam = (standby_fd > 0) ? 1 : 0;
		if (standby_fd > 0) {
			retstatus = close(standby_fd);
			if (retstatus < 0) cs_log("ERROR: Could not close standby fd (errno=%d %s)", errno, strerror(errno));
		}
	}

	if (cfg.dvbapi_boxtype == BOXTYPE_IPBOX || cfg.dvbapi_pmtmode == 1)
	{
		pthread_mutex_unlock(&event_handler_lock);
		return;
	}

	for (i=0;i<MAX_DEMUX;i++)
	{
		if (demux[i].pmt_file[0] != 0) {
			snprintf(dest, sizeof(dest), "%s%s", TMPDIR, demux[i].pmt_file);
			pmt_fd = open(dest, O_RDONLY);
			if (pmt_fd>0)
			{
				if (fstat(pmt_fd, &pmt_info) != 0) {
					retstatus = close(pmt_fd);
					if (retstatus < 0) cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno));
					continue;
				}

				if ((time_t)pmt_info.st_mtime != demux[i].pmt_time) {
				 	dvbapi_stop_descrambling(i);
				}

				retstatus = close(pmt_fd);
				if (retstatus < 0) cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno));
				continue;
			}
			else
			{
				cs_log("Demuxer %d Unable to open PMT file %s -> stop descrambling!", i, dest);
				dvbapi_stop_descrambling(i);
			}
		}
	}

	if (disable_pmt_files) {
		pthread_mutex_unlock(&event_handler_lock);
		return;
	}

	dirp = opendir(TMPDIR);
	if (!dirp) {
		cs_log_dbg(D_DVBAPI,"opendir failed (errno=%d %s)", errno, strerror(errno));
		pthread_mutex_unlock(&event_handler_lock);
		return;
	}

	while (!cs_readdir_r(dirp, &entry, &dp))
	{
		if (!dp) break;

		if (strlen(dp->d_name) < 7)
			{ continue; }
		if (strncmp(dp->d_name, "pmt", 3)!=0 || strncmp(dp->d_name+strlen(dp->d_name)-4, ".tmp", 4)!=0)
			{ continue; }
#ifdef WITH_STAPI
		struct s_dvbapi_priority *p;
		for (p=dvbApi_priority; p != NULL; p=p->next) { // stapi: check if there is a device connected to this pmt file!
			if (p->type!='s') continue; // stapi rule?
			if (strcmp(dp->d_name, p->pmtfile)!=0) continue; // same file?
			break; // found match!
		}
		if (p == NULL) {
			cs_log_dbg(D_DVBAPI, "No matching S: line in oscam.dvbapi for pmtfile %s -> skip!", dp->d_name);
			continue;
		}
#endif
		snprintf(dest, sizeof(dest), "%s%s", TMPDIR, dp->d_name);
		pmt_fd = open(dest, O_RDONLY);
		if (pmt_fd < 0)
			continue;

		if (fstat(pmt_fd, &pmt_info) != 0) {
			retstatus = close(pmt_fd);
			if (retstatus < 0) cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno));
			continue;
		}

		MYDVB_TRACE("mydvb:pmtfile{%s}\n", dest);
		int32_t found=0;
		for (i=0;i<MAX_DEMUX;i++)
		{
			if (strcmp(demux[i].pmt_file, dp->d_name)==0)
			{
				if ((time_t)pmt_info.st_mtime == demux[i].pmt_time)
				{
				 	found = 1;
					break;
				}
			}
		}
		if (found) {
			retstatus = close(pmt_fd);
			if (retstatus < 0) cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno));
			continue;
		}

		cs_log_dbg(D_DVBAPI,"found pmt file %s", dest);
		cs_sleepms(100);

		uint32_t len = read(pmt_fd,mbuf,sizeof(mbuf));
		retstatus = close(pmt_fd);
		if (retstatus < 0) cs_log("ERROR: Could not close PMT fd (errno=%d %s)", errno, strerror(errno));

		if (len < 1) {
			cs_log_dbg(D_DVBAPI,"pmt file %s have invalid len!", dest);
			continue;
		}

		int32_t pmt_id;

#ifdef QBOXHD
		uint32_t j1,j2;
		// QboxHD pmt.tmp is the full capmt written as a string of hex values
		// pmt.tmp must be longer than 3 bytes (6 hex chars) and even length
		if ((len<6) || ((len%2) != 0) || ((len/2)>sizeof(dest))) {
			cs_log_dbg(D_DVBAPI,"error parsing QboxHD pmt.tmp, incorrect length");
			continue;
		}

		for (j2=0,j1=0;j2<len;j2+=2,j1++)
		{
			unsigned int tmp;
			if (sscanf((char *)mbuf + j2, "%02X", &tmp) != 1)
			{
				cs_log_dbg(D_DVBAPI,"error parsing QboxHD pmt.tmp, data not valid in position %d",j2);
				pthread_mutex_unlock(&event_handler_lock);
				return;
			}
			else
			{
				memcpy(dest + j1, &tmp, 4);
			}
		}

		cs_log_dump_dbg(D_DVBAPI, (unsigned char *)dest, len/2, "QboxHD pmt.tmp:");
		pmt_id = dvbapi_parse_capmt((unsigned char *)dest+4, (len/2)-4, -1, dp->d_name);
#else
		if (len>sizeof(dest)) {
			cs_log_dbg(D_DVBAPI,"event_handler() dest buffer is to small for pmt data!");
			continue;
		}
		if (len<16) {
			cs_log_dbg(D_DVBAPI,"event_handler() received pmt is too small! (%d < 16 bytes!)", len);
			continue;
		}
		cs_log_dump_dbg(D_DVBAPI, mbuf,len,"pmt:");

		dest[0] = 0x03;
		dest[1] = mbuf[3];
		dest[2] = mbuf[4];
		uint32_t pmt_program_length = b2i(2, mbuf + 10)&0xFFF;
		i2b_buf(2, pmt_program_length + 1, (uchar *) dest + 4);
		dest[6] = 0;

		memcpy(dest + 7, mbuf + 12, len - 12 - 4);
//		MYDVB_TRACE("mydvb:sockmsg{dp=%s}\n", dp->d_name);
		pmt_id = dvbapi_parse_capmt((uchar *)dest, 7 + len - 12 - 4, -1, dp->d_name);
#endif

		if (pmt_id>=0) {
			cs_strncpy(demux[pmt_id].pmt_file, dp->d_name, sizeof(demux[pmt_id].pmt_file));
			demux[pmt_id].pmt_time = (time_t)pmt_info.st_mtime;
		}

		if (cfg.dvbapi_pmtmode == 3) {
			disable_pmt_files = 1;
			break;
		}
	}
	closedir(dirp);
	pthread_mutex_unlock(&event_handler_lock);
}

void *dvbapi_event_thread(void *cli)
{
	struct s_client *client = (struct s_client *) cli;
	pthread_setspecific(getclient, client);
	set_thread_name(__func__);
	while (1) {
		cs_sleepms(750);
		event_handler(0);
	}

	return NULL;
}

void dvbapi_process_input(int32_t demux_id, int32_t filnum, uchar *buffer, int32_t bufsize)
{
	int32_t pidx = demux[demux_id].demux_fd[filnum].pidindex;
	struct s_ecmpids *curpids = NULL;
	if (pidx != -1)
	{
		curpids = &demux[demux_id].ECMpids[pidx];
	}

	uint32_t chid 	 = 0x10000;
	int32_t  ecmlen = (b2i(2, buffer + 1)&0xFFF)+3;
	uint32_t crcval = (uint32_t)-1;
	int32_t  bAlternation;
	ECM_REQUEST *er;

	myprintf("mtdvb:DVBAPI_PROCESS_INPUT(%d:%d, %d, %02X}\n", demux_id, filnum, demux[demux_id].demux_fd[filnum].type, buffer[0]);
	if (bufsize != SCT_LEN(buffer)) {
		cs_log_dbg(D_DVBAPI, "[DVBAPI] Received an ECM with invalid length!");
		return;
	}
	if (demux[demux_id].demux_fd[filnum].type == TYPE_ECM)
	{
		if (bufsize != 0)  // len = 0 receiver encountered an internal bufferoverflow!
		{
		   if (bufsize < ecmlen) // invalid CAT length
		   {
				cs_log_dbg(D_DVBAPI, "Received data with length %03X but ECM length is %03X", bufsize, ecmlen);
			   return;
		   }
		   if (bufsize >= MAX_ECM_SIZE) {
				cs_log_dbg(D_DVBAPI, "Received data with length %03X but ECM length is %03X", bufsize, ecmlen);
			   return;
		   }

//			cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d fetched ECM data (ecmlength = %03X)", demux_id, filter_num + 1, ecmlen);
		   if (!curpids) {
				cs_log_dbg(D_DVBAPI, "curpids is 0 -> ignoring!");
		   	return;
		   }

			if (!(buffer[0] == 0x80 || buffer[0] == 0x81))
			{
				// sky(dvn) /* tableid:0x50 */
				if (caid_is_dvn(curpids->CAID)) {
				}
				else {
					cs_log_dbg(D_DVBAPI, "Received an ECM with invalid ecmtable ID %02X -> ignoring!", buffer[0]);
					return;
				}
		   }

		   // sky(n)
		   // wait for odd / even ecm change (only not for irdeto!)
#if 1
			myprintf("curpids->table:%02x,%02x!", curpids->table, buffer[0]);
			if (curpids->table == buffer[0]) {
				// sky(dvn) /* tableid:0x50 */
				uint16_t	casysid = curpids->CAID;
				if (caid_is_dvn(casysid)) {
					uint32_t	crcv;
					crcv = crc32(0L, buffer, bufsize);
					if (curpids->crcprevious == crcv) return;
					curpids->crcprevious = crcv;
				}
				else {
					if (!caid_is_irdeto(casysid)) return;
				}
			}
#else
			if (curpid->table == buffer[0] && !caid_is_irdeto(curpid->CAID))  // wait for odd / even ecm change (only not for irdeto!)
			{

				if(!(er = get_ecmtask()))
				{
					return;
				}

				er->dmuxid 	= demux_id;// sky(powervu)
				er->srvid 	= demux[demux_id].program_number;

				er->tsid 	= demux[demux_id].tsid;
				er->onid 	= demux[demux_id].onid;
				er->pmtpid 	= demux[demux_id].pmtpid;
				er->ens 		= demux[demux_id].enigma_namespace;

				er->caid  = curpid->CAID;
				er->pid   = curpid->ECM_PID;
				er->prid  = curpid->PROVID;
				er->vpid  = curpid->VPID;
				er->ecmlen = ecmlen;
				memcpy(er->ecm, buffer, er->ecmlen);
				chid = get_subid(er); // fetch chid or fake chid
				er->chid = chid;
				dvbapi_set_section_filter(demux_id, er, filnum);
				NULLFREE(er);
				return;
			}
#endif

#if defined(__DVCCRC_AVAILABLE__)
		   bool bAdded  = 1;
		   bAdded = DVBCRC_Addon(curpids, buffer, bufsize, &crcval);
		   if (IS_IRDETO(curpids->CAID)) {
			   if (!bAdded) return;
		   }
#endif

		   myprintf("ECM:%02X:%02X:%02X%02X{%08X}\n", buffer[0], buffer[4], buffer[6], buffer[7], crcval);
		   bAlternation = 1;
		   curpids->tableid = buffer[0];
			if (caid_is_irdeto(curpids->CAID))
		   {
			   // 80 70 39 53 04 05 00 88
			   // 81 70 41 41 01 06 00 13 00 06 80 38 1F 52 93 D2
			   // if (buffer[5]>20) return;
			   if (curpids->irdeto_maxindex != buffer[5]) { // 6, register max irdeto index
				   cs_log_dbg(D_DVBAPI,"Found %d IRDETO ECM CHIDs", buffer[5]+1);
				   curpids->irdeto_maxindex = buffer[5]; // numchids = 7 (0..6)
			   }
		   }
		}

		if (!(er = get_ecmtask())) return;
		er->dmuxid = demux_id;// sky(powervu)
		er->srvid  = demux[demux_id].program_number;
		er->tsid   = demux[demux_id].tsid;
		er->onid   = demux[demux_id].onid;
		er->pmtpid = demux[demux_id].pmtpid;
		er->ens    = demux[demux_id].enigma_namespace;
		er->caid   = curpids->CAID;
		er->pid    = curpids->ECM_PID;
		er->prid   = curpids->PROVID;
		er->vpid   = curpids->VPID;
		er->ecmlen = ecmlen;
		memcpy(er->ecm, buffer, er->ecmlen);

		er->ecm_crc = (uint32_t)crcval;
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
		er->chSets.muxid	 	= demux_id;
		er->chSets.degree		= demux[demux_id].cs_degree;
		er->chSets.frequency = demux[demux_id].cs_frequency;
		er->chSets.vpid	 	= demux[demux_id].cs_vidpid;
		er->chSets.srvid  	= demux[demux_id].program_number;
#endif

		chid = get_subid(er); // fetch chid or fake chid
		er->chid   = (chid) ? chid : 0x10000; // if not zero apply, otherwise use no chid value 0x10000
#if defined(MODULE_AVAMGCAMD)
		er->exprid = er->prid;
		if (IS_IRDETO(er->caid)) {
			er->exprid = (er->chid << 8) | (((er->onid>>8)&0xff) ^ ((er->onid)&0xff));
		}
#endif
		if (bufsize == 0) // only used on receiver internal bufferoverflow to get quickly fresh ecm filterdata otherwise freezing!
		{
			curpids->table = 0;
			dvbapi_set_section_filter(demux_id, er, filnum);
			NULLFREE(er);
			return;
		}

		if (caid_is_irdeto(curpids->CAID))
		{
			cs_log_dbg(D_DVBAPI,"[DVBAPI] Demuxer %d ECMTYPE %02X CAID %04X PROVID %06X ECMPID %04X IRDETO INDEX %02X MAX INDEX %02X CHID %04X CYCLE %02X",
					demux_id, er->ecm[0], er->caid, er->prid, er->pid, er->ecm[4], er->ecm[5], er->chid, curpids->irdeto_cycle);

			if (curpids->irdeto_curindex != buffer[4]) // old style wrong irdeto index
			{
				if (curpids->irdeto_curindex == 0xfe) // check if this ecmfilter just started up
				{
					curpids->irdeto_curindex = buffer[4];// on startup set the current index to the irdeto index of the ecm
				}
				// sky(a)
				else // we are already running and not interested in this ecm
				if (curpids->CHID == 0x10000)
				{
				   bAlternation = 0;
					curpids->irdeto_curindex = buffer[4];// on startup set the current index to the irdeto index of the ecm
				}
				else // we are already running and not interested in this ecm
				{
//					curpids->table = 0;
					if (curpids->table != buffer[0]) curpids->table = 0; // fix for receivers not supporting section filtering
					dvbapi_set_section_filter(demux_id, er, filnum); 	// set ecm filter to odd + even since this ecm doesnt match with current irdeto index
					NULLFREE(er);
					MYDVB_TRACE("11111 {%02X,%02X}{%4X}\n", buffer[4], curpids->irdeto_curindex, curpids->CHID);
					return;
				}
			}
			else //fix for receivers not supporting section filtering
			{
				if (curpids->table == buffer[0]){
					NULLFREE(er);
					return;
				}
			}
			cs_log_dbg(D_DVBAPI, "Demuxer %d ECMTYPE %02X CAID %04X PROVID %06X ECMPID %04X IRDETO INDEX %02X MAX INDEX %02X CHID %04X CYCLE %02X VPID %04X",
					demux_id, er->ecm[0], er->caid, er->prid, er->pid, er->ecm[4], er->ecm[5], er->chid, curpids->irdeto_cycle, er->vpid);
		}
		else
		{
			cs_log_dbg(D_DVBAPI, "Demuxer %d ECMTYPE %02X CAID %04X PROVID %06X ECMPID %04X FAKECHID %04X (unique part in ecm)",
					demux_id, er->ecm[0], er->caid, er->prid, er->pid, er->chid);
		}

		// check for matching chid (unique ecm part in case of non-irdeto cas) + added fix for seca2 monthly changing fakechid
		if ((curpids->CHID < 0x10000) && !((chid == curpids->CHID) || ((curpids->CAID >> 8 == 0x01) && (chid&0xF0FF) == (curpids->CHID&0xF0FF)) ) )
		{
			if (caid_is_irdeto(curpids->CAID))
			{

				if ((curpids->irdeto_cycle < 0xfe) && (curpids->irdeto_cycle == curpids->irdeto_curindex)) // if same: we cycled all indexes but no luck!
				{
					struct s_dvbapi_priority *forceentry = dvbapi_check_prio_match(demux_id, pidx, 'p');
					if (!forceentry || !forceentry->force) // forced pid? keep trying the forced ecmpid, no force kill ecm filter
					{
						if (curpids->checked == 2) curpids->checked = 4;
						if (curpids->checked == 1) {
							 curpids->checked  = 2;
							 curpids->CHID = 0x10000;
						}
						dvbapi_stop_filternum(demux_id, filnum); // stop this ecm filter!
						NULLFREE(er);
						return;
					}
				}

				curpids->irdeto_curindex++; // set check on next index
				if (curpids->irdeto_cycle == 0xFE) curpids->irdeto_cycle = buffer[4]; // on startup set to current irdeto index
				if (curpids->irdeto_curindex > curpids->irdeto_maxindex) curpids->irdeto_curindex = 0; // check if we reached max irdeto index, if so reset to 0
				MYDVB_TRACE("22222 {%04X,%04X, %02X}\n", chid, curpids->CHID, curpids->irdeto_curindex);

				curpids->table = 0;
				dvbapi_set_section_filter(demux_id, er, filnum); // set ecm filter to odd + even since this ecm doesnt match with current irdeto index
				NULLFREE(er);
				return;
			}
			else // all nonirdeto cas systems
			{
				struct s_dvbapi_priority *forceentry = dvbapi_check_prio_match(demux_id, pidx, 'p');
				curpids->table = 0;
				dvbapi_set_section_filter(demux_id, er, filnum); // set ecm filter to odd + even since this ecm doesnt match with current irdeto index
				if (forceentry && forceentry->force) {
					NULLFREE(er);
					return; // forced pid? keep trying the forced ecmpid!
				}
				if (curpids->checked == 2) curpids->checked = 4;
				if (curpids->checked == 1) {
					 curpids->checked  = 2;
					 curpids->CHID = 0x10000;
				}
				dvbapi_stop_filternum(demux_id, filnum); // stop this ecm filter!
				NULLFREE(er);
				return;
			}
		}

		struct s_dvbapi_priority *p;

		for (p = dvbApi_priority; p != NULL; p = p->next)
		{
			if (p->type != 'l'
				|| (p->caid   && p->caid   != curpids->CAID)
				|| (p->provid && p->provid != curpids->PROVID)
				|| (p->ecmpid && p->ecmpid != curpids->ECM_PID)
				|| (p->srvid  && p->srvid  != demux[demux_id].program_number))
				continue;

			if (p->delay == ecmlen && p->force < 6) {
				p->force++;
				NULLFREE(er);
				return;
			}
			if (p->force >= 6) p->force = 0;
		}

		if (!curpids->PROVID)
			curpids->PROVID = chk_ecm_provid(buffer, curpids->CAID);

		if (caid_is_irdeto(curpids->CAID))   // irdeto: wait for the correct index
		{
			if (buffer[4] != curpids->irdeto_curindex)
			{
				curpids->table = 0;
				dvbapi_set_section_filter(demux_id, er, filnum); // set ecm filter to odd + even since this ecm doesnt match with current irdeto index
				NULLFREE(er);
				MYDVB_TRACE("33333 {%02X,%02X}\n", buffer[4], curpids->irdeto_curindex);
			   return;
		 	}
		}
		// we have an ecm with the correct irdeto index (or fakechid)
		for (p = dvbApi_priority; p != NULL; p = p->next) // check for ignore!
		{
			if ((p->type != 'i')
				|| (p->caid   && p->caid   != curpids->CAID)
				|| (p->provid && p->provid != curpids->PROVID)
				|| (p->ecmpid && p->ecmpid != curpids->ECM_PID)
				|| (p->pidx   && p->pidx-1 != pidx)
				|| (p->srvid  && p->srvid  != demux[demux_id].program_number))
				continue;

			if (p->type == 'i' && (p->chid < 0x10000 && p->chid == chid)) // found a ignore chid match with current ecm -> ignoring this irdeto index
			{
				curpids->irdeto_curindex++;
				if (curpids->irdeto_cycle == 0xFE) curpids->irdeto_cycle = buffer[4]; // on startup set to current irdeto index
				if (curpids->irdeto_curindex > curpids->irdeto_maxindex) { // check if curindex is over the max
					curpids->irdeto_curindex = 0;
				}
				curpids->table = 0;
				if (caid_is_irdeto(curpids->CAID) && (curpids->irdeto_cycle != curpids->irdeto_curindex))   // irdeto: wait for the correct index
				{
					dvbapi_set_section_filter(demux_id, er, filnum); // set ecm filter to odd + even since this chid has to be ignored!
				}
				else // this fakechid has to be ignored, kill this filter!
				{
					if (curpids->checked == 2) { curpids->checked = 4; }
					if (curpids->checked == 1)
					{
						curpids->checked = 2;
						curpids->CHID = 0x10000;
					}
					dvbapi_stop_filternum(demux_id, filnum); // stop this ecm filter!
				}
				NULLFREE(er);
				return;
			}
		}
		// sky(a)
		if (er) {
			curpids->table = (bAlternation) ? er->ecm[0] : 0;
		}
		dvbapi_request_cw(dvbApi_client, er, demux_id, filnum, 1); // register this ecm for delayed ecm response check
		return; // end of ecm filterhandling!
	}

	if (demux[demux_id].demux_fd[filnum].type == TYPE_EMM && bufsize != 0)  // len = 0 receiver encountered an internal bufferoverflow!
	{
		if (demux[demux_id].demux_fd[filnum].pid == 0x01) // CAT
		{
			myascdump("CAT", buffer, bufsize);
			cs_log_dbg(D_DVBAPI, "receiving cat");
			dvbapi_parse_cat(demux_id, buffer, bufsize);

			dvbapi_stop_filternum(demux_id, filnum);
			return;
		}
		if (bufsize >= MAX_EMM_SIZE) {
			cs_log_dbg(D_DVBAPI, "Received an EMM invalid length{%d}!\n", bufsize);
			return;
		}
		dvbapi_process_emm(demux_id, filnum, buffer, bufsize);
	}
}

#if defined(WITH_HISILICON)
#define PMT_SERVER_SOCKET "/var/.listen.camd.socket"
#else
#define PMT_SERVER_SOCKET "/tmp/.listen.camd.socket"
#endif

static void *dvbapi_main_local(void *cli)
{
	static int32_t next_scrambled_tries = 0;
	int32_t 	i,j;
	struct s_client *client = (struct s_client *)cli;
	client->thread = pthread_self();
	pthread_setspecific(getclient, cli);

	dvbApi_client  = cli;
	MYDVB_TRACE("mydvb:dvbapi_main_local{%d,%p}\n", cfg.dvbapi_pmtmode, dvbApi_client);

	int32_t 	maxpfdsize = (MAX_DEMUX*maxfilter)+MAX_DEMUX+2;
	struct pollfd pfd2[maxpfdsize];
	struct timeb  start, end;  // start time poll, end time poll
	struct sockaddr_un saddr;
	saddr.sun_family = AF_UNIX;
	strncpy(saddr.sun_path, PMT_SERVER_SOCKET, 107);
	saddr.sun_path[107] = '\0';

	int32_t 	rc,pfdcount,g,connfd,clilen;
	int32_t 	ids[maxpfdsize], fdn[maxpfdsize], type[maxpfdsize];
	struct SOCKADDR servaddr;
	ssize_t 	len = 0;
	uchar 	mbuf[1024];

	struct s_auth *account;
	int32_t 	ok = 0, retstatus;
	for (account = cfg.account; account != NULL; account=account->next)
	{
		if ((ok = is_dvbapi_usr(account->usr)))
			{ break; }
	}
	cs_auth_client(client, ok ? account : (struct s_auth *)(-1), "dvbapi");

	memset(demux, 0, sizeof(struct demux_s) * MAX_DEMUX);
	memset(ca_fd, 0, sizeof(ca_fd));
	memset(unassoc_fd, 0, sizeof(unassoc_fd));

	dvbapi_read_priority();
	dvbapi_load_channel_cache();
	dvbapi_detect_api();

	if (selected_box == -1 || selected_api == -1)
	{
		cs_log("ERROR: Could not detect DVBAPI version.");
		return NULL;
	}

	if (cfg.dvbapi_pmtmode == 1)
		{ disable_pmt_files = 1; }

	int32_t listenfd = -1;
	if (cfg.dvbapi_boxtype != BOXTYPE_IPBOX_PMT &&
		 cfg.dvbapi_pmtmode != 2 && cfg.dvbapi_pmtmode != 5 && cfg.dvbapi_pmtmode != 6)
	{
		if (!cfg.dvbapi_listenport)
			listenfd = dvbapi_init_listenfd();
		else
			listenfd = dvbapi_net_init_listenfd();
		if (listenfd < 1)
		{
			cs_log("ERROR: Could not init socket: (errno=%d: %s)", errno, strerror(errno));
			return NULL;
		}
	}

	pthread_mutex_init(&event_handler_lock, NULL);

	for(i = 0; i < MAX_DEMUX; i++)  // init all demuxers!
	{
		demux[i].pidindex  = -1;
		demux[i].curindex  = -1;
		demux[i].cs_cwidx  = -1;
	}

	if (cfg.dvbapi_pmtmode != 4 && cfg.dvbapi_pmtmode != 5 && cfg.dvbapi_pmtmode != 6)
	{
		struct sigaction signal_action;
		signal_action.sa_handler = event_handler;
		sigemptyset(&signal_action.sa_mask);
		signal_action.sa_flags = SA_RESTART;
		sigaction(SIGRTMIN + 1, &signal_action, NULL);

		dir_fd = open(TMPDIR, O_RDONLY);
		if (dir_fd >= 0)
		{
			fcntl(dir_fd, F_SETSIG, SIGRTMIN + 1);
			fcntl(dir_fd, F_NOTIFY, DN_MODIFY | DN_CREATE | DN_DELETE | DN_MULTISHOT);
			event_handler(SIGRTMIN + 1);
		}
	}
	else
	{
		pthread_t event_thread;
		retstatus = pthread_create(&event_thread, NULL, dvbapi_event_thread, (void *)dvbApi_client);
		if (retstatus) {
			cs_log("ERROR: Can't create dvbapi event thread (errno=%d %s)", retstatus, strerror(retstatus));
			return NULL;
		}
	}

	if (listenfd != -1)
	{
		pfd2[0].fd = listenfd;
		pfd2[0].events = (POLLIN|POLLPRI);
		type[0] = 1;
	}

	#ifdef WITH_COOLAPI
		system("pzapit -rz");
	#endif
	cs_ftime(&start); // register start time
	while (1)
	{
		if (pausecam) 	// for dbox2, STAPI or PC in standby mode dont parse any ecm/emm or try to start next filter
			{ continue; }

		if (cfg.dvbapi_pmtmode == 6)
		{
			if (listenfd < 0)
			{
				cs_log("PMT6: Trying connect to enigma CA PMT listen socket...");
				/* socket init */
				if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
				{
					cs_log("socket error (errno=%d %s)", errno, strerror(errno));
					listenfd = -1;
				}
				else if (connect(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
				{
					cs_log("socket connect error (errno=%d %s)", errno, strerror(errno));
					close(listenfd);
					listenfd = -1;
				}
				else
				{
					pfd2[0].fd = listenfd;
					pfd2[0].events = (POLLIN|POLLPRI);
					type[0] = 1;
					cs_log("PMT6 CA PMT Server connected on fd %d!", listenfd);
				}
			}
 		}
		pfdcount = (listenfd > -1) ? 1 : 0;

		for (i=0;i<MAX_DEMUX;i++)
		{
			// add client fd's which are not yet associated with the demux but needs to be polled for data
			if (unassoc_fd[i]) {
				pfd2[pfdcount].fd = unassoc_fd[i];
				pfd2[pfdcount].events = (POLLIN | POLLPRI);
				type[pfdcount++] = 1;
			}

			if (demux[i].program_number == 0) continue; // only evalutate demuxers that have channels assigned

			uint32_t ecmcounter = 0, emmcounter = 0;
			for (g=0;g<maxfilter;g++)
			{
				if (demux[i].demux_fd[g].fd <= 0) continue; // deny obvious invalid fd!
// sky(!)
//				if (!cfg.dvbapi_listenport && cfg.dvbapi_boxtype != BOXTYPE_PC_NODMX && selected_api != STAPI && selected_api != COOLAPI)
				if (!cfg.dvbapi_listenport && cfg.dvbapi_boxtype != BOXTYPE_PC_NODMX && selected_api != STAPI && selected_api != COOLAPI && selected_api != HISILICONAPI)
				{
					pfd2[pfdcount].fd = demux[i].demux_fd[g].fd;
					pfd2[pfdcount].events = (POLLIN | POLLPRI);
					ids [pfdcount] 	= i;
					fdn [pfdcount] 	= g;
					type[pfdcount++] 	= 0;
				}
				if (demux[i].demux_fd[g].type == TYPE_ECM) { ecmcounter++; }  // count ecm filters to see if demuxing is possible anyway
				if (demux[i].demux_fd[g].type == TYPE_EMM) { emmcounter++; }  // count emm filters also
			}
			if (ecmcounter != demux[i].old_ecmfiltercount || emmcounter != demux[i].old_emmfiltercount) // only produce log if something changed
			{
			//	cs_log_dbg(D_DVBAPI, "Demuxer %d has %d ecmpids, %d streampids, %d ecmfilters and %d of max %d emmfilters", i, demux[i].ECMpidcount,
			//				  demux[i].STREAMpidcount, ecmcounter, emmcounter, demux[i].max_emm_filter);
				demux[i].old_ecmfiltercount = ecmcounter; // save new amount of ecmfilters
				demux[i].old_emmfiltercount = emmcounter; // save new amount of emmfilters
			}

			// delayed emm start for non irdeto caids, start emm cat if not already done for this demuxer!

			struct timeb now;
			cs_ftime(&now);

			// sky(!)
			//	myprintf("mydvb:CAT{%d}{%d,%d,%d}\n", cfg.dvbapi_au, demux[i].emm_filter, demux[i].EMMpidcount, emmcounter);
			if (cfg.dvbapi_au > 0 && demux[i].emm_filter == -1 && demux[i].EMMpidcount == 0 && emmcounter == 0)
			{
				int64_t gone = comp_timeb(&now, &demux[i].emmstart);
				//	myprintf("mydvb:gone{%lld}\n", gone);
				if (gone > 10*1000LL) { // sky(30->10)
					cs_ftime(&demux[i].emmstart); // trick to let emm fetching start after 30 seconds to speed up zapping
				   dvbapi_stop_filter(demux[i].demux_index, TYPE_EMM);
					myprintf("mydvb:start CAT\n");
				   dvbapi_start_filter(i,
							   demux[i].pidindex,
							   CAT_PID,
							   0x001,
							   0x01,
							   0x01,
							   0xFF,
							   0,
							   TYPE_EMM); 	// CAT
				//	continue; // proceed with next demuxer
				}
			}

			// early start for irdeto since they need emm before ecm (pmt emmstart = 1 if detected caid 0x06)
			int32_t emmstarted = demux[i].emm_filter;
			if (cfg.dvbapi_au && demux[i].EMMpidcount > 0) 	// check every time since share readers might give us new filters due to hexserial change
			{
				if (!emmcounter && emmstarted == -1)
				{
					demux[i].emmstart = now;
					dvbapi_start_emm_filter(i);	// start emmfiltering if emmpids are found
				}
				else
				{
					int64_t gone = comp_timeb(&now, &demux[i].emmstart);
					if (gone > 10*1000) // sky(30->10)
					{
						demux[i].emmstart = now;
						dvbapi_start_emm_filter(i); 	// start emmfiltering delayed if filters already were running
						rotate_emmfilter(i); // rotate active emmfilters
					}
				}
			//	if (emmstarted != demux[i].emm_filter && !emmcounter) { continue; }  // proceed with next demuxer if no emms where running before
			}
			next_scrambled_tries++;
			if (!(next_scrambled_tries%4) && ecmcounter == 0 && demux[i].ECMpidcount > 0) // Restart decoding all caids we have ecmpids but no ecm filters!
			{
				int32_t started = 0;

#if defined(WITH_HISILICON)
// test.futures
//				if (chk_av_descrambling(client)) continue;
#endif

				for (g=0; g<demux[i].ECMpidcount; g++) // avoid race: not all pids are asked and checked out yet!
				{
					if (demux[i].ECMpids[g].checked == 0 && demux[i].ECMpids[g].status >= 0) // check if prio run is done
					{
						MYDVB_TRACE("mydvb:eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee TRY{%d}\n", next_scrambled_tries);
						dvbapi_try_next_caid(i, 0, 0); // not done, so start next prio pid
						started = 1;
						break;
					}
				}
				if (started) continue; // if started a filter proceed with next demuxer

				if (g == demux[i].ECMpidcount) // all usable pids (with prio) are tried, lets start over again without prio!
				{
					for (g=0; g<demux[i].ECMpidcount; g++) // avoid race: not all pids are asked and checked out yet!
					{
						if (demux[i].ECMpids[g].checked == 2 && demux[i].ECMpids[g].status >= 0) // check if noprio run is done
						{
							demux[i].ECMpids[g].irdeto_curindex	= 0xfe;
							demux[i].ECMpids[g].irdeto_maxindex	= 0;
							demux[i].ECMpids[g].irdeto_cycle		= 0xfe;
							demux[i].ECMpids[g].tries = 0xfe;
							demux[i].ECMpids[g].table = 0;
							demux[i].ECMpids[g].CHID  = 0x10000; // remove chid prio
							MYDVB_TRACE("mydvb:cccccccccccccccccccccccccccccccccccccccc TRY{%d}\n", next_scrambled_tries);
							dvbapi_try_next_caid(i, 2, 0); // not done, so start next no prio pid
							started = 1;
							break;
						}
					}
				}
				if (started) continue; // if started a filter proceed with next demuxer

				if (g == demux[i].ECMpidcount) // all usable pids are tried, lets start over again!
				{
					if (demux[i].decodingtries == -1) // first redecoding attempt?
					{
						cs_ftime(&demux[i].decstart);
					   demux[i].scrambled_counter = (demux[i].scrambled_counter+1)%10;
					   if (demux[i].scrambled_counter==0) {
						   for (g=0; g<demux[i].ECMpidcount; g++) // reinit some used things from second run (without prio)
						   {
							   demux[i].ECMpids[g].checked = 0;
							   demux[i].ECMpids[g].irdeto_curindex = 0xfe;
							   demux[i].ECMpids[g].irdeto_maxindex = 0;
							   demux[i].ECMpids[g].irdeto_cycle		= 0xfe;
							   demux[i].ECMpids[g].table = 0;
							   demux[i].decodingtries = 0;
							   dvbapi_edit_channel_cache(i, g, 0); // remove this pid from channelcache since we had no founds on any ecmpid!
						   }
						}
					}
					uint8_t number_of_enabled_pids = 0;
					demux[i].decodingtries++;
					dvbapi_resort_ecmpids(i);

					for(g = 0; g < demux[i].ECMpidcount; g++)  // count number of enabled pids!
					{
						if (demux[i].ECMpids[g].status >= 0) number_of_enabled_pids++;
					}
					if (!number_of_enabled_pids)
					{
						if (demux[i].decodingtries == 10)
						{
							demux[i].decodingtries = 0;
							cs_log("Demuxer %d no enabled matching ecmpids -> decoding is waiting for matching readers!",i);
						}
					}
					else
					{
						cs_ftime(&demux[i].decend);
						demux[i].decodingtries = -1; // reset to first run again!
						int64_t gone = comp_timeb(&demux[i].decend, &demux[i].decstart);
						cs_log("Demuxer %d restarting decodingrequests after %"PRId64" ms with %d enabled and %d disabled ecmpids!", i, gone, number_of_enabled_pids,
							   (demux[i].ECMpidcount-number_of_enabled_pids));
						MYDVB_TRACE("mydvb:ssssssssssssssssssssssssssssssssssssssss TRY{%d}\n", next_scrambled_tries);
						dvbapi_try_next_caid(i, 0, 1);
					}
				}
			}

			if (demux[i].socket_fd > 0 && cfg.dvbapi_pmtmode != 6)
			{
				rc = 0;
				for(j = 0; j < pfdcount; j++)
				{
					if (pfd2[j].fd == demux[i].socket_fd)
					{
						rc = 1;
						break;
					}
				}
				if (rc==1) continue;

				pfd2[pfdcount].fd = demux[i].socket_fd;
				pfd2[pfdcount].events = (POLLIN|POLLPRI);
				ids [pfdcount]   = i;
				type[pfdcount++] = 1;
			}
		}

		rc = 0;
		while (!(listenfd == -1 && cfg.dvbapi_pmtmode == 6))
		{
			rc = poll(pfd2, pfdcount, 300);
			if (rc < 0) {
				if (errno == EINTR || errno == EAGAIN) continue; // try again in case of interrupt
			}
			break;
		}

		if (rc > 0)
		{
			cs_ftime(&end); 	// register end time
			int64_t timeout = comp_timeb(&end, &start);
			if (timeout < 0) {
				cs_log("*** WARNING: BAD TIME AFFECTING WHOLE OSCAM ECM HANDLING ***");
				// sky(a)
				if (cs_timefaults) {
					cs_log("*** OSCAM LOGIN_TIME CHANGE ***");
					cs_timefaults = 0;
					first_client->login = time((time_t *)0);
				}
			}
			cs_log_dbg(D_TRACE, "New events occurred on %d of %d handlers after %"PRId64" ms inactivity", rc, pfdcount, timeout);
			cs_ftime(&start); // register new start time for next poll
		}

		for (i=0; i<pfdcount && rc>0; i++)
		{
			if (pfd2[i].revents == 0) { continue; }  // skip sockets with no changes
			rc--; // event handled!
			cs_log_dbg(D_TRACE, "Now handling fd %d that reported event %x(%d)", pfd2[i].fd, pfd2[i].revents, type[i]);

			if (pfd2[i].revents & (POLLHUP  | POLLNVAL | POLLERR))
			{
				if (type[i]==1)
				{
					for (j=0;j<MAX_DEMUX;j++)
					{
						if (demux[j].socket_fd==pfd2[i].fd) { // if listenfd closes stop all assigned decoding!
							mycs_debug(D_DVBAPI, "stop pfd(%d.%d, %X)(%d) closed !!!\n", pfd2[i].fd, listenfd, pfd2[i].revents, pfdcount);
							dvbapi_stop_descrambling(j);
						}
					}
					int32_t ret = close(pfd2[i].fd);
					if (ret < 0 && errno != 9) { cs_log("ERROR: Could not close demuxer socket fd (errno=%d %s)", errno, strerror(errno)); }
					if (pfd2[i].fd==listenfd && cfg.dvbapi_pmtmode==6)
					{
						listenfd = -1;
					}
				}
				else // type = 0
				{
					int32_t demux_index = ids[i];
					int32_t n = fdn[i];
					dvbapi_stop_filternum(demux_index, n); // stop filter since its giving errors and wont return anything good.
				}
				// sky()
				if (pfd2[i].revents & (POLLNVAL)) {
					usleep(100 * 1000);
				}
				continue; // continue with other events
			}

			if (pfd2[i].revents & (POLLIN | POLLPRI))
			{
				if (type[i]==1 && pmt_handling == 0)
				{
					pmt_handling = 1; // pmthandling in progress!
					pmt_stopmarking = 0; // to stop_descrambling marking in PMT 6 mode

					connfd = -1;    // initially no socket to read from
					int add_to_poll = 0; // we may need to additionally poll this socket when no PMT data comes in

					if (pfd2[i].fd == listenfd)
					{
						if (cfg.dvbapi_pmtmode == 6) {
							connfd = listenfd;
							disable_pmt_files = 1;
						}
						else {
							clilen = sizeof(servaddr);
							connfd = accept(listenfd, (struct sockaddr *)&servaddr, (socklen_t *)&clilen);
							cs_log_dbg(D_DVBAPI, "new socket connection fd: %d(%d)", connfd, listenfd);
							if (cfg.dvbapi_listenport)
							{
								//update webif data
								client->ip   = SIN_GET_ADDR(servaddr);
								client->port = ntohs(SIN_GET_PORT(servaddr));
							}
							add_to_poll = 1;

							if (cfg.dvbapi_pmtmode == 3 || cfg.dvbapi_pmtmode == 0) { disable_pmt_files = 1; }

							if (connfd <= 0) {
								cs_log_dbg(D_DVBAPI,"accept() returns error on fd event %d (errno=%d %s)", pfd2[i].revents, errno, strerror(errno));
							}
						}
					}
					else
					{
						cs_log_dbg(D_DVBAPI, "PMT Update on socket %d.", pfd2[i].fd);
						connfd = pfd2[i].fd;
					}

					//reading and completing data from socket
					if (connfd > 0) {
						uint32_t pmtlen = 0, chunks_processed = 0;

						int tries = 100;
						do {
							len = recv(connfd, mbuf + pmtlen, sizeof(mbuf) - pmtlen, MSG_DONTWAIT);
							if (len > 0) pmtlen += len;

							if ((cfg.dvbapi_listenport || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX) &&
								 (len == 0 || (len == -1 && (errno != EINTR && errno != EAGAIN))))
							{
								//client disconnects, stop all assigned decoding
								cs_log_dbg(D_DVBAPI, "Socket %d reported connection close", connfd);
								int active_conn = 0; //other active connections counter
								for (j = 0; j < MAX_DEMUX; j++)
								{
									if (demux[j].socket_fd == connfd)
										dvbapi_stop_descrambling(j);
									else if (demux[j].socket_fd) active_conn++;
									// remove from unassoc_fd when necessary
									if (unassoc_fd[j] == connfd) unassoc_fd[j] = 0;
								}
								close(connfd);
								connfd = -1;
								add_to_poll = 0;
								if (!active_conn) //last connection closed
								{
									client_proto_version = 0;
									if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON) {}
									else if (client_name)
									{
										free(client_name);
										client_name = NULL;
									}
								   if (cfg.dvbapi_listenport)
								   {
									   //update webif data
									   client->ip = get_null_ip();
									   client->port = 0;
								   }
								}
								break;
							}
							if (pmtlen >= 8) //if we received less then 8 bytes, than it's not complete for sure
							{
								// check and try to process complete PMT objects and filter data by chunks to avoid PMT buffer overflows
								uint32_t opcode_ptr;
								memcpy( &opcode_ptr, &mbuf[0], 4);                     //used only to silent compiler warning about dereferencing type-punned pointer
								uint32_t opcode = ntohl(opcode_ptr);                  //get the client opcode (4 bytes)
								uint32_t chunksize = 0;                               //size of complete chunk in the buffer (an opcode with the data)
								uint32_t data_len  = 0;                               //variable for internal data length (eg. for the filter data size, PMT len)

								//detect the opcode, its size (chunksize) and its internal data size (data_len)
								if ((opcode & 0xFFFFF000) == DVBAPI_AOT_CA)
								{
									// parse packet size (ASN.1)
									uint32_t size = 0;
									if (mbuf[3] & 0x80)
									{
										data_len = 0;
										size = mbuf[3] & 0x7F;
										if (pmtlen > 4 + size)
										{
											uint32_t k;
											for (k = 0; k < size; k++)
												data_len = (data_len << 8) | mbuf[3 + 1 + k];
											size++;
										}
									}
									else
									{
										data_len = mbuf[3] & 0x7F;
										size = 1;
									}
									chunksize = 3 + size + data_len;
								}
								else switch (opcode)
								{
									case DVBAPI_FILTER_DATA:
									{
										data_len  = b2i(2, mbuf + 7) & 0x0FFF;
										chunksize = 6 + 3 + data_len;
										break;
									}
									case DVBAPI_CLIENT_INFO:
									{
										data_len  = mbuf[6];
										chunksize = 6 + 1 + data_len;
										break;
									}
									default:
										cs_log("Unknown socket command received: 0x%08X", opcode);
								}

								//processing the complete data according to type
								if (chunksize < sizeof(mbuf) && chunksize <= pmtlen) // only handle if we fetched a complete chunksize!
								{
									chunks_processed++;
									if ((opcode & 0xFFFFF000) == DVBAPI_AOT_CA)
									{
										cs_log_dump_dbg(D_DVBAPI, mbuf, chunksize, "Parsing PMT object %d:", chunks_processed);
										mycs_debug(D_DVBAPI,"tvh:handlesockmsg{%x}{%d}{size=%d}\n", opcode, connfd, pmtlen);
										dvbapi_handlesockmsg(mbuf, chunksize, connfd);
										add_to_poll = 0;
										if (cfg.dvbapi_listenport && opcode == DVBAPI_AOT_CA_STOP) add_to_poll = 1;
									}
									else switch (opcode)
									{
										case DVBAPI_FILTER_DATA:
										{
											if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON) break;
											int32_t demux_index = mbuf[4];
											int32_t filter_num  = mbuf[5];
											dvbapi_process_input(demux_index, filter_num, mbuf + 6, data_len + 3);
											break;
										}
										case DVBAPI_CLIENT_INFO:
										{
											if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON) break;
											uint16_t client_proto_ptr;
											memcpy(&client_proto_ptr, &mbuf[4], 2);
											uint16_t client_proto = ntohs(client_proto_ptr);
											if (client_name) free(client_name);
											if (cs_malloc(&client_name, data_len + 1))
											{
												memcpy(client_name, &mbuf[7], data_len);
												client_name[data_len] = 0;
												cs_log("Client connected: '%s' (protocol version = %d)", client_name, client_proto);
											}
											client_proto_version = client_proto; //setting the global var according to the client

											// as a response we are sending our info to the client:
											dvbapi_net_send(DVBAPI_SERVER_INFO, connfd, -1, -1, NULL, NULL, NULL);
											break;
										}
									}

									if (pmtlen == chunksize) // if we fetched and handled the exact chunksize reset buffer counter!
										pmtlen = 0;

									// if we read more data then processed, move it to beginning
									if (pmtlen > chunksize)
									{
										memmove(mbuf, mbuf + chunksize, pmtlen - chunksize);
										pmtlen -= chunksize;
									}
									continue;
								}
							}
							if (len <= 0) {
								if (pmtlen > 0 || chunks_processed > 0) //all data read
									break;
								else { // wait for data become available and try again

									// remove from unassoc_fd if the socket fd is invalid
									if (errno == EBADF)
										for (j = 0; j < MAX_DEMUX; j++)
											if (unassoc_fd[j] == connfd) unassoc_fd[j] = 0;
									cs_sleepms(20);
									continue;
								}
							}
						} while (pmtlen < sizeof(mbuf) && tries--);

						// if the connection is new and we read no PMT data, then add it to the poll,
						// otherwise this socket will not be checked with poll when data arives
						// because fd it is not yet assigned with the demux
						if (add_to_poll && !pmtlen && !chunks_processed) {
							for (j = 0; j < MAX_DEMUX; j++) {
								if (!unassoc_fd[j]) {
									unassoc_fd[j] = connfd;
									break;
								}
							}
						}

						if (pmtlen > 0) {
							if (pmtlen < 3) {
								cs_log_dbg(D_DVBAPI, "CA PMT server message too short!");
							}
							else
							{
								if (pmtlen >= sizeof(mbuf)) {
									cs_log("***** WARNING: PMT BUFFER OVERFLOW, PLEASE REPORT! ****** ");
								}
								cs_log_dump_dbg(D_DVBAPI, mbuf, pmtlen, "New PMT info from socket (total size: %d)", pmtlen);
								mycs_debug(D_DVBAPI,"tvh:handlesockmsg{%d}{size=%d}\n", connfd, pmtlen);
								dvbapi_handlesockmsg(mbuf, pmtlen, connfd);
							}
						}
					}
					pmt_handling = 0; // pmthandling done!
					continue; // continue with other events!
				}
				else // type==0
				{
					int32_t demux_index = ids[i];
					int32_t n = fdn[i];

					if ((int)demux[demux_index].demux_fd[n].fd != pfd2[i].fd) { continue; } // filter already killed, no need to process this data!

					len = dvbapi_read_device(pfd2[i].fd, mbuf, sizeof(mbuf));
					if (len < 0) // serious filterdata read error
					{
					   dvbapi_stop_filternum(demux_index, n); // stop filter since its giving errors and wont return anything good.
					   maxfilter--; // lower maxfilters to avoid this with new filter setups!
					   continue;
					}
					if (!len) // receiver internal filterbuffer overflow
					{
						memset(mbuf, 0, sizeof(mbuf));
					}

  					myprintf("tvh::process_input len=%d\n", (int)len);
					dvbapi_process_input(demux_index, n, mbuf, (int32_t)len);
				}
				continue; // continue with other events!
			}
		}
	}
	return NULL;
}
// sky(powervu)
void dvbapi_write_cw(int32_t demux_id, uchar *cw, CWEXTENTION *cwEx, int32_t pidx, int cwdesalgo)
{
	ca_descr_t ca_descr;
	ca_exdescr_t ca_exdescr; // sky(powervu)
	int32_t n;
	int8_t  cwEmpty = 0;
	uint8_t nullcw[8];

	memset(nullcw, 0, 8);
	memset(&ca_descr,0,sizeof(ca_descr));
	memset(&ca_exdescr,0,sizeof(ca_exdescr_t));

	mycs_debug(D_DVBAPI,"mydvb:dvbapi_write_cw{%d,%04X,%d, %04X}{%02X%02X...%02X%02X}\n",
			demux_id,
			demux[demux_id].ca_mask,
			demux[demux_id].adapter_index,
			pidx,
			cw[0], cw[7], cw[8], cw[15]);
	if (memcmp(demux[demux_id].lastcw[0],nullcw,8)==0
			&& memcmp(demux[demux_id].lastcw[1], nullcw, 8) == 0)
		{ cwEmpty = 1; } // to make sure that both cws get written on constantcw


	for (n=0;n<2;n++)
	{
		char lastcw[9*3];
		char newcw [9*3];
		cs_hexdump(0, demux[demux_id].lastcw[n], 8, lastcw, sizeof(lastcw));
		cs_hexdump(0, cw+(n*8), 8, newcw, sizeof(newcw));

		if (((memcmp(cw+(n*8), demux[demux_id].lastcw[n],8) != 0) || cwEmpty)
			&& memcmp(cw+(n*8), nullcw, 8) != 0) // check if already delivered and new cw part is valid!
		{
			int32_t idx = dvbapi_ca_setpid(demux_id, pidx); // prepare ca
			mycs_debug(D_DVBAPI,"mydvb:dvbapi_write_cw{idx=%d}\n", idx);
			if (idx == -1) return; // return on no index!

#ifdef WITH_COOLAPI
			ca_descr.index  = idx;
			ca_descr.parity = n;
			memcpy(demux[demux_id].lastcw[n],cw+(n*8),8);
			memcpy(ca_descr.cw,cw+(n*8),8);
			cs_log_dbg(D_DVBAPI, "Demuxer %d write cw%d index: %d (ca_mask %d)", demux_id, n, ca_descr.index, demux[demux_id].ca_mask);
			coolapi_write_cw(demux[demux_id].ca_mask, demux[demux_id].STREAMpids, demux[demux_id].STREAMpidcount, &ca_descr);
#else
			int32_t i,j, write_cw = 0;
			for (i=0;i<MAX_DEMUX;i++)
			{
				if (!(demux[demux_id].ca_mask & (1 << i))) continue; // ca not in use by this demuxer!
				// sky(constant.cw)
				if (demux[demux_id].constcw_fbiss)
				{
					cs_log_dbg(D_DVBAPI,"Demuxer %d ca%d is ftacas", demux_id, i);
					write_cw = 1;
				}
				else if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON) write_cw = 1; // always
				else {
					// sky(future test...)
					for(j = 0; j < demux[demux_id].STREAMpidcount; j++)
					{
						if (!demux[demux_id].ECMpids[pidx].streams || ((demux[demux_id].ECMpids[pidx].streams & (1 << j)) == (uint) (1 << j)))
						{
							int32_t usedidx = is_ca_used(i, demux[demux_id].STREAMpids[j]);
							if (idx != usedidx)
							{
								cs_log_dbg(D_DVBAPI,"Demuxer %d ca%d is using index %d for streampid %04X -> skip!", demux_id, i, usedidx, demux[demux_id].STREAMpids[j]);
								continue; // if not used for descrambling -> skip!
							}
							else
							{
								cs_log_dbg(D_DVBAPI,"Demuxer %d ca%d is using index %d for streampid %04X -> write!", demux_id, i, usedidx, demux[demux_id].STREAMpids[j]);
								write_cw = 1;
							}
						}
					}
				}
				if (!write_cw) { continue; } // no need to write the cw since this ca isnt using it!
				ca_descr.index  = idx & 0x7f;
				ca_descr.parity = n;
				memcpy(demux[demux_id].lastcw[n], cw + (n * 8), 8);
				memcpy(ca_descr.cw, cw + (n * 8), 8);
				cs_log_dbg(D_DVBAPI, "Demuxer %d writing %4s part (%s) of controlword, replacing expired (%s)", demux_id, (n == 1 ? "even" : "odd"), newcw, lastcw);
				cs_log_dbg(D_DVBAPI, "Demuxer %d write cw%d index: %d (ca%d)", demux_id, n, ca_descr.index, i);

				if (cfg.dvbapi_boxtype == BOXTYPE_PC || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
					dvbapi_net_send(DVBAPI_CA_SET_DESCR, demux[demux_id].socket_fd, demux_id, -1 /*unused*/, (unsigned char *) &ca_descr, NULL, NULL);
				else
				{
#if defined(WITH_HISILICON)
					if (cfg.dvbapi_boxtype == BOXTYPE_HISILICON)
					{
						// sky(powervu)
	          		int32_t request = CA_SET_DESCR;
						ca_descr.index |= (demux[demux_id].program_number << 16);
						ca_descr.index |= (demux[demux_id].adapter_index  << 12);
						if (cwdesalgo) ca_descr.index |= 0x100;

					   if (ca_fd[i]<= 0)
					   {
						   ca_fd[i] = dvbapi_open_netdevice(1, i, demux[demux_id].adapter_index);
						   if (ca_fd[i] <= 0) { continue; }
					   }

					   if (cwEx && cwEx->type == CW_MULTIPLE)
					   {
					   	request = CA_SET_EXDESCR;
					   	ca_exdescr.parity = ca_descr.parity;
					   	ca_exdescr.index  = ca_descr.index;
					   	ca_exdescr.type	= cwEx->type;
					   	ca_exdescr.algo	= cwEx->algo;
					   	ca_exdescr.cipher	= cwEx->cipher;
							memcpy(ca_exdescr.cw, cw + (n * 8), 8);
							memcpy(ca_exdescr.a0, cwEx->audio[0] + (n * 8), 8);
							memcpy(ca_exdescr.a1, cwEx->audio[1] + (n * 8), 8);
							memcpy(ca_exdescr.a2, cwEx->audio[2] + (n * 8), 8);
							memcpy(ca_exdescr.a3, cwEx->audio[3] + (n * 8), 8);
							memcpy(ca_exdescr.data, cwEx->data + (n * 8), 8);
							memcpy(ca_exdescr.vbi,  cwEx->vbi  + (n * 8), 8);
	                  unsigned char packet[sizeof(request) + sizeof(ca_exdescr)];
	                  memcpy(&packet, &request, sizeof(request));
	                  memcpy(&packet[sizeof(request)], &ca_exdescr, sizeof(ca_exdescr));
	                  // sending data(MSG_NOSIGNAL)
	                  send(ca_fd[i], &packet, sizeof(packet), 0);
	                  mycs_trace(D_ADB, "mydvb:tvheadend sending cwex%d data{%x,%d}", n, ca_exdescr.index, ca_exdescr.parity);
					   }
					   else
					   {
					   	request = CA_SET_DESCR;
	                  unsigned char packet[sizeof(request) + sizeof(ca_descr)];
	                  memcpy(&packet, &request, sizeof(request));
	                  memcpy(&packet[sizeof(request)], &ca_descr, sizeof(ca_descr));
	                  // sending data(MSG_NOSIGNAL)
	                  send(ca_fd[i], &packet, sizeof(packet), 0);
	                  mycs_trace(D_ADB, "mydvb:tvheadend sending cw%d data{%x,%d}", n, ca_descr.index, ca_descr.parity);
                  }
					}
					else
#endif
					{
					   if (ca_fd[i]<= 0)
					   {
							ca_fd[i] = dvbapi_open_device(1, i, demux[demux_id].adapter_index);
						   if (ca_fd[i] <= 0) { continue; }
					   }
					 	if (dvbapi_ioctl(ca_fd[i], CA_SET_DESCR, &ca_descr) < 0) {
						 	cs_log("ERROR: ioctl(CA_SET_DESCR): %s", strerror(errno));
						}
					}
				}
			}
#endif	// #if defined(WITH_COOLAPI)
		}
	}
}

void delayer(ECM_REQUEST *er)
{
	if (cfg.dvbapi_delayer <= 0) { return; }

	struct timeb tpe;
	cs_ftime(&tpe);
	int64_t gone = comp_timeb(&tpe, &er->tps);
	if ( gone < cfg.dvbapi_delayer)
	{
		cs_log_dbg(D_DVBAPI, "delayer: gone=%"PRId64" ms, cfg=%d ms -> delay=%"PRId64" ms", gone, cfg.dvbapi_delayer, cfg.dvbapi_delayer - gone);
		cs_sleepms(cfg.dvbapi_delayer - gone);
	}
}

void dvbapi_send_dcw(struct s_client *client, ECM_REQUEST *er)
{
	int32_t i, j, handled = 0;

	MYDVB_TRACE("mydvb:dvbapi_send_dcw{%d}{%04X:%06X,%d}{%04x}.%s(%d)(%d)\n",
			 er->rc,
			 er->caid, er->prid, er->srvid, er->pid,
			(er->selected_reader) ? er->selected_reader->label : "unk",
			 er->ecm_bypass,
			 er->cwdesalgo);


#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	if (er->ecm_useless) {
		mycs_trace(D_ADB, "mydvb:??? skip ecm_useless");
		return;
	}
#endif

	for (i=0;i<MAX_DEMUX;i++)
	{
		uint32_t nocw_write = 0; // 0 = write cw, 1 = dont write cw to hardware demuxer
		if (demux[i].program_number == 0) continue; // ignore empty demuxers
		if (demux[i].program_number != er->srvid) continue; // skip ecm response for other srvid
		demux[i].rdr = er->selected_reader;

		for (j=0; j<demux[i].ECMpidcount; j++) // check for matching ecmpid
		{
			if ((demux[i].ECMpids[j].CAID == er->caid || demux[i].ECMpids[j].CAID == er->ocaid) &&
				 (demux[i].ECMpids[j].ECM_PID == er->pid) &&
//				 (demux[i].ECMpids[j].VPID    == er->vpid) &&
				 (demux[i].ECMpids[j].PROVID  == er->prid))
				break;
		}
		MYDVB_TRACE("mydvb:    ecmpid{%d}{%d,%d}{%d,%d}(%s)\n",
						i,
						demux[i].pidindex,
						demux[i].curindex,
						j,
						demux[i].ECMpidcount,
						demux[i].rdr ? demux[i].rdr->label : "UNK");
		if (!demux[i].rdr)
		{
			MYDVB_TRACE("mydvb:    ecmpid.unk readers\n");
			continue; // ecm response srvid ok but no matching ecmpid, perhaps this for other demuxer
		}
		if (j == demux[i].ECMpidcount)
		{
			MYDVB_TRACE("mydvb:    ecmpid.non matching\n");
			continue; // ecm response srvid ok but no matching ecmpid, perhaps this for other demuxer
		}

		cs_log_dbg(D_DVBAPI,"Demuxer %d %scontrolword received for PID %d CAID %04X PROVID %06X ECMPID %04X CHID %04X", i,
				(er->rc >= E_NOTFOUND ? "no ":""), j,
				 er->caid, er->prid, er->pid, er->chid);

// sky(?)
//		if (er->rc < E_NOTFOUND) // check for delayed response on already expired ecmrequest
		{
			uint32_t status = dvbapi_check_ecm_delayed_delivery(i, er);

			uint32_t comparecw0 = 0, comparecw1 = 0;
			char 		ecmd5[17*3];
			cs_hexdump(0, er->ecmd5, 16, ecmd5, sizeof(ecmd5));

			MYDVB_TRACE("mydvb:    ecm_delayed_delivery{%d}\n", status);
			if (status == 1 && er->rc) // wrong ecmhash
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d not interested in response ecmhash %s (requested different one)", i, ecmd5);
				continue;
			}
			if (status == 2) // no filter
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d not interested in response ecmhash %s (filter already killed)", i, ecmd5);
				continue;
			}
			if (status == 5) // empty cw
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d not interested in response ecmhash %s (delivered cw is empty!)", i, ecmd5);
				nocw_write = 1;
				if (er->rc < E_NOTFOUND) {
					// sky(powervu)
					if (caid_is_powervu(er->caid)) continue;
					er->rc = E_NOTFOUND;
				}
			}

			if ((status == 0 || status == 3 || status == 4) && er->rc < E_NOTFOUND)   // 0=matching ecm hash, 2=no filter, 3=table reset, 4=cache-ex response
			{
				if (memcmp(er->cw, demux[i].lastcw[0], 8) == 0 && memcmp(er->cw + 8, demux[i].lastcw[1], 8) == 0)    // check for matching controlword
				{
					comparecw0 = 1;
				}
				else if (memcmp(er->cw, demux[i].lastcw[1], 8) == 0 && memcmp(er->cw + 8, demux[i].lastcw[0], 8) == 0)    // check for matching controlword
				{
					comparecw1 = 1;
				}
				if (comparecw0 == 1 || comparecw1 == 1)
				{
					cs_log_dbg(D_DVBAPI,"Demuxer %d duplicate controlword ecm response hash %s (duplicate controlword!)", i, ecmd5);
					mycs_trace(D_ADB, "mydvb:    cw.same!!{%02x...%02x, %02x...%02x}", er->cw[0],er->cw[7], er->cw[8],er->cw[15]);
					nocw_write = 1;
				}
			}

			if (status == 3) // table reset
			{
				cs_log_dbg(D_DVBAPI, "Demuxer %d luckyshot new controlword ecm response hash %s (ecm table reset)", i, ecmd5);
			}

			if (status == 4) // no check on cache-ex responses!
			{
				cs_log_dbg(D_DVBAPI,"Demuxer %d new controlword from cache-ex reader (no ecmhash check possible)", i);
			}
		}

		handled = 1; // mark this ecm response as handled
		if (er->rc < E_NOTFOUND && cfg.dvbapi_requestmode == 0 && (demux[i].pidindex == -1) && er->caid != 0)
		{
			demux[i].ECMpids[j].tries = 0xfe; // reset timeout retry flag
			demux[i].ECMpids[j].irdeto_cycle = 0xfe; // reset irdetocycle
			demux[i].pidindex = j; // set current index as *the* pid to descramble
			demux[i].ECMpids[j].checked = 4;
			cs_log_dbg(D_DVBAPI, "Demuxer %d descrambling PID %d CAID %04X PROVID %06X ECMPID %04X CHID %02X VPID %04X",
						  i, demux[i].pidindex, er->caid, er->prid, er->pid, er->chid, er->vpid);
		}

		if (er->rc < E_NOTFOUND && cfg.dvbapi_requestmode == 1 && er->caid != 0) // FOUND
		{
			pthread_mutex_lock(&demux[i].answerlock); // only process one ecm answer
			if (demux[i].ECMpids[j].checked != 4)
			{

				int32_t t,o,ecmcounter = 0;
				int32_t oldpidindex = demux[i].pidindex;
				demux[i].pidindex = j; // set current ecmpid as the new pid to descramble
				if (oldpidindex != -1) demux[i].ECMpids[j].index = demux[i].ECMpids[oldpidindex].index; // swap index with lower status pid that was descrambling
				for (t=0; t<demux[i].ECMpidcount; t++) // check this pid with controlword FOUND for higher status:
				{
					if (t!=j && demux[i].ECMpids[j].status >= demux[i].ECMpids[t].status)
					{
						for (o = 0; o < maxfilter; o++)  // check if ecmfilter is in use & stop all ecmfilters of lower status pids
						{
							if (demux[i].demux_fd[o].fd > 0 && demux[i].demux_fd[o].type == TYPE_ECM && (demux[i].demux_fd[o].pidindex == t))
							{
								dvbapi_stop_filternum(i, o); 	// ecmfilter belongs to lower status pid -> kill!
							}
						}
						dvbapi_edit_channel_cache(i, t, 0); // remove lowerstatus pid from channelcache
						demux[i].ECMpids[t].checked = 4; // mark index t as low status
					}
				}


				for(o = 0; o < maxfilter; o++) if (demux[i].demux_fd[o].type == TYPE_ECM) { ecmcounter++; }   // count all ecmfilters

				demux[i].ECMpids[j].tries = 0xfe; // reset timeout retry flag
				demux[i].ECMpids[j].irdeto_cycle = 0xfe; // reset irdetocycle
				demux[i].pidindex = j; // set current index as *the* pid to descramble

				if (ecmcounter == 1) // if total found running ecmfilters is 1 -> we found the "best" pid
				{
					dvbapi_edit_channel_cache(i, j, 1);
					demux[i].ECMpids[j].checked = 4; // mark best pid last ;)
				}

				cs_log_dbg(D_DVBAPI,"Demuxer %d descrambling PID %d CAID %04X PROVID %06X ECMPID %04X CHID %02X",
							i, demux[i].curindex, er->caid, er->prid, er->pid, er->chid);
			}
			pthread_mutex_unlock(&demux[i].answerlock); // and release it!
		}

		if (er->rc >= E_NOTFOUND) // not found on requestmode 0 + 1
		{
			if (er->rc == E_SLEEPING)
			{
				dvbapi_stop_descrambling(i);
				return;
			}
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
			if (er->ecm_bypass) continue;
#endif
			struct s_dvbapi_priority *forceentry = dvbapi_check_prio_match(i, j, 'p');

			if (forceentry && forceentry->force) // forced pid? keep trying the forced ecmpid!
			{
				if (!caid_is_irdeto(er->caid) || forceentry->chid < 0x10000)   //all cas or irdeto cas with forced prio chid
				{
					demux[i].ECMpids[j].table = 0;
					dvbapi_set_section_filter(i, er, -1);
					continue;
				}
				else // irdeto cas without chid prio forced
				{
					if (  demux[i].ECMpids[j].irdeto_curindex == 0xfe) { demux[i].ECMpids[j].irdeto_curindex = 0x00; }  // init irdeto current index to first one
					if (!(demux[i].ECMpids[j].irdeto_curindex + 1 > demux[i].ECMpids[j].irdeto_maxindex))  // check for last / max chid
					{
						cs_log_dbg(D_DVBAPI, "Demuxer %d trying next irdeto chid of FORCED PID %d CAID %04X PROVID %06X ECMPID %04X", i,
								j, er->caid, er->prid, er->pid);
						demux[i].ECMpids[j].irdeto_curindex++; // irdeto index one up
						demux[i].ECMpids[j].table = 0;
						dvbapi_set_section_filter(i, er, -1);
						continue;
					}
				}
			}

			// in case of timeout or fatal LB event give this pid another try but no more than 1 try
			if ((er->rc == E_TIMEOUT || (er->rcEx && er->rcEx <= E2_CCCAM_NOCARD)) && demux[i].ECMpids[j].tries == 0xfe)
			{
#if defined(MODULE_XCAMD)
				MYDVB_TRACE("mydvb:    ecm.cw %s timeout{%d, %04X:%04X:%04X}\n",
						(demux[i].rdr) ? demux[i].rdr->label : "UNK",
						 er->ecm_bypass,
						 er->chid,
						 demux[i].ECMpids[j].CHID,
						 demux[i].ECMpids[j].iks_irdeto_chid);
				if (IS_ICS_READERS(demux[i].rdr)) {
					if ((er->caid>>8) == 0x06) {
						if (demux[i].ECMpids[j].CHID < 0x10000) continue;
						if (demux[i].ECMpids[j].CHID != er->chid) continue;
					}
				}
#endif
				demux[i].ECMpids[j].tries = 1;
				demux[i].ECMpids[j].table = 0;
				dvbapi_set_section_filter(i, er, -1);
				continue;
			}
			else // all not found responses exception: first timeout response and first fatal loadbalancer response
			{
				MYDVB_TRACE("mydvb:    ecm.cw %s failure{%d,%d, %02X}\n",
						(demux[i].rdr) ? demux[i].rdr->label : "UNK",
						 er->rc, er->rcEx,
						 demux[i].ECMpids[j].tries);
				demux[i].ECMpids[j].CHID  = 0x10000; // get rid of this prio chid since it failed!
				demux[i].ECMpids[j].tries = 0xfe; 	 // reset timeout retry
			}

			if (caid_is_irdeto(er->caid))
			{
				if (demux[i].ECMpids[j].irdeto_curindex == 0xfe) { demux[i].ECMpids[j].irdeto_curindex = 0x00; }  // init irdeto current index to first one
				if (!(demux[i].ECMpids[j].irdeto_curindex+1 > demux[i].ECMpids[j].irdeto_maxindex)) // check for last / max chid
				{
					cs_log_dbg(D_DVBAPI, "Demuxer %d trying next irdeto chid of PID %d CAID %04X PROVID %06X ECMPID %04X VPID %04X", i,
								  j, er->caid, er->prid, er->pid, er->vpid);
					demux[i].ECMpids[j].irdeto_curindex++; // irdeto index one up
					demux[i].ECMpids[j].table = 0;
					mycs_trace(D_ADB, "mydvb:    irdeto_curindex increase{%d}", demux[i].ECMpids[j].irdeto_curindex);
					dvbapi_set_section_filter(i, er, -1);
					continue;
				}
				// sky(a)
				else if (++demux[i].ECMpids[j].irdeto_cycling < 3)
				{
					demux[i].ECMpids[j].irdeto_curindex = 0;
					demux[i].ECMpids[j].table = 0;
					mycs_trace(D_ADB, "mydvb:    irdeto_cycling increase{%d}", demux[i].ECMpids[j].irdeto_curindex);
					continue;
				}
			}

			dvbapi_edit_channel_cache(i, j, 0); // remove this pid from channelcache
			if (demux[i].pidindex == j)
			{
				demux[i].pidindex = -1; // current pid delivered a notfound so this pid isnt being used to descramble any longer-> clear pidindex
			}
			demux[i].ECMpids[j].irdeto_maxindex = 0;
			demux[i].ECMpids[j].irdeto_curindex = 0xfe;
			demux[i].ECMpids[j].irdeto_cycle 	= 0xfe; // reset irdetocycle
			demux[i].ECMpids[j].tries 	 = 0xfe; // reset timeout retry flag
			demux[i].ECMpids[j].table   = 0;
			demux[i].ECMpids[j].checked = 4; 	// flag ecmpid as checked
			demux[i].ECMpids[j].status  = -1; 	// flag ecmpid as unusable
			int32_t found = 1; // setup for first run
			int32_t filternum = -1;

			while (found > 0) // disable all ecm + emm filters for this notfound
			{
				found = 0;
				filternum = dvbapi_get_filternum(i,er, TYPE_ECM); // get ecm filternumber
				if (filternum > -1) // in case valid filter found
				{
					int32_t fd = demux[i].demux_fd[filternum].fd;
					if (fd > 0) // in case valid fd
					{
						mycs_trace(D_ADB, "mydvb:    ecm filternum{%d}", filternum);
						dvbapi_stop_filternum(i, filternum); // stop ecmfilter
						found = 1;
					}
				}
				if (caid_is_irdeto(er->caid))   // in case irdeto cas stop old emm filters
				{
					filternum = dvbapi_get_filternum(i,er, TYPE_EMM); // get emm filternumber
					if (filternum > -1) // in case valid filter found
					{
						int32_t fd = demux[i].demux_fd[filternum].fd;
						if (fd > 0) // in case valid fd
						{
							mycs_trace(D_ADB, "mydvb:    irdeto emm filternum{%d}", filternum);
							dvbapi_stop_filternum(i, filternum); // stop emmfilter
							found = 1;
						}
					}
				}
			}
		//	emux[i].ECMpids[j].CHID = 0x10000; // get rid of this prio chid since it obvious failed!
		//	if (cfg.dvbapi_requestmode == 0) dvbapi_try_next_caid(i, 0, 0); // in case of requestmode 0 start descrambling of next ecmpid
			continue;
		}


		// below this should be only run in case of ecm answer is found
#if defined(MODULE_XCAMD)
		if (er->ecm_bypass)
		{
			MYDVB_TRACE("mydvb:    ecm_bypass{%d,%d}\n", demux[i].curindex, demux[i].cs_soleMatch);
			if ((IS_IRDETO(er->caid)) &&
				 (demux[i].cs_soleMatch) &&
				 (demux[i].ECMpids[j].iks_irdeto_chid < 0x10000))
			{
				demux[i].ECMpids[j].CHID = demux[i].ECMpids[j].iks_irdeto_chid;
				MYDVB_TRACE("mydvb:    csxir_cw?{%02X.%04X}\n", demux[i].ECMpids[j].iks_irdeto_pi, demux[i].ECMpids[j].iks_irdeto_chid);
				if (nocw_write) continue;
			}
		}
		else
#endif
		{
			uint32_t chid = get_subid(er); // derive current chid in case of irdeto, or a unique part of ecm on other cas systems

#if defined(MODULE_XCAMD)
			if ((IS_IRDETO(er->caid)) &&
				 (er->ecm_cssolo) &&
				 (demux[i].ECMpids[j].iks_irdeto_chid < 0x10000))
			{
				demux[i].ECMpids[j].CHID = demux[i].ECMpids[j].iks_irdeto_chid;
				MYDVB_TRACE("mydvb:    csxir_cw?{%02X.%04X:%04X}\n", demux[i].ECMpids[j].iks_irdeto_pi, demux[i].ECMpids[j].iks_irdeto_chid, demux[i].ECMpids[j].CHID);
			}
			else
#endif
			{
				demux[i].ECMpids[j].CHID = (chid != 0 ? chid : 0x10000); // if not zero apply, otherwise use no chid value 0x10000
			}
			dvbapi_edit_channel_cache(i, j, 1); // do it here to here after the right CHID is registered
		// sky()
		// is not needed anymore (unsure)
		//	dvbapi_set_section_filter(i, er);
			demux[i].ECMpids[j].tries = 0xfe; // reset timeout retry flag
			demux[i].ECMpids[j].irdeto_cycle = 0xfe; // reset irdeto cycle
			MYDVB_TRACE("mydvb:    write_cw(%d.%d) {%d,%04X.%04X}\n", j, nocw_write, demux[i].curindex, chid, demux[i].ECMpids[j].CHID);

			if (nocw_write || demux[i].pidindex != j) { continue; }  // cw was already written by another filter or current pid isnt pid used to descramble so it ends here!

			struct s_dvbapi_priority *delayentry = dvbapi_check_prio_match(i, demux[i].pidindex, 'd');
			if (delayentry)
			{
				if (delayentry->delay < 1000)
				{
					cs_log_dbg(D_DVBAPI, "wait %d ms", delayentry->delay);
					cs_sleepms(delayentry->delay);
				}
			}

			delayer(er);
		}

		switch(selected_api)
		{
#ifdef WITH_STAPI
			case STAPI:
				stapi_write_cw (i, er->cw, demux[i].STREAMpids, demux[i].STREAMpidcount, demux[i].pmt_file);
				break;
#endif
			default:
				dvbapi_write_cw(i, er->cw, &er->cwEx, j, er->cwdesalgo);
				break;
		}

		// reset idle-Time
		client->last = time((time_t *)0); // ********* TO BE FIXED LATER ON ******
#if defined(MODULE_XCAMD)
		if (demux[i].cs_soleMatch)
		{
			int16_t current = demux[i].curindex;
			int16_t iksidx  = demux[i].cs_cwidx;
			if (iksidx != -1 && current != iksidx) {
				MYDVB_TRACE("mydvb:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx TRY{%d,%d}\n", current, iksidx);
				dvbapi_stop_filter(i, TYPE_ECM);
				dvbapi_try_goto_caid(i, 0, iksidx); // start descrambling of direct ecmpid
				return;
			}
		}
#endif

#if defined(WITH_HISILICON)
		//
		// UNAVAILABLE...
		//
#else
	#ifdef WITH_DEBUG
		FILE *ecmtxt = NULL;
		if (cfg.dvbapi_listenport && client_proto_version >= 2)
			dvbapi_net_send(DVBAPI_ECM_INFO, demux[i].socket_fd, i, 0, NULL, client, er);
		else if (!cfg.dvbapi_listenport && cfg.dvbapi_boxtype != BOXTYPE_PC_NODMX)
			ecmtxt = fopen(ECMINFO_FILE, "w");
		if (ecmtxt != NULL && er->rc < E_NOTFOUND)
		{
			char tmp[25];
			fprintf(ecmtxt, "caid: 0x%04X\npid: 0x%04X\nprov: 0x%06X\n", er->caid, er->pid, (uint) er->prid);
			switch(er->rc)
			{
				case E_FOUND:
					if (er->selected_reader)
					{
						fprintf(ecmtxt, "reader: %s\n", er->selected_reader->label);
						if (is_network_reader(er->selected_reader))
						{ fprintf(ecmtxt, "from: %s\n", er->selected_reader->device); }
						else
						{ fprintf(ecmtxt, "from: local\n"); }
						fprintf(ecmtxt, "protocol: %s\n", reader_get_type_desc(er->selected_reader, 1));
						fprintf(ecmtxt, "hops: %d\n", er->selected_reader->currenthops);
					}
					break;

				case E_CACHE1:
					fprintf(ecmtxt, "reader: Cache\n");
					fprintf(ecmtxt, "from: cache1\n");
					fprintf(ecmtxt, "protocol: none\n");
					break;

				case E_CACHE2:
					fprintf(ecmtxt, "reader: Cache\n");
					fprintf(ecmtxt, "from: cache2\n");
					fprintf(ecmtxt, "protocol: none\n");
					break;

				case E_CACHEEX:
					fprintf(ecmtxt, "reader: Cache\n");
					fprintf(ecmtxt, "from: cache3\n");
					fprintf(ecmtxt, "protocol: none\n");
					break;
			}
			fprintf(ecmtxt, "ecm time: %.3f\n", (float) client->cwlastresptime/1000);
			fprintf(ecmtxt, "cw0: %s\n", cs_hexdump(1,demux[i].lastcw[0],8, tmp, sizeof(tmp)));
			fprintf(ecmtxt, "cw1: %s\n", cs_hexdump(1,demux[i].lastcw[1],8, tmp, sizeof(tmp)));
		}
		if (ecmtxt)
		{
			int32_t ret = fclose(ecmtxt);
			if (ret < 0) { cs_log("ERROR: Could not close ecmtxt fd (errno=%d %s)", errno, strerror(errno)); }
			ecmtxt = NULL;
		}
	#endif
#endif	// defined(WITH_HISILICON)
	}

	if (handled == 0)
	{
		cs_log_dbg(D_DVBAPI, "Unhandled ECM response received for CAID %04X PROVID %06X ECMPID %04X CHID %04X VPID %04X",
					  er->caid, er->prid, er->pid, er->chid, er->vpid);
	}

}

static void *dvbapi_handler(struct s_client *cl, uchar* UNUSED(mbuf), int32_t module_idx)
{
//	cs_log("dvbapi loaded fd=%d", idx);
	if (cfg.dvbapi_enabled == 1)
	{
		cl = create_client(get_null_ip());
		cl->module_idx = module_idx;
		cl->typ = 'c';
		int32_t ret = pthread_create(&cl->thread, NULL, dvbapi_main_local, (void *) cl);
		if (ret) {
			cs_log("ERROR: Can't create dvbapi handler thread (errno=%d %s)", ret, strerror(ret));
			return NULL;
		}
		else {
			pthread_detach(cl->thread);
		}
	}

	return NULL;
}

int32_t dvbapi_set_section_filter(int32_t demux_index, ECM_REQUEST *er, int32_t n)
{
//	if (cfg.dvbapi_requestmode == 1) return 0; // no support for requestmode 1
// sky(a)
	if (selected_api == HISILICONAPI) return 0;
	if (selected_api != DVBAPI_3 && selected_api != DVBAPI_1 && selected_api != STAPI) { // only valid for dvbapi3 && dvbapi1
		return 0;
	}
	if (!er) return -1;
	mycs_trace(D_ADB, "mydvb:dvbapi_set_section_filter{%04X:%08X: %04x}(%d)", er->caid, er->prid, er->pid, er->rc);
	if (n == -1)
	{
		 n = dvbapi_get_filternum(demux_index, er, TYPE_ECM);
	}

	if (n < 0) { return -1; }  // in case no valid filter found;

	int32_t fd = demux[demux_index].demux_fd[n].fd;
	if (fd < 1) { return -1 ; }  // in case no valid fd

	uchar filter[16];
	uchar fimask[16];
	memset(filter,0,16);
	memset(fimask,0,16);

	struct s_ecmpids *curpids = NULL;
	int32_t pidx = demux[demux_index].demux_fd[n].pidindex;
	if (pidx == -1) { return -1 ; }
	curpids = &demux[demux_index].ECMpids[pidx];
	if (curpids->table != er->ecm[0] && curpids->table != 0) return -1; // if current ecmtype differs from latest requested ecmtype do not apply section filtering!
	uint8_t ecmfilter = 0;

	if (er->ecm[0]==0x80) ecmfilter = 0x81; // current processed ecm is even, next will be filtered for odd
	else ecmfilter = 0x80; // current processed ecm is odd, next will be filtered for even

	if (curpids->table != 0) { // cycle ecmtype from odd to even or even to odd
		filter[0] = ecmfilter; 	// only accept new ecms (if previous odd, filter for even and visaversa)
		fimask[0] = 0xFF;
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d set ecmtable to %s (CAID %04X PROVID %06X FD %d)", demux_index, n+1,
				(ecmfilter == 0x80?"EVEN":"ODD"), curpids->CAID, curpids->PROVID, fd);
	}
	else { // not decoding right now so we are interessted in all ecmtypes!
		filter[0] = 0x80; // set filter to wait for any ecms
		fimask[0] = 0xF0;
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d set ecmtable to ODD+EVEN (CAID %04X PROVID %06X FD %d)", demux_index, n+1,
				curpids->CAID, curpids->PROVID, fd);
	}
	uint32_t offset = 0;
	uint32_t extramask = 0xFF;

	struct s_dvbapi_priority *forceentry = dvbapi_check_prio_match(demux_index, pidx, 'p');
//	cs_log("**** curpids->CHID %04X, checked = %d, er->chid = %04X *****", curpids->CHID, curpids->checked, er->chid);
	// checked 4 to make sure we dont set chid filter and no such ecm in dvbstream except for forced pids!
	if (curpids->CHID < 0x10000 && (curpids->checked==4 || (forceentry && forceentry->force)))
	{

		switch(er->caid >> 8)
		{
		case 0x01:
			offset = 7;
			extramask = 0xF0;
			break; // seca
		case 0x05:
			offset = 8;
			break; // viaccess
		case 0x06:
			offset = 6;
			break; // irdeto
		case 0x09:
			offset = 11;
			break; // videoguard
		case 0x4A:								// DRE-Crypt, Bulcrypt,Tongang and others?
			if (!caid_is_bulcrypt(er->caid))
				{ offset = 6; }
			break;
		}
	}

	int32_t irdetomatch = 1; // check if wanted irdeto index is the one the delivers current chid!
	if (caid_is_irdeto(curpids->CAID))
	{
		if (curpids->irdeto_curindex == er->ecm[4]) irdetomatch = 1; // ok apply chid filtering
		else irdetomatch = 0; // skip chid filtering but apply irdeto index filtering
	}

	if (offset && irdetomatch) // we have a cas with chid or unique part in checked ecm
	{
		i2b_buf(2, curpids->CHID, filter + (offset-2));
		fimask[(offset - 2)] = 0xFF&extramask; // additional mask seca2 chid can be FC10 or FD10 varies each month so only apply F?10
		fimask[(offset - 1)] = 0xFF;
		cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d set chid to %04X on fd %d", demux_index, n + 1, curpids->CHID, fd);
	}
	else
	{
		if (caid_is_irdeto(curpids->CAID) && (curpids->irdeto_curindex < 0xfe)) // on irdeto we can always apply irdeto index filtering!
		{
			filter[2] = curpids->irdeto_curindex;
			fimask[2] = 0xFF;
			cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d set irdetoindex to %d on fd %d", demux_index, n + 1, curpids->irdeto_curindex, fd);
		}
		else  // all other cas systems also cas systems without chid or unique ecm part
		{
			cs_log_dbg(D_DVBAPI, "Demuxer %d Filter %d set chid to ANY CHID on fd %d", demux_index, n + 1, fd);
		}
	}

	int32_t ret = dvbapi_activate_section_filter(demux_index, n, fd, curpids->ECM_PID, filter, fimask);
	if (ret < 0) // something went wrong setting filter!
	{
		cs_log("Demuxer %d Filter %d (fd %d) error setting section filtering -> stop filter!", demux_index, n + 1, fd);
		ret = dvbapi_stop_filternum(demux_index, n);
		if (ret == -1)
		{
			cs_log("Demuxer %d Filter %d (fd %d) stopping filter failed -> kill all filters of this demuxer!", demux_index, n + 1, fd);
			dvbapi_stop_filter(demux_index, TYPE_EMM);
			dvbapi_stop_filter(demux_index, TYPE_ECM);
		}
		return -1;
	}
	return (n);
}

int32_t dvbapi_activate_section_filter(int32_t demux_index, int32_t num, int32_t fd, int32_t pid, uchar *filter, uchar *mask)
{

	int32_t ret = -1;
	switch (selected_api)
	{
		case DVBAPI_3:
		{
		    struct dmx_sct_filter_params sFP2;
		    memset(&sFP2,0,sizeof(sFP2));
		    sFP2.pid			= pid;
		    sFP2.timeout		= 0;
		    sFP2.flags			= DMX_IMMEDIATE_START;
			if (cfg.dvbapi_boxtype == BOXTYPE_NEUMO)
			{
				//DeepThought: on dgs/cubestation and neumo images, perhaps others
				//the following code is needed to descramble
				sFP2.filter.filt[0]=filter[0];
				sFP2.filter.mask[0]=mask[0];
				sFP2.filter.filt[1]=0;
				sFP2.filter.mask[1]=0;
				sFP2.filter.filt[2]=0;
				sFP2.filter.mask[2]=0;
				memcpy(sFP2.filter.filt+3,filter+1,16-3);
				memcpy(sFP2.filter.mask+3,mask+1,16-3);
				//DeepThought: in the drivers of the dgs/cubestation and neumo images,
				//dvbapi 1 and 3 are somehow mixed. In the kernel drivers, the DMX_SET_FILTER
				//ioctl expects to receive a dmx_sct_filter_params structure (DVBAPI 3) but
				//due to a bug its sets the "positive mask" wrongly (they should be all 0).
				//On the other hand, the DMX_SET_FILTER1 ioctl also uses the dmx_sct_filter_params
				//structure, which is incorrect (it should be  dmxSctFilterParams).
				//The only way to get it right is to call DMX_SET_FILTER1 with the argument
				//expected by DMX_SET_FILTER. Otherwise, the timeout parameter is not passed correctly.
				ret = dvbapi_ioctl(fd, DMX_SET_FILTER1, &sFP2);
			}
			else
			{
				memcpy(sFP2.filter.filt,filter,16);
		    	memcpy(sFP2.filter.mask,mask,16);
				if (cfg.dvbapi_listenport || cfg.dvbapi_boxtype == BOXTYPE_PC_NODMX)
					ret = dvbapi_net_send(DVBAPI_DMX_SET_FILTER, demux[demux_index].socket_fd, demux_index, num, (unsigned char *) &sFP2, NULL, NULL);
				else
					ret = dvbapi_ioctl(fd, DMX_SET_FILTER, &sFP2);
			}
			break;
		}

		case DVBAPI_1:
		{
		    struct dmxSctFilterParams sFP1;
		    memset(&sFP1,0,sizeof(sFP1));
		    sFP1.pid = pid;
		    sFP1.timeout = 0;
		    sFP1.flags = DMX_IMMEDIATE_START;
		    memcpy(sFP1.filter.filt,filter,16);
		    memcpy(sFP1.filter.mask,mask,16);
			ret = dvbapi_ioctl(fd, DMX_SET_FILTER1, &sFP1);
			break;
		}
#ifdef WITH_STAPI
	   case STAPI:
	   {
		   ret = stapi_activate_section_filter(fd, filter, mask);
			break;
		}
#endif
/*
#ifdef WITH_COOLAPI ******* NOT IMPLEMENTED YET ********
		case COOLAPI: {
			coolapi_set_filter(demux[demux_id].demux_fd[n].fd, n, pid, filter, mask, TYPE_ECM);
			break;
		}
#endif
*/
#ifdef WITH_HISILICON
		case HISILICONAPI:
			break;
#endif
		default:
			break;
	}
	return ret;
}


int32_t dvbapi_check_ecm_delayed_delivery(int32_t demux_index, ECM_REQUEST *er)
{
	char nullcw[CS_ECMSTORESIZE];

	memset(nullcw, 0, CS_ECMSTORESIZE);
	if (memcmp(er->cw, nullcw, 8) == 0 && memcmp(er->cw+8, nullcw, 8) == 0) return 5; // received a null cw -> not usable!
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	if (demux[demux_index].constcw_fbiss) return 9;
	if (caid_is_biss(er->caid)) return 9;
	if (er->ecm_bypass) { return 0; }
#endif
	int32_t filternum = dvbapi_get_filternum(demux_index, er, TYPE_ECM);
	if (filternum < 0) {
		MYDVB_TRACE("mydvb:    unk.filter(?%d)\n", filternum);
		return 2; // if no matching filter act like ecm response is delayed
	}

	struct s_ecmpids *curpids = NULL;
	int32_t pidx = demux[demux_index].demux_fd[filternum].pidindex;
	if(pidx != -1)
	{
		curpids = &demux[demux_index].ECMpids[pidx];
		if (curpids->table == 0) return 3; // on change table act like ecm response is found
	}

	if (er->rc == E_CACHEEX) return 4; // on cache-ex response act like ecm response is found

	if (memcmp(demux[demux_index].demux_fd[filternum].ecmd5, nullcw, CS_ECMSTORESIZE))
	{
		char ecmd5[17*3];
		cs_hexdump(0, demux[demux_index].demux_fd[filternum].ecmd5, 16, ecmd5, sizeof(ecmd5));
		cs_log_dbg(D_DVBAPI, "Demuxer %d requested controlword for ecm %s on fd %d", demux_index, ecmd5, demux[demux_index].demux_fd[filternum].fd);
		return memcmp(demux[demux_index].demux_fd[filternum].ecmd5, er->ecmd5, CS_ECMSTORESIZE); // 1 = no response on the ecm we request last for this fd!
	}
	else return 0;
}

int32_t dvbapi_get_filternum(int32_t demux_index, ECM_REQUEST *er, int32_t type)
{
	if (!er) return -2;

	int32_t n;
	int32_t fd = -1;

	for (n = 0; n < maxfilter; n++) // determine fd
	{
		if (demux[demux_index].demux_fd[n].fd > 0 && demux[demux_index].demux_fd[n].type == type ) // check for valid and right type (ecm or emm)
		{
			if (type == TYPE_ECM && er->srvid != demux[demux_index].program_number) continue;
			if ((demux[demux_index].demux_fd[n].pid == er->pid) &&
				((demux[demux_index].demux_fd[n].provid == er->prid) || demux[demux_index].demux_fd[n].provid == 0 || er->prid == 0) &&
				((demux[demux_index].demux_fd[n].caid == er->caid) || (demux[demux_index].demux_fd[n].caid == er->ocaid))) // current ecm pid?
			{
				fd = demux[demux_index].demux_fd[n].fd; // found!
				break;
			}
		}
	}
	if (fd > 0 && demux[demux_index].demux_fd[n].provid == 0) demux[demux_index].demux_fd[n].provid = er->prid; // hack to fill in provid into demuxer

	return (fd > 0 ? n : fd); // return -1(fd) on not found, on found return filternumber(n)
}

int32_t dvbapi_ca_setpid(int32_t demux_index, int32_t pidk)
{
	int32_t idx = -1, n;
	if (pidk == -1 || pidk > demux[demux_index].ECMpidcount) return -1;

	idx = demux[demux_index].ECMpids[pidk].index;

	if (!idx) // if no indexer for this pid get one!
	{
		idx = dvbapi_get_descindex(demux_index);
		demux[demux_index].ECMpids[pidk].index= idx;
		cs_log_dbg(D_DVBAPI,"Demuxer %d PID: %d CAID: %04X ECMPID: %04X is using index %d", demux_index, pidk,
			demux[demux_index].ECMpids[pidk].CAID, demux[demux_index].ECMpids[pidk].ECM_PID, idx-1);
	}

	for (n=0;n<demux[demux_index].STREAMpidcount;n++)
	{
		if (!demux[demux_index].ECMpids[pidk].streams || (demux[demux_index].ECMpids[pidk].streams & (1 << n))) {
			dvbapi_set_pid(demux_index, n, idx-1, true);  // enable streampid
		}
		else {
			dvbapi_set_pid(demux_index, n, idx-1, false); // disable streampid
		}
	}

	return idx-1; // return caindexer
}

int8_t update_streampid_list(uint8_t cadevice, uint16_t pid, int32_t idx)
{
	struct s_streampid *listitem, *newlistitem;
	if (!ll_activestreampids)
		{ ll_activestreampids = ll_create("ll_activestreampids"); }
	LL_ITER itr;
	if (ll_count(ll_activestreampids) > 0)
	{
		itr = ll_iter_create(ll_activestreampids);
		while ((listitem = ll_iter_next(&itr)))
		{
			if (cadevice == listitem->cadevice && pid == listitem->streampid) {
				if((listitem->activeindexers & (1 << idx)) == (uint) (1 << idx)){
					return FOUND_STREAMPID_INDEX; // match found
				}else{
					listitem->activeindexers|=(1 << idx); // ca + pid found but not this index -> add this index
					cs_log_dbg(D_DVBAPI, "Added existing streampid %04X with new index %d to ca%d", pid, idx, cadevice);
					return ADDED_STREAMPID_INDEX;
				}
			}
		}
	}
	if (!cs_malloc(&newlistitem, sizeof(struct s_streampid)))
		{ return FIRST_STREAMPID_INDEX; }
	newlistitem->cadevice = cadevice;
	newlistitem->streampid = pid;
	newlistitem->activeindexers = (1 << idx);
	newlistitem->caindex = idx; // set this index as used to decode on ca device
	ll_append(ll_activestreampids, newlistitem);
	cs_log_dbg(D_DVBAPI, "Added new streampid %04X with index %d to ca%d", pid, idx, cadevice);
	return FIRST_STREAMPID_INDEX;
}

int8_t remove_streampid_from_list(uint8_t cadevice, uint16_t pid, int32_t idx)
{
	if (!ll_activestreampids) return NO_STREAMPID_LISTED;

	struct s_streampid *listitem;
	int8_t removed = 0;

	LL_ITER itr;
	if (ll_count(ll_activestreampids) > 0)
	{
		itr = ll_iter_create(ll_activestreampids);
		while ((listitem = ll_iter_next(&itr)))
		{
			if (cadevice == listitem->cadevice && pid == listitem->streampid)
			{
				if (idx == -1) { // idx -1 means disable all!
					listitem->activeindexers = 0;
					removed = 1;
				}
				else if ((listitem->activeindexers & (1 << idx)) == (uint) (1 << idx))
				{
					listitem->activeindexers &= ~(1 << idx); // flag it as disabled for this index
					removed = 1;
				}

				if (removed)
				{
					cs_log_dbg(D_DVBAPI, "Remove streampid %04X using indexer %d from ca%d", pid, idx, cadevice);
				}
				if (listitem->activeindexers == 0 && removed == 1) // all indexers disabled? -> remove pid from list!
				{
					ll_iter_remove_data(&itr);
					cs_log_dbg(D_DVBAPI, "Removed last indexer of streampid %04X from ca%d", pid, cadevice);
					return REMOVED_STREAMPID_LASTINDEX;
				}
				else if(removed == 1)
				{
					if (idx > 0 && (uint) idx != listitem->caindex)
					{
				      return REMOVED_STREAMPID_INDEX;
			      }
					else
					{
						listitem->caindex = 0xFFFF;
						cs_log_dbg(D_DVBAPI, "Streampid %04X index %d was used for decoding on ca%d", pid, idx, cadevice);
						return REMOVED_DECODING_STREAMPID_INDEX;
					}
				}
		   }
	   }
	}
	return NO_STREAMPID_LISTED;
}

void disable_unused_streampids(int16_t demux_id)
{
	if (!ll_activestreampids) return;
	if (ll_count(ll_activestreampids) == 0) return; // no items in list?

	int32_t ecmpid = demux[demux_id].pidindex;
	if (ecmpid == -1) return; // no active ecmpid!

	int32_t idx = demux[demux_id].ECMpids[ecmpid].index;
	int32_t i,n;
	struct s_streampid *listitem;
	// search for old enabled streampids on all ca devices that have to be disabled, index 0 is skipped as it belongs to fta!
	for (i = 0; i < MAX_DEMUX && idx; i++) {
		if (!((demux[demux_id].ca_mask & (1 << i)) == (uint) (1 << i))) continue; // continue if ca is unused by this demuxer

		LL_ITER itr;
		itr = ll_iter_create(ll_activestreampids);
		while ((listitem = ll_iter_next(&itr)))
		{
			if (i != listitem->cadevice) continue; // ca doesnt match
			if (!((listitem->activeindexers & (1 << (idx-1))) == (uint) (1 << (idx-1)))) continue; // index doesnt match
			for(n = 0; n < demux[demux_id].STREAMpidcount; n++) {
				if (listitem->streampid == demux[demux_id].STREAMpids[n]) { // check if pid matches with current streampid on demuxer
					break;
				}
			}
			if (n == demux[demux_id].STREAMpidcount) {
				demux[demux_id].STREAMpids[n] = listitem->streampid; // put it temp here!
				dvbapi_set_pid(demux_id, n, idx - 1, false); // no match found so disable this now unused streampid
				demux[demux_id].STREAMpids[n] = 0; // remove temp!
			}
		}
	}
}


int8_t is_ca_used(uint8_t cadevice, int32_t pid)
{
	if (!ll_activestreampids) return CA_IS_CLEAR;

	struct s_streampid *listitem;

	LL_ITER itr;
	if (ll_count(ll_activestreampids) > 0)
	{
		itr = ll_iter_create(ll_activestreampids);
		while((listitem = ll_iter_next(&itr)))
		{
			if (listitem->cadevice != cadevice) continue;
			if (pid && listitem->streampid != pid) continue;
			uint32_t i = 0;
			int32_t newindex = -1;
			if (listitem->caindex != 0xFFFF)
			{
				newindex = listitem->caindex;
			}
			while(newindex == -1)
			{
				if ((listitem->activeindexers&(1 << i)) == (uint)(1 << i))
				{
					newindex = i;
				}
				i++;
			}
			if (listitem->caindex == 0xFFFF) // check if this pid has active index for ca device (0xFFFF means no active index!)
			{

				listitem->caindex = newindex; // set fresh one
				cs_log_dbg(D_DVBAPI, "Streampid %04X is now using index %d for decoding on ca%d", pid, newindex, cadevice);
			}
			return newindex;
		}
	}
	return CA_IS_CLEAR; // no indexer found for this pid!
}

const char *dvbapi_get_client_name(void)
{
	return client_name;
}

void check_add_emmpid(int32_t demux_index, uchar *filter, int32_t l, int32_t emmtype)
{
	if (l<0) return;

	uint32_t typtext_idx = 0;
	int32_t ret = -1;
	const char *typtext[] = { "UNIQUE", "SHARED", "GLOBAL", "UNKNOWN" };

	while(((emmtype >> typtext_idx) & 0x01) == 0 && typtext_idx < sizeof(typtext) / sizeof(const char *))
	{
		++typtext_idx;
	}

	//filter already in list?
	if (is_emmfilter_in_list(filter, demux[demux_index].EMMpids[l].PID, demux[demux_index].EMMpids[l].PROVID, demux[demux_index].EMMpids[l].CAID))
	{
		cs_log_dbg(D_DVBAPI, "Demuxer %d duplicate emm filter type %s, emmpid: 0x%04X, emmcaid: %04X, emmprovid: %06X -> SKIPPED!", demux_index,
			typtext[typtext_idx], demux[demux_index].EMMpids[l].PID, demux[demux_index].EMMpids[l].CAID, demux[demux_index].EMMpids[l].PROVID);
		return;
	}

	if (demux[demux_index].emm_filter < demux[demux_index].max_emmfilters) // can this filter be started? if not add to list of inactive emmfilters
	{
		// try to activate this emmfilter
		ret = dvbapi_set_filter(demux_index, selected_api, demux[demux_index].EMMpids[l].PID, demux[demux_index].EMMpids[l].CAID,
						demux[demux_index].EMMpids[l].PROVID, filter, filter + 16, 0, demux[demux_index].pidindex, TYPE_EMM, 1);
	}

	if (ret != -1) // -1 if maxfilter reached or filter start error!
	{
		if (demux[demux_index].emm_filter == -1) // -1: first run of emm filtering on this demuxer
		{
			demux[demux_index].emm_filter = 0;
		}
		demux[demux_index].emm_filter++; // increase total active filters
		cs_log_dump_dbg(D_DVBAPI, filter, 32, "Demuxer %d started emm filter type %s, pid: 0x%04X", demux_index, typtext[typtext_idx], demux[demux_index].EMMpids[l].PID);
		return;
	}
	else   // not set successful, so add it to the list for try again later on!
	{
		add_emmfilter_to_list(demux_index, filter, demux[demux_index].EMMpids[l].CAID, demux[demux_index].EMMpids[l].PROVID, demux[demux_index].EMMpids[l].PID, 0, false);
		cs_log_dump_dbg(D_DVBAPI, filter, 32, "Demuxer %d added inactive emm filter type %s, pid: 0x%04X", demux_index, typtext[typtext_idx], demux[demux_index].EMMpids[l].PID);
	}
	return;
}

void rotate_emmfilter(int32_t demux_id)
{
	// emm filter iteration
	if (!ll_emm_active_filter)
		{ ll_emm_active_filter = ll_create("ll_emm_active_filter"); }

	if (!ll_emm_inactive_filter)
		{ ll_emm_inactive_filter = ll_create("ll_emm_inactive_filter"); }

	if (!ll_emm_pending_filter)
		{ ll_emm_pending_filter = ll_create("ll_emm_pending_filter"); }

	uint32_t filter_count = ll_count(ll_emm_active_filter) + ll_count(ll_emm_inactive_filter);

	if (demux[demux_id].max_emmfilters > 0
			&& ll_count(ll_emm_inactive_filter) > 0
			&& filter_count > demux[demux_id].max_emmfilters)
	{

		int32_t filter_queue = ll_count(ll_emm_inactive_filter);
		int32_t stopped = 0, started = 0;
		struct timeb now;
		cs_ftime(&now);

		struct s_emm_filter *filter_item;
		LL_ITER itr;
		itr = ll_iter_create(ll_emm_active_filter);

		while((filter_item = ll_iter_next(&itr)) != NULL)
		{
			if (!ll_count(ll_emm_inactive_filter) || started == filter_queue)
				{ break; }
			int64_t gone = comp_timeb(&now, &filter_item->time_started);
			if ( gone > 45*1000)
			{
				struct s_dvbapi_priority *forceentry = dvbapi_check_prio_match_emmpid(filter_item->demux_id, filter_item->caid,
													   filter_item->provid, 'p');

				if (!forceentry || (forceentry && !forceentry->force))
				{
					// stop active filter and add to pending list
					dvbapi_stop_filternum(filter_item->demux_id, filter_item->num - 1);
					ll_iter_remove_data(&itr);
					add_emmfilter_to_list(filter_item->demux_id, filter_item->filter, filter_item->caid,
										  filter_item->provid, filter_item->pid, -1, false);
					stopped++;
				}
			}

			int32_t ret;
			if (stopped > started) // we have room for new filters, try to start an inactive emmfilter!
			{
				struct s_emm_filter *filter_item2;
				LL_ITER itr2 = ll_iter_create(ll_emm_inactive_filter);

				while((filter_item2 = ll_iter_next(&itr2)))
				{
					ret = dvbapi_set_filter(filter_item2->demux_id, selected_api, filter_item2->pid, filter_item2->caid,
											filter_item2->provid, filter_item2->filter, filter_item2->filter + 16, 0,
											demux[filter_item2->demux_id].pidindex, TYPE_EMM, 1);
					if(ret != -1)
					{
						ll_iter_remove_data(&itr2);
						started++;
						break;
					}
				}
			}
		}

		itr = ll_iter_create(ll_emm_pending_filter);

		while((filter_item = ll_iter_next(&itr)) != NULL) // move pending filters to inactive
		{
			add_emmfilter_to_list(filter_item->demux_id, filter_item->filter, filter_item->caid, filter_item->provid, filter_item->pid, 0, false);
			ll_iter_remove_data(&itr);
		}
	}
}

uint16_t dvbapi_get_client_proto_version(void)
{
	return client_proto_version;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
#if defined(MODULE_CONSTCW) || defined(MODULE_XCAS) || defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
void DVBICS_ChSettings(int32_t demux_id, int chstages)
{
	mycs_debug(D_ADB, "myics:requests{%d}...", chstages);
}

void DVBICS_ChCloser(int32_t demux_id)
{
	mycs_debug(D_ADB, "myics:closer...");
	#if defined(MODULE_XCAS)
		CSREADER_ChCloser(demux_id, &demux[demux_id], R_XCAS);
	#endif
	#if defined(MODULE_CONSTCW)
		CSREADER_ChCloser(demux_id, &demux[demux_id], R_CONSTCW);
	#endif
}

bool DVBICS_ChRetuning(int32_t demux_id, int32_t typ)
{
	bool bRetune = 0;

	if (demux[demux_id].curindex < 0)
	{
		MYDVB_TRACE("mydvb:rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr TRY\n");
		mycs_debug(D_ADB, "mycsdvb:cwRetune{%02X}", typ);
		dvbapi_try_next_caid(demux_id, 0, 1);
		bRetune = 2;
	}
	else
	{
		mycs_debug(D_ADB, "mycsdvb:cwRetuning{%02X}", typ);
		bRetune = CSREADER_ChRetuning(demux_id, &demux[demux_id], typ);
	}
	return (bRetune);
}

void DVBICS_ChDescrambled(int32_t demux_id, int descramble)
{
	#if defined(MODULE_XCAS)
		CSREADER_ChDescrambled(R_XCAS, descramble);
	#endif
	#if defined(MODULE_CONSTCW)
		CSREADER_ChDescrambled(R_CONSTCW, descramble);
	#endif
}


int32_t DVBICS_ChkEcmReaders(int32_t demux_index)
{
	struct s_reader *rdr;
	ECM_REQUEST ere;
	int32_t n;
	int32_t othersfound = 0;
	int32_t xcamfound = 0;

	for (n = 0; n < demux[demux_index].ECMpidcount; n++)
	{
		ere.dmuxid = demux_index;// sky(powervu)
		ere.caid   = ere.ocaid = demux[demux_index].ECMpids[n].CAID;
		ere.prid   = demux[demux_index].ECMpids[n].PROVID;
		ere.pid    = demux[demux_index].ECMpids[n].ECM_PID;
		ere.srvid  = demux[demux_index].program_number;
		ere.onid   = demux[demux_index].onid;
#if defined(MODULE_AVAMGCAMD)
		ere.exprid = ere.prid;
		if (IS_IRDETO(ere.caid)) {
			ere.exprid = (((ere.onid>>8)&0xff) ^ ((ere.onid)&0xff));
		}
#endif
		ere.client = cur_client();
		for (rdr=first_active_reader; rdr; rdr=rdr->next)
		{
			if (matching_reader(&ere, rdr, -1))
			{
				if (IS_ICS_READERS(rdr))
				{
					xcamfound++;
				}
				else othersfound++;
			}
		}
	}

	if (!othersfound && xcamfound) {
		mycs_trace(D_ADB, "mydvb:cs_soleMatch");
		demux[demux_index].cs_soleMatch = 1;
		return 1;
	}
	return 0;
}


int32_t
DVBICS_ChkEcmPids(int32_t demux_id, uint16_t srvid, uint16_t casid, uint32_t prid)
{
	int32_t n;

	if (!casid) return 0;
	if (demux[demux_id].program_number != srvid) return 0;
	for (n=0; n<demux[demux_id].ECMpidcount; n++)
	{
		struct s_ecmpids *epids = &(demux[demux_id].ECMpids[n]);
		if (epids->CAID == casid && epids->PROVID == prid)
		{
			demux[demux_id].cs_cwidx = n;
			demux[demux_id].cs_ecmRequisite = n;
			mycs_trace(D_ADB, "mydvb:--- ch.epid{%d.%04X}", n, epids->ECM_PID);
			return 1;
		}
	}
	return 0;
}
#endif	// defined(MODULE_XCAMD) || defined(MODULE_MORECAM)


#if defined(__DVCCRC_AVAILABLE__)
bool
DVBCRC_Addon(struct s_ecmpids *curpids, uint8_t *ecm, uint16_t ecmlen, uint32_t *crcval)
{
	uint32_t chid = 0x10000;
	uint8_t  ixpi = 0xfe;
	uint32_t	crcv;
	int32_t  i, bAdded = 1;

	if (!curpids) return 0;
	if ((crcv = crc32(0L, ecm, ecmlen))==0) return 0;
	for (i=0; i<MAX_CSCRC_CACHES; i++) {
		if (curpids->cc.ctab[i].crc == crcv) bAdded = 0;
	}

	chid = get_ecm_subid(ecm, curpids->CAID);
	ixpi = (IS_IRDETO(curpids->CAID)) ? ecm[4] : 0xfe;
	curpids->cc.current = (curpids->cc.current+1) % MAX_CSCRC_CACHES;
	curpids->cc.ctab[curpids->cc.current].chid = chid;
	curpids->cc.ctab[curpids->cc.current].pi   = ixpi;
	curpids->cc.ctab[curpids->cc.current].crc  = crcv;
//	curpids->cc.rto = time(NULL);
//	MYDVB_TRACE("mydvb:    crc{%02X,%04X}{%08X}\n", ixpi, chid, crcv);
	if (curpids->tableid && curpids->tableid != ecm[0]) {
		if (!curpids->cc.cwjustice) curpids->cc.cwjustice = 1;
	}
#if defined(MODULE_XCAMD)
	if (curpids->cc.cwcrc == crcv)	{
		if (IS_IRDETO(curpids->CAID))	{
			if (curpids->cc.cwjustice != 2) {
				curpids->cc.cwjustice   = 2;
				curpids->iks_irdeto_pi   = ixpi;
				curpids->iks_irdeto_chid = chid;
				curpids->irdeto_curindex = ixpi;
//				MYDVB_TRACE("mydvb:--- crc.chkAdd{%02X,%04X}{%08X}\n", ixpi, chid, crcv);
			}
		}
	}
#endif
	*crcval = crcv;
	return (bAdded);
}

void
DVBCRC_Clean(struct s_ecmpids *epids)
{
	int32_t i;

	if (!epids) return;
	epids->cc.cwcrc = 0;
	epids->cc.current = 0;
	epids->cc.cwjustice = 0;
	for (i=0; i<MAX_CSCRC_CACHES; i++) {
		epids->cc.ctab[i].chid = 0x10000;
		epids->cc.ctab[i].pi   = 0xfe;
		epids->cc.ctab[i].crc  = 0x0;
	}
//	epids->cc.rto = time(NULL);
#if defined(MODULE_XCAMD)
	epids->iks_irdeto_pi = 0xfe;
	epids->iks_irdeto_chid = 0x10000;
#endif
}

void
DVBCRC_Remove(int32_t demux_id, int32_t ecmpid)
{
	struct s_ecmpids *epids;
	int32_t n;

	for (n=0; n<demux[demux_id].ECMpidcount; n++) {
		epids = &demux[demux_id].ECMpids[n];
		if (ecmpid == 0x0)
		{
			DVBCRC_Clean(epids);
		}
		else
		if (epids->ECM_PID == ecmpid) {
			DVBCRC_Clean(epids);
			break;
		}
	}
}


int32_t
DVBCRC_ChkMatched(struct s_reader *rdr, int32_t demux_id, uint16_t casid, uint32_t prid, uint32_t ecmcrc)
{
	struct s_ecmpids *epids;
	int32_t  ccidx, ccfound = 0;
	int32_t  n, i;

	// CHINASAT Channel
	if (ecmcrc == 0) return -2;
	if (ecmcrc == (uint32_t)-1) return -1;
	if (demux[demux_id].curindex == -1) return -1;

	for (n=0; n<demux[demux_id].ECMpidcount; n++)
	{
		epids = &(demux[demux_id].ECMpids[n]);

		if (epids->CAID == casid && epids->PROVID == prid)
		{
			epids->cc.cwcrc = ecmcrc;
			for (i=epids->cc.current; !ccfound && i>=0; i--)
			{
			//	MYDVB_TRACE("mydvb:CRC{%2d:%04X.%08X}\n", n, ecmcrc, epids->cc.ctab[i].crc);
				if (epids->cc.ctab[i].crc == ecmcrc) {
					ccidx = i;
					ccfound = 1;
					break;
				}
			}
			for (i=(MAX_CSCRC_CACHES-1); !ccfound && i>epids->cc.current; i--)
			{
			//	MYDVB_TRACE("mydvb:CRC{%2d:%04X.%08X}\n", n, ecmcrc, epids->cc.ctab[i].crc);
				if (epids->cc.ctab[i].crc == ecmcrc) {
					ccidx = i;
					ccfound = 1;
					break;
				}
			}
			if (ccfound)
			{
#if defined(MODULE_XCAMD)
				if (IS_IRDETO(epids->CAID)) {
					uint32_t chid = epids->cc.ctab[ccidx].chid;
					uint8_t  ixpi = epids->cc.ctab[ccidx].pi;
					epids->iks_irdeto_pi   = ixpi;
					epids->iks_irdeto_chid = chid;
					epids->irdeto_curindex = ixpi;
					MYDVB_TRACE("mydvb:--- crc.irdeto{%d:%02X.%04X}{%08X}\n", n, ixpi, chid, ecmcrc);
				}
#endif // defined(MODULE_XCAMD)
				epids->cc.cwjustice = 2;
				if (ccidx==epids->cc.current) return 2;
//				int32_t tdiff;
//				time_t  now = time(0);
//				tdiff = abs(now - epids->cc.rto);
//				if (tdiff > 30) {
//					MYDVB_TRACE("mydvb:--- crc.bygone...\n");
//					return 0;
//				}
				return 1;
			}
			if (epids->cc.cwjustice == 0) return -1;
//			if (casid == 0x09CD) return 99;
			return 0;
		}
	}
	return -1;
}
#endif // defined(__DVCCRC_AVAILABLE__)

/*
 *	protocol structure
 */

void module_dvbapi(struct s_module *ph)
{
	MYDVB_TRACE("module_dvbapi...\n");
	ph->desc 			= "dvbapi";
	ph->type 			= MOD_CONN_SERIAL;
	ph->listenertype 	= LIS_DVBAPI;
#if defined(WITH_AZBOX)
	ph->s_handler 		= azbox_handler;
	ph->send_dcw 		= azbox_send_dcw;
#elif defined(WITH_MCA)
	ph->s_handler 		= mca_handler;
	ph->send_dcw 		= mca_send_dcw;
	selected_box 		= selected_api = 0; // HACK: This fixes incorrect warning about out of bounds array access in functionas that are not even called when WITH_MCA is defined
#else
	ph->s_handler 		= dvbapi_handler;
	ph->send_dcw 		= dvbapi_send_dcw;
#endif
}
#endif // HAVE_DVBAP
