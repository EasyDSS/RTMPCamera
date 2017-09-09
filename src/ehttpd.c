#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "ehttpd.h"
#include "http_parser.h"
#include "queue.h"

#ifdef __WIN32__
#define FD_SETSIZE 1024
#include <winsock2.h>
#endif

#ifdef __LINUX__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif 

#if 1
#define dbg(fmt, ...) do {printf("[DEBUG %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define info(fmt, ...) do {printf("[INFO  %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define warn(fmt, ...) do {printf("[WARN  %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define err(fmt, ...) do {printf("[ERROR %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#else
#include "Comm.h"
#endif

#ifdef __WIN32__
#define MSG_DONTWAIT 0
#define SK_EAGAIN   (WSAEWOULDBLOCK)
#define SK_EINTR	(WSAEINTR)
typedef int SOCKLEN;
#define snprintf _snprintf
#endif

#ifdef __LINUX__
#define SOCKET_ERROR	(-1)
#define INVALID_SOCKET  (-1)
#define SK_EAGAIN   (EAGAIN)
#define SK_EINTR	(EINTR)
#define closesocket(x)  close(x)
typedef int SOCKET;
typedef socklen_t SOCKLEN;
#endif 

static int sk_errno (void)
{
#ifdef __WIN32__
	return WSAGetLastError();
#endif
#ifdef __LINUX__
	return (errno);
#endif
}

static const char *sk_strerror (int err)
{
#ifdef __WIN32__
	static char serr_code_buf[24];
	sprintf(serr_code_buf, "WSAE-%d", err);
	return serr_code_buf;
#endif
#ifdef __LINUX__
	return strerror(err);
#endif
}

#define EHTTPD_MSG_MAX_SIZ (1024+1024)

struct ehttpd_client_context
{
	SOCKET sockfd;		//http client socket
	struct in_addr peer_addr; //peer ipv4 addr
	unsigned int   peer_port; //peer ipv4 port
	void* user_data;

	char reqbuf[EHTTPD_MSG_MAX_SIZ];
	unsigned int reqlen;

	char http_field[32];
	int http_parsed;

	struct ehttpd_context* httpd;
	TAILQ_ENTRY(ehttpd_client_context) httpd_entry;	
};

struct ehttpd_context
{
	SOCKET sockfd;
	ehttp_msg reqmsg;
	ehttp_msg resmsg;
	ehttpd_callback callback;
	void* user_data;
	TAILQ_HEAD(, ehttpd_client_context) client_qhead;
};

static struct ehttpd_context *_alloc_httpd (void)
{
	struct ehttpd_context *d = (struct ehttpd_context*) calloc(1, sizeof(struct ehttpd_context));
	if (NULL == d) {
		err("alloc memory for ehttpd_context failed");
		return NULL;
	}
	TAILQ_INIT(&d->client_qhead);
	return d;
}

static void _free_httpd (struct ehttpd_context *d)
{
	if (d) {
		free(d);
	}
}

static struct ehttpd_client_context *_alloc_client_context (struct ehttpd_context *d)
{
	struct ehttpd_client_context *c = (struct ehttpd_client_context*) calloc(1, sizeof(struct ehttpd_client_context));
	if (NULL == c) {
		err("alloc memory for ehttpd_client_context failed");
		return NULL;
	}

	c->httpd = d;
	TAILQ_INSERT_TAIL(&d->client_qhead, c, httpd_entry);
	return c;
}

static void _free_client_context (struct ehttpd_client_context *c)
{
	if (c) {
		struct ehttpd_context *d = c->httpd;
		TAILQ_REMOVE(&d->client_qhead, c, httpd_entry);
		free(c);
	}
}

#ifdef __WIN32__
#include <mstcpip.h>
#endif
#ifdef __LINUX__
#include <fcntl.h>
#include <netinet/tcp.h>
#endif

