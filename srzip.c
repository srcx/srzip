/* srzip (c)1999,2000 Stepan Roh
 * see license.txt for copying
 */

/* srzip.c
 * hlavni program
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define PROG_NAME	"srzip"
#define PROG_VER	"0.3"
#define PROG_DATE	__DATE__
#define PROG_COPY	"(c)1999,2000 Stepan Roh"

#include "global.h"
#include "parseopt.h"
#include "m_cpy.h"
#include "m_rle.h"
#include "m_shc.h"
#include "m_ahc.h"
#include "m_bwt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max. pocet souboru pro zpracovani */
#define SRZIP_MAX_FILES	1024

/* max. delka jmena souboru */
#define SRZIP_MAX_NAME_LEN	256

/* definice prilisneho stari (v sec.) pro make_time */
#define SRZIP_TIME_LIMIT	(6*30*24*60*60)

/* typ kompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
typedef int (*enc_t)(FILE *, FILE *, fsize_t *, fsize_t *, int, crc_t *, int);

/* typ dekompresni fce */
/* par. : z, do, kolik_z, kolik_do, uroven, CRC, ascii mod */
typedef int (*dec_t)(FILE *, FILE *, fsize_t, fsize_t, int, crc_t *, int);

/* typ kompresni metody */
typedef struct {
  enc_t	enc_f;	/* komprese */
  dec_t	dec_f;	/* dekomprese */
  char *name;	/* jmeno (5 znaku max.) */
} meth_t;

/* tabulka kompresnich metod */
meth_t meth_tab[] = {
  { enc_cpy, dec_cpy, "cpy" },
  { enc_rle, dec_rle, "rle" },
  { enc_shc, dec_shc, "shc" },
  { enc_ahc, dec_ahc, "ahc" },
  { enc_bwt, dec_bwt, "bwt" },
};

/* cislo nejvyssi metody */
#define SRZIP_METHODS	4

/* verze */
char str_ver[] = PROG_NAME " " PROG_VER " (" PROG_DATE ") (" TARGET_NAME ")\n";
/* licence */
char str_lic[] = "FREEWARE " PROG_COPY "\n";
/* help */
char str_help[] = "\
usage: %s [flags and input files in any order]
 -a --ascii       ascii text mode (convert end-of-lines)
 -c --stdout      write on standard output (forces --keep)
 -d --decompress  decompress
 -f --force       force overwrite of output files
 -h --help        this help
 -k --keep        keep original files unchanged
 -l --list        give info about compressed files
 -L --license     display \"license\" information
 -n --noname      do not save or restore the original name and time stamp
 -N --name        save or restore the original name and time stamp
 -q --quiet       suppress all warnings
 -S .suf  --suffix .suf      use suffix .suf on compressed files
 -t --test        test compressed file integrity
 -v --verbose     verbose mode
 -V --version     display version information
 -z --compress    force compression
 -1 --fast .. -9 --best      compression level
 -m m --method m  compression method (%s)
";

/* parametry */
/* kratke */
char so[] = "a-c-d-f-h-l-L-n-N-q-S+t-v-V-1-2-3-4-5-6-7-8-9-z-k-m+";
/* dlouhe */
char *lo[] = {
  "-ascii",	"-stdout",	"-decompress",	"-force",	"-help",
  "-list",	"-license",	"-name",	"-quiet",	"+suffix",
  "-test",	"-verbose",	"-version",	"-fast",	"-best",
  "-compress",	"-keep",	"+method",	"-to-stdout",
#ifdef SRZIP_DEBUG
  "-debug",
#endif
  NULL
};

/* chybova hlaseni */
char err_badopt[] = "error in option '%s'";
char err_bug[]    = "BUG ALERT! File %s, line %d";
char err_mem[]    = "memory allocation error";
char err_method[] = "unknown compression method number %d";
char err_head[]   = "%s: corrupted header";
char err_crc[]    = "%s: checksum error";
char err_name[]   = "%s: cannot create new name";
char err_file[]   = "%s: file damaged";
char err_dec[]    = "%s: decompression error";
char err_enc[]    = "%s: compression error";
char err_short[]  = "%s: cannot truncate filename";
char err_id[]     = "%s: not in srzip format";
char err_term[]   = "compressed data not written to a terminal. Use -f to force compression.\nFor help, type: %s -h";

