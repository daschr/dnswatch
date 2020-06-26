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

#ifdef USE_DEPRECATED
typedef struct __res_state * res_state;
#endif
res_state state_p=NULL;

void sig_handler(int ec){
#ifndef USE_DEPRECATED
	if(state_p!=NULL)
		res_nclose(state_p);
#else
	res_close();
#endif
	exit(0);
}

int parse_args(char **, int, char **, int *, char **, char **);
char *get_ip(char *, char *, size_t, int);
int setup_resolver(res_state, struct sockaddr_in *, char *);
void loop(char **, char *, int);

int main(int ac, char *as[]){
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

    char *cmd[ac];
    for(int i=0;i<ac;++i) cmd[i]=NULL;

    int stime=S_STIME;
    char *nameserver=S_NAMESERVER;
    char *fqdn=NULL;
    if(!parse_args(as, ac, cmd, &stime, &nameserver, &fqdn))
		return EXIT_FAILURE;

	struct __res_state state;
	memset(&state, 0, sizeof(struct __res_state));

	struct sockaddr_in addr;
	
	if(!setup_resolver(&state, &addr, nameserver)){
	    fprintf(stderr, "Error: address of nameserver is not valid!\n");
      	
#ifndef USE_DEPRECATED
		res_nclose(&state);
#else
		res_close();
#endif
        return EXIT_FAILURE;
    }

    loop(cmd, fqdn, stime);
    
#ifndef USE_DEPRECATED
	res_nclose(&state);
#else
	res_close();
#endif
	return EXIT_SUCCESS;
}


int setup_resolver(res_state sp, struct sockaddr_in *addr, char *nameserver){
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


char *get_ip(char *fqdn, char *dispbuf, size_t buflen, int ipv6){
	ns_msg msg;
    ns_rr rr;
	u_char abuf[S_BUFLEN];	
	int len;

#ifndef USE_DEPRECATED
	if((len=res_nquery(state_p, fqdn, C_IN, T_A, abuf, sizeof(abuf)))<0)
#else
	if((len=res_query(fqdn, C_IN, T_A, abuf, sizeof(abuf)))<0)
#endif
		return NULL;

	ns_initparse(abuf, len, &msg);
	len=ns_msg_count(msg, ns_s_an);

	for(int i=0;i<len;++i){
		ns_parserr(&msg, ns_s_an, i, &rr);
		switch(ns_rr_type(rr)){
			case ns_t_a:
				if(ipv6)
                    break;
                if(ns_rr_rdlen(rr)!=NS_INADDRSZ)
					break;
                inet_ntop(AF_INET, ns_rr_rdata(rr), dispbuf, buflen);
				return dispbuf;
				break;
			case ns_t_aaaa:
				if(!ipv6)
					break;
				if(ns_rr_rdlen(rr)!=NS_IN6ADDRSZ)
					break;
                inet_ntop(AF_INET6, ns_rr_rdata(rr), dispbuf, buflen);
				return dispbuf;
				break;
			default:
				break;
		}
	}

	return NULL;
}

void loop(char **cmd, char *fqdn, int stime){
    char o_ip[128], n_ip[128];
    memset(o_ip, 0, sizeof(o_ip));
    memset(n_ip, 0, sizeof(n_ip));

    get_ip(fqdn, o_ip, sizeof(o_ip), 0);

    for(;;){
        if( get_ip(fqdn, n_ip, sizeof(n_ip), 0)!=NULL
            && strcmp(o_ip, n_ip)!=0){
            
            setenv("ADDRESS", n_ip, 1);

            if(fork()==0)
                execvp(cmd[0], cmd);
            
            strncpy(o_ip, n_ip, sizeof(o_ip));
        }
        
        sleep(stime);
    }
    
}

void help(char *pname){
    printf("Usage: %s [@<nameserver>|T<stime>] [--] [fqdn] [command] [args]...\n", pname);
}

int parse_args( char **args, int arglen, char **cmd, int *stime, 
                char **nameserver, char **fqdn){
    #define IS(X,Y) strcmp(X,Y)==0
    if(arglen==1 || IS(args[1], "-h") || IS(args[1], "--help")){
        help(args[0]);
        return 0;
    }

    int i=1;
    for(;i<arglen;++i){
        if(args[i][0]=='@'){
            if(strlen(args[i])>1)
                *nameserver=args[i]+1;
        }else if(args[i][0]=='T'){
            if(strlen(args[i])==1)
                continue;
            if(sscanf(args[i]+1, "%d", stime)!=1){
                fprintf(stderr, "Error: \"%s\" is not a valid number!\n", args[i]+1);
                return 0;
            }
            if(*stime<0)
                *stime*=-1;
            if(*stime==0)
                *stime=S_STIME;
        }else if(IS(args[i],"--")){
            ++i;
            break;
        }else
            break;
    }

    if(i>=arglen-1){
        help(args[0]);
        return 0;
    }

    *fqdn=args[i];

    for(int j=0;j<arglen-i;++j)
       cmd[j]=args[j+i+1];

    return 1;
    #undef IS
}
