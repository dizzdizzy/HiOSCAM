#include "globals.h"
#if defined(MODULE_XCAS)
#include "oscam-array.h"
#include "oscam-conf.h"
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-net.h"
#include "oscam-chk.h"
#include "oscam-string.h"
#include "module-xcas.h"

// oscam.keys(=SoftCam.Key)
#define 	MAXSCAMLINESIZE 	4096
#define	MAXCOLSEPERATES	4


void
emukey_colon2space(char *str)
{
	char *p1 = str;

	if (NULL != p1)
	{
		while ('\0' != *p1) {
			if (':' == *p1) *p1 = ' ';
			p1++;
		}
	}
}

int
emukey_line2useless(char *str)
{
	if (str    ==NULL) return 1;
	if (str[0] == 0x0) return 1;
	if (str[0] == '#') return 1;
	if (str[0] == ';') return 1;
	if (str[0] == ':') return 1;
	if (str[0] == ' ') return 1;
	if (str[0] == 0xA) return 1;
	if (str[0] == 0xD) return 1;
	return 0;
}

static bool
emukey_RdrChkDevices(struct s_reader *rdr)
{
	FILE *fp;

	if (!rdr) return 0;
//	fp = fopen(rdr->device, "r");
	fp = open_config_file(rdr->device);
	if (!fp) return (0);
	fclose(fp);
	return 1;
}


