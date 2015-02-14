rem Batch created from SRZIP Makefile (c)1998,1999,2000 Stepan Roh

cp config.dj config.h
cp global.dj global.h
gcc -Wall -Werror -Wno-unused -m486 -O3 -o srzip.o -c srzip.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o config.o -c config.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o bitfile.o -c bitfile.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o parseopt.o -c parseopt.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o crc.o -c crc.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o m_cpy.o -c m_cpy.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o m_rle.o -c m_rle.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o m_shc.o -c m_shc.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o m_ahc.o -c m_ahc.c
gcc -Wall -Werror -Wno-unused -m486 -O3 -o m_bwt.o -c m_bwt.c
ar rsc methods.a m_cpy.o m_rle.o m_shc.o m_ahc.o m_bwt.o
gcc -Wall -Werror -Wno-unused -m486 -O3 -o srzip.exe srzip.o config.o bitfile.o parseopt.o crc.o methods.a
