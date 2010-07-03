#include <errno.h>
#include <inttypes.h>
#include <stdio.h>        /* fread(), printf(), stdin */
#include <stdlib.h>       /* atoi() */
#include <string.h>
#include "config.h"
#include "params.h"
#include "opts.h"
#include "output.h"
#include "iconv_wrap.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v1e_speed.h"
#include "common.h"

#define OPT_SPEED      1
#define OPT_START_TIME 2
#define OPT_END_TIME   3
#define OPT_NO_UNSYNC  4

extern void help();

/***
 * setup_encodings
 *
 * @enc_str - string containing a colon-separated list of encodings
 *
 * The routine setups encodings in g_config from a colon-separated list of
 * encodings passed in the string @enc_str and checks if they are supported
 * by iconv.
 *
 * Returns 0 on success, or -1 on error.
 *
 * Notes: This function reports about errors itself.
 */

static inline int setup_encodings(char *enc_str)
{
    unsigned i;
    char *cur = NULL;
    char *pos = enc_str;
    char *new_pos;
    id321_iconv_t cd;
    static struct 
    {
        const char  *desc;
        const char **name;
    }
    enc[] =
    {
        { "ID3v1",      &g_config.enc_v1 },
        { "ISO-8859-1", &g_config.enc_iso8859_1 },
        { "UCS-2",      &g_config.enc_ucs2 },
        { "UTF-16",     &g_config.enc_utf16 },
        { "UTF-16BE",   &g_config.enc_utf16be },
        { "UTF-8",      &g_config.enc_utf8 },
    };

    for_each (i, enc)
    {
        new_pos = strchr(pos, ':');

        if (!new_pos)
        {
            if (strlen(pos) != 0)
                *(enc[i].name) = pos;
            else if (!IS_EMPTY_STR(cur))
            {
                *(enc[i].name) = cur;
                continue;
            }
            break;
        }
        else
        {
            *new_pos = '\0';
            if (strlen(pos) != 0)
                *(enc[i].name) = pos;
            cur = pos;
            pos = new_pos + 1;
        }
    }

    print(OS_INFO, "Encoding mapping:");

    for_each (i, enc)
    {
        print(OS_INFO, "   %s=%s", enc[i].desc, *(enc[i].name));
        cd = id321_iconv_open(*(enc[i].name), *(enc[i].name));
        if (cd == (id321_iconv_t)-1)
        {
            print(OS_ERROR, "codeset '%s' is not supported by your iconv",
                  *(enc[i].name));
            return -1;
        }
        else
            id321_iconv_close(cd);
    }

    return 0;
}

static void unescape_chars(char *str, const char *chars, char esc)
{
    char *read;
    char *write;

    for (read = str, write = str; *read; read++, write++)
    {
        if (read[0] == esc && strchr(chars, read[1]))
            read++;

        *write = *read;
    }

    *write = '\0';
}

/***
 * parse_comment_optarg - parse comment option argument
 *
 * The routine modifies comment-related fields of the g_config structure
 * in accordance with @optarg passed.
 *
 * Comment option argument shall be in the format
 *
 *     [[lang:]desc:]text
 *
 * where lang and desc can have special value '*' that means any value.
 * Because of the format, colons and asterisks within lang, desc and
 * test shall be escaped with '\' not to have their special meaning.
 */

static inline int parse_comment_optarg(char *optarg)
{
    int i, j;
    char *stack[3];
    char *pos;
    struct
    {
        const char **value;
        uint8_t flag;
    }
    conf[] =
    {
        { &g_config.comment,      0 },
        { &g_config.comment_desc, ID321_OPT_ANY_COMM_DESC },
        { &g_config.comment_lang, ID321_OPT_ANY_COMM_LANG }
    };

    pos = stack[0] = optarg;

    /* at first, fill stack with all values available */

    for (i = 0; (i < sizeof(stack) - 1)
                && (pos = memchr(pos, ':', strlen(pos))); i++)
    {
        if (pos - 1 >= stack[i] && pos[-1] == '\\')
        {
            /* skip escaped colon */
            i--;
            pos++;
            continue;
        }

        *pos = '\0';
        pos++;
        stack[i+1] = pos;
    }

    /* then, propagate the collected values to the proper fields */

    for (j = 0; i >= 0; i--, j++)
    {
        if (!strcmp(stack[i], "*"))
            g_config.options |= conf[j].flag;
        else
        {
            unescape_chars(stack[i], ":*\\", '\\');
            *conf[j].value = stack[i];
        }
    }

    return (g_config.comment_lang && strlen(g_config.comment_lang) != 3)
           ? -EFAULT : 0;
}

