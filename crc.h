/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* crc.h
 * vypocet CRC 32
 */

#ifndef __CRC_H
#define __CRC_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CRC inicializacni a porovnavaci hodnota */
#define CRC_INIT	0xFFFFFFFFL

/* init - zbudovani CRC tabulky */
void	init_crc (void);

/* pridani k CRC */
INLINE crc_t	add_crc (crc_t crc, uchar byte);

/* zapis bytu do souboru spolu s vypoctem CRC */
INLINE int fputc_crc (int character, FILE *file, crc_t *crc);

/* cteni bytu ze souboru spolu s vypoctem CRC */
INLINE int fgetc_crc (FILE *file, crc_t *crc);

/* zapis bufferu do souboru spolu s vypoctem CRC */
INLINE size_t fwrite_crc (void *buffer, size_t size, size_t number, FILE *file, crc_t *crc);

/* cteni bufferu ze souboru spolu s vypoctem CRC */
INLINE size_t fread_crc (void *buffer, size_t size, size_t number, FILE *file, crc_t *crc);

#ifdef __cplusplus
}
#endif

#endif