#include "globals.h"
#if defined(MODULE_XCAS)
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-net.h"
#include "oscam-chk.h"
#include "oscam-string.h"
#include "cscrypt/bn.h"
#include "module-dvbapi.h"
#include "module-xcas.h"
/* sky(2016.03.07) */
#if defined(__XCAS_POWERVU__)
//	#define	WITH_EMU
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
struct xpowervu_keydata
{
	int16_t	found;
	uint32_t	ppid;
	uint8_t	knr;
	uint8_t	keys[16];
	int16_t	failure;
};
struct xpowervu_keydata g_xpowervu[MAX_DEMUX];

static const uint8_t PowerVu_A0_S_1[16] = {0x33, 0xA4, 0x44, 0x3C, 0xCA, 0x2E, 0x75, 0x7B, 0xBC, 0xE6, 0xE5, 0x35, 0xA0, 0x55, 0xC9, 0xA2};
static const uint8_t PowerVu_A0_S_2[16] = {0x5A, 0xB0, 0x2C, 0xBC, 0xDA, 0x32, 0xE6, 0x92, 0x40, 0x53, 0x6E, 0xF9, 0x69, 0x11, 0x1E, 0xFB};
static const uint8_t PowerVu_A0_S_3[16] = {0x4E, 0x18, 0x9B, 0x19, 0x79, 0xFB, 0x01, 0xFA, 0xE3, 0xE1, 0x28, 0x3D, 0x32, 0xE4, 0x92, 0xEA};
static const uint8_t PowerVu_A0_S_4[16] = {0x05, 0x6F, 0x37, 0x66, 0x35, 0xE1, 0x58, 0xD0, 0xB4, 0x6A, 0x97, 0xAE, 0xD8, 0x91, 0x27, 0x56};
static const uint8_t PowerVu_A0_S_5[16] = {0x7B, 0x26, 0xAD, 0x34, 0x3D, 0x77, 0x39, 0x51, 0xE0, 0xE0, 0x48, 0x8C, 0x39, 0xF5, 0xE8, 0x47};
static const uint8_t PowerVu_A0_S_6[16] = {0x74, 0xFA, 0x4D, 0x79, 0x42, 0x39, 0xD1, 0xA4, 0x99, 0xA3, 0x97, 0x07, 0xDF, 0x14, 0x3A, 0xC4};
static const uint8_t PowerVu_A0_S_7[16] = {0xC6, 0x1E, 0x3C, 0x24, 0x11, 0x08, 0x5D, 0x6A, 0xEB, 0x97, 0xB9, 0x25, 0xA7, 0xFA, 0xE9, 0x1A};
static const uint8_t PowerVu_A0_S_8[16] = {0x9A, 0xAD, 0x72, 0xD7, 0x7C, 0x68, 0x3B, 0x55, 0x1D, 0x4A, 0xA2, 0xB0, 0x38, 0xB9, 0x56, 0xD0};
static const uint8_t PowerVu_A0_S_9[32] = {0x61, 0xDA, 0x5F, 0xB7, 0xEB, 0xC6, 0x3F, 0x6C, 0x09, 0xF3, 0x64, 0x38, 0x33, 0x08, 0xAA, 0x15,
													    0xCC, 0xEF, 0x22, 0x64, 0x01, 0x2C, 0x12, 0xDE, 0xF4, 0x6E, 0x3C, 0xCD, 0x1A, 0x64, 0x63, 0x7C
													  	};

