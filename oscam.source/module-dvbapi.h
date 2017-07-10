#ifndef MODULE_DVBAPI_H_
#define MODULE_DVBAPI_H_

#ifdef HAVE_DVBAPI
#include <sys/un.h>

#define TYPE_ECM 				1
#define TYPE_EMM 				2
#define TYPE_CAT 				3
#define CAT_PID 				0x1

// demux api
#define DVBAPI_3				0
#define DVBAPI_1				1
#define STAPI					2
#define COOLAPI				3
#define HISILICONAPI			4

#define PORT					9000

#if defined(WITH_HISILICON)
	#define TMPDIR				"/var/"
	#define STANDBY_FILE		"/var/.pauseoscam"
	#define ECMINFO_FILE		"/var/ecm.info"
#else
	#define TMPDIR				"/tmp/"
	#define STANDBY_FILE		"/tmp/.pauseoscam"
	#define ECMINFO_FILE		"/tmp/ecm.info"
#endif

#define MAX_DEMUX 			16
#define MAX_CAID 				50
#define ECM_PIDS 				30
#define EMM_PIDS 				30
#define MAX_FILTER 			24

// WITH_HISILICON)
#define BOX_COUNT 			7

#define BOXTYPE_DREAMBOX	1
#define BOXTYPE_DUCKBOX		2
#define BOXTYPE_UFS910		3
#define BOXTYPE_DBOX2		4
#define BOXTYPE_IPBOX		5
#define BOXTYPE_IPBOX_PMT	6
#define BOXTYPE_DM7000		7
#define BOXTYPE_QBOXHD		8
#define BOXTYPE_COOLSTREAM	9
#define BOXTYPE_NEUMO		10
#define BOXTYPE_PC			11
// WITH_HISILICON)
//	현재 CAPMT는 BOXTYPE_PC로 주고있음.
#define BOXTYPE_HISILICON	12	// = BOXTYPE_PC
#define BOXTYPE_PC_NODMX 	13
#define BOXTYPES				13

#define REMOVED_STREAMPID_INDEX 				1
#define REMOVED_STREAMPID_LASTINDEX 		2
#define REMOVED_DECODING_STREAMPID_INDEX 	3
#define NO_STREAMPID_LISTED 			0
#define FOUND_STREAMPID_INDEX 		0
#define ADDED_STREAMPID_INDEX 		1
#define FIRST_STREAMPID_INDEX 		2
#define CA_IS_CLEAR 	-1
#define DUMMY_FD    	0xFFFF

//constants used int socket communication:
#define DVBAPI_PROTOCOL_VERSION         1

#define DVBAPI_CA_SET_PID      0x40086f87
#define DVBAPI_CA_SET_DESCR    0x40106f86
#define DVBAPI_DMX_SET_FILTER  0x403c6f2b
#define DVBAPI_DMX_STOP        0x00006f2a

#define DVBAPI_AOT_CA          0x9F803000
#define DVBAPI_AOT_CA_PMT      0x9F803200  //least significant byte is length (ignored)
#define DVBAPI_AOT_CA_STOP     0x9F803F04
#define DVBAPI_FILTER_DATA     0xFFFF0000
#define DVBAPI_CLIENT_INFO     0xFFFF0001
#define DVBAPI_SERVER_INFO     0xFFFF0002
#define DVBAPI_ECM_INFO        0xFFFF0003

#define DVBAPI_MAX_PACKET_SIZE 262         //maximum possible packet size

struct box_devices
{
	char 		*path;
	char 		*ca_device;
	char 		*demux_device;
	char 		*cam_socket_path;
	int8_t 	api;
};
//
//
// sky(n)
#if defined(MODULE_XCAMD)
	#define __DVCCRC_AVAILABLE__
#endif

#if defined(__DVCCRC_AVAILABLE__)
#define MAX_CSCRC_CACHES	32
struct s_crctab
{
	uint32_t chid;
	uint16_t	pi;
	uint32_t crc;
};

struct s_crccaches
{
	int32_t 	cwjustice;
	int32_t 	current;
	uint32_t	cwcrc;
	time_t	rto;	// last received
	struct 	s_crctab	ctab[MAX_CSCRC_CACHES];
};
#endif

