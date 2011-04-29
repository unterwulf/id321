AM_CPPFLAGS = -I$(top_srcdir)/compat
id321_LDADD = @LIBICONV@ $(top_srcdir)/compat/libcompat.a

bin_PROGRAMS = id321

id321_SOURCES = \
  alias.c \
  alias.h \
  common.c \
  common.h \
  copy.c \
  delete.c \
  dump.c \
  dump.h \
  file.c \
  file.h \
  framelist.c \
  framelist.h \
  frames.c \
  frames.h \
  frm_comm.c \
  frm_comm.h \
  frm_tcon.c \
  frm_tcon.h \
  frm_trck.c \
  frm_trck.h \
  get.c \
  help.c \
  id3v1.c \
  id3v1e_speed.c \
  id3v1e_speed.h \
  id3v1_genres.c \
  id3v1_genres.h \
  id3v1.h \
  id3v23.c \
  id3v2.c \
  id3v2.h \
  init.c \
  langcodes.c \
  langcodes.h \
  main.c \
  modify.c \
  opts.c \
  opts.h \
  output.c \
  output.h \
  params.h \
  print.c \
  printfmt.c \
  printfmt.h \
  sync.c \
  synchsafe.c \
  synchsafe.h \
  textframe.c \
  textframe.h \
  trim.c \
  trim.h \
  u32_char.c \
  u32_char.h \
  write.c

if STATIC_LIB313
AM_CPPFLAGS += -I$(top_srcdir)/lib313/
id321_LDADD += $(top_srcdir)/lib313/lib313.a
endif
