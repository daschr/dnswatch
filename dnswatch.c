#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <resolv.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#define S_BUFLEN 1024
#define S_STIME 60
#define S_NAMESERVER "1.1.1.1"
#define MAX_RECORDS 24
#define RECORD_SIZE 128

#ifdef USE_DEPRECATED
typedef struct __res_state * res_state;
#endif
res_state state_p=NULL;

char **o_records, **n_records;

void sig_handler(int ec) {
#ifndef USE_DEPRECATED
    if(state_p!=NULL)
        res_nclose(state_p);
#endif

    for(int i=0; i<MAX_RECORDS; ++i) {
        free(n_records[i]);
        free(o_records[i]);
    }

    free(n_records);
    free(o_records);

    exit(0);
}

int parse_args(char **, int, char **, int *, int *, const char **, const char **);
char *get_ip(const char *, char *, size_t, int);
int setup_resolver(res_state, struct sockaddr_in *, const char *);
void loop(char * const *, const char *, int, int);

int main(int ac, char *as[]) {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGCHLD, SIG_IGN);

    char *cmd[ac];
    for(int i=0; i<ac; ++i) cmd[i]=NULL;

    int stime=S_STIME;
    const char *nameserver=S_NAMESERVER;
    const char *fqdn=NULL;
    int aaaa=0;
    if(!parse_args(as, ac, cmd, &stime, &aaaa, &nameserver, &fqdn))
        return EXIT_FAILURE;

#ifndef USE_DEPRECATED
    struct __res_state state;
    memset(&state, 0, sizeof(struct __res_state));
#endif

    struct sockaddr_in addr;

#ifndef USE_DEPRECATED
    if(!setup_resolver(&state, &addr, nameserver)) {
#else
    if(!setup_resolver(&_res, &addr, nameserver)) {
#endif
        fprintf(stderr, "Error: address of nameserver is not valid!\n");

#ifndef USE_DEPRECATED
        res_nclose(&state);
#endif
        return EXIT_FAILURE;
    }

    loop(cmd, fqdn, stime, aaaa);

#ifndef USE_DEPRECATED
    res_nclose(&state);
#endif
    return EXIT_SUCCESS;
}


int setup_resolver(res_state sp, struct sockaddr_in *addr, const char *nameserver) {
    state_p=sp;

#ifndef USE_DEPRECATED
    res_ninit(sp);
#else
    res_init();
#endif

    addr->sin_family=AF_INET;
    addr->sin_port=htons(53);
    int r=inet_aton(nameserver,  (struct in_addr *) &addr->sin_addr.s_addr);

    state_p->nscount=1;
    state_p->nsaddr_list[0]=*addr;
    return r;
}


int get_records(const char *fqdn, char **dispbuf, size_t element_maxsize, size_t maxnum_elems, int aaaa) {
    ns_msg msg;
    ns_rr rr;
    unsigned char abuf[S_BUFLEN];
    int len;

    int addr_type=aaaa?T_AAAA:T_A;
    int record_pos=0;

    for(int i=0; i<maxnum_elems; ++i)
        dispbuf[i][0]='\0';


#ifndef USE_DEPRECATED
    if((len=res_nquery(state_p, fqdn, C_IN, addr_type, abuf, sizeof(abuf)))<0)
#else
    if((len=res_query(fqdn, C_IN, addr_type, abuf, sizeof(abuf)))<0)
#endif
        return 0;

    ns_initparse(abuf, len, &msg);
    len=ns_msg_count(msg, ns_s_an);

    for(int i=0; i<len; ++i) {
        if(record_pos == maxnum_elems)
            break;

        ns_parserr(&msg, ns_s_an, i, &rr);
        switch(ns_rr_type(rr)) {
        case ns_t_a:
            if(ns_rr_rdlen(rr)!=NS_INADDRSZ)
                break;
            inet_ntop(AF_INET, ns_rr_rdata(rr), dispbuf[record_pos++], element_maxsize);
            break;
        case ns_t_aaaa:
            if(ns_rr_rdlen(rr)!=NS_IN6ADDRSZ)
                break;
            inet_ntop(AF_INET6, ns_rr_rdata(rr), dispbuf[record_pos++], element_maxsize);
            break;
        default:
            break;
        }
    }

    return record_pos;
}

