#include <stdlib.h>
#include <stdio.h>        /* snprintf() */
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "common.h"
#include "alias.h"        /* alias_to_frame_id() */

/*
 * Function:     get_id3v2_tag_encoding
 *
 * Description:  gets v2.@minor tag frame encoding name by encoding byte @enc
 *
 * Return value: On success, string with encoding name is returned.
 *               On error, NULL is returned.
 */

const char *get_id3v2_tag_encoding_name(unsigned minor, char enc)
{
    switch (minor)
    {
        case 2:
            switch (enc)
            {
                case ID3V22_STR_ISO88591: return g_config.enc_iso8859_1;
                case ID3V22_STR_UCS2:     return g_config.enc_ucs2;
            }
            break;

        case 3:
            switch (enc)
            {
                case ID3V23_STR_ISO88591: return g_config.enc_iso8859_1;
                case ID3V23_STR_UCS2:     return g_config.enc_ucs2;
            }
            break;

        case 4:
            switch (enc)
            {
                case ID3V24_STR_ISO88591: return g_config.enc_iso8859_1;
                case ID3V24_STR_UTF16:    return g_config.enc_utf16;
                case ID3V24_STR_UTF16BE:  return g_config.enc_utf16be;
                case ID3V24_STR_UTF8:     return g_config.enc_utf8;
            }
            break;

        default:
            assert(1);
    }

    return NULL;
}

static void print_str_frame(unsigned minor, const struct id3v2_frame *frame)
{
    const char *from_enc = get_id3v2_tag_encoding_name(minor, frame->data[0]);

    if (!from_enc)
    {
        print(OS_WARN, "invalid string encoding `%u' in frame `%.4s'",
                       frame->data[0], frame->id);
        return;
    }
    
    lnprint(from_enc, frame->size - 1, frame->data + 1);
}

static void print_v22_str_frame(const struct id3v2_frame *frame)
{
    print_str_frame(2, frame);
}

static void print_v23_str_frame(const struct id3v2_frame *frame)
{
    print_str_frame(3, frame);
}

static void print_v24_str_frame(const struct id3v2_frame *frame)
{
    print_str_frame(4, frame);
}

static void print_url_frame(const struct id3v2_frame *frame)
{
    size_t len = strnlen(frame->data, frame->size);

    lnprint(g_config.enc_iso8859_1, len, frame->data);
}

static void print_hex_frame(const struct id3v2_frame *frame)
{
}

id3_frame_handler_table_t v22_frames[] =
{
    { "BUF", NULL, "Recommended buffer size" },
    { "CNT", NULL, "Play counter" },
    { "COM", print_v22_str_frame, "Comments" },
    { "CRA", NULL, "Audio encryption" },
    { "CRM", NULL, "Encrypted meta frame" },
    { "ETC", NULL, "Event timing codes" },
    { "EQU", NULL, "Equalization" },
    { "GEO", NULL, "General encapsulated object" },
    { "IPL", NULL, "Involved people list" },
    { "LNK", NULL, "Linked information" },
    { "MCI", NULL, "Music CD Identifier" },
    { "MLL", NULL, "MPEG location lookup table" },
    { "PIC", NULL, "Attached picture" },
    { "POP", NULL, "Popularimeter" },
    { "REV", NULL, "Reverb" },
    { "RVA", NULL, "Relative volume adjustment" },
    { "SLT", NULL, "Synchronized lyric/text" },
    { "STC", NULL, "Synced tempo codes" },
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
    { "UFI", NULL, "Unique file identifier" },
    { "ULT", NULL, "Unsychronized lyric/text transcription" },
    { "WAF", print_url_frame, "Official audio file webpage" },
    { "WAR", print_url_frame, "Official artist/performer webpage" },
    { "WAS", print_url_frame, "Official audio source webpage" },
    { "WCM", print_url_frame, "Commercial information" },
    { "WCP", print_url_frame, "Copyright/Legal information" },
    { "WPB", print_url_frame, "Publishers official webpage" },
    { "WXX", print_url_frame, "User defined URL link frame" },
    { NULL,  NULL,            NULL }
};

