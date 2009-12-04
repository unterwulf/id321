SUBDIRS = . src
EXTRA_DIST = include docs specs

dist_man1_MANS = id321.1

autoclean: distclean
	find -name Makefile.in -exec rm -r {} \;
	rm -f build/* config.h.in configure aclocal.m4
	rm -rf autom4te.cache

.PHONY: autoclean
