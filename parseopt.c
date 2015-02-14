/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* parseopt.c
 * zpracovani prikazove radky
 */

#include "parseopt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* zruseni ladicich vypisu */
#undef DBG
#define DBG(x)

/* prefixy voleb */
#define SOPT_PREFIX		"-"	/* prefix kratkych voleb */
#define LOPT_PREFIX_GNU		"+"	/* prefix dlouhych voleb dle GNU */
#define LOPT_PREFIX_POSIX	"--"	/* prefix dlouhych voleb dle POSIX */
#define NO_PREFIX		"no"	/* prefix zaporne volby */
#define NOM_PREFIX		"no-"	/* prefix zaporne volby */
#define LEN_SOPT_PREFIX		1	/* delky prefixu */
#define LEN_LOPT_PREFIX_GNU	1
#define LEN_LOPT_PREFIX_POSIX	2
#define LEN_NO_PREFIX		2
#define LEN_NOM_PREFIX		3

/* konstanty typu */
#define PO_SOPT			3
#define PO_LOPT                 2
#define PO_VOID			1
#define PO_NULL			0
#define PO_TERM			-1

/* spec. */
#define OPT_END			"--"	/* ukoncovac */
#define NO_LOPT			"0"	/* hodnota pri --nojmeno */
#define IS_LOPT			"1"	/* hodnota pri --jmeno */

/* zpracovani prikazove radky
   moznosti :
   jednoznakove prepinace zacinajici na '-'
   - mozno i shlukovat a davat parametry primo za bez mezery
   - pr. (pro "a+b-c:") :
     -abc	a='bc'
     -a -bc	a='bc'
     -bac	b='' a='c'
     -b -a c	b='' a='c'
     -c -a c	c='' a='c'
     -c a c	c='a' volny par. 'c'
     ...
   jmenne prepinace zacinajici na '+' nebo '--'
   - parametry jsou ve forme 'jmeno=par' nebo 'jmeno par'
   - pr. (pro {"+par1","-par2",":par3"})
     --par1=ahoj +par2		par1='ahoj' par2='1'
     --par3 ahoj --nopar2	par2='0' par3='ahoj'
     ...
   - pozn. : forma noxxxx se uvazuje az pri nenalezeni puvodniho textu
             existuje i forma no-xxxx, ktera se hleda az jako posledni
   ukonceni prepinacu pomoci '--' nebo narazeni na prepinac bez uvodniho znaku '-' ci '+' (= volny par.)
 */

PRIVATE int	_argc;		/* pocet polozek v _argv */
PRIVATE char **	_argv;		/* prikazova radka */
PRIVATE char *	_sopts;		/* kratke volby */
PRIVATE char **	_lopts;		/* dlouhe volby */
PRIVATE int	_arg_i;		/* index do _argv */
PRIVATE char *	_arg;		/* volba */
PRIVATE char *	_val;		/* mozny parametr */
PRIVATE int	_arg_t;		/* typ aktualniho _arg (PO_*) */
PRIVATE int	_val_t;		/* typ aktualniho _val (PO_*) */
PRIVATE int	_shift;		/* priznak posunu */
PRIVATE int	_term;		/* priznak probehnuvsiho ukoncovace */

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
void	init_parse_opts (int argc, char *argv[], char *sopts, char *lopts[]) {

  PDBG("PO::init_parse_opts (%d, %p, '%s', %p)\n", argc, argv, sopts, lopts);

  /* naplneni mistnich promennych */
  _argc = argc; _argv = argv; _sopts = sopts; _lopts = lopts;

  _arg_i = _term = _shift = 0;
  _arg_t = PO_NULL;
  _val_t = PO_NULL;
}

