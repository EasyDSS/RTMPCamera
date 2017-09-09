#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "Comm.h"
#include "RTMPMgr.h"

#ifdef __WIN32__
#include <Windows.h>
#include <Mmsystem.h>
#include "w32pthreads.h"
#include <io.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#define USE_FILE_STREAM

#ifdef USE_FILE_STREAM
#else
#include <hi_net_dev_sdk.h>
#endif

typedef struct
{
	Easy_U32 m_iChannelID;
#ifdef USE_FILE_STREAM
	int m_bCanRun;
	pthread_t m_hPushThd;
#else
	HI_U32 m_hHiChannelHandle;
#endif
} MgrChnnelData;

#define MAX_BASE_CHN_NUM	(16)
static RTMPMgr_Handle g_hMgr;
static MgrChnnelData g_stChannels[MAX_BASE_CHN_NUM];

#ifdef USE_FILE_STREAM
//return us from system running
uint64_t os_get_reltime (void)
{
#ifdef __WIN32__
	return (timeGetTime() * 1000ULL);
#endif
#ifdef __LINUX__
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000ULL);
#endif
}

static int get_next_video_frame (FILE *fp, uint8_t **buff, int *size)
{
	uint8_t szbuf[1024];
	int szlen = 0;
	int ret;
	if (!(*buff)) {
		*buff = (uint8_t*)malloc(2*1024*1024);
		if (!(*buff))
			return -1;
	}

	*size = 0;

	while ((ret = fread(szbuf + szlen, 1, sizeof(szbuf) - szlen, fp)) > 0) {
		int i = 3;
		szlen += ret;
		while (i < szlen - 3 && !(szbuf[i] == 0 &&  szbuf[i+1] == 0 && (szbuf[i+2] == 1 || (szbuf[i+2] == 0 && szbuf[i+3] == 1)))) i++;
		memcpy(*buff + *size, szbuf, i);
		*size += i;
		memmove(szbuf, szbuf + i, szlen - i);
		szlen -= i;
		if (szlen > 3) {
			//printf("szlen %d\n", szlen);
			fseek(fp, -szlen, SEEK_CUR);
			break;
		}
	}
	if (ret > 0)
		return *size;
	return 0;
}

static int get_next_audio_frame (FILE *fp, uint8_t **buff, int *size)
{
	int ret;
#define AUDIO_FRAME_SIZE 320
	if (!(*buff)) {
		*buff = (uint8_t*)malloc(AUDIO_FRAME_SIZE);
		if (!(*buff))
			return -1;
	}

	ret = fread(*buff, 1, AUDIO_FRAME_SIZE, fp);
	if (ret > 0) {
		*size = ret;
		return ret;
	}
	return 0;
}

#ifdef __WIN32__
#define FILE_VIDEO  "..\\BarbieGirl.h264"
#define FILE_AUDIO  "..\\BarbieGirl.alaw"
#else
#define FILE_VIDEO  "BarbieGirl.h264"
#define FILE_AUDIO  "BarbieGirl.alaw"
#endif

static void* _FileChannelThd(void* arg)
{
	MgrChnnelData* pChannel = (MgrChnnelData*)arg;
	Easy_U32 channelID = pChannel->m_iChannelID;
	FILE* vfp = NULL;
	FILE* afp = NULL;
	uint8_t* vbuf = NULL;
	uint8_t* vbuf1 = NULL;
	uint8_t* abuf = NULL;
	int vsize = 0, vsize1 = 0, asize = 0;
	uint64_t ts;
	int ret;
	
	vfp = fopen(FILE_VIDEO, "rb");
	afp = fopen(FILE_AUDIO, "rb");
    if (!vfp || !afp) {
        err("fopen %s(%p) or %s(%p) failed", FILE_VIDEO, vfp, FILE_AUDIO, afp);
        if (vfp)
            fclose(vfp);
        if (afp)
            fclose(afp);
        return NULL;
    }

	ts = os_get_reltime();
	vbuf1 = (uint8_t*)malloc(2*1024*1024);

	while (pChannel->m_bCanRun) {
		uint8_t type = 0;
		vsize1 = 0;
read_video_again:
		ret = get_next_video_frame(vfp, &vbuf, &vsize);
		if (ret < 0) {
			fprintf(stderr, "get_next_video_frame failed\n");
			break;
		}
		if (ret == 0) {
			fseek(vfp, 0, SEEK_SET);
			fseek(afp, 0, SEEK_SET);
			continue;
		}

		memcpy(vbuf1 + vsize1, vbuf, vsize);
		vsize1 += vsize;

		type = 0;
		if (vbuf[0] == 0 && vbuf[1] == 0 && vbuf[2] == 1) {
			type = vbuf[3] & 0x1f;
		}
		if (vbuf[0] == 0 && vbuf[1] == 0 && vbuf[2] == 0 && vbuf[3] == 1) {
			type = vbuf[4] & 0x1f;
		}
		if (type != 5 && type != 1) {
			goto read_video_again;
		}

		RTMPMgr_PushVideo(g_hMgr, channelID, vbuf1, vsize1, (Easy_U32)(ts/1000));

		ret = get_next_audio_frame(afp, &abuf, &asize);
		if (ret < 0) {
			fprintf(stderr, "get_next_audio_frame failed\n");
			break;
		}
		if (ret == 0) {
			fseek(vfp, 0, SEEK_SET);
			fseek(afp, 0, SEEK_SET);
			continue;
		}

		RTMPMgr_PushAudio(g_hMgr, channelID, abuf, asize, (Easy_U32)(ts/1000));

        do {
            usleep(20000);
        } while (os_get_reltime() - ts < 1000000 / 25);
		if (ret < 0)
			break;
		ts += 1000000 / 25;
//		printf(".");fflush(stdout);
	}

	free(vbuf1);
	free(vbuf);
	free(abuf);
	fclose(afp);
	fclose(vfp);
	return NULL;
}

