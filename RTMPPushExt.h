#ifndef __RTMP_PUSH_EXT_H__
#define __RTMP_PUSH_EXT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <EasyTypes.h>

typedef struct __AV_Frame
{
    Easy_U32    u32AVFrameFlag;		/* ֡��־  ��Ƶ or ��Ƶ */
    Easy_U32    u32AVFrameLen;		/* ֡�ĳ��� */
    Easy_U32    u32VFrameType;		/* ��Ƶ�����ͣ�I֡��P֡ */
    Easy_U8     *pBuffer;			/* ���� */
	Easy_U32	u32TimestampSec;	/* ʱ���(��)*/
	Easy_U32	u32TimestampUsec;	/* ʱ���(΢��) */
} AV_Frame;

/* �����¼����Ͷ��� */
typedef enum __EVENT_T
{
    EVENT_RTMP_CONNECTING   =   1,     /* ������ */
    EVENT_RTMP_CONNECTED,              /* ���ӳɹ� */
    EVENT_RTMP_CONNECT_FAILED,         /* ����ʧ�� */
    EVENT_RTMP_CONNECT_ABORT,          /* �����쳣�ж� */
    EVENT_RTMP_PUSHING,                /* ������ */
    EVENT_RTMP_DISCONNECTED,           /* �Ͽ����� */
    EVENT_RTMP_ERROR,
    EVENT_RTSPSRC_CONNECTING,			/* ������ */
    EVENT_RTSPSRC_CONNECTED,            /* ���ӳɹ� */
    EVENT_RTSPSRC_CONNECT_FAILED,       /* ����ʧ�� */
    EVENT_RTSPSRC_DISCONNECTED,         /* �Ͽ����� */
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
