#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "comm.h"
#include "RTMPPushExt.h"
#include "EasyRTMPAPI.h"
#include "EasyAACEncoderAPI.h"
#include "EasyRTSPClientAPI.h"

typedef struct
{
	Easy_RTMP_Handle m_hRtmp;
	EasyAACEncoder_Handle m_hAacEnc;
	Easy_RTSP_Handle m_hRtspSrc;

	int m_bStarted;
	int m_bIsRtspSrc;
	char m_szRtspUrl[128];
	char m_szRtspUsername[64];
	char m_szRtspPasswd[64];
	Easy_U32 m_iBufferKSize;
	EASY_MEDIA_INFO_T m_stSrcMediaInfo;

	unsigned char m_pAACEncBufer[64*1024];

	RTMPPushExt_CallBack m_fCallback;
	void* m_pUserData;
} RTMPExt_Context;

Easy_I32 RTMPPushExt_Activate(const char* rtmpLicense, const char* aacEncLicense, const char* rtspLicense)
{
	Easy_I32 ret0 = 0, ret1 = 0, ret2 = 0;

	ret0 = EasyRTMP_Activate((char*)rtmpLicense);
    if (ret0 < 0) {
        err("EasyRTMP_Activate fail %d", ret0);
    }

	ret1 = 0;//EasyAACEncoder_Activate((char*)aacEncLicense);
    if (ret1 < 0) {
        err("EasyAACEncoder_Activate fail %d", ret1);
    }

	ret2 = EasyRTSP_Activate((char*)rtspLicense);
    if (ret2 < 0) {
        err("EasyRTSP_Activate fail %d", ret2);
    }

	return (ret0 + ret1 + ret2);
}

RTMPPushExt_Handle RTMPPushExt_Create(void)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) calloc(1, sizeof(RTMPExt_Context));
	if (NULL == ctx) {
		err("Calloc for RTMPExt_Context failed, %s", strerror(errno));
		return NULL;
	}

	return ctx;
}

Easy_I32 RTMPPushExt_InitLocalSource(RTMPPushExt_Handle handle, const EASY_MEDIA_INFO_T* pstruStreamInfo)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) handle;
	if (NULL == ctx || NULL == pstruStreamInfo) {
		return -1;
	}
	if (ctx->m_bStarted) {
		err("RTMPPush has started!");
		return -1;
	}

	if (pstruStreamInfo->u32VideoCodec != EASY_SDK_VIDEO_CODEC_H264) {
		err("Only support h264 video");
		return -1;
	}
	ctx->m_bIsRtspSrc = 0;
	memcpy(&ctx->m_stSrcMediaInfo, pstruStreamInfo, sizeof(EASY_MEDIA_INFO_T));
	return 0;
}

Easy_I32 RTMPPushExt_InitRTSPSource(RTMPPushExt_Handle handle, const char* rtspUrl, const char* username, const char* passwd)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) handle;
	if (NULL == ctx) {
		return -1;
	}
	if (ctx->m_bStarted) {
		err("RTMPPush has started!");
		return -1;
	}
	if (NULL == rtspUrl || strncmp(rtspUrl, "rtsp", 4)) {
		err("Invalid rtspUrl %s", rtspUrl);
		return -1;
	}
	ctx->m_bIsRtspSrc = 1;
	memset(ctx->m_szRtspUrl, 0, sizeof(ctx->m_szRtspUrl));
	memset(ctx->m_szRtspUsername, 0, sizeof(ctx->m_szRtspUsername));
	memset(ctx->m_szRtspPasswd, 0, sizeof(ctx->m_szRtspPasswd));
	strncpy(ctx->m_szRtspUrl, rtspUrl, sizeof(ctx->m_szRtspUrl) - 1);
	if (username) {
		strncpy(ctx->m_szRtspUsername, username, sizeof(ctx->m_szRtspUsername) - 1);
	}
	if (passwd) {
		strncpy(ctx->m_szRtspPasswd, passwd, sizeof(ctx->m_szRtspPasswd) - 1);
	}
	return 0;
}