static Easy_I32 _FileChannelStart (Easy_U32 channelID, EASY_MEDIA_INFO_T* mediaInfo, void* _user_data)
{
	MgrChnnelData* pChannel = NULL;
	dbg("Channel %u starting", channelID);

	if (channelID >= MAX_BASE_CHN_NUM) {
		return -1;
	}
	pChannel = &g_stChannels[channelID];
	pChannel->m_iChannelID = channelID;
	pChannel->m_bCanRun = 1;

    if (access(FILE_VIDEO, 0) || access(FILE_AUDIO, 0)) {
        err("%s or %s is not exist", FILE_VIDEO, FILE_AUDIO);
        return -1;
    }

	pthread_create(&pChannel->m_hPushThd, NULL, _FileChannelThd, pChannel);

	memset(mediaInfo, 0, sizeof(*mediaInfo));
	mediaInfo->u32VideoCodec = EASY_SDK_VIDEO_CODEC_H264;
	mediaInfo->u32VideoFps = 25;
	mediaInfo->u32AudioCodec = EASY_SDK_AUDIO_CODEC_G711A;
	mediaInfo->u32AudioSamplerate = 8000;
	mediaInfo->u32AudioChannel = 1;
	mediaInfo->u32AudioBitsPerSample = 16;

	dbg("Channel %u started", channelID);
	return 0;
}

static void _FileChannelStop(Easy_U32 channelID, void* _user_data)
{
	MgrChnnelData* pChannel = NULL;
	dbg("Channel %u stopping", channelID);

	if (channelID >= MAX_BASE_CHN_NUM) {
		return;
	}
	pChannel = &g_stChannels[channelID];

	pChannel->m_bCanRun = 0;
	pthread_join(pChannel->m_hPushThd, NULL);
	dbg("Channel %u stoped", channelID);
}

#else

static HI_S32 NETSDK_APICALL _HiChannelOnStreamCallback(HI_U32 u32Handle,
	HI_U32 u32DataType,	HI_U8* pu8Buffer, HI_U32 u32Length, HI_VOID* pUserData)
{
	MgrChnnelData* pChannel = (MgrChnnelData*) pUserData;
    HI_S_AVFrame* pstruAV = HI_NULL;
	HI_S_SysHeader* pstruSys = HI_NULL;
	
	if (u32DataType == HI_NET_DEV_AV_DATA)
	{
		pstruAV = (HI_S_AVFrame*)pu8Buffer;

		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_VIDEO_FRAME_FLAG)
		{
			if(pstruAV->u32AVFrameLen > 0)
			{
				unsigned char* pbuf = (unsigned char*)(pu8Buffer+sizeof(HI_S_AVFrame));
				RTMPMgr_PushVideo(g_hMgr, pChannel->m_iChannelID, pbuf, pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS);
			}	
		}
		else
		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_AUDIO_FRAME_FLAG)
		{
			if(pstruAV->u32AVFrameLen > 4) {
				unsigned char* pbuf = (unsigned char*)(pu8Buffer+sizeof(HI_S_AVFrame)) + 4;
				RTMPMgr_PushAudio(g_hMgr, pChannel->m_iChannelID, pbuf, pstruAV->u32AVFrameLen - 4, pstruAV->u32AVFramePTS);
			}	
		}
	}
	else
	if (u32DataType == HI_NET_DEV_SYS_DATA)
	{
		pstruSys = (HI_S_SysHeader*)pu8Buffer;
		printf("Video W:%u H:%u Audio: %u \n", pstruSys->struVHeader.u32Width, pstruSys->struVHeader.u32Height, pstruSys->struAHeader.u32Format);
	}
	return HI_SUCCESS;
}

