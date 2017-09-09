#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "RTMPMgr.h"
#include "RTMPPushExt.h"
#include "ehttpd.h"
#include "cJSON.h"
#include "einifile.h"

#ifdef __WIN32__
#include "w32pthreads.h"
#define strdup _strdup
#define snprintf _snprintf
#else
#include <unistd.h>
#include <pthread.h>
#endif

enum ChannelStartState
{
	CHANNELSTART_STAT_NONE,			/* 未启动 */
	CHANNELSTART_STAT_CONFIG_ERROR,	/* 配置错误 */
	CHANNELSTART_STAT_START_FAILED,	/* 启动失败 */
	CHANNELSTART_STAT_SUCCESS,		/* 启动成功 */
};

enum RtmpState
{
	RTMP_STAT_NONE,				/* 未启动 */
    RTMP_STAT_CONNECTING,		/* 连接中 */
    RTMP_STAT_CONNECTED,        /* 连接成功 */
    RTMP_STAT_CONNECT_FAILED,   /* 连接失败 */
    RTMP_STAT_CONNECT_ABORT,    /* 连接异常中断 */
    RTMP_STAT_PUSHING,          /* 推流中 */
    RTMP_STAT_DISCONNECTED,     /* 断开连接 */
    RTMP_STAT_ERROR,			/* 未知错误 */
};

enum RtspSrcState
{
	RTSPSRC_STAT_NONE,				/* 未启动 */
    RTSPSRC_STAT_CONNECTING,		/* 连接中 */
    RTSPSRC_STAT_CONNECTED,			/* 连接成功 */
    RTSPSRC_STAT_CONNECT_FAILED,	/* 连接失败 */
    RTSPSRC_STAT_DISCONNECTED,		/* 断开连接 */
	RTSPSRC_STAT_ERROR,				/* 未知错误 */
};

enum LocalSrcState
{
	LOCALSRC_STAT_NONE,				/* 未启动 */
	LOCALSRC_STAT_SUCCESS,			/* 启动成功 */
	LOCALSRC_STAT_FAILED,			/* 启动失败 */
};

struct PushChannelContext;
struct RTMPMgr_Context;

struct PushChannelContext
{
	unsigned int m_iChannelID;
	pthread_mutex_t m_hLock;
	int m_bEnable;
	int m_bIsRtspSrc;
	char* m_szRtspUrl;
	char* m_szRtmpUrl;
	EASY_MEDIA_INFO_T m_stLocalSrcMediaInfo;
	RTMPPushExt_Handle m_hPushHandle;
	enum ChannelStartState m_eStartStat;
	enum RtmpState m_eRtmpStat;
	enum RtspSrcState m_eRtspSrcStat;
	enum LocalSrcState m_eLocalSrcStat;
	struct RTMPMgr_Context* m_pMgr;
};

struct RTMPMgr_Context
{
	ehttpd_handle m_hHttpSrv;
	eini_handle m_hIni;
	Easy_U32 m_iBaseChannelNum;
	Easy_U32 m_iExtChannelNum;
	struct PushChannelContext* m_pstPushChannelAll;
	
	int m_bCanRun;
	pthread_t m_hThread;
//	pthread_mutex_t m_hThreadLock;

	char* m_szConfigFile;
	RTMPMgr_Callback m_fCallback;
	void* m_pUserData;
};

static int _ChannelClearCfg(struct PushChannelContext* pChannel)
{
	pthread_mutex_lock(&pChannel->m_hLock);
	pChannel->m_bEnable = 0;
	pChannel->m_bIsRtspSrc = 0;
	if (pChannel->m_szRtspUrl) {
		free(pChannel->m_szRtspUrl);
		pChannel->m_szRtspUrl = NULL;
	}
	if (pChannel->m_szRtmpUrl) {
		free(pChannel->m_szRtmpUrl);
		pChannel->m_szRtmpUrl = NULL;
	}
	pthread_mutex_unlock(&pChannel->m_hLock);
	return 0;
}

static int _ChannelLoadCfg(struct PushChannelContext* pChannel)
{
	eini_handle hIni = pChannel->m_pMgr->m_hIni;
	char name[EINI_NAME_BUF_SIZ];
	char value[EINI_VALUE_BUF_SIZ];
	int ret;

	_ChannelClearCfg(pChannel);

	pthread_mutex_lock(&pChannel->m_hLock);

	sprintf(name, "Channel_%u.Enable", pChannel->m_iChannelID);
	ret = eini_ext_get(hIni, name, value);
	if (ret == 0) {
		pChannel->m_bEnable = !strcmp(value, "true");
	}
	
	sprintf(name, "Channel_%u.IsRtspSrc", pChannel->m_iChannelID);
	ret = eini_ext_get(hIni, name, value);
	if (ret == 0) {
		pChannel->m_bIsRtspSrc = !strcmp(value, "true");
	}
	pChannel->m_bIsRtspSrc = !strcmp(value, "true");

	sprintf(name, "Channel_%u.RtspUrl", pChannel->m_iChannelID);
	ret = eini_ext_get(hIni, name, value);
	if (ret == 0) {
		pChannel->m_szRtspUrl = strdup(value);
	}

	sprintf(name, "Channel_%u.RtmpUrl", pChannel->m_iChannelID);
	ret = eini_ext_get(hIni, name, value);
	if (ret == 0) {
		pChannel->m_szRtmpUrl = strdup(value);
	}

	pthread_mutex_unlock(&pChannel->m_hLock);
	return 0;
}

