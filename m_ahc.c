/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* m_ahc.c
 * komprimacni metoda ahc (adaptivni huffmanovo kodovani)
 */

#include "m_ahc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* escape symbol */
#define SYMBOL_ESC	CHAR_COUNT

/* pocet hodnot pro strom */
#define SYMBOL_COUNT	(CHAR_COUNT + 2)

/* max. velikost stromu */
#define TREE_SIZE	((SYMBOL_COUNT << 1) - 1)

/* limit pro snizeni frekvenci - vypocet na zaklade level */
#define COMPUTE_LIMIT(x)	(1 << (13 + (10 - x) * 2))
PRIVATE long	limit = COMPUTE_LIMIT (1);

/* typ prvku v huffmanove stromu */
typedef struct _hn {
  long weight;	/* cetnost */
  struct _hn *parent;	/* rodic */
  struct _hn *f_son;	/* 1. potomek */
  int	val;	/* hodnota, je-li v listu */
} huff_node_t;

/* typ huffmanova stromu */
typedef struct {
  huff_node_t *	leaf[SYMBOL_COUNT];	/* listy stromu */
  huff_node_t *	free_node;	/* prvni volny prvek */
  huff_node_t	nodes[TREE_SIZE];	/* uzly stromu */
} huff_tree_t;

/* tabulka cetnosti */
PRIVATE huff_tree_t	tree;

/* inicializace stromu */
void ahc_init_tree (long level) {
  int i;

  limit = COMPUTE_LIMIT (level);
  /* koren */
  tree.nodes[0].weight = 2;
  tree.nodes[0].parent = NULL;
  tree.nodes[0].f_son = tree.nodes + 1;
  tree.nodes[0].val = -1;
  /* escape */
  tree.nodes[1].weight = 1;
  tree.nodes[1].parent = tree.nodes;
  tree.nodes[1].f_son = NULL;
  tree.nodes[1].val = SYMBOL_ESC;
  tree.leaf[SYMBOL_ESC] = tree.nodes + 1;
  /* end-of-data */
  tree.nodes[2].weight = 1;
  tree.nodes[2].parent = tree.nodes;
  tree.nodes[2].f_son = NULL;
  tree.nodes[2].val = SYMBOL_EOD;
  tree.leaf[SYMBOL_EOD] = tree.nodes + 2;
  /* prvni volny */
  tree.free_node = tree.nodes + 3;
  /* ostatni listy */
  for (i = 0; i < SYMBOL_ESC; i++)
    tree.leaf[i] = NULL;
}

/* prebudovani stromu pri prilisne vaze korene */
PRIVATE void	rebuild_tree (void) {
  huff_node_t *node, *i, *n; long w;

  /* kopie listu na konec stromu spolu se zmensenim vah */
  node = tree.free_node - 1;
  for (i = node; i >= tree.nodes; i--)
    if (!i->f_son) {
      *node = *i;
      node->weight = (node->weight + 1) / 2;
      node--;
    }

  /* zbudovani vyssich uzlu */
  /* node = novy uzel */
  for (i = tree.free_node - 2; node >= tree.nodes; i -= 2, node--) {
    n = i + 1;
    w = i->weight + n->weight;
    for (n = node + 1; w < n->weight; n++);
    n--;
    /* posun */
    memmove (node, node + 1, (n - node) * sizeof (huff_node_t));
    n->weight = w;
    n->f_son = i;
  }

  /* urceni mist jednotlivych symbolu a urceni rodicu */
  for (i = tree.free_node - 1; i >= tree.nodes; i--)
    if ((n = i->f_son))
      n->parent = (n+1)->parent = i;
    else
      tree.leaf[i->val] = i;
}

/* vymena uzlu */
PRIVATE void	swap_nodes (huff_node_t *n1, huff_node_t *n2) {
  huff_node_t temp;

  if (n1->f_son)	/* je to uzel */
    n1->f_son->parent = (n1->f_son+1)->parent = n2;
  else	/* je to list */
    tree.leaf[n1->val] = n2;

  if (n2->f_son)	/* je to uzel */
    n2->f_son->parent = (n2->f_son+1)->parent = n1;
  else	/* je to list */
    tree.leaf[n2->val] = n1;

  /* vymena */
  temp = *n1;
  *n1 = *n2;
  /* zachovani puvodnich rodicu */
  n1->parent = temp.parent;
  temp.parent = n2->parent;
  *n2 = temp;
}

/* zvyseni cetnosti pro symbol */
PRIVATE void	update_tree (int c) {
  huff_node_t *cur, *node;

  /* predelani pri limitu */
  if (tree.nodes[0].weight == limit)
    rebuild_tree ();

  cur = tree.leaf[c];
  while (cur) {
    cur->weight++;
    /* hledani uzlu pro vymenu */
    for (node = cur; node > tree.nodes; node--)
      if ((node - 1)->weight >= cur->weight) break;
    /* vymena uzlu */
    if (cur != node) {
      swap_nodes (cur, node);
      cur = node;
    }
    /* o stupinek vyse */
    cur = cur->parent;
  }
}

/* pridani noveho symbolu s nulovou vahou */
PRIVATE void	add_node (int c) {
  huff_node_t *lnode, *old_node, *new_node;

  /* nalezeni nejlehciho prvku (nejvice vpravo) */
  lnode = tree.free_node - 1;
  /* dalsi dva prvky (volne) */
  old_node = tree.free_node;
  new_node = tree.free_node + 1;
  /* posun za */
  tree.free_node += 2;
  /* kopie z lnode do old_node */
  *old_node = *lnode;
  old_node->parent = lnode;
  tree.leaf[old_node->val] = old_node;
  /* novy nulovy prvek */
  new_node->parent = lnode;
  new_node->f_son = NULL;
  new_node->weight = 0;
  new_node->val = c;
  tree.leaf[c] = new_node;
  /* pretvoreni lnode na uzel */
  lnode->f_son = old_node;
}

