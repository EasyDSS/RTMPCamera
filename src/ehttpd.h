#ifndef __EHTTPD_H__
#define __EHTTPD_H__

#ifdef __cplusplus
extern "C" {
#endif

//http message
typedef struct
{
	char method[8];		//request only
	char uri[128];		//request only
	unsigned int status_code;	//responsse only
	unsigned int version;		//0-HTTP/1.0 1-HTTP/1.1

	char h_host[64];		//req
	char h_accept[64];		//req
	char h_user_agent[64];	//req
	char h_server[64];		//res
	char h_date[64];		//req/res
	char h_content_type[64];//req/res

	unsigned int  content_length;//0(no content field)
	unsigned char content_buff[1024];
} ehttp_msg;

//callback
typedef struct
{
	//ret 0:accept -1:disconnect
	int (*on_client_comming) (const char* ipaddr, unsigned int port, void* pdata, void** ppdata_new);
	//ret 0:not-response 1:response -1:disconnect
	int (*on_client_request) (const ehttp_msg* reqmsg, ehttp_msg* resmsg, void* pdata);
	void (*on_client_close) (void* pdata);
} ehttpd_callback;

typedef void* ehttpd_handle;

//ret handle or NULL
ehttpd_handle ehttpd_create(unsigned int port, const ehttpd_callback* cb, void* pdata);

//ret 0:idle 1:something-handled -1:error
int ehttpd_do_event(ehttpd_handle handle);

void ehttpd_release(ehttpd_handle handle);

#ifdef __cplusplus
}
#endif
#endif