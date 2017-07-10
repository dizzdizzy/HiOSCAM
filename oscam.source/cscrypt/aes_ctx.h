// sky(n)
#ifndef __AES_CTX_H
#define __AES_CTX_H
#if !defined(WITH_LIBCRYPTO)
#define AES_FIXED_TABLES

typedef struct
{
	unsigned int 	erk[64];	 /* encryption round keys */
	unsigned int 	drk[64];	 /* decryption round keys */
	unsigned char  	nr; 		 /* number of rounds	  */
} AES_CTX_t;

void	AES_CTX_SetKey	(AES_CTX_t *ctx_p, unsigned char *Aeskey_p);
void	AES_CTX_Decrypt	(AES_CTX_t *ctx_p, unsigned char *In_p, unsigned char *Out_p);
void	AES_CTX_Encrypt	(AES_CTX_t *ctx_p, unsigned char *In_p, unsigned char *Out_p);
#endif	// #if !defined(WITH_LIBCRYPTO)
#endif	// #ifndef __AES_CTX_H