/* ruzne */
char list_head[]  = "compressed  uncompr. ratio uncompressed_name\n";
char list_row[]	  = " %8lu  %8lu  %5.1f%% %s\n";
char vlist_head[] = "method  crc     date  time  ";
char vlist_row[]  = "%-5s %08x %-12s ";
char vlist_pre[]  = "                            ";

char str_verb_pre[]  = "%s -> %s:\t\t";
char str_verb_post[] = "%5.1f%%\n";

char str_stdin[]  = "<stdin>";
char str_stdout[] = "<stdout>";
char str_null[]   = "<null device>";

typedef struct {
  char *name;	/* id retezec */
  int delta;	/* kolik bytu je nutno docist oproti predchozimu (nutno >=0) */
  int num;	/* vysledne ID cislo */
} ids_t;

/* ID */
char srzip_id[] = "SRZ\022";

/* hlavicka */
typedef struct {
  char		is_time;	/* jsou v hlavicce i name a stamp ? */
  char *	name;		/* orig. jmeno */
  time_t	stamp;		/* orig. cas a datum */
  char		method;		/* komprimacni metoda */
  char		level;		/* kvalita komprimace */
  fsize_t	orig_len;	/* orig. delka */
  fsize_t	size;		/* delka zkomprimovanych dat */
  crc_t		crc;		/* CRC 32 zkomprimovanych dat */
} header_t;

/* zapis hlavicky */
int write_head (FILE *f, header_t h) {

  /* zapis ID */
  if (!fwrite (srzip_id, strlen(srzip_id), 1, f)) return -1;
  /* zapis hlavicky */
  if (fputc (h.is_time, f) < 0) return -1;
  if (h.is_time) {
    if (!fwrite (h.name, strlen (h.name) + 1, 1, f)) return -1;
    h.stamp = MACHINE_TO_HOST_ENDIAN_UL (h.stamp);
    if (!fwrite (&h.stamp, sizeof (time_t), 1, f)) return -1;
  }
  if (fputc (h.method, f) < 0) return -1;
  if (fputc (h.level, f) < 0) return -1;
  h.orig_len = MACHINE_TO_HOST_ENDIAN_UL (h.orig_len);
  if (!fwrite (&h.orig_len, sizeof (fsize_t), 1, f)) return -1;
  h.size = MACHINE_TO_HOST_ENDIAN_UL (h.size);
  if (!fwrite (&h.size, sizeof (fsize_t), 1, f)) return -1;
  h.crc = MACHINE_TO_HOST_ENDIAN_UL (h.crc);
  if (!fwrite (&h.crc, sizeof (crc_t), 1, f)) return -1;

  return 0;
}

/* cteni hlavicky - vraci poradi ID v ids[] */
int read_head (FILE *f, header_t *h) {
  char id[16]; int c, j, i;

  /* cteni ID */
  memset (id, 0, 16);

  if (!fread (id, 1, strlen (srzip_id), f))
    return -3;	/* neprecetlo se vubec nic - konec souboru */
  if (strcmp (srzip_id, id))
    return -4;	/* chybne ID */

  /* cteni hlavicky */
  if ((i = fgetc (f)) < 0) return -1;
  h->is_time = i;
  if (h->is_time) {
    j = 0;
    do {
      if ((c = fgetc (f)) < 0) return -1;
      if (j > SRZIP_MAX_NAME_LEN) return -1;
      h->name[j++] = c;
    } while (c);
    if (!fread (&h->stamp, sizeof (time_t), 1, f)) return -1;
    h->stamp = HOST_TO_MACHINE_ENDIAN_UL (h->stamp);
  }
  if ((i = fgetc (f)) < 0) return -1;
  h->method = i;
  if ((i = fgetc (f)) < 0) return -1;
  h->level = i;
  if (!fread (&h->orig_len, sizeof (fsize_t), 1, f)) return -1;
  h->orig_len = HOST_TO_MACHINE_ENDIAN_UL (h->orig_len);
  if (!fread (&h->size, sizeof (fsize_t), 1, f)) return -1;
  h->size = HOST_TO_MACHINE_ENDIAN_UL (h->size);
  if (!fread (&h->crc, sizeof (crc_t), 1, f)) return -1;
  h->crc = HOST_TO_MACHINE_ENDIAN_UL (h->crc);

  /* kontroly */
  if (h->method > SRZIP_METHODS) return -1;
  if (h->level > 9) return -1;

  return 0;
}

/* test souboru na existenci */
FILE *exist (char *name) {
  FILE *f = fopen (name, "r");
  
  if (!f) return NULL;

  fclose (f);

  return f;
}

