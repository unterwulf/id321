#define _GNU_SOURCE     /* strnlen() */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "compat.h"
#include "framelist.h"
#include "frm_comm.h"
#include "id3v2.h"
#include "output.h"
#include "params.h"
#include "textframe.h"  /* get_id3v2_tag_encoding_name() */
#include "u32_char.h"

static int get_comm_frame(unsigned minor, const struct id3v2_frame *frame,
                          u32_char *buf, size_t size)
{
    size_t reqsize;
    struct id3v2_frm_comm *comm;
    int ret;

    ret = unpack_id3v2_frm_comm(frame, minor, &comm);

    if (ret != 0)
        return ret;

    /* reserve six extra chars for " []: " and null-terminator */
    reqsize = (comm->desc ? u32_strlen(comm->desc) : 0)
              + (comm->text ? u32_strlen(comm->text) : 0)
              + ID3V2_LANG_HDR_SIZE + 6;

    if (reqsize <= size)
    {
        u32_char lang[ID3V2_LANG_HDR_SIZE] = { };
        u32_char u32_empty_str[] = { U32_CHAR('\0') };
        u32_char u32_space_str[] = { U32_CHAR(' '), U32_CHAR('\0') };

        iconvordie(U32_CHAR_CODESET, ISO_8859_1_CODESET,
                   comm->lang, ID3V2_LANG_HDR_SIZE,
                   (char *)lang, sizeof(lang)*sizeof(lang[0]));

        u32_snprintf(buf, size, "%ls%ls[%.*ls]: %ls",
                    comm->desc ? comm->desc : u32_empty_str,
                    comm->desc && comm->desc[0] ? u32_space_str : u32_empty_str,
                    ID3V2_LANG_HDR_SIZE, lang, comm->text ?
                    comm->text : u32_empty_str);
    }

    free_id3v2_frm_comm(comm);
    return reqsize;
}

static int get_v22_comm_frame(const struct id3v2_frame *frame,
                              u32_char *buf, size_t size)
{
    return get_comm_frame(2, frame, buf, size);
}

static int get_v23_comm_frame(const struct id3v2_frame *frame,
                              u32_char *buf, size_t size)
{
    return get_comm_frame(3, frame, buf, size);
}

static int get_v24_comm_frame(const struct id3v2_frame *frame,
                              u32_char *buf, size_t size)
{
    return get_comm_frame(4, frame, buf, size);
}

static int get_str_frame(unsigned minor, const struct id3v2_frame *frame,
                         u32_char *buf, size_t size)
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

    return iconvordie(U32_CHAR_CODESET, from_enc,
                      frame->data + ID3V2_ENC_HDR_SIZE,
                      frame->size - ID3V2_ENC_HDR_SIZE,
                      (char *)buf, size*sizeof(u32_char))
           / sizeof(u32_char);
}

static int get_v22_str_frame(const struct id3v2_frame *frame,
                             u32_char *buf, size_t size)
{
    return get_str_frame(2, frame, buf, size);
}

static int get_v23_str_frame(const struct id3v2_frame *frame,
                             u32_char *buf, size_t size)
{
    return get_str_frame(3, frame, buf, size);
}

static int get_v24_str_frame(const struct id3v2_frame *frame,
                             u32_char *buf, size_t size)
{
    int u32_size = get_str_frame(4, frame, buf, size);

    /* The ID3v2.4 informal standard says:
     *
     * "All text information frames supports multiple strings, stored as
     * a null separated list, where null is reperesented by the termination
     * code for the charater encoding."
     *
     * To print such lists as plain text we will just replace all termination
     * codes with '/', so they will look just like ID3v2.3 values. */

    if (u32_size > 0)
    {
        size_t i;

        if (u32_size < size)
            size = u32_size;

        for (i = 0; i < size; i++)
            if (buf[i] == U32_CHAR('\0'))
                buf[i] = U32_CHAR('/');
    }

    return u32_size;
}

static int get_url_frame(const struct id3v2_frame *frame,
                         u32_char *buf, size_t size)
{
    size_t slen = strnlen(frame->data, frame->size);

    return iconvordie(U32_CHAR_CODESET, g_config.enc_iso8859_1,
                      frame->data, slen,
                      (char *)buf, size*sizeof(u32_char))
           / sizeof(u32_char);
}

static int get_hex_frame(const struct id3v2_frame *frame,
                         u32_char *buf, size_t size)
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
 * get_frame_data - get frame payload as u32_str
 *
 * @tag - tag which frame belongs to
 * @frame - frame
 * @buf - output buffer
 * @size - output buffer size
 *
 * Returns -EINVAL if the tag has unsupported version,
 *         -ENOSYS if parser for the frame is not implemented,
 *         -EILSEQ on parser error,
 *         or non-negative number of u32_chars which would be written
 *         if @buf was large enough.
 */

int get_frame_data(const struct id3v2_tag *tag, const struct id3v2_frame *frame,
                   u32_char *buf, size_t size)
{
    id3_frame_handler_table_t *table = NULL;
    size_t idlen = tag->header.version == 2 ? 3 : 4;

    switch (tag->header.version)
    {
        case 2: table = v22_frames; break;
        case 3: table = v23_frames; break;
        case 4: table = v24_frames; break;
        default: return -EINVAL; /* no need to report error here */
    }

    for (; table->id != NULL; table++)
    {
        if (!memcmp(table->id, frame->id, idlen))
        {
            if (table->get_data)
                return table->get_data(frame, buf, size);
            else
                return -ENOSYS;
        }
    }

    return -EINVAL;
}