static int _ChannelSaveCfg(struct PushChannelContext* pChannel)
{
	eini_handle hIni = pChannel->m_pMgr->m_hIni;
	char name[EINI_NAME_BUF_SIZ];
	int ret;

	pthread_mutex_lock(&pChannel->m_hLock);

	sprintf(name, "Channel_%u.Enable", pChannel->m_iChannelID);
	if (0 != eini_ext_set(hIni, name, pChannel->m_bEnable ? "true" : "false")) {
		eini_ext_new(hIni, name, pChannel->m_bEnable ? "true" : "false");
	}

	sprintf(name, "Channel_%u.IsRtspSrc", pChannel->m_iChannelID);
	if (0 != eini_ext_set(hIni, name, pChannel->m_bIsRtspSrc ? "true" : "false")) {
		eini_ext_new(hIni, name, pChannel->m_bIsRtspSrc ? "true" : "false");
	}

	if (pChannel->m_szRtspUrl) {
		sprintf(name, "Channel_%u.RtspUrl", pChannel->m_iChannelID);
		if (0 != eini_ext_set(hIni, name, pChannel->m_szRtspUrl)) {
			eini_ext_new(hIni, name, pChannel->m_szRtspUrl);
		}
	}

	if (pChannel->m_szRtmpUrl) {
		sprintf(name, "Channel_%u.RtmpUrl", pChannel->m_iChannelID);
		if (0 != eini_ext_set(hIni, name, pChannel->m_szRtmpUrl)) {
			eini_ext_new(hIni, name, pChannel->m_szRtmpUrl);
		}
	}

	ret = eini_save_file(hIni, pChannel->m_pMgr->m_szConfigFile);
	if (ret < 0) {
		err("eini_save_file %s fail", pChannel->m_pMgr->m_szConfigFile);
	}

	pthread_mutex_unlock(&pChannel->m_hLock);
	return 0;
}

static int _ChannelSetCfg(struct PushChannelContext* pChannel, int bEnable, int bIsRtspSrc, const char* rtspUrl, const char* rtmpUrl)
{
	pthread_mutex_lock(&pChannel->m_hLock);
	pChannel->m_bEnable = !!bEnable;
	pChannel->m_bIsRtspSrc = !!bIsRtspSrc;
	if (rtspUrl) {
		if (pChannel->m_szRtspUrl) {
			free(pChannel->m_szRtspUrl);
		}
		pChannel->m_szRtspUrl = strdup(rtspUrl);
	}
	if (rtmpUrl) {
		if (pChannel->m_szRtmpUrl)
			free(pChannel->m_szRtmpUrl);
		pChannel->m_szRtmpUrl = strdup(rtmpUrl);
	}
	pthread_mutex_unlock(&pChannel->m_hLock);
	return 0;
}

static Easy_I32 _RTMPPushExt_CallBack (RTMPPushExt_Handle handle, EVENT_T _event_type, void* _userPtr)
{
	struct PushChannelContext* pChannel = (struct PushChannelContext*) _userPtr;
	if (!pChannel) {
		err("BUG!!! _userPtr = NULL");
		return -1;
	}
	
//	pthread_mutex_lock(&pChannel->m_hLock);

	switch (_event_type) {
	case EVENT_RTMP_CONNECTING:/* 连接中 */
		dbg("RTMP Connecting..");
		pChannel->m_eRtmpStat = RTMP_STAT_CONNECTING;
		break;
	case EVENT_RTMP_CONNECTED:/* 连接成功 */
		dbg("RTMP Connected");
		pChannel->m_eRtmpStat = RTMP_STAT_CONNECTED;
		break;
	case EVENT_RTMP_CONNECT_FAILED:/* 连接失败 */
		dbg("RTMP Connect fail");
		pChannel->m_eRtmpStat = RTMP_STAT_CONNECT_FAILED;
		break;
	case EVENT_RTMP_CONNECT_ABORT:/* 连接异常中断 */
		dbg("RTMP Connect abort");
		pChannel->m_eRtmpStat = RTMP_STAT_CONNECT_ABORT;
		break;
	case EVENT_RTMP_PUSHING:/* 推流中 */
		dbg("RTMP Pushing..");
		pChannel->m_eRtmpStat = RTMP_STAT_PUSHING;
		break;
	case EVENT_RTMP_DISCONNECTED:/* 断开连接 */
		dbg("RTMP Disconnected");
		pChannel->m_eRtmpStat = RTMP_STAT_DISCONNECTED;
		break;
	case EVENT_RTMP_ERROR:
		dbg("RTMP Error !!!");
		pChannel->m_eRtmpStat = RTMP_STAT_ERROR;
		break;
	case EVENT_RTSPSRC_CONNECTING:/* 连接中 */
		dbg("RTSP Connecting..");
		pChannel->m_eRtspSrcStat = RTSPSRC_STAT_CONNECTING;
		break;
	case EVENT_RTSPSRC_CONNECTED:/* 连接成功 */
		dbg("RTSP Connected");
		pChannel->m_eRtspSrcStat = RTSPSRC_STAT_CONNECTED;
		break;
	case EVENT_RTSPSRC_CONNECT_FAILED:/* 连接失败 */
		dbg("RTSP Connect fail");
		pChannel->m_eRtspSrcStat = RTSPSRC_STAT_CONNECT_FAILED;
		break;
	case EVENT_RTSPSRC_DISCONNECTED:/* 断开连接 */
		dbg("RTSP Disconnected");
		pChannel->m_eRtspSrcStat = RTSPSRC_STAT_DISCONNECTED;
		break;
	default:
		warn("Unknown event %d", _event_type);
		break;
	}

//	pthread_mutex_unlock(&pChannel->m_hLock);
	return 0;
}