id3_frame_handler_table_t v23_frames[] =
{
    { "AENC", NULL, "Audio encryption" },
    { "APIC", NULL, "Attached picture" },
    { "COMM", print_v23_str_frame, "Comments" },
    { "COMR", NULL, "Commercial frame" },
    { "ENCR", NULL, "Encryption method registration" },
    { "EQUA", NULL, "Equalization" },
    { "ETCO", NULL, "Event timing codes" },
    { "GEOB", NULL, "General encapsulated object" },
    { "GRID", NULL, "Group identification registration" },
    { "IPLS", print_v23_str_frame, "Involved people list" },
    { "LINK", NULL, "Linked information" },
    { "MCDI", NULL, "Music CD identifier" },
    { "MLLT", NULL, "MPEG location lookup table" },
    { "OWNE", NULL, "Ownership frame" },
    { "PRIV", NULL, "Private frame" },
    { "PCNT", NULL, "Play counter" },
    { "POPM", NULL, "Popularimeter" },
    { "POSS", NULL, "Position synchronisation frame" },
    { "RBUF", NULL, "Recommended buffer size" },
    { "RVAD", NULL, "Relative volume adjustment" },
    { "RVRB", NULL, "Reverb" },
    { "SYLT", NULL, "Synchronized lyric/text" },
    { "SYTC", NULL, "Synchronized tempo codes" },
    { "TALB", print_v23_str_frame, "Album/Movie/Show title" },
    { "TBPM", print_v23_str_frame, "BPM (beats per minute)" },
    { "TCOM", print_v23_str_frame, "Composer" },
    { "TCON", print_v23_str_frame, "Content type" },
    { "TCOP", print_v23_str_frame, "Copyright message" },
    { "TDAT", print_v23_str_frame, "Date" },
    { "TDLY", print_v23_str_frame, "Playlist delay" },
    { "TENC", print_v23_str_frame, "Encoded by" },
    { "TEXT", print_v23_str_frame, "Lyricist/Text writer" },
    { "TFLT", print_v23_str_frame, "File type" },
    { "TIME", print_v23_str_frame, "Time" },
    { "TIT1", print_v23_str_frame, "Content group description" },
    { "TIT2", print_v23_str_frame, "Title/songname/content description" },
    { "TIT3", print_v23_str_frame, "Subtitle/Description refinement" },
    { "TKEY", print_v23_str_frame, "Initial key" },
    { "TLAN", print_v23_str_frame, "Language(s)" },
    { "TLEN", print_v23_str_frame, "Length" },
    { "TMED", print_v23_str_frame, "Media type" },
    { "TOAL", print_v23_str_frame, "Original album/movie/show title" },
    { "TOFN", print_v23_str_frame, "Original filename" },
    { "TOLY", print_v23_str_frame, "Original lyricist(s)/text writer(s)" },
    { "TOPE", print_v23_str_frame, "Original artist(s)/performer(s)" },
    { "TORY", print_v23_str_frame, "Original release year" },
    { "TOWN", print_v23_str_frame, "File owner/licensee" },
    { "TPE1", print_v23_str_frame, "Lead performer(s)/Soloist(s)" },
    { "TPE2", print_v23_str_frame, "Band/orchestra/accompaniment" },
    { "TPE3", print_v23_str_frame, "Conductor/performer refinement" },
    { "TPE4", print_v23_str_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPOS", print_v23_str_frame, "Part of a set" },
    { "TPUB", print_v23_str_frame, "Publisher" },
    { "TRCK", print_v23_str_frame, "Track number/Position in set" },
    { "TRDA", print_v23_str_frame, "Recording dates" },
    { "TRSN", print_v23_str_frame, "Internet radio station name" },
    { "TRSO", print_v23_str_frame, "Internet radio station owner" },
    { "TSIZ", print_v23_str_frame, "Size" },
    { "TSRC", print_v23_str_frame, "ISRC (international standard recording code)" },
    { "TSSE", print_v23_str_frame, "Software/Hardware and settings used for encoding" },
    { "TYER", print_v23_str_frame, "Year" },
    { "TXXX", print_v23_str_frame, "User defined text information frame" },
    { "UFID", NULL, "Unique file identifier" },
    { "USER", NULL, "Terms of use" },
    { "USLT", NULL, "Unsychronized lyric/text transcription" },
    { "WCOM", print_url_frame, "Commercial information" },
    { "WCOP", print_url_frame, "Copyright/Legal information" },
    { "WOAF", print_url_frame, "Official audio file webpage" },
    { "WOAR", print_url_frame, "Official artist/performer webpage" },
    { "WOAS", print_url_frame, "Official audio source webpage" },
    { "WORS", print_url_frame, "Official internet radio station homepage" },
    { "WPAY", print_url_frame, "Payment" },
    { "WPUB", print_url_frame, "Publishers official webpage" },
    { "WXXX", NULL, "User defined URL link frame" },
    { NULL,   NULL, NULL }
};

id3_frame_handler_table_t v24_frames[] = {
    { "AENC", NULL, "Audio encryption" },
    { "APIC", NULL, "Attached picture" },
    { "ASPI", NULL, "Audio seek point index" },
    { "COMM", NULL, "Comments" },
    { "COMR", NULL, "Commercial frame" },
    { "ENCR", NULL, "Encryption method registration" },
    { "EQU2", NULL, "Equalisation (2)" },
    { "ETCO", NULL, "Event timing codes" },
    { "GEOB", NULL, "General encapsulated object" },
    { "GRID", NULL, "Group identification registration" },
    { "LINK", NULL, "Linked information" },
    { "MCDI", NULL, "Music CD identifier" },
    { "MLLT", NULL, "MPEG location lookup table" },
    { "OWNE", NULL, "Ownership frame" },
    { "PRIV", NULL, "Private frame" },
    { "PCNT", NULL, "Play counter" },
    { "POPM", NULL, "Popularimeter" },
    { "POSS", NULL, "Position synchronisation frame" },
    { "RBUF", NULL, "Recommended buffer size" },
    { "RVA2", NULL, "Relative volume adjustment (2)" },
    { "RVRB", NULL, "Reverb" },
    { "SEEK", NULL, "Seek frame" },
    { "SIGN", NULL, "Signature frame" },
    { "SYLT", NULL, "Synchronised lyric/text" },
    { "SYTC", NULL, "Synchronised tempo codes" },
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
    { "UFID", NULL, "Unique file identifier" },
    { "USER", NULL, "Terms of use" },
    { "USLT", NULL, "Unsynchronised lyric/text transcription" },
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
        case 3: table = v23_frames; break;
        case 4: table = v24_frames; break;
        default: return; /* no need to report error here */
    }

    for (i = 0; table[i].id != NULL; i++)
    {
        if (!memcmp(table[i].id, frame->id, idlen))
        {
            if (table[i].print)
                table[i].print(frame);
            else
                printf("[no parser for this frame implemented yet]\n");
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
