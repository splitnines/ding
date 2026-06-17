#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "../include/statistics.h"
#include "../include/twriter.h"

#define PROGNAME "ding"
#define VERSION "v0.0.1"
#define AUTHOR proper_name("Rickey Ingrassia")
#define DNS_PORT "53"


volatile sig_atomic_t got_sigint = 0;

void handle_sigint(int sig) {
    got_sigint = sig;
}


void proper_name(char *name) {
    printf("%s", name);
}

void version() {
    printf("DNS Ping (%s) version: %s\n", PROGNAME, VERSION);
    fputs("Written by: ", stdout);
    AUTHOR;
    puts("");
}


typedef struct{
    const char *opt;
    const char *desc;
} OptionsHelp;

void usage() {
    static const OptionsHelp options[] = {
        { "-n, --name <hostname>",      "host name to lookup (default: google.com)" },
        { "-c, --count <count>",        "stop after <count> replies" },
        { "-t, --timeout <timeout>",    "time, in seconds to wait for a response" },
        { "-i, --interval <interval>",  "time, in seconds between sending each packet" },
        { "-q, --quiet",                "quiet output" },
        { "-s, --save [path/to/file]",  "save summary to file" },
        { "-h, --help",                 "print help and exit" },
        { "-V, --version",              "print version and exit" },
    };

    size_t usecs = 10000;
    twriter("usage: ding <dns server> [options]\n\n", usecs);
    twriter("Options:\n", usecs);

    for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  %-28s %s\n", options[i].opt, options[i].desc);
        twriter(msg, usecs);
    }
}


typedef struct {
    int     quiet;
    char    *target;
    char    *name;
    char    *save;
    size_t  count;
    double  timeout;
    double  interval;
} Args;

static struct option const long_options[] = {
    {"save",       required_argument, NULL, 's'},
    {"name",       required_argument, NULL, 'n'},
    {"count",      required_argument, NULL, 'c'},
    {"timeout",    required_argument, NULL, 't'},
    {"interval",   required_argument, NULL, 'i'},
    {"quiet",      no_argument,       NULL, 'q'},
    {"help",       no_argument,       NULL, 'h'},
    {"version",    no_argument,       NULL, 'V'},
    {NULL,         0,                 NULL,  0 },
};

int check_args_opts(const char *s, const char *arg) {
    if (s && s[0] == '-') {
        fprintf(stderr, "%s: option requires an argument -- %s\n", PROGNAME, arg);
        return -1;
    }
    return 0;
}

int parse_args(int argc, char *argv[], Args *args) {
    int opt;
    char *s_count    = NULL;
    char *s_timeout  = NULL;
    char *s_interval = NULL;


    while((opt = getopt_long(argc, argv, "s:c:n:t:i:qhV", long_options, NULL)) != -1) {
        switch(opt) {
            case 's':
                if (check_args_opts(optarg, "-s, --save") == -1)
                    return -1;
                args->save = optarg;
                break;
            case 'c':
                if (check_args_opts(optarg, "-c, --count") == -1)
                    return -1;
                s_count = optarg;
                break;
            case 'n':
                if (check_args_opts(optarg, "-n, --name") == -1)
                    return -1;
                args->name = optarg;
                break;
            case 't':
                if (check_args_opts(optarg, "-t, --timeout") == -1)
                    return -1;
                s_timeout = optarg;
                break;
            case 'i':
                if (check_args_opts(optarg, "-i, --interval") == -1)
                    return -1;
                s_interval = optarg;
                break;
            case 'q':
                args->quiet = 1;
                break;
            case 'h':
                usage();
                return -1;
            case 'V':
                version();
                return -1;
            case '?':
                return -1;
            default:
                break;
        }
    }

    if (s_count != NULL) {
        args->count = (size_t)strtod(s_count, NULL);
    }

    if (s_timeout != NULL) {
        args->timeout = strtof(s_timeout, NULL);
    }

    if (s_interval != NULL) {
        args->interval = (double)strtof(s_interval, NULL);
    }

    if (optind >= argc) {
        usage();
        return -1;
    }
    args->target = argv[optind];

    return 0;
}