static int _split_url(const char* url, char** real_url, char** username, char** passwd)
{
	const char *p0, *p1, *p2;
	if (!url || !real_url || !username || !passwd) {
		return -1;
	}

	//rtsp://admin:123456@127.0.0.1:8554/live/chn0
	p0 = strstr(url, "://");
	if (!p0){
		return -1;
	}

	p0 += 3;
	p1 = strstr(p0, "@");
	p2 = strstr(p0, "/");
	if (p1 && (!p2 || p1 < p2)) {
		p2 = strstr(p0, ":");
		if (p2 && p2 < p1) {
			*username = (char*) calloc(1, p2 - p0 + 1);
			memcpy(*username, p0, p2 - p0);
			*passwd = (char*) calloc(1, p1 - p2);
			memcpy(*passwd, p2 + 1, p1 - p2 - 1);
			*real_url = (char*) calloc(1, strlen(url) - (p1 - p0));
			memcpy(*real_url, url, p0 - url);
			memcpy(*real_url + (p0 - url), p1 + 1, strlen(url) - (p1 - url + 1));
		} 
		else {
			*username = (char*) calloc(1, p1 - p0 + 1);
			memcpy(*username, p0, p1 - p0);
			*passwd = strdup("");
			*real_url = (char*) calloc(1, strlen(url) - (p1 - p0));
			memcpy(*real_url, url, p0 - url);
			memcpy(*real_url + (p0 - url), p1 + 1, strlen(url) - (p1 - url + 1));
		}
	}
	else {
		*username = strdup("");
		*passwd = strdup("");
		*real_url = strdup(url);
	}
	return 0;
}