Easy_I32 RTMPPushExt_InitCallback(RTMPPushExt_Handle handle, RTMPPushExt_CallBack _callback, void* _userPtr)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) handle;
	if (NULL == ctx) {
		return -1;
	}
	if (ctx->m_bStarted) {
		err("RTMPPush has started!");
		return -1;
	}
	ctx->m_fCallback = _callback;
	ctx->m_pUserData = _userPtr;
	return 0;
}

static int InitRtmpMediaInfoAndAACEncoder (RTMPExt_Context* ctx, EASY_MEDIA_INFO_T* pmediaInfo)
{
	memset(pmediaInfo, 0, sizeof(EASY_MEDIA_INFO_T));

	if (ctx->m_stSrcMediaInfo.u32VideoCodec == EASY_SDK_VIDEO_CODEC_H264) {
		pmediaInfo->u32VideoCodec = EASY_SDK_VIDEO_CODEC_H264;
		pmediaInfo->u32VideoFps = 25;//XXX
		pmediaInfo->u32SpsLength = ctx->m_stSrcMediaInfo.u32SpsLength;
		pmediaInfo->u32PpsLength = ctx->m_stSrcMediaInfo.u32PpsLength;
		memcpy(pmediaInfo->u8Sps, ctx->m_stSrcMediaInfo.u8Sps, pmediaInfo->u32SpsLength);
		memcpy(pmediaInfo->u8Pps, ctx->m_stSrcMediaInfo.u8Pps, pmediaInfo->u32PpsLength);
	}

	if (ctx->m_stSrcMediaInfo.u32AudioCodec) {
		if (ctx->m_stSrcMediaInfo.u32AudioCodec != EASY_SDK_AUDIO_CODEC_AAC) {
			InitParam initParam;
			initParam.u32AudioSamplerate = ctx->m_stSrcMediaInfo.u32AudioSamplerate;
			initParam.ucAudioChannel =  ctx->m_stSrcMediaInfo.u32AudioChannel;
			initParam.u32PCMBitSize =  ctx->m_stSrcMediaInfo.u32AudioBitsPerSample;

			if (EASY_SDK_AUDIO_CODEC_G711A == ctx->m_stSrcMediaInfo.u32AudioCodec) {
				initParam.ucAudioCodec = Law_ALaw;
			} 
			else if (EASY_SDK_AUDIO_CODEC_G711U == ctx->m_stSrcMediaInfo.u32AudioCodec) {
				initParam.ucAudioCodec = Law_ULaw;
			}
			else if (EASY_SDK_AUDIO_CODEC_G726 == ctx->m_stSrcMediaInfo.u32AudioCodec) {
				initParam.ucAudioCodec = Law_G726;
			}
			else {
				warn("Not support Audio Codec %u", ctx->m_stSrcMediaInfo.u32AudioCodec);
				return 0;
			}

			if (ctx->m_hAacEnc) {
				Easy_AACEncoder_Release(ctx->m_hAacEnc);
				ctx->m_hAacEnc = NULL;
			}

			ctx->m_hAacEnc = Easy_AACEncoder_Init(initParam);
			if (NULL == ctx->m_hAacEnc) {
				warn("Easy_AACEncoder_Init failed");
				return 0;
			}
		}
		pmediaInfo->u32AudioCodec = EASY_SDK_AUDIO_CODEC_AAC;
		pmediaInfo->u32AudioSamplerate = ctx->m_stSrcMediaInfo.u32AudioSamplerate;
		pmediaInfo->u32AudioBitsPerSample = ctx->m_stSrcMediaInfo.u32AudioBitsPerSample;
		pmediaInfo->u32AudioChannel = ctx->m_stSrcMediaInfo.u32AudioChannel;
	}

	return 0;
}