struct s_ecmpids
{
	uint16_t CAID;
	uint32_t PROVID;
	uint16_t ECM_PID;
	uint32_t CHID;
	uint16_t EMM_PID;
	uint32_t VPID; 				// videopid
	uint8_t  irdeto_maxindex; 	// max irdeto indexes always fresh fetched from current ecm
	uint8_t  irdeto_curindex; 	// current irdeto index we want to handle
	uint8_t  irdeto_cycle; 		// temp var that holds the irdeto index we started with to detect if we cycled trough all indexes
	uint32_t irdeto_cycling;	// sky(a)
#if defined(MODULE_XCAMD)
	uint32_t iks_irdeto_chid;	// sky(n.for xcamd)
	uint8_t 	iks_irdeto_pi;
#endif
	int8_t   constcw;
	int8_t   checked;
	int8_t   status;
	uint8_t  tries;
	uint8_t 	table;
	uint8_t 	tableid;
	int8_t   index;
	uint32_t streams;
	// sky(dvn) /* tableid:0x50 */
	uint32_t crcprevious;
// sky(n)
#if defined(__DVCCRC_AVAILABLE__)
	struct s_crccaches cc;
#endif
};

typedef struct filter_s
{
	uint32_t fd; //FilterHandle
	int32_t  pidindex;
	int32_t  pid;
	uint16_t caid;
	uint32_t provid;
	uint16_t type;
	int32_t  count;
	uchar	 	ecmd5[CS_ECMSTORESIZE]; // last requested ecm md5
#ifdef WITH_STAPI
	int32_t  NumSlots;
	uint32_t SlotHandle[10];
	uint32_t BufferHandle[10];
#endif
} FILTERTYPE;

struct s_emmpids
{
	uint16_t CAID;
	uint32_t PROVID;
	uint16_t PID;
	uint8_t  type;
};

#define PTINUM 	10
#define SLOTNUM 	20
#define DMXMD5HASHSIZE  	16  // use MD5()
// sky(n)
enum polarisation {
	POLARISATION_HORIZONTAL     = 0x00,
	POLARISATION_VERTICAL       = 0x01,
	POLARISATION_CIRCULAR_LEFT  = 0x02,
	POLARISATION_CIRCULAR_RIGHT = 0x03
};

typedef struct demux_s
{
	int8_t   demux_index;
	FILTERTYPE demux_fd[MAX_FILTER];
	uint32_t ca_mask;
	int8_t   adapter_index;
	int32_t  socket_fd;
	int8_t   ECMpidcount;
	int8_t   EMMpidcount;
	struct timeb emmstart; // last time emm cat was started
	struct s_ecmpids ECMpids[ECM_PIDS];
	struct s_emmpids EMMpids[EMM_PIDS];
	uint16_t max_emmfilters;
	int8_t   STREAMpidcount;
	uint16_t STREAMpids[ECM_PIDS];
	int16_t  pidindex;
	int16_t  curindex;
	int8_t   max_status;
	uint16_t program_number;
	uint16_t onid;
	uint16_t tsid;
	uint16_t pmtpid;

	//	MODULE_XCAMD/MODULE_MORECAM
	int16_t  cs_cwidx;
	uint16_t cs_degree;
	uint16_t cs_frequency;
	uint16_t cs_symbolrate;
	uint16_t cs_polarisation;
	uint16_t cs_vidpid;
	int16_t  cs_ecmRequisite;
	uint16_t cs_soleMatch;
	uint32_t cs_filtnum;
	uint32_t cs_subsequence;
	int16_t  constcw_fbiss;
	uint32_t scrambled_counter;

	uint32_t enigma_namespace;
	uint8_t 	lastcw[2][8];
	int8_t 	emm_filter;
	uchar  	hexserial[8];
	struct 	s_reader *rdr;
	char 	 	pmt_file [30];
	time_t 	pmt_time;
	uint8_t  stopdescramble;
	uint8_t 	running;
	uint8_t 	old_ecmfiltercount; // previous ecm filtercount
	uint8_t 	old_emmfiltercount; // previous emm filtercount
	pthread_mutex_t answerlock; // requestmode 1 avoid race
#ifdef WITH_STAPI
	uint32_t DescramblerHandle[PTINUM];
	int32_t  desc_pidcount;
	uint32_t slot_assc[PTINUM][SLOTNUM];
#endif
	int8_t 	decodingtries; // -1 = first run
	struct timeb decstart,decend;
} DEMUXTYPE;

typedef struct s_streampid
{
	uint8_t	cadevice; 		// holds ca device
	uint16_t	streampid; 		// holds pids
	uint32_t	activeindexers;// bitmask indexers if streampid enabled for index bit is set
	uint32_t	caindex; 		// holds index that is used to decode on ca device
} STREAMPIDTYPE;

