#include <string.h> /* strlen(), strncmp() */
#include "opts.h"
#include "output.h"
#include "params.h"

int opt_ind = 1;
char *opt_arg;

static int prev_opt_ind = -1;
static unsigned prev_opt_offset = 0;

static int parse_option(int argc, char **argv,
                        const struct opt *opt, int is_long)
{
    if (opt->has_arg != OPT_NO_ARG)
    {
        if (!is_long && prev_opt_offset)
        {
            opt_arg = argv[opt_ind] + prev_opt_offset;
        }
        else if (opt_ind + 1 < argc
                 && (is_long || (!is_long && opt->has_arg == OPT_REQ_ARG)))
        {
            opt_arg = argv[opt_ind + 1];
            opt_ind++;
        }
        else if (opt->has_arg != OPT_OPT_ARG)
        {
            char shortoptstr[] = { (char)opt->shortopt, '\0' };

            print(OS_ERROR, "option '%s%s' requires an argument",
                            is_long ? "--" : "",
                            is_long ? opt->longopt : shortoptstr);
            return '?';
        }

        prev_opt_offset = 0;
    }

    if (!prev_opt_offset)
        opt_ind++;

    return opt->shortopt;
}

int get_opt(int argc, char **argv, const struct opt *optlist)
{
    char *opt;
    int is_long = 0;

    if (opt_ind >= argc)
        return -1;
    else if (opt_ind == prev_opt_ind)
        opt = argv[opt_ind] + prev_opt_offset;
    else if (argv[opt_ind][0] == '-')
    {
        prev_opt_offset = 0;
        opt = argv[opt_ind] + 1; /* skip the dash */
    }
    else
        return -1;

    opt_arg = NULL;

    if (!prev_opt_offset && *opt == '-') /* long option */
    {
        size_t optlen;
        const struct opt *opt_match = NULL;

        opt++; /* skip the second dash */

        if (*opt == '\0') /* -- */
        {
            opt_ind++;
            return -1;
        }

        optlen = strlen(opt);

        for (; optlist->shortopt; optlist++)
            if (optlist->longopt && !strncmp(opt, optlist->longopt, optlen)
                && optlist->actions & g_config.action)
            {
                if (!opt_match)
                    opt_match = optlist;
                else
                {
                    print(OS_ERROR, "option '--%s' is ambiguous", opt);
                    return '?';
                }
            }

        if (opt_match)
            return parse_option(argc, argv, opt_match, 1);

        is_long = 1;
    }
    else /* short option */
    {
        for (; optlist->shortopt; optlist++)
            if (*opt == (char)optlist->shortopt
                && optlist->actions & g_config.action)
            {
                if (opt[1] == '\0')
                    prev_opt_offset = 0;
                else
                {
                    prev_opt_offset = opt - argv[opt_ind] + 1;
                    prev_opt_ind = opt_ind;
                }

                return parse_option(argc, argv, optlist, 0);
            }
    }

    {
        char shortoptstr[] = { *opt, '\0' };

        print(OS_ERROR, "invalid option '%s%s'",
                        is_long ? "--" : "",
                        is_long ? opt : shortoptstr);
    }

    return '?';
}
