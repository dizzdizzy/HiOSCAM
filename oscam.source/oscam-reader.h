#ifndef _OSCAM_READER_H_
#define _OSCAM_READER_H_
//
//
//
// sky(n)
#if defined(WITH_HISILICON)
#define CS_INFORMATIONDIR		"/var/"
#else
#define CS_INFORMATIONDIR		"/tmp/"
#endif
#define cs_SMCINFORMATION		"oscam.smartcard"
#define cs_XCAMDSERVICES		"oscam.ibservices"
#define cs_MORECAMSERVICES		"oscam.imservices"
#define cs_CASERVICES			"oscam.caservices"
#define cs_OSCAMSERVER 			"oscam.server"


#define IS_SOCKET_OK(s)			((s)> 0)
#define IS_SOCKET_FAIL(s)		((s)<=0)
#define IS_SOCKET_ERRORNO(e)	((errno)==(e))

#define INET_TIMEOUT 			-1
#define INET_DISCONNECTED 		-2
#define IS_INET_OK(nstatus)	((nstatus)> 0)
//
//
//
const struct s_cardsystem *get_cardsystem_by_caid(uint16_t caid);
struct s_reader *get_reader_by_label(char *lbl);
const char *reader_get_type_desc(struct s_reader *rdr, int32_t extended);
// sky(n)
bool 		chk_reader_devices(struct s_reader *rdr);
void 		chk_group_violation(struct s_reader *rdr);
struct s_reader *chk_sole_reader_protocols(char *protocols);

bool 		hexserialset(struct s_reader *rdr);
void 		hexserial_to_newcamd(uchar *source, uchar *dest, uint16_t caid);
void 		newcamd_to_hexserial(uchar *source, uchar *dest, uint16_t caid);

S_ENTITLEMENT *cs_add_entitlement(struct s_reader *rdr, uint16_t caid, uint32_t provid, uint64_t id, uint32_t class, time_t start, time_t end, uint8_t type, char *comments, uint8_t add);
void 		cs_clear_entitlement(struct s_reader *rdr);
// sky(oscam.smartcard)
void 		cs_clean_cardinformation(void);
int  		cs_save_cardinformation (struct s_reader *rdr);

int32_t 	hostResolve(struct s_reader *reader);
int32_t 	network_tcp_connection_open(struct s_reader *);
void    	network_tcp_connection_close(struct s_reader *, char *);
// sky(Add)
int32_t	network_chk_intefaces(uint32_t *ip, uint8_t *mac);
int32_t 	network_tcp_socket_open(struct s_reader *rdr, char *devices, int32_t rport);
void 		network_tcp_socket_close(struct s_reader *rdr, int32_t sockfd);

void 		clear_block_delay(struct s_reader *rdr);
void    	block_connect(struct s_reader *rdr);
int32_t 	is_connect_blocked(struct s_reader *rdr);

void 		reader_do_idle(struct s_reader *reader);
void 		casc_check_dcw(struct s_reader *reader, int32_t idx, int32_t cwfound, uchar *cw, CWEXTENTION *cwEx);
int32_t 	casc_process_ecm(struct s_reader *reader, ECM_REQUEST *er);
void 		reader_do_card_info(struct s_reader *reader);
int32_t  reader_slots_available(struct s_reader * reader, ECM_REQUEST *er);

void 		cs_card_info(void);
int32_t 	reader_init(struct s_reader *reader);
void 		remove_reader_from_active(struct s_reader *rdr);
int32_t 	restart_cardreader(struct s_reader *rdr, int32_t restart);
void 		init_cardreader(void);
void 		kill_all_readers(void);
//
// sky(n)
uint16_t	CSREADER_Chkstatus(struct s_reader *rdr);
void		CSREADER_Restart(int32_t typ);
struct s_reader *CSREADER_GetReaders(int32_t typ);
bool		CSREADER_IsFTAbiss(uint16_t degree, uint32_t frequency, uint16_t srvid, uint16_t vidpid);

#if defined(MODULE_CONSTCW) || defined(MODULE_XCAS) || defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
#define ERE_CS_BYPASS	1
#define ERE_CS_FIRST		2
#define ERE_CS_FINAL		254

enum
{
	CHCS_CASIDLE		= 0,
	CHCS_CASFIRST		= 0x1,
	CHCS_CASFOWARD		= 0x2,
	CHCS_CAS2FAKE		= 0x4,
	CHCS_CASRETUNE		= 0x8
};

struct s_reader *CSREADER_ChkActive(int32_t typ);
int32_t	CSREADER_ChkEcmSkipto(struct s_reader *rdr, ECM_REQUEST *er);
int32_t	CSREADER_ChkEcmNextto(struct s_reader *rdr, ECM_REQUEST *er);
int32_t	CSREADER_ChSetting (int dmuxid, struct demux_s *demux, int32_t typ, int chstages);
void		CSREADER_ChCloser	 (int dmuxid, struct demux_s *demux, int32_t typ);
bool		CSREADER_ChRetuning(int dmuxid, struct demux_s *demux, int32_t typ);
void		CSREADER_ChDescrambled(int32_t typ, int descramble);
#endif // defined(MODULE_XCAMD) || defined(MODULE_MORECAM)

#endif

