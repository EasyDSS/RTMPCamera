#ifndef __RTMP_PUSH_EXT_H__
#define __RTMP_PUSH_EXT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <EasyTypes.h>

typedef struct __AV_Frame
{
    Easy_U32    u32AVFrameFlag;		/* 帧标志  视频 or 音频 */
    Easy_U32    u32AVFrameLen;		/* 帧的长度 */
    Easy_U32    u32VFrameType;		/* 视频的类型，I帧或P帧 */
    Easy_U8     *pBuffer;			/* 数据 */
	Easy_U32	u32TimestampSec;	/* 时间戳(秒)*/
	Easy_U32	u32TimestampUsec;	/* 时间戳(微秒) */
} AV_Frame;

/* 推送事件类型定义 */
typedef enum __EVENT_T
{
    EVENT_RTMP_CONNECTING   =   1,     /* 连接中 */
    EVENT_RTMP_CONNECTED,              /* 连接成功 */
    EVENT_RTMP_CONNECT_FAILED,         /* 连接失败 */
    EVENT_RTMP_CONNECT_ABORT,          /* 连接异常中断 */
    EVENT_RTMP_PUSHING,                /* 推流中 */
    EVENT_RTMP_DISCONNECTED,           /* 断开连接 */
    EVENT_RTMP_ERROR,
    EVENT_RTSPSRC_CONNECTING,			/* 连接中 */
    EVENT_RTSPSRC_CONNECTED,            /* 连接成功 */
    EVENT_RTSPSRC_CONNECT_FAILED,       /* 连接失败 */
    EVENT_RTSPSRC_DISCONNECTED,         /* 断开连接 */
} EVENT_T;


typedef void* RTMPPushExt_Handle;
typedef Easy_I32 (*RTMPPushExt_CallBack)(RTMPPushExt_Handle handle, EVENT_T _event_type, void* _userPtr);

Easy_I32 RTMPPushExt_Activate(const char* rtmpLicense, const char* aacEncLicense, const char* rtspLicense);
RTMPPushExt_Handle RTMPPushExt_Create(void);
Easy_I32 RTMPPushExt_InitLocalSource(RTMPPushExt_Handle handle, const EASY_MEDIA_INFO_T* pstruStreamInfo);
Easy_I32 RTMPPushExt_InitRTSPSource(RTMPPushExt_Handle handle, const char* rtspUrl, const char* username, const char* passwd);
Easy_I32 RTMPPushExt_InitCallback(RTMPPushExt_Handle handle, RTMPPushExt_CallBack _callback, void* _userPtr);
Easy_I32 RTMPPushExt_Start(RTMPPushExt_Handle handle, const char* rtmpUrl, Easy_U32 m_iBufferKSize);
Easy_U32 RTMPPushExt_SendFrame(RTMPPushExt_Handle handle, const AV_Frame* frame);
void RTMPPushExt_Release(RTMPPushExt_Handle handle);

#ifdef __cplusplus
}
#endif

#endif