static const uint8_t PowerVu_00_S_1[16] = {0x97, 0x13, 0xEB, 0x6B, 0x04, 0x5E, 0x60, 0x3A, 0xD9, 0xCC, 0x91, 0xC2, 0x5A, 0xFD, 0xBA, 0x0C};
static const uint8_t PowerVu_00_S_2[16] = {0x61, 0x3C, 0x03, 0xB0, 0xB5, 0x6F, 0xF8, 0x01, 0xED, 0xE0, 0xE5, 0xF3, 0x78, 0x0F, 0x0A, 0x73};
static const uint8_t PowerVu_00_S_3[16] = {0xFD, 0xDF, 0xD2, 0x97, 0x06, 0x14, 0x91, 0xB5, 0x36, 0xAD, 0xBC, 0xE1, 0xB3, 0x00, 0x66, 0x41};
static const uint8_t PowerVu_00_S_4[16] = {0x8B, 0xD9, 0x18, 0x0A, 0xED, 0xEE, 0x61, 0x34, 0x1A, 0x79, 0x80, 0x8C, 0x1E, 0x7F, 0xC5, 0x9F};
static const uint8_t PowerVu_00_S_5[16] = {0xB0, 0xA1, 0xF2, 0xB8, 0xEA, 0x72, 0xDD, 0xD3, 0x30, 0x65, 0x2B, 0x1E, 0xE9, 0xE1, 0x45, 0x29};
static const uint8_t PowerVu_00_S_6[16] = {0x5D, 0xCA, 0x53, 0x75, 0xB2, 0x24, 0xCE, 0xAF, 0x21, 0x54, 0x9E, 0xBE, 0x02, 0xA9, 0x4C, 0x5D};
static const uint8_t PowerVu_00_S_7[16] = {0x42, 0x66, 0x72, 0x83, 0x1B, 0x2D, 0x22, 0xC9, 0xF8, 0x4D, 0xBA, 0xCD, 0xBB, 0x20, 0xBD, 0x6B};
static const uint8_t PowerVu_00_S_8[16] = {0xC4, 0x0C, 0x6B, 0xD3, 0x6D, 0x94, 0x7E, 0x53, 0xCE, 0x96, 0xAC, 0x40, 0x2C, 0x7A, 0xD3, 0xA9};
static const uint8_t PowerVu_00_S_9[32] = {0x31, 0x82, 0x4F, 0x9B, 0xCB, 0x6F, 0x9D, 0xB7, 0xAE, 0x68, 0x0B, 0xA0, 0x93, 0x15, 0x32, 0xE2,
													    0xED, 0xE9, 0x47, 0x29, 0xC2, 0xA8, 0x92, 0xEF, 0xBA, 0x27, 0x22, 0x57, 0x76, 0x54, 0xC0, 0x59,
													  	};


static uint8_t _get_bit(uint8_t byte, uint8_t bitnb)
{
	return ((byte&(1<<bitnb)) ? 1: 0);
}

static uint8_t _set_bit(uint8_t val, uint8_t bitnb, uint8_t biton)
{
	return (biton ? (val | (1<<bitnb)) : (val & ~(1<<bitnb)));
}

static void xpowervu_expandDesKey(unsigned char *key)
{
	uint8_t i, j, parity;
	uint8_t tmpKey[7];

	memcpy(tmpKey, key, 7);

	key[0] = (tmpKey[0] & 0xFE);
	key[1] = ((tmpKey[0] << 7) | ((tmpKey[1] >> 1) & 0xFE));
	key[2] = ((tmpKey[1] << 6) | ((tmpKey[2] >> 2) & 0xFE));
	key[3] = ((tmpKey[2] << 5) | ((tmpKey[3] >> 3) & 0xFE));
	key[4] = ((tmpKey[3] << 4) | ((tmpKey[4] >> 4) & 0xFE));
	key[5] = ((tmpKey[4] << 3) | ((tmpKey[5] >> 5) & 0xFE));
	key[6] = ((tmpKey[5] << 2) | ((tmpKey[6] >> 6) & 0xFE));
	key[7] = (tmpKey[6] << 1);

	for (i = 0; i < 8; i++)
	{
		parity = 1;
		for (j = 1; j < 8; j++) if ((key[i] >> j) & 0x1) { parity = ~parity & 0x01; }
		key[i] |= parity;
	}
}

