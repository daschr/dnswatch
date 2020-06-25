#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <resolv.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#define S_BUFLEN 1024
#define S_NAMESERVER "1.1.1.1"


res_state state_p=NULL;

void sig_handler(int ec){
	if(state_p!=NULL)
		res_nclose(state_p);
	exit(0);
}

char *get_ip(char *, char *, size_t, int);
void setup_resolver(res_state, struct sockaddr_in *, char *);

int main(int ac, char *as[]){
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	struct __res_state state;
	memset(&state, 0, sizeof(struct __res_state));

	struct sockaddr_in addr;
	
	setup_resolver(&state, &addr, S_NAMESERVER);
	
	char ip[256];
	if(get_ip(as[1], ip, 256, 0)!=NULL)
		printf("%s\n", ip);
	
	res_nclose(&state);
	return EXIT_SUCCESS;
}


void setup_resolver(res_state sp, struct sockaddr_in *addr, char *nameserver){
	state_p=sp;
	res_ninit(sp);
	
	addr->sin_family=AF_INET;
	addr->sin_port=htons(53);
	inet_aton(nameserver,  (struct in_addr *) &addr->sin_addr.s_addr);

	state_p->nscount=1;
	state_p->nsaddr_list[0]=*addr;
}

char *get_ip(char *fqdn, char *dispbuf, size_t buflen, int ipv6){
	ns_msg msg;
    ns_rr rr;
	u_char abuf[S_BUFLEN];	
	int len;

	if((len=res_nquery(state_p, fqdn, C_IN, T_A, abuf, sizeof(abuf)))<0)
		return NULL;

	ns_initparse(abuf, len, &msg);
	len=ns_msg_count(msg, ns_s_an);

	for(int i=0;i<len;++i){
		ns_parserr(&msg, ns_s_an, i, &rr);
		switch(ns_rr_type(rr)){
			case ns_t_a:
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