/* EasyRTMP callback */
static int __RTMP_Callback(int _frameType, char *pBuf, EASY_RTMP_STATE_T _state, void *_userPtr)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) _userPtr;
	if (NULL == ctx) {
		err("BUG!!! _userPtr = NULL");
		return -1;
	}

	if (_state == EASY_RTMP_STATE_CONNECTING) {
//		dbg("Connecting...");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_CONNECTING, ctx->m_pUserData);
		}
	}
	else if (_state == EASY_RTMP_STATE_CONNECTED) {
//		dbg("Connected");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_CONNECTED, ctx->m_pUserData);
		}
	}
	else if (_state == EASY_RTMP_STATE_CONNECT_FAILED) {
//		dbg("Connect failed");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_CONNECT_FAILED, ctx->m_pUserData);
		}
	}
	else if (_state == EASY_RTMP_STATE_CONNECT_ABORT) {
//		dbg("Connect abort");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_CONNECT_ABORT, ctx->m_pUserData);
		}
	}
	else if (_state == EASY_RTMP_STATE_PUSHING) {
//		dbg("Pushing");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_PUSHING, ctx->m_pUserData);
		}
	}
	else if (_state == EASY_RTMP_STATE_DISCONNECTED) {
//		dbg("Disconnect.");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_DISCONNECTED, ctx->m_pUserData);
		}
	} 
	else if (_state == EASY_RTMP_STATE_ERROR) {
//		dbg("Error.");
		if (ctx->m_fCallback) {
			ctx->m_fCallback(ctx, EVENT_RTMP_ERROR, ctx->m_pUserData);
		}
	} 
	else {
		warn("Unknown state = %d", _state);
	}

	return 0;
}

static int Easy_APICALL __RTSPSourceCallBack (int _channelId, void *_userPtr, int _frameType, char *pBuf, RTSP_FRAME_INFO* _frameInfo)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) _userPtr;
	int processed = 0, ret;
//	dbg("chn%d ptr:%p frameType:%d pBuf:%p pframeInfo:%p", _channelId, _userPtr, _frameType, pBuf, _frameInfo);

	if (NULL == ctx || NULL == ctx->m_hRtmp || 0 == ctx->m_bIsRtspSrc) {
		err("BUG!!! Invalid ctx(%p) or m_hRtmp or m_bIsRtspSrc", ctx);
		return -1;
	}