static uint8_t xpowervu_sbox(uint8_t *input, uint8_t mode)
{
	uint8_t s_index, bit, last_index, last_bit;
	uint8_t const *Sbox1, *Sbox2, *Sbox3, *Sbox4, *Sbox5, *Sbox6, *Sbox7, *Sbox8, *Sbox9;

	if (mode)
	{
		Sbox1 = PowerVu_A0_S_1;
		Sbox2 = PowerVu_A0_S_2;
		Sbox3 = PowerVu_A0_S_3;
		Sbox4 = PowerVu_A0_S_4;
		Sbox5 = PowerVu_A0_S_5;
		Sbox6 = PowerVu_A0_S_6;
		Sbox7 = PowerVu_A0_S_7;
		Sbox8 = PowerVu_A0_S_8;
		Sbox9 = PowerVu_A0_S_9;
	}
	else
	{
		Sbox1 = PowerVu_00_S_1;
		Sbox2 = PowerVu_00_S_2;
		Sbox3 = PowerVu_00_S_3;
		Sbox4 = PowerVu_00_S_4;
		Sbox5 = PowerVu_00_S_5;
		Sbox6 = PowerVu_00_S_6;
		Sbox7 = PowerVu_00_S_7;
		Sbox8 = PowerVu_00_S_8;
		Sbox9 = PowerVu_00_S_9;
	}

	bit = (_get_bit(input[2],0)<<2) | (_get_bit(input[3],4)<<1) | (_get_bit(input[5],3));
	s_index = (_get_bit(input[0],0)<<3) | (_get_bit(input[2],6)<<2) | (_get_bit(input[2],4)<<1) | (_get_bit(input[5],7));
	last_bit = _get_bit(Sbox1[s_index],7-bit);

	bit = (_get_bit(input[5],0)<<2) | (_get_bit(input[4],0)<<1) | (_get_bit(input[6],2));
	s_index = (_get_bit(input[2],1)<<3) | (_get_bit(input[2],2)<<2) | (_get_bit(input[5],5)<<1) | (_get_bit(input[5],1));
	last_bit = last_bit | (_get_bit(Sbox2[s_index],7-bit)<<1);

	bit = (_get_bit(input[6],0)<<2) | (_get_bit(input[1],7)<<1) | (_get_bit(input[6],7));
	s_index = (_get_bit(input[1],3)<<3) | (_get_bit(input[3],7)<<2) | (_get_bit(input[1],5)<<1) | (_get_bit(input[5],2));
	last_bit = last_bit | (_get_bit(Sbox3[s_index], 7-bit)<<2);

	bit = (_get_bit(input[1],0)<<2) | (_get_bit(input[2],7)<<1) | (_get_bit(input[2],5));
	s_index = (_get_bit(input[6],3)<<3) | (_get_bit(input[6],4)<<2) | (_get_bit(input[6],6)<<1) | (_get_bit(input[3],5));
	last_index = _get_bit(Sbox4[s_index], 7-bit);

	bit = (_get_bit(input[3],3)<<2) | (_get_bit(input[4],6)<<1) | (_get_bit(input[3],2));
	s_index = (_get_bit(input[3],1)<<3) | (_get_bit(input[4],5)<<2) | (_get_bit(input[3],0)<<1) | (_get_bit(input[4],7));
	last_index = last_index | (_get_bit(Sbox5[s_index], 7-bit)<<1);

	bit = (_get_bit(input[5],4)<<2) | (_get_bit(input[4],4)<<1) | (_get_bit(input[1],2));
	s_index = (_get_bit(input[2],3)<<3) | (_get_bit(input[6],5)<<2) | (_get_bit(input[1],4)<<1) | (_get_bit(input[4],1));
	last_index = last_index | (_get_bit(Sbox6[s_index], 7-bit)<<2);

	bit = (_get_bit(input[0],6)<<2) | (_get_bit(input[0],7)<<1) | (_get_bit(input[0],4));
	s_index = (_get_bit(input[0],5)<<3) | (_get_bit(input[0],3)<<2) | (_get_bit(input[0],1)<<1) | (_get_bit(input[0],2));
	last_index = last_index | (_get_bit(Sbox7[s_index], 7-bit)<<3);

	bit = (_get_bit(input[4],2)<<2) | (_get_bit(input[4],3)<<1) | (_get_bit(input[1],1));
	s_index = (_get_bit(input[1],6)<<3) | (_get_bit(input[6],1)<<2) | (_get_bit(input[5],6)<<1) | (_get_bit(input[3],6));
	last_index = last_index | (_get_bit(Sbox8[s_index], 7-bit)<<4);

	return (_get_bit(Sbox9[last_index&0x1f],7-last_bit)&1) ? 1: 0;
}

