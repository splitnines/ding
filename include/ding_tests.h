#ifndef DING_TESTS_H
#define DING_TESTS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>

#define PROGNAME "ding"
#define VERSION "v0.0.1"
#define AUTHOR proper_name("Rickey Ingrassia")
#define DNS_PORT "53"

typedef struct {
    int     quiet;
    char    *target;
    char    *name;
    char    *save;
    size_t  count;
    double  timeout;
    double  interval;
} Args;

typedef struct{
    const char *opt;
    const char *desc;
} OptionsHelp;

typedef struct {
    FILE           *log;
    const int      quiet;
    const double   avg_rtt;
    const double   min;
    const double   max;
    const double   mdev;
    const double   sum_ms;
    const size_t   recvd;
    const size_t   sent;
    const char     *target;
    const char     *name;
    const char     *timestamp;
    const char     *save;
} LogResp;


int check_args_opts(const char *, const char *);
int ding(int, char **);
int get_hostname(struct addrinfo *, char *);
int get_socket_fd(const char *, char *);
void handle_sigint(int);
int log_resp(const LogResp *);
double max(double *, size_t);
double mdev(double *, size_t);
double min(double *, size_t);
int parse_args(int, char **, Args *);
void proper_name(char *);
double sum_total(double *, size_t);
void usage();
void version();


#endif
