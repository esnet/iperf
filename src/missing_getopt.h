#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define no_argument        0
#define required_argument  1
#define optional_argument  2
#endif

typedef struct option
{
   const char *name;
   int         has_arg;
   int        *flag;
   int         val;
} option;

int
netbsd_getopt_long(int nargc, char * const *nargv, const char *options,
    const struct option *long_options, int *idx);

extern char *optarg;
extern int optind, opterr, optopt;