/***
 * parse_frame_optarg - parse frame option argument
 *
 * The routine modifies arbitrary frame-related fields of the g_config
 * structure in accordance with @optarg passed.
 *
 * Frame option argument shall be in the format
 *
 *     <frame_id>['['<frame_number>']'][[:{<enc>|bin}]:<value>]
 *
 * where value can have special value '-' that means stdin. Use '\-' to set
 * literaly single dash.
 */

static inline int parse_frame_optarg(char *optarg)
{
    int i, j;
    char *stack[3];
    char *pos;

    pos = stack[0] = optarg;

    /* at first, fill stack with all values available */

    for (i = 0; (i < sizeof(stack) - 1)
                && (pos = memchr(pos, ':', strlen(pos))); i++)
    {
        *pos = '\0';
        pos++;
        stack[i+1] = pos; /* push */
    }

    /* then, propagate the collected values to the proper fields */

    j = 0;
    g_config.frame_enc = NULL;

    /* parse frame id and frame number */
    {
        char *opb;

        g_config.frame_id = stack[j++];
        opb = strchr(g_config.frame_id, '[');

        if (opb)
        {
            char *clb = strchr(opb + 1, ']');
            long frame_no;

            if (!clb || clb[1] != '\0')
                return -EILSEQ;

            *opb = *clb = '\0';
            
            if (opb + 1 == clb) /* empty brackets */
                g_config.options |= ID321_OPT_CREATE_FRAME;
            else if (!strcmp(opb + 1, "*"))
                g_config.options |= ID321_OPT_ALL_FRAMES;
            else if (str_to_long(opb + 1, &frame_no) == 0)
                g_config.frame_no = (int) frame_no;
            else
                return -EILSEQ;
        }
        else
        {
            g_config.frame_no = 0;
            g_config.options |= ID321_OPT_CREATE_FRAME_IF_NOT_EXISTS;
        }
    }

    if (i == 0 && !(g_config.options & ID321_OPT_CREATE_FRAME))
        g_config.options |= ID321_OPT_RM_FRAME;

    if (i == 2)
    {
        if (!strcasecmp(stack[j], "bin"))
            g_config.options |= ID321_OPT_BIN_FRAME;
        else
            g_config.frame_enc = stack[j];

        j++;
    }

    if (i > 0)
    {
        if (!strcmp(stack[j], "-"))
        {
            int bufsize = BLOCK_SIZE;
            char *buf = malloc(bufsize);
            char *ptr = buf;
            ssize_t size = 0;

            if (!buf)
                return -ENOMEM;

            do {
                size_t avail = bufsize - (ptr - buf);
                ssize_t bytes = fread(ptr, 1, avail, stdin);
                size += bytes;

                if (bytes == avail && !feof(stdin))
                {
                    char *newbuf;
                    newbuf = realloc(buf, bufsize*2);
                    if (!newbuf)
                    {
                        free(buf);
                        return -ENOMEM;
                    }
                    ptr = newbuf + bufsize;
                    buf = newbuf;
                    bufsize *= 2;
                }
            } while(!feof(stdin));

            g_config.frame_data = buf;
            g_config.frame_size = size;
        }
        else if (!strcmp(stack[j], "\\-"))
        {
            g_config.frame_data = "-";
            g_config.frame_size = 1;
        }
        else
        {
            g_config.frame_data = stack[j];
            g_config.frame_size = strlen(g_config.frame_data);
        }
    }

    return 0;
}

static inline int parse_genre_optarg(char *optarg)
{
    /* genre shall be in the format id3v1_genre_id[:genre_str]
     * where id3v1_genre_id may be specified by name */
    int ret;
    long long_val;
    char *sep;
    
    if (optarg[0] == '\0')
    {
        g_config.options |= ID321_OPT_RM_GENRE_FRAME | ID321_OPT_SET_GENRE_ID;
        g_config.genre_id = ID3V1_UNKNOWN_GENRE;
        g_config.genre_str = "";
        return 0;
    }

    sep = strchr(optarg, ':');

    if (sep)
    {
        *sep = '\0';
        g_config.genre_str = sep + 1;
    }

    if (sep == optarg)
        return 0; /* genre_id is omitted */

    ret = str_to_long(optarg, &long_val);
    if (ret == 0 && long_val >= 0 && long_val <= 0xFF)
    {
        g_config.genre_id = long_val;
        g_config.options |= ID321_OPT_SET_GENRE_ID;
    }
    else
    {
        g_config.genre_id = get_id3v1_genre_id(optarg);
        if (g_config.genre_id == ID3V1_UNKNOWN_GENRE)
            return -EILSEQ;
        g_config.options |= ID321_OPT_SET_GENRE_ID;
    }

    return 0;
}

