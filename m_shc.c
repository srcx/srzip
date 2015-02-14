/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_shc.c
 * komprimacni metoda shc (staticke huffmanovo kodovani)
 */

#include "m_shc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* zruseni ladicich vypisu */
#undef DBG
#define DBG(x)

/* prirustek velikosti bufferu */
#define BUF_DELTA	10240

/* pocet hodnot pro uchar */
#define CHAR_COUNT	(1 << BITS_IN_CHAR)

/* max. velikost stromu */
#define TREE_SIZE	((CHAR_COUNT << 1) - 1)

/* typ prvku v huffmanove stromu */
typedef struct _hn {
  long freq;	/* cetnost */
  struct _hn *l_node, *r_node;	/* levy a pravy */
  struct _hn *prev, *next;	/* ipredchozi / dalsi v rade */
  uchar code[2];	/* bitovy kod */
  int bits;	/* pocet bitu v kodu */
} huff_node_t;

/* tabulka cetnosti */
PRIVATE huff_node_t	tree[TREE_SIZE];	/* max. 511 prvku v huffmanove stromu */

/* prvni volny prvek */
PRIVATE int	free_node;

/* inicializuje strom */
/* je-li f != NULL -> nacte cetnosti z nej */
PRIVATE int	init_tree (FILE *f, crc_t *crc) {
  int i;

  PDBG("init_tree : ");
  for (i = 0; i < CHAR_COUNT; i++) {	/* init spodni vrstvy */
    if (f) {	/* nacteni ze souboru */
      /* tyto cetnosti jiz jsou zmensene */
      if ((tree[i].freq = fgetc_crc (f, crc)) == EOF) return -1;
      PDBG("%lu, ", tree[i].freq);
    } else {	/* vynulovani */
      tree[i].freq = 0;
    }
    tree[i].l_node = tree[i].r_node = NULL;
    tree[i].next = (i != CHAR_COUNT - 1) ? tree + i + 1 : NULL;
    tree[i].prev = (i) ? tree + i - 1 : NULL;
    tree[i].bits = 0;
  }
  free_node = CHAR_COUNT;
  PDBG("\n");

  return 0;
}

/* spocitani cetnosti pro buffer */
PRIVATE void	comp_freq (uchar *buf, ulong len) {

  while (len--)
    tree[*buf++].freq += 1;
}

/* prirazeni bitovych kodu - rekurzivni cast */
/* node = pro ktery, up = nadrizeny */
PRIVATE void    comp_codes_r (huff_node_t *node, huff_node_t *up, int bit) {

  *((ushort *)node->code) = *((ushort *)up->code);	/* kopie */
  node->bits = bfaddb (node->code, up->bits, bit);
  if (node->l_node) {
    comp_codes_r (node->l_node, node, 0);
    comp_codes_r (node->r_node, node, 1);
  }
}

/* prirazeni bitovych kodu */
/* node = ktery je nejvys */
PRIVATE INLINE void    comp_codes (huff_node_t *node) {

  node->bits = 0;
  if (node->l_node) {
    comp_codes_r (node->l_node, node, 0);
    comp_codes_r (node->r_node, node, 1);
  } else
    comp_codes_r (node, node, 1);
}

/* vybudovani huffmanova stromu */
/* recomp = 1 -> provede zmenseni frekvenci */
/* codes = 1 -> spocita kody */
/* vraci koren stromu */
PRIVATE huff_node_t	*build_tree (int recomp, int codes) {
  huff_node_t *m1, *m2, *t, *first = tree, *n; int i; long max;

  /* zmenseni cetnosti tak, aby vysledny kod mel max. 16 bitu (freq 0 .. 255) */
  if (recomp) {
    max = 0;
    for (i = 0; i < CHAR_COUNT; i++)
      if (tree[i].freq > max) max = tree[i].freq;
    if (!max) max = 1;
    max /= CHAR_COUNT - 1;
    max += 1;
    for (i = 0; i < CHAR_COUNT; i++)
      if (tree[i].freq && !(tree[i].freq /= max))
        tree[i].freq = 1;
  }

  /* zbudovani stromu */
  while (first->next) {
    n = first; m1 = m2 = NULL;
    while (n) {
      if (!m1 || (n->freq <= m1->freq)) {
        if (!m2 || (m1->freq <= m2->freq))
          m2 = m1;
        m1 = n;
      } else if (!m2 || (n->freq <= m2->freq))
        m2 = n;
      n = n->next;	/* posun */
    }
    n = &tree[free_node++];	/* volny prvek */
    n->l_node = (m1->freq) ? m1 : NULL;
    n->r_node = (m2->freq) ? m2 : NULL;
    /* zruseni degenerovanych uzlu */
    if (!n->l_node) {
      if (first == m1) first = m1->next;
      if (m1->prev) m1->prev->next = m1->next;
      if (m1->next) m1->next->prev = m1->prev;
    } else if (!n->r_node) {
      if (first == m2) first = m2->next;
      if (m2->prev) m2->prev->next = m2->next;
      if (m2->next) m2->next->prev = m2->prev;
    } else {
      n->freq = m1->freq + m2->freq;
      /* rusit je nutno toho na konci retezce */
      if (!m2->prev) {
        t = m2;
        m2 = m1;
        m1 = t;
      }
      /* zruseni druheho */
      if (m2->prev) m2->prev->next = m2->next;
      if (m2->next) m2->next->prev = m2->prev;
      /* vytesneni prvniho */
      if ((n->prev = m1->prev)) m1->prev->next = n;
      if ((n->next = m1->next)) m1->next->prev = n;
      /* zac. vrstvy */
      if (first == m1) first = n;
    }
  }

  /* spocitani kodu */
  if (codes)
    comp_codes (first);

  return first;
}

