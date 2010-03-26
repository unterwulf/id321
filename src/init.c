#include <inttypes.h>
#include <stdlib.h>       /* atoi() */
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <iconv.h>
#include "params.h"
#include "output.h"
#include "id3v1.h"
#include "id3v1_genres.h"
#include "id3v1e_speed.h"
#include "common.h"

#define OPT_SPEED      1
#define OPT_START_TIME 2
#define OPT_END_TIME   3
#define OPT_NO_UNSYNC  4

/*
 * Function:     setup_encodings
 *
 * Description:  Setups encodings in g_config from a colon-separated list of
 *               encodings passed in enc_str and checks if they are supported
 *               by iconv.
 *
 * Return value: On success, 0 is returned. On error, -1 is returned.
 *
 * Notes:        This function reports about errors itself.
 */

static int setup_encodings(char *enc_str)
{
    unsigned i;
    char *cur = NULL;
    char *pos = enc_str;
    char *new_pos;
    iconv_t cd;
    static char const **enc[] =
    {
        &g_config.enc_v1,
        &g_config.enc_iso8859_1,
        &g_config.enc_ucs2,
        &g_config.enc_utf16,
        &g_config.enc_utf16be,
        &g_config.enc_utf8,
    };

    for_each (i, enc)
    {
        new_pos = strchr(pos, ':');

        if (!new_pos)
        {
            if (strlen(pos) != 0)
                (*enc)[i] = pos;
            else if (cur && strlen(cur) != 0)
            {
                (*enc)[i] = cur;
                continue;
            }
            break;
        }
        else
        {
            *new_pos = '\0';
            if (strlen(pos) != 0)
                (*enc)[i] = pos;
            cur = pos;
            pos = new_pos + 1;
        }
    }

    for_each (i, enc)
    {
        print(OS_DEBUG, "%u: %s", i, (*enc)[i]);
        cd = iconv_open((*enc)[i], (*enc)[i]);
        if (cd == (iconv_t)-1)
        {
            print(OS_ERROR, "codeset `%s' is not supported by your iconv",
                  (*enc)[i]);
            return -1;
        }
        else
            iconv_close(cd);
    }

    return 0;
}