ehttpd_handle ehttpd_create(unsigned int port, const ehttpd_callback* cb, void* pdata)
{
	struct ehttpd_context *d = NULL;
	struct sockaddr_in inaddr;
	SOCKET sockfd;
	int ret;
	
#ifdef __WIN32__
	WSADATA ws;
	WSAStartup(MAKEWORD(2,2), &ws);
#endif

	if (NULL == cb) {
		return NULL;
	}

	d = _alloc_httpd();
	if (NULL == d) {
		return NULL;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == INVALID_SOCKET) {
		err("create socket failed : %s", sk_strerror(sk_errno()));
		_free_httpd(d);
		return NULL;
	}

	if (port <= 0)
		port = 80;

	memset(&inaddr, 0, sizeof(inaddr));
	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inaddr.sin_port = htons(port);
	ret = bind(sockfd, (struct sockaddr*)&inaddr, sizeof(inaddr));
	if (ret == SOCKET_ERROR) {
		err("bind socket to address failed : %s", sk_strerror(sk_errno()));
		closesocket(sockfd);
		_free_httpd(d);
		return NULL;
	}

	ret = listen(sockfd, 32); //XXX
	if (ret == SOCKET_ERROR) {
		err("listen socket failed : %s", sk_strerror(sk_errno()));
		closesocket(sockfd);
		_free_httpd(d);
		return NULL;
	}

	d->sockfd = sockfd;
	d->user_data = pdata;
	memcpy(&d->callback, cb, sizeof(ehttpd_callback));

	info("http server demo starting on port %u", port);
	return (ehttpd_handle)d;
}

static int _ehttpd_set_client_socket (SOCKET sockfd)
{
		int ret;

#ifdef __WIN32__
		unsigned long nonblocked = 1;
		int sndbufsiz = EHTTPD_MSG_MAX_SIZ;
		int keepalive = 1;
		struct tcp_keepalive alive_in, alive_out;
		unsigned long alive_retlen;
		struct linger ling;

		ret = ioctlsocket(sockfd, FIONBIO, &nonblocked);
		if (ret == SOCKET_ERROR) {
			warn("ioctlsocket FIONBIO failed: %s", sk_strerror(sk_errno()));
		}

		ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&sndbufsiz, sizeof(sndbufsiz));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt SO_SNDBUF failed: %s", sk_strerror(sk_errno()));
		}

		ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepalive, sizeof(keepalive));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt setsockopt SO_KEEPALIVE failed: %s", sk_strerror(sk_errno()));
		}

		alive_in.onoff = TRUE;
		alive_in.keepalivetime = 60000;
		alive_in.keepaliveinterval = 30000;
		ret = WSAIoctl(sockfd, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), 
			&alive_out, sizeof(alive_out), &alive_retlen, NULL, NULL);
		if (ret == SOCKET_ERROR) {
			warn("WSAIoctl SIO_KEEPALIVE_VALS failed: %s", sk_strerror(sk_errno()));
		}

		memset(&ling, 0, sizeof(ling));
		ling.l_onoff = 1;
		ling.l_linger = 0;
		ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(ling)); //resolve too many TCP CLOSE_WAIT
		if (ret == SOCKET_ERROR) {
			warn("setsockopt SO_LINGER failed: %s", sk_strerror(sk_errno()));
		}
#endif