int init_config(int *argc, char ***argv)
{
    int       c;
    long      long_val;
    int       ret;
    uint16_t  debug_mask = OS_ERROR | OS_WARN;
    char     *enc_str = getenv("ID321_ENCODING");

#define FATAL(cond, ...) \
    { if (cond) { print(OS_ERROR, __VA_ARGS__); return -1; } }
#define ID3_GRP_WRITE ( ID3_MODIFY | ID3_SYNC | ID3_COPY )
#define ID3_GRP_ALL ( ID3_GRP_WRITE | ID3_PRINT | ID3_DELETE )

    static char v2_def_encs[] =
    {
        [2] = ID3V22_STR_UCS2,
        [3] = ID3V23_STR_UCS2,
        [4] = ID3V24_STR_UTF8,
    };

    static const struct opt optlist[] =
    {
        { NULL,         '1',            OPT_OPT_ARG, ID3_GRP_ALL },
        { NULL,         '2',            OPT_OPT_ARG, ID3_GRP_ALL },
        { NULL,         'e',            OPT_OPT_ARG, ID3_GRP_ALL },
        { "fmt",        'f',            OPT_REQ_ARG, ID3_MODIFY | ID3_PRINT },
        { "expert",     'E',            OPT_NO_ARG,  ID3_MODIFY | ID3_DELETE },
        { "frame",      'F',            OPT_REQ_ARG, ID3_MODIFY | ID3_PRINT },
        { "help",       'h',            OPT_NO_ARG,  ID3_GRP_ALL },
        { "verbose",    'v',            OPT_NO_ARG,  ID3_GRP_ALL },
        { "version",    'V',            OPT_NO_ARG,  ID3_GRP_ALL },
        { "title",      't',            OPT_REQ_ARG, ID3_MODIFY },
        { "artist",     'a',            OPT_REQ_ARG, ID3_MODIFY },
        { "album",      'l',            OPT_REQ_ARG, ID3_MODIFY },
        { "year",       'y',            OPT_REQ_ARG, ID3_MODIFY },
        { "comment",    'c',            OPT_REQ_ARG, ID3_MODIFY },
        { "genre",      'g',            OPT_REQ_ARG, ID3_MODIFY },
        { "track",      'n',            OPT_REQ_ARG, ID3_MODIFY },
        { "size",       's',            OPT_REQ_ARG, ID3_GRP_WRITE },
        { "no-unsync",  OPT_NO_UNSYNC,  OPT_NO_ARG,  ID3_GRP_WRITE },
        { "speed",      OPT_SPEED,      OPT_REQ_ARG, ID3_MODIFY },
        { "start-time", OPT_START_TIME, OPT_REQ_ARG, ID3_MODIFY },
        { "end-time",   OPT_END_TIME,   OPT_REQ_ARG, ID3_MODIFY },
        { NULL,         0,              0, 0 }
    };

    static const struct
    {
        const char      *act_name;
        enum id3_action  act_id;
    }
    actions[] =
    {
        { "pr", ID3_PRINT  }, { "print",  ID3_PRINT  },
        { "rm", ID3_DELETE }, { "delete", ID3_DELETE },
        { "mo", ID3_MODIFY }, { "modify", ID3_MODIFY },
        { "sy", ID3_SYNC   }, { "sync",   ID3_SYNC   },
        { "cp", ID3_COPY   }, { "copy",   ID3_COPY   },
    };

    init_output(OS_ERROR);
    g_config.action = ID3_PRINT;
    g_config.ver.major = NOT_SET;
    g_config.ver.minor = NOT_SET;

    g_config.enc_v1 = g_config.enc_iso8859_1 = "ISO-8859-1";
    g_config.enc_ucs2 = "UCS-2";
    g_config.enc_utf16 = "UTF-16";
    g_config.enc_utf16be = "UTF-16BE";
    g_config.enc_utf8 = "UTF-8";

    g_config.v2_def_encs = v2_def_encs;

    /* determine action if specified, by default print tags */
    if (*argc > 1 && (*argv)[1][0] != '-')
    {
        unsigned i;

        for_each (i, actions)
        {
            if (!strcmp((*argv)[1], actions[i].act_name))
            {
                g_config.action = actions[i].act_id;
                opt_ind = 2;
                break;
            }
        }
    }

    while ((c = get_opt(*argc, *argv, optlist)) != -1)
    {
        switch (c)
        {
            case 'h':
                help();
                exit(EXIT_SUCCESS);

            case 'V':
                printf("id321 " VERSION "\n");
                exit(EXIT_SUCCESS);

            case '1':
                g_config.ver.major = 1;
                if (opt_arg != 0)
                {
                    if (strlen(opt_arg) == 1)
                    {
                        switch (opt_arg[0])
                        {
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                                g_config.ver.minor = atoi(opt_arg);
                                break;

                            case 'e':
                                g_config.ver.minor = ID3V1E_MINOR;
                                break;
                        }
                    }
                    FATAL(g_config.ver.minor == NOT_SET,
                          "unknown minor version of ID3v1: %s", opt_arg);
                }
                break;

            case '2':
                g_config.ver.major = 2;
                if (opt_arg != 0)
                {
                    if (strlen(opt_arg) == 1)
                    {
                        switch (opt_arg[0])
                        {
                            case '2':
                            case '3':
                            case '4':
                                g_config.ver.minor = atoi(opt_arg);
                                break;
                        }
                    }
                    FATAL(g_config.ver.minor == NOT_SET,
                          "unknown minor version of ID3v2: %s", opt_arg);
                }
                break;

            case 's':                
                ret = str_to_long(opt_arg, &long_val);
                FATAL(ret != 0 || long_val < 0, "invalid tag size specified");
                g_config.size = long_val;
                g_config.options |= ID321_OPT_CHANGE_SIZE;
                break;

            case 'g':
                ret = parse_genre_optarg(opt_arg);
                FATAL(ret != 0, "invalid genre specified");
                break;

            case 'c':
                ret = parse_comment_optarg(opt_arg);
                FATAL(ret != 0, "invalid comment language specified");
                break;

            case 'a': g_config.artist = opt_arg; break;
            case 'l': g_config.album = opt_arg; break;
            case 'n': g_config.track = opt_arg; break;
            case 't': g_config.title = opt_arg; break;
            case 'y': g_config.year = opt_arg; break;
            case 'F':
                ret = parse_frame_optarg(opt_arg);
                FATAL(ret == -ENOMEM, "out of memory");
                FATAL(ret != 0, "invalid frame spec specified");
                break;

            case 'e':
                if (!opt_arg)
                {
                    g_config.enc_v1 = g_config.enc_iso8859_1 =
                        locale_encoding();
                    enc_str = NULL;
                }
                else
                    enc_str = opt_arg;
                break;

            case 'v': debug_mask = (debug_mask << 1) | 1; break;
            case 'f': g_config.fmtstr = opt_arg; break;
            case 'E': g_config.options |= ID321_OPT_EXPERT; break;
            case OPT_NO_UNSYNC: g_config.options |= ID321_OPT_NO_UNSYNC; break;

            case OPT_SPEED:
                g_config.speed = get_id3v1e_speed_id(opt_arg);
                if (g_config.speed == 0)
                {
                    ret = str_to_long(opt_arg, &long_val);
                    FATAL(ret != 0 || long_val != (long_val & 0xFF),
                          "invalid speed value");
                    g_config.speed = (uint8_t)long_val;
                }
                g_config.options |= ID321_OPT_SET_SPEED;
                break;

            case '?':
                return -1;
        }
    }

    /* no debug output before this line is possible (i.e. only errors) */
    init_output(debug_mask);

    ret = setup_encodings(enc_str ? enc_str : "");
    if (ret != 0)
        return -1;

    FATAL(g_config.action == ID3_SYNC && g_config.ver.major == NOT_SET,
          "target version for synchronisation is not specified");
    
    if (!(g_config.options & ID321_OPT_EXPERT))
    {
        FATAL(g_config.action == ID3_DELETE
              && (g_config.ver.minor == 0 || g_config.ver.minor == 1
                  || g_config.ver.minor == 3),
              "removing of namely ID3v1.%u tag may lead to "
              "a garbage at the end of the file in case there is an "
              "ID3v1.2 or ID3v1 enhanced tag; if you are sure use -E "
              "to do so",
              g_config.ver.minor);

        FATAL((g_config.options & ID321_OPT_SET_SPEED)
              && !(is_valid_id3v1e_speed_id(g_config.speed)),
              "non standard speed value '%u' specified; "
              "if you are sure what you are doing use -E to force this",
              g_config.speed);

        FATAL((g_config.options & ID321_OPT_SET_GENRE_ID)
              && g_config.genre_id > ID3V1_GENRE_ID_MAX
              && g_config.genre_id != ID3V1_UNKNOWN_GENRE,
              "non standard genre id '%u' specified; "
              "if you are sure what you are doing use -E to force this",
              g_config.genre_id);
    }

    *argc -= opt_ind;
    *argv += opt_ind;

    return 0;
}