static int _ChannelStart(struct PushChannelContext* pChannel)
{
	struct RTMPMgr_Context* ctx = pChannel->m_pMgr;
	int ret;

	pthread_mutex_lock(&pChannel->m_hLock);

	if (pChannel->m_hPushHandle) {
		warn("Channel %u has already started", pChannel->m_iChannelID);
		pthread_mutex_unlock(&pChannel->m_hLock);
		return 0;
	}

	memset(&pChannel->m_stLocalSrcMediaInfo, 0, sizeof(EASY_MEDIA_INFO_T));
	pChannel->m_eStartStat = CHANNELSTART_STAT_NONE;
	pChannel->m_eRtmpStat = RTMP_STAT_NONE;
	pChannel->m_eLocalSrcStat = LOCALSRC_STAT_NONE;
	pChannel->m_eRtspSrcStat = RTSPSRC_STAT_NONE;

	if (!pChannel->m_szRtmpUrl || strlen(pChannel->m_szRtmpUrl) == 0) {
		err("Invalid rtmpUrl");
		pChannel->m_eStartStat = CHANNELSTART_STAT_CONFIG_ERROR;
		goto _fail;
	}

	if (pChannel->m_bIsRtspSrc) {
		if (!pChannel->m_szRtspUrl || strlen(pChannel->m_szRtspUrl) == 0) {
			err("Invalid rtspUrl");
			pChannel->m_eStartStat = CHANNELSTART_STAT_CONFIG_ERROR;
			goto _fail;
		}
	}
	else {
		if (pChannel->m_iChannelID >= ctx->m_iBaseChannelNum) {
			err("Extern channel %u not allow local source", pChannel->m_iChannelID);
			pChannel->m_eStartStat = CHANNELSTART_STAT_CONFIG_ERROR;
			goto _fail;
		}

		ret = ctx->m_fCallback.StartStream(pChannel->m_iChannelID, &pChannel->m_stLocalSrcMediaInfo, ctx->m_pUserData);
		if (ret < 0) {
			err("StartStream(%u) fail", pChannel->m_iChannelID);
			pChannel->m_eStartStat = CHANNELSTART_STAT_START_FAILED;
			pChannel->m_eLocalSrcStat = LOCALSRC_STAT_FAILED;
			goto _fail;
		}
		pChannel->m_eLocalSrcStat = LOCALSRC_STAT_SUCCESS;
	}

	pChannel->m_hPushHandle = RTMPPushExt_Create();
	if (!pChannel->m_hPushHandle) {
		err("RTMPPushExt_Create for %u failed", pChannel->m_iChannelID);
		pChannel->m_eStartStat = CHANNELSTART_STAT_START_FAILED;
		goto _fail;
	}

	if (pChannel->m_bIsRtspSrc) {
		char *real_url = NULL, *username = NULL, *passwd = NULL;
		ret = _split_url(pChannel->m_szRtspUrl, &real_url, &username, &passwd);
		if (ret < 0) {
			err("_split_url(%u,%s) fail", pChannel->m_iChannelID, pChannel->m_szRtspUrl);
			pChannel->m_eStartStat = CHANNELSTART_STAT_CONFIG_ERROR;
			goto _fail;
		}

		dbg("url:%s username:%s passwd:%s", real_url, username, passwd);
		ret = RTMPPushExt_InitRTSPSource(pChannel->m_hPushHandle, real_url, username, passwd);
		free(real_url);free(username);free(passwd);
		if (ret < 0) {
			err("RTMPPushExt_InitRTSPSource(%u,%s) fail", pChannel->m_iChannelID, pChannel->m_szRtspUrl);
			pChannel->m_eStartStat = CHANNELSTART_STAT_START_FAILED;
			goto _fail;
		}
	}
	else {
		ret = RTMPPushExt_InitLocalSource(pChannel->m_hPushHandle, &pChannel->m_stLocalSrcMediaInfo);
		if (ret < 0) {
			err("RTMPPushExt_InitLocalSource(%u) fail", pChannel->m_iChannelID);
			pChannel->m_eStartStat = CHANNELSTART_STAT_START_FAILED;
			goto _fail;
		}
	}

	ret = RTMPPushExt_InitCallback(pChannel->m_hPushHandle, _RTMPPushExt_CallBack, pChannel);
	if (ret < 0) {
		err("RTMPPushExt_InitCallback(%u) fail", pChannel->m_iChannelID);
		pChannel->m_eStartStat = CHANNELSTART_STAT_START_FAILED;
		goto _fail;
	}

	ret = RTMPPushExt_Start(pChannel->m_hPushHandle, pChannel->m_szRtmpUrl, 1024);
	if (ret < 0) {
		err("RTMPPushExt_Start(%u,%s) fail", pChannel->m_iChannelID, pChannel->m_szRtmpUrl);
		pChannel->m_eStartStat = CHANNELSTART_STAT_START_FAILED;
		goto _fail;
	}

	pChannel->m_eStartStat = CHANNELSTART_STAT_SUCCESS;
	pthread_mutex_unlock(&pChannel->m_hLock);

	info("Channel %u started", pChannel->m_iChannelID);
	return 0;

_fail:
	if (pChannel->m_eLocalSrcStat == LOCALSRC_STAT_SUCCESS) {
		ctx->m_fCallback.StopStream(pChannel->m_iChannelID, ctx->m_pUserData);
	}
	if (pChannel->m_hPushHandle) {
		RTMPPushExt_Release(pChannel->m_hPushHandle);
		pChannel->m_hPushHandle = NULL;
	}
	pthread_mutex_unlock(&pChannel->m_hLock);
	return -1;
}

static int _ChannelStop(struct PushChannelContext* pChannel)
{
	struct RTMPMgr_Context* ctx = pChannel->m_pMgr;
	if (pChannel->m_hPushHandle) {
		if (pChannel->m_bIsRtspSrc == 0) {
			ctx->m_fCallback.StopStream(pChannel->m_iChannelID, ctx->m_pUserData);
		}
		pthread_mutex_lock(&pChannel->m_hLock);
		RTMPPushExt_Release(pChannel->m_hPushHandle);
		pChannel->m_hPushHandle = NULL;
		pChannel->m_eStartStat = CHANNELSTART_STAT_NONE;
		pChannel->m_eRtmpStat = RTMP_STAT_NONE;
		pChannel->m_eLocalSrcStat = LOCALSRC_STAT_NONE;
		pChannel->m_eRtspSrcStat = RTSPSRC_STAT_NONE;
		pthread_mutex_unlock(&pChannel->m_hLock);
		info("Channel %u stoped", pChannel->m_iChannelID);
	}
	return 0;
}

//ret 0:accept -1:disconnect
static int _http_on_client_comming (const char* ipaddr, unsigned int port, void* pdata, void** ppdata_new)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*) pdata;
	if (!ctx) {
		err("BUG!!! pdata = NULL");
		return -1;
	}
	dbg("ip:%s,%u comming !!!", ipaddr, port);
	return 0;
}