#ifdef __LINUX__
		int sndbufsiz = EHTTPD_MSG_MAX_SIZ;
		int keepalive = 1;
		int keepidle = 60;
		int keepinterval = 3;
		int keepcount = 5;
		struct linger ling;

		ret = fcntl(sockfd, F_GETFL, 0);
		if (ret < 0) {
			warn("fcntl F_GETFL failed: %s", strerror(errno));
		} else {
			ret |= O_NONBLOCK;
			ret = fcntl(sockfd, F_SETFL, ret);
			if (ret < 0) {
				warn("fcntl F_SETFL failed: %s", strerror(errno));
			}
		}

		ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&sndbufsiz, sizeof(sndbufsiz));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt SO_SNDBUF failed: %s", sk_strerror(sk_errno()));
		}

		ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepalive, sizeof(keepalive));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt SO_KEEPALIVE failed: %s", sk_strerror(sk_errno()));
		}

		ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (const char*)&keepidle, sizeof(keepidle));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt TCP_KEEPIDLE failed: %s", sk_strerror(sk_errno()));
		}

		ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (const char*)&keepinterval, sizeof(keepinterval));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt TCP_KEEPINTVL failed: %s", sk_strerror(sk_errno()));
		}

		ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (const char*)&keepcount, sizeof(keepcount));
		if (ret == SOCKET_ERROR) {
			warn("setsockopt TCP_KEEPCNT failed: %s", sk_strerror(sk_errno()));
		}

		memset(&ling, 0, sizeof(ling));
		ling.l_onoff = 1;
		ling.l_linger = 0;
		ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(ling)); //resolve too many TCP CLOSE_WAIT
		if (ret == SOCKET_ERROR) {
			warn("setsockopt SO_LINGER failed: %s", sk_strerror(sk_errno()));
		}
#endif
		return 0;
}

static struct ehttpd_client_context *_httpd_new_client_context (struct ehttpd_context *d)
{
	struct ehttpd_client_context *c = NULL;
	struct sockaddr_in inaddr;
	SOCKET sockfd;
	SOCKLEN addrlen = sizeof(inaddr);
	void* new_user_data = NULL;
	int ret;

	sockfd = accept(d->sockfd, (struct sockaddr*)&inaddr, &addrlen);
	if (sockfd == INVALID_SOCKET) {
		err("accept failed : %s", sk_strerror(sk_errno()));
		return NULL;
	}

	_ehttpd_set_client_socket(sockfd);//XXX DEBUG

	if (d->callback.on_client_comming) {
		ret = d->callback.on_client_comming(inet_ntoa(inaddr.sin_addr), ntohs(inaddr.sin_port), 
			d->user_data, &new_user_data);
		if (ret < 0) {
			warn("client be refused [peer %s:%u]", inet_ntoa(inaddr.sin_addr), ntohs(inaddr.sin_port));
			closesocket(sockfd);
			return NULL;
		}
	}

	info("new http client comming [peer %s:%u]", inet_ntoa(inaddr.sin_addr), ntohs(inaddr.sin_port));

	c = _alloc_client_context(d);
	if (c == NULL) {
		closesocket(sockfd);
		return NULL;
	}

	c->sockfd = sockfd;
	c->peer_addr = inaddr.sin_addr;
	c->peer_port = ntohs(inaddr.sin_port);
	c->user_data = new_user_data ? new_user_data : d->user_data;

	return c;
}

static void _ehttpd_del_client_context (struct ehttpd_client_context *c)
{
	if (c) {
		struct ehttpd_context* d = c->httpd;
		info("delete client socket %d [peer %s:%u]", c->sockfd, inet_ntoa(c->peer_addr), c->peer_port);
		if (d->callback.on_client_close) {
			d->callback.on_client_close(c->user_data);
		}
		closesocket(c->sockfd);
		_free_client_context(c);
	}
}