static const char* ConfigUName	= "admin";			//SDK UserName
static const char* ConfigPWD	= "admin";			//SDK Password
static const char* ConfigDHost	= "192.168.1.88";	//SDK Host
static const char* ConfigDPort	= "80";				//SDK Port

static Easy_I32 _HiChannelStart (Easy_U32 channelID, EASY_MEDIA_INFO_T* mediaInfo, void* _user_data)
{
	MgrChnnelData* pChannel = NULL;
	HI_S_STREAM_INFO struStreamInfo;
	HI_S32 s32Ret = HI_SUCCESS;

	dbg("Channel %u starting", channelID);

	if (channelID >= MAX_BASE_CHN_NUM) {
		return -1;
	}
	pChannel = &g_stChannels[channelID];
	pChannel->m_iChannelID = channelID;

	s32Ret = HI_NET_DEV_Login(&pChannel->m_hHiChannelHandle, ConfigUName, ConfigPWD, ConfigDHost, atoi(ConfigDPort));
	if (HI_SUCCESS != s32Ret) {
		printf("HI_NET_DEV_Login fail");
		return -1;
	}

	HI_NET_DEV_SetStreamCallBack(pChannel->m_hHiChannelHandle, _HiChannelOnStreamCallback, pChannel);

	struStreamInfo.u32Channel = HI_NET_DEV_CHANNEL_1;
	struStreamInfo.blFlag = (channelID % 2) == 0 ? HI_TRUE : HI_FALSE;
	struStreamInfo.u32Mode = HI_NET_DEV_STREAM_MODE_TCP;
	struStreamInfo.u8Type = HI_NET_DEV_STREAM_ALL;

	s32Ret = HI_NET_DEV_StartStream(pChannel->m_hHiChannelHandle, &struStreamInfo);
	if (HI_SUCCESS != s32Ret) {
		printf("HI_NET_DEV_StartStream fail");
		HI_NET_DEV_Logout(pChannel->m_hHiChannelHandle);
		return -1;
	}

	memset(mediaInfo, 0, sizeof(*mediaInfo));
	mediaInfo->u32VideoCodec = EASY_SDK_VIDEO_CODEC_H264;
	mediaInfo->u32VideoFps = 25;
	mediaInfo->u32AudioCodec = EASY_SDK_AUDIO_CODEC_G711A;
	mediaInfo->u32AudioSamplerate = 8000;
	mediaInfo->u32AudioChannel = 1;
	mediaInfo->u32AudioBitsPerSample = 16;

	dbg("Channel %u started", channelID);
	return 0;
}

static void _HiChannelStop(Easy_U32 channelID, void* _user_data)
{
	MgrChnnelData* pChannel = NULL;
	dbg("Channel %u stopping", channelID);

	if (channelID >= MAX_BASE_CHN_NUM) {
		return;
	}
	pChannel = &g_stChannels[channelID];

	HI_NET_DEV_StopStream(pChannel->m_hHiChannelHandle);
	HI_NET_DEV_Logout(pChannel->m_hHiChannelHandle);
	dbg("Channel %u stoped", channelID);
}
#endif

#ifdef __WIN32__
#define INI_FILE    "..\\rtmpcam.ini"
#else
#define INI_FILE    "rtmpcam.ini"
#endif

int main(int argc, char *argv[])
{
#ifdef USE_FILE_STREAM
	RTMPMgr_Callback cb = {_FileChannelStart, _FileChannelStop};
#else
	RTMPMgr_Callback cb = {_HiChannelStart, _HiChannelStop};
	HI_NET_DEV_Init();
#endif

	g_hMgr = RTMPMgr_Start(INI_FILE, cb, NULL);
	if (NULL == g_hMgr) {
		fprintf(stderr, "RTMPMgr_Start failed\n");
		getchar();
		return -1;
	}

	printf("Press Enter to exit...\n");
	getchar();

	RTMPMgr_Stop(g_hMgr);

#ifdef USE_FILE_STREAM
#else
	HI_NET_DEV_DeInit();
#endif

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif
	getchar();
	return 0;
}