/* vyzva pro prepis souboru */
int ask_overwrite (char *name) {
  static char s[256];

  printf ("file %s exists; overwrite (y or n)? ", name);
  scanf ("%s", s);
  if (s[0] != 'y') printf ("skipping %s\n", name);

  return (s[0] == 'y');
}

/* do s ulozi retezcovou reprezentaci casu t */
/* je-li t drive nez pred pul rokem -> Mesic Den Rok,
   jinak Mesic Den Cas */
/* vraci s */
char *	make_time (char *s, time_t t) {

  if (difftime (time (NULL), t) > SRZIP_TIME_LIMIT)
    strftime (s, 12, "%h %e %Y", localtime (&t));
  else
    strftime (s, 13, "%h %e %H:%M", localtime (&t));

  return s;
}

/* priznaky */
int f_ascii = 0;	/* neni ascii mod */
int f_stdout = 0;	/* neni na stdout */
int f_decomp = 0;	/* std. komprese */
int f_force = 0;	/* jsou ohledy */
int f_keep = 0;		/* puvodni nezachovavat */
int f_list = 0;		/* nevypisovat obsah */
int f_name = -1;	/* rozhodne se pozdeji */
int f_quiet = 0;	/* neni ticho */
int f_test = 0;		/* neni test */
int f_verb = 0;		/* neukecany */
int f_level = 9;	/* uroven komprese - std. best */
int f_method = SRZIP_METHODS;	/* metoda - std. posledni */
char f_suffix[32] = SRZIP_SUFFIX;	/* pripona */

/* glob. */
header_t h;
fsize_t t_size = 0, t_orig = 0;
char *file_in_process = NULL;