/* zapis cetnosti do souboru */
PRIVATE int     write_tree (FILE *f, crc_t *crc) {
  int i;

  PDBG("write_tree : ");
  for (i = 0; i < CHAR_COUNT; i++) {
    PDBG("%lu, ", tree[i].freq);
    if (fputc_crc ((int)tree[i].freq, f, crc) == EOF) return -1;
  }
  PDBG("\n");

  return 0;
}

/* vypusteni pripadnych prebytku z bufferu */
/* vraci pocet skutecne zapsanych bytu */
PRIVATE INLINE int	shc_flush (BFILE *f, crc_t *crc) {

  bf_bytes = 0;

  if (bfflush (f, crc) == -1) return -1;

  return bf_bytes;
}

/* zakodovany zapis znaku do souboru */
/* vraci pocet skutecne zapsanych bytu */
PRIVATE INLINE int	shc_putc (BFILE *f, int c, crc_t *crc) {

  PDBG("shc_putc %d -> %d (%d, %d)\n", c, tree[c].bits, tree[c].code[0], tree[c].code[1]);
  if (bfwriteb (f, tree[c].code, tree[c].bits, crc) != tree[c].bits)
    return -1;

  return bf_bytes;
}

/* precteni znaku ze souboru */
/* vraci pocet skutecne prectenych bytu */
PRIVATE INLINE int	shc_getc (BFILE *f, int *c, crc_t *crc, huff_node_t *node) {
  int b;

  bf_bytes = 0;

  PDBG("shc_getc -> ");
  if ((b = bfgetb (f, crc)) == EOF) return -1;
  PDBG("%d", b);
  while (node) {
    if (node->l_node) {	/* uzel */
      if (b)
        node = node->r_node;
      else
        node = node->l_node;
      if (node->l_node) {
        if ((b = bfgetb (f, crc)) == EOF) return -1;
        PDBG("%d", b);
      }
    } else {	/* list */
      /* vypocet pozice */
      *c = node - tree;
      PDBG(" = %d (%c)", *c, *c);
      break;
    }
  }
  PDBG("\n");

  return bf_bytes;
}

/* kompresni buffer */
PRIVATE uchar *	buffer;

/* delka bufferu v bytech (dle level) */
#define BUF_LEN(x)	((10 - (x)) * BUF_DELTA)

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int enc_shc (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; int l; BFILE *bf; fsize_t buf_len, i; int r = 0;

  *crc = CRC_INIT; *orig_len = *enc_len = 0;

  /* alokace bufferu */
  if (!(buffer = malloc (BUF_LEN (level)))) return M_ERR_MEM;

  /* asociace s bitovym souborem */
  if (!(bf = bfasoc (fw, BF_WRITE))) { r = M_ERR_OTHER; goto lend; }

  /* a jedem */
  while ((buf_len = fread (buffer, 1, BUF_LEN (level), fr))) {
    *orig_len += buf_len;
    /* konstrukce stromu */
    (void) init_tree (NULL, crc);
    comp_freq (buffer, buf_len);
    (void) build_tree (1, 1);	/* zmenseni frekvenci a spocitani kodu */
    /* vystup stromu */
    if (write_tree (fw, crc) == -1) { r = M_ERR_OUTPUT; goto lend; }
    *enc_len += CHAR_COUNT;
    /* vystup bufferu */
    for (i = 0; i < buf_len; i++) {
      c = buffer[i];
      if (ascii) {
        c2 = MACHINE_TO_UNIX_EOL (c, &s[0], &s[1]);
        for (c = 0; c < c2; c++) {
          if ((l = shc_putc (bf, s[c], crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
          *enc_len += l;
        }
      } else {
        if ((l = shc_putc (bf, c, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
        *enc_len += l;
      }
    }
    if ((l = shc_flush (bf, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
    *enc_len += l;
  }

  lend:

  (void) bfdetach (bf);
  free (buffer);

  return r;
}

/* dekompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int dec_shc (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; int l; fsize_t block_len; BFILE *bf;
  huff_node_t *root; int r = 0;

  *crc = CRC_INIT; block_len = BUF_LEN (level); enc_len = enc_len;
  l = 0;

  /* asociace s bitovym souborem */
  if (!(bf = bfasoc (fr, BF_READ))) return M_ERR_OTHER;

  if (init_tree (fr, crc) == -1) { r = M_ERR_DAMAGE; goto lend; }
  root = build_tree (0, 0);	/* neni treba prepoctu nebo kodu */

  while (orig_len) {
    l = shc_getc (bf, &c, crc, root);
    block_len--; orig_len--;

    if (ascii) {
      c2 = UNIX_TO_MACHINE_EOL (c, &s[0], &s[1]);
      if (c2 && !fwrite (s, c2, 1, fw)) { r = M_ERR_OUTPUT; goto lend; }
    } else {
      if (fputc (c, fw) == EOF) { r = M_ERR_OUTPUT; goto lend; }	/* nejde zapsat */
    }

    if (!block_len && orig_len) {	/* konec bloku */
      block_len = BUF_LEN (level);
      bfclear (bf, BF_READ);	/* vyprazdneni bufferu */
      /* nacteni stromu */
      if (init_tree (fr, crc) == -1) { r = M_ERR_DAMAGE; goto lend; }
      root = build_tree (0, 0);	/* neni treba prepoctu nebo kodu */
    }
  }

  if (l < 0)
    r = M_ERR_DAMAGE;	/* EOF by nemel nastat */

  lend:

  (void) bfdetach (bf);

  return r;
}

/* obnoveni puvodniho stavu */
#undef DBG
#define DBG(x)	ORIG_DBG(x)

#ifdef __cplusplus
}
#endif