static int _ehttpd_on_url (http_parser* parser, const char *at, size_t length)
{
	struct ehttpd_client_context *c = (struct ehttpd_client_context*) parser->data;
	struct ehttpd_context* d = NULL;
	ehttp_msg *req = NULL;
	if (c == NULL || c->httpd == NULL) {
		return -1;
	}
	d = c->httpd;
	req = &d->reqmsg;
//	dbg("%.*s", (int)length, at);
	if (length + 1 > sizeof(req->uri)) {
		err("uri too long %.*s", (int)length, at);
		return -1;
	}
	memset(req->uri, 0, sizeof(req->uri));
	memcpy(req->uri, at, length);
	return 0;
}
static int _ehttpd_on_header_field (http_parser* parser, const char *at, size_t length)
{
	struct ehttpd_client_context *c = (struct ehttpd_client_context*) parser->data;
//	struct ehttpd_context* d = NULL;
//	ehttp_msg *req = NULL;
	if (c == NULL || c->httpd == NULL) {
		return -1;
	}
//	d = c->httpd;
//	req = &d->reqmsg;
//	dbg("%.*s", (int)length, at);
	memset(c->http_field, 0, sizeof(c->http_field));
	if (length < sizeof(c->http_field)) {
		memcpy(c->http_field, at, length);
	}
	else {
		warn("header field is too long, ignore %.*s", (int)length, at);
	}
	return 0;
}
static int _ehttpd_on_header_value (http_parser* parser, const char *at, size_t length)
{
	struct ehttpd_client_context *c = (struct ehttpd_client_context*) parser->data;
	struct ehttpd_context* d = NULL;
	ehttp_msg *req = NULL;
	if (c == NULL || c->httpd == NULL) {
		return -1;
	}
	d = c->httpd;
	req = &d->reqmsg;
//	dbg("%.*s", (int)length, at);
	if (strcmp(c->http_field, "Host") == 0) {
		memset(req->h_host, 0, sizeof(req->h_host));
		if (length + 1 > sizeof(req->h_host)) {
			length = sizeof(req->h_host) - 1;
		}
		memcpy(req->h_host, at, length);
	}
	else if (strcmp(c->http_field, "Accept") == 0) {
		memset(req->h_accept, 0, sizeof(req->h_accept));
		if (length + 1 > sizeof(req->h_accept)) {
			length = sizeof(req->h_accept) - 1;
		}
		memcpy(req->h_accept, at, length);
	}
	else if (strcmp(c->http_field, "User-Agent") == 0) {
		memset(req->h_user_agent, 0, sizeof(req->h_user_agent));
		if (length + 1 > sizeof(req->h_user_agent)) {
			length = sizeof(req->h_user_agent) - 1;
		}
		memcpy(req->h_user_agent, at, length);
	}
	else if (strcmp(c->http_field, "Date") == 0) {
		memset(req->h_date, 0, sizeof(req->h_date));
		if (length + 1 > sizeof(req->h_date)) {
			length = sizeof(req->h_date) - 1;
		}
		memcpy(req->h_date, at, length);
	}
	else if (strcmp(c->http_field, "Content-Type") == 0) {
		memset(req->h_content_type, 0, sizeof(req->h_content_type));
		if (length + 1 > sizeof(req->h_content_type)) {
			length = sizeof(req->h_content_type) - 1;
		}
		memcpy(req->h_content_type, at, length);
	}
	 return 0;
}
static int _ehttpd_on_body (http_parser* parser, const char *at, size_t length)
{
	struct ehttpd_client_context *c = (struct ehttpd_client_context*) parser->data;
	struct ehttpd_context* d = NULL;
	ehttp_msg *req = NULL;
	if (c == NULL || c->httpd == NULL) {
		return -1;
	}
	d = c->httpd;
	req = &d->reqmsg;
//	dbg("%.*s", (int)length, at);
	if (length > sizeof(req->content_buff))
		length = sizeof(req->content_buff);
	memcpy(req->content_buff, at, length);
	req->content_length = length;
	return 0;
}
static int _ehttpd_on_message_complete (http_parser* parser)
{
	struct ehttpd_client_context *c = (struct ehttpd_client_context*) parser->data;
//	struct ehttpd_context* d = NULL;
//	ehttp_msg *req = NULL;
	if (c == NULL || c->httpd == NULL) {
		return -1;
	}
//	d = c->httpd;
//	req = &d->reqmsg;
//	dbg("");
	c->http_parsed = 1;
	return 0;
}

