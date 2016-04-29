#ifndef MD5_H
#define MD5_H

#include "../p2pcommon/base/common.h"

typedef unsigned int php_uint32;

/* MD5 context. */
typedef struct _MD5_CTX
{
	php_uint32 state[4];				/* state (ABCD) */
	php_uint32 count[2];				/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];	/* input buffer */
}MD5_CTX;

void MD5_init(MD5_CTX *);
void MD5_update(MD5_CTX *, const unsigned char *, unsigned int);
void MD5_final(unsigned char[16], MD5_CTX *);
void MD5_hash(char *MD5,char *instr);
bool MD5_file_hash(char* MD5, char* pszFileName);

std::string MD5_to_string(unsigned char md5[16]);

#endif
