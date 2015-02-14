/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_ahc.h
 * komprimacni metoda ahc (adaptivni huffmanovo kodovani)
 */

#ifndef __M_AHC_H
#define __M_AHC_H

#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "bitfile.h"
#include "crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pocet hodnot pro uchar */
#define CHAR_COUNT	(1 << BITS_IN_CHAR)

/* symbol konce dat */
#define SYMBOL_EOD	(CHAR_COUNT + 1)

/* inicializace stromu */
/* par. : kompresni level */
void ahc_init_tree (long level);

/* zakodovany zapis znaku do souboru */
/* vraci pocet skutecne zapsanych bytu */
INLINE int ahc_putc (BFILE *f, int c, crc_t *crc);

/* zakodovany zapis bufferu do souboru */
/* vraci pocet skutecne zapsanych bytu */
INLINE long ahc_fwrite (void *buf, long len, BFILE *f, crc_t *crc);

/* precteni znaku ze souboru */
/* vraci pocet skutecne prectenych bytu */
INLINE int ahc_getc (BFILE *f, int *c, crc_t *crc);

/* vypusteni pripadnych prebytku z bufferu */
/* vraci pocet skutecne zapsanych bytu */
INLINE int ahc_flush (BFILE *f, crc_t *crc);

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int enc_ahc (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii);

/* dekompresni fce */
/* par. : z, do, kolik_z, uroven, CRC, ascii mod */
int dec_ahc (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii);

#ifdef __cplusplus
}
#endif

#endif