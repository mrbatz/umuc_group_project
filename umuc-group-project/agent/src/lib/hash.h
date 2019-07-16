/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

/* Edits made by me are highlighted below. */

#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
    BYTE data[64];
    WORD datalen;
    unsigned long long bitlen;
    WORD state[8];
} SHA256_CTX_t;

/*********************** FUNCTION DECLARATIONS **********************/
extern void sha256_init(SHA256_CTX_t *ctx);
extern void sha256_update(SHA256_CTX_t *ctx, const BYTE data[], size_t len);
extern void sha256_final(SHA256_CTX_t *ctx, BYTE hash[]);



//My additions
#include "tasks/dirwalk.h"
#define SHA256_LENGTH 32

extern int sha256sum_file(char filepath[MAX_PATH_LENGTH], unsigned char hash[SHA256_LENGTH]);

#endif   // SHA256_H
