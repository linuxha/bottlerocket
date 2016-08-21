# Generated automatically from Makefile.in by configure.
#
# Makefile for BottleRocket (controller for X10 FireCracker home automation
#  kit)
#

srcdir = .
top_srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include

CFLAGS = -g -O2

CFLAGS += -I. -Wall  -O2 -DX10_PORTNAME=\"/dev/ttyS0\"
DEFS=-DHAVE_CONFIG_H
LIBS=
INSTALL= /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CC=gcc

#
# uncomment if you want to use TX instead of DTR (necessary on some
#   Macs and with Boca boards, etc.
#

# DEFS += -DTIOCM_FOR_0=TIOCM_ST

bin: br

lib: libbr.a

br: br.o libbr.a
	${CC} ${CFLAGS} ${DEFS} -o br br.o -L. -lbr

br.o: ${srcdir}/br.c ${srcdir}/br_cmd.h ${srcdir}/br_cmd_engine.h
	${CC} ${CFLAGS} ${DEFS} -c ${srcdir}/br.c

libbr.a: br_cmd.o br_cmd_engine.o
	${AR} cru libbr.a br_cmd.o br_cmd_engine.o
	
br_cmd.o: ${srcdir}/br_cmd.c ${srcdir}/br_cmd.h ${srcdir}/br_translate.h
	${CC} ${CFLAGS} ${DEFS} -c ${srcdir}/br_cmd.c

br_cmd_engine.o: ${srcdir}/br_cmd_engine.c ${srcdir}/br_cmd_engine.h
	${CC} ${CFLAGS} ${DEFS} -c ${srcdir}/br_cmd_engine.c

install: br
	${INSTALL} -d -m 755 ${bindir}
	${INSTALL} -m 555 br ${bindir}

lib_install: libbr.a br_cmd.h br_cmd_engine.h
	${INSTALL} -d -m 755 ${libdir}
	${INSTALL} -d -m 744 ${includedir}
	${INSTALL} -m 644 libbr.a ${libdir}
	${INSTALL} -m 644 br_cmd.h ${includedir}
	${INSTALL} -m 644 br_cmd_engine.h ${includedir}

clean:
	-rm -f *.o *.a br core

really_clean: clean
	-rm -f config.h config.cache config.status config.log Makefile *.bak