//	if (_frameInfo) {
//		dbg("codec:%x type:%x fps:%u, size:%ux%u sample_rate:%u channels:%u bitwidth:%u length:%u ts:%u.%03u bitrate:%f loss:%f", 
//			_frameInfo->codec, _frameInfo->type, _frameInfo->fps, _frameInfo->width, _frameInfo->height, 
//			_frameInfo->sample_rate, _frameInfo->channels, _frameInfo->bits_per_sample, _frameInfo->length,
//			_frameInfo->timestamp_sec, _frameInfo->timestamp_usec/1000, _frameInfo->bitrate, _frameInfo->losspacket);
//	}

	if (_frameType == EASY_SDK_MEDIA_INFO_FLAG && pBuf) 
	{
		const EASY_MEDIA_INFO_T *pmi = (const EASY_MEDIA_INFO_T*)pBuf;
		dbg("VCodec:%x fps:%u ACodec:%x samplerate:%u channels:%u bitwidth:%u, VPS/SPS/PPS/SEI:%u/%u/%u/%u SPSRaw:%02X%02X%02X%02X%02X", 
			pmi->u32VideoCodec, pmi->u32VideoFps, pmi->u32AudioCodec, pmi->u32AudioSamplerate, pmi->u32AudioChannel, pmi->u32AudioBitsPerSample,
			pmi->u32VpsLength, pmi->u32SpsLength, pmi->u32PpsLength, pmi->u32SeiLength,
			pmi->u8Sps[0], pmi->u8Sps[1], pmi->u8Sps[2], pmi->u8Sps[3], pmi->u8Sps[4]);

		if (ctx->m_fCallback) { //XXX
			ctx->m_fCallback(ctx, EVENT_RTSPSRC_CONNECTED, ctx->m_pUserData);
		}

		memcpy(&ctx->m_stSrcMediaInfo, pBuf, sizeof(EASY_MEDIA_INFO_T));
		EASY_MEDIA_INFO_T mediaInfo;
		InitRtmpMediaInfoAndAACEncoder(ctx, &mediaInfo);

		ret = EasyRTMP_InitMetadata(ctx->m_hRtmp, &mediaInfo, ctx->m_iBufferKSize);
		if (ret < 0) {
			err("EasyRTMP_InitMetadata failed %d", ret);
			return -1;
		}
		processed = 1;
	}

	if (_frameType == EASY_SDK_EVENT_FRAME_FLAG) 
	{
		if (NULL == _frameInfo) {
			if (ctx->m_fCallback) { //XXX
				ctx->m_fCallback(ctx, EVENT_RTSPSRC_CONNECTING, ctx->m_pUserData);
			}
			processed = 1;
		}
		else if (_frameInfo && _frameInfo->codec == EASY_SDK_EVENT_CODEC_ERROR) {
			if (ctx->m_fCallback) { //XXX
				ctx->m_fCallback(ctx, EVENT_RTSPSRC_CONNECT_FAILED, ctx->m_pUserData);
			}
			processed = 1;
		} 
		else if (_frameInfo && _frameInfo->codec == EASY_SDK_EVENT_CODEC_EXIT) {
			if (ctx->m_fCallback) { //XXX
				ctx->m_fCallback(ctx, EVENT_RTSPSRC_DISCONNECTED, ctx->m_pUserData);
			}
			processed = 1;
		}
	}

	if (_frameType == EASY_SDK_VIDEO_FRAME_FLAG && pBuf && _frameInfo && _frameInfo->length > 0) 
	{
		if (_frameInfo->codec == EASY_SDK_VIDEO_CODEC_H264) {
			EASY_AV_Frame avFrame;
			memset(&avFrame, 0, sizeof(EASY_AV_Frame));

			avFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
			avFrame.u32AVFrameLen = _frameInfo->length;
			avFrame.pBuffer = (unsigned char*)pBuf;
			avFrame.u32VFrameType = _frameInfo->type;
			avFrame.u32TimestampSec = _frameInfo->timestamp_sec;
			avFrame.u32TimestampUsec = _frameInfo->timestamp_usec;
			
//			dbg("H264 %02X%02X%02X%02X%02X", avFrame.pBuffer[0], avFrame.pBuffer[1], 
//				avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4]);
			ret = EasyRTMP_SendPacket(ctx->m_hRtmp, &avFrame);
			if (ret < 0) {
				err("Fail to EasyRTMP_SendPacket Video...");
			}
			processed = 1;
		}
	}

	else if (_frameType == EASY_SDK_AUDIO_FRAME_FLAG && pBuf && _frameInfo && _frameInfo->length > 0) 
	{
		EASY_AV_Frame avFrame;
		memset(&avFrame, 0x00, sizeof(EASY_AV_Frame));

		avFrame.u32AVFrameFlag = EASY_SDK_AUDIO_FRAME_FLAG;
		avFrame.u32TimestampSec = _frameInfo->timestamp_sec;
		avFrame.u32TimestampUsec = _frameInfo->timestamp_usec;

		if(_frameInfo->codec == EASY_SDK_AUDIO_CODEC_AAC)
		{
			avFrame.pBuffer = (Easy_U8*)(pBuf);
			avFrame.u32AVFrameLen  = _frameInfo->length;
//			dbg("AAC %02X%02X%02X%02X%02X", avFrame.pBuffer[0], avFrame.pBuffer[1], 
//				avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4]);
			ret = EasyRTMP_SendPacket(ctx->m_hRtmp, &avFrame);
			if (ret < 0) {
				err("Fail to EasyRTMP_SendPacket Audio...");
			}
		}
		else if (ctx->m_hAacEnc)
		{
			unsigned int iAACBufferLen = 0;

			if(Easy_AACEncoder_Encode(ctx->m_hAacEnc, (unsigned char*)pBuf,  _frameInfo->length, ctx->m_pAACEncBufer, &iAACBufferLen) > 0)
			{
				//dbg("AACEncoder_Encode() = %d", iAACBufferLen);
				avFrame.pBuffer = (Easy_U8*)(ctx->m_pAACEncBufer);
				avFrame.u32AVFrameLen  = iAACBufferLen;	
//				dbg("AAC %02X%02X%02X%02X%02X", avFrame.pBuffer[0], avFrame.pBuffer[1], 
//					avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4]);
				ret = EasyRTMP_SendPacket(ctx->m_hRtmp, &avFrame);
				if (ret < 0) {
					err("Fail to EasyRTMP_SendPacket Audio...");
				}
			}
		}
		processed = 1;
	}

	if (processed == 0){
		warn("Unknown chn%d ptr:%p frameType:%d pBuf:%p pframeInfo:%p", _channelId, _userPtr, _frameType, pBuf, _frameInfo);
		if (_frameInfo) {
			warn("Unkown codec:%x type:%x fps:%u, size:%ux%u sample_rate:%u channels:%u bitwidth:%u length:%u ts:%u.%03u bitrate:%f loss:%f", 
				_frameInfo->codec, _frameInfo->type, _frameInfo->fps, _frameInfo->width, _frameInfo->height, 
				_frameInfo->sample_rate, _frameInfo->channels, _frameInfo->bits_per_sample, _frameInfo->length,
				_frameInfo->timestamp_sec, _frameInfo->timestamp_usec/1000, _frameInfo->bitrate, _frameInfo->losspacket);
		}
	}

	return 0;
}