static void xpowervu_decrypt(uint8_t *data, uint32_t length, uint8_t *key, uint8_t sbox)
{
	uint32_t i;
	int32_t j, k;
	uint8_t curByte, tmpBit;

	for(i=0; i<length; i++)
	{
		curByte = data[i];

		for(j=7; j>=0; j--)
		{
			data[i] = _set_bit(data[i], j,(_get_bit(curByte,j)^xpowervu_sbox(key, sbox))^_get_bit(key[0],7));

			tmpBit  = _get_bit(data[i],j)^(_get_bit(key[6],0));
			if (tmpBit)
			{
				key[3] ^= 0x10;
			}

			for (k = 6; k > 0; k--)
			{
				key[k] = (key[k]>>1) | (key[k-1]<<7);
			}
			key[0] = (key[0]>>1);

			key[0] = _set_bit(key[0], 7, tmpBit);
		}
	}
}

#define PVU_CW_VID 	0	// VIDeo
#define PVU_CW_HSD 	1	// High Speed Data
#define PVU_CW_A1 	2	// Audio 1
#define PVU_CW_A2		3	// Audio 2
#define PVU_CW_A3 	4	// Audio 3
#define PVU_CW_A4 	5	// Audio 4
#define PVU_CW_UTL 	6	// UTiLity
#define PVU_CW_VBI 	7	// Vertical Blanking Interval

#define PVU_CONVCW_VID_ECM	0x80	// VIDeo
#define PVU_CONVCW_HSD_ECM 0x40 	// High Speed Data
#define PVU_CONVCW_A1_ECM 	0x20	// Audio 1
#define PVU_CONVCW_A2_ECM 	0x10	// Audio 2
#define PVU_CONVCW_A3_ECM 	0x08	// Audio 3
#define PVU_CONVCW_A4_ECM 	0x04	// Audio 4
#define PVU_CONVCW_UTL_ECM 0x02	// UTiLity
#define PVU_CONVCW_VBI_ECM 0x01	// Vertical Blanking Interval

static uint8_t xpowervu_getConvcwIndex(uint8_t ecmTag)
{
	switch(ecmTag)
	{
	case PVU_CONVCW_VID_ECM: return PVU_CW_VID;
	case PVU_CONVCW_HSD_ECM: return PVU_CW_HSD;
	case PVU_CONVCW_A1_ECM:	 return PVU_CW_A1;
	case PVU_CONVCW_A2_ECM:	 return PVU_CW_A2;
	case PVU_CONVCW_A3_ECM:	 return PVU_CW_A3;
	case PVU_CONVCW_A4_ECM:	 return PVU_CW_A4;
	case PVU_CONVCW_UTL_ECM: return PVU_CW_UTL;
	case PVU_CONVCW_VBI_ECM: return PVU_CW_VBI;
	default:	return PVU_CW_VBI;
	}
}

static uint16_t xpowervu_getetSeedIV(uint8_t seedType, uint8_t *ecm)
{
	switch(seedType)
	{
	case PVU_CW_VID: return ( (ecm[0x10] & 0x1F) <<3) | 0;
	case PVU_CW_HSD: return ( (ecm[0x12] & 0x1F) <<3) | 2;
	case PVU_CW_A1:  return ( (ecm[0x11] & 0x3F) <<3) | 1;
	case PVU_CW_A2:  return ( (ecm[0x13] & 0x3F) <<3) | 1;
	case PVU_CW_A3:  return ( (ecm[0x19] & 0x3F) <<3) | 1;
	case PVU_CW_A4:  return ( (ecm[0x1A] & 0x3F) <<3) | 1;;
	case PVU_CW_UTL: return ( (ecm[0x14] & 0x0F) <<3) | 4;
	case PVU_CW_VBI: return (((ecm[0x15] & 0xF8) >>3)<<3) | 5;
	default:
		return 0;
	}
}

static void xpowervu_expandSeed(uint8_t seedType, uint8_t *seed)
{
	uint8_t seedLength, i;

	switch(seedType)
	{
	case PVU_CW_VID:
	case PVU_CW_HSD: seedLength = 4; break;
	case PVU_CW_A1:
	case PVU_CW_A2:
	case PVU_CW_A3:
	case PVU_CW_A4:  seedLength = 3; break;
	case PVU_CW_UTL:
	case PVU_CW_VBI: seedLength = 2; break;
	default:	return;
	}

	for(i=seedLength; i<7; i++)
	{
		seed[i] = seed[i%seedLength];
	}
}