static int
emukey_RdrCasMakeup(struct s_reader *rdr)
{
	FILE 	*fp;
	char 	*token, *seperates, *coments;
	char	*tmp;
	int32_t  lenn;
	uint16_t ucaid;
	uint32_t uppid;
	char	sscas [64];
	char	ssprov[64];
	char	ssknr [64];
	char	sskeys[1024];

//	fp = fopen(rdr->device, "r");
	fp = open_config_file(rdr->device);
	if (!fp) return 0;
	if (!cs_malloc(&token, MAXSCAMLINESIZE))
	{
		fclose(fp);
		return 0;
	}

	int i,k;
	int numfilts = 0;
	int numcaids = 0;
	CAIDTAB_DATA xcts[CS_MAXCAIDTAB];
	FILTER xfilts[CS_MAXFILTERS];

	memset(&xcts, 0, sizeof(xcts));
	memset(&xfilts, 0, sizeof(xfilts));
	for (i=0; i<CS_MAXCAIDTAB; xcts[i++].mask = 0xffff);

	#if defined(__XCAS_BISS__)
		xcts[numcaids++].caid = 0x2600;
		xfilts[numfilts].caid = 0x2600;
		numfilts++;
	#endif
	#if defined(__XCAS_VIACESS__)
		xcts[numcaids++].caid = 0x0500;
		xfilts[numfilts].caid = 0x0500;
		xfilts[numfilts].prids[0] = 0x030B00;
		xfilts[numfilts].nprids   = 1;
		numfilts++;
	#endif
	// sky(2016.03.07)
	#if defined(__XCAS_POWERVU__)
		xcts[numcaids++].caid = 0x0E00;
		xfilts[numfilts].caid = 0x0E00;
		numfilts++;
	#endif

	while (fgets(token, MAXSCAMLINESIZE, fp))
	{
		tmp = trim(token);
		if (emukey_line2useless(tmp)) continue;
		if ((lenn=strlen(tmp)) < 9) continue;
		coments=strchr(token, ';');
		if (coments) *coments = 0;


		int spacing;
		seperates=token;
		for (spacing=0; (spacing<MAXCOLSEPERATES) && (seperates); spacing++)
		{
			seperates=strchr(seperates, ' ');
			if (!seperates) break;
			while (*seperates == ' ') seperates++;
		}
		if (spacing==MAXCOLSEPERATES && seperates) *seperates = 0;


		if (strchr(token, ':'))
		{
		}
		else
		{
			sscanf(token, "%s %s %s %s", sscas, ssprov, ssknr, sskeys);
		//	MYEMU_TRACE("xcaskey:%s,%s,%d,%s,%s\n", sscas, ssprov, uppid, ssknr, sskeys);
			uppid = a2i(ssprov,8);
			ucaid = 0;
			if (sscas[0] == CASS_VIACCESS)   { ucaid = 0x500; 		}
			if (sscas[0] == CASS_SEKA)       { ucaid = 0x100; 		}
//			if (sscas[0] == CASS_CRYPTOWORKS){ ucaid = uppid>>8; uppid &= 0xfc; }
			if (sscas[0] == CASS_CRYPTOWORKS){ ucaid = uppid>>8;	}
			if (sscas[0] == CASS_NAGRA)      { ucaid = 0x1800; 	}
			if (sscas[0] == CASS_POWERVU)    { ucaid = 0x0E00; uppid = 0x0000; }
			if (sscas[0] == CASS_BISS)       { uppid = 0x0000; }
			if (sscas[0] == CASS_CONSTANT)   { uppid = 0x0000; }
			if (!XEMUKEY_IsAvailable(rdr, ucaid, uppid)) continue;
			//-------------------------------------------------------------
			//-------------------------------------------------------------
			for (i=0; i<numcaids; i++)
			{
				if (xcts[i].caid == ucaid) break;
			}
			if (i==numcaids)
			{
				if (numcaids>(CS_MAXCAIDTAB-1))
				{
					mycs_debug(D_ADB, "xcaskey:CA{%04X} CS_MAXCAIDTAB exceed\n", ucaid);
					break;
				}
				xcts[numcaids++].caid = ucaid;
			}
			//-------------------------------------------------------------
			//-------------------------------------------------------------
			for (i=0; i<numfilts; i++)
			{
				if (xfilts[i].caid == ucaid) break;
			}
			if (i==numfilts)
			{
			//	if (i>(CS_MAXFILTERS-1)) break;
				if (i>(CS_MAXFILTERS-1))
				{
					mycs_debug(D_ADB, "xcaskey:CA{%04X.%08X} CS_MAXFILTERS exceed\n", ucaid, uppid);
					break;
				}
				xfilts[i].caid = ucaid;
				xfilts[i].prids[0] = uppid;
				xfilts[i].nprids = 1;
				numfilts++;
				myprintf("xcaskey:CA{%3d: %04X.%08X}\n", xfilts[i].nprids, ucaid, uppid);
			}
			else
			{
				for (k=0; k<xfilts[i].nprids; k++)
				{
					if (xfilts[i].prids[k] == uppid) break;
				}
				if (k==xfilts[i].nprids)
				{
				//	if (k>(CS_MAXPRFLT-1)) continue;
					if (k>(CS_MAXPRFLT-1))
					{
						mycs_debug(D_ADB, "xcaskey:CA{%04X.%08X} CS_MAXPRFLT exceed\n", ucaid, uppid);
						continue;
					}
					myprintf("xcaskey:CA{%3d: %04X.%08X}\n", xfilts[i].nprids, ucaid, uppid);
					xfilts[i].prids[k] = uppid;
					xfilts[i].nprids++;
				}
			}
		}
	}
	for (i=0; i<numfilts; i++) {
		ftab_add(&rdr->ftab, &xfilts[i]);
	}
	for (i=0; i<numcaids; i++) {
		caidtab_add(&rdr->ctab, &xcts[i]);
	}

	cs_free(token);
	fclose(fp);
	return (numfilts);
}