static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}


static int build_dns_query(const char *name, unsigned char *buf, size_t buflen) {
    if (buflen < 12) {
        return -1;
    }

    size_t pos = 0;

    buf[pos++] = 0x12;
    buf[pos++] = 0x34;
    buf[pos++] = 0x01;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;

    const char *start = name;
    const char *dot;

    while (*start) {
        dot = strchr(start, '.');

        size_t len;
        if (dot) {
            len = (size_t)(dot - start);
        } else {
            len = strlen(start);
        }

        if (len == 0 || len > 63) {
            fprintf(stderr, "incorrect length");
            return -1;
        }

        if (pos + 1 + len >= buflen) {
            fprintf(stderr, "buffer exceeded\n");
            return -1;
        }

        buf[pos++] = (unsigned char)len;
        memcpy(&buf[pos], start, len);
        pos += len;

        if (!dot) {
            break;
        }

        start = dot + 1;
    }

    if (pos + 5 > buflen) {
        fprintf(stderr, "buffer exceeded\n");
        return -1;
    }

    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;

    return (int)pos;
}


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

int log_resp(const LogResp *logresp) {
    if (logresp->log != NULL) {
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "%.3f,%.3f,%.3f,%.3f,%zu,%zu,%s,%s,%s\n",
                logresp->min, logresp->avg_rtt, logresp->max, logresp->mdev,
                logresp->sent, logresp->recvd, logresp->target, logresp->name,
                logresp->timestamp);

        size_t wr = fwrite(log_msg, 1, strlen(log_msg), logresp->log);
        if (wr != strlen(log_msg)) {
            if (ferror(logresp->log))
                fprintf(stderr, "%s: fwrite failed\n", PROGNAME);
            else {
                fprintf(stderr, "%s: short write without stream error", PROGNAME);
            }
            return -1;
        }
    } else {
        printf("\n--- %s ding statistics ---\n", logresp->target);
        printf("%zd packets transmitted, %zd received, %.2f%% packet loss, time %.0fms\n",
                logresp->sent, logresp->recvd,
                ((logresp->sent - logresp->recvd) / (double)logresp->sent) * 100,
                logresp->sum_ms);
        printf("rtt min/avg/max/mdev %.3f/%.3f/%.3f/%.3f ms\n",
               logresp->min, logresp->avg_rtt, logresp->max, logresp->mdev);
    }
    return 0;
}


int get_hostname(struct addrinfo *res, char *hostname){
    int rc;
    struct addrinfo *p = NULL;
    
    for (p = res; p != NULL; p = p->ai_next)
       rc = getnameinfo(p->ai_addr, p->ai_addrlen, hostname,
               NI_MAXHOST, NULL, 0, NI_NAMEREQD);

    return rc;
}

int get_socket_fd(const char *target, char *hostname) {
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    int status = getaddrinfo(target, DNS_PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "ding: %s: %s\n",target, gai_strerror(status));
        return -1;
    }

    if (get_hostname(res, hostname) != 0) {
        if (strlen(target) < sizeof(hostname)) {
            strcpy(hostname, target);
        } else {
            fprintf(stderr, "ding: %s too large\n", target);
            return -1;
        }
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        fprintf(stderr, "%s: %s: %s\n", PROGNAME, "socket()", strerror(errno));
        freeaddrinfo(res);
        return -1;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        fprintf(stderr, "%s: %s: %s\n", PROGNAME, "connect()", strerror(errno));
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);

    return fd;
}


