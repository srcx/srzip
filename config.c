/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* config.c
 * konfiguracni fce
 */

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SRZIP_DEBUG
int debug_flag = 1;
#endif

char prog_name[32];

int be_quiet = 0;

#ifdef WANT_BASENAME
/* vraci jmeno souboru */
char *basename (char *fname) {
  char *s;

#ifdef HAVE_SLASH_DELIM
  if (!(s = strrchr (fname, '/')))
#endif
#ifdef HAVE_BACKSLASH_DELIM
    if (!(s = strrchr (fname, '\\')))
#endif
#ifdef HAVE_COLON_DELIM
      if (!(s = strrchr (fname, ':')))
#endif
        return fname;

  return s + 1;
}
#endif

#ifdef WANT_TRUNC_FILENAME
/* zkracovani jmen souboru - meni svuj parametr */
int TRUNC_FILENAME (char *name) {
  char fshort[13]; char *fn, *s, *s2;

  if (_USE_LFN) return 0;	/* nezkracuje se */

  memset (fshort, 0, 13);
  fn = basename (name);	/* vzeti jmena */
  /* rozdeleni jmena na cast pred teckou a po tecce (posledni) */
  if (!(s = strrchr (fn, '.'))) {	/* neni tecka */
    strncpy (fshort, fn, 8);
  } else {	/* je tecka */
    /* premena tecek na '_' */
    s2 = fn;
    while ((s2 = strchr (s2, '.'))) *s2++ = '_';

    *s = 0;	/* rozpuleni */
    strncpy (fshort, fn, 8);	/* jmeno */
    s2 = strchr (fshort, 0);
    *s2 = '.';	/* pridani tecky */
    strncpy (s2+1, s+1, 3);	/* pripona */
  }
  strcpy (fn, fshort);

  return 0;
}
#endif

/* separace jmena bez pripony spustitelneho souboru - meni argument */
char *	STRIP_EXE_EXT (char *name) {
  int l = strlen (name), l2 = strlen (EXE_EXT); char *s;

  if (l2 && (l >= l2)) {
    s = name + l - l2;	/* posun na predpokladane misto */
    if (!stricmp (s, EXE_EXT)) *s = 0;	/* preruseni */
  }

  return name;
}

/* nastaveni casu souboru - f je jmeno, t je typu time_t == 32 bitu (uint) */
int SET_FTIME (char *f, time_t t) {
  struct utimbuf b;
  b.modtime = t;
  return utime (f, &b);
}

/* cteni casu souboru - f je jmeno, v t vraci time_t */
int GET_FTIME (char *f, time_t *t) {
  struct stat s; int r;
  r = stat (f, &s);
  *t = s.st_mtime;
  return r;
}

/* urceni typu souboru - f je FILE *, v t vraci 1 je-li to konzole ci pod. */
int IS_CON (FILE *f) {
  struct stat s;
  if (!fstat (fileno (f), &s))
    return S_ISCHR (s.st_mode);
  else
    return -1;
}

#ifdef WANT_SET_BINARY
/* zmena typu souboru na binarni */
#ifdef DJGPP
#include <io.h>

void SET_BINARY (FILE *f) {
  setmode (fileno (f), O_BINARY);
}
#else
void SET_BINARY (FILE *f) {

  f = f;
}
#endif
#endif

#ifdef WANT_STRLWR
char *strlwr (char *s) {
  char *ss = s;

  if (ss)
    do { *ss = tolower (*ss); } while (*ss++);
  
  return s;
}
#endif

#ifdef WANT_STRICMP
int stricmp (const char *s1, const char *s2) {
  int r = 0;

  while (!(r = (uchar)(tolower (*s1)) - (uchar)(tolower (*s2))) && (*s1++ && *s2++));
  
  return r;
}
#endif

#ifdef WANT_STRDUP
char *strdup (const char *s) {
  char *ns = NULL;

  if ((ns = malloc (strlen (s) + 1)))
    strcpy (ns, s);
    
  return ns;
}
#endif

#ifdef __cplusplus
}
#endif
