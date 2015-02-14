/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_bwt.h
 * komprimacni metoda bwt (Burrows-Wheelerova transformace + pomocne algoritmy)
 */

#ifndef __M_BWT_H
#define __M_BWT_H

#include <stdio.h>

#include "global.h"
#include "crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BWT */
/* par. : vstupni_buffer, vystupni_buffer, delka_bufferu
 * vraci pozici puvodniho bufferu v setridenem
 * out_buf musi byt o 1 vetsi nez buf_len (kvuli EOF znaku)
 */
long do_bwt (uchar *in_buf, uchar *out_buf, long buf_len);

/* reverzni BWT */
/* par. : vstupni buffer, pozice puvodniho bufferu v setridenem, vystupni buffer, delka bufferu, buffer pro transformacni vektor
 * poslednim znakem v out_buf je EOF, ktery se ignoruje
 */
void rev_bwt (uchar *in_buf, long op, uchar *out_buf, long buf_len, long *tbuf);

/* MTF */
/* par. : buffer, delka bufferu */
void do_mtf (uchar *buf, long len);

/* reverzni MTF */
/* par. : buffer, delka bufferu */
void rev_mtf (uchar *buf, long len);

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int enc_bwt (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii);

/* dekompresni fce */
/* par. : z, do, kolik_z, uroven, CRC, ascii mod */
int dec_bwt (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii);

#ifdef __cplusplus
}
#endif

#endif