static int
emukey_RdrFindkey(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	char 	   *kNstr,
	uint8_t  *kBufs,
	uint16_t  ksize,
	int 		*pfIndex,
	bool	   bMultiple)
{
	FILE 	*fp;
	char 	*token, *seperates, *coments;
	char	*tmp;
	int32_t  lenn;
	uint32_t uppid;
	char	sscas [64];
	char	ssprov[64];
	char	ssknr [64];
	char	sskeys[1024];
	int	counter = 0;
	int 	spacing;
	int 	keyfound = 0;

//	fp = fopen(rdr->device, "r");
	fp = open_config_file(rdr->device);
	if (!fp) return 0;
	if (!cs_malloc(&token, MAXSCAMLINESIZE))
	{
		fclose(fp);
		return 0;
	}

	while (fgets(token, MAXSCAMLINESIZE, fp))
	{
		if (keyfound) break;
		tmp = trim(token);
		if (emukey_line2useless(tmp)) continue;
		if ((lenn=strlen(tmp)) < 9) continue;
		coments = strchr(token, ';');
		if (coments) *coments = 0;


		seperates=token;
		for (spacing=0; (spacing<MAXCOLSEPERATES) && (seperates); spacing++)
		{
			seperates=strchr(seperates, ' ');
			if (!seperates) break;
			while (*seperates == ' ') seperates++;
		}
		if (spacing==MAXCOLSEPERATES && seperates) *seperates = 0;


		if (strchr(token, ':'))	{ }
		else
		{
			sscanf(token, "%s %s %s %s", sscas, ssprov, ssknr, sskeys);
		//	MYEMU_TRACE("xcaskey:%s,%s,%d,%s,%s\n", sscas, ssprov, uppid, ssknr, sskeys);
			uppid = a2i(ssprov,8);
			if (cCAS != sscas[0]) continue;
			if (ppid != uppid) continue;
			if (kNstr)
			{
				if (strcmp(kNstr,ssknr)) continue;
			}
			if (kBufs)
			{
				if (cs_atob((uchar *)kBufs, sskeys, ksize) < 0) continue;
			//	MYEMU_TRACE("xcaskey:%5d.%5d:%02X...%02X\n",
			//			counter,
			//			(pfIndex) ? *pfIndex : 9999,
			//			kBufs[0], kBufs[ksize-1]);
			//	kBufs += ksize;
			}
			counter++;
			if (pfIndex) {
				if (*pfIndex < 0) keyfound = 1;
				else if (counter >= *pfIndex) keyfound = 1;
			}
			else {
				keyfound = 1;
				if (!bMultiple) break;
			}
			if (counter>(MAXMULTIKEY-1)) break;
		}
	}

	cs_free(token);
	fclose(fp);

	if (counter > 0 && pfIndex) *pfIndex = counter;
	return (keyfound);
}