static void xpowervu_calculateSeed(uint8_t seedType, uint8_t *ecm, uint8_t *seedBase, uint8_t *key, uint8_t *seed, uint8_t sbox)
{
	uint16_t tmpSeed;

	tmpSeed = xpowervu_getetSeedIV(seedType, ecm+23);
	seed[0] = (tmpSeed>>2) & 0xFF;
	seed[1] = ((tmpSeed&0x3)<<6) | (seedBase[0]>>2);
	seed[2] = (seedBase[0]<<6) | (seedBase[1]>>2);
	seed[3] = (seedBase[1]<<6) | (seedBase[2]>>2);
	seed[4] = (seedBase[2]<<6) | (seedBase[3]>>2);
	seed[5] = (seedBase[3]<<6);

	xpowervu_decrypt(seed, 6, key, sbox);

	seed[0] = (seed[1]<<2) | (seed[2]>>6);
	seed[1] = (seed[2]<<2) | (seed[3]>>6);
	seed[2] = (seed[3]<<2) | (seed[4]>>6);
	seed[3] = (seed[4]<<2) | (seed[5]>>6);
}

static void xpowervu_calculateCw(uint8_t seedType, uint8_t *seed, uint8_t csaUsed,
							   uint8_t *convolvedCw, uint8_t *cw, uint8_t *baseCw)
{
	int32_t k;

	xpowervu_expandSeed(seedType, seed);

	if (csaUsed)
	{
		for(k=0; k<7; k++)
		{
			seed[k] ^= baseCw[k];
		}

		cw[0] = seed[0] ^ convolvedCw[0];
		cw[1] = seed[1] ^ convolvedCw[1];
		cw[2] = seed[2] ^ convolvedCw[2];
		cw[3] = seed[3] ^ convolvedCw[3];
		cw[4] = seed[3] ^ convolvedCw[4];
		cw[5] = seed[4] ^ convolvedCw[5];
		cw[6] = seed[5] ^ convolvedCw[6];
		cw[7] = seed[6] ^ convolvedCw[7];
	}
	else
	{
		for(k=0; k<7; k++)
		{
			cw[k] = seed[k] ^ baseCw[k];
		}
		xpowervu_expandDesKey(cw);
	}
}

