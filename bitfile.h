/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* bitfile.h
 * rutiny pro vstup a vystup bitu
 */

#ifndef __BITFILE_H
#define __BITFILE_H

#include <stdio.h>
#include <string.h>

#include "global.h"
#include "crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* bitovy soubor */
typedef struct {
  FILE *f;
  int buf;
  uchar mask;
} BFILE;

/* definice pro w parametr v bfasoc */
#define BF_READ		0	/* zapis */
#define BF_WRITE	1	/* cteni */

/* pocet zapsanych / prectenych bytu z vnitrniho bufferu */
extern int	bf_bytes;

/* inicializace */
int	bfinit (void);

/* otevreni a zavreni */

/* asociace s otevrenym souborem */
BFILE	*bfasoc (FILE *f, int w);

/* odpojeni od otevreneho souboru */
int	bfdetach (BFILE *bf);

/* vyprazdneni bitoveho bufferu */
INLINE int	bfflush (BFILE *bf, crc_t *crc);

/* vycisteni bitoveho bufferu */
INLINE void	bfclear (BFILE *bf, int w);

/* cteni a zapis */

/* zapis JEDNOHO bitu */
/* vraci zapsany bit nebo EOF */
INLINE int	bfputb (BFILE *bf, int bit, crc_t *crc);

/* cteni JEDNOHO bitu */
/* vraci precteny bit nebo EOF */
INLINE int	bfgetb (BFILE *bf, crc_t *crc);

/* zapis posloupnosti bitu */
/* vraci pocet zapsanych bitu nebo EOF */
INLINE int	bfwriteb (BFILE *bf, uchar *bits, int len, crc_t *crc);

/* cteni posloupnosti bitu */
/* vraci pocet prectenych bitu */
INLINE int	bfreadb (BFILE *bf, uchar *bits, int len, crc_t *crc);

/* zapis bitu do bufferu */
/* vraci novou delku */
INLINE int	bfaddb (uchar buf[], int len, int bit);

#ifdef __cplusplus
}
#endif

#endif
