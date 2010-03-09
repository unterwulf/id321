#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include "params.h"
#include "output.h"
#include "id3v1.h"
#include "common.h"

int init_config(int *argc, char ***argv)
{
    int      c;
    uint16_t debug_mask = OS_ERROR | OS_WARN;

    static const struct
    {
        char         *act_name;
        id3_action_t  act_id;
    }
    actions[] =
    {
        { "pr", ID3_PRINT  }, { "print",  ID3_PRINT  },
        { "rm", ID3_DELETE }, { "delete", ID3_DELETE },
        { "mo", ID3_MODIFY }, { "modify", ID3_MODIFY },
        { "sy", ID3_SYNC   }, { "sync",   ID3_SYNC   },
        { "cp", ID3_COPY   }, { "copy",   ID3_COPY   }
    };

    init_output(OS_ERROR);
    g_config.fmtstr = NULL;
    g_config.action = ID3_PRINT;
    g_config.ver.major = NOT_SET;
    g_config.ver.minor = NOT_SET;

    g_config.enc_iso8859_1 = "ISO-8859-1";
    g_config.enc_utf8 = "UTF-8";
    g_config.enc_utf16 = "UTF-16";
    g_config.enc_utf16be = "UTF-16BE";

    /* determine action if specified, by default print tags */
    if (*argc > 1 && (*argv)[1][0] != '-')
    {
        unsigned i;

        for_each (i, actions)
        {
            if (strcmp((*argv)[1], actions[i].act_name) == 0)
            {
                g_config.action = actions[i].act_id;
                optind = 2;
                break;
            }
        }
    }

    while ((c = getopt(*argc, *argv, "1::2::e::vf:a:c:g:G:l:n:t:y:")) != -1)
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

            case 'a': g_config.artist = optarg; break;
            case 'c': g_config.comment = optarg; break;
            case 'g': g_config.genre = optarg; break;
            case 'G': g_config.genre = optarg; break;
            case 'l': g_config.album = optarg; break;
            case 'n': g_config.track = optarg; break;
            case 't': g_config.title = optarg; break;
            case 'y': g_config.year = optarg; break;

            case 'e':
                g_config.enc_iso8859_1 = g_config.enc_utf8 =
                    g_config.enc_utf16 = g_config.enc_utf16be =
                    optarg ? optarg : locale_encoding();
                break;

            case 'v':
                debug_mask |= OS_INFO | OS_DEBUG;
                break;

            case 'f':
                g_config.fmtstr = optarg;
                break;
        }
    }

    init_output(debug_mask);
    *argc -= optind;
    *argv += optind;

    return 0;
}
