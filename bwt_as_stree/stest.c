#include <stdio.h>
#include <string.h>

#include "stree.h"

void stree_node_info (stree_t *tree, stree_node_t *node, uchar **suf, long *len);

int main (int argc, char *argv[]) {
  stree_t *t; stree_node_t *n, *nn; uchar *s; long l, d, i;
  
  if (argv[1]) {
    t = stree_init (argv[1], argv[2] ? atoi(argv[2]) : strlen (argv[1]), 0);
    if (!t) { printf ("no mem\n"); exit (1); }
    
    /* vypis stromu */
    printf ("\ntree walk\n\n");
    n = stree_root (t); d = 0;
    while (n) {
      stree_node_info (t, n, &s, &l);
      for (i = 0; i < d; i++) putchar ('-');
      printf (" %s/%ld\n", s, l);
      nn = stree_child (t, n);
      if (nn) d++;
      if (!nn) nn = stree_next (t, n);
      while (!nn) {
        n = nn = stree_parent (t, n);
        if (!nn) break;
        nn = stree_next (t, nn);
        d--;
      }
      n = nn;
    }
    
    /* vypis setridenych suffixu */
    printf ("\nsorted suffixes\n\n");
    n = stree_root (t);
    while (n) {
      s = stree_suffix (t, n);
      if (s) printf ("[%3d] %s\n", (char *)s - argv[1], s);
      nn = stree_child (t, n);
      if (!nn) nn = stree_next (t, n);
      while (!nn) {
        n = nn = stree_parent (t, n);
        if (!nn) break;
        nn = stree_next (t, nn);
      }
      n = nn;
    }
    
    stree_done (t);
  } else {
    printf ("%s text [len]\n", argv[0]);
  }

  return 0;
}
