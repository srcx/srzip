/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* stree.h
 * funkce pro praci se suffixovym stromem
 */

/* pozn. k implementaci :
   abecedou je vzdy prostor bytu (0-255)
   nelze vlozit vice retezcu, pouze jeden
 */

#ifndef __STREE_H
#define __STREE_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

/* uzel stromu */
typedef struct stree_node_t stree_node_t;

/* suffixovy strom */
typedef struct stree_t stree_t;

/* inicializace stromu
   - do stromu se rovnou pridaji vsechny suffixy
   - pokud je has_eof nastaven na 1, tak se pocita i se spec. eof znakem
     za koncem str (jako len+1 znak) */
stree_t *stree_init (uchar *str, long len, int has_eof);

/* zruseni stromu */
void stree_done (stree_t *tree);

/* vraci koren */
stree_node_t *stree_root (stree_t *tree); 

/* vraci naslednika daneho uzlu (v abecednim usporadani) */
stree_node_t *stree_next (stree_t *tree, stree_node_t *node);

/* vraci potomka daneho uzlu */
stree_node_t *stree_child (stree_t *tree, stree_node_t *node);

/* vraci predka daneho uzlu */
stree_node_t *stree_parent (stree_t *tree, stree_node_t *node);

/* vraci suffix (ukazatel >= str u stree_init()) prislusejici danemu uzlu
   NULL je uzel bez suffixu */
uchar *stree_suffix (stree_t *tree, stree_node_t *node);

#ifdef __cplusplus
}
#endif

#endif