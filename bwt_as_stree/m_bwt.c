/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_bwt.c
 * komprimacni metoda bwt (Burrows-Wheelerova transformace + pomocne algoritmy)
 */

#include "m_bwt.h"
#include "m_ahc.h"
#include "stree.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

/* prirustek velikosti bufferu */
#define BUF_DELTA	100000

/* BWT */
/* par. : vstupni_buffer, vystupni_buffer, delka_bufferu, buffer pro bwt ukazatele
 * vraci pozici puvodniho bufferu v setridenem
 * out_buf musi byt o 1 vetsi nez buf_len (kvuli EOF znaku)
 */
long do_bwt (uchar *in_buf, uchar *out_buf, long buf_len) {
  long i, op = 0; stree_t *tree; stree_node_t *node, *nn;
  
#ifdef SRZIP_DEBUG
  PDBG ("\nKruhovy buffer :\n\n");
  if (buf_len > 99)
    PDBG ("Prilis mnoho dat k zobrazeni\n");
  else {
  for (i = 0; i <= buf_len; i++) {
    long j;
    PDBG ("%2ld ", i);
    for (j = i; j < buf_len; j++) {
      PDBG ("%c", in_buf[j]);
    }
    PDBG ("~");
    for (j = 0; j < i; j++) {
      PDBG ("%c", in_buf[j]);
    }
    PDBG ("\n");
  }
  }
#endif

  /* suffixovy strom */
  tree = stree_init (in_buf, buf_len, 1);
  if (!tree) return M_ERR_MEM;

  /* ulozeni do vystupniho bufferu */
  node = stree_root (tree);
  { uchar *pp = out_buf, *p;
#ifdef SRZIP_DEBUG
    PDBG("\nSetrideny buffer :\n\n   F     L\n");
    if (buf_len > 99)
      PDBG ("Prilis mnoho dat k zobrazeni\n");
#endif
    i = 0;
    while (node) {
      p = stree_suffix (tree, node);
      if (p) {
        if (p == in_buf) {
          op = i;
          *pp++ = '~';	/* EOF znak */
        } else {
          *pp++ = *(p-1);
        }
#ifdef SRZIP_DEBUG
        if (buf_len < 100)
          PDBG ("%2ld %c ... %c (%2ld)\n", i, (i < buf_len) ? *p : '~', *(pp-1), (long)(p - in_buf));
#endif
        i++;
      }
      nn = stree_child (tree, node);
      if (!nn) nn = stree_next (tree, node);
      while (!nn) {
        node = nn = stree_parent (tree, node);
        if (!nn) break;
        nn = stree_next (tree, nn);
      }
      node = nn;
    }
  }
  
  stree_done (tree);
  
#ifdef SRZIP_DEBUG
  PDBG ("\nVystup :\n\n(\"");
  if (buf_len > 99)
    PDBG ("Prilis mnoho dat k zobrazeni\n");
  else {
  for (i = 0; i <= buf_len; i++) {
    PDBG ("%c", out_buf[i]);
  }
  }
  PDBG("\", %ld)\n", op);
#endif

  return op;
}

/* reverzni BWT */
/* par. : vstupni buffer, pozice puvodniho bufferu v setridenem, vystupni buffer, delka bufferu, buffer pro transformacni vektor
 * poslednim znakem v out_buf je EOF, ktery se ignoruje
 */