static int
xpowervu_searchDeskey(struct s_reader *reader, uint32_t *psearchids, uint32_t searchnum, uint8_t keyNr, uint8_t *cDeskey, int *pfoundindex)
{
	int found;
	found = XEMUKEY_PidsSearch(reader, CASS_POWERVU, psearchids, searchnum, keyNr, cDeskey, 7, pfoundindex);
	if (found && cs_Iszero(cDeskey,7)) found = 0;
	return found;
}
/*
81 30 3D
30 37
20
0E 00 00 00 00 74
B0
00
52 C8 D8 0D 69 E9 4A AF AB EF 97 62 06 33
00
00 10
00 00
5C 9C DD 0D ED 2F 62 9A 61 02 41 B9 CC 74 30 14
1F B4 FA 20 B1 3F 85 A4 78 11 49
2F 59 5F 95
*/
static int
xpowervu_ecmProcess(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw, CWEXTENTION *cwEx)
{
	struct xpowervu_keydata *pxpowerpvu;
	uint8_t *ecm = er->ecm;
	uint16_t ecmLen = SCT_LEN(ecm);
	uint32_t ecmCrc32;
	uint8_t  nanoCmd, nanoChecksum, keyType, fixedKey, oddKey, bid, csaUsed;
	uint16_t nanoLen;
	uint32_t channelId, ecmSrvid, keyIndex;
	uint32_t searchIds[5];
	uint8_t  convolvedCw[8][8];
	uint8_t  ecmKey[7], tmpEcmKey[7], seedBase[4], baseCw[7], seed[8][8], excw[8][8];
	uint8_t  decrypt_ok;
	uint8_t  ecmPart1[14], ecmPart2[27];
	uint8_t  sbox;
	uint8_t *dwp;
	int 		dmuxid;
	int      keyReference;
	int 		i, j, k;

	myascdump("powervu", ecm, ecmLen);
	if (ecmLen < 7) return 0;

	dmuxid = er->dmuxid % MAX_DEMUX;
	pxpowerpvu = &g_xpowervu[dmuxid];
	if ((ecm[0xb] > rdr->powervu_ecmnb[dmuxid]) ||
		 ( rdr->powervu_ecmnb[dmuxid] == 255 && ecm[0xb] == 0) ||
		 ((rdr->powervu_ecmnb[dmuxid] - ecm[0xb]) > 5))
	{
		rdr->powervu_ecmnb[dmuxid] = ecm[0xb];
	}
	else
	{
		myprintf("########## skip:%02x:%02x\n", ecm[0xb], rdr->powervu_ecmnb[dmuxid]);
		return -1;
	}
	ecmCrc32 = b2i(4, ecm+ecmLen-4);
	myprintf("crc:%04x:%04x\n", ecmCrc32, dvb_crc32(ecm, ecmLen-4));
	if (dvb_crc32(ecm, ecmLen-4) != ecmCrc32) return 0;
	ecmLen -= 4;

	for(i=0; i<8; i++) {
		memset(convolvedCw[i], 0, 8);
	}

	for(i=3; i+3<ecmLen; ) {
		nanoLen = (((ecm[i] & 0x0f)<< 8) | ecm[i+1]);
		i +=2;
		if (nanoLen > 0)
		{
			nanoLen--;
		}
		nanoCmd = ecm[i++];
		if (i+nanoLen > ecmLen) return 0;
		myprintf("NANO:%02x\n", nanoCmd);
		switch (nanoCmd) {
			case 0x27:
				if (nanoLen < 15) break;
				nanoChecksum = 0;
				for(j=4; j<15; j++)
				{
					nanoChecksum += ecm[i+j];
				}

				if (nanoChecksum != 0) break;
				keyType = xpowervu_getConvcwIndex(ecm[i+4]);
				memcpy(convolvedCw[keyType], &ecm[i+6], 8);
				break;

			default:	break;
		}
		i += nanoLen;
	}
	MYEMU_TRACE("xpowervu:muxer{%d,%d)\n", er->chSets.frequency, er->chSets.degree);
	searchIds[1] = er->chSets.frequency | ((er->chSets.degree) << 16);
	searchIds[2] = er->chSets.frequency | ((er->chSets.degree/10) << 16);

	for(i=3; i+3<ecmLen; ) {
		nanoLen = (((ecm[i] & 0x0f)<< 8) | ecm[i+1]);
		i +=2;
		if (nanoLen > 0)
		{
			nanoLen--;
		}
		nanoCmd = ecm[i++];
		myprintf("nano:%02x\n", nanoCmd);
		if (i+nanoLen > ecmLen) return 0;

		switch (nanoCmd) {
			case 0x20:
				if (nanoLen < 54) break;
				csaUsed   =  _get_bit(ecm[i+7], 7);
				fixedKey  = !_get_bit(ecm[i+6], 5);
				oddKey    =  _get_bit(ecm[i+6], 4);
				bid       = (_get_bit(ecm[i+7], 1)<<1) | _get_bit(ecm[i+7], 0);
				sbox      =  _get_bit(ecm[i+6], 3);

				keyIndex  = (fixedKey<<3) | (bid<<2) | oddKey;
				channelId = b2i(2, ecm+i+23);
				ecmSrvid  = (channelId >> 4) | ((channelId & 0xF) << 12);
				searchIds[0] = channelId;
				searchIds[3] = (er->chSets.frequency << 16) | channelId;


				memcpy(ecmPart1, ecm+i+ 8, 14);
				memcpy(ecmPart2, ecm+i+27, 27);
				myprintf("ecm:chid:%04x, e6:%02x,%02x, cp:%02x,%02x \n", channelId, ecm[i+6], ecm[i+7], ecm[i+8], ecm[i+27]);

				decrypt_ok = 0;
				keyReference = -1;
				do
				{

					if (pxpowerpvu->found && pxpowerpvu->ppid == channelId && pxpowerpvu->knr == keyIndex)
					{
						memcpy(ecmKey, pxpowerpvu->keys, 7);
						MYEMU_TRACE("powervu:key:%06X:%02X..%02X(%2X)\n",
								pxpowerpvu->ppid, pxpowerpvu->keys[0], pxpowerpvu->keys[6], pxpowerpvu->knr);
					}
					else
					{
						if (!xpowervu_searchDeskey(rdr, searchIds, 4, keyIndex, ecmKey, &keyReference))
						{
							mycs_log("[Emu] Key not found: P %04X 0%X(%d)", ecmSrvid, keyIndex, keyReference);
							pxpowerpvu->found = 0;
							return -2;
						}
						memcpy(pxpowerpvu->keys, ecmKey, 7);
						MYEMU_TRACE("powervu:key found{%d}\n", keyReference);
						MYEMU_TRACE("powervu:key:%06X.%06X:%2X:%02X..%02X\n", channelId, ecmSrvid, keyIndex, ecmKey[0], ecmKey[6]);
					}
					xpowervu_decrypt(ecm+i+8, 14, ecmKey, sbox);
					if ((ecm[i+6] != ecm[i+6+7]) || (ecm[i+6+8] != ecm[i+6+15]))
					{
						memcpy(ecm+i+8, ecmPart1, 14);
						keyReference++;
						continue;
					}

					memcpy(tmpEcmKey, ecmKey, 7);

					xpowervu_decrypt(ecm+i+27, 27, ecmKey, sbox);
					if ((ecm[i+23] != ecm[i+23+29]) || (ecm[i+23+1] != ecm[i+23+30]))
					{
						memcpy(ecm+i+ 8, ecmPart1, 14);
						memcpy(ecm+i+27, ecmPart2, 27);
						keyReference++;
						continue;
					}

					decrypt_ok = 1;
					pxpowerpvu->knr = keyIndex;
					pxpowerpvu->ppid = channelId;
					pxpowerpvu->found = 1;
				}
				while(!decrypt_ok);

				memcpy(seedBase, ecm+i+6+2, 4);

				// Calculate only video seed
				//	memcpy(ecmKey, tmpEcmKey, 7);
				//	xpowervu_calculateSeed(PVU_CW_VID, ecm+i, seedBase, ecmKey, seed[PVU_CW_VID], sbox);
				for(j=0; j<8; j++)
				{
					memcpy(ecmKey, tmpEcmKey, 7);
					xpowervu_calculateSeed(j, ecm+i, seedBase, ecmKey, seed[j], sbox);
				}

				memcpy(baseCw, ecm+i+6+8, 7);

				// Calculate all cws
				for(j=0; j<8; j++)
				{
					xpowervu_calculateCw(j,  seed[j], csaUsed, convolvedCw[j], excw[j], baseCw);
					if (csaUsed)
					{
						for(k = 0; k < 8; k += 4) {
							excw[j][k + 3] = ((excw[j][k] + excw[j][k + 1] + excw[j][k + 2]) & 0xff);
						}
					}
				}

				if (cwEx != NULL)
				{
					cwEx->type = CW_MULTIPLE;
					if (csaUsed)
					{
						cwEx->algo = CA_ALGO_DVBCSA;
						cwEx->cipher = CA_MODE_ECB;
					}
					else
					{
						cwEx->algo = CA_ALGO_DES;
						cwEx->cipher = CA_MODE_ECB;
					}

					for(j=0; j<4; j++)
					{
						dwp = cwEx->audio[j];
						memset(dwp, 0, 16);
						if (ecm[0] == 0x80)
						{
							memcpy(dwp, excw[PVU_CW_A1+j], 8);
							if (csaUsed)
							{
								for(k = 0; k < 8; k += 4)
								{
									dwp[k + 3] = ((dwp[k] + dwp[k + 1] + dwp[k + 2]) & 0xff);
								}
							}
						}
						else
						{
							memcpy(&dwp[8], excw[PVU_CW_A1+j], 8);
							if (csaUsed)
							{
								for(k = 8; k < 16; k += 4)
								{
									dwp[k + 3] = ((dwp[k] + dwp[k + 1] + dwp[k + 2]) & 0xff);
								}
							}
						}
					}

					dwp = cwEx->data;
					memset(dwp, 0, 16);
					if (ecm[0] == 0x80)
					{
						memcpy(dwp, excw[PVU_CW_HSD], 8);
						if (csaUsed)
						{
							for(k = 0; k < 8; k += 4)
							{
								dwp[k + 3] = ((dwp[k] + dwp[k + 1] + dwp[k + 2]) & 0xff);
							}
						}
					}
					else
					{
						memcpy(&dwp[8], excw[PVU_CW_HSD], 8);
						if (csaUsed)
						{
							for(k = 8; k < 16; k += 4)
							{
								dwp[k + 3] = ((dwp[k] + dwp[k + 1] + dwp[k + 2]) & 0xff);
							}
						}
					}

					dwp = cwEx->vbi;
					memset(dwp, 0, 16);
					if (ecm[0] == 0x80)
					{
						memcpy(dwp, excw[PVU_CW_VBI], 8);
						if (csaUsed)
						{
							for(k = 0; k < 8; k += 4)
							{
								dwp[k + 3] = ((dwp[k] + dwp[k + 1] + dwp[k + 2]) & 0xff);
							}
						}
					}
					else
					{
						memcpy(&dwp[8], excw[PVU_CW_VBI], 8);
						if (csaUsed)
						{
							for(k = 8; k < 16; k += 4)
							{
								dwp[k + 3] = ((dwp[k] + dwp[k + 1] + dwp[k + 2]) & 0xff);
							}
						}
					}
				}

				memset(cw, 0, 16);
				if (ecm[0] == 0x80)
				{
					memcpy(cw, excw[PVU_CW_VID], 8);
					if (csaUsed)
					{
						for(k = 0; k < 8; k += 4)
						{
							cw[k + 3] = ((cw[k] + cw[k + 1] + cw[k + 2]) & 0xff);
						}
					}
				}
				else
				{
					memcpy(&cw[8], excw[PVU_CW_VID], 8);
					if (csaUsed)
					{
						for(k = 8; k < 16; k += 4)
						{
							cw[k + 3] = ((cw[k] + cw[k + 1] + cw[k + 2]) & 0xff);
						}
					}
				}
				er->cwdesalgo = csaUsed ? 0 : 1;
				return 1;

			default:	break;
		}
		i += nanoLen;
	}

	return -1;
}