int ding(int argc, char **argv) {

    struct sigaction sa;
    
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    Args args;
    memset(&args, 0, sizeof(args));
    if (parse_args(argc, argv, &args) < 0)
        return 42;

    FILE *f = NULL;
    if (args.save != NULL) {
        f = fopen(args.save, "a");
        if (f == NULL) {
            fprintf(stderr, "%s: %s: %s", PROGNAME, args.save, strerror(errno));
            return 42;
        }
    }

    const char *target = args.target;
    const char *name = (args.name != NULL) ? args.name : "google.com";
    const size_t query_count = (args.count != 0) ? args.count : 0;
    const double timeout = (args.timeout != 0) ? args.timeout : 1.0;
    const double interval = (args.interval != 0) ? args.interval : 1.0;

    char hostname[NI_MAXHOST];
    int fd;
    if ((fd = get_socket_fd(target, hostname)) == -1)
        return 42;

    unsigned char query[512];
    int query_len = build_dns_query(name, query, sizeof(query));
    if (query_len < 0) {
        fprintf(stderr, "ding: failed to build DNS query for name: %s\n", name);
        close(fd);
        if (f != NULL)
            fclose(f);
        return 42;
    }

    if (args.save == NULL)
        printf("DNS server: %s (%s) DNS lookup: %s\n", target, hostname, name);

    double total_ms   = 0.0;
    size_t recv_count = 0;
    size_t xmit_count = 0;
    size_t iter_count = 0;
    size_t sequence   = 0;
    size_t capacity   = 16;

    double *samples = malloc(sizeof(double) * capacity); 
    if (!samples) {
        fprintf(stderr, "%s: %s: %s\n", PROGNAME, "malloc()", strerror(errno));
        if (f != NULL)
            fclose(f);
        return 42;
    }
 
    while (1) {
        sequence++;

        if (got_sigint) {
            got_sigint = 0;
            break;
        }

        long long start = now_ns();

        ssize_t sent = send(fd, query, query_len, 0);
        if (sent < 0) {
            fprintf(stderr, "%s: %s: %s\n", PROGNAME, "send()", strerror(errno));
            if (f != NULL)
                fclose(f);
            return 42;
        }
        xmit_count++;

        unsigned char reply[512];
        ssize_t received = recv(fd, reply, sizeof(reply), 0);
        if (received < 0) {
            if (errno == EINTR) {
                break;
            } else {
                fprintf(stderr, "%s: %s: %s\n", PROGNAME, "recv()", strerror(errno));
                if (f != NULL)
                    fclose(f);
                return 42;
            }
        }

        long long end = now_ns();

        double ms = (double)(end - start) / 1000000.0;

        if ((ms / 1000) > timeout) {
            if (args.save == NULL && args.quiet == 0)
                printf("Request timed out.\n");
            iter_count++;
            if (query_count > 0 && iter_count >= query_count) {
                break;
            }
            usleep(interval * 1000000);
            continue;
        }

        total_ms += ms;

        if (recv_count >= capacity) {
            capacity *= 2;
            double *tmp = realloc(samples, capacity * sizeof(*samples));
            if (!tmp) {
                fprintf(stderr, "%s: %s: %s\n", PROGNAME, "realloc()", strerror(errno));
                free(samples);
                if (f != NULL)
                    fclose(f);
                return 42;
            }
            samples = tmp;
        }
        samples[recv_count++] = ms;

        if (args.save == NULL && args.quiet == 0)
            printf("response from %s: name=%s sequence=%zu time=%.3f ms\n",
                   target, name, sequence, ms);

        iter_count++;
        if (query_count > 0 && iter_count >= query_count)
            break;

        usleep(interval * 1000000);
    }

    close(fd);

    double avg_rtt = 0.0;
    if (total_ms > 0)
        avg_rtt = (total_ms / recv_count);
    else
        avg_rtt = INFINITY;

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", &tm);

    const LogResp logresp = {
        .log       = f,
        .min       = min(samples, recv_count),
        .avg_rtt   = avg_rtt,
        .max       = max(samples, recv_count),
        .mdev      = mdev(samples, recv_count),
        .sum_ms    = sum_total(samples, recv_count),
        .sent      = xmit_count,
        .recvd     = recv_count, 
        .target    = target,
        .name      = name,
        .timestamp = timestamp,
        .save      = args.save,
        .quiet     = args.quiet
    };

    int n;
    n = log_resp(&logresp);
    if (n < 0) {
        fprintf(stderr, "%s: failed to log results\n", PROGNAME);
        free(samples);
        if (f != NULL)
            fclose(f);
        return 42;
    }

    free(samples);

    if (f != NULL)
        fclose(f);

    return 0;
}
