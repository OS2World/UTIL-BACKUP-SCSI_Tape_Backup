/* getopt.h */

extern int opterr, optind;
extern char *optarg;

int getopt(int argc, char **argv, char *opts);
