#include <config.h>
#include <stdio.h>

void help(void)
{
    puts(
"id321 " VERSION " Copyright (c) 2010, 2021 Vitaly Sinilin\n"
"\n"
"usage: id321 [pr[int]] [VEROPT] [-eENC] [-f FMT|-F FRAME] FILE...\n"
"       id321 mo[dify] [VEROPT] [-eENC] [-EENC] [-x] [-s SIZE] MODOPT... FILE...\n"
"       id321 {rm|delete} [VEROPT] [-x] FILE...\n"
"       id321 sy[nc] VEROPT [-eENC] [-EENC] [-s SIZE] FILE...\n"
"       id321 {cp|copy} [VEROPT] FILE1 FILE2\n"
"\n"
"VEROPT is one of the following:\n"
"       -1[0|1|2|3|e]                 use ID3v1[.x] tag only\n"
"       -2[2|3|4]                     use ID3v2[.x] tag only\n"
"\n"
"MODOPT is one of the following:\n"
"       -t, --title TITLE\n"
"       -a, --artist ARTIST\n"
"       -l, --album ALBUM\n"
"       -y, --year YEAR\n"
"       -c, --comment [[{LANG|*}:]{DESC|*}:]COMMENT\n"
"       -g, --genre {GENRE[:GENRE_STR]|:GENRE_STR}\n"
"       -n, --track TRACKNO\n"
"       -F, --frame FRAME_ID['['{INDEX|*}']'][[:{ENC|bin}]:{-|DATA}]\n"
"\n"
"General options:\n"
"       -u, --unsync                  unsynchronise ID3v2 tags\n"
"       -v, --verbose                 be verbose\n"
"       -V, --version                 print the version number\n"
"       -h, --help                    print this message"
    );
}