void rev_bwt (uchar *in_buf, long op, uchar *out_buf, long buf_len, long *tbuf) {
  long bc[257], bp[257], l; uchar *pp; long *pt = tbuf; int i;
  
  /* naplneni poli bc (pocet danych znaku) a bp (poradi prvniho daneho znaku) */
  for (i = 0; i < 257; i++) {
    bc[i] = bp[i] = 0;
  }
  pp = in_buf;
  for (l = 0; l < buf_len; l++) {
    bc[(l == op) ? 256 : *pp]++;
    pp++;
  }
  bp[0] = 0;
  for (i = 1; i < 257; i++) {
    bp[i] = bp[i - 1] + bc[i - 1];
  }
  
#ifdef SRZIP_DEBUG
  PDBG ("\nPocet a poradi znaku v setridenem vektoru F :\n\n");
  if (buf_len > 99)
    PDBG ("Prilis mnoho dat k zobrazeni\n");
  else {
  for (i = 0; i < 257; i++) {
    if (bc[i]) {
      if (isprint (i)) PDBG ("%3c", i); else PDBG ("%3d", i);
      PDBG (" %2ld %2ld\n", bc[i], bp[i]);
    }
  }
  }
#endif
  
  /* konstrukce transformacniho vektoru */
  pp = in_buf; pt = tbuf;
  for (l = 0; l < buf_len; l++) {
    *pt++ = bp[(l == op) ? 256 : *pp]++;
    pp++;
  }
  
#ifdef SRZIP_DEBUG
  PDBG ("\nTransformacni vektor :\n\n   F L T\n");
  if (buf_len > 99)
    PDBG ("Prilis mnoho dat k zobrazeni\n");
  else {
    uchar c = 0;
    for (l = 0; l < buf_len; l++) {
      while (!bc[c]) c++;
      PDBG ("%2ld %c %c %ld\n", l, c--, in_buf[l], tbuf[l]);
    }
  }
#endif

  /* zapis do vystupniho bufferu */
  pp = out_buf + buf_len - 1; l = buf_len;
  while (l--) {
    *pp-- = in_buf[op];
    op = tbuf[op];
  }

#ifdef SRZIP_DEBUG
  PDBG ("\nVystup :\n\n");
  if (buf_len > 99)
    PDBG ("Prilis mnoho dat k zobrazeni\n");
  else {
  for (l = 0; l < buf_len; l++) {
    PDBG ("%c", out_buf[l]);
  }
  PDBG("\n");
  }
#endif

}

/* MTF */
/* par. : buffer, delka bufferu */
void do_mtf (uchar *buf, long len) {
  uchar stack[256], p, pp; int i; long l;
  
  for (i = 0; i < 256; i++) stack[i] = i;
  
  for (l = 0; l < len; l++) {
    if (stack[0] == *buf) {
      *buf++ = 0;
    } else {
      p = stack[0];
      for (i = 1; i < 256; i++) {
        if (stack[i] == *buf) {
          stack[0] = *buf;
          stack[i] = p;
          *buf++ = i;
          break;
        }
        pp = stack[i];
        stack[i] = p;
        p = pp;
      }
    }
  }
}

/* reverzni MTF */
/* par. : buffer, delka bufferu */
void rev_mtf (uchar *buf, long len) {
  uchar stack[256], p, pp; int i; long l;
  
  for (i = 0; i < 256; i++) stack[i] = i;
  
  for (l = 0; l < len; l++) {
    p = *buf;
    *buf++ = pp = stack[p];
    if (p) {
      memmove (stack + 1, stack, p);
      stack[0] = pp;
    }
  }
}

