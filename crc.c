/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* crc.c
 * vypocet CRC 32
 */

#include "crc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRC32_POLYNOMIAL   0xEDB88320L

/* tabulka koeficientu pro CRC32 */
PRIVATE crc_t	crc_tab[256];

/* init - zbudovani CRC tabulky */
void	init_crc (void) {
  int i, j; crc_t v;

  for (i = 0; i <= 255; i++) {
    v = i;
    for (j = 8; j > 0; j--) {
      if (v & 1)
        v = (v >> 1) ^ CRC32_POLYNOMIAL;
      else
        v >>= 1;
    }
    crc_tab[i] = v;
  }
}

/* pridani k CRC */
crc_t	add_crc (crc_t crc, uchar byte) {

  return ((crc >> 8) & 0x00FFFFFFL) ^ (crc_tab [((int)crc ^ byte) & 0xff]);
}

/* zapis bytu do souboru spolu s vypoctem CRC */
INLINE int fputc_crc (int character, FILE *file, crc_t *crc) {

  *crc = add_crc (*crc, character);
  return fputc (character, file);
}

/* cteni bytu ze souboru spolu s vypoctem CRC */
INLINE int fgetc_crc (FILE *file, crc_t *crc) {

  int c = fgetc (file);
  if (c != EOF) *crc = add_crc (*crc, c);
  return c;
}

/* zapis bufferu do souboru spolu s vypoctem CRC */
INLINE size_t fwrite_crc (void *buffer, size_t size, size_t number, FILE *file, crc_t *crc) {
  uint i;

  for (i = 0; i < size * number; i++)
    *crc = add_crc (*crc, ((uchar *)buffer)[i]);
  return fwrite (buffer, size, number, file);
}

/* cteni bufferu ze souboru spolu s vypoctem CRC */
INLINE size_t fread_crc (void *buffer, size_t size, size_t number, FILE *file, crc_t *crc) {
  uint i, r = fread (buffer, size, number, file);

  for (i = 0; i < size * r; i++)
    *crc = add_crc (*crc, ((uchar *)buffer)[i]);
  return r;
}

#ifdef __cplusplus
}
#endif