/* provede vse co je treba s jednim souborem */
/* name = NULL -> stdin */
/* vraci 0 = OK, >0 = WARN, pri chybe panikari */
int process_file (char *name) {
  int result = 0; int n, r; FILE *f, *fw; char *x; crc_t crc; int e;
  static char s[256]; long h_pos, h_pos2;

  if ((n = name ? 1 : 0)) {
    if (!(f = fopen (name, "rb"))) {	/* otevreni souboru */
      PERROR (name); return 1;
    }
  } else {
    f = stdin;
    name = str_stdin;
    f_name = 0;
  }

  /* --list */
  if (f_list) {	/* neco o kazdem souboru */
    /* pres vsechny casti */
    while ((r = read_head (f, &h)) != -3) { /* nacteni hlavicky */
      if (r == -4) {	/* jiny format */
        WARN (err_id, name); result = 2; break;
      } else if (r < 0) {
        WARN (err_head, name); result = 2; break;
      } else {	/* vypocet delky a pomeru */
        t_size += h.size; t_orig += h.orig_len;	/* pricteni k celkovemu */
        if (!h.is_time) {	/* tyto informace nejsou k dispozici */
          strncpy (h.name, basename (name), SRZIP_MAX_NAME_LEN);
          h.stamp = 0;
          if ((x = strstr (h.name + strlen (h.name) - strlen (f_suffix), f_suffix)))	/* konci na to */
            *x = 0;	/* uriznuti */
          else {	/* odriznuti kratkeho */
            x = h.name + (strlen(h.name) - 1);
            if (*x == SRZIP_SHORT_SUFFIX)
              *x = 0;	/* uriznuti */
            else {	/* nepovedlo se nic - chybka */
              WARN (err_name, name);
            }
          }
          /* zkraceni dle systemu */
          if (TRUNC_FILENAME (h.name)) {
            WARN (err_short, h.name);
          }
        }
        /* posun za data */
        if (fseek (f, h.size, SEEK_CUR)) {
          WARN (err_file, name); result = 2; break;
        }
        if (f_verb) printf (vlist_row, meth_tab[(int)h.method].name, h.crc, make_time (s, h.stamp));
        printf (list_row, h.size, h.orig_len, 100 - (float)h.size / (float)h.orig_len * 100, h.name);
      }
    }
  /* --decompress */
  } else if (f_decomp) {	/* dekomprese */
    /* v jednom souboru muze byt i vic */
    while ((r = read_head (f, &h)) != -3) {
      if (r == -4) {	/* jiny format */
        WARN (err_id, name); result = 2; break;
      } else if (r < 0) {	/* nacteni hlavicky */
        WARN (err_head, name); result = 2; break;
      } else {	/* jdem na to */
        if (f_stdout) {
          strncpy (h.name, str_stdout, SRZIP_MAX_NAME_LEN);
          fw = stdout;	/* na stdout */
        } else if (f_test) {	/* pri testu jde do null */
          strncpy (h.name, str_null, SRZIP_MAX_NAME_LEN);
          if (!(fw = FOPEN_NULL ())) {
            PERROR (h.name); result = 2; break;
          }
        } else {
          if (!h.is_time || !f_name) {  /* neni ci nechci */
            if (!n) {	/* nelze zkonstruovat jmeno a vystup nejde na stdout */
              WARN (err_name, name); result = 2; break;
            }
            strncpy (h.name, name, SRZIP_MAX_NAME_LEN);
            if ((x = strstr (h.name + strlen (h.name) - strlen (f_suffix), f_suffix)))	/* konci na to */
              *x = 0;	/* uriznuti */
            else {	/* odriznuti kratkeho */
              x = h.name + (strlen(h.name) - 1);
              if (*x == SRZIP_SHORT_SUFFIX)
                *x = 0;	/* uriznuti */
              else {	/* nepovedlo se nic - chybka */
                WARN (err_name, name); result = 2; break;
              }
            }
          } else {
            /* konstrukce noveho jmena */
            char *x = basename (name); char tbuf[256];
            strncpy (tbuf, name, (x - name));
            #ifdef HAVE_SLASH_DELIM
            if (*x != '/')
            #endif
              #ifdef HAVE_BACKSLASH_DELIM
              if (*x != '\\')
              #endif
                #ifdef HAVE_COLON_DELIM
                if (*x != ':')
                #endif
                  #ifdef HAVE_SLASH_DELIM
                  strncat (tbuf, "/", 255);
                  #elif defined (HAVE_BACKSLASH_DELIM)
                  strncat (tbuf, "\\", 255);
                  #endif
            strncat (tbuf, h.name, 255);
          }
          /* zkraceni dle systemu */
          if (TRUNC_FILENAME (h.name)) {
            WARN (err_short, h.name); result = 2; break;
          }
          /* test existence */
          if (exist (h.name) && !f_force) {	/* vyzva */
            if (!ask_overwrite (h.name)) break;
          }
          file_in_process = h.name;
          if (!(fw = fopen (h.name, "wb"))) {	/* otevreni souboru pro zapis */
            PERROR (h.name); result = 2; break;
          }
        }
        /* dekomprese */
        if (f_verb) { printf (str_verb_pre, name, h.name); fflush (stdout); }
        if ((e = meth_tab[(int)h.method].dec_f (f, fw, h.size, h.orig_len, h.level, &crc, f_ascii))) {
          switch (e) {
            case M_ERR_OUTPUT :
              PERROR (h.name); break;
            case M_ERR_DAMAGE :
              WARN (err_file, name); break;
            case M_ERR_MEM :
              PANIC (err_mem); break;
            case M_ERR_OTHER :
              WARN (err_dec, name); break;
          }
          if (!f_stdout) fclose (fw); result = 2; break;
        }
        if (f_verb) printf (str_verb_post, 100 - (float)h.size / (float)h.orig_len * 100);
        if (!f_stdout) {
          fclose (fw);
          /* zmena casu */
          if ((h.is_time && !f_test) && f_name) {
            if (SET_FTIME (h.name, h.stamp)) {
              PERROR (h.name); result = 2; break;
            }
          }
        }
        file_in_process = NULL;
        /* porovnani CRC */
        if (h.crc != (crc ^ CRC_INIT)) {
          WARN (err_crc, name); result = 2; break;
        }
        /* zruseni zkomprimovaneho */
        if (!f_keep && remove (name)) {
          PERROR (name); result = 2; break;
        }
      }
    }
  /* --compress */
  } else {	/* komprese */
    /* utvoreni hlavicky */
    if ((h.is_time = f_name && n)) {	/* chci jmeno ? */
      strncpy (h.name, basename (name), SRZIP_MAX_NAME_LEN);
      if (GET_FTIME (name, &h.stamp)) {
        PERROR (h.name); result = 2; goto lend;
      }
    }
    h.method = f_method;
    h.level = f_level;
    /* budou doplneny po kompresi */
    h.orig_len = 0;
    h.size = 0;
    h.crc = 0;
    /* dotvoreni jmena zkomprimovaneho souboru a jeho otevreni */
    if (f_stdout) {	/* stdout */
      strncpy (s, str_stdout, SRZIP_MAX_NAME_LEN);
      fw = stdout;
    } else if (f_test) {	/* test jde do null */
      strncpy (s, str_null, SRZIP_MAX_NAME_LEN);
      if (!(fw = FOPEN_NULL ())) {
        PERROR (s); result = 2; goto lend;
      }
    } else {	/* konstrukce jmena */
      /* konstrukce */
      strncpy (s, name, 255);
      strncat (s, f_suffix, 255);
      /* zkraceni dle systemu */
      if (TRUNC_FILENAME (s)) {
        WARN (err_short, s); result = 2; goto lend;
      }
      /* test existence */
      if (exist (s) && !f_force) {	/* vyzva */
        if (!ask_overwrite (s)) goto lend;
      }
      file_in_process = s;
      if (!(fw = fopen (s, "wb"))) {	/* otevreni souboru pro zapis */
        PERROR (s); result = 2; goto lend;
      }
    }
    /* zapis hlavicky */
    h_pos = ftell (fw);
    if (write_head (fw, h)) {
      PERROR (s); result = 2; goto lend;
    }
    /* komprese */
    if (f_verb) { printf (str_verb_pre, name, s); fflush (stdout); }
    if ((e = meth_tab[(int)h.method].enc_f (f, fw, &h.orig_len, &h.size, h.level, &h.crc, f_ascii))) {
      switch (e) {
        case M_ERR_OUTPUT :
          PERROR (h.name); break;
        case M_ERR_DAMAGE :
          WARN (err_file, name); break;
        case M_ERR_MEM :
          PANIC (err_mem); break;
        case M_ERR_OTHER :
          WARN (err_enc, name); break;
      }
      if (!f_stdout) fclose (fw); result = 2; goto lend;
    }
    h.crc ^= CRC_INIT;
    if (f_verb) printf (str_verb_post, 100 - (float)h.size / (float)h.orig_len * 100);
    /* doplneni hlavicky */
    h_pos2 = ftell (fw);
    fseek (fw, h_pos, SEEK_SET);
    if (write_head (fw, h)) {
      PERROR (name); if (!f_stdout) fclose (fw); result = 2; goto lend;
    }
    fseek (fw, h_pos2, SEEK_SET);
    file_in_process = NULL;
    /* zavreni souboru */
    if (!f_stdout)
      fclose (fw);
    /* zruseni originalu */
    if (!f_keep && remove (name)) {
      PERROR (name); result = 2; goto lend;
    }
  }

  lend:
  if (n)
    fclose (f);

  file_in_process = NULL;

  return result;
}