int
XPVU_Process(struct s_reader *reader, ECM_REQUEST *er, uint8_t *cw, CWEXTENTION *cwEx)
{
	int dmuxid;
	int cwfound;

	if (!er) return 0;
	if (!reader) return 0;
	if (!IS_POWERVU(er->caid)) return 0;
	dmuxid = er->dmuxid % MAX_DEMUX;
	cwfound = xpowervu_ecmProcess(reader, er, cw, cwEx);
	if (cwfound == 1)
	{
		g_xpowervu[dmuxid].failure = 0;
		MYEMU_TRACE("xpowervu:cw{%d}{%02X...%02X,%02X...%02X}(%d)\n", dmuxid, cw[0], cw[7], cw[8], cw[15], er->cwdesalgo);
		return 1;
	}
	if (cwfound < 0)
	{
		MYEMU_TRACE("xpowervu:cwfailure{%d}\n", g_xpowervu[dmuxid].failure);
		if (++g_xpowervu[dmuxid].failure < 3) return -1;
	}
	g_xpowervu[dmuxid].found = 0;
	g_xpowervu[dmuxid].failure = 0;
	return 0;
}


int
XPVU_EmmProcess(struct s_reader *rdr, unsigned char *emm, int emmLen)
{
	return 1;
}


void
XPVU_Cleanup(struct s_reader *rdr)
{
	MYEMU_TRACE("xpowervu:clean\n");
	memset(rdr->powervu_ecmnb,0,sizeof(rdr->powervu_ecmnb));
	memset(&g_xpowervu,0,sizeof(g_xpowervu));
}
#endif	// defined(__XCAS_POWERVU__)
#endif	// defined(MODULE_XCAS)

