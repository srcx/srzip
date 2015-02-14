/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* bitfile.c
 * rutiny pro vstup a vystup bitu
 */

#include "bitfile.h"

/* zruseni ladicich vypisu */
#undef DBG
#define DBG(x)

#ifdef __cplusplus
extern "C" {
#endif

#define BF_MASK		(1 << (BITS_IN_CHAR - 1))	/* hodnota nejvyznamejsiho bitu */
#define BFILES_NUM	16	/* max. pocet bitovych souboru */

/* privatni promenne */

/* pocet zapsanych / prectenych bytu z vnitrniho bufferu */
int	bf_bytes = 0;

/* pole otevrenych souboru */
PRIVATE BFILE	bfiles[BFILES_NUM];

/* inicializace */
int	bfinit (void) {

  PDBG("BF::bfinit\n");

  memset (bfiles, 0, sizeof (bfiles));

  return 0;
}

/* otevreni a zavreni */

/* asociace s otevrenym souborem */
BFILE	*bfasoc (FILE *f, int w) {
  int i;

  PDBG("BF::bfasoc = ");

  /* nalezeni volneho mista v tabulce */
  for (i = 0; i < BFILES_NUM; i++)
    if (!bfiles[i].f) {
      bfiles[i].f = f;
      bfiles[i].buf = 0;
      bfiles[i].mask = w ? BF_MASK : 0;	/* maska se lisi pri cteni a zapisu */
      PDBG("%p\n", &bfiles[i]);
      return &bfiles[i];
    }

  PDBG("NULL\n");
  return NULL;
}

/* odpojeni od otevreneho souboru */
int	bfdetach (BFILE *bf) {

  PDBG("BF::bfdetach (%p) = ", bf);

  if (bf) {
    bf->f = NULL;
    PDBG("0\n");
    return 0;
  }

  PDBG("-1\n");
  return -1;
}

/* vyprazdneni bitoveho bufferu - pro zapis */
int	bfflush (BFILE *bf, crc_t *crc) {

  PDBG("BF::bfflush (%p) = ", bf);

  if (bf->mask < BF_MASK) {
    while (bf->mask) {	/* posun bufferu tak, aby byl od vyssich bitu vyplnen nulami */
      bf->buf >>= 1; bf->mask >>= 1;
    }
    PDBG("fputc (%x)\n", bf->buf);
    bf_bytes++;
    return (fputc_crc (bf->buf, bf->f, crc) == EOF) ? -1 : (bf->mask = BF_MASK, bf->buf = 0);	/* zapis bufferu */
  }

  PDBG("0\n");
  return 0;
}

/* vycisteni bitoveho bufferu*/
void	bfclear (BFILE *bf, int w) {

  PDBG("BF::bfclear (%p)\n", bf);

  bf->mask = w ? BF_MASK : 0; bf->buf = 0;
}

/* cteni a zapis */

/* zapis JEDNOHO bitu */
/* vraci zapsany bit nebo EOF */
int	bfputb (BFILE *bf, int bit, crc_t *crc) {

  PDBG("BF::bfputb (%p, %d) { ", bf, bit);

  bf->buf >>= 1;
  if (bit) bf->buf |= BF_MASK;	/* buffer je naplnovan smerem od nizsich bitu k vyssim */
  PDBG("bf->buf = %x } = ", bf->buf);
  if (!(bf->mask >>= 1)) {	/* buffer je plny - zapis */
    bf->mask = BF_MASK;
    PDBG ("fputc\n");
    bf_bytes++;
    return (fputc_crc (bf->buf, bf->f, crc) == EOF) ? EOF : (bf->buf = 0, bit);
  }

  PDBG ("%d\n", bit);

  return bit;
}

/* cteni JEDNOHO bitu */
/* vraci precteny bit nebo EOF */
int	bfgetb (BFILE *bf, crc_t *crc) {
  int bit;

  PDBG ("BF::bfgetb (%p) { ", bf);

  if (!bf->mask) {	/* buffer je prazdny - cteni */
    PDBG ("fgetc ");
    bf_bytes++;
    if ((bf->buf = fgetc_crc (bf->f, crc)) == EOF)
      { PDBG("} = EOF\n"); return EOF; }
    else
      bf->mask = BF_MASK;
  }

  bit = bf->buf & 1;	/* cteni probiha od vyssich bitu k nizsim - opak zapisu */
  bf->buf >>= 1;
  bf->mask >>= 1;

  PDBG ("bf->buf = %x bf->mask = %x } = %d\n", bf->buf, bf->mask, bit);

  return bit;
}

/* zapis posloupnosti bitu */
/* vraci pocet zapsanych bitu nebo EOF */
int	bfwriteb (BFILE *bf, uchar *bits, int len, crc_t *crc) {
  int i, j, te; uchar b;

  PDBG("BF::bfwriteb (%p, %p, %d) = ", bf, bits, len);

  bf_bytes = 0;
  for (i = 0; i < (len / BITS_IN_CHAR); i++)	/* zapis celistvych bytu na zacatku pole */
    for (j = BF_MASK, b = bits[i]; j; j >>= 1, b >>= 1)
      if ((bfputb (bf, b & 1, crc) == EOF)) { PDBG("EOF\n"); return EOF; }

  te = (BF_MASK >> (BITS_IN_CHAR - (len % BITS_IN_CHAR)));	/* zapis neuplneho bytu na konci - vyznamne bity jsou od nizsich bitu k vyssim */
  b = (bits[i] >> (BITS_IN_CHAR - (len % BITS_IN_CHAR)));
  for (j = te; j; j >>= 1, b >>= 1)
    if ((bfputb (bf, b & 1, crc) == EOF)) { PDBG("EOF\n"); return EOF; }

  PDBG("%d\n", len);

  return len;
}

/* cteni posloupnosti bitu */
/* vraci pocet prectenych bitu */
int	bfreadb (BFILE *bf, uchar *bits, int len, crc_t *crc) {
  int b, i, j, l;

  PDBG("BF::bfreadb (%p, %p, %d) = ", bf, bits, len);

  bf_bytes = 0;
  j = BF_MASK; l = i = 0; bits[0] = 0;
  while ((l < len) && ((b = bfgetb (bf, crc)) != EOF)) {	/* cteni probiha od vyssich bitu k nizsim - opak zapisu */
    if (b) bits[i] |= BF_MASK;
    if (!(j >>= 1)) {
      j = BF_MASK; i++; bits[i] = 0;
    } else
      bits[i] >>= 1;
    l++;
  }

  PDBG ("%d\n", l);

  return l;
}

/* zapis bitu do bufferu */
/* vraci novou delku */
int	bfaddb (uchar buf[], int len, int bit) {
  uchar *c;

  PDBG("BF::bfaddb (%p, %d, %d) = ", buf, len, bit);

  c = &buf[len++ / BITS_IN_CHAR];

  *c >>= 1;
  if (bit) *c |= BF_MASK;

  PDBG("%d\n", len);

  return len;
}

/* obnoveni puvodniho stavu */
#undef DBG
#define DBG(x)	ORIG_DBG(x)

#ifdef __cplusplus
}
#endif

