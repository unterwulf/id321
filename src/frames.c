#define _GNU_SOURCE       /* strnlen(), wcsdup() */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>        /* snprintf() */
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "id3v1_genres.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "common.h"
#include "compat.h"
#include "alias.h"        /* alias_to_frame_id() */
#include "framelist.h"
#include "frm_comm.h"

/***
 * get_id3v2_tag_encoding - gets v2 tag frame encoding name by encoding byte
 *
 * @minor - v2 tag minor version
 * @enc - encoding byte
 *
 * Returns string with encoding name on success, or NULL on error.
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
            assert(0);
    }

    return NULL;
}

char get_id3v2_tag_encoding_byte(unsigned minor, const char *enc_name)
{
    if (!strcasecmp(enc_name, g_config.enc_iso8859_1))
    {
        switch (minor)
        {
            case 2: return ID3V22_STR_ISO88591;
            case 3: return ID3V23_STR_ISO88591;
            case 4: return ID3V24_STR_ISO88591;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_ucs2))
    {
        switch (minor)
        {
            case 2: return ID3V22_STR_UCS2;
            case 3: return ID3V23_STR_UCS2;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_utf16))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF16;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_utf16be))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF16BE;
        }
    }
    else if (!strcasecmp(enc_name, g_config.enc_utf8))
    {
        switch (minor)
        {
            case 4: return ID3V24_STR_UTF8;
        }
    }

    return ID3V2_UNSUPPORTED_ENCODING;
}

static int get_comm_frame(unsigned minor, const struct id3v2_frame *frame,
                          wchar_t *buf, size_t size)
{
    size_t reqsize;
    struct id3v2_frm_comm *comm;
    int ret;

    ret = unpack_id3v2_frm_comm(minor, frame, &comm);

    if (ret != 0)
        return ret;

    /* reserve six extra chars for " []: " and null-terminator */
    reqsize = (comm->desc ? wcslen(comm->desc) : 0) +
              (comm->text ? wcslen(comm->text) : 0) + ID3V2_LANG_HDR_SIZE + 6;

    if (reqsize <= size)
    {
        wchar_t lang[ID3V2_LANG_HDR_SIZE] = { };

        iconvordie(WCHAR_CODESET, "ISO-8859-1",
                   comm->lang, ID3V2_LANG_HDR_SIZE,
                   (char *)lang, sizeof(lang)*sizeof(lang[0]));

        swprintf(buf, size, L"%ls%ls[%.*ls]: %ls",
                 comm->desc ? comm->desc : L"",
                 comm->desc && comm->desc[0] ? L" " : L"",
                 ID3V2_LANG_HDR_SIZE, lang, comm->text ? comm->text : L"");
    }

    free_id3v2_frm_comm(comm);
    return reqsize;
}

static int get_v22_comm_frame(const struct id3v2_frame *frame,
                              wchar_t *buf, size_t size)
{
    return get_comm_frame(2, frame, buf, size);
}

static int get_v23_comm_frame(const struct id3v2_frame *frame,
                              wchar_t *buf, size_t size)
{
    return get_comm_frame(3, frame, buf, size);
}

static int get_v24_comm_frame(const struct id3v2_frame *frame,
                              wchar_t *buf, size_t size)
{
    return get_comm_frame(4, frame, buf, size);
}

static int get_str_frame(unsigned minor, const struct id3v2_frame *frame,
                         wchar_t *buf, size_t size)
{
    const char *from_enc;

    if (frame->size < 1)
        return -EILSEQ;

    from_enc = get_id3v2_tag_encoding_name(minor, frame->data[0]);

    if (!from_enc)
    {
        print(OS_WARN, "invalid string encoding 0x%.2X in frame '%.4s'",
                       (unsigned char)frame->data[0], frame->id);
        return -EILSEQ;
    }

    return iconvordie(WCHAR_CODESET, from_enc,
                      frame->data + ID3V2_ENC_HDR_SIZE,
                      frame->size - ID3V2_ENC_HDR_SIZE,
                      (char *)buf, size*sizeof(wchar_t)) / sizeof(wchar_t);
}

