/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* stree.c
 * funkce pro praci se suffixovym stromem
 */

#include "stree.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef DBG
#define DBG(x)	((void)(0))

/* uzel stromu */
struct stree_node_t {
  uchar *suffix;	/* suffix prislusejici danemu uzlu */
  long len;		/* delka suffixu */
  stree_node_t *child;	/* nejlevejsi potomek */
  stree_node_t *next;	/* naslednik */
  stree_node_t *parent;	/* rodic */
};

/* suffixovy strom */
struct stree_t {
  stree_node_t *root;	/* koren - vytvari se uz pri inicializaci stromu
  				 - ve skutecnosti prvni prvek pod korenem */
};

PRIVATE int stree_add (stree_t *tree, uchar *suf, long len);

/* inicializace stromu */
stree_t *stree_init (uchar *str, long len, int has_eof) {
  stree_t *tree; long i;
  
  if (!(tree = malloc (sizeof (stree_t)))) return NULL;
  if (!(tree->root = malloc (sizeof (stree_node_t)))) return NULL;
  tree->root->suffix = str + len - (has_eof ? 0 : 1);
  tree->root->len = (has_eof ? 0 : 1);
  tree->root->child = tree->root->next = tree->root->parent = NULL;

  for (i = (has_eof ? 1 : 2); i <= len; i++) {
    if (stree_add (tree, str + len - i, i)) return NULL;
  }
  
  return tree;
}

PRIVATE int stree_add (stree_t *tree, uchar *suf, long len) {
  stree_node_t *n = tree->root, *nn, *a, *b; long i, j, k;
  
  PDBG("stree_add (suf='%s', len=%ld)\n", suf, len);
  
  if (!(a = malloc (sizeof (stree_node_t)))) return -1;
  a->suffix = suf;
  a->len = len;
  a->child = a->next = a->parent = NULL;

  i = 0; nn = NULL;
  while (n) {
    if (i < n->len) {
    PDBG("- checking '%s'[%ld]='%c' and new '%s'[%ld]='%c'\n", n->suffix, i, n->suffix[i], suf, i, suf[i]);
    if (n->suffix[i] < suf[i]) {
      PDBG("- - move next\n");    
      nn = n;
      n = n->next;
      if (n) continue;
    } else if (n->suffix[i] == suf[i]) {
      PDBG("- - descent\n");    
      if (!n->child) {
        PDBG("- - - split\n");      
        j = 0; k = i;
        do { i++; j++; k++; } while ((k < n->len) && (n->suffix[i] == suf[i]));
        if (!(b = malloc (sizeof (stree_node_t)))) return -1;
        b->suffix = n->suffix;
        b->len = n->len;
        b->child = b->next = NULL;
        b->parent = a->parent = n;
        if ((n->suffix[i] > suf[i]) || (k == n->len)) {
          n->child = a;
          a->next = b;
        } else {
          n->child = b;
          b->next = a;
        }
        n->len = j;
        break;
      } else {
        PDBG("- - - descent\n");
        j = i;
        do { i++; j++; } while ((j < n->len) && (n->suffix[i] == suf[i]));
        nn = NULL;
        n = n->child;
      }
      continue;
    }
    }
    PDBG("- - add\n");
    a->next = n;
    if (nn) {
      nn->next = a;
      a->parent = nn->parent;
    } else {
      if (n->parent)
        n->parent->child = a;
      else
        tree->root = a;
      a->parent = n->parent;
    }
    break;
  }
  
  return 0;
}

/* zruseni stromu */
void stree_done (stree_t *tree) {
  stree_node_t *n, *nn;
  n = tree->root;
  while (n) {
    if (n->child) {
      nn = n->child;
      n->child = NULL;
      n = nn;
    } else if (n->next) {
      nn = n;
      n = n->next;
      free (nn);
    } else {
      nn = n;
      n = n->parent;
      free (nn);
    }
  }
  free (tree);
}

/* vraci koren */
stree_node_t *stree_root (stree_t *tree) {
  return tree->root;
}

/* vraci naslednika daneho uzlu (v abecednim usporadani) */
stree_node_t *stree_next (stree_t *tree, stree_node_t *node) {
  tree = tree;
  return node->next;
}

/* vraci potomka daneho uzlu */
stree_node_t *stree_child (stree_t *tree, stree_node_t *node) {
  tree = tree;
  return node->child;
}

/* vraci predka daneho uzlu */
stree_node_t *stree_parent (stree_t *tree, stree_node_t *node) {
  tree = tree;
  return node->parent;
}

/* vraci suffix (ukazatel >= str u stree_init()) prislusejici danemu uzlu */
uchar *stree_suffix (stree_t *tree, stree_node_t *node) {
  tree = tree;
  return node->child ? NULL : node->suffix;
}

/* vraci suffix a len pro dany uzel - pro ladici ucely */
void stree_node_info (stree_t *tree, stree_node_t *node, uchar **suf, long *len) {
  tree = tree;
  *suf = node->suffix;
  *len = node->len;
}

#ifdef __cplusplus
}
#endif
