#include <errno.h>
#include <inttypes.h>
#include <stdio.h>        /* fread(), puts(), stdin */
#include <stdlib.h>       /* atoi() */
#include <string.h>
#include "common.h"
#include "config.h"
#include "iconv_wrap.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v2.h"
#include "id3v1e_speed.h"
#include "langcodes.h"
#include "opts.h"
#include "output.h"
#include "params.h"
#include "u32_char.h"

#define OPT_SPEED      1
#define OPT_START_TIME 2
#define OPT_END_TIME   3
#define OPT_NO_UNSYNC  4

extern void help();

/***
 * split_colon_separated_list
 *
 * @list - string containing colon-separated list to split
 * @array - array to be filled with list elements
 *
 * Note that this function does not unescape escaped colons, it just keeps
 * them in strings as is. Consider using of unescape_chars().
 *
 * Returns number of @array elements filled.
 */

size_t split_colon_separated_list(char *list, char **array, size_t size)
{
    int escaped;
    size_t origsize = size;

    if (size == 0)
        return 0;

    *array++ = list;
    size--;

    if (size > 0)
        for (; *list; list++)
        {
            for (escaped = 0; *list == '\\'; list++)
                escaped = !escaped;

            if (*list == ':' && !escaped)
            {
                *list = '\0';
                *array++ = list + 1;
                size--;
                if (size == 0)
                    break;
            }
        }

    return origsize - size;
}

/***
 * unescape_chars - unescape special charactes @chars in string @str
 *
 * @str - string to process
 * @chars - string containing list of characters to be unescaped
 * @esc - esc-character
 */