static int
emukey_RdrPidsFindkey(struct s_reader *rdr, uint8_t cCAS, uint32_t *ppids, uint32_t ppnum,
	char 	   *kNstr,
	uint8_t  *kBufs,
	uint16_t  ksize,
	int 		*pfIndex,
	bool	   bMultiple)
{
	FILE 	*fp;
	char 	*token, *seperates, *coments;
	char	*tmp;
	int32_t  lenn;
	uint32_t uppid;
	char	sscas [64];
	char	ssprov[64];
	char	ssknr [64];
	char	sskeys[1024];
	int	counter = 0;
	int 	spacing;
	int 	keyfound = 0;
	int	i;

//	fp = fopen(rdr->device, "r");
	fp = open_config_file(rdr->device);
	if (!fp) return 0;
	if (!cs_malloc(&token, MAXSCAMLINESIZE))
	{
		fclose(fp);
		return 0;
	}

	while (fgets(token, MAXSCAMLINESIZE, fp))
	{
		if (keyfound) break;
		tmp = trim(token);
		if (emukey_line2useless(tmp)) continue;
		if ((lenn=strlen(tmp)) < 9) continue;
		coments = strchr(token, ';');
		if (coments) *coments = 0;


		seperates=token;
		for (spacing=0; (spacing<MAXCOLSEPERATES) && (seperates); spacing++)
		{
			seperates=strchr(seperates, ' ');
			if (!seperates) break;
			while (*seperates == ' ') seperates++;
		}
		if (spacing==MAXCOLSEPERATES && seperates) *seperates = 0;


		if (strchr(token, ':'))	{ }
		else
		{
			sscanf(token, "%s %s %s %s", sscas, ssprov, ssknr, sskeys);
		//	MYEMU_TRACE("xcaskey:%s,%s,%d,%s,%s\n", sscas, ssprov, uppid, ssknr, sskeys);
			uppid = a2i(ssprov,8);
			if (cCAS != sscas[0]) continue;
			for (i=0; i<(int)ppnum; i++) {
				if (ppids[i] == uppid) break;
			}
			if (i >= (int)ppnum) continue;

			if (kNstr)
			{
				if (strcmp(kNstr,ssknr)) continue;
			}
			if (kBufs)
			{
				if (cs_atob((uchar *)kBufs, sskeys, ksize) < 0) continue;
			//	MYEMU_TRACE("xcaskey:%5d.%5d:%02X...%02X\n",
			//			counter,
			//			(pfIndex) ? *pfIndex : 9999,
			//			kBufs[0], kBufs[ksize-1]);
			//	kBufs += ksize;
			}
			counter++;
			if (pfIndex) {
				if (*pfIndex < 0) keyfound = 1;
				else if (counter >= *pfIndex) keyfound = 1;
			}
			else {
				keyfound = 1;
				if (!bMultiple) break;
			}
			if (counter>(MAXMULTIKEY-1)) break;
		}
	}

	cs_free(token);
	fclose(fp);

	if (counter > 0 && pfIndex) *pfIndex = counter;
	return (keyfound);
}


static int
emukey_RdrChLfsFindkey(struct s_reader *rdr, uint8_t cCAS, uint32_t ufrequency,
	uint32_t ufsymrate,
	uint8_t *kBufs,
	uint16_t ksize,
	bool	   bMultiple)
{
	FILE 	*fp;
	char 	*token, *seperates, *coments;
	char 	*tmp;
	int32_t  lenn;
	char	sscas [64];
	char	sskeys[1024];
	int	counter = 0;

//	fp = fopen(rdr->device, "r");
	fp = open_config_file(rdr->device);
	if (!fp) return 0;
	if (!cs_malloc(&token, MAXSCAMLINESIZE))
	{
		fclose(fp);
		return 0;
	}

	while (fgets(token, MAXSCAMLINESIZE, fp))
	{
		tmp = trim(token);
		if (emukey_line2useless(tmp)) continue;
		if ((lenn=strlen(tmp)) < 9) continue;
		coments=strchr(token, ';');
		if (coments) *coments = 0;


		int	spacing;
		seperates=token;
		for (spacing=0; (spacing<MAXCOLSEPERATES) && (seperates); spacing++)
		{
			seperates=strchr(seperates, ' ');
			if (!seperates) break;
			while (*seperates == ' ') seperates++;
		}
		if (spacing==MAXCOLSEPERATES && seperates) *seperates = 0;


		if (strchr(token, ':'))
		{
		//	char ssfreq[64];
		//	char ssfsym[64];
			char ssfpol[64];
			uint32_t usfreq;
			uint32_t usfsym;

		//	emukey_colon2space(token);
		//	sscanf(token, "%s %s %s %s %s", sscas, ssfreq, ssfsym, ssfpol, sskeys);
		//	MYEMU_TRACE("xcaskey:%s,%s,%s,%s,%s\n", sscas, ssfreq, ssfsym, ssfpol, sskeys);

			sscanf(token, "%s %d:%d:%s %s", sscas, &usfreq, &usfsym, ssfpol, sskeys);
		//	MYEMU_TRACE("xcaskey:%s,%d,%d,%s,%s\n", sscas, usfreq, usfsym, ssfpol, sskeys);
			if (cCAS  != sscas[0]) continue;
			if (ufrequency && ufrequency != usfreq) continue;
			if (ufsymrate && ufsymrate != usfsym) continue;
			if (kBufs)
			{
				if (cs_atob((uchar *)kBufs, sskeys, ksize) < 0) continue;
				MYEMU_TRACE("xcaskey:%5d:%02X...%02X\n", counter, kBufs[0], kBufs[ksize-1]);
			//	kBufs += ksize;
			}
			counter++;
			if (!bMultiple) break;
			if (counter>(MAXMULTIKEY-1)) break;
		}
	}

	cs_free(token);
	fclose(fp);
	return (counter);
}