//ret -1:应断开 0:消息不完整 1:收到消息 消息存放在c->httpd->reqmsg
static int _ehttpd_recv_msg (struct ehttpd_client_context *c)
{
	struct ehttpd_context* d = c->httpd;
	ehttp_msg* reqmsg = &d->reqmsg;
	struct http_parser_settings settings;
	struct http_parser parser;
	int len, ret;
	
	if (c->reqlen < sizeof(c->reqbuf)) {
		ret = recv(c->sockfd, c->reqbuf + c->reqlen, sizeof(c->reqbuf) - c->reqlen - 1, MSG_DONTWAIT);
		if (ret == 0) {
			dbg("peer closed [peer %s:%u]", inet_ntoa(c->peer_addr), c->peer_port);
			return -1;
		}
		if (ret == SOCKET_ERROR) {
			if (sk_errno() != SK_EAGAIN && sk_errno() != SK_EINTR) {
				err("recv data failed: %s", sk_strerror(sk_errno()));
				return -1;
			}
			ret = 0;
		}
		c->reqlen += ret;
	}

	if (c->reqlen == 0) {
		return 0;
	}

	http_parser_init(&parser, HTTP_REQUEST);
	parser.data = c;
	http_parser_settings_init(&settings);
	settings.on_url = _ehttpd_on_url;
	settings.on_header_field = _ehttpd_on_header_field;
	settings.on_header_value = _ehttpd_on_header_value;
	settings.on_body = _ehttpd_on_body;
	settings.on_message_complete = _ehttpd_on_message_complete;
	c->http_parsed = 0;
	memset(reqmsg, 0, sizeof(*reqmsg));

	len = http_parser_execute(&parser, &settings, c->reqbuf, c->reqlen);
	if (parser.http_errno) {
		err("http_error=%u (%s)", parser.http_errno, http_errno_description((enum http_errno)parser.http_errno));
		return -1;
	}
	if (0 == c->http_parsed) {
		if (c->reqlen >= sizeof(c->reqbuf)) {
			err("peer msg too long, close it [peer %s:%u]", inet_ntoa(c->peer_addr), c->peer_port);
			return -1;
		}
		return 0;
	}

	dbg("len=%d\n%.*s", len, len - reqmsg->content_length, c->reqbuf);

	strncpy(reqmsg->method, http_method_str((enum http_method)parser.method), sizeof(reqmsg->method));
	reqmsg->version = parser.http_minor;

	memmove(c->reqbuf, c->reqbuf + len, c->reqlen - len);
	c->reqlen -= len;
	return 1;
}

#define ARRAY_SIZE(_arr) (sizeof(_arr)/sizeof(_arr[0]))

typedef struct
{
	int intval;
	const char *strval;
} _ehttpd_int2str_tbl_s;

static const char *_ehttpd_msg_int2str (const _ehttpd_int2str_tbl_s *tbl, int num, int intval)
{
	int i;
	for (i = 0; i < num; i++) {
		if (intval == tbl[i].intval)
			return tbl[i].strval;
	}
	return tbl[num-1].strval;
}

static const _ehttpd_int2str_tbl_s _ehttpd_status_code_tbl[] = {
	{ 100, "Continue" },
	{ 101, "Switching Protocols" },
	{ 200, "OK" },
	{ 201, "Created" },
	{ 202, "Accepted" },
	{ 203, "Non-Authoritative Information" },
	{ 204, "No Content" },
	{ 205, "Reset Content" },
	{ 206, "Partial Content" },
	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Found" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 307, "Temporary Redirect" },
	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Time-out" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request-URI Too Large" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Requested range not satisfiable" },
	{ 417, "Expectation Failed" },
	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Time-out" },
	{ 505, "HTTP Version not supported" },
};

static const char* _ehttpd_status_desc(unsigned int status_code)
{
	return _ehttpd_msg_int2str(_ehttpd_status_code_tbl, ARRAY_SIZE(_ehttpd_status_code_tbl), status_code);
}

