#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "common.h"

static void unpack_string_frame(struct id3v2_frame *frame)
{
    iconv_t     cd;
    char        buf[1024];
    char       *out = buf;
    char       *in = frame->data + 1;
    size_t      inbytesleft = frame->size - 1;
    size_t      outbytesleft = sizeof(buf);
    const char *tocode = "UTF-8";
    const char *fromcode = NULL;

    memset(buf, 0, sizeof(buf));

    switch (frame->data[0])
    {
        case 0: fromcode = g_config.enc_iso8859_1; break;
        case 1: fromcode = g_config.enc_utf16; break;
        case 2: fromcode = g_config.enc_utf16be; break;
        case 3: fromcode = g_config.enc_utf8; break;
    }

    cd = xiconv_open(tocode, fromcode);
    
    errno = 0;
    iconv(cd, &in, &inbytesleft, &out, &outbytesleft);

    if (inbytesleft == 0)
        print(OS_DEBUG, "str: %s", buf);
    else
        print(OS_ERROR, "iconv error: %s", strerror(errno));

    iconv_close(cd);

    free(frame->data);
    frame->data = strdup(buf);
    frame->size = strlen(frame->data);
}

static void unpack_url_frame(struct id3v2_frame *frame)
{
    print(OS_DEBUG, "URL: %s", frame->data + 1);
}

static void unpack_hex_frame(struct id3v2_frame *frame)
{
}

id3_frame_handler_table_t frames[] = {
    /*
       4.19  AENC Audio encryption
       4.14  APIC Attached picture
       4.30  ASPI Audio seek point index

       4.10  COMM Comments
       4.24  COMR Commercial frame

       4.25  ENCR Encryption method registration
       4.12  EQU2 Equalisation (2)
       4.5   ETCO Event timing codes

       4.15  GEOB General encapsulated object
       4.26  GRID Group identification registration

       4.20  LINK Linked information

       4.4   MCDI Music CD identifier
       4.6   MLLT MPEG location lookup table

       4.23  OWNE Ownership frame

       4.27  PRIV Private frame
       4.16  PCNT Play counter
       4.17  POPM Popularimeter
       4.21  POSS Position synchronisation frame

       4.18  RBUF Recommended buffer size
       4.11  RVA2 Relative volume adjustment (2)
       4.13  RVRB Reverb

       4.29  SEEK Seek frame
       4.28  SIGN Signature frame
       4.9   SYLT Synchronised lyric/text
       4.7   SYTC Synchronised tempo codes
       */
    { "TALB", unpack_string_frame, "Album/Movie/Show title" },
    { "TBPM", unpack_string_frame, "BPM (beats per minute)" },
    { "TCOM", unpack_string_frame, "Composer" },
    { "TCON", unpack_string_frame, "Content type" },
    { "TCOP", unpack_string_frame, "Copyright message" },
    { "TDEN", unpack_string_frame, "Encoding time" },
    { "TDLY", unpack_string_frame, "Playlist delay" },
    { "TDOR", unpack_string_frame, "Original release time" },
    { "TDRC", unpack_string_frame, "Recording time" },
    { "TDRL", unpack_string_frame, "Release time" },
    { "TDTG", unpack_string_frame, "Tagging time" },
    { "TENC", unpack_string_frame, "Encoded by" },
    { "TEXT", unpack_string_frame, "Lyricist/Text writer" },
    { "TFLT", unpack_string_frame, "File type" },
    { "TIPL", unpack_string_frame, "Involved people list" },
    { "TIT1", unpack_string_frame, "Content group description" },
    { "TIT2", unpack_string_frame, "Title/songname/content description" },
    { "TIT3", unpack_string_frame, "Subtitle/Description refinement" },
    { "TKEY", unpack_string_frame, "Initial key" },
    { "TLAN", unpack_string_frame, "Language(s)" },
    { "TLEN", unpack_string_frame, "Length" },
    { "TMCL", unpack_string_frame, "Musician credits list" },
    { "TMED", unpack_string_frame, "Media type" },
    { "TMOO", unpack_string_frame, "Mood" },
    { "TOAL", unpack_string_frame, "Original album/movie/show title" },
    { "TOFN", unpack_string_frame, "Original filename" },
    { "TOLY", unpack_string_frame, "Original lyricist(s)/text writer(s)" },
    { "TOPE", unpack_string_frame, "Original artist(s)/performer(s)" },
    { "TOWN", unpack_string_frame, "File owner/licensee" },
    { "TPE1", unpack_string_frame, "Lead performer(s)/Soloist(s)" },
    { "TPE2", unpack_string_frame, "Band/orchestra/accompaniment" },
    { "TPE3", unpack_string_frame, "Conductor/performer refinement" },
    { "TPE4", unpack_string_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPOS", unpack_string_frame, "Part of a set" },
    { "TPRO", unpack_string_frame, "Produced notice" },
    { "TPUB", unpack_string_frame, "Publisher" },
    { "TRCK", unpack_string_frame, "Track number/Position in set" },
    { "TRSN", unpack_string_frame, "Internet radio station name" },
    { "TRSO", unpack_string_frame, "Internet radio station owner" },
    { "TSOA", unpack_string_frame, "Album sort order" },
    { "TSOP", unpack_string_frame, "Performer sort order" },
    { "TSOT", unpack_string_frame, "Title sort order" },
    { "TSRC", unpack_string_frame, "ISRC (international standard recording code)" },
    { "TSSE", unpack_string_frame, "Software/Hardware and settings used for encoding" },
    { "TSST", unpack_string_frame, "Set subtitle" },
    { "TXXX", unpack_string_frame, "User defined text information frame" },

    /*
       4.1   UFID Unique file identifier
       4.22  USER Terms of use
       4.8   USLT Unsynchronised lyric/text transcription
     */

    { "WCOM", unpack_url_frame, "Commercial information" },
    { "WCOP", unpack_url_frame, "Copyright/Legal information" },
    { "WOAF", unpack_url_frame, "Official audio file webpage" },
    { "WOAR", unpack_url_frame, "Official artist/performer webpage" },
    { "WOAS", unpack_url_frame, "Official audio source webpage" },
    { "WORS", unpack_url_frame, "Official Internet radio station homepage" },
    { "WPAY", unpack_url_frame, "Payment" },
    { "WPUB", unpack_url_frame, "Publishers official webpage" },
    { "WXXX", unpack_url_frame, "User defined URL link frame" },

    {NULL,   unpack_hex_frame}
};

void unpack_frame_data(struct id3v2_frame *frame)
{
    int i;

    for (i = 0; frames[i].id != NULL; i++)
    {
        if (memcmp(frames[i].id, frame->id, 4) == 0)
            break;
    }

    frames[i].unpack(frame);
}
