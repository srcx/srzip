/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_cpy.h
 * komprimacni metoda cpy (prosta kopie)
 */

#ifndef __M_CPY_H
#define __M_CPY_H

#include <stdio.h>

#include "global.h"
#include "crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int enc_cpy (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii);

/* dekompresni fce */
/* par. : z, do, kolik_z, uroven, CRC, ascii mod */
int dec_cpy (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii);

#ifdef __cplusplus
}
#endif

#endif