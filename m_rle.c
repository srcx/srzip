/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_rle.c
 * komprimacni metoda rle (RLE)
 */

#include "m_rle.h"

#ifdef __cplusplus
extern "C" {
#endif

/* tvar dat :
   ([<ridici bit><delka>] <data>)
   ridici bit == 0 -> nasleduje <delka> bytu dat
                 1 -> nasleduje jeden byte opakovany <delka> krat
 */

/* escape maska pro priznak sekvence */
#define	RLE_SEQ_MASK	0x80
/* mez pro len */
#define RLE_MAXLEN	127

PRIVATE int	in_seq;	/* v sekvenci ? */
PRIVATE int	last;	/* posledni byte */
PRIVATE int	len;	/* pocet opakovani posledniho bytu (0 = 1x) */
PRIVATE char	buf[RLE_MAXLEN+1];	/* sekvenci buffer */

/* zapise to, co je v last a len */
/* vraci pocet skutecne zapsanych bytu */
/* -1 = chyba */
INLINE PRIVATE int	rle_flush (FILE *f, crc_t *crc) {
  int r = 0, i;

  if (len > -1) {
    if (in_seq) {
      if (fputc_crc (len | RLE_SEQ_MASK, f, crc) == EOF) return -1;
      if (fputc_crc (last, f, crc) == EOF) return -1;
      r += 2;
      len = -1;
    } else {
      if (fputc_crc (len, f, crc) == EOF) return -1;
      r += len + 2;
      for (i = 0; i <= len; i++) { 
        if (fputc_crc (buf[i], f, crc) == EOF) return -1;
      }
      len = -1;
    }
  }

  return r;
}

/* zapis bytu */
/* vraci pocet skutecne zapsanych bytu (v dany okamzik) */
INLINE PRIVATE int	rle_putc (FILE *f, int c, crc_t *crc) {
  int r = 0;

  /* len = -1 -> prazdny */
  if (len == -1) {
    in_seq = 0;
    len = 0;
    last = buf[0] = c;
  } else if (in_seq) {
    if (((last == c) && (len == RLE_MAXLEN)) || (last != c)) {
      r = rle_flush (f, crc);
      in_seq = 0;
      len = 0;
      last = buf[0] = c;
    } else
      len++;
  } else {
    if (last == c) {
      len--;
      r = rle_flush (f, crc);
      in_seq = 1;
      len = 1;
      last = buf[0] = c;
    } else {
      if (len == RLE_MAXLEN) {
        r = rle_flush (f, crc);
        in_seq = 0;
        len = 0;
        last = buf[0] = c;
      } else {
        last = buf[++len] = c;
      }
    }
  }

  return r;
}

/* cteni bytu */
/* vraci pocet skutecne prectenych bytu (v dany okamzik) */
INLINE PRIVATE int	rle_getc (FILE *f, int *c, crc_t *crc) {
  int r = 0;

  if (len == -1) {	/* cteni dalsich */
    if ((len = fgetc_crc (f, crc)) == EOF) return -1;
    r++;
    in_seq = ((len & RLE_SEQ_MASK) != 0);
    len &= ~RLE_SEQ_MASK;
    if ((last = fgetc_crc (f, crc)) == EOF) return -1;
    r++;
  } else if (!in_seq) {	/* cteni primych dat */
    if ((last = fgetc_crc (f, crc)) == EOF) return -1;
    r++;
  }
  if (len >= 0)
    len--;

  *c = last;

  return r;
}

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, verb_str, ascii mod */
int enc_rle (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; int l;

  len = -1; *crc = CRC_INIT; *orig_len = *enc_len = 0; level = level;
  while ((c = fgetc (fr)) != EOF) {
    (*orig_len)++;
    if (ascii) {
      c2 = MACHINE_TO_UNIX_EOL (c, &s[0], &s[1]);
      for (c = 0; c < c2; c++) {
        if ((l = rle_putc (fw, s[c], crc)) == -1) return M_ERR_OUTPUT;
        *enc_len += l;
      }
    } else {
      if ((l = rle_putc (fw, c, crc)) == -1) return M_ERR_OUTPUT;
      *enc_len += l;
    }
  }

  if ((l = rle_flush (fw, crc)) == -1) return M_ERR_OUTPUT;
  *enc_len += l;

  return 0;
}

/* dekompresni fce */
/* par. : z, do, kolik, uroven, CRC, verb_str, ascii mod */
int dec_rle (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; int l = 0;

  len = -1; *crc = CRC_INIT; level = level; enc_len = enc_len;
  while (orig_len && ((l = rle_getc (fr, &c, crc)) != -1)) {
    orig_len--;
    if (ascii) {
      c2 = UNIX_TO_MACHINE_EOL (c, &s[0], &s[1]);
      if (c2 && !fwrite (s, c2, 1, fw)) return M_ERR_OUTPUT;
    } else {
      if (fputc (c, fw) == EOF) return M_ERR_OUTPUT;	/* nejde zapsat */
    }
  }

  if (l < 0)
    return M_ERR_DAMAGE;	/* EOF by nemel nastat */

  return 0;
}

#ifdef __cplusplus
}
#endif