/* osetreni signalu */
void signal_handler (int signum) {
  if (file_in_process) remove (file_in_process);
  exit (0);
}

/* hlavni prog. */
int main (int argc, char **argv) {
  char s[256]; int i; int r; char *s2;

  /* celkovy vysledek */
  int result = 0;

  /* pole souboru */
  static char *files[SRZIP_MAX_FILES];
  static int last_file = -1;

  /* separace jmena (odriznutim cesty) */
  strncpy (s, argv[0], 255);
  prog_name[31] = 0;
  strncpy (prog_name, strlwr (STRIP_EXE_EXT (GET_FILENAME (s))), 31);
  /* pro srunzip */
  if (!strcmp (prog_name, "srunzip")) {
    f_decomp = 1;
  /* pro srzcat */
  } else if (!strcmp (prog_name, "srzcat")) {
    f_decomp = 1;
    f_stdout = 1;
    f_keep = 1;
  }

  init_parse_opts (argc, argv, so, lo);

  while ((i = parse_opts (s)) || s[0]) {
    switch (i) {
      case 256 :
      case 'a' :	/* ascii mod */
        f_ascii = (s[0] == '1') || (!s[0]);
        break;
      case 257 :
      case 274 :
      case 'c' :	/* vystup na stdout */
        f_keep = f_stdout = (s[0] == '1') || (!s[0]);
        break;
      case 258 :
      case 'd' :	/* dekomprese */
        f_decomp = (s[0] == '1') || (!s[0]);
        break;
      case 259 :
      case 'f' :	/* zadne ohledy */
        f_force = (s[0] == '1') || (!s[0]);
        break;
      case 260 :
      case 'h' :	/* help */
        fputs (str_ver, stderr);
        /* vytvoreni textu jedn. metod */
        s[0] = 0; s2 = s;
        for (i = 0; i <= SRZIP_METHODS; i++) {
          *s2++ = i + '0';
          strcpy (s2, " = ");
          s2 += 3;
          strcpy (s2, meth_tab[i].name);
          if (i != SRZIP_METHODS) strcat (s2, ", ");
          s2 += strlen (s2);
        }
        fprintf (stderr, str_help, argv[0], s);
        return 0;
      case 272 :
      case 'k' :	/* zachovat puvodni soubory */
        f_keep = (s[0] == '1') || (!s[0]);
        break;
      case 261 :	/* vystup obsahu */
      case 'l' :
        f_list = (s[0] == '1') || (!s[0]);
        break;
      case 262 :
      case 'L' :	/* licence */
        fputs (str_ver, stderr);
        fputs (str_lic, stderr);
        return 0;
      case 263 :
      case 'n' :
      case 'N' :	/* zachovavat jmeno a cas ? */
        f_name = ((i == 'N') || (s[0] == '1'));
        break;
      case 264 :
      case 'q' :	/* ticho */
        f_quiet = (s[0] == '1') || (!s[0]);
        break;
      case 265 :
      case 'S' :	/* pripona */
        strcpy (f_suffix, s);
        break;
      case 266 :
      case 't' :	/* test */
        f_test = (s[0] == '1') || (!s[0]);
        break;
      case 267 :
      case 'v' :	/* ukecanost */
        f_verb = (s[0] == '1') || (!s[0]);
        break;
      case 268 :
      case 'V' :	/* verze */
        fputs (str_ver, stderr);
        return 0;
      case 271 :
      case 'z' :	/* komprese */
        f_decomp = (s[0] == '0');
        break;
      case 269 :	/* = -1 */
        f_level = 1;
        break;
      case 270 :	/* = -9 */
        f_level = 9;
        break;
      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
      case '6' :
      case '7' :
      case '8' :
      case '9' :	/* kvalita */
        f_level = i - '0';
        break;
      case 'm' :
      case 273 :	/* metoda */
        if ((s[0] - '0') > SRZIP_METHODS)
          PANIC (err_method, s[0] - '0');
        else
          f_method = s[0] - '0';
        break;
      case 275 :	/* ladici priznak */
        #ifdef SRZIP_DEBUG
        debug_flag = atoi (s);
        #endif
        break;
      default :	/* zbytek */
        if (i < 0)
          PANIC (err_badopt, s);
        else if (!i) {	/* volny par. = jmeno souboru */
          if (last_file < SRZIP_MAX_FILES)
            if (!(files[++last_file] = strdup (s))) PANIC (err_mem);
        } else
          PANIC (err_bug, __FILE__, __LINE__);
        break;
    }
  }

  /* rozhodnuti o uchovani jmen dle cinnosti */
  if (f_name == -1)
    f_name = !f_decomp;	/* pri dekompresi je std. nezachovat */
  if (f_test) {
    f_decomp = 1;	/* pri testu se provadi dekomprese */
    f_keep = 1;
  }
  if (last_file == -1) {
    f_keep = 1;
    if (!f_list && !f_test)
      f_stdout = 1;	/* nejsou-li zadne soubory, tak na stdout */
    /* kontrola konzole */
    if (!f_decomp && !f_list && !f_force && IS_CON (stdout))
      PANIC (err_term, prog_name);
  }
  if (f_stdout) {	/* zruseni textoveho vystupu na stdout */
    f_force = 1;
    f_verb = 0;
    /* otevreni stdout v binarnim rezimu */
    SET_BINARY (stdout);
  }
  be_quiet = f_quiet;	/* do globalniho */

  /* priprava */
  if (f_list && !f_quiet) {
    if (f_verb) printf (vlist_head);
    printf (list_head);
  }

  if (!(h.name = malloc (SRZIP_MAX_NAME_LEN+2)))
    PANIC (err_mem);

  init_crc ();
  (void) bfinit ();
  
  /* registrace signal handleru */
  signal (SIGINT, signal_handler);
  signal (SIGHUP, signal_handler);
  signal (SIGTERM, signal_handler);

  /* zadne soubory */
  if (last_file == -1) {
    SET_BINARY (stdin);
    result = process_file (NULL);
  }

  /* pres vsechny soubory */
  for (i = 0; i <= last_file; i++) {
    r = process_file (files[i]);
    if (!result) result = r;
  }

  /* ukonceni */
  free (h.name);

  /* celkovy vysledek */
  if (f_list && !f_quiet) {
    if (f_verb) printf (vlist_pre);
    printf (list_row, t_size, t_orig, 100 - (float)t_size / (float)t_orig * 100, "(totals)");
  }

  return result;
}

#ifdef __cplusplus
}
#endif

