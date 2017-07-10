#include "globals.h"
#ifdef READER_DRE
#include "cscrypt/des.h"
#include "reader-common.h"
#include "oscam-string.h"

struct dre_data {
	uint8_t	version;
	uint8_t	prcode;
	uint8_t	provider;
};

#define OK_RESPONSE 	0x61
#define CMD_BYTE 		0x59

static uchar dre_xor(const uchar *cmd, int32_t cmdlen)
{
	int32_t i;
	uchar checksum = 0x00;
	for (i = 0; i < cmdlen; i++)
		checksum ^= cmd[i];
	return checksum;
}
// attention: inputcommand will be changed!!!! answer will be in cta_res, length cta_lr ; returning 1 = no error, return ERROR = err
static int32_t dre_transmission(struct s_reader *reader, const uchar *cmd, int32_t cmdlen, unsigned char *cta_res, uint16_t *p_cta_lr, unsigned char *startcmd)
{
	uchar starthdrs[5];
//	uchar startcmd [] = { 0x80,0xFF,0x10,0x01,0x05 };	// any command starts with this,
//	uchar startcmd3[] = { 0x80,0xFF,0x01,0x01,0x05 };
	// 80 00 00 00 05/80 00 01 01 05
	// last byte is nr of bytes of the command that will be sent
	// after the startcmd
	// response on startcmd+cmd: = { 0x61,0x05 }  // 0x61 = "OK", last byte is nr. of bytes card will send
	uchar reqanswer[] = { 0x00,0xC0,0x00,0x00,0x08 };	// after command answer has to be requested,
	// last byte must be nr. of bytes that card has reported to send
	uchar command[256];
	char  tmp[256];
	int32_t headerlen = sizeof(starthdrs);

	memcpy(starthdrs, startcmd, headerlen);
	starthdrs[4] = cmdlen + 3;	//commandlength + type + len + checksum bytes
	memcpy(command, starthdrs, headerlen);
	command[headerlen++] = CMD_BYTE;	//type
	command[headerlen++] = cmdlen + 1;	//len = command + 1 checksum byte
	memcpy(command + headerlen, cmd, cmdlen);

	uchar checksum = ~(dre_xor(cmd, cmdlen));
//	rdr_log_dbg(reader, D_READER, "Checksum: %02x", checksum);
	cmdlen += headerlen;
	command[cmdlen++] = checksum;
	myprdump("drecommand", (void *)command, cmdlen);

	reader_cmd2icc(reader, command, cmdlen, cta_res, p_cta_lr);

	if ((*p_cta_lr != 2) || (cta_res[0] != OK_RESPONSE))
	{
		rdr_log(reader, "command sent to card: %s", cs_hexdump(0, command, cmdlen, tmp, sizeof(tmp)));
		rdr_log(reader, "unexpected answer from card: %s", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
		return ERROR;			//error
	}

	reqanswer[4] = cta_res[1];	//adapt length byte
	myprdump("reqanswer", (void *)reqanswer, 5);
	reader_cmd2icc(reader, reqanswer, 5, cta_res, p_cta_lr);

	if (cta_res[0] != CMD_BYTE) {
		rdr_log(reader, "unknown response: cta_res[0] expected to be %02x, is %02x", CMD_BYTE, cta_res[0]);
		return ERROR;
	}
	if ((cta_res[1] == 0x03) && (cta_res[2] == 0xe2))
	{
		switch (cta_res[3]) {
			case 0xe1:
				rdr_log(reader, "checksum error: %s.",  cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				myprintf("checksum error: %s.",  cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				break;
			case 0xe2:
				rdr_log(reader, "wrong provider: %s.",  cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				myprintf("wrong provider: %s.",  cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				break;
			case 0xe3:
				rdr_log(reader, "illegal command: %s.", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				myprintf("illegal command: %s.", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				break;
			case 0xec:
				rdr_log(reader, "wrong signature: %s.", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				myprintf("wrong signature: %s.", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				break;
			default:
				rdr_log_dbg(reader, D_READER, "unknown error: %s.", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				myrdr_debug(reader, D_READER, "unknown error: %s.", cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
				break;
		}
		return ERROR; //error
	}
	int32_t length_excl_leader = *p_cta_lr;
	if ((cta_res[*p_cta_lr - 2] == 0x90) && (cta_res[*p_cta_lr - 1] == 0x00))
		length_excl_leader -= 2;

	checksum = ~(dre_xor(cta_res + 2, length_excl_leader - 3));

	if (cta_res[length_excl_leader - 1] != checksum) {
		rdr_log(reader, "checksum does not match, expected %02x received %02x:%s", checksum,
					cta_res[length_excl_leader - 1], cs_hexdump(0, cta_res, *p_cta_lr, tmp, sizeof(tmp)));
		return ERROR;			//error
	}
	return OK;
}

#define dre_cmd(cmd) \
{ 	\
	uchar startcmd1[] = { 0x80,0xFF,0x10,0x01,0x05 }; \
	uchar startcmd3[] = { 0x80,0x00,0x00,0x00,0x05 }; \
	uchar *pstartcmd; \
	pstartcmd = (reader->DRE_3s || \
					(reader->DRE_type==TRICOLOR_PHMEDIA || \
					 reader->DRE_type==TRICOLOR_SYBERIA || \
					 reader->DRE_type==TRICOLOR_TVDRE3)) ? startcmd3 : startcmd1; \
  	dre_transmission(reader, cmd, sizeof(cmd),cta_res,&cta_lr,pstartcmd); \
}

#define dre3_cmds(cmd) \
{ 	\
	uchar startcmd3[] = { 0x80,0x00,0x01,0x01,0x05 }; \
  	dre_transmission(reader, cmd, sizeof(cmd),cta_res,&cta_lr,startcmd3); \
}

static int32_t dre_set_provider_info(struct s_reader *reader)
{
	def_resp;
	uchar cmd59[] = { 0x59,0x14 };	// get subscriptions
	uchar cmd5b[] = { 0x5b,0x00,0x14 };	// get validity dates
	struct dre_data *csystem_data = reader->csystem_data;
	int32_t i, oksucced = 0;

	cs_clear_entitlement(reader);

	cmd59[1] = csystem_data->provider;
	oksucced = (dre_cmd(cmd59));
	myascdump("drex pbmresp", cta_res, cta_lr);
	if (oksucced) // ask subscription packages, returns error on 0x11 card
	{
		uchar pbm[32];
		char  tmp_dbg[65];

		memcpy(pbm, cta_res+3, cta_lr-6);
		rdr_log_dbg(reader, D_READER, "pbm: %s", cs_hexdump(0, pbm, 32, tmp_dbg, sizeof(tmp_dbg)));

		if (pbm[0] == 0xff) {
			rdr_log(reader, "no active packages");
		}
		else {
			for (i = 0; i < 32; i++) {
				if (pbm[i] != 0xff) {
					cmd5b[1] = i;
					cmd5b[2] = csystem_data->provider;
					dre_cmd(cmd5b);	// ask for validity dates
					myascdump("drex packageresp", cta_res, cta_lr);

					time_t start, end;
					struct tm temp;

					start = (cta_res[3] << 24) | (cta_res[4] << 16) | (cta_res[5] << 8) | cta_res[6];
					end   = (cta_res[7] << 24) | (cta_res[8] << 16) | (cta_res[9] << 8) | cta_res[10];
					localtime_r(&start, &temp);
					int32_t startyear  = temp.tm_year + 1900;
					int32_t startmonth = temp.tm_mon + 1;
					int32_t startday   = temp.tm_mday;
					localtime_r(&end, &temp);
					int32_t endyear    = temp.tm_year + 1900;
					int32_t endmonth   = temp.tm_mon + 1;
					int32_t endday     = temp.tm_mday;
				// sky(date)
				//	rdr_log(reader, "active package %i valid from %04i/%02i/%02i to %04i/%02i/%02i",
				//				i,
				//				startyear, startmonth, startday,
				//	  			endyear, endmonth, endday);
					rdr_log(reader, "active package %i valid from %02i/%02i/%04i to %02i/%02i/%04i",
								i+1,
								startday, startmonth, startyear,
								endday, endmonth, endyear);
					// sky(!)
					cs_add_entitlement(reader, reader->caid, b2ll(4, reader->prid[0]), 0, 0, start, end, 1, NULL, 1);
				}
			}
		}
	}
	else {
		myrdr_log(reader, "dre packages failure.");
	}
	return OK;
}

// sky(DRE3)
static int32_t dre_set_dre3_providers(struct s_reader *reader)
{
	def_resp;
	uchar cmd84[] = { 0x84,0x00,0x40,0x02 }; // sky(DRE3)
	uchar cmd85[] = { 0x85,0x00,0x02 };
	uchar pbm[0x80];
	char  tmp_dbg[65];
	struct dre_data *csystem_data = reader->csystem_data;
	int32_t i, oksucced = 0;

	cs_clear_entitlement(reader);

	// 84 00 40 02/84 40 40 02
	cmd84[1] = 0x00;
	cmd84[2] = 0x5f;
	cmd84[3] = csystem_data->provider;
	oksucced = (dre_cmd(cmd84));
	myascdump("dre3 pbmresp(00)", cta_res, cta_lr);
	if (oksucced) // ask subscription packages, returns error on 0x11 card
	{
		memset(pbm, 0xff, sizeof(pbm));
		memcpy(pbm, cta_res+3, cta_lr-6);
		rdr_log_dbg(reader, D_READER, "pbm00: %s", cs_hexdump(0, pbm, 32, tmp_dbg, sizeof(tmp_dbg)));

		cmd84[1] = 0x5f;
		cmd84[2] = 0x21;
		cmd84[3] = csystem_data->provider;
		oksucced = (dre_cmd(cmd84));
		myascdump("dre3 pbmresp(5f)", cta_res, cta_lr);
		if (oksucced) // ask subscription packages, returns error on 0x11 card
		{
			memcpy(pbm+0x5f, cta_res+3, cta_lr-6);
			rdr_log_dbg(reader, D_READER, "pbm5f: %s", cs_hexdump(0, pbm+0x5f, 32, tmp_dbg, sizeof(tmp_dbg)));

			if (pbm[0] == 0xff) {
				rdr_log(reader, "no active packages");
			}
			else {
				for (i = 0; i < 0x80; i++) {
					if (pbm[i] != 0xff) {
						// sky(DRE3)
						cmd85[1] = i;
						cmd85[2] = csystem_data->provider;
						dre_cmd(cmd85);	// ask for validity dates
						myascdump("dre3 packageresp", cta_res, cta_lr);

						time_t start, end;
						struct tm temp;

						start = (cta_res[3] << 24) | (cta_res[4] << 16) | (cta_res[5] << 8) | cta_res[6];
						end   = (cta_res[7] << 24) | (cta_res[8] << 16) | (cta_res[9] << 8) | cta_res[10];
						localtime_r(&start, &temp);
						int32_t startyear  = temp.tm_year + 1900;
						int32_t startmonth = temp.tm_mon + 1;
						int32_t startday   = temp.tm_mday;
						localtime_r(&end, &temp);
						int32_t endyear    = temp.tm_year + 1900;
						int32_t endmonth   = temp.tm_mon + 1;
						int32_t endday     = temp.tm_mday;
					// sky(date)
					//	rdr_log(reader, "active package %i valid from %04i/%02i/%02i to %04i/%02i/%02i",
					//				i,
					//				startyear, startmonth, startday,
					//	  			endyear, endmonth, endday);
						rdr_log(reader, "active package %i valid from %02i/%02i/%04i to %02i/%02i/%04i",
									i+1,
									startday, startmonth, startyear,
									endday, endmonth, endyear);
						// sky(!)
						cs_add_entitlement(reader, reader->caid, b2ll(4, reader->prid[0]), 0, 0, start, end, 1, NULL, 1);
					}
				}
			}
		}
	}
	else {
		myrdr_log(reader, "dre packages failure.");
	}
	return OK;
}

static int32_t dre_card_init(struct s_reader *reader, ATR *newatr)
{
	get_atr;
	def_resp;
	char 	stcard[256];
	char  tmp[9];
	uchar checksum;
	int32_t i;
	// sky:3B 15 11 12 CA 07 00 DB
	if ( (atr[0] != 0x3B) || (atr[1] != 0x15) || (atr[2] != 0x11) || (atr[3] != 0x12) || (
		 ((atr[4] != 0xCA) || (atr[5] != 0x07)) &&
		 ((atr[4] != 0x01) || (atr[5] != 0x01))
		))
	{
		return ERROR;
	}
	checksum = dre_xor(atr + 1, 6);
	if (checksum != atr[7]) {
		rdr_log(reader, "warning: expected ATR checksum %02x, smartcard reports %02x", checksum, atr[7]);
	}

	if (!cs_malloc(&reader->csystem_data, sizeof(struct dre_data))) return ERROR;
	struct dre_data *csystem_data = reader->csystem_data;
	reader->DRE_3s = 0;
	csystem_data->provider = csystem_data->prcode = atr[6];
	switch (atr[6]) {
		case 0x11:
			strcpy(stcard, "Tricolor Centr");
			reader->caid = 0x4AE0;
			reader->DRE_3s = 0;
			reader->DRE_type = TRICOLOR_CENTR;
			reader->DRE_cass = 41;
			break;	// 59 type card = MSP (74 type = ATMEL)
		case 0x12:
			strcpy(stcard, "Cable TV");
			reader->caid = 0x4AE0; //TODO not sure about this one
			reader->DRE_3s = 0;
			reader->DRE_type = TRICOLOR_CABLETV;
			reader->DRE_cass = 41;
			break;
		case 0x13:
			strcpy(stcard, "Tricolor PH_MEDIA");
			reader->caid = 0x4AE1;
			reader->DRE_3s = 0;
			reader->DRE_type = TRICOLOR_PHMEDIA;
			reader->DRE_cass = 51;
			break;	// 59 type card
		case 0x14:
			strcpy(stcard, "Tricolor Syberia/Platforma HD new");
			reader->caid = 0x4AE1;
			reader->DRE_3s = 0;
			reader->DRE_type = TRICOLOR_SYBERIA;
			reader->DRE_cass = 51;
			break;	// 59 type card
		case 0x15:
			strcpy(stcard, "Platforma HD/DW old");
			reader->caid = 0x4AE1;
			reader->DRE_3s = 0;
			reader->DRE_type = TRICOLOR_PLATFORMAHD;
			reader->DRE_cass = 51;
			break;	// 59 type card
		// sky(DRE3)
		case 0x00:
			strcpy(stcard, "DRE3");
			reader->caid = 0x4AE1; /* 0x2710(?) */
			reader->DRE_3s = 3;
			reader->DRE_type = TRICOLOR_TVDRE3;
			reader->DRE_cass = 51;
			csystem_data->provider = 0x02;
			break;	// DRE3/Exset
		default:
			strcpy(stcard, "Unknown");
			reader->caid = 0x4AE1;
			break;
	}

	memset(reader->prid, 0, sizeof(reader->prid));
	// sky(DRE3)
	if (reader->DRE_3s) {
		uchar pridcmd[] = { 0x56,0x00 }; // get provider ID
		if ((dre_cmd(pridcmd))) {
			myascdump("dre3 pridresp", cta_res, cta_lr); // 59 04 93 01 02 6F
			if ((cta_res[cta_lr-2] == 0x90) && (cta_res[cta_lr-1] == 0x00)) {
				csystem_data->provider = cta_res[4];
				if (csystem_data->provider==0x06) {
					strcpy(stcard, "Lybid TV");
					reader->caid = 0x2710;
					reader->DRE_3s = 4;
					reader->DRE_type = TRICOLOR_LYBIDTV;
				}
			}
		}
		else {
			myrdr_log(reader, "dre3 provid failure");
		}
	}

	if (reader->DRE_3s) {
	}
	else {
		static const uchar cmd30[] = {
			0x30,0x81,0x00,0x81,0x82,0x03,0x84,0x05,0x06,0x87,0x08,0x09,0x00,0x81,0x82,0x03,
			0x84,0x05,0x00
		};
		dre_cmd(cmd30); //unknown command, generates error on card 0x11 and 0x14
		/*	response: 59 03 E2 E3 FE 48 */
	}
	uchar geocode  = 0;
	uchar geocmd[] = { 0x54,0x14 }; // get geocode
	geocmd[1] = csystem_data->provider;
	if ((dre_cmd(geocmd))) { //error would not be fatal, like on 0x11 cards
		geocode = cta_res[3];
	}

	uchar provname[128];
	uchar provcmd [] = { 0x49,0x14 }; // get providers
	provcmd[1] = csystem_data->provider;
	// sky(DRE3):ok
	if (reader->DRE_3s) {
		provcmd[0] = 0x83; // get providers
		provcmd[1] = csystem_data->provider;
	}
	if (!(dre_cmd(provcmd))) return ERROR; //fatal error
	myascdump("drex provresp", cta_res, cta_lr);
	if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
		return ERROR;
	for (i = 0;((i < cta_res[2]- 6) && (i < 128)); i++) {
		provname[i] = cta_res[6 + i];
		if (provname[i] == 0x00) break;
	}
	int32_t majorversion = cta_res[3];
	int32_t minorversion = cta_res[4];
	csystem_data->version= majorversion;

	uchar uacmd[] = { 0x43,0x14 };	// get serial number(UA)
	uacmd[1] = csystem_data->provider;
	dre_cmd(uacmd); //error would not be fatal
	myascdump("drex uaresp", cta_res, cta_lr);
	reader->hexserial[0] = 0;
	reader->hexserial[1] = 0;
	int32_t hexlength    = cta_res[1] - 2;	// discard first and last byte, last byte is always checksum, first is answer code
	memcpy(reader->hexserial + 2, cta_res + 3, hexlength);

	int32_t dre_low_id;
	int32_t dre_chksum = 0;
	uchar   dre_stid[32];
	// sky(DRE3)
	// DRE1-2:2912.02115539, DRE3:250023.14422151
	if (reader->DRE_3s) {
		dre_low_id = ((cta_res[4] << 16) | (cta_res[5] << 8) | cta_res[6]);
		snprintf((char *)dre_stid, sizeof(dre_stid), "%02i%i%i%08i",
				csystem_data->prcode,
				csystem_data->provider,
				majorversion,
				dre_low_id);
	}
	else {
		dre_low_id = ((cta_res[4] << 16) | (cta_res[5] << 8) | cta_res[6]) - 48608;
		snprintf((char *)dre_stid, sizeof(dre_stid), "%i%i%08i",
				csystem_data->provider - 0x10,
				(csystem_data->provider==0x15) ? minorversion : majorversion + 1,
				dre_low_id);
	}
	for (i = 0; i < 32; i++) {
		if (dre_stid[i] == 0x00) break;
		dre_chksum += dre_stid[i] - 48;
	}

	// sky(oscam.smartcard)
	sprintf(reader->ascserial, "%i%s", dre_chksum, dre_stid);
	rdr_log_sensitive(reader, "type: DRE Crypt, caid: %04X, serial: {%s}, dre id: %i%s, geocode %i, card: %s v%i.%i",
				reader->caid,
				cs_hexdump(0, reader->hexserial + 2, 4, tmp, sizeof(tmp)),
				dre_chksum,
				dre_stid,
				geocode,
				stcard,
				majorversion, minorversion);
	myprintf("type: DRE Crypt, caid: %04X, serial: {%s}, dre id: %i%s, geocode %i, card: %s v%i.%i",
				reader->caid,
				cs_hexdump(0, reader->hexserial + 2, 4, tmp, sizeof(tmp)),
				dre_chksum,
				dre_stid,
				geocode,
				stcard,
				majorversion, minorversion);
	rdr_log(reader, "Provider name:%s.", provname);

	memset(reader->sa,0,  sizeof(reader->sa));
	memcpy(reader->sa[0], reader->hexserial + 2, 1);	//copy first byte of unique address also in shared address, because we dont know what it is...

	rdr_log_sensitive(reader, "SA = %02X%02X%02X%02X, UA = {%s}",
			reader->sa[0][0], reader->sa[0][1], reader->sa[0][2],
			reader->sa[0][3], cs_hexdump(0, reader->hexserial + 2, 4, tmp, sizeof(tmp)));
	myprintf("SA = %02X%02X%02X%02X, UA = {%s}",
			reader->sa[0][0], reader->sa[0][1], reader->sa[0][2],
			reader->sa[0][3], cs_hexdump(0, reader->hexserial + 2, 4, tmp, sizeof(tmp)));
	// set provider infor.
	reader->nprov = 1;
	// sky(DRE3)
	if (csystem_data->provider == 0x13) {
		if (csystem_data->version == 0x01) {
			reader->caid = 0x4AE0;
			reader->DRE_cass = 41;
		}
		else {
			reader->caid = 0x4AE1;
			reader->DRE_cass = 51;
		}
	}
	if (reader->DRE_3s) {
		reader->prid[0][3] = csystem_data->provider;
		if (!dre_set_dre3_providers(reader))
			return ERROR; //fatal error
	}
	else {
		if ((csystem_data->provider == 0x15) &&
			 (csystem_data->version >= 0x04) && (strcmp("Tricolor_NC1", (char *)provname) == 0)) {
			reader->caid = 0x4AE0;
			reader->DRE_type = TRICOLOR_NC1;
			reader->DRE_cass = 41;
		}
		else if (csystem_data->version >= 0x02)	{
			reader->DRE_cass = 51;
			if (csystem_data->provider == 0x11)	{
				reader->caid = 0x4AE1;
				reader->DRE_type = TRICOLOR_TV;
			}
		}
		else {
			reader->DRE_cass = 41;
		}
		if (!dre_set_provider_info(reader))
			return ERROR; //fatal error
	}
	rdr_log(reader, "ready for requests");
	return OK;
}

static unsigned char DESkeys[16*8]=
{
	0x4A,0x11,0x23,0xB1,0x45,0x99,0xCF,0x10, // 00
	0x21,0x1B,0x18,0xCD,0x02,0xD4,0xA1,0x1F, // 01
	0x07,0x56,0xAB,0xB4,0x45,0x31,0xAA,0x23, // 02
	0xCD,0xF2,0x55,0xA1,0x13,0x4C,0xF1,0x76, // 03
	0x57,0xD9,0x31,0x75,0x13,0x98,0x89,0xC8, // 04
	0xA3,0x36,0x5B,0x18,0xC2,0x83,0x45,0xE2, // 05
	0x19,0xF7,0x35,0x08,0xC3,0xDA,0xE1,0x28, // 06
	0xE7,0x19,0xB5,0xD8,0x8D,0xE3,0x23,0xA4, // 07
	0xA7,0xEC,0xD2,0x15,0x8B,0x42,0x59,0xC5, // 08
	0x13,0x49,0x83,0x2E,0xFB,0xAD,0x7C,0xD3, // 09
	0x37,0x25,0x78,0xE3,0x72,0x19,0x53,0xD9, // 0A
	0x7A,0x15,0xA4,0xC7,0x15,0x49,0x32,0xE8, // 0B
	0x63,0xD5,0x96,0xA7,0x27,0xD8,0xB2,0x68, // 0C
	0x42,0x5E,0x1A,0x8C,0x41,0x69,0x8E,0xE8, // 0D
	0xC2,0xAB,0x37,0x29,0xD3,0xCF,0x93,0xA7, // 0E
	0x49,0xD3,0x33,0xC2,0xEB,0x71,0xD3,0x14  // 0F
};

static void dre_over(const unsigned char *ECMdata, unsigned char *DW)
{
	uchar key[8];
	if (ECMdata[2] >= (43+4) && ECMdata[40] == 0x3A && ECMdata[41] == 0x4B)
	{
		memcpy(key, &DESkeys[(ECMdata[42] & 0x0F) * 8], 8);

		doPC1(key);

		des(key, DES_ECS2_DECRYPT, DW);   // even DW post-process
		des(key, DES_ECS2_DECRYPT, DW+8); // odd  DW post-process
	};
};


static int32_t dre_do_ecm(struct s_reader *reader, const ECM_REQUEST *er, struct s_ecm_answer *ea)
{
	def_resp;
	char tmp_dbg[256];
	struct dre_data *csystem_data = reader->csystem_data;
	// sky(DRE3)
	if (reader->DRE_3s)
	{
		myprdump("dreecm", (void *)er->ecm, er->ecmlen);
		uchar ecmcmd3[0x39];
		memset(ecmcmd3, 0, sizeof(ecmcmd3));
		memcpy(ecmcmd3, er->ecm + 0x11, 0x38 /* er->ecmlen-0x15 */);
		ecmcmd3[0x38] = csystem_data->provider;
//		myprintf("[dre]provider:%02x{%02x}\n", csystem_data->provider, type);
//		myprintf("[dre]unused ECM front:%s\n", cs_hexdump(0, er->ecm, 0x11, tmp_dbg, sizeof(tmp_dbg)));
//		myprintf("[dre]unused ECM back :%s\n", cs_hexdump(0, er->ecm+ 0x49, 4, tmp_dbg, sizeof(tmp_dbg)));
		if ((dre3_cmds(ecmcmd3))) // ecm request
		{
			if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00)) return ERROR; //exit if response is not 90 00
			memcpy(ea->cw,   cta_res+11, 8);
			memcpy(ea->cw+8, cta_res+3,  8);
			return OK;
		}
	}
	else
	if (reader->DRE_cass == 0x41)
	{
		if ((er->ecm[3] == 0x56 && er->ecm[4] == 0x0b)
			&& (csystem_data->version  >= 0x04)
			&& (csystem_data->provider == 0x15)
			&& (reader->caid == 0x4AE0))
		{
			memcpy(ea->cw,   &er->ecm[16], 8);
			memcpy(ea->cw+8, &er->ecm[ 8], 8);
			return OK;

		}

		uchar ecmcmd41[] = { 0x41,
						0x58,0x1f,0x00,		//fixed part, dont change
						0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,	//0x01 - 0x08: next key
						0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,	//0x11 - 0x18: current key
						0x3b,0x59,0x11, // 0x3b = keynumber, can be a value 56 ;; 0x59 number of package = 58+1 - Pay Package ;; 0x11 = provider
		};
		ecmcmd41[22] = csystem_data->provider;
		memcpy(ecmcmd41 + 4, er->ecm + 8, 16);
		ecmcmd41[20] = er->ecm[6];	// keynumber
		ecmcmd41[21] = 0x58 + er->ecm[25];	// package number
		rdr_log_dbg(reader, D_READER, "unused ECM front:%s", cs_hexdump(0, er->ecm, 8, tmp_dbg, sizeof(tmp_dbg)));
		rdr_log_dbg(reader, D_READER, "unused ECM back :%s", cs_hexdump(0, er->ecm+24, er->ecm[2]+2-24, tmp_dbg, sizeof(tmp_dbg)));
		if ((dre_cmd(ecmcmd41))) // ecm request
		{
			if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00)) return ERROR; // exit if response is not 90 00
			memcpy(ea->cw,   cta_res+11, 8);
			memcpy(ea->cw+8, cta_res+3,  8);
			return OK;
		}
	}
	else
	{
		uchar ecmcmd51[] = { 0x51,0x02,0x56,0x05,0x00,0x4A,0xE3,	//fixed header?
						0x9C,0xDA,		// first three nibbles count up, fourth nibble counts down; all ECMs sent twice
						0xC1,0x71,0x21,0x06,0xF0,0x14,0xA7,0x0E,	//next key?
						0x89,0xDA,0xC9,0xD7,0xFD,0xB9,0x06,0xFD,	//current key?
						0xD5,0x1E,0x2A,0xA3,0xB5,0xA0,0x82,0x11,	//key or signature?
						0x14,	// provider
		};
		memcpy(ecmcmd51 + 1, er->ecm + 5, 0x21);
		rdr_log_dbg(reader, D_READER, "unused ECM front:%s", cs_hexdump(0, er->ecm, 5, tmp_dbg, sizeof(tmp_dbg)));
		rdr_log_dbg(reader, D_READER, "unused ECM back :%s", cs_hexdump(0, er->ecm + 37, 4, tmp_dbg, sizeof(tmp_dbg)));
		ecmcmd51[33] = csystem_data->provider;	//no part of sig
		if ((dre_cmd(ecmcmd51))) //ecm request
		{
			if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00)) return ERROR; //exit if response is not 90 00
			if (reader->caid == 0x4AE1) {
				dre_over(er->ecm, cta_res + 3);
			}
			memcpy(ea->cw,   cta_res+11, 8);
			memcpy(ea->cw+8, cta_res+3,  8);
			return OK;
		}
	}
	return ERROR;
}

static int32_t dre_get_emm_type(EMM_PACKET *ep, struct s_reader *rdr)
{
	switch (ep->emm[0])
	{
		case 0x83:
		case 0x89:
			ep->type = SHARED;
			// FIXME: Seems to be that SA is only used with caid 0x4AE1
			if (rdr->DRE_cass==51) {
				memset(ep->hexserial, 0, 8);
				memcpy(ep->hexserial, ep->emm + 3, 4);
			//	return (!memcmp(&rdr->sa[0][0], ep->emm + 3, 4));
				return (ep->hexserial[0] == rdr->sa[0][0]);
			}
			else {
				return 1;
			}

		case 0x80:
		case 0x82:
		case 0x86:
		case 0x88: // sky(DRE!)
		case 0x8c:
			ep->type = SHARED;
			memset(ep->hexserial, 0, 8);
			ep->hexserial[0] = ep->emm[3];
			return (ep->hexserial[0] == rdr->sa[0][0]);

		case 0x87: // sky(DRE?)
		case 0x8b: // sky(DRE3)
			ep->type = UNIQUE;
			if (rdr->DRE_cass==51) {
				memset(ep->hexserial, 0, 8);
				memcpy(ep->hexserial, ep->emm + 3, 4);
				return (!memcmp(rdr->hexserial+2, ep->hexserial, 4));
			}
			else {
				return 1;
			}
			break;
		default:
			ep->type = UNKNOWN;
			return 1;
	}
}

static int32_t dre_get_emm_filter(struct s_reader *rdr, struct s_csystem_emm_filter **emm_filters, unsigned int *filter_count)
{
	if (*emm_filters == NULL) {
		const unsigned int max_filter_count = 9; // sky(DRE!)
		if (!cs_malloc(emm_filters, max_filter_count * sizeof(struct s_csystem_emm_filter)))
		  	return ERROR;

		struct s_csystem_emm_filter *filters = *emm_filters;
		*filter_count = 0;

		int32_t idx = 0;
#if 0
		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x80;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xF2;
		filters[idx].mask  [1] = 0xFF;
		idx++;
#endif

		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x82;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xFF;
		filters[idx].mask  [1] = 0xFF;
		idx++;

		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x83;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xFF;
		filters[idx].mask  [1] = 0xFF;
		if (rdr->DRE_cass==51) {
			memcpy(&filters[idx].filter[1], &rdr->sa[0][0], 4);
			memset(&filters[idx].mask  [1], 0xFF, 4);
		}
		idx++;

		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x83;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xFF;
		filters[idx].mask  [1] = 0xFF;
		// FIXME: Seems to be that SA is only used with caid 0x4AE1
		if (rdr->DRE_cass==51) {
			memcpy(&filters[idx].filter[1], &rdr->sa[0][0], 4);
			memset(&filters[idx].mask  [1], 0xFF, 4);
		}
		idx++;

		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x86;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xFF;
		filters[idx].mask  [1] = 0xFF;
		idx++;

		// sky(DRE!)
		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x88;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xFF;
		filters[idx].mask  [1] = 0xFF;
		idx++;

		// sky(DRE3)
		filters[idx].type = EMM_SHARED;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x8C;
		filters[idx].filter[1] = rdr->sa[0][0];
		filters[idx].mask  [0] = 0xFF;
		filters[idx].mask  [1] = 0xFF;
		idx++;

		// EMM-Unique
		filters[idx].type = EMM_UNIQUE;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x87;
		filters[idx].mask  [0] = 0xFF;
		//FIXME: No filter for hexserial
		if (rdr->DRE_cass==51) {
			memcpy(&filters[idx].filter[1], rdr->hexserial+2, 4);
			memset(&filters[idx].mask  [1], 0xFF, 4);
		}
		idx++;

		// sky(DRE3)
		filters[idx].type = EMM_UNIQUE;
		filters[idx].enabled   = 1;
		filters[idx].filter[0] = 0x8b;
		filters[idx].mask  [0] = 0xFF;
		if (rdr->DRE_cass==51) {
			memcpy(&filters[idx].filter[1], rdr->hexserial+2, 4);
			memset(&filters[idx].mask  [1], 0xFF, 4);
		}
		idx++;

		*filter_count = idx;
	}

	return OK;
}

static int32_t dre_do_emm(struct s_reader *reader, EMM_PACKET *ep)
{
	def_resp;
	struct dre_data *csystem_data = reader->csystem_data;
	int32_t i;

	// sky(DRE3)
	if (reader->DRE_cass==51) {
		if (reader->DRE_3s)
		{
			if (ep->emm[0]==0x8B) // 0x8B
			{	/* For new package activation. */
				uchar emmcmd3[0x39];
				memcpy(emmcmd3, ep->emm + 0x13, 0x38);
				emmcmd3[0x38] = csystem_data->provider; // sky(DRE!)
				if ((dre3_cmds(emmcmd3)))
					if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
						return ERROR;
			}
			else // 0x8C
			{
				// check for shared address
				if (ep->emm[3] != reader->sa[0][0]) return OK; // ignore, wrong address
				uchar emmcmd3[0x49];
				for (i=0; i<ep->emmlen-0x17; i+=0x48) {
					memcpy(emmcmd3, ep->emm + 0x13 + i, 0x48);
					emmcmd3[0x48] = csystem_data->provider;
					if ((dre3_cmds(emmcmd3)))
						if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
					    	return ERROR; //exit if response is not 90 00
				}
			}
			return OK; //success
		}
		if (reader->DRE_type==TRICOLOR_TV)
		{
			if (ep->emm[39] == 0x3d || ep->emm[39] == 0x3b || ep->emm[39] == 0x3a) { // sky(DRE!)
				/* For new package activation. */
				uchar emmcmd58[26]  = {0x58, };
				memcpy(&emmcmd58[1], &ep->emm[40], 24);
			//	emmcmd58[25] = 0x15;
				emmcmd58[25] = csystem_data->provider; // sky(DRE!)
				if ((dre_cmd(emmcmd58)))
					if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
						return ERROR;
				return OK; //success
			}
		}
		if ((ep->emm[4] != 0) || (ep->emm[5] != 0) || (ep->emm[6] != 0))
		{
			if ((ep->emm[40] == reader->sa[0][0]) &&
				 (ep->emm[41] == 0x00 && ep->emm[42] == 0x00 && ep->emm[43] == 0x00))
			{
				i = 0;
			}
			else if (memcmp(&ep->emm[3], reader->hexserial+2, 4))
			{
				return ERROR;
			}
		}
		// 0x86
		// check for shared address
		if (ep->emm[3] != reader->sa[0][0]) return OK; // ignore, wrong address
		uchar emmcmd52[0x3a] = {0x52, };
		for (i=0; i<2; i++) {
			memcpy(emmcmd52 + 1, ep->emm + 5 + 32 + i * 56, 56);
			emmcmd52[0x39] = csystem_data->provider;
			if ((dre_cmd(emmcmd52)))
				if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
			    	return ERROR; //exit if response is not 90 00
		}
	}
	else
	{
		if ((ep->emm[4] != 0) || (ep->emm[5] != 0) || (ep->emm[6] != 0))
		{
			if ((ep->emm[ 9] == reader->sa[0][0]) &&
				 (ep->emm[10] == 0x00 && ep->emm[11] == 0x00 && ep->emm[12] == 0x00))
			{
				i = 0;
			}
			else if (memcmp(&ep->emm[3], reader->hexserial+2, 4))
			{
				return ERROR;
			}
		}
		uchar emmcmd42[] =
		{ 	0x42,0x85,0x58,0x01,0xC8,0x00,0x00,0x00,0x05,0xB8,0x0C,0xBD,0x7B,0x07,0x04,0xC8,
			0x77,0x31,0x95,0xF2,0x30,0xB7,0xE9,0xEE,0x0F,0x81,0x39,0x1C,0x1F,0xA9,0x11,0x3E,
			0xE5,0x0E,0x8E,0x50,0xA4,0x31,0xBB,0x01,0x00,0xD6,0xAF,0x69,0x60,0x04,0x70,0x3A,
			0x91,
			0x56,0x58,0x11
		};
		if (ep->emm[0]==0x87)
		{
			for (i = 0; i < 2; i++) {
				memcpy(emmcmd42 + 1, ep->emm + 42 + i*49, 48);
				emmcmd42[49] = ep->emm[i*49 + 41]; //keynr
				emmcmd42[50] = 0x58 + ep->emm[40]; //package nr
				emmcmd42[51] = csystem_data->provider;
				if ((dre_cmd(emmcmd42))) {
			      if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
						return ERROR; //exit if response is not 90 00
				}
			}
		}
		else
		{
			memcpy(emmcmd42 + 1, ep->emm + 6, 48);
			emmcmd42[51] = csystem_data->provider;
		//	emmcmd42[50] = ecmcmd42[2]; //TODO package nr could also be fixed 0x58
			emmcmd42[50] = 0x58;
			emmcmd42[49] = ep->emm[5];	 //keynr
			/* response:
			   59 05 A2 02 05 01 5B
			   90 00 */
			if ((dre_cmd(emmcmd42))) {	 //first emm request
			  	if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
					return ERROR; //exit if response is not 90 00

				memcpy(emmcmd42 + 1, ep->emm + 55, 7);	 //TODO OR next two lines?
			/* memcpy(emmcmd42 + 1, ep->emm + 55, 7);  //FIXME either I cant count or my EMM log contains errors
				memcpy(emmcmd42 + 8, ep->emm + 67, 41); */
				emmcmd42[51] = csystem_data->provider;
			//	emmcmd42[50] = ecmcmd42[2]; //TODO package nr could also be fixed 0x58
				emmcmd42[50] = 0x58;
				emmcmd42[49] = ep->emm[54]; //keynr
				if ((dre_cmd(emmcmd42))) {	 //second emm request
					if ((cta_res[cta_lr-2] != 0x90) || (cta_res[cta_lr-1] != 0x00))
						return ERROR; //exit if response is not 90 00
			  	}
			}
		}
	}
	return OK; //success
}

static int32_t dre_card_info(struct s_reader *UNUSED(rdr))
{
	return OK;
}

const struct s_cardsystem reader_dre =
{
	.desc           = "dre",
	.caids          = (uint16_t[]){ 0x4AE0, 0x4AE1, 0x7BE0, 0x7BE1, 0x2710, 0 }, // sky(DRE4/EXSET)
	.do_emm         = dre_do_emm,
	.do_ecm         = dre_do_ecm,
	.card_info      = dre_card_info,
	.card_init      = dre_card_init,
	.get_emm_type   = dre_get_emm_type,
	.get_emm_filter = dre_get_emm_filter,
	.caidsvarious   = 1,
};

#endif
