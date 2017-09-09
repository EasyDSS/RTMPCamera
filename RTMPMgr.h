#ifndef __RTMP_MGR_H__
#define __RTMP_MGR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <EasyTypes.h>

typedef struct
{
	Easy_I32 (*StartStream)(Easy_U32 channelID, EASY_MEDIA_INFO_T* mediaInfo, void* _user_data);
	void (*StopStream)(Easy_U32 channelID, void* _user_data);
} RTMPMgr_Callback;

typedef void* RTMPMgr_Handle;
RTMPMgr_Handle RTMPMgr_Start(const char* config, RTMPMgr_Callback cb, void* _user_data);
Easy_I32 RTMPMgr_PushVideo(RTMPMgr_Handle handle, Easy_U32 channelID, const Easy_U8* frame, Easy_U32 size, Easy_U32 ts_ms);
Easy_I32 RTMPMgr_PushAudio(RTMPMgr_Handle handle, Easy_U32 channelID, const Easy_U8* frame, Easy_U32 size, Easy_U32 ts_ms);
void RTMPMgr_Stop(RTMPMgr_Handle handle);

#ifdef __cplusplus
}
#endif

#endif