//ret -1:应断开 0:发送完成 消息存放在c->httpd->resmsg
static int _ehttpd_send_msg (struct ehttpd_client_context *c)
{
	struct ehttpd_context* d = c->httpd;
	ehttp_msg* resmsg = &d->resmsg;
	char buff[EHTTPD_MSG_MAX_SIZ] = "", *ptr;
	int len, ret;

	ptr = buff;
	ptr += snprintf(ptr, sizeof(buff)-(buff-ptr)-1, "HTTP/1.%u %u %s\r\n", 
		!!resmsg->version, resmsg->status_code, _ehttpd_status_desc(resmsg->status_code));
	if (resmsg->h_server[0])
		ptr += snprintf(ptr, sizeof(buff)-(buff-ptr)-1, "Server: %s\r\n", resmsg->h_server);
	if (resmsg->h_date[0])
		ptr += snprintf(ptr, sizeof(buff)-(buff-ptr)-1, "Date: %s\r\n", resmsg->h_date);
	if (resmsg->h_content_type[0])
		ptr += snprintf(ptr, sizeof(buff)-(buff-ptr)-1, "Content-Type: %s\r\n", resmsg->h_content_type);
	if (resmsg->content_length > 0)
		ptr += snprintf(ptr, sizeof(buff)-(buff-ptr)-1, "Content-Length: %u\r\n", resmsg->content_length);
	ptr += snprintf(ptr, sizeof(buff)-(buff-ptr)-1, "\r\n");

	len = ptr - buff;
	dbg("len=%d\n%.*s", len + resmsg->content_length, len, buff);

	ret = send(c->sockfd, buff, len, 0);
	if (ret != len) {
		err("Send Header, close it [peer %s:%u]", inet_ntoa(c->peer_addr), c->peer_port);
		return -1;
	}

	len = resmsg->content_length;
	if (len > 0) {
		ret = send(c->sockfd, (char*)resmsg->content_buff, len, 0);
		if (ret != len) {
			err("Send Content, close it [peer %s:%u]", inet_ntoa(c->peer_addr), c->peer_port);
			return -1;
		}
	}
	return 0;
}

int ehttpd_do_event(ehttpd_handle handle)
{
	struct ehttpd_context *d = (struct ehttpd_context*)handle;
	struct ehttpd_client_context *c = NULL;
	struct timeval tv;
	fd_set rfds;
	SOCKET maxfd;
	int ret;

	if (NULL == d) {
		return -1;
	}

	FD_ZERO(&rfds);

	FD_SET(d->sockfd, &rfds);
	maxfd = d->sockfd;

	TAILQ_FOREACH(c, &d->client_qhead, httpd_entry) {
		FD_SET(c->sockfd, &rfds);
		if (c->sockfd > maxfd)
			maxfd = c->sockfd;
	}

	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);
	if (ret < 0) {
		err("select failed : %s", strerror(errno));
		return -1;
	}
	if (ret == 0) {
		return 0;
	}

	if (FD_ISSET(d->sockfd, &rfds)) {
		//new client_connection
		_httpd_new_client_context(d);
	}

	c = TAILQ_FIRST(&d->client_qhead); //NOTE do not use TAILQ_FOREACH
	while (c) {
		struct ehttpd_client_context *c1 = c;
		c = TAILQ_NEXT(c, httpd_entry);

		if (FD_ISSET(c1->sockfd, &rfds)) {
			do {
				//TODO process request
				ret = _ehttpd_recv_msg(c1);
				if (ret < 0) {
					_ehttpd_del_client_context(c1);
					c1 = NULL;
					break;
				}
				if (ret == 0) {
					break;
				}

				if (d->callback.on_client_request) {
					memset(&d->resmsg, 0, sizeof(ehttp_msg));
					ret = d->callback.on_client_request(&d->reqmsg, &d->resmsg, c1->user_data);
					if (ret < 0) {
						_ehttpd_del_client_context(c1);
						break;
					}
					if (ret > 0) {
						ret = _ehttpd_send_msg(c1);
						if (ret < 0) {
							_ehttpd_del_client_context(c1);
							break;
						}
					}
				}
			} while (c1);
		}
	}

	return 1;
}

void ehttpd_release(ehttpd_handle handle)
{
	struct ehttpd_context *d = (struct ehttpd_context*)handle;
	if (d) {
		struct ehttpd_client_context *c;
		while ((c = TAILQ_FIRST(&d->client_qhead))) {
			_ehttpd_del_client_context(c);
		}
		closesocket(d->sockfd);
		_free_httpd(d);
	}
}
