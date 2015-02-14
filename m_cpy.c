/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_cpy.c
 * komprimacni metoda cpy (prosta kopie)
 */

#include "m_cpy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, verb_str, ascii mod */
int enc_cpy (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2];

  *crc = CRC_INIT; *orig_len = *enc_len = 0; level = level;
  while ((c = fgetc (fr)) != EOF) {
    (*orig_len)++;
    if (ascii) {
      c2 = MACHINE_TO_UNIX_EOL (c, &s[0], &s[1]);
      *enc_len += c2;
      if (c2 && !fwrite_crc (s, c2, 1, fw, crc)) return M_ERR_OUTPUT;
    } else {
      (*enc_len)++;
      if (fputc_crc (c, fw, crc) == EOF) return M_ERR_OUTPUT;	/* nejde zapsat */
    }
  }

  return 0;
}

/* dekompresni fce */
/* par. : z, do, kolik, uroven, CRC, verb_str, ascii mod */
int dec_cpy (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii) {
  int c = 0, c2; char s[2];

  *crc = CRC_INIT; level = level; orig_len = orig_len;
  while ((enc_len--) && ((c = fgetc_crc (fr, crc)) != EOF)) {
    if (ascii) {
      c2 = UNIX_TO_MACHINE_EOL (c, &s[0], &s[1]);
      if (c2 && !fwrite (s, c2, 1, fw)) return M_ERR_OUTPUT;
    } else {
      if (fputc (c, fw) == EOF) return M_ERR_OUTPUT;	/* nejde zapsat */
    }
  }

  if (c < 0)
    return M_ERR_DAMAGE;	/* EOF by nemel nastat */

  return 0;
}

#ifdef __cplusplus
}
#endif