/* delka bufferu v bytech (dle level) */
#define BUF_LEN(x)	(x * BUF_DELTA)

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, verb_str, ascii mod */
int enc_bwt (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; fsize_t i; int r = 0;
  uchar *in_buf, *out_buf, *p_buf, uc;
  long op, el, l;
  BFILE *bf;
  int j, zrun;

  *crc = CRC_INIT; *orig_len = *enc_len = 0;

  /* asociace s bitovym souborem */
  if (!(bf = bfasoc (fw, BF_WRITE))) return M_ERR_OTHER;

  /* alokace bufferu */
  if (!(in_buf = malloc (BUF_LEN (level)))) return M_ERR_MEM;
  if (!(out_buf = malloc (BUF_LEN (level)+1))) return M_ERR_MEM;

  /* priprava */
  ahc_init_tree (level);

  /* nacteni vstupnich dat */
  while (1) {
    /* nacteni bloku */
    i = 0; p_buf = in_buf;
    while (1) {
      if ((c = fgetc (fr)) == EOF) break;
      (*orig_len)++;
      if (ascii) {
        c2 = MACHINE_TO_UNIX_EOL (c, &s[0], &s[1]);
        for (c = 0; c < c2; c++) {
          *p_buf++ = s[c];
          i++;
        }
      } else {
        *p_buf++ = c;
        i++;
      }
      if (i >= BUF_LEN (level)) break;
    }
    if (!i) break;	/* konec */
    
    op = do_bwt (in_buf, out_buf, i);
    if (op < 0) { r = op; goto lend; }
    do_mtf (out_buf, i+1);
    
    /* zapis dat */
    el = MACHINE_TO_HOST_ENDIAN_UL (i+1);
    if ((l = ahc_fwrite ((void *)(&el), sizeof (i), bf, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
    *enc_len += l;
    
    p_buf = out_buf; zrun = -1;
    for (j = 0; j < i+1; j++) {
      uc = *p_buf++;
      if (!uc) {	/* retezce nul se koduji nulou a delkou */
        if (zrun == 255) {
          if ((l = ahc_putc (bf, zrun, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
          *enc_len += l;
          zrun = -1;
        }
        if (zrun == -1) {
          if ((l = ahc_putc (bf, 0, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
          *enc_len += l;
        }
        zrun++;
      } else {
        if (zrun > -1) {
          if ((l = ahc_putc (bf, zrun, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
          *enc_len += l;
          zrun = -1;
        }
        if ((l = ahc_putc (bf, uc, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
        *enc_len += l;
      }
    }
    if (zrun > -1) {
      if ((l = ahc_putc (bf, zrun, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
      *enc_len += l;
    }
    
    el = MACHINE_TO_HOST_ENDIAN_UL (op);
    if ((l = ahc_fwrite ((void *)(&el), sizeof (op), bf, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
    *enc_len += l;
  }
  
  /* end-of-data na konec */
  if ((l = ahc_putc (bf, SYMBOL_EOD, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
  *enc_len += l;

  if ((l = ahc_flush (bf, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
  *enc_len += l;

  lend:

  free (in_buf);
  free (out_buf);

  (void) bfdetach (bf);

  return r;
}

/* dekompresni fce */
/* par. : z, do, kolik, uroven, CRC, verb_str, ascii mod */
int dec_bwt (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; fsize_t i, buf_len; int r = 0;
  uchar *in_buf, *out_buf, *p_buf;
  long op, l; long *tbuf; int last = 0;
  BFILE *bf; int j, zrun;

  *crc = CRC_INIT;

  /* asociace s bitovym souborem */
  if (!(bf = bfasoc (fr, BF_READ))) return M_ERR_OTHER;

  /* alokace bufferu */
  if (!(in_buf = malloc (BUF_LEN (level) + 1))) return M_ERR_MEM;
  if (!(out_buf = malloc (BUF_LEN (level) + 1))) return M_ERR_MEM;
  if (!(tbuf = malloc (sizeof (long) * (BUF_LEN (level) + 1)))) return M_ERR_MEM;

  ahc_init_tree (level);
  
  /* nacteni vstupnich dat */
  while (!last) {
    /* nacteni bloku */
    for (j = 0; j < sizeof (buf_len); j++) {
      if ((l = ahc_getc (bf, &c, crc)) == -1) { r = M_ERR_DAMAGE; goto lend; }
      enc_len -= l;
      if (c == SYMBOL_EOD) {
        if (j) {
          r = M_ERR_DAMAGE;
          goto lend;
        } else
          last = 1;
          break;
      }
      ((uchar *)(&buf_len))[j] = c;
    }
    if (last) break;
    p_buf = in_buf; i = buf_len; zrun = -1;
    while (1) {
      if (zrun > -1) {
        zrun--;
        c = 0;
      } else {
        if ((l = ahc_getc (bf, &c, crc)) == -1) { r = M_ERR_DAMAGE; goto lend; }
        enc_len -= l;
        if (c == SYMBOL_EOD) { last = 1; break; }
        if (!c) {
          if ((l = ahc_getc (bf, &zrun, crc)) == -1) { r = M_ERR_DAMAGE; goto lend; }
          enc_len -= l;
          zrun--;
        }
      }
      if (ascii) {
        c2 = UNIX_TO_MACHINE_EOL (c, &s[0], &s[1]);
        for (c = 0; c < c2; c++) {
          *p_buf++ = s[c];
          i--;
        }
      } else {
        *p_buf++ = c;
        i--;
      }
      if (!i) break;
    }
    
    for (j = 0; j < sizeof (op); j++) {
      if ((l = ahc_getc (bf, &c, crc)) == -1) { r = M_ERR_DAMAGE; goto lend; }
      enc_len -= l;
      if (c == SYMBOL_EOD) { r = M_ERR_DAMAGE; goto lend; }
      ((uchar *)(&op))[j] = c;
    }
    op = HOST_TO_MACHINE_ENDIAN_UL (op);
    
    rev_mtf (in_buf, buf_len);
    rev_bwt (in_buf, op, out_buf, buf_len, tbuf);
    
    if (!fwrite (out_buf, buf_len-1, 1, fw)) { r = M_ERR_OUTPUT; goto lend; }
  }
  
  if (enc_len) r = M_ERR_DAMAGE;

  lend:

  free (in_buf);
  free (out_buf);
  free (tbuf);

  (void) bfdetach (bf);

  return r;
}

#ifdef __cplusplus
}
#endif