static void unescape_chars(char *str, const char *chars, char esc)
{
    char *write;

    for (write = str; *str; str++, write++)
    {
        if (*str == esc && *(str + 1) && strchr(chars, *(str + 1)))
            str++;

        *write = *str;
    }

    *write = '\0';
}

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
#define ENC_OPT_ARG_CNT 6

    size_t i;
    size_t argc;
    char *argv[ENC_OPT_ARG_CNT] = { };
    id321_iconv_t cd;
    static struct 
    {
        const char  *desc;
        const char **name;
    }
    enc[ENC_OPT_ARG_CNT] =
    {
        { "ID3v1",      &g_config.enc_v1 },
        { "ISO-8859-1", &g_config.enc_iso8859_1 },
        { "UCS-2",      &g_config.enc_ucs2 },
        { "UTF-16",     &g_config.enc_utf16 },
        { "UTF-16BE",   &g_config.enc_utf16be },
        { "UTF-8",      &g_config.enc_utf8 },
    };

    if (enc_str)
    {
        argc = split_colon_separated_list(enc_str, argv, ENC_OPT_ARG_CNT);

        for (i = 0; i < argc; i++)
            unescape_chars(argv[i], ":\\", '\\');

        /* if the last arument is an empty string, propagate the value of
         * the argument before last to all futher arguments */
        if (argc > 1 && argv[argc-1][0] == '\0')
            for (i = argc - 1; i < ENC_OPT_ARG_CNT; i++)
                argv[i] = argv[argc-2];
    }

    print(OS_INFO, "Encoding mapping:");

    for (i = 0; i < ENC_OPT_ARG_CNT; i++)
    {
        if (argv[i])
            *(enc[i].name) = argv[i];

        print(OS_INFO, "   %s=%s", enc[i].desc, *(enc[i].name));
        cd = id321_iconv_open(*(enc[i].name), U32_CHAR_CODESET);
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

/***
 * parse_comment_optarg - parse comment option argument
 *
 * The routine modifies comment-related fields of the g_config structure
 * in accordance with @arg passed.
 *
 * Comment option argument shall be in the format
 *
 *     [[<lang>:]<desc>:]<text>
 *
 * where lang and desc can have special value '*' that means any value.
 * Because of the format, colons and asterisks within lang, desc and
 * test shall be escaped with '\' not to have their special meaning.
 */

static inline int parse_comment_optarg(char *arg)
{
#define COM_OPT_ARG_CNT 3

    size_t argc;
    size_t i;
    char *stack[COM_OPT_ARG_CNT];
    struct
    {
        const char **value;
        unsigned int flag;
    }
    conf[COM_OPT_ARG_CNT] =
    {
        { &g_config.comment,      0 },
        { &g_config.comment_desc, ID321_OPT_ANY_COMM_DESC },
        { &g_config.comment_lang, ID321_OPT_ANY_COMM_LANG }
    };

    /* at first, fill stack with all values available */
    argc = split_colon_separated_list(arg, stack, COM_OPT_ARG_CNT);

    /* then, propagate the collected values to the proper fields */
    for (i = 0; argc > 0; argc--, i++)
    {
        if (!strcmp(stack[argc-1], "*") && conf[i].flag)
            g_config.options |= conf[i].flag;
        else
        {
            unescape_chars(stack[argc-1], ":*\\", '\\');
            *conf[i].value = stack[argc-1];
        }
    }

    return (g_config.comment_lang
            && strlen(g_config.comment_lang) != ID3V2_LANG_HDR_SIZE)
           ? -EFAULT : 0;
}

/***
 * parse_frame_optarg - parse frame option argument
 *
 * The routine modifies arbitrary frame-related fields of the g_config
 * structure in accordance with @arg passed.
 *
 * Frame option argument shall be in the format
 *
 *     <frame_id>['['<frame_number>']'][[:{<enc>|bin}]:<value>]
 *
 * where value can have special value '-' that means stdin. Use '\-' to set
 * literaly single dash.
 */

static inline int parse_frame_optarg(char *arg)
{
#define FRAME_OPT_ARG_CNT 3

    size_t argc;
    size_t i;
    char *argv[FRAME_OPT_ARG_CNT];
    char **curarg = argv;

    /* at first, fill argv with all values available */
    argc = split_colon_separated_list(arg, argv, FRAME_OPT_ARG_CNT);

    for (i = 0; i < argc; i++)
        unescape_chars(argv[i], ":\\", '\\');

    /* then, propagate the collected values to the proper fields */
    g_config.frame_enc = NULL;

    /* parse frame id and frame number */
    {
        char *opb;

        g_config.frame_id = *curarg++;
        opb = strchr(g_config.frame_id, '[');

        if (opb)
        {
            char *index = opb + 1;
            char *clb = strchr(index, ']');
            long frame_no;

            if (!clb || clb[1] != '\0')
                return -EILSEQ;

            *opb = *clb = '\0';
            
            if (index == clb) /* empty brackets */
                g_config.options |= ID321_OPT_CREATE_FRAME;
            else if (!strcmp(index, "*"))
                g_config.options |= ID321_OPT_ALL_FRAMES;
            else if (str_to_long(index, &frame_no) == 0)
                g_config.frame_no = (int) frame_no;
            else
                return -EILSEQ;
        }
        else
        {
            g_config.frame_no = 0;
            g_config.options |= ID321_OPT_CREATE_FRAME_IF_NOT_EXISTS;
        }

        if (strlen(g_config.frame_id) > ID3V2_FRAME_ID_MAX_SIZE)
            return -EILSEQ;
    }

    if (argc == 1 && !(g_config.options & ID321_OPT_CREATE_FRAME))
        g_config.options |= ID321_OPT_RM_FRAME;

    if (argc == 3)
    {
        if (!strcasecmp(*curarg, "bin"))
            g_config.options |= ID321_OPT_BIN_FRAME;
        else
            g_config.frame_enc = *curarg;

        curarg++;
    }

    if (argc > 1)
    {
        if (!strcmp(*curarg, "-"))
        {
            size_t bufsize = BLOCK_SIZE;
            size_t datasize = 0;
            char *buf = malloc(bufsize);
            char *ptr = buf;

            if (!buf)
                return -ENOMEM;

            do {
                datasize += fread(ptr, 1, bufsize - datasize, stdin);

                if (datasize == bufsize && !feof(stdin))
                {
                    char *newbuf = realloc(buf, bufsize*2);
                    if (!newbuf)
                    {
                        free(buf);
                        return -ENOMEM;
                    }
                    ptr = newbuf + bufsize;
                    buf = newbuf;
                    bufsize *= 2;
                }
            } while (!feof(stdin));

            g_config.frame_data = buf;
            g_config.frame_size = datasize;
        }
        else
        {
            g_config.frame_data = (!strcmp(*curarg, "\\-")) ? "-" : *curarg;
            g_config.frame_size = strlen(g_config.frame_data);
        }
    }

    return 0;
}

/***
 * parse_genre_optarg
 *
 * Genre shall be in the format:
 *
 *    <id3v1_genre_id>[:<genre_str>]
 *
 * where id3v1_genre_id may be specified by name.
 */

static inline int parse_genre_optarg(char *arg)
{
#define GENRE_OPT_ARG_CNT 2

    size_t argc;
    char *argv[FRAME_OPT_ARG_CNT];
    int ret;
    long long_val;
    
    if (arg[0] == '\0')
    {
        g_config.options |= ID321_OPT_RM_GENRE_FRAME | ID321_OPT_SET_GENRE_ID;
        g_config.genre_id = ID3V1_UNKNOWN_GENRE;
        g_config.genre_str = "";
        return 0;
    }

    argc = split_colon_separated_list(arg, argv, GENRE_OPT_ARG_CNT);

    if (argc == 2)
        g_config.genre_str = argv[1];

    if (argv[0][0] == '\0')
        return 0; /* genre_id is omitted */

    ret = str_to_long(argv[0], &long_val);
    if (ret == 0 && long_val >= 0 && long_val <= 0xFF)
    {
        g_config.genre_id = long_val;
        g_config.options |= ID321_OPT_SET_GENRE_ID;
    }
    else
    {
        g_config.genre_id = get_id3v1_genre_id(argv[0]);
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

    g_config.enc_v1 = g_config.enc_iso8859_1 = ISO_8859_1_CODESET;
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
                puts("id321 " VERSION);
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
                FATAL(ret != 0, "invalid comment spec specified");
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

    ret = setup_encodings(enc_str);
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

        FATAL(g_config.comment_lang
              && !is_valid_langcode(g_config.comment_lang),
              "non standard language code '%s' specified; "
              "if you are sure what you are doing use -E to force this",
              g_config.comment_lang);
    }

    *argc -= opt_ind;
    *argv += opt_ind;

    return 0;
}