static int
emukey_RdrSavekey(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	char 	   *kNstr,
	uint8_t  *kBufs,
	uint16_t  ksize)
{
	FILE 	*fp;
	char 	*token, *seperates, *coments;
	char	*tmp;
	int32_t  lenn;
	uint32_t uppid;
	char	sscas [64];
	char	ssprov[64];
	char	ssknr [64];
	char	sskeys[1024];
	int 	spacing;
	int 	keyfound = 0;

	fp = create_config_file(rdr->device);
	if (!fp) return 0;
	if (!cs_malloc(&token, MAXSCAMLINESIZE))
	{
		fclose(fp);
		return 0;
	}

	while (fgets(token, MAXSCAMLINESIZE, fp))
	{
		if (keyfound) {
			fputs(token, fp);
			continue;
		}
		tmp = trim(token);
		if (emukey_line2useless(tmp)) {
			fputs(token, fp);
			continue;
		}
		if ((lenn=strlen(tmp)) < 9) {
			fputs(token, fp);
			continue;
		}
		coments = strchr(token, ';');
		if (coments) *coments = 0;


		seperates=token;
		for (spacing=0; (spacing<MAXCOLSEPERATES) && (seperates); spacing++)
		{
			seperates=strchr(seperates, ' ');
			if (!seperates) break;
			while (*seperates == ' ') seperates++;
		}
		if (spacing==MAXCOLSEPERATES && seperates) *seperates = 0;

		if (strchr(token, ':'))	{
			fputs(token, fp);
			continue;
		}

		sscanf(token, "%s %s %s %s", sscas, ssprov, ssknr, sskeys);
		uppid = a2i(ssprov,8);
		if (cCAS != sscas[0] || ppid != uppid || strcmp(kNstr,ssknr)) {
			fputs(token, fp);
			continue;
		}

		char_to_hex((uchar *)kBufs, ksize, (unsigned char *)sskeys);
		MYEMU_TRACE("xcaskey:update:%s,%s,%d,%s,%s\n", sscas, ssprov, uppid, ssknr, sskeys);
		fprintf(fp, "%s %s %s %s", sscas, ssprov, ssknr, sskeys);
		keyfound = 1;
	}

	if (!keyfound) {
		char_to_hex((uchar *)kBufs, ksize, (unsigned char *)sskeys);
		MYEMU_TRACE("xcaskey:add:%c,%08X,%s,%s\n", cCAS, ppid, kNstr, sskeys);
		fprintf(fp, "%c %08X %s %s", cCAS, ppid, kNstr, sskeys);
	}

	cs_free(token);
	fclose(fp);
	return (keyfound);
}

bool
XEMUKEY_IsExistance(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid)
{
	int existance;

	if (!rdr) return 0;
	existance = emukey_RdrFindkey(rdr, cCAS, ppid, NULL, NULL, 0, NULL, false);
	if (existance)
	{
		MYEMU_TRACE("xcaskey:{%c}{%06X}\n", cCAS, ppid);
	}
	return (existance);
}