/* vraci typ stringu + jeho posun o uvodni znaky (neni-li NULL) */
PRIVATE int	opt_type (char *s, char **n) {
  int r, l; char *x;

  PDBG("PO::opt_type ('%s', %p) = ", s, n);

  x = s; l = strlen (s);
  if (!s) {	/* zadny */
    r = PO_NULL;
  } else if (!strcmp (s, OPT_END)) {	/* konec voleb */
    r = PO_TERM;
  } else if ((l > LEN_LOPT_PREFIX_POSIX) && (!strncmp (s, LOPT_PREFIX_POSIX, LEN_LOPT_PREFIX_POSIX))) {
    r = PO_LOPT;
    x += LEN_LOPT_PREFIX_POSIX;
  } else if ((l > LEN_LOPT_PREFIX_GNU) && (!strncmp (s, LOPT_PREFIX_GNU, LEN_LOPT_PREFIX_GNU))) {
    r = PO_LOPT;
    x += LEN_LOPT_PREFIX_GNU;
  } else if ((l > LEN_SOPT_PREFIX) && (!strncmp (s, SOPT_PREFIX, LEN_SOPT_PREFIX))) {
    r = PO_SOPT;
    x += LEN_SOPT_PREFIX;
  } else {	/* zadna volba */
    r = PO_VOID;
  }

  if (n) *n = x;

  PDBG("('%s') %d\n", n ? *n : NULL, r);

  return r;
}

/* posune se o argument dale (NE o volbu!) */
/* par. udava, zda-li ma prekrocit hodnotu nebo ne */
/* take provede rozdeleni na jmeno a hodnotu vcetne urceni typu */
PRIVATE void	next_arg (int val) {

  PDBG("PO::next_arg\n");

  _arg_i += _shift ? 1 : val + 1;	/* posun */
  if (_arg_i  >= _argc) {	/* konec argumentu */
    _arg = NULL;
    _arg_t = PO_NULL;
    _shift = 0;
  } else if (_term) {	/* za ukoncovacem */
    _arg_t = PO_VOID;
  } else {
    _arg_t = opt_type (_argv[_arg_i], &_arg);	/* typ argumentu */
    if (_arg_t == PO_TERM) {	/* ukoncovac */
      _term = 1;
      next_arg (0);
    }
    if ((_arg_t == PO_LOPT) && (_val = strchr (_arg, '='))) {	/* typ jmeno=hodnota */
      _val[0] = 0;	/* rozdeleni na 2 casti */
      _val++;
      _val_t = PO_VOID;
      _shift = 1;	/* posun bude vzdy o 1 dale */
    } else if ((_arg_t == PO_SOPT) && (strlen (_arg) > 1)) {	/* kratke s nejakym zbytkem */
      _val = _arg + 1;	/* posun za volbu */
      _val_t = PO_VOID;
      _shift = 1;	/* posun bude vzdy o 1 dale */
    } else if (_arg_i+1 >= _argc) {	/* dalsi uz nejsou */
      _val_t = PO_NULL;
      _shift = 0;
    } else {
      _val_t = opt_type (_argv[_arg_i+1], NULL);	/* typ hodnoty */
      _val = _argv[_arg_i+1];
      _shift = 0;
    }
  }
}

/* posune se o jednu volbu dale */
/* par. udava, zda-li ma prekrocit hodnotu nebo ne */
PRIVATE void	next_opt (int val) {

  PDBG("PO::next_opt\n");

  /* posun o cely argument */
  if (((_term) || (val)) || ((_arg_t != PO_SOPT) || !((_arg_t == PO_SOPT) && (strlen (_arg) > 1)))) {
    next_arg (_term ? 0 : val);
  } else {	/* posun o 1 znak pri kratkych */
    _arg++;
    _val++;
  }
}

/* pomocnik k search_lopts */
/* prohledava lopts na vyskyt s */
PRIVATE int	sl2 (char *lopts[], char *s) {
  int i;

  PDBG("sl2 (%p, '%s') = ", lopts, s);

  i = 0;
  while (lopts[i] && strcmp (lopts[i]+1, s))	/* lopts jsou ukonceny pomoci NULL */
    i++;
  if (!lopts[i]) i = -1;	/* nenasel */

  PDBG("%d\n", i);

  return i;
}

/* prohleda lopts na vyskyt s (ignoruje 1. formatovaci znak) */
/* hleda i formu zacinajici na 'no' */
/* vraci pozici v lopts nebo -1 */
/* v n vraci 1, je-li s v 'no' forme
   pr. : 'nopar' je ekvivalentni s 'par', ale n = 1 */
