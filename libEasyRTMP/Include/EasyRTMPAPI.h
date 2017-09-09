/*
	Copyright (c) 2013-2015 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
#ifndef _Easy_RTMP_API_H
#define _Easy_RTMP_API_H

#include "EasyTypes.h"

#ifdef _WIN32
#define EasyRTMP_API  __declspec(dllexport)
#define Easy_APICALL  __stdcall
#else
#define EasyRTMP_API
#define Easy_APICALL 
#endif

#define Easy_RTMP_Handle void*

typedef struct __EASY_AV_Frame
{
    Easy_U32    u32AVFrameFlag;		/* ֡��־  ��Ƶ or ��Ƶ */
    Easy_U32    u32AVFrameLen;		/* ֡�ĳ��� */
    Easy_U32    u32VFrameType;		/* ��Ƶ�����ͣ�I֡��P֡ */
    Easy_U8     *pBuffer;			/* ���� */
	Easy_U32	u32TimestampSec;	/* ʱ���(��)*/
	Easy_U32	u32TimestampUsec;	/* ʱ���(΢��) */
}EASY_AV_Frame;

/* �����¼����Ͷ��� */
typedef enum __EASY_RTMP_STATE_T
{
    EASY_RTMP_STATE_CONNECTING   =   1,     /* ������ */
    EASY_RTMP_STATE_CONNECTED,              /* ���ӳɹ� */
    EASY_RTMP_STATE_CONNECT_FAILED,         /* ����ʧ�� */
    EASY_RTMP_STATE_CONNECT_ABORT,          /* �����쳣�ж� */
    EASY_RTMP_STATE_PUSHING,                /* ������ */
    EASY_RTMP_STATE_DISCONNECTED,           /* �Ͽ����� */
    EASY_RTMP_STATE_ERROR
}EASY_RTMP_STATE_T;

/*
	_frameType:		EASY_SDK_VIDEO_FRAME_FLAG/EASY_SDK_AUDIO_FRAME_FLAG/EASY_SDK_EVENT_FRAME_FLAG/...	
	_pBuf:			�ص������ݲ��֣������÷���Demo
	_frameInfo:		֡�ṹ����
	_userPtr:		�û��Զ�������
*/
typedef int (*EasyRTMPCallBack)(int _frameType, char *pBuf, EASY_RTMP_STATE_T _state, void *_userPtr);

#ifdef __cplusplus
extern "C" 
{
#endif
	/* ����EasyRTMP */
#ifdef ANDROID
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_Activate(char *license, char* userPtr);
#else
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_Activate(char *license);
#endif

	/* ����RTMP����Session �������;�� */
	EasyRTMP_API Easy_RTMP_Handle Easy_APICALL EasyRTMP_Create(void);

	/* �������ݻص� */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_SetCallback(Easy_RTMP_Handle handle, EasyRTMPCallBack _callback, void * _userptr);

	/* ����RTMP���͵Ĳ�����Ϣ */
	EasyRTMP_API Easy_I32 Easy_APICALL Easy_APICALL EasyRTMP_InitMetadata(Easy_RTMP_Handle handle, EASY_MEDIA_INFO_T*  pstruStreamInfo, Easy_U32 bufferKSize);
	
	/* ����RTMP������ */
	EasyRTMP_API Easy_Bool Easy_APICALL EasyRTMP_Connect(Easy_RTMP_Handle handle, const char *url);

	/* ����H264��AAC�� */
	EasyRTMP_API Easy_U32 Easy_APICALL EasyRTMP_SendPacket(Easy_RTMP_Handle handle, EASY_AV_Frame* frame);

	/* ֹͣRTMP���ͣ��ͷž�� */
	EasyRTMP_API void Easy_APICALL EasyRTMP_Release(Easy_RTMP_Handle handle);

#ifdef __cplusplus
};
#endif

#endif
