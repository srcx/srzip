/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* parseopt.h
 * zpracovani prikazove radky
 */

#ifndef __PARSEOPT_H
#define __PARSEOPT_H

#include <stdio.h>
#include <string.h>

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

/* specifikace parametru voleb */
#define OPTS_YES		'+'	/* vzdy */
#define OPTS_NO			'-'	/* nikdy */
#define OPTS_MAYBE		':'	/* muze byt */

/* zpracovani prikazove radky
   moznosti :
   jednoznakove prepinace zacinajici na '-'
   - mozno i shlukovat a davat parametry primo za bez mezery
   - pr. (pro "a+b-c ") :
     -abc	a='bc'
     -a -bc	a='bc'
     -bac	b='' a='c'
     -b -a c	b='' a='c'
     -c -a c	c='' a='c'
     -c a c	c='a' volny par. 'c'
     ...
   jmenne prepinace zacinajici na '+' nebo '--'
   - parametry jsou ve forme 'jmeno=par' nebo 'jmeno par'
   - pr. (pro {"+par1","-par2"," par3"})
     --par1=ahoj +par2		par1='ahoj' par2='1'
     --par3 ahoj --nopar2	par2='0' par3='ahoj'
     ...
   - pozn. : forma noxxxx se uvazuje az pri nenalezeni puvodniho textu
   ukonceni prepinacu pomoci '--' nebo narazeni na prepinac bez uvodniho znaku '-' ci '+' (= volny par.)
 */

/* priprava na parsing */
/* argc = pocet parametru */
/* argv = parametry */
/* sopts = seznam kratkych voleb "x(+|:|-)...", kde :
	   x je znak,
	   + znaci povinny par.,
	   : volitelny par.,
           - zadny par.
 */
/* lopts = seznam dlouhych voleb {"(+|:|-)xxxxxx", ... } - viz nahore */
void	init_parse_opts (int argc, char *argv[], char *sopts, char *lopts[]);

/* parsing */
/* val = hodnota prepinace (nutno alokovat predem) */
/* vraci :
	>127	- poradi v lopts - 127
        >0      - ASCII hodnota jednoznakove volby ze sopts
        =0	- neni prepinac
        <0      - chyba u prepinace (viz >0 a >127)
 */
int	parse_opts (char *val);

#ifdef __cplusplus
}
#endif

#endif