static const char* _GetChannelStatus(struct PushChannelContext* pChannel)
{
	if (pChannel->m_bEnable == 0)
		return "Disabled";
	switch (pChannel->m_eStartStat) {
	case CHANNELSTART_STAT_SUCCESS:
		break;
	case CHANNELSTART_STAT_CONFIG_ERROR:
		return "ConfigErr";
	case CHANNELSTART_STAT_NONE:
	case CHANNELSTART_STAT_START_FAILED:
	default:
		return "StartFailed";
	}
	switch(pChannel->m_eRtmpStat) {
	case RTMP_STAT_CONNECTED:        /* 连接成功 */
	case RTMP_STAT_PUSHING:          /* 推流中 */
		break;
	case RTMP_STAT_CONNECTING:		/* 连接中 */
		return "RtmpConnecting";
	case RTMP_STAT_CONNECT_FAILED:   /* 连接失败 */
	case RTMP_STAT_CONNECT_ABORT:    /* 连接异常中断 */
	case RTMP_STAT_DISCONNECTED:     /* 断开连接 */
	case RTMP_STAT_ERROR:				/* 未知错误 */
	case RTMP_STAT_NONE:				/* 未启动 */
	default:
		return "RtmpError";
	}
	if (pChannel->m_bIsRtspSrc) {
		switch(pChannel->m_eRtspSrcStat) {
		case RTSPSRC_STAT_CONNECTED:		/* 连接成功 */
			break;
		case RTSPSRC_STAT_CONNECTING:		/* 连接中 */
			return "RtspSrcConnecting";
		case RTSPSRC_STAT_NONE:				/* 未启动 */
		case RTSPSRC_STAT_CONNECT_FAILED:	/* 连接失败 */
		case RTSPSRC_STAT_DISCONNECTED:		/* 断开连接 */
		case RTSPSRC_STAT_ERROR:			/* 未知错误 */
		default:
			return "RtspSrcError";
		}
	}
	else {
		switch(pChannel->m_eLocalSrcStat) {
		case LOCALSRC_STAT_SUCCESS:			/* 启动成功 */
			break;
		case LOCALSRC_STAT_NONE:			/* 未启动 */
		case LOCALSRC_STAT_FAILED:			/* 启动失败 */
		default:
			return "LocalSrcError";
		}
	}
	return "Success";
}

static int _http_status_page(struct RTMPMgr_Context* ctx, const ehttp_msg* reqmsg, ehttp_msg* resmsg)
{
	//GET / HTTP/1.1
	//
	//HTTP/1.1 200 OK
	//Content-Type: text/html
	//
	//<html>...</html>
	struct PushChannelContext* pChannel = NULL;
	unsigned int ch;
	//html status
	char* buff = (char*)resmsg->content_buff;
	int   len = sizeof(resmsg->content_buff);
	char* ptr = buff;
	resmsg->version = reqmsg->version;
	resmsg->status_code = 200;
	strcpy(resmsg->h_server, "RTMPMGR");
	strcpy((char*)resmsg->h_content_type, "text/html");
	ptr += snprintf(ptr, len-(ptr-buff)-1, ""
		"<html>"
		"<head><title>RtmpPushStatus</title></head>"
		"<body>"
		"<p>RTMP推流状态</p>"
		);
	for (ch = 0; ch < ctx->m_iBaseChannelNum + ctx->m_iExtChannelNum; ch++) {
		pChannel = &ctx->m_pstPushChannelAll[ch];
		ptr += snprintf(ptr, len-(ptr-buff)-1, "Channel%u<br/>attr:%s status:%s<br/>source:%s<br/>url:%s<br/><br/>",
			ch, ch < ctx->m_iBaseChannelNum ? "base" : "ext", 
			_GetChannelStatus(pChannel),
			pChannel->m_bIsRtspSrc?pChannel->m_szRtspUrl:"Device", 
			pChannel->m_szRtmpUrl);
	}
	ptr += snprintf(ptr, len-(ptr-buff)-1, ""
		"</body>"
		"</html>"
		);
	resmsg->content_length = ptr - buff;
	return 1;
}