int cmp_records(char **l_records, char **r_records, size_t num_records) {
    for(int i=num_records-1; i>=0; --i)
        if(strcmp(l_records[i], r_records[i]) != 0)
            return 1;
    return 0;
}

int cmp_rectuple(const void *v1, const void *v2) {
    return strcmp(*((const char **) v1), *((const char **) v2));
}

void loop(char * const *cmd, const char *fqdn, int stime, int aaaa) {
    char **temp_records;
    n_records=malloc(sizeof(char *)*MAX_RECORDS);
    o_records=malloc(sizeof(char *)*MAX_RECORDS);

    for(int i=0; i<MAX_RECORDS; ++i) {
        n_records[i]=malloc(RECORD_SIZE);
        o_records[i]=malloc(RECORD_SIZE);
    }

    int totnum_records=get_records(fqdn, o_records, RECORD_SIZE, MAX_RECORDS, aaaa);
    qsort(o_records, MAX_RECORDS, sizeof(char *), cmp_rectuple);

    for(;;) {
        totnum_records=get_records(fqdn, n_records, RECORD_SIZE, MAX_RECORDS, aaaa);
        qsort(n_records, MAX_RECORDS, sizeof(char *), cmp_rectuple);

        if(cmp_records(o_records, n_records, MAX_RECORDS)) {

            char *addresses=malloc(totnum_records*RECORD_SIZE+totnum_records);
            memset(addresses, 0, totnum_records*RECORD_SIZE+totnum_records);
            for(int i=0; i<totnum_records; ++i) {
                strcat(addresses, n_records[MAX_RECORDS-1-i]);
                if(i != totnum_records-1)
                    strcat(addresses, ",");
            }
            setenv("ADDRESSES", addresses, 1);
            setenv("ADDRESS", addresses, 1);
            free(addresses);

            if(fork()==0)
                execvp(cmd[0], cmd);

            temp_records=o_records;
            o_records=n_records;
            n_records=temp_records;
        }

        sleep(stime);
    }
}

void help(char *pname) {
    printf("Usage: %s [@<nameserver>|T<stime>|AAAA] [--] [fqdn] [command] [args]...\n", pname);
}

int parse_args( char **args, int arglen, char **cmd, int *stime,
                int *aaaa, const char **nameserver, const char **fqdn) {
#define IS(X,Y) strcmp(X,Y)==0
    if(arglen==1 || IS(args[1], "-h") || IS(args[1], "--help")) {
        help(args[0]);
        return 0;
    }

    int i=1;
    for(; i<arglen; ++i) {
        if(args[i][0]=='@') {
            if(strlen(args[i])>1)
                *nameserver=args[i]+1;
        } else if(args[i][0]=='T') {
            if(strlen(args[i])==1)
                continue;
            if(sscanf(args[i]+1, "%d", stime)!=1) {
                fprintf(stderr, "Error: \"%s\" is not a valid number!\n", args[i]+1);
                return 0;
            }
            if(*stime<0)
                *stime*=-1;
            if(*stime==0)
                *stime=S_STIME;
        } else if(IS(args[i],"AAAA")) {
            *aaaa=1;
        } else if(IS(args[i],"--")) {
            ++i;
            break;
        } else
            break;
    }

    if(i>=arglen-1) {
        help(args[0]);
        return 0;
    }

    *fqdn=args[i];

    for(int j=0; j<arglen-i; ++j)
        cmd[j]=args[j+i+1];

    return 1;
#undef IS
}
