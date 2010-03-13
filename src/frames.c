#include <stdlib.h>
#include <stdio.h>        /* snprintf() */
#include <inttypes.h>
#include <string.h>
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "common.h"
#include "alias.h"        /* alias_to_frame_id() */

static void print_v24_str_frame(const struct id3v2_frame *frame)
{
    const char *from_enc;

    switch (frame->data[0])
    {
        case ID3V24_STR_ISO88591: from_enc = g_config.enc_iso8859_1; break;
        case ID3V24_STR_UTF16:    from_enc = g_config.enc_utf16; break;
        case ID3V24_STR_UTF16BE:  from_enc = g_config.enc_utf16be; break;
        case ID3V24_STR_UTF8:     from_enc = g_config.enc_utf8; break;
        default:
            print(OS_WARN, "invalid string encoding `%u' in frame `%.4s'",
                  frame->data[0], frame->id);
            return;
    }
    
    lnprint(from_enc, frame->size - 1, frame->data + 1);
}

static void print_v22_str_frame(const struct id3v2_frame *frame)
{
    const char *from_enc;

    switch (frame->data[0])
    {
        case ID3V22_STR_ISO88591: from_enc = g_config.enc_iso8859_1; break;
        case ID3V22_STR_UCS2:     from_enc = g_config.enc_utf16; break;
        default:
            print(OS_WARN, "invalid string encoding `%u' in frame `%.4s'",
                  frame->data[0], frame->id);
            return;
    }
    
    lnprint(from_enc, frame->size - 1, frame->data + 1);
}

static void print_url_frame(const struct id3v2_frame *frame)
{
    lnprint(g_config.enc_iso8859_1, frame->size, frame->data);
}

static void print_hex_frame(const struct id3v2_frame *frame)
{
}

