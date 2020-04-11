/*
**
**	$Id: nhinet.c,v 1.1 1999/11/16 05:07:03 issei Exp issei $
**
*/

/* Copyright (c) Issei Numata 1994-2000 */
/* JNetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "hack.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#include <setjmp.h>
#include <signal.h>
#else
#include <winsock.h>
#endif

static char	*errstr;
static char	*homeurl;
static char	*proxy = HTTP_PROXY;
static int	proxy_port = HTTP_PROXY_PORT;

void
set_homeurl(char *s)
{
    if(homeurl)
	free(homeurl);

    homeurl = malloc(strlen(s) + 1);
    strcpy(homeurl, s);
}

char *
get_homeurl()
{
    if(homeurl)
	return homeurl;
    else
	return "";
}

void
set_proxy(char *s)
{
#ifdef INET6
    char *l, *r, *p;
#else
    size_t len;
#endif

/*
    if(proxy)
	free(proxy);
    proxy = NULL;
*/

#ifndef WIN32
    if(!strncasecmp(s, "http://", 7)){
	s += 7;
    }
#else
    if(!strnicmp(s, "http://", 7)){
	s += 7;
    }
#endif

#ifdef INET6
    l = s;
    if (*l == '['){
	r = strrchr(l, ']');
	if (r != NULL){
	    *r++ = '\0';
	    l++;
	}
	else
	    r = s;
    }
    else
	r = s;
    p = strrchr(r, ':');
    if (p != NULL)
	*p++ = '\0';
    proxy = malloc(strlen(l)+1);
    if (proxy != NULL)
	strcpy(proxy, l);
    proxy_port = (p != NULL && *p) ? atoi(p) : HTTP_PROXY_PORT;
#else
    len = strlen(s);
    proxy = malloc(len + 1);

    --len;
    while(len > 0 && s[len] >= '0' && s[len] <= '9')
	--len;

    if(len > 0 && s[len] == ':' && s[len + 1] != '\0'){
	s[len] = '\0';
	strcpy(proxy, s);
	proxy_port = atoi(s + (len + 1));
    }
    else{
	strcpy(proxy, s);
	proxy_port = HTTP_PROXY_PORT;
    }
#endif

    if(proxy_port == 0)
	proxy_port = 80;
}

char *
get_proxy()
{
    static char buf[BUFSIZ];

    if(proxy && proxy[0]){
#ifndef WIN32
#ifdef INET6
	snprintf(buf, sizeof(buf), 
		 (strchr(proxy, ':') != NULL ? "[%s]:%d" : "%s:%d"),
		 proxy, proxy_port);
#else
	snprintf(buf, sizeof(buf), "%s:%d", proxy, proxy_port);
#endif
#else
#ifdef INET6
	_snprintf(buf, sizeof(buf),
		  (strchr(proxy, ':') != NULL ? "[%s]:%d" : "%s:%d"),
		  proxy, proxy_port);
#else
	_snprintf(buf, sizeof(buf), "%s:%d", proxy, proxy_port);
#endif
#endif
	buf[sizeof(buf)-1] = '\0';
	return buf;
    }
    else
	return "";
}

char *
get_proxy_host()
{
    return proxy;
}

int
get_proxy_port()
{
    return proxy_port;
}

int
soc_read(int sd, char *buf, size_t sz)
{
#ifndef WIN32
    return read(sd, buf, sz);
#else
    return recv(sd, buf, sz, 0);
#endif
}

int
soc_write(int sd, char *buf, size_t sz)
{
#ifndef WIN32
    write(sd, buf, sz);
#else
    int nleft, nwritten;
	
    nleft = sz;

    while (nleft > 0) {
	nwritten = send(sd, buf, nleft, 0);
	if (nwritten <= 0)
	    return (nwritten);
	nleft -= nwritten;
	buf += nwritten;
    }
#endif
    return sz;
}

int
soc_write_str(int sd, char *buf)
{
    return soc_write(sd, buf, strlen(buf));
}

#ifndef WIN32
static jmp_buf	env;
static void (*sig_int_saved)(int);
static void (*sig_alm_saved)(int);

static void
interrupt_report(int sig)
{
    unlock_file(RECORD);
    longjmp(env, sig);
}
#endif

