SUBDIRS = . src
EXTRA_DIST = include build docs specs

autoclean: distclean
	find -name Makefile.in -exec rm -r {} \;
	rm -f build/* config.h.in configure aclocal.m4
	rm -rf autom4te.cache

.PHONY: autoclean