id3_frame_handler_table_t v22_frames[] = {
    /*
       4.19  BUF Recommended buffer size

       4.17  CNT Play counter
       */
    { "COM", print_v22_str_frame, "Comments" },
    /*
       4.21  CRA Audio encryption
       4.20  CRM Encrypted meta frame

       4.6   ETC Event timing codes
       4.13  EQU Equalization

       4.16  GEO General encapsulated object

       4.4   IPL Involved people list

       4.22  LNK Linked information

       4.5   MCI Music CD Identifier
       4.7   MLL MPEG location lookup table

       4.15  PIC Attached picture
       4.18  POP Popularimeter

       4.14  REV Reverb
       4.12  RVA Relative volume adjustment

       4.10  SLT Synchronized lyric/text
       4.8   STC Synced tempo codes
       */
    { "TAL", print_v22_str_frame, "Album/Movie/Show title" },
    { "TBP", print_v22_str_frame, "BPM (Beats Per Minute)" },
    { "TCM", print_v22_str_frame, "Composer" },
    { "TCO", print_v22_str_frame, "Content type" },
    { "TCR", print_v22_str_frame, "Copyright message" },
    { "TDA", print_v22_str_frame, "Date" },
    { "TDY", print_v22_str_frame, "Playlist delay" },
    { "TEN", print_v22_str_frame, "Encoded by" },
    { "TFT", print_v22_str_frame, "File type" },
    { "TIM", print_v22_str_frame, "Time" },
    { "TKE", print_v22_str_frame, "Initial key" },
    { "TLA", print_v22_str_frame, "Language(s)" },
    { "TLE", print_v22_str_frame, "Length" },
    { "TMT", print_v22_str_frame, "Media type" },
    { "TOA", print_v22_str_frame, "Original artist(s)/performer(s)" },
    { "TOF", print_v22_str_frame, "Original filename" },
    { "TOL", print_v22_str_frame, "Original Lyricist(s)/text writer(s)" },
    { "TOR", print_v22_str_frame, "Original release year" },
    { "TOT", print_v22_str_frame, "Original album/Movie/Show title" },
    { "TP1", print_v22_str_frame, "Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group" },
    { "TP2", print_v22_str_frame, "Band/Orchestra/Accompaniment" },
    { "TP3", print_v22_str_frame, "Conductor/Performer refinement" },
    { "TP4", print_v22_str_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPA", print_v22_str_frame, "Part of a set" },
    { "TPB", print_v22_str_frame, "Publisher" },
    { "TRC", print_v22_str_frame, "ISRC (International Standard Recording Code)" },
    { "TRD", print_v22_str_frame, "Recording dates" },
    { "TRK", print_v22_str_frame, "Track number/Position in set" },
    { "TSI", print_v22_str_frame, "Size" },
    { "TSS", print_v22_str_frame, "Software/hardware and settings used for encoding" },
    { "TT1", print_v22_str_frame, "Content group description" },
    { "TT2", print_v22_str_frame, "Title/Songname/Content description" },
    { "TT3", print_v22_str_frame, "Subtitle/Description refinement" },
    { "TXT", print_v22_str_frame, "Lyricist/text writer" },
    { "TXX", print_v22_str_frame, "User defined text information frame" },
    { "TYE", print_v22_str_frame, "Year" },

    /*
       UFI Unique file identifier
       ULT Unsychronized lyric/text transcription
       */

    { "WAF", print_url_frame, "Official audio file webpage" },
    { "WAR", print_url_frame, "Official artist/performer webpage" },
    { "WAS", print_url_frame, "Official audio source webpage" },
    { "WCM", print_url_frame, "Commercial information" },
    { "WCP", print_url_frame, "Copyright/Legal information" },
    { "WPB", print_url_frame, "Publishers official webpage" },
    { "WXX", print_url_frame, "User defined URL link frame" },
    { NULL,   NULL,            NULL }
};

id3_frame_handler_table_t v24_frames[] = {
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
    { "TALB", print_v24_str_frame, "Album/Movie/Show title" },
    { "TBPM", print_v24_str_frame, "BPM (beats per minute)" },
    { "TCOM", print_v24_str_frame, "Composer" },
    { "TCON", print_v24_str_frame, "Content type" },
    { "TCOP", print_v24_str_frame, "Copyright message" },
    { "TDEN", print_v24_str_frame, "Encoding time" },
    { "TDLY", print_v24_str_frame, "Playlist delay" },
    { "TDOR", print_v24_str_frame, "Original release time" },
    { "TDRC", print_v24_str_frame, "Recording time" },
    { "TDRL", print_v24_str_frame, "Release time" },
    { "TDTG", print_v24_str_frame, "Tagging time" },
    { "TENC", print_v24_str_frame, "Encoded by" },
    { "TEXT", print_v24_str_frame, "Lyricist/Text writer" },
    { "TFLT", print_v24_str_frame, "File type" },
    { "TIPL", print_v24_str_frame, "Involved people list" },
    { "TIT1", print_v24_str_frame, "Content group description" },
    { "TIT2", print_v24_str_frame, "Title/songname/content description" },
    { "TIT3", print_v24_str_frame, "Subtitle/Description refinement" },
    { "TKEY", print_v24_str_frame, "Initial key" },
    { "TLAN", print_v24_str_frame, "Language(s)" },
    { "TLEN", print_v24_str_frame, "Length" },
    { "TMCL", print_v24_str_frame, "Musician credits list" },
    { "TMED", print_v24_str_frame, "Media type" },
    { "TMOO", print_v24_str_frame, "Mood" },
    { "TOAL", print_v24_str_frame, "Original album/movie/show title" },
    { "TOFN", print_v24_str_frame, "Original filename" },
    { "TOLY", print_v24_str_frame, "Original lyricist(s)/text writer(s)" },
    { "TOPE", print_v24_str_frame, "Original artist(s)/performer(s)" },
    { "TOWN", print_v24_str_frame, "File owner/licensee" },
    { "TPE1", print_v24_str_frame, "Lead performer(s)/Soloist(s)" },
    { "TPE2", print_v24_str_frame, "Band/orchestra/accompaniment" },
    { "TPE3", print_v24_str_frame, "Conductor/performer refinement" },
    { "TPE4", print_v24_str_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPOS", print_v24_str_frame, "Part of a set" },
    { "TPRO", print_v24_str_frame, "Produced notice" },
    { "TPUB", print_v24_str_frame, "Publisher" },
    { "TRCK", print_v24_str_frame, "Track number/Position in set" },
    { "TRSN", print_v24_str_frame, "Internet radio station name" },
    { "TRSO", print_v24_str_frame, "Internet radio station owner" },
    { "TSOA", print_v24_str_frame, "Album sort order" },
    { "TSOP", print_v24_str_frame, "Performer sort order" },
    { "TSOT", print_v24_str_frame, "Title sort order" },
    { "TSRC", print_v24_str_frame, "ISRC (international standard recording code)" },
    { "TSSE", print_v24_str_frame, "Software/Hardware and settings used for encoding" },
    { "TSST", print_v24_str_frame, "Set subtitle" },
    { "TXXX", print_v24_str_frame, "User defined text information frame" },

    /*
       4.1   UFID Unique file identifier
       4.22  USER Terms of use
       4.8   USLT Unsynchronised lyric/text transcription
     */

    { "WCOM", print_url_frame, "Commercial information" },
    { "WCOP", print_url_frame, "Copyright/Legal information" },
    { "WOAF", print_url_frame, "Official audio file webpage" },
    { "WOAR", print_url_frame, "Official artist/performer webpage" },
    { "WOAS", print_url_frame, "Official audio source webpage" },
    { "WORS", print_url_frame, "Official Internet radio station homepage" },
    { "WPAY", print_url_frame, "Payment" },
    { "WPUB", print_url_frame, "Publishers official webpage" },
    { "WXXX", print_url_frame, "User defined URL link frame" },
    { NULL,   NULL,            NULL }
};

void print_frame_data(const struct id3v2_tag *tag,
                      const struct id3v2_frame *frame)
{
    unsigned i;
    id3_frame_handler_table_t *table = NULL;
    int idlen = tag->header.version == 2 ? 3 : 4;

    switch (tag->header.version)
    {
        case 2: table = v22_frames; break;
        case 3: table = v24_frames; break;
        case 4: table = v24_frames; break;
        default: return; /* no need to report error here */
    }

    for (i = 0; table[i].id != NULL; i++)
    {
        if (!memcmp(table[i].id, frame->id, idlen))
        {
            table[i].print(frame);
            break;
        }
    }
}

int set_id3v2_tag_genre_by_id(struct id3v2_tag *tag, uint8_t genre_id)
{
    const char         *genre_str = get_id3v1_genre_str(genre_id);
    const char         *frame_id = alias_to_frame_id('g', tag->header.version);
    struct id3v2_frame  frame = { };
    const char         *fmt;
    int                 ret;

    if (!genre_str)
        genre_str = "";

    switch (tag->header.version)
    {
        case 4:
            frame.size = 1 +
                snprintf(NULL, 0, "%u", genre_id) + 1 + strlen(genre_str) + 1;

            frame.data = calloc(1, frame.size);
            if (!frame.data)
                return -1;

            frame.data[0] = ID3V24_STR_ISO88591; 
            memcpy(frame.data + 2 + sprintf(frame.data + 1, "%u", genre_id),
                   genre_str, strlen(genre_str));
            break;

        default:
        case 2:
        case 3:
            fmt = "(%u)%s";
            frame.size = 1 + snprintf(NULL, 0, fmt, genre_id, genre_str);
            /* we allocate one extra byte at the end of frame.date to
             * make snprintf happy; it will place null terminator there  */
            frame.data = calloc(1, frame.size + 1);
            if (!frame.data)
                return -1;

            snprintf(frame.data + 1, frame.size, fmt, genre_id, genre_str);
            frame.data[0] = ID3V22_STR_ISO88591;
    }

    strncpy(frame.id, frame_id, 4);

    ret = update_id3v2_tag_frame(tag, &frame);
    if (ret != 0)
        free(frame.data);

    return ret;
}