static int _http_get_config(struct RTMPMgr_Context* ctx, const ehttp_msg* reqmsg, ehttp_msg* resmsg)
{
	//GET /get_config HTTP/1.1
	//
	//HTTP/1.1 200 OK
	//Content-Type: application/json
	//
	//{"code" : 0, "msg" : "成功", data : {"status" : "Success", "enable" : true, "source" : "device", "rtmp_url" : "rtmp://xxx"}}
	struct PushChannelContext* pChannel = NULL;
	//char jBuf[1024] = "";
	cJSON* jRoot = NULL, *jTmp = NULL;

	if (strcmp(reqmsg->method, "GET")) {
		resmsg->version = reqmsg->version;
		resmsg->status_code = 400;
		strcpy(resmsg->h_server, "RTMPMGR");
		return 1;
	}

	pChannel = &ctx->m_pstPushChannelAll[0];//XXX
	pthread_mutex_lock(&pChannel->m_hLock);

	jRoot = cJSON_CreateObject();
	cJSON_AddItemToObject(jRoot, "code", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(jRoot, "msg", cJSON_CreateString("成功"));

	jTmp = cJSON_CreateObject();
	cJSON_AddStringToObject(jTmp, "status", _GetChannelStatus(pChannel));
	cJSON_AddBoolToObject(jTmp, "enable", pChannel->m_bEnable);
	cJSON_AddStringToObject(jTmp, "source", pChannel->m_bIsRtspSrc?"rtsp":"device");
	cJSON_AddStringToObject(jTmp, "rtsp_url", pChannel->m_szRtspUrl);
	cJSON_AddStringToObject(jTmp, "rtmp_url", pChannel->m_szRtmpUrl);
	cJSON_AddItemToObject(jRoot, "data", jTmp);

	pthread_mutex_unlock(&pChannel->m_hLock);

	cJSON_PrintPreallocated(jRoot, (char*)resmsg->content_buff, sizeof(resmsg->content_buff) - 1, 0);
	resmsg->content_length = strlen((char*)resmsg->content_buff);
	dbg("JSON: \n%s", resmsg->content_buff);

	resmsg->version = reqmsg->version;
	resmsg->status_code = 200;
	strcpy(resmsg->h_server, "RTMPMGR");
	strcpy((char*)resmsg->h_content_type, "application/json");

	cJSON_Delete(jRoot);
	return 1;
}

static int _http_set_config(struct RTMPMgr_Context* ctx, const ehttp_msg* reqmsg, ehttp_msg* resmsg)
{
	//POST /get_config HTTP/1.1
	//Content-Type: application/json
	//
	//{"channel" : 0, "enable" : true, "source" : "rtsp", "rtsp_url" : "rtsp://xxx", "rtmp_url" : "rtmp://xxx"}
	//HTTP/1.1 200 OK
	//Content-Type: application/json
	//
	//{"code" : 0, "msg" : "成功"}
	struct PushChannelContext* pChannel = NULL;
	char jBuf[1024] = "";
	unsigned int ch = 0;
	cJSON* jRoot = NULL;
	cJSON* jChannel = NULL;
	cJSON* jEnable = NULL;
	cJSON* jSource = NULL;
	cJSON* jRtspUrl = NULL;
	cJSON* jRtmpUrl = NULL;
	int bEnable, bIsRtspSrc;
	const char* rtspUrl, *rtmpUrl;

	if (strcmp(reqmsg->method, "POST") || strcmp(reqmsg->h_content_type, "application/json") || reqmsg->content_length <= 0) {
		resmsg->version = reqmsg->version;
		resmsg->status_code = 400;
		strcpy(resmsg->h_server, "RTMPMGR");
		return 1;
	}

	memcpy(jBuf, reqmsg->content_buff, reqmsg->content_length);
	jBuf[reqmsg->content_length] = 0;
	jRoot = cJSON_Parse(jBuf);
	if (NULL == jRoot) {
		resmsg->version = reqmsg->version;
		resmsg->status_code = 400;
		strcpy(resmsg->h_server, "RTMPMGR");
		return 1;
	}

	jChannel = cJSON_GetObjectItem(jRoot, "channel");
	jEnable = cJSON_GetObjectItem(jRoot, "enable");
	jSource = cJSON_GetObjectItem(jRoot, "source");
	jRtspUrl = cJSON_GetObjectItem(jRoot, "rtsp_url");
	jRtmpUrl = cJSON_GetObjectItem(jRoot, "rtmp_url");

	if (jChannel && cJSON_IsNumber(jChannel)) {
		if (jChannel->valueint >= 0 && (Easy_U32)jChannel->valueint < ctx->m_iBaseChannelNum + ctx->m_iExtChannelNum) {
			ch = jChannel->valueint;
		}
	}

	pChannel = &ctx->m_pstPushChannelAll[ch];
	bEnable = pChannel->m_bEnable;
	bIsRtspSrc = pChannel->m_bIsRtspSrc;
	rtspUrl = NULL;
	rtmpUrl = NULL;

	if (jEnable && cJSON_IsBool(jEnable)) {
		bEnable = cJSON_IsTrue(jEnable);
	}
	if (jSource && cJSON_IsString(jSource)) {
		bIsRtspSrc = strcmp(jSource->valuestring, "device");
	}
	if (jRtspUrl && cJSON_IsString(jRtspUrl)) {
		rtspUrl = jRtspUrl->valuestring;
	}
	if (jRtmpUrl && cJSON_IsString(jRtmpUrl)) {
		rtmpUrl = jRtmpUrl->valuestring;
	}

	_ChannelStop(pChannel);
	_ChannelSetCfg(pChannel, bEnable, bIsRtspSrc, rtspUrl, rtmpUrl);
	_ChannelSaveCfg(pChannel);
	if (bEnable) {
		_ChannelStart(pChannel);
	}

	cJSON_Delete(jRoot);

	jRoot = cJSON_CreateObject();
	cJSON_AddItemToObject(jRoot, "code", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(jRoot, "msg", cJSON_CreateString("成功"));

	cJSON_PrintPreallocated(jRoot, (char*)resmsg->content_buff, sizeof(resmsg->content_buff) - 1, 0);
	resmsg->content_length = strlen((char*)resmsg->content_buff);
	dbg("JSON: \n%s", resmsg->content_buff);

	resmsg->version = reqmsg->version;
	resmsg->status_code = 200;
	strcpy(resmsg->h_server, "RTMPMGR");
	strcpy((char*)resmsg->h_content_type, "application/json");

	cJSON_Delete(jRoot);
	return 1;
}

//ret 0:not-response 1:response -1:disconnect
static int _http_on_client_request (const ehttp_msg* reqmsg, ehttp_msg* resmsg, void* pdata)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*) pdata;

	if (!ctx) {
		err("BUG!!! pdata = NULL");
		return -1;
	}

	if (0 == strcmp(reqmsg->uri, "/get_config")) {
		return _http_get_config(ctx, reqmsg, resmsg);
	}

	if (0 == strcmp(reqmsg->uri, "/set_config")) {
		return _http_set_config(ctx, reqmsg, resmsg);
	}

	if (0 == strcmp(reqmsg->uri, "/reboot")) {
#ifdef __WIN32__
#else
#endif
	}
	return _http_status_page(ctx, reqmsg, resmsg);
}

