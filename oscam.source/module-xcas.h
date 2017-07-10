#ifndef MODULE_XCAS_H_
#define MODULE_XCAS_H_
#if defined(MODULE_XCAS)
//
//
//
#if 1
	#define	MYXCAS_TRACE	myprintf
	#define	MYEMU_TRACE		myprintf
#else
	#define	MYXCAS_TRACE(...)
	#define	MYEMU_TRACE(...)
#endif
//
//
//
//
//
//
	#define	__XCAS_BISS__
	#define	__XCAS_VIACESS__
	#define	__XCAS_SEKA__
	#define	__XCAS_CRYPTOWORKS__
	#define	__XCAS_POWERVU__	/* sky(2016.03.07) */

// futures...
//	#define	__XCAS_XIRDETO__
//	#define	__XCAS_XBETACRYPT__
//	#define	__XCAS_XNAGRA__
//	#define	__XCAS_XCONAX__
//	#define	__XCAS_XNDS__
//
//	#define	__XCAS_AUTOROLL__	// E00,500:030B00 /* sky(powervu) */
//
//
//
//
//
#define MAXMULTIKEY			32

#define CASS_NAGRA			'N'
#define CASS_VIACCESS		'V'
#define CASS_IRDETO			'I'
#define CASS_SEKA				'S'
#define CASS_CRYPTOWORKS	'W'
#define CASS_BISS				'B'
#define CASS_CONSTANT		'F'
#define CASS_POWERVU			'P'

#define EMU_BISS				0x1
#define EMU_VIACESS			0x2
#define EMU_CRYPTOWORKS		0x4
#define EMU_SEKA				0x8
#define EMU_CONSTANT			0x10
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
int	XEMUKEY_Initialize	(struct s_reader *rdr);
void	XEMUKEY_Cleanup		(struct s_reader *rdr); // sky(powervu)
bool	XEMUKEY_IsExistance	(struct s_reader *rdr, uint8_t  cCAS, uint32_t ppid);
int	XEMUKEY_ChLfsSearch	(struct s_reader *rdr, uint8_t  cCAS, uint32_t frequency, uint32_t symrate, uint8_t *kbufs, uint16_t klenn);
int	XEMUKEY_SpecialSearch(struct s_reader *rdr, uint8_t  cCAS, uint32_t ppid, char *kstr,  uint8_t *kbufs, uint16_t klenn);
int	XEMUKEY_MultiSearch	(struct s_reader *rdr, uint8_t  cCAS, uint32_t ppid, uint8_t knr, uint8_t *kbufs, uint16_t klenn);
int	XEMUKEY_Searchkey		(struct s_reader *rdr, uint8_t  cCAS, uint32_t ppid, uint8_t knr, uint8_t *kbufs, uint16_t klenn);
int	XEMUKEY_IndexedSearch(struct s_reader *rdr, uint8_t  cCAS, uint32_t ppid, uint8_t knr, uint8_t *kbufs, uint16_t klenn, int *pfoundindex);
int	XEMUKEY_IsAvailable	(struct s_reader *rdr, uint16_t caid, uint32_t prid);
int	XEMUKEY_PidsSearch	(struct s_reader *rdr, uint8_t  cCAS, uint32_t *ppids, uint32_t ppnum, uint8_t knr, uint8_t *kbufs, uint16_t klenn, int *pfoundindex);
int	XEMUKEY_Savekey      (struct s_reader *rdr, uint8_t  cCAS, uint32_t ppid, char *kNstr, uint8_t *kBufs,	uint16_t ksize);
// viacess
// biss
// cryptoworks
// seca
#if defined(__XCAS_VIACESS__)
int	XVIACESS_EmmProcess	(struct s_reader *rdr, unsigned char *emm, int emmlen, unsigned long provid);
int	XVIACESS_Process		(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw);
void	XVIACESS_Cleanup		(void);
#endif
#if defined(__XCAS_BISS__)
int	XBISS_Process			(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw);
void	XBISS_Cleanup			(void);
#endif
#if defined(__XCAS_SEKA__)
int	XSEKA_Process			(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw);
void	XSEKA_Cleanup			(void);;
#endif
#if defined(__XCAS_CRYPTOWORKS__)
int	XCRYPTOWORKS_Process	(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw);
void	XCRYPTOWORKS_Cleanup	(void);
#endif
#if defined(__XCAS_CRYPTOWORKS__)
int	XPVU_Process			(struct s_reader *rdr, ECM_REQUEST *er, uint8_t *cw, CWEXTENTION *cwEx);
int	XPVU_EmmProcess		(struct s_reader *rdr, unsigned char *emm, int emmlen);
void	XPVU_Cleanup			(struct s_reader *rdr);
#endif
// sky(powervu)
bool	xcas_IsAuAvailable	(struct s_reader *rdr, uint16_t casid, uint32_t prid);
bool	xcas_IsEmmAvailable	(struct s_reader *rdr, EMM_PACKET *ep);

#endif	// defined(MODULE_XCAS)
#endif	// #ifndef MODULE_XCAS_H_

