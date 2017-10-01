# srzip

**Warning: unmaintained since 2005!**

Compression program srzip is designated rather for demonstration of simple
compress algorithms than for real use. Some implementation notes are in file
doc/tech-notes.html.

## Compilation and installation

Procedure is simple. Unpack distribution archive, switch to directory srzip
and enter these commands on the command line:
```
./configure
make
```
This procedure is intended mainly for systems compatible with Unix. It can
work with packages allowing compilation of Unix programs like Cygwin and
mingw32, but this was not tried. Other systems may require larger changes.
If make chokes on a Makefile syntax, try running GNU make (gmake). If you
have DJGPP (GCC port for DOS) you can run included Makefile.dj copied as
Makefile by running make (without running ./configure). In case you don't
have this utility or it refuses to work, run Makefile.bat.

Configure can have some additional paramaters - see ./configure --help.
Make can have some from the following parameters:
```
all (or nothing)	compiles whole srzip
clean			deletes files generated during compilation which
                        are not needed for srzip's run
cleanall		deletes all files generated during compilation
distclean		deletes all files generated during compilation and
                        configuration
```
If you're using Makefile.dj I suggest to look into it and change settings as
you like.

Name of resulting file is srzip (or with some extension - depends on the
system). It can be run immediately. Installation must be done manually,
srzip is not intended for real use anyway.

Documentation is in the HTML format and is placed in directory doc/.