static void _http_on_client_close (void* pdata)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*) pdata;
	if (!ctx) {
		err("BUG!!! pdata = NULL");
		return;
	}
	dbg("");
}

static void* RTMPMgr_Thd (void* arg)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*)arg;
	unsigned int ch, chNum = ctx->m_iBaseChannelNum + ctx->m_iExtChannelNum;
	dbg("Starting ...");

	for (ch = 0; ch < chNum; ch++) {
		if (ctx->m_pstPushChannelAll[ch].m_bEnable) {
			_ChannelStart(&ctx->m_pstPushChannelAll[ch]);
		}
	}

	while (ctx->m_bCanRun)
	{
		int bIdle = 1;
		int ret;

//		pthread_mutex_lock(&ctx->m_hThreadLock);

		ret = ehttpd_do_event(ctx->m_hHttpSrv);
		if (ret > 0) {
			bIdle = 0;
		}

//		pthread_mutex_unlock(&ctx->m_hThreadLock);
		if (bIdle) {
			usleep(100000);
		}
	}

	for (ch = 0; ch < chNum; ch++) {
		_ChannelStop(&ctx->m_pstPushChannelAll[ch]);
	}

	dbg("Stoped ...");
	return NULL;
}

RTMPMgr_Handle RTMPMgr_Start(const char* config, RTMPMgr_Callback cb, void* _user_data)
{
	struct RTMPMgr_Context* ctx = NULL;
	char* rtmpLicense = strdup("");
	char* aacEncLicense = strdup("");
	char* rtspLicense = strdup("");
	unsigned int httpPort = 80;
	unsigned int baseChnNum = 0, extChnNum = 0;
//	char name[EINI_NAME_BUF_SIZ];
	char value[EINI_VALUE_BUF_SIZ];
	int ret;
	ehttpd_callback http_cb = {_http_on_client_comming, _http_on_client_request, _http_on_client_close};
	unsigned int ch;

	if (!config || !cb.StartStream || !cb.StopStream){
		err("Invalid Param");
		goto _fail;
	}

	ctx = (struct RTMPMgr_Context*) calloc(1, sizeof(struct RTMPMgr_Context));
	if (!ctx) {
		err("Calloc for RTMPMgr_Context fail");
		goto _fail;
	}

	ctx->m_szConfigFile = strdup(config);
	ctx->m_fCallback = cb;
	ctx->m_pUserData = _user_data;

	ctx->m_hIni = eini_alloc();
	if (!ctx->m_hIni) {
		err("eini_alloc failed");
		goto _fail;
	}

	ret = eini_load_file(ctx->m_hIni, ctx->m_szConfigFile);
	if (ret < 0) {
		err("eini_load_file %s fail", ctx->m_szConfigFile);
		goto _fail;
	}

	ret = eini_ext_get(ctx->m_hIni, "Base.EasyRTMP_License", value);
	if (ret == 0) {
		free(rtmpLicense);
		rtmpLicense = strdup(value);
	}
	ret = eini_ext_get(ctx->m_hIni, "Base.EasyAACEncoder_License", value);
	if (ret == 0) {
		free(aacEncLicense);
		aacEncLicense = strdup(value);
	}
	ret = eini_ext_get(ctx->m_hIni, "Base.EasyRTSPClient_License", value);
	if (ret == 0) {
		free(rtspLicense);
		rtspLicense = strdup(value);
	}
	
	RTMPPushExt_Activate(rtmpLicense, aacEncLicense, rtspLicense);

	eini_ext_scanf(ctx->m_hIni, "Base.HttpSrvPort", "%u", &httpPort);
	eini_ext_scanf(ctx->m_hIni, "Base.BaseChannelNum", "%u", &baseChnNum);
	eini_ext_scanf(ctx->m_hIni, "Base.ExtendChannelNum", "%u", &extChnNum);

	if (baseChnNum + extChnNum < 1) {
		err("Invalid config BaseChannelNum or ExtendChannelNum");
		goto _fail;
	}

	ctx->m_iBaseChannelNum = baseChnNum;
	ctx->m_iExtChannelNum = extChnNum;
	ctx->m_pstPushChannelAll = (struct PushChannelContext*) calloc(baseChnNum + extChnNum, sizeof(struct PushChannelContext));
	if (NULL == ctx->m_pstPushChannelAll) {
		err("Call for channel context fail");
		goto _fail;
	}
	for (ch = 0; ch < baseChnNum + extChnNum; ch++) {
		ctx->m_pstPushChannelAll[ch].m_iChannelID = ch;
		ctx->m_pstPushChannelAll[ch].m_pMgr = ctx;
		pthread_mutex_init(&ctx->m_pstPushChannelAll[ch].m_hLock, NULL);

		ret = _ChannelLoadCfg(&ctx->m_pstPushChannelAll[ch]);
		if (ret < 0) {
			err("_ChannelLoadCfg for Channel %u fail", ch);
			goto _fail;
		}
	}

	ctx->m_hHttpSrv = ehttpd_create(httpPort, &http_cb, ctx);
	if (NULL == ctx->m_hHttpSrv) {
		err("ehttpd_create on %u", httpPort);
		goto _fail;
	}

//	pthread_mutex_init(&ctx->m_hThreadLock, NULL);
	ctx->m_bCanRun = 1;
	ret = pthread_create(&ctx->m_hThread, NULL, RTMPMgr_Thd, ctx);
	if (ret < 0) {
		err("pthread_create for RTMPMgr_Thd failed");
		goto _fail;
	}

	free(rtspLicense);
	free(aacEncLicense);
	free(rtmpLicense);
	return ctx;

_fail:
	if (ctx) {
		if (ctx->m_hHttpSrv)
			ehttpd_release(ctx->m_hHttpSrv);
		for (ch = 0; ch < baseChnNum + extChnNum; ch++) {
			_ChannelClearCfg(&ctx->m_pstPushChannelAll[ch]);
			pthread_mutex_destroy(&ctx->m_pstPushChannelAll[ch].m_hLock);
		}
		if (ctx->m_pstPushChannelAll)
			free(ctx->m_pstPushChannelAll);
		if (ctx->m_hIni)
			eini_free(ctx->m_hIni);
		if (ctx->m_szConfigFile)
			free(ctx->m_szConfigFile);
		free(ctx);
	}
	free(rtspLicense);
	free(aacEncLicense);
	free(rtmpLicense);
	return NULL;
}

