#include <unistd.h>
#include <string.h>
#include "params.h"
#include "output.h"
#include "id3v1.h"

int init_config(int *argc, char ***argv)
{
    int        c;
    uint16_t   debug_mask = OS_ERROR | OS_WARN;
    uint32_t   options = 0;
    uint16_t   size;

    init_output(OS_ERROR);
    g_config.fmtstr = NULL;
    g_config.action = ID3_GET;
    g_config.major_ver = NOT_SET;
    g_config.minor_ver = NOT_SET;

    while ((c = getopt(*argc, *argv, "1::2::dgnpse:vf:")) != -1)
    {
        switch (c)
        {
            case '1':
                g_config.major_ver = 1;
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
                                g_config.minor_ver = atoi(optarg);
                                break;

                            case 'e':
                                g_config.minor_ver = ID3V1E_MINOR;
                                break;
                        }
                    }

                    if (g_config.minor_ver == NOT_SET)
                    {
                        print(OS_ERROR, "unknown minor version of id3v1: %s",
                                optarg);
                        return -1;
                    }
                }
                break;

            case '2':
                g_config.major_ver = 2;
                if (optarg != 0)
                {
                    if (strlen(optarg) == 1)
                    {
                        switch (optarg[0])
                        {
                            case '2':
                            case '3':
                            case '4':
                                g_config.minor_ver = atoi(optarg);
                                break;
                        }
                    }

                    if (g_config.minor_ver == NOT_SET)
                    {
                        print(OS_ERROR, "unknown minor version of id3v2: %s",
                                optarg);
                        return -1;
                    }
                }
                break;

            case 'd':
                g_config.action = ID3_DELETE;
                break;

            case 'g':
                g_config.action = ID3_GET;
                break;

            case 's':
                g_config.action = ID3_SYNC;
                break;

            case 'm':
                g_config.action = ID3_MODIFY;
                break;

            case 'e':
                g_config.options |= ID3T_FORCE_ENCODING;
                g_config.encoding = optarg;
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
