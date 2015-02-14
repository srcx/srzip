dnl Srzip package configure macros

dnl SZ_SYS_EOL - checking end-of-lines
AC_DEFUN(SZ_SYS_EOL,
[AC_MSG_CHECKING(type of end-of-line)
AC_CACHE_VAL(sz_cv_eol_t, [
AC_TRY_RUN([
#ifdef STDC_HEADERS
#  include <stdio.h>
#endif
int main (void) {
  int c1, c2;
  FILE *f = fopen ("conftesteol", "w+"); if (!f) exit (1);
  fputc ('\n', f); fseek (f, 0, SEEK_SET);
  if ((c1 = fgetc (f)) == EOF) exit (1); c2 = fgetc (f);
  fseek (f, 0, SEEK_SET); fclose (f);
  f = fopen ("conftesteolval", "w"); if (!f) exit (1);
  fprintf (f, "%d:%d\n", c1, c2); fclose (f);
  exit (0);
}], sz_cv_eol_t=`cat conftesteolval`, sz_cv_eol_t='10:-1')])
AC_MSG_RESULT($sz_cv_eol_t)
AC_DEFINE_UNQUOTED(EOL_VAL1, `echo $sz_cv_eol_t | sed "s/:.*$//"`, First end-of-line value or -1)
AC_DEFINE_UNQUOTED(EOL_VAL2, `echo $sz_cv_eol_t | sed "s/^.*://"`, Second end-of-line value or -1)
])

dnl SZ_PROG_LD_S - whether $(CC) -s works
AC_DEFUN(SZ_PROG_LD_S,
[AC_MSG_CHECKING(whether linker works with -s option)
sz_save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -s"
AC_CACHE_VAL(sz_cv_prog_ld_s, [
AC_TRY_LINK(,{},sz_cv_prog_ld_s=yes, sz_cv_prog_ld_s=no)])
LDFLAGS=$sz_save_LDFLAGS
AC_MSG_RESULT($sz_cv_prog_ld_s)
])