Easy_I32 RTMPMgr_PushVideo(RTMPMgr_Handle handle, Easy_U32 channelID, const Easy_U8* frame, Easy_U32 size, Easy_U32 ts_ms)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*) handle;
	struct PushChannelContext* pChannel = NULL;
	AV_Frame stFrame;
	int ret;

	if (NULL == ctx) {
		return -1;
	}

	if (channelID >= ctx->m_iBaseChannelNum) {
		err("Invalid channelID");
		return -1;
	}

	pChannel = &ctx->m_pstPushChannelAll[channelID];
	pthread_mutex_lock(&pChannel->m_hLock);//XXX

	memset(&stFrame, 0, sizeof(stFrame));
	stFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
	stFrame.u32VFrameType = (frame[4]&0x1f) == 1 ? EASY_SDK_VIDEO_FRAME_P : EASY_SDK_VIDEO_FRAME_I; //FIXME
	stFrame.pBuffer = (Easy_U8*)frame;
	stFrame.u32AVFrameLen = size;
	stFrame.u32TimestampSec = (Easy_U32)(ts_ms / 1000);
	stFrame.u32TimestampUsec = (Easy_U32)(ts_ms % 1000) * 1000;
	//must be SPS/PPS/I | P

	ret = RTMPPushExt_SendFrame(pChannel->m_hPushHandle, &stFrame);

	pthread_mutex_unlock(&pChannel->m_hLock);//XXX
	return ret;
}

Easy_I32 RTMPMgr_PushAudio(RTMPMgr_Handle handle, Easy_U32 channelID, const Easy_U8* frame, Easy_U32 size, Easy_U32 ts_ms)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*) handle;
	struct PushChannelContext* pChannel = NULL;
	AV_Frame stFrame;
	int ret;

	if (NULL == ctx) {
		return -1;
	}

	if (channelID >= ctx->m_iBaseChannelNum) {
		err("Invalid channelID");
		return -1;
	}

	pChannel = &ctx->m_pstPushChannelAll[channelID];
	pthread_mutex_lock(&pChannel->m_hLock);//XXX

	memset(&stFrame, 0, sizeof(stFrame));
	stFrame.u32AVFrameFlag = EASY_SDK_AUDIO_FRAME_FLAG;
	stFrame.pBuffer = (Easy_U8*)frame;
	stFrame.u32AVFrameLen = size;
	stFrame.u32TimestampSec = (Easy_U32)(ts_ms / 1000);
	stFrame.u32TimestampUsec = (Easy_U32)(ts_ms % 1000) * 1000;
	//must be SPS/PPS/I | P

	ret = RTMPPushExt_SendFrame(pChannel->m_hPushHandle, &stFrame);

	pthread_mutex_unlock(&pChannel->m_hLock);//XXX
	return ret;
}

void RTMPMgr_Stop(RTMPMgr_Handle handle)
{
	struct RTMPMgr_Context* ctx = (struct RTMPMgr_Context*) handle;
	if (ctx) {
		unsigned int ch;

		dbg("Waitting for RTMPMgr_Thd exit ...");
		ctx->m_bCanRun = 0;
		pthread_join(ctx->m_hThread, NULL);
//		pthread_mutex_destroy(&ctx->m_hThreadLock);

		dbg("Relase resource ...");
		if (ctx->m_hHttpSrv)
			ehttpd_release(ctx->m_hHttpSrv);
		for (ch = 0; ch < ctx->m_iBaseChannelNum + ctx->m_iExtChannelNum; ch++) {
			_ChannelClearCfg(&ctx->m_pstPushChannelAll[ch]);
			pthread_mutex_destroy(&ctx->m_pstPushChannelAll[ch].m_hLock);
		}
		if (ctx->m_pstPushChannelAll)
			free(ctx->m_pstPushChannelAll);
		if (ctx->m_hIni)
			eini_free(ctx->m_hIni);
		if (ctx->m_szConfigFile)
			free(ctx->m_szConfigFile);
		free(ctx);
	}
}