static int get_v22_str_frame(const struct id3v2_frame *frame,
                             wchar_t *buf, size_t size)
{
    return get_str_frame(2, frame, buf, size);
}

static int get_v23_str_frame(const struct id3v2_frame *frame,
                             wchar_t *buf, size_t size)
{
    return get_str_frame(3, frame, buf, size);
}

static int get_v24_str_frame(const struct id3v2_frame *frame,
                             wchar_t *buf, size_t size)
{
    return get_str_frame(4, frame, buf, size);
}

static int get_url_frame(const struct id3v2_frame *frame,
                         wchar_t *buf, size_t size)
{
    size_t slen = strnlen(frame->data, frame->size);

    return iconvordie(WCHAR_CODESET, g_config.enc_iso8859_1,
                      frame->data, slen,
                      (char *)buf, size*sizeof(wchar_t)) / sizeof(wchar_t);
}

static int get_hex_frame(const struct id3v2_frame *frame,
                         wchar_t *buf, size_t size)
{
    return 0;
}

id3_frame_handler_table_t v22_frames[] =
{
    { "BUF", NULL, "Recommended buffer size" },
    { "CNT", NULL, "Play counter" },
    { "COM", get_v22_comm_frame, "Comments" },
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
    { "TAL", get_v22_str_frame, "Album/Movie/Show title" },
    { "TBP", get_v22_str_frame, "BPM (Beats Per Minute)" },
    { "TCM", get_v22_str_frame, "Composer" },
    { "TCO", get_v22_str_frame, "Content type" },
    { "TCR", get_v22_str_frame, "Copyright message" },
    { "TDA", get_v22_str_frame, "Date" },
    { "TDY", get_v22_str_frame, "Playlist delay" },
    { "TEN", get_v22_str_frame, "Encoded by" },
    { "TFT", get_v22_str_frame, "File type" },
    { "TIM", get_v22_str_frame, "Time" },
    { "TKE", get_v22_str_frame, "Initial key" },
    { "TLA", get_v22_str_frame, "Language(s)" },
    { "TLE", get_v22_str_frame, "Length" },
    { "TMT", get_v22_str_frame, "Media type" },
    { "TOA", get_v22_str_frame, "Original artist(s)/performer(s)" },
    { "TOF", get_v22_str_frame, "Original filename" },
    { "TOL", get_v22_str_frame, "Original Lyricist(s)/text writer(s)" },
    { "TOR", get_v22_str_frame, "Original release year" },
    { "TOT", get_v22_str_frame, "Original album/Movie/Show title" },
    { "TP1", get_v22_str_frame, "Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group" },
    { "TP2", get_v22_str_frame, "Band/Orchestra/Accompaniment" },
    { "TP3", get_v22_str_frame, "Conductor/Performer refinement" },
    { "TP4", get_v22_str_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPA", get_v22_str_frame, "Part of a set" },
    { "TPB", get_v22_str_frame, "Publisher" },
    { "TRC", get_v22_str_frame, "ISRC (International Standard Recording Code)" },
    { "TRD", get_v22_str_frame, "Recording dates" },
    { "TRK", get_v22_str_frame, "Track number/Position in set" },
    { "TSI", get_v22_str_frame, "Size" },
    { "TSS", get_v22_str_frame, "Software/hardware and settings used for encoding" },
    { "TT1", get_v22_str_frame, "Content group description" },
    { "TT2", get_v22_str_frame, "Title/Songname/Content description" },
    { "TT3", get_v22_str_frame, "Subtitle/Description refinement" },
    { "TXT", get_v22_str_frame, "Lyricist/text writer" },
    { "TXX", get_v22_str_frame, "User defined text information frame" },
    { "TYE", get_v22_str_frame, "Year" },
    { "UFI", NULL, "Unique file identifier" },
    { "ULT", NULL, "Unsychronized lyric/text transcription" },
    { "WAF", get_url_frame, "Official audio file webpage" },
    { "WAR", get_url_frame, "Official artist/performer webpage" },
    { "WAS", get_url_frame, "Official audio source webpage" },
    { "WCM", get_url_frame, "Commercial information" },
    { "WCP", get_url_frame, "Copyright/Legal information" },
    { "WPB", get_url_frame, "Publishers official webpage" },
    { "WXX", get_url_frame, "User defined URL link frame" },
    { NULL,  NULL,            NULL }
};

id3_frame_handler_table_t v23_frames[] =
{
    { "AENC", NULL, "Audio encryption" },
    { "APIC", NULL, "Attached picture" },
    { "COMM", get_v23_comm_frame, "Comments" },
    { "COMR", NULL, "Commercial frame" },
    { "ENCR", NULL, "Encryption method registration" },
    { "EQUA", NULL, "Equalization" },
    { "ETCO", NULL, "Event timing codes" },
    { "GEOB", NULL, "General encapsulated object" },
    { "GRID", NULL, "Group identification registration" },
    { "IPLS", get_v23_str_frame, "Involved people list" },
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
    { "TALB", get_v23_str_frame, "Album/Movie/Show title" },
    { "TBPM", get_v23_str_frame, "BPM (beats per minute)" },
    { "TCOM", get_v23_str_frame, "Composer" },
    { "TCON", get_v23_str_frame, "Content type" },
    { "TCOP", get_v23_str_frame, "Copyright message" },
    { "TDAT", get_v23_str_frame, "Date" },
    { "TDLY", get_v23_str_frame, "Playlist delay" },
    { "TENC", get_v23_str_frame, "Encoded by" },
    { "TEXT", get_v23_str_frame, "Lyricist/Text writer" },
    { "TFLT", get_v23_str_frame, "File type" },
    { "TIME", get_v23_str_frame, "Time" },
    { "TIT1", get_v23_str_frame, "Content group description" },
    { "TIT2", get_v23_str_frame, "Title/songname/content description" },
    { "TIT3", get_v23_str_frame, "Subtitle/Description refinement" },
    { "TKEY", get_v23_str_frame, "Initial key" },
    { "TLAN", get_v23_str_frame, "Language(s)" },
    { "TLEN", get_v23_str_frame, "Length" },
    { "TMED", get_v23_str_frame, "Media type" },
    { "TOAL", get_v23_str_frame, "Original album/movie/show title" },
    { "TOFN", get_v23_str_frame, "Original filename" },
    { "TOLY", get_v23_str_frame, "Original lyricist(s)/text writer(s)" },
    { "TOPE", get_v23_str_frame, "Original artist(s)/performer(s)" },
    { "TORY", get_v23_str_frame, "Original release year" },
    { "TOWN", get_v23_str_frame, "File owner/licensee" },
    { "TPE1", get_v23_str_frame, "Lead performer(s)/Soloist(s)" },
    { "TPE2", get_v23_str_frame, "Band/orchestra/accompaniment" },
    { "TPE3", get_v23_str_frame, "Conductor/performer refinement" },
    { "TPE4", get_v23_str_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPOS", get_v23_str_frame, "Part of a set" },
    { "TPUB", get_v23_str_frame, "Publisher" },
    { "TRCK", get_v23_str_frame, "Track number/Position in set" },
    { "TRDA", get_v23_str_frame, "Recording dates" },
    { "TRSN", get_v23_str_frame, "Internet radio station name" },
    { "TRSO", get_v23_str_frame, "Internet radio station owner" },
    { "TSIZ", get_v23_str_frame, "Size" },
    { "TSRC", get_v23_str_frame, "ISRC (international standard recording code)" },
    { "TSSE", get_v23_str_frame, "Software/Hardware and settings used for encoding" },
    { "TYER", get_v23_str_frame, "Year" },
    { "TXXX", get_v23_str_frame, "User defined text information frame" },
    { "UFID", NULL, "Unique file identifier" },
    { "USER", NULL, "Terms of use" },
    { "USLT", NULL, "Unsychronized lyric/text transcription" },
    { "WCOM", get_url_frame, "Commercial information" },
    { "WCOP", get_url_frame, "Copyright/Legal information" },
    { "WOAF", get_url_frame, "Official audio file webpage" },
    { "WOAR", get_url_frame, "Official artist/performer webpage" },
    { "WOAS", get_url_frame, "Official audio source webpage" },
    { "WORS", get_url_frame, "Official internet radio station homepage" },
    { "WPAY", get_url_frame, "Payment" },
    { "WPUB", get_url_frame, "Publishers official webpage" },
    { "WXXX", NULL, "User defined URL link frame" },
    { NULL,   NULL, NULL }
};

id3_frame_handler_table_t v24_frames[] = {
    { "AENC", NULL, "Audio encryption" },
    { "APIC", NULL, "Attached picture" },
    { "ASPI", NULL, "Audio seek point index" },
    { "COMM", get_v24_comm_frame, "Comments" },
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
    { "TALB", get_v24_str_frame, "Album/Movie/Show title" },
    { "TBPM", get_v24_str_frame, "BPM (beats per minute)" },
    { "TCOM", get_v24_str_frame, "Composer" },
    { "TCON", get_v24_str_frame, "Content type" },
    { "TCOP", get_v24_str_frame, "Copyright message" },
    { "TDEN", get_v24_str_frame, "Encoding time" },
    { "TDLY", get_v24_str_frame, "Playlist delay" },
    { "TDOR", get_v24_str_frame, "Original release time" },
    { "TDRC", get_v24_str_frame, "Recording time" },
    { "TDRL", get_v24_str_frame, "Release time" },
    { "TDTG", get_v24_str_frame, "Tagging time" },
    { "TENC", get_v24_str_frame, "Encoded by" },
    { "TEXT", get_v24_str_frame, "Lyricist/Text writer" },
    { "TFLT", get_v24_str_frame, "File type" },
    { "TIPL", get_v24_str_frame, "Involved people list" },
    { "TIT1", get_v24_str_frame, "Content group description" },
    { "TIT2", get_v24_str_frame, "Title/songname/content description" },
    { "TIT3", get_v24_str_frame, "Subtitle/Description refinement" },
    { "TKEY", get_v24_str_frame, "Initial key" },
    { "TLAN", get_v24_str_frame, "Language(s)" },
    { "TLEN", get_v24_str_frame, "Length" },
    { "TMCL", get_v24_str_frame, "Musician credits list" },
    { "TMED", get_v24_str_frame, "Media type" },
    { "TMOO", get_v24_str_frame, "Mood" },
    { "TOAL", get_v24_str_frame, "Original album/movie/show title" },
    { "TOFN", get_v24_str_frame, "Original filename" },
    { "TOLY", get_v24_str_frame, "Original lyricist(s)/text writer(s)" },
    { "TOPE", get_v24_str_frame, "Original artist(s)/performer(s)" },
    { "TOWN", get_v24_str_frame, "File owner/licensee" },
    { "TPE1", get_v24_str_frame, "Lead performer(s)/Soloist(s)" },
    { "TPE2", get_v24_str_frame, "Band/orchestra/accompaniment" },
    { "TPE3", get_v24_str_frame, "Conductor/performer refinement" },
    { "TPE4", get_v24_str_frame, "Interpreted, remixed, or otherwise modified by" },
    { "TPOS", get_v24_str_frame, "Part of a set" },
    { "TPRO", get_v24_str_frame, "Produced notice" },
    { "TPUB", get_v24_str_frame, "Publisher" },
    { "TRCK", get_v24_str_frame, "Track number/Position in set" },
    { "TRSN", get_v24_str_frame, "Internet radio station name" },
    { "TRSO", get_v24_str_frame, "Internet radio station owner" },
    { "TSOA", get_v24_str_frame, "Album sort order" },
    { "TSOP", get_v24_str_frame, "Performer sort order" },
    { "TSOT", get_v24_str_frame, "Title sort order" },
    { "TSRC", get_v24_str_frame, "ISRC (international standard recording code)" },
    { "TSSE", get_v24_str_frame, "Software/Hardware and settings used for encoding" },
    { "TSST", get_v24_str_frame, "Set subtitle" },
    { "TXXX", get_v24_str_frame, "User defined text information frame" },
    { "UFID", NULL, "Unique file identifier" },
    { "USER", NULL, "Terms of use" },
    { "USLT", NULL, "Unsynchronised lyric/text transcription" },
    { "WCOM", get_url_frame, "Commercial information" },
    { "WCOP", get_url_frame, "Copyright/Legal information" },
    { "WOAF", get_url_frame, "Official audio file webpage" },
    { "WOAR", get_url_frame, "Official artist/performer webpage" },
    { "WOAS", get_url_frame, "Official audio source webpage" },
    { "WORS", get_url_frame, "Official Internet radio station homepage" },
    { "WPAY", get_url_frame, "Payment" },
    { "WPUB", get_url_frame, "Publishers official webpage" },
    { "WXXX", get_url_frame, "User defined URL link frame" },
    { NULL,   NULL,            NULL }
};

/***
 * get_frame_data - get frame payload as wcs
 *
 * @tag - tag which frame belongs to
 * @frame - frame
 * @buf - output buffer
 * @size - output buffer size
 *
 * Returns -EINVAL if the tag has unsupported version,
 *         -ENOSYS if parser for the frame is not implemented,
 *         -EILSEQ on parser error,
 *         or non-negative number of wchars which would be written if @buf was
 *         large enough.
 */

int get_frame_data(const struct id3v2_tag *tag, const struct id3v2_frame *frame,
                   wchar_t *buf, size_t size)
{
    unsigned i;
    id3_frame_handler_table_t *table = NULL;
    int idlen = tag->header.version == 2 ? 3 : 4;

    switch (tag->header.version)
    {
        case 2: table = v22_frames; break;
        case 3: table = v23_frames; break;
        case 4: table = v24_frames; break;
        default: return -EINVAL; /* no need to report error here */
    }

    for (i = 0; table[i].id != NULL; i++)
    {
        if (!memcmp(table[i].id, frame->id, idlen))
        {
            if (table[i].get_data)
                return table[i].get_data(frame, buf, size);
            else
                return -ENOSYS;
        }
    }

    return -EINVAL;
}

int update_id3v2_tag_text_frame_payload(struct id3v2_frame *frame,
                                        char frame_enc_byte,
                                        char *data, size_t size)
{
    size_t frame_size = size + 1;
    char *frame_data = malloc(frame_size);

    if (!frame_data)
        return -ENOMEM;

    frame_data[0] = frame_enc_byte;
    memcpy(frame_data + 1, data, frame_size - 1);
    free(frame->data);
    frame->size = frame_size;
    frame->data = frame_data;

    return 0;
}

int update_id3v2_tag_text_frame(struct id3v2_tag *tag, const char *frame_id,
                                char frame_enc_byte, char *data, size_t size)
{
    struct id3v2_frame *frame = peek_frame(&tag->frame_head, frame_id);
    int ret = 0;

    if (frame)
    {
        ret = update_id3v2_tag_text_frame_payload(
                  frame, frame_enc_byte, data, size);
    }
    else
    {
        frame = calloc(1, sizeof(struct id3v2_frame));

        if (!frame)
            return -ENOMEM;

        ret = update_id3v2_tag_text_frame_payload(
                  frame, frame_enc_byte, data, size);

        if (ret == 0)
        {
            strncpy(frame->id, frame_id, ID3V2_FRAME_ID_MAX_SIZE);
            append_frame(&tag->frame_head, frame);
        }
        else
            free_frame(frame);
    }

    return ret;
}

int set_id3v2_tag_genre(struct id3v2_tag *tag, uint8_t genre_id,
                        wchar_t *genre_wcs)
{
    const struct alias *al = get_alias('g');
    const char *frame_id;
    wchar_t    *wdata;
    int         wsize;
    char       *frame_data;
    size_t      frame_size;
    char        frame_enc_byte;
    const char *frame_enc_name;
    int         ret;

    assert(al);
    frame_id = alias_to_frame_id(al, tag->header.version); 

    if (genre_id != ID3V1_UNKNOWN_GENRE)
    {
        if (!genre_wcs)
            genre_wcs = L"";

        switch (tag->header.version)
        {
            case 4:
                wsize = swprintf_alloc(&wdata, L"%u%lc%ls",
                                       genre_id, L'\0', genre_wcs);
                
                if (wsize > 0 && genre_wcs[0] == L'\0')
                    wsize--; /* no need to have the separator */
                break;

            default:
            case 2:
            case 3:
                wsize = swprintf_alloc(&wdata, L"(%u)%ls", genre_id, genre_wcs);
        }

        if (wsize < 0)
            return wsize;
    }
    else if (!IS_EMPTY_WCS(genre_wcs))
    {
        wdata = genre_wcs;
        wsize = wcslen(genre_wcs);
    }
    else
        return -EINVAL;

    frame_enc_byte = g_config.v2_def_encs[tag->header.version];
    frame_enc_name = get_id3v2_tag_encoding_name(tag->header.version,
                                                 frame_enc_byte);

    assert(frame_enc_name);

    ret = iconv_alloc(frame_enc_name, WCHAR_CODESET,
                      (char *)wdata, wsize*sizeof(wchar_t),
                      &frame_data, &frame_size);

    if (wdata != genre_wcs)
        free(wdata);

    if (ret != 0)
        return ret;

    ret = update_id3v2_tag_text_frame(tag, frame_id, frame_enc_byte,
                                      frame_data, frame_size);

    free(frame_data);

    return ret;
}

static int get_text_frame_data_by_alias(const struct id3v2_tag *tag,
                                        char alias,
                                        wchar_t **data, size_t *datasize)
{
    const char *frame_id;
    struct id3v2_frame *frame;
    const char *frame_enc_name;
    const struct alias *al = get_alias(alias);
    int ret;

    assert(al);
    frame_id = alias_to_frame_id(al, tag->header.version);
    frame = peek_frame(&tag->frame_head, frame_id);

    if (!frame)
        return -ENOENT;

    if (frame->size <= 1)
        return -EILSEQ;

    frame_enc_name =
        get_id3v2_tag_encoding_name(tag->header.version, frame->data[0]);

    if (!frame_enc_name)
        return -EILSEQ;

    ret = iconv_alloc(WCHAR_CODESET, frame_enc_name,
                      frame->data + 1, frame->size - 1,
                      (void *)data, datasize);

    if (datasize)
        *datasize /= sizeof(wchar_t);

    return ret;
}

int get_id3v2_tag_trackno(const struct id3v2_tag *tag)
{
    wchar_t *wdata;
    long trackno;
    int ret;

    ret = get_text_frame_data_by_alias(tag, 'n', &wdata, NULL);

    if (ret != 0)
        return ret;

    errno = 0;
    trackno = wcstol(wdata, NULL, 10);
    free(wdata);

    if (errno == 0 && trackno >= 0 && trackno <= 255)
        return (int)trackno;

    return -EILSEQ;
}

int get_id3v2_tag_genre(const struct id3v2_tag *tag, wchar_t **genre_wcs)
{
    wchar_t *ptr;
    wchar_t *wdata;
    size_t wsize;
    int genre_id = ID3V1_UNKNOWN_GENRE;
    long long_genre_id = -1;
    int ret;

    ret = get_text_frame_data_by_alias(tag, 'g', &wdata, &wsize);

    if (ret != 0)
        return ret;

    ptr = wdata;

    switch (tag->header.version)
    {
        case 4:
            errno = 0;
            long_genre_id = wcstol(wdata, &ptr, 10);
            if (errno != 0)
                long_genre_id = -1;
            break;

        case 3:
        case 2:
            if (wdata[0] == L'(')
            {
                errno = 0;
                long_genre_id = wcstol(wdata + 1, &ptr, 10);
                if (errno != 0)
                    long_genre_id = -1;

                if (*ptr != L')')
                {
                    ptr = wdata;
                    long_genre_id = -1;
                }
                else
                    ptr++;
            }
            break;
    }

    if (long_genre_id >= 0 && long_genre_id <= 255)
        genre_id = long_genre_id;

    if (ptr < wdata + wsize && ptr[1] != L'\0' && genre_wcs)
    {
        *genre_wcs = wcsdup(ptr);
        if (!genre_wcs)
        {
            free(wdata);
            return -ENOMEM;
        }
    }
    else
        *genre_wcs = NULL;

    free(wdata);

    return genre_id;
}