PRIVATE int	search_lopts (char *lopts[], char *s, int *n) {
  int r;

  PDBG("search_lopts (%p, '%s', %p) = ", lopts, s, n);

  *n = 0;
  /* hledani normalni formy */
  if ((r = sl2 (lopts, s)) < 0) {	/* nenalezeno */
    /* hledani 'no' a 'no-' formy */
    if (!strncmp (s, NO_PREFIX, LEN_NO_PREFIX)) {	/* je to 'no' forma */
      s += LEN_NO_PREFIX;
      r = sl2 (lopts, s);
      *n = 1;
    } else if (!strncmp (s, NOM_PREFIX, LEN_NOM_PREFIX)) {	/* je to 'no-' forma */
      s += LEN_NOM_PREFIX;
      r = sl2 (lopts, s);
      *n = 1;
    } 
  }

  PDBG("(n = %d) %d\n", *n, r);

  return r;
}

/* parsing */
/* val = hodnota prepinace (nutno alokovat predem) */
/* vraci :
	>256	- poradi v lopts + 256
        >0      - ASCII hodnota jednoznakove volby ze sopts
        =0	- neni prepinac (val[0] = 0 -> konec zpracovani)
        <0      - chyba u prepinace (jmeno je ve val)
 */
int	parse_opts (char *val) {
  int r = 0, n; char *s2;
  static int v = 0;	/* byla posledne vzata hodnota ? */

  PDBG("PO::parse_opts (%p) = ", val);

  next_opt (v);
  v = 0;	/* zatim bez par. */
  switch (_arg_t) {
    case PO_SOPT :	/* kratka volba */
      PDBG("(short option) ");
      r = _arg[0];	/* jmeno volby */
      if (!(s2 = strchr (_sopts, r))) {	/* nenalezen */
        PDBG("('%c' not found in sopts) ", r);
        val[0] = r; val[1] = 0;
        r = -1;
      } else {
        switch (s2[1]) {
          case OPTS_YES :
            if (_val_t == PO_NULL) {
              PDBG("(parsing error) ");
              val[0] = r;
              val[1] = 0;
              r = -1;
            } else {
              strcpy (val, _val);
              v = 1;
            }
            break;
          case OPTS_MAYBE :
            if (_val_t == PO_VOID) {
              strcpy (val, _val);
              v = 1;
              break;
            }
          case OPTS_NO :
            val[0] = 0;
            break;
        }
      }
      break;
    case PO_LOPT :	/* dlouha volba */
      PDBG("(long option) ");
      /* hledani v lopts */
      if ((r = search_lopts (_lopts, _arg, &n)) < 0) {	/* neni tam */
        PDBG("('%s' not found in lopts) ", _arg);
        strcpy (val, _arg);
      } else {	/* mame ho */
        switch (_lopts[r][0]) {	/* dle typu */
          case OPTS_YES :
            if (_val_t == PO_NULL) {	/* neni hodnota */
              PDBG("(parsing error) ");
              strcpy (val, _lopts[r]+1);
              r = -1;
            } else {	/* je hodnota */
              strcpy (val, _val);
              v = 1;
            }
            break;
          case OPTS_MAYBE :
            if (_val_t == PO_VOID) {	/* je hodnota ? */
              strcpy (val, _val);
              v = 1;
              break;
            }
          case OPTS_NO :
            strcpy (val, n ? NO_LOPT : IS_LOPT);	/* v jake forme ? */
            break;
        }
        if (r > -1) r += 256;	/* r zacina na 1 */
      }
      break;
    case PO_VOID :	/* volny parametr */
      PDBG("(void option) ");
      strcpy (val, _arg);	/* s muze byt '\0' -> konec argumentu */
      r = 0;
      break;
    case PO_NULL :	/* konec zpracovani */
      r = 0;
      val[0] = 0;
      break;
  }

  PDBG("(val = '%s') %d\n", val, r);

  return r;
}

/* obnoveni puvodniho stavu */
#undef DBG
#define DBG(x)	ORIG_DBG(x)

#ifdef __cplusplus
}
#endif