/* zakodovany zapis znaku do souboru */
/* vraci pocet skutecne zapsanych bytu */
INLINE int	ahc_putc (BFILE *f, int c, crc_t *crc) {
  huff_node_t *cur; int l; uchar uc = c;
  static uchar buf[SYMBOL_COUNT];

  bf_bytes = 0;

  cur = tree.leaf[c];
  if (!cur)	/* neni ve stromu */
    cur = tree.leaf[SYMBOL_ESC];
  /* zapis kodu */
  l = 0;
  while (cur->parent) {
    buf[l++] = (cur->parent->f_son != cur);	/* je-li levym synem -> 0 */
    cur = cur->parent;
  }
  while (l)
    if (bfputb (f, buf[--l], crc) == -1) return -1;
  l = bf_bytes;
  /* vystup znaku po ESC */
  if (!tree.leaf[c]) {
    if (bfwriteb (f, &uc, BITS_IN_CHAR, crc) != BITS_IN_CHAR)
      return -1;
    l += bf_bytes;
    /* pridani uzlu */
    add_node (c);
  }
  /* zvetseni vahy */
  update_tree (c);

  return l;
}

/* zakodovany zapis bufferu do souboru */
/* vraci pocet skutecne zapsanych bytu */
INLINE long ahc_fwrite (void *buf, long len, BFILE *f, crc_t *crc) {
  long l, r;

  r = 0;
  while (len--) {
    if ((l = ahc_putc (f, *((uchar *)buf)++, crc)) == -1) return -1;
    r += l;
  }
  
  return r;
}

/* precteni znaku ze souboru */
/* vraci pocet skutecne prectenych bytu */
INLINE int	ahc_getc (BFILE *f, int *c, crc_t *crc) {
  huff_node_t *cur; int b, l; uchar uc[2];

  bf_bytes = 0;
  cur = tree.nodes;
  while (cur) {
    if (cur->f_son) {	/* uzel */
      if ((b = bfgetb (f, crc)) == EOF) return -1;
      if (b)	/* sestup */
        cur = cur->f_son + 1;
      else
        cur = cur->f_son;
    } else {	/* list */
      *c = cur->val;
      break;
    }
  }
  l = bf_bytes;
  /* nacteni ESC */
  if (*c == SYMBOL_ESC) {
    if (bfreadb (f, uc, BITS_IN_CHAR, crc) != BITS_IN_CHAR) return -1;
    *c = uc[0];
    l += bf_bytes;
    add_node (*c);
  }
  /* zvetseni vahy */
  update_tree (*c);

  return l;
}

/* vypusteni pripadnych prebytku z bufferu */
/* vraci pocet skutecne zapsanych bytu */
INLINE int	ahc_flush (BFILE *f, crc_t *crc) {

  bf_bytes = 0;

  if (bfflush (f, crc) == -1) return -1;

  return bf_bytes;
}

/* kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int enc_ahc (FILE *fr, FILE *fw, fsize_t *orig_len, fsize_t *enc_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; int l; BFILE *bf; int r = 0;

  *crc = CRC_INIT; *orig_len = *enc_len = 0;

  /* asociace s bitovym souborem */
  if (!(bf = bfasoc (fw, BF_WRITE))) return M_ERR_OTHER;

  /* priprava */
  ahc_init_tree (level);

  /* a jedem */
  while ((c = fgetc (fr)) != -1) {
    (*orig_len)++;
    if (ascii) {
      c2 = MACHINE_TO_UNIX_EOL (c, &s[0], &s[1]);
      for (c = 0; c < c2; c++) {
        if ((l = ahc_putc (bf, s[c], crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
        *enc_len += l;
      }
    } else {
      if ((l = ahc_putc (bf, c, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
      *enc_len += l;
    }
  }
  /* end-of-data na konec */
  if ((l = ahc_putc (bf, SYMBOL_EOD, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
  *enc_len += l;

  if ((l = ahc_flush (bf, crc)) == -1) { r = M_ERR_OUTPUT; goto lend; }
  *enc_len += l;

  lend:

  (void) bfdetach (bf);

  return r;
}

/* dekompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
int dec_ahc (FILE *fr, FILE *fw, fsize_t enc_len, fsize_t orig_len, int level, crc_t *crc, int ascii) {
  int c, c2; char s[2]; int l; BFILE *bf; int r = 0;

  *crc = CRC_INIT; orig_len = orig_len; enc_len = enc_len;
  l = 0;

  /* asociace s bitovym souborem */
  if (!(bf = bfasoc (fr, BF_READ))) return M_ERR_OTHER;

  ahc_init_tree (level);

  while (((l = ahc_getc (bf, &c, crc)) != -1) && (c != SYMBOL_EOD)) {
    if (ascii) {
      c2 = UNIX_TO_MACHINE_EOL (c, &s[0], &s[1]);
      if (c2 && !fwrite (s, c2, 1, fw)) { r = M_ERR_OUTPUT; goto lend; }
    } else {
      if (fputc (c, fw) == EOF) { r = M_ERR_OUTPUT; goto lend; }	/* nejde zapsat */
    }
  }

  if (l < 0)
    r = M_ERR_DAMAGE;	/* EOF by nemel nastat */

  lend:

  (void) bfdetach (bf);

  return r;
}

#ifdef __cplusplus
}
#endif