int init_config(int *argc, char ***argv)
{
    int       c;
    long      long_val;
    int       ret;
    uint16_t  debug_mask = OS_ERROR | OS_WARN;
    char     *enc_str = NULL;

    static char v2_def_encs[] =
    {
        [2] = ID3V22_STR_UCS2,
        [3] = ID3V23_STR_UCS2,
        [4] = ID3V24_STR_UTF8,
    };

    static const struct option long_opts[] =
    {
        { "title",      1, 0, 't' },
        { "artist",     1, 0, 'a' },
        { "album",      1, 0, 'l' },
        { "year",       1, 0, 'y' },
        { "comment",    1, 0, 'c' },
        { "genre",      1, 0, 'g' },
        { "size",       1, 0, 's' },
        { "no-unsync",  0, 0, OPT_NO_UNSYNC },
        { "speed",      1, 0, OPT_SPEED },
        { "start-time", 1, 0, OPT_START_TIME },
        { "end-time",   1, 0, OPT_END_TIME },
        { 0, 0, 0, 0 }
    };

    static const struct
    {
        char            *act_name;
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
                optind = 2;
                break;
            }
        }
    }

    while ((c = getopt_long(*argc, *argv,
                       "1::2::e::vEf:F:a:c:g:l:n:t:y:s:",
                       long_opts, NULL)) != -1)
    {
        switch (c)
        {
            case '1':
                g_config.ver.major = 1;
                if (optarg != 0)
                {
                    if (strlen(optarg) == 1)
                    {
                        switch (optarg[0])
                        {
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                                g_config.ver.minor = atoi(optarg);
                                break;

                            case 'e':
                                g_config.ver.minor = ID3V1E_MINOR;
                                break;
                        }
                    }

                    if (g_config.ver.minor == NOT_SET)
                    {
                        print(OS_ERROR, "unknown minor version of id3v1: %s",
                              optarg);
                        return -1;
                    }
                }
                break;

            case '2':
                g_config.ver.major = 2;
                if (optarg != 0)
                {
                    if (strlen(optarg) == 1)
                    {
                        switch (optarg[0])
                        {
                            case '2':
                            case '3':
                            case '4':
                                g_config.ver.minor = atoi(optarg);
                                break;
                        }
                    }

                    if (g_config.ver.minor == NOT_SET)
                    {
                        print(OS_ERROR, "unknown minor version of id3v2: %s",
                              optarg);
                        return -1;
                    }
                }
                break;

            case 's':                
                ret = str_to_long(optarg, &long_val);
                if (ret == 0 && long_val >= 0)
                {
                    g_config.size = long_val;
                    g_config.options |= ID321_OPT_CHANGE_SIZE;
                }
                else
                {
                    print(OS_ERROR, "invalid tag size value specified");
                    return -1;
                }
                break;

            case 'g':
            {
                /* genre shall be in format id3v1_genre_id[:genre_str]
                 * where id3v1_genre_id may be specified by name */
                char *sep = strchr(optarg, ':');

                if (sep)
                {
                    *sep = '\0';
                    g_config.genre_str = sep + 1;
                }

                if (sep == optarg)
                    break;

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
                    {
                        print(OS_ERROR, "invalid genre specified");
                        return -1;
                    }

                    g_config.options |= ID321_OPT_SET_GENRE_ID;
                }
                break;
            }

            case 'a': g_config.artist = optarg; break;
            case 'c': g_config.comment = optarg; break;
            case 'l': g_config.album = optarg; break;
            case 'n': g_config.track = optarg; break;
            case 't': g_config.title = optarg; break;
            case 'y': g_config.year = optarg; break;
            case 'F': g_config.frame = optarg; break;

            case 'e':
                if (!optarg)
                    g_config.enc_v1 = locale_encoding();
                else
                    enc_str = optarg;
                break;

            case 'v': debug_mask |= OS_INFO | OS_DEBUG; break;
            case 'f': g_config.fmtstr = optarg; break;
            case 'E': g_config.options |= ID321_OPT_EXPERT; break;
            case OPT_NO_UNSYNC: g_config.options |= ID321_OPT_NO_UNSYNC; break;

            case OPT_SPEED:
                g_config.speed = get_id3v1e_speed_id(optarg);
                if (g_config.speed == 0)
                {
                    ret = str_to_long(optarg, &long_val);
                    if (ret == 0 && long_val == (long_val & 0xFF))
                        g_config.speed = (uint8_t)long_val;
                    else
                    {
                        print(OS_ERROR, "invalid speed value");
                        return -1;
                    }
                }
                g_config.options |= ID321_OPT_SET_SPEED;
                break;
        }
    }

    /* no debug output before this line is possible (i.e. only errors) */
    init_output(debug_mask);

    ret = setup_encodings(enc_str ? enc_str : "");
    if (ret != 0)
        return -1;
    
    if (!(g_config.options & ID321_OPT_EXPERT))
    {
        if (g_config.action == ID3_DELETE
            && (g_config.ver.minor == 0 || g_config.ver.minor == 1
                || g_config.ver.minor == 3))
        {
            print(OS_ERROR, "removing of namely ID3v1.%u tag may lead to "
                  "a garbage at the end of the file in case there is an "
                  "ID3v1.2 or ID3v1 enhanced tag; if you are sure use -E "
                  "to do so",
                  g_config.ver.minor);
            return -1;
        }

        if ((g_config.options & ID321_OPT_SET_SPEED)
            && !(is_valid_id3v1e_speed_id(g_config.speed)))
        {
            print(OS_ERROR, "non standard speed value `%u' specified; "
                  "if you are sure what are you doing use -E to force this",
                  g_config.speed);
            return -1;
        }

        if ((g_config.options & ID321_OPT_SET_GENRE_ID)
            && g_config.genre_id > ID3V1_GENRE_ID_MAX)
        {
            print(OS_ERROR, "non standard genre id `%u' specified; "
                  "if you are sure what are you doing use -E to force this",
                  g_config.genre_id);
            return -1;
        }

    }

    *argc -= optind;
    *argv += optind;

    return 0;
}