Easy_I32 RTMPPushExt_Start(RTMPPushExt_Handle handle, const char* rtmpUrl, Easy_U32 bufferKSize)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) handle;
	int ret;

	if (NULL == ctx || NULL == rtmpUrl) {
		return -1;
	}
	if (ctx->m_bStarted) {
		err("RTMPPush has started!");
		return -1;
	}
	if (ctx->m_bIsRtspSrc) {
		if (ctx->m_szRtspUrl[0] == 0) {
			err("Rtsp source not init");
			return -1;
		}
	} else {
		if (ctx->m_stSrcMediaInfo.u32VideoCodec == 0 && ctx->m_stSrcMediaInfo.u32AudioCodec == 0) {
			err("Local source not init");
			return -1;
		}
	}

	ctx->m_hRtmp = EasyRTMP_Create();
	if (NULL == ctx->m_hRtmp) {
		err("EasyRTMP_Create failed");
		return -1;
	}

	ret = EasyRTMP_SetCallback(ctx->m_hRtmp, __RTMP_Callback, ctx);
	if (ret < 0) {
		err("EasyRTMP_SetCallback failed %d", ret);
		return -1;
	}

	ret = EasyRTMP_Connect(ctx->m_hRtmp, rtmpUrl);
	if (ret < 0) {
		err("EasyRTMP_Connect failed %d", ret);
		return -1;
	}

	ctx->m_iBufferKSize = bufferKSize;

	if (0 == ctx->m_bIsRtspSrc) {
		EASY_MEDIA_INFO_T mediaInfo;
		InitRtmpMediaInfoAndAACEncoder(ctx, &mediaInfo);

		ret = EasyRTMP_InitMetadata(ctx->m_hRtmp, &mediaInfo, ctx->m_iBufferKSize);
		if (ret < 0) {
			err("EasyRTMP_InitMetadata failed %d", ret);
			return -1;
		}
	}
	else {
		ret = EasyRTSP_Init(&ctx->m_hRtspSrc);
		if (ret < 0 || NULL == ctx->m_hRtspSrc) {
			err("EasyRTSP_Init failed %d", ret);
			return -1;
		}

		ret = EasyRTSP_SetCallback(ctx->m_hRtspSrc, __RTSPSourceCallBack);
		if (ret < 0) {
			err("EasyRTSP_SetCallback failed %d", ret);
			return -1;
		}

		unsigned int mediaType = EASY_SDK_VIDEO_FRAME_FLAG | EASY_SDK_AUDIO_FRAME_FLAG;
		const char* username = ctx->m_szRtspUsername[0] ? ctx->m_szRtspUsername : NULL;
		const char* passwd = ctx->m_szRtspPasswd[0] ? ctx->m_szRtspPasswd : NULL;
		ret = EasyRTSP_OpenStream(ctx->m_hRtspSrc, 0, ctx->m_szRtspUrl, EASY_RTP_OVER_TCP, mediaType, 
			(char*)username, (char*)passwd, ctx, 1000, 0, 1, 0);
		if (ret < 0) {
			err("EasyRTSP_OpenStream failed %d", ret);
			return -1;
		}
	}

	ctx->m_bStarted = 1;

	return 0;
}

