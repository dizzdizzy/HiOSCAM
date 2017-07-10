#ifndef MODULE_CONSTCW_H_
#define MODULE_CONSTCW_H_
#if defined(MODULE_CONSTCW)
#define MAX_FTABISS	256

typedef struct
{
	uint16_t caid;
	uint32_t pfid;
	uint16_t srvid;
	uint16_t vidpid;
	uint16_t edgrid;
} FTA_BISTAB;

bool CONSTCW_IsFtaBisses(struct s_reader *reader, uint16_t ccaid, uint32_t cpfid, uint16_t csrvid, uint16_t cvidpid, uint16_t cdgrid);
#endif	// defined(MODULE_CONSTCW)
#endif	// #ifndef MODULE_CONSTCW_H_

