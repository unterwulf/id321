#ifndef OPTS_H
#define OPTS_H

enum opt_has_arg {
    OPT_NO_ARG = 0, /* option has no argument */
    OPT_REQ_ARG,    /* option has mandatory argument */
    OPT_OPT_ARG,    /* option has optional argument */
};

struct opt
{
    const char       *longopt;
    int               shortopt;
    enum opt_has_arg  has_arg;
    unsigned          actions;
};

extern int opt_ind;
extern char *opt_arg;

int get_opt(int argc, char **argv, const struct opt *optlist);

#endif /* OPTS_H */
