AM_CPPFLAGS = -I$(top_srcdir)/include

id321_LDADD = @LIBICONV@

if STATIC_LIB313
AM_CPPFLAGS += -I$(top_srcdir)/lib313/include
id321_LDADD += $(top_srcdir)/lib313/src/lib313.a
endif

bin_PROGRAMS = id321

id321_SOURCES = main.c print.c delete.c modify.c sync.c copy.c \
                get.c write.c crop.c \
                file.c init.c dump.c opts.c \
                framelist.c frames.c \
                textframe.c frm_comm.c frm_tcon.c frm_trck.c \
                id3v1.c id3v1_genres.c id3v1e_speed.c id3v2.c id3v23.c \
                printfmt.c output.c iconv_ucs2.c compat.c u32_char.c \
                synchsafe.c common.c alias.c \
                langcodes.c \
                help.c