int
XEMUKEY_ChLfsSearch(struct s_reader *rdr, uint8_t cCAS, uint32_t ufrequency,
	uint32_t ufsymrate,
	uint8_t *kBufs,
	uint16_t ksize)
{
	int counter;

	if (!rdr) return 0;
	if (!kBufs) return 0;
	MYEMU_TRACE("xcaskey:lfssearch{%c}{%05d}(%d)\n", cCAS, ufrequency, ksize);
	counter = emukey_RdrChLfsFindkey(rdr, cCAS, ufrequency, ufsymrate, kBufs, ksize, true);
	if (counter)
	{
		MYEMU_TRACE("xcaskey:lfs.key{%02X...%02X}{%d}\n", kBufs[0], kBufs[ksize-1], counter);
	}
	else
	{
		MYEMU_TRACE("xcaskey:lfs.key none\n");
	}
	return (counter);
}


int
XEMUKEY_SpecialSearch(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	char    *kNstr,
	uint8_t *kBufs,
	uint16_t ksize)
{
	int counter;

	if (!rdr) return 0;
	if (!kBufs) return 0;
	MYEMU_TRACE("xcaskey:specialsearch{%c}{%06X.%s}(%d)\n", cCAS, ppid, kNstr, ksize);
	counter = emukey_RdrFindkey(rdr, cCAS, ppid, kNstr, kBufs, ksize, NULL, false);
	if (counter)
	{
		MYEMU_TRACE("xcaskey:special.key{%02X...%02X}\n", kBufs[0], kBufs[ksize-1]);
	}
	else
	{
		MYEMU_TRACE("xcaskey:special.key none\n");
	}
	return (counter);
}


int
XEMUKEY_MultiSearch(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	uint8_t	kNr,
	uint8_t *kBufs,
	uint16_t ksize)
{
	int	counter;
	char 	cKstr[3];

	if (!rdr) return 0;
	if (!kBufs) return 0;
	sprintf(cKstr, "%02X", kNr);
	MYEMU_TRACE("xcaskey:multisearch{%c}{%08X.%s}(%d)\n", cCAS, ppid, cKstr, ksize);
	counter = emukey_RdrFindkey(rdr, cCAS, ppid, cKstr, kBufs, ksize, NULL, true);
	if (counter)
	{
		MYEMU_TRACE("xcaskey:multi.key{%02X...%02X}{%d}\n", kBufs[0], kBufs[ksize-1], counter);
	}
	else
	{
		MYEMU_TRACE("xcaskey:multi.key.none\n");
	}
	return (counter);
}


int
XEMUKEY_Searchkey(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	uint8_t	kNr,
	uint8_t *kBufs,
	uint16_t ksize)
{
	char 	cKstr[3];
	int	counter;

	if (!rdr) return 0;
	if (!kBufs) return 0;
	sprintf(cKstr, "%02X", kNr);
	MYEMU_TRACE("xcaskey:search{%c}{%06X.%s, %3d}\n", cCAS, ppid, cKstr, ksize);
	counter = emukey_RdrFindkey(rdr, cCAS, ppid, cKstr, kBufs, ksize, NULL, false);
	if (counter)
	{
		MYEMU_TRACE("xcaskey:key{%02X...%02X}{%d}\n", kBufs[0], kBufs[7], counter);
	}
	else
	{
		MYEMU_TRACE("xcaskey:key none\n");
	}
	return (counter);
}


int
XEMUKEY_IndexedSearch(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	uint8_t	kNr,
	uint8_t *kBufs,
	uint16_t ksize,
	int 	  *pFoundIndex)
{
	char 	cKstr[3];
	int	counter;

	if (!rdr) return 0;
	if (!kBufs) return 0;
	sprintf(cKstr, "%02X", kNr);
	MYEMU_TRACE("xcaskey:search{%c}{%06X.%s, %3d}{%d}\n", cCAS, ppid, cKstr, ksize, (pFoundIndex) ? *pFoundIndex : 0);
	counter = emukey_RdrFindkey(rdr, cCAS, ppid, cKstr, kBufs, ksize, pFoundIndex, false);
	if (counter)
	{
		MYEMU_TRACE("xcaskey:key{%02X...%02X}{%d}\n", kBufs[0], kBufs[7], counter);
	}
	else
	{
		MYEMU_TRACE("xcaskey:key none\n");
	}
	return (counter);
}