Easy_U32 RTMPPushExt_SendFrame(RTMPPushExt_Handle handle, const AV_Frame* frame)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) handle;
	int ret;

	if (NULL == ctx || NULL == frame || NULL == frame->pBuffer || 0 == frame->u32AVFrameLen) {
		return -1;
	}

	if (ctx->m_bIsRtspSrc) {
		err("Stream source is rtsp");
		return -1;
	}

	if (NULL == ctx || NULL == ctx->m_hRtmp || 0 == ctx->m_bStarted) {
		err("BUG!!! Invalid ctx(%p) or m_hRtmp or m_bStarted", ctx);
		return -1;
	}

	ret = 0;

	if (frame->u32AVFrameFlag == EASY_SDK_VIDEO_FRAME_FLAG) {
		EASY_AV_Frame avFrame;
		memset(&avFrame, 0, sizeof(EASY_AV_Frame));

		avFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
		avFrame.u32AVFrameLen = frame->u32AVFrameLen;
		avFrame.pBuffer = (unsigned char*)frame->pBuffer;
		avFrame.u32VFrameType = frame->u32VFrameType;
		avFrame.u32TimestampSec = frame->u32TimestampSec;
		avFrame.u32TimestampUsec = frame->u32TimestampUsec;

//		dbg("H264 %02X%02X%02X%02X%02X", avFrame.pBuffer[0], avFrame.pBuffer[1], 
//			avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4]);

		//must be SPS/PPS/I | P
		ret = EasyRTMP_SendPacket(ctx->m_hRtmp, &avFrame);
		if (ret < 0) {
			err("Fail to EasyRTMP_SendPacket Video...\n");
		}
	}

	if (frame->u32AVFrameFlag == EASY_SDK_AUDIO_FRAME_FLAG) {
		EASY_AV_Frame avFrame;
		memset(&avFrame, 0x00, sizeof(EASY_AV_Frame));

		avFrame.u32AVFrameFlag = EASY_SDK_AUDIO_FRAME_FLAG;
		avFrame.u32TimestampSec = frame->u32TimestampSec;
		avFrame.u32TimestampUsec = frame->u32TimestampUsec;

		if(ctx->m_stSrcMediaInfo.u32AudioCodec == EASY_SDK_AUDIO_CODEC_AAC)
		{
			avFrame.pBuffer = (Easy_U8*)(frame->pBuffer);
			avFrame.u32AVFrameLen = frame->u32AVFrameLen;	
//			dbg("AAC %02X%02X%02X%02X%02X", avFrame.pBuffer[0], avFrame.pBuffer[1], 
//				avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4]);
			ret = EasyRTMP_SendPacket(ctx->m_hRtmp, &avFrame);
			if (ret < 0) {
				err("Fail to EasyRTMP_SendPacket Audio...\n");
			}
		}
		else if (ctx->m_hAacEnc)
		{
			unsigned int iAACBufferLen = 0;

			if(Easy_AACEncoder_Encode(ctx->m_hAacEnc, (unsigned char*)frame->pBuffer,  frame->u32AVFrameLen, ctx->m_pAACEncBufer, &iAACBufferLen) > 0)
			{
				avFrame.pBuffer = (Easy_U8*)(ctx->m_pAACEncBufer);
				avFrame.u32AVFrameLen  = iAACBufferLen;
//				dbg("AAC %02X%02X%02X%02X%02X", avFrame.pBuffer[0], avFrame.pBuffer[1], 
//					avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4]);
				ret = EasyRTMP_SendPacket(ctx->m_hRtmp, &avFrame);
				if (ret < 0) {
					err("Fail to EasyRTMP_SendPacket Audio...\n");
				}
			}
		}
	}

	return ret;
}

void RTMPPushExt_Release(RTMPPushExt_Handle handle)
{
	RTMPExt_Context* ctx = (RTMPExt_Context*) handle;
	if (ctx) {
		if (ctx->m_hRtspSrc) {
			if (ctx->m_bStarted) {
				EasyRTSP_CloseStream(ctx->m_hRtspSrc);
			}
			EasyRTSP_Deinit(&ctx->m_hRtspSrc);
		}
		if (ctx->m_hAacEnc) {
			Easy_AACEncoder_Release(ctx->m_hAacEnc);
		}
		if (ctx->m_hRtmp) {
			EasyRTMP_Release(ctx->m_hRtmp);
		}
		free(ctx);
	}
}