struct s_dvbapi_priority
{
	char 	 	type; // p or i
	uint16_t caid;
	uint32_t provid;
	uint16_t srvid;
	uint32_t chid;
	uint16_t ecmpid;
	uint16_t mapcaid;
	uint32_t mapprovid;
	uint16_t mapecmpid;
	int16_t  delay;
	int8_t   force;
	int8_t 	pidx;
#ifdef WITH_STAPI
	char 	 	devname[30];
	char 	 	pmtfile[30];
	int8_t 	disablefilter;
#endif
	struct s_dvbapi_priority *next;
};


#define DMX_FILTER_SIZE 		16


// dvbapi 1
typedef struct dmxFilter
{
	uint8_t 	filt[DMX_FILTER_SIZE];
	uint8_t 	mask[DMX_FILTER_SIZE];
} dmxFilter_t;

struct dmxSctFilterParams
{
	uint16_t		pid;
	dmxFilter_t	filter;
	uint32_t		timeout;
	uint32_t		flags;
#define DMX_CHECK_CRC	    	1
#define DMX_ONESHOT	    		2
#define DMX_IMMEDIATE_START 	4
#define DMX_BUCKET	    		0x1000	/* added in 2005.05.18 */
#define DMX_KERNEL_CLIENT   	0x8000
};

#define DMX_START1		  		_IOW('o',41,int)
#define DMX_STOP1		  			_IOW('o',42,int)
#define DMX_SET_FILTER1 		_IOW('o',43,struct dmxSctFilterParams *)
//------------------------------------------------------------------


// dbox2+ufs
typedef struct dmx_filter
{
	uint8_t  filt	[DMX_FILTER_SIZE];
	uint8_t  mask	[DMX_FILTER_SIZE];
	uint8_t  mode	[DMX_FILTER_SIZE];
} dmx_filter_t;


struct dmx_sct_filter_params
{
	uint16_t	    	pid;
	dmx_filter_t	filter;
	uint32_t	    	timeout;
	uint32_t	    	flags;
#define DMX_CHECK_CRC	    	1
#define DMX_ONESHOT	    		2
#define DMX_IMMEDIATE_START 	4
#define DMX_KERNEL_CLIENT   	0x8000
};

typedef struct ca_descr {
	uint32_t	index;
	uint32_t parity;	/* 0 == even, 1 == odd */
	uint8_t 	cw[8];
} ca_descr_t;

typedef struct ca_pid {
	uint32_t pid;
	int32_t  index;	/* -1 == disable */
} ca_pid_t;

// sky(powervu)
enum ca_descr_algo {
	CA_ALGO_DVBCSA,
	CA_ALGO_DES,
	CA_ALGO_AES128,
};
enum ca_descr_cipher_mode {
	CA_MODE_ECB,
	CA_MODE_CBC,
};
#define CW_SINGLE	 	0
#define CW_MULTIPLE	1
#define MAX_EXAUDIO	4
typedef struct ca_exdescr {
	unsigned int  	index;
	unsigned int  	parity;	/* 0 == even, 1 == odd */
	unsigned int	type;
	unsigned int	algo;
	unsigned int	cipher;
	unsigned char 	cw[8]; /* use video */
	unsigned char	a0[8];
	unsigned char	a1[8];
	unsigned char	a2[8];
	unsigned char	a3[8];
	unsigned char	data[8];
	unsigned char	vbi [8];
} ca_exdescr_t;

#define DMX_START		 	_IO('o', 41)
#define DMX_STOP		 	_IO('o', 42)
#define DMX_SET_FILTER	_IOW('o',43, struct dmx_sct_filter_params)

#define CA_SET_DESCR		_IOW('o',134, ca_descr_t)
#define CA_SET_PID		_IOW('o',135, ca_pid_t)
// sky(powervu)
#define CA_SET_EXDESCR 	_IOW('o',140, ca_exdescr_t)
// --------------------------------------------------------------------

void 		dvbapi_stop_descrambling(int);
void 		dvbapi_stop_all_descrambling(void);
void 		dvbapi_process_input(int32_t demux_id, int32_t filter_num, uchar *buffer, int32_t len);
int32_t 	dvbapi_open_device(int32_t, int32_t, int);
int32_t 	dvbapi_stop_filternum(int32_t demux_index, int32_t num);
int32_t 	dvbapi_stop_filter(int32_t demux_index, int32_t type);
void 		dvbapi_send_dcw(struct s_client *client, ECM_REQUEST *er);
void 		dvbapi_write_cw(int32_t demux_id, uchar *cw, CWEXTENTION *cwEx, int32_t idx, int cwdesalgo); // sky(powervu)
int32_t 	dvbapi_parse_capmt(unsigned char *buffer, uint32_t length, int32_t connfd, char *pmtfile);
void 		dvbapi_request_cw(struct s_client *client, ECM_REQUEST *er, int32_t demux_id, int32_t num, uint8_t delayed_ecm_check);
void 		dvbapi_try_next_caid(int32_t demux_id, int8_t checked, int32_t atfirst);
// sky(n,chknum)
void 		dvbapi_try_goto_caid(int32_t demux_id, int8_t checked, int32_t chknum);
void 		dvbapi_try_stop_caid(int32_t demux_id, int8_t checked);