int
XEMUKEY_PidsSearch(struct s_reader *rdr, uint8_t cCAS, uint32_t *ppids, uint32_t ppnum,
	uint8_t	kNr,
	uint8_t *kBufs,
	uint16_t ksize,
	int 	  *pFoundIndex)
{
	char 	cKstr[3];
	int	counter;

	if (!rdr) return 0;
	if (!kBufs) return 0;
	if (!ppids) return 0;
	if (!ppnum) return 0;
	sprintf(cKstr, "%02X", kNr);
	MYEMU_TRACE("xcaskey:search{%c}{%06X.%s, %3d}{%d}\n", cCAS, ppids[0], cKstr, ksize, (pFoundIndex) ? *pFoundIndex : 0);
	counter = emukey_RdrPidsFindkey(rdr, cCAS, ppids, ppnum, cKstr, kBufs, ksize, pFoundIndex, false);
	if (counter)
	{
		MYEMU_TRACE("xcaskey:key{%02X...%02X}{%d}\n", kBufs[0], kBufs[7], counter);
	}
	else
	{
		MYEMU_TRACE("xcaskey:key none\n");
	}
	return (counter);
}


int
XEMUKEY_Savekey(struct s_reader *rdr, uint8_t cCAS, uint32_t ppid,
	char 	  *kNstr,
	uint8_t *kBufs,
	uint16_t ksize)
{
	if (!rdr) return 0;
	if (!kNstr) return 0;
	if (!kBufs) return 0;
	MYEMU_TRACE("xcaskey:save{%c}{%06X.%s, %3d}\n", cCAS, ppid, kNstr, ksize);
	emukey_RdrSavekey(rdr, cCAS, ppid, kNstr, kBufs, ksize);
	return 1;
}


int
XEMUKEY_IsAvailable(struct s_reader *rdr, uint16_t casid, uint32_t prid)
{
	if (!casid) return 0;
	#if defined(__XCAS_BISS__)
		if (IS_BISS(casid)) return 1;
	#endif
	#if defined(__XCAS_CRYPTOWORKS__)
		if (IS_CRYPTOWORKS(casid)) {
			if (!prid) return 1;	// futures
			return 1;
		}
	#endif
	#if defined(__XCAS_VIACESS__)
		if (IS_VIACESS(casid)) {
			if (!prid) return 0;	// futures
			return 1;
		}
	#endif
	#if defined(__XCAS_SEKA__)
		if (IS_SEKA(casid)) {
			if (!prid) return 0;	// futures
			return 1;
		}
	#endif
	#if defined(__XCAS_POWERVU__)
		if (IS_POWERVU(casid)) return 1;
	#endif
	return 0;
}
// sky(powervu)
void
XEMUKEY_Cleanup(struct s_reader *rdr)
{
	memset(rdr->powervu_ecmnb,0,sizeof(rdr->powervu_ecmnb));
}

int
XEMUKEY_Initialize(struct s_reader *rdr)
{
	if (!emukey_RdrChkDevices(rdr)) return 0;
	#if defined(__XCAS_BISS__)
		XBISS_Cleanup();
	#endif
	#if defined(__XCAS_VIACESS__)
		XVIACESS_Cleanup();
	#endif
	#if defined(__XCAS_SEKA__)
		XSEKA_Cleanup();
	#endif
	#if defined(__XCAS_CRYPTOWORKS__)
		XCRYPTOWORKS_Cleanup();
	#endif
	#if defined(__XCAS_POWERVU__)
		XPVU_Cleanup(rdr);
	#endif

	caidtab_clear(&rdr->ctab);
	ftab_clear(&rdr->ftab);
	emukey_RdrCasMakeup(rdr);
	return 1;
}

#endif	// defined(MODULE_XCAS)