static int
connect_server(int timeout, const char *host, int port)
{
    int			sd;
#ifdef INET6
    struct addrinfo	hints, *res0, *res;
    int			gai;
    char		servbuf[NI_MAXSERV];
#else
    struct sockaddr_in	to;
    struct hostent	*hp;
#endif

#ifndef WIN32
    struct itimerval	val, val0;
    int			ret;

    val0.it_interval.tv_sec = 0;
    val0.it_interval.tv_usec = 0;
    val0.it_value.tv_sec = 0;
    val0.it_value.tv_usec = 0;

    val.it_interval.tv_sec = 0;
    val.it_interval.tv_usec = 0;
    val.it_value.tv_sec = 10;
    val.it_value.tv_usec = 0;
    
    if((ret = setjmp(env)) != 0){
	signal(SIGINT, sig_int_saved);
	signal(SIGALRM, sig_alm_saved);
	if(ret == SIGALRM)
	    errstr = "error: timeout!\n";
	else
	    errstr = "error: interrupt!\n";
	
	return -1;
    }
    sig_int_saved = signal(SIGINT, interrupt_report);
    sig_alm_saved = signal(SIGALRM, interrupt_report);
    
    setitimer(ITIMER_REAL, &val, NULL);
#endif

#ifdef INET6
    snprintf(servbuf, sizeof(servbuf), "%d", port);
    servbuf[sizeof(servbuf)-1] = '\0';

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    gai = getaddrinfo(((proxy && proxy[0]) ? proxy : host), servbuf,
		      &hints, &res0);
    if (gai){
	errstr = gai_strerror(gai);
#ifndef WIN32
	signal(SIGINT, sig_int_saved);
	signal(SIGALRM, sig_alm_saved);
#endif
	return -1;
    }
#else /* INET6 / !INET6 */
    if(proxy && proxy[0]){
	if((hp = gethostbyname(proxy)) == NULL){
	    errstr = "error: bad score server!\n";
	    
#ifndef WIN32
	    signal(SIGINT, sig_int_saved);
	    signal(SIGALRM, sig_alm_saved);
#endif

	    return -1;
	}
    }
    else if((hp = gethostbyname(host)) == NULL){
	errstr = "error: bad score server!\n";
	
#ifndef WIN32
	signal(SIGINT, sig_int_saved);
	signal(SIGALRM, sig_alm_saved);
#endif
	return -1;
    }

    memset(&to, 0, sizeof(to));
    memcpy(&to.sin_addr, hp->h_addr_list[0], hp->h_length);
    
    to.sin_family = AF_INET;
    
    if(proxy && proxy[0] && proxy_port)
	to.sin_port = htons(proxy_port);
    else
	to.sin_port = htons(port);
#endif /* !INET6 */

#ifndef WIN32
    setitimer(ITIMER_REAL, &val0, NULL);

    signal(SIGALRM, sig_alm_saved);
#endif

#ifdef INET6
    sd = -1;
    for (res=res0; res; res=res->ai_next){
	sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifndef WIN32
	if (sd < 0)
	    continue;
#else
	if (sd == INVALID_SOCKET)
	    continue;
#endif
	if (connect(sd, res->ai_addr, res->ai_addrlen) < 0){
#ifndef WIN32
	    close (sd);
#else
	    closesocket (sd);
#endif
	    sd = -1;
	    continue;
	}
    }
    freeaddrinfo(res0);
    if (sd < 0){
	errstr = "error: socket() / connect(): cannot connect score server";
	return -1;
    }
#else /* INET6 / !INET6 */
#ifndef WIN32
    if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
	errstr = "error: cannot create socket!\n";
	return -1;
    }
#else
    if((sd = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
	errstr = "error: cannot create socket!\n";
	return -1;
    }
#endif

    if(connect(sd, (struct sockaddr *)&to, sizeof(to)) < 0){
	errstr = "error: cannot connect score server!\n";
#ifndef WIN32
	close(sd);
#else
	closesocket(sd);
#endif
	return -1;
    }
#endif /* !INET6 */

#ifndef WIN32
    signal(SIGINT, sig_int_saved);
#endif

    return sd;
}

int
connect_scoreserver()
{
    return connect_server(HTTP_TIMEOUT, SCORE_SERVER, SCORE_PORT);
}

int
connect_bonesserver()
{
    return connect_server(HTTP_TIMEOUT, BONES_SERVER, BONES_PORT);
}

int
disconnect_server(int sd)
{
#ifndef WIN32
    return close(sd);
#else
    return closesocket(sd);
#endif
}

char *
soc_err()
{
    return errstr;
}