void 		dvbapi_read_priority(void);
int32_t 	dvbapi_set_section_filter(int32_t demux_index, ECM_REQUEST *er, int32_t n);
int32_t  dvbapi_activate_section_filter(int32_t demux_index, int32_t num, int32_t fd, int32_t pid, uchar *filter, uchar *mask);
int32_t 	dvbapi_check_ecm_delayed_delivery(int32_t demux_index, ECM_REQUEST *er);
int32_t 	dvbapi_get_filternum(int32_t demux_index, ECM_REQUEST *er, int32_t type);
int32_t 	dvbapi_ca_setpid(int32_t demux_index, int32_t pid);
void 		dvbapi_set_pid(int32_t demux_id, int32_t num, int32_t idx, bool enable);

int8_t 	update_streampid_list(uint8_t cadevice, uint16_t pid, int32_t idx);
int8_t 	remove_streampid_from_list(uint8_t cadevice, uint16_t pid, int32_t idx);
void 		disable_unused_streampids(int16_t demux_id);
int8_t 	is_ca_used(uint8_t cadevice, int32_t pid);
const char *dvbapi_get_client_name(void);
void 		rotate_emmfilter(int32_t demux_id);
uint16_t dvbapi_get_client_proto_version(void);
void 		check_add_emmpid(int32_t demux_index, uchar *filter, int32_t l, int32_t emmtype);

int32_t 	dvbapi_constcw_afterwards(int32_t typ, int32_t demux_id, int16_t direction);

#if defined(__DVCCRC_AVAILABLE__)
bool		DVBCRC_Addon(struct s_ecmpids *stpids, uint8_t *ecm, uint16_t ecmlen, uint32_t *crcval);
void 		DVBCRC_Remove(int32_t demux_id, int32_t ecmpid);
void 		DVBCRC_Clean(struct s_ecmpids *curpid);
#if defined(MODULE_XCAMD)
int32_t 	DVBCRC_ChkMatched(struct s_reader *rdr, int32_t demux_id, uint16_t caid, uint32_t prid, uint32_t chkcrc);
#endif
#endif

#if defined(MODULE_CONSTCW) || defined(MODULE_XCAS) || defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
bool		DVBICS_ChRetuning(int32_t demux_id, int32_t typ);
void		DVBICS_ChSettings(int32_t demux_id, int chstages);
void		DVBICS_ChCloser(int32_t demux_id);
void 		DVBICS_ChDescrambled(int32_t demux_id, int descramble);
int32_t 	DVBICS_ChkEcmReaders(int32_t demux_id);
int32_t	DVBICS_ChkEcmPids(int32_t demux_id, uint16_t srvid, uint16_t casid, uint32_t prid);
#endif // defined(MODULE_XCAMD) || defined(MODULE_MORECAM)

#ifdef DVBAPI_LOG_PREFIX
#undef 	cs_log
#define 	cs_log(fmt, params...)   cs_log_txt(MODULE_LOG_PREFIX, "dvbapi: " fmt, ##params)
#ifdef WITH_DEBUG
	#undef	cs_log_dbg
	#define 	cs_log_dbg(mask, fmt, params...) do { if (config_enabled(WITH_DEBUG) && ((mask) & cs_dblevel)) cs_log_txt(MODULE_LOG_PREFIX, "dvbapi: " fmt, ##params); } while(0)
#endif
#endif

bool   is_dvbapi_usr(char *usr);
static inline bool module_dvbapi_enabled(void) { return cfg.dvbapi_enabled; }
#else
static inline void dvbapi_stop_all_descrambling(void) { }
static inline void dvbapi_read_priority(void) { }
static inline bool is_dvbapi_usr(char *UNUSED(usr)) { return 0; }
static inline bool module_dvbapi_enabled(void) { return 0; }
#endif // WITH_DVBAPI

#endif // MODULE_DVBAPI_H_
