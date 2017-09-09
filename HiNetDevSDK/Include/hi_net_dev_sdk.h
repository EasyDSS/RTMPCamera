#ifndef _HI_NET_DEV_SDK_H_
#define _HI_NET_DEV_SDK_H_

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif 

#ifdef HI_OS_LINUX
#define NETSDK_API
#define NETSDK_APICALL 
#elif defined NETLIB_LIB
#define NETSDK_API
#define NETSDK_APICALL 
#else
#define NETSDK_API	__declspec(dllexport)
#define NETSDK_APICALL  __stdcall
#endif   

/* ��������������־ */ 
#define HINET_IN
#define HINET_OUT

#define HI_GET 0
#define HI_SET 1

#define HI_CONTINUE 0
#define HI_STEP		1

/* �ַ����ĳ��� */
#define HI_NET_DEV_STR_LEN 64

/* �豸 ͨ���ţ�Ŀǰ��֧��һ��ͨ��*/
#define HI_NET_DEV_CHANNEL_1    1
//#define HI_NET_DEV_CHANNEL_2    2
//#define HI_NET_DEV_CHANNEL_3    3
//#define HI_NET_DEV_CHANNEL_4    4

#define HI_NET_DEV_STREAM_1    0
#define HI_NET_DEV_STREAM_2    1
#define HI_NET_DEV_STREAM_3    2

/*  ������������ģʽ��Ŀǰ��֧��TCP*/
#define HI_NET_DEV_STREAM_MODE_TCP  0
//#define HI_NET_DEV_STREAM_MODE_UDP  1

/*���������ͣ�Ŀǰ��֧���������� */
/*��������֧�ֱ������ݺ��������� */
#define HI_NET_DEV_STREAM_VIDEO_ONLY   0x01
#define HI_NET_DEV_STREAM_AUDIO_ONLY   0x02
#define HI_NET_DEV_STREAM_VIDEO_AUDIO 0x03
/*Ŀǰ��֧�ֽ����䱨�����ݺ��������� */
#define HI_NET_DEV_STREAM_DATA_ONLY    0x04
#define HI_NET_DEV_STREAM_VIDEO_DATA   0x05
#define HI_NET_DEV_STREAM_AUDIO_DATA   0x06
#define HI_NET_DEV_STREAM_ALL                  0x07

#define HI_NET_DEV_VIDEO_AVI 0
#define HI_NET_DEV_VIDEO_ASF 1
#define HI_NET_DEV_VIDEO_264 2

/* ��ʼ������ */
typedef struct 
{
	HI_U32 u32Channel;      /*ͨ���� */
	HI_BOOL blFlag;            /*HI_TRUE:������������HI_FALSE:���Ӵ�����*/
	HI_U32 u32Mode;         /*��������ģʽ*/
    HI_U8 u8Type;         /*���������ͣ���Ƶ����Ƶ����������*/
} HI_S_STREAM_INFO;

typedef struct 
{
	HI_U32 u32Channel;      /*ͨ���� */
	HI_U32 u32Stream;            /*0:������������1:���Ӵ����� 2:��������*/
	HI_U32 u32Mode;         /*��������ģʽ*/
    HI_U8 u8Type;         /*���������ͣ���Ƶ����Ƶ����������*/
} HI_S_STREAM_INFO_EXT;

/* ���������¼��ص�*/
#define HI_NET_DEV_CONNECTING                       0 /* �������� */
#define HI_NET_DEV_CONNECTED                        1 /* ���ӳɹ� */
#define HI_NET_DEV_CONNECT_FAILED					2 /* ����ʧ�� */
#define HI_NET_DEV_ABORTIBE_DISCONNECTED			3 /* �����ж� */
#define HI_NET_DEV_NORMAL_DISCONNECTED				4 /* �Ͽ����� */
#define HI_NET_DEV_RECONNECTING	                    5
#define HI_NET_DEV_RECORD_START	                    6
#define HI_NET_DEV_RECORD_STOP	                    7

typedef HI_S32 (NETSDK_APICALL *HI_ON_EVENT_CALLBACK)(HI_U32 u32Handle, /* ��� */
                                HI_U32 u32Event,      /* �¼� */
                                HI_VOID* pUserData  /* �û�����*/
                                );

/*�����ý�����ݻص� */
#define HI_NET_DEV_AV_DATA   0	/* ����Ƶ���� */
#define HI_NET_DEV_SYS_DATA  1  /* ϵͳ���� */
#define HI_NET_DEV_FR_DATA   2  /* ֡�� */

#define HI_NET_DEV_VIDEO_FRAME_FLAG 0x46565848	/* ��Ƶ֡��־ */
#define HI_NET_DEV_AUDIO_FRAME_FLAG 0x46415848  /* ��Ƶ֡��־ */

#define HI_NET_DEV_VIDEO_FRAME_I    1 /* �ؼ�֡ */
#define HI_NET_DEV_VIDEO_FRAME_P    2 /* �ǹؼ�֡ */
typedef struct 
{
    HI_U32 u32AVFrameFlag;               /* ֡��־*/
    HI_U32 u32AVFrameLen;          /* ֡�ĳ���*/
    HI_U32 u32AVFramePTS;        /*ʱ��� */
    HI_U32 u32VFrameType;       /* ��Ƶ�����ͣ�I֡��P֡*/
} HI_S_AVFrame;

typedef struct 
{
    HI_U32 u32FrameRate;               /* ֡��־*/
} HI_S_FrameRate;

/* ϵͳͷ */
#define HI_NET_DEV_SYS_FLAG 0x53565848
typedef struct
{
   HI_U32 u32Width;		/* ��Ƶ�� */
   HI_U32 u32Height;    /* ��Ƶ�� */
} HI_S_VideoHeader;

typedef struct
{
    HI_U32 u32Format;       /*��Ƶ��ʽ*/
} HI_S_AudioHeader;

typedef struct 
{
   HI_U32 u32SysFlag;
   HI_S_VideoHeader struVHeader;
   HI_S_AudioHeader struAHeader;
} HI_S_SysHeader;

typedef HI_S32 (NETSDK_APICALL *HI_ON_STREAM_CALLBACK)(HI_U32 u32Handle, /* ��� */
                                HI_U32 u32DataType,     /* �������ͣ�ϵͳ���ݻ�����Ƶ���� */
                                HI_U8*  pu8Buffer,      /* ���ݰ���֡ͷ */
                                HI_U32 u32Length,      /* ���ݳ��� */
                                HI_VOID* pUserData    /* �û�����*/
                                );

#define NALU_TYPE_VIDEO 0
#define NALU_TYPE_AUDIO 1
#define NALU_TYPE_DATA  2
#define NALU_TYPE_SYS   3
#define NALU_TYPE_FRATE 4
typedef HI_S32 (NETSDK_APICALL *HI_ON_NALU_CALLBACK)(HI_U32 u32Handle, /* ��� */
								HI_U32 u32DataType,     /* �������ͣ�ϵͳ���ݻ�����Ƶ���� */
								HI_U8*  pu8Buffer,      /* ���ݰ���֡ͷ */
								HI_U32 u32Length,      /* ���ݳ��� */
								HI_U32 u32Pts,      /* ���ݳ��� */
								HI_VOID* pUserData    /* �û�����*/
                                );

#define HI_NET_DEV_MOTION_DETECTION	0	/* �ƶ�����־ */
#define HI_NET_DEV_INPUT_ALARM	 	1   /* ���ñ�����־ */
#define HI_NET_DEV_KEEP_ALIVE		2	// keep alive
#define HI_NET_DEV_DEC_STATE		3	// decoder state
#define HI_NET_DEV_AUDIO_ALARM		4	// audio alarm

/*�ƶ���� */
#define HI_NET_DEV_MOTION_AREA_MAX	4
#define HI_NET_DEV_MOTION_AREA_1    1
#define HI_NET_DEV_MOTION_AREA_2    2
#define HI_NET_DEV_MOTION_AREA_3    3
#define HI_NET_DEV_MOTION_AREA_4    4
typedef struct 
{
	HI_U32 u32Area;
	HI_U32 u32X;
	HI_U32 u32Y;
	HI_U32 u32Width;
	HI_U32 u32Height;
} HI_S_ALARM_MD;

typedef HI_S32 (NETSDK_APICALL *HI_ON_DATA_CALLBACK)(HI_U32 u32Handle, /* ��� */
                                HI_U32 u32DataType,       /* ��������*/
                                HI_U8*  pu8Buffer,      /* ���� */
                                HI_U32 u32Length,      /* ���ݳ��� */
                                HI_VOID* pUserData    /* �û�����*/
                                );

/* �豸������ */
/* Product and Vendor */
typedef struct HI_ProductVendor
{
	HI_CHAR sProduct[32];	//��Ʒ����ID
	HI_CHAR	sVendor[32];		//��������ID	
}HI_S_ProductVendor;

/* ͼ��������չ���ܣ���CMOS��ͷ��Ч*/
#define HI_NET_DEV_SCENE_AUTO		0   /* �Զ�ģʽ */
#define HI_NET_DEV_SCENE_INDOOR	1   /* ����ģʽ */
#define HI_NET_DEV_SCENE_OUTDOOR	2   /* ����ģʽ */
typedef struct HI_Display_Ext
{
	HI_BOOL blFlip;         /* ���·�ת */
	HI_BOOL blMirror;     /* ���ҷ�ת*/
	HI_S32 s32Scene;
} HI_S_Display_Ext;

/* ͼ��������� */
typedef struct HI_Display
{
	HI_U32 u32Brightness;   /*  ����  */
	HI_U32 u32Saturation;   /* ���Ͷ�  */
	HI_U32 u32Contrast;     /*  �Աȶ�  */
	HI_U32 u32Hue;          /*  ɫ��  */
} HI_S_Display;

typedef struct HI_Sharpness
{
	HI_U32 u32Sharpness;   /*  ���  */
} HI_S_Sharpness;

typedef struct HI_Color
{
	HI_U32 u32Brightness;   /*  ����  */
	HI_U32 u32Saturation;   /* ���Ͷ�  */
	HI_U32 u32Contrast;     /*  �Աȶ�  */
	HI_U32 u32Hue;          /*  ɫ��  */
	HI_U32 u32Shutter;      /* ���� */
	HI_U32 u32Dnt;			/* ������ */
	HI_U32 u32Lumivalue;	/* ����� */
} HI_S_Color;

/* ����ƿ��� */
#define HI_NET_DEV_INFRARED_AUTO	       0    /* �Զ����� */
#define HI_NET_DEV_INFRARED_ON		1   /* ǿ�ƿ��� */
#define HI_NET_DEV_INFRARED_OFF		2   /* ǿ�ƹر�*/
typedef struct HI_Infrared
{
	HI_S32 s32Infrared;
} HI_S_Infrared;

/* ��Ƶ���� */
typedef struct HI_Video
{
	HI_U32 u32Channel;      /* ͨ�� */
       HI_BOOL blFlag;          /* ����������  */
	HI_U32 u32Bitrate;	/* ���� */
	HI_U32 u32Frame;    /* ֡�� */
	HI_U32 u32Iframe;	/* ��֡���1-300 */
	HI_BOOL blCbr;	    /* ����ģʽ��HI_TRUEΪ�̶�������HI_FALSEΪ�ɱ����� */		
	HI_U32 u32ImgQuality;	/* ��������1-6 */
} HI_S_Video;

/* ��Ƶ���� */
typedef struct HI_VideoEx
{
	HI_U32 u32Channel;      /* ͨ�� */
	HI_BOOL blFlag;          /* ����������  */
	HI_U32 u32Bitrate;	/* ���� */
	HI_U32 u32Frame;    /* ֡�� */
	HI_U32 u32Iframe;	/* ��֡���1-300 */
	HI_BOOL blCbr;	    /* ����ģʽ��HI_TRUEΪ�̶�������HI_FALSEΪ�ɱ����� */		
	HI_U32 u32ImgQuality;	/* ��������1-6 */
	HI_U32 u32Width;
	HI_U32 u32Height;
} HI_S_VideoEx;

typedef struct HI_Video_Ext
{
	HI_U32 u32Channel;      /* ͨ�� */
	HI_U32 u32Stream;          /* ����������  */
	HI_U32 u32Bitrate;	/* ���� */
	HI_U32 u32Frame;    /* ֡�� */
	HI_U32 u32Iframe;	/* ��֡���1-300 */
	HI_BOOL blCbr;	    /* ����ģʽ��HI_TRUEΪ�̶�������HI_FALSEΪ�ɱ����� */		
	HI_U32 u32ImgQuality;	/* ��������1-6 */
	HI_U32 u32Width;
	HI_U32 u32Height;
} HI_S_Video_Ext;

/* ͼ�������� */
#define HI_NET_DEV_RESOLUTION_VGA		0
#define HI_NET_DEV_RESOLUTION_QVGA		1
#define HI_NET_DEV_RESOLUTION_QQVGA 	2
#define HI_NET_DEV_RESOLUTION_D1		3
#define HI_NET_DEV_RESOLUTION_CIF		4
#define HI_NET_DEV_RESOLUTION_QCIF		5
#define HI_NET_DEV_RESOLUTION_720P		6
#define HI_NET_DEV_RESOLUTION_Q720		7
#define HI_NET_DEV_RESOLUTION_QQ720		8
#define HI_NET_DEV_RESOLUTION_UXGA		9
#define HI_NET_DEV_RESOLUTION_960H		10
#define HI_NET_DEV_RESOLUTION_Q960H		11
#define HI_NET_DEV_RESOLUTION_QQ960H	12
#define HI_NET_DEV_RESOLUTION_1080P		13
#define HI_NET_DEV_RESOLUTION_960P		14
typedef struct HI_Resolution
{
	HI_U32 u32Channel;      /* ͨ�� */
       HI_BOOL blFlag;            /* ���������� HI_TRUE:��������HI_FALSE:������ */ 
	HI_U32 u32Resolution;
} HI_S_Resolution;

/* ��Ƶ��ʽ��Ƶ��  */
typedef enum HI_Frequency
{
	FREQ_50HZ_PAL = 50,
	FREQ_60HZ_NTSC = 60
} HI_E_Frequency;

/* ��Ƶ��ʽ */
#define HI_NET_DEV_AUDIO_TYPE_G711		0 /* g711a */
#define HI_NET_DEV_AUDIO_TYPE_G726		1
#define HI_NET_DEV_AUDIO_TYPE_AMR		2 /* AMR NB 12.2kbps */
typedef struct HI_Audio
{
	HI_U32 u32Channel;              /* ͨ�� */
       HI_BOOL blFlag;                      /*����������־��HI_TRUEΪ��������HI_FALSEΪ������*/
	HI_BOOL blEnable;               /* HI_TRUE:������Ƶ��HI_FALSE:�ر���Ƶ  */
	HI_U32 u32Type;
} HI_S_Audio;

typedef struct HI_Audio_Ext
{
	HI_U32 u32Channel;              /* ͨ�� */
	HI_U32 u32Stream;                      /*����������־��0Ϊ��������1Ϊ��������2Ϊ��������*/
	HI_BOOL blEnable;               /* HI_TRUE:������Ƶ��HI_FALSE:�ر���Ƶ  */
	HI_U32 u32Type;
} HI_S_Audio_Ext;

typedef enum HI_AudioInput
{
	AUDIO_INPUT_MIC = 100,	/* ������� */
	AUDIO_INPUT_LINE = 10  /* �������� */
} HI_E_AudioInput;

typedef struct HI_AudioVolume
{
	HI_U32 u32AudioVolume;
} HI_S_AudioVolume;

/* OSD */
typedef struct HI_OSD
{
	HI_BOOL blEnTime;	/* HI_TRUE��ʾ����ʱ��OSD��HI_FALSE ����ʾ */
	HI_BOOL blEnName;	/* HI_TRUE��ʾ����OSD��HI_FALSE ����ʾ */
	HI_CHAR sName[64];	/* OSD���� */
} HI_S_OSD;

typedef struct HI_MD_PARAM
{
       HI_U32 u32Channel;       /* ͨ�� */
	HI_U32 u32Area;            /* �������� */
       HI_BOOL blEnable;        /* HI_TRUE����������HI_FALSE�رձ���   */
       HI_U32 u32Sensitivity;  /* ���������ȣ�ȡֵ��Χ1~99 */
	HI_U32 u32X;
	HI_U32 u32Y;
	HI_U32 u32Width;
	HI_U32 u32Height;    
} HI_S_MD_PARAM;

/* ��̨������������*/
#define HI_NET_DEV_PTZ_PRO_PELCOD	0
#define HI_NET_DEV_PTZ_PRO_PELCOP	1
#define HI_NET_DEV_PTZ_B110		110
#define HI_NET_DEV_PTZ_B300		300
#define HI_NET_DEV_PTZ_B1200		1200
#define HI_NET_DEV_PTZ_B2400		2400
#define HI_NET_DEV_PTZ_B4800		4800
#define HI_NET_DEV_PTZ_B9600		9600
#define HI_NET_DEV_PTZ_B19200		19200
#define HI_NET_DEV_PTZ_B38400		38400
#define HI_NET_DEV_PTZ_B57600		57600
#define HI_NET_DEV_PTZ_STOP_1		1
#define HI_NET_DEV_PTZ_STOP_2		2
#define HI_NET_DEV_PTZ_DATA_5		5
#define HI_NET_DEV_PTZ_DATA_6		6
#define HI_NET_DEV_PTZ_DATA_7		7
#define HI_NET_DEV_PTZ_DATA_8		8
#define HI_NET_DEV_PTZ_PARITY_NONE	0
#define HI_NET_DEV_PTZ_PARITY_ODD	       1
#define HI_NET_DEV_PTZ_PARITY_EVEN	2
typedef struct HI_PTZ
{
	HI_U32 u32Protocol;     /* ��̨Э��  */
	HI_U32 u32Address;     /* ��ַ��1-255 */ 
	HI_U32 u32Baud;          /* ������*/
	HI_U32 u32DataBit;      /* ��ַλ*/     
	HI_U32 u32StopBit;      /* ֹͣλ*/
	HI_U32 u32Parity;       /* У��λ*/
} HI_S_PTZ;


#define SYSINFO_MAX_STRINGLENGTH (40)		// ϵͳ��Ϣ����
#define SYSINFO_MAX_VERLENGTH (64)

typedef struct tagHI_NETINFO
{   
	HI_CHAR aszServerIP[SYSINFO_MAX_STRINGLENGTH];       /*IP��ַ*/
	HI_CHAR aszNetMask[SYSINFO_MAX_STRINGLENGTH];        /*��������*/
	HI_CHAR aszGateWay[SYSINFO_MAX_STRINGLENGTH];        /*����*/
	HI_CHAR aszMacAddr[SYSINFO_MAX_STRINGLENGTH];        /*MAC ��ַ*/
	HI_CHAR aszFDNSIP[SYSINFO_MAX_STRINGLENGTH];         /*first DNSIP*/
	HI_CHAR aszSDNSIP[SYSINFO_MAX_STRINGLENGTH];         /*DNSIP*/
	HI_S32  s32DhcpFlag;                                 /*DHCP*/
	HI_S32  s32DnsDynFlag;                               /*DNS ��̬�����ʶ*/
}HI_S_NETINFO, *PHI_S_NETINFO;

// �豸��Ϣ
typedef struct tagHI_DEVICE_INFO
{   
	HI_CHAR aszServerSerialNumber[SYSINFO_MAX_STRINGLENGTH + 1];	/*�豸���к�*/
	//HI_CHAR aszServerHardVersion[SYSINFO_MAX_VERLENGTH + 1];		/*Ӳ���汾*/
    HI_CHAR aszServerSoftVersion[SYSINFO_MAX_VERLENGTH + 1];		/*����汾*/
    HI_CHAR aszServerName[SYSINFO_MAX_STRINGLENGTH + 1];			/*����������*/
    HI_CHAR aszServerModel[SYSINFO_MAX_STRINGLENGTH + 1];			/*�ͺ�*/
    HI_CHAR aszStartDate[SYSINFO_MAX_STRINGLENGTH + 1];				/*ϵͳ��������ʱ��*/
	//HI_CHAR aszTimes[SYSINFO_MAX_STRINGLENGTH + 1];				/*ϵͳ������ʱ��,��λ:����.uptime*/
	//HI_CHAR aszServerInterface[48 + 1];							/*�ӿ����ͺ�����*/ //������fengjf
	HI_S32 s32ConnectState;											/*��������״̬*/
}HI_DEVICE_INFO, *PHI_DEVICE_INFO;

typedef struct tagHI_SYSTEM_INFO
{   
    HI_CHAR aszSystemModel[SYSINFO_MAX_STRINGLENGTH];
    HI_CHAR aszSystemSoftVersion[SYSINFO_MAX_VERLENGTH];
    HI_CHAR aszSystemName[SYSINFO_MAX_STRINGLENGTH];
    HI_CHAR aszStartDate[SYSINFO_MAX_STRINGLENGTH];
	HI_S32  s32SDStatus;
	HI_S32  s32SDFreeSpace;
	HI_S32  s32SDTotalSpace;
}HI_SYSTEM_INFO, *PHI_SYSTEM_INFO;

typedef struct tagHI_SYSDEVICE{
	HI_S_NETINFO netInfo;			/*���������Ϣ*/
	HI_DEVICE_INFO deviceInfo;		/*�豸������Ϣ*/
}HI_S_SYSDEVICE;


typedef HI_S32 (NETSDK_APICALL *OnUpgradeCallBack)(HI_U32 u32Chn, HI_S32 s32Persent, HI_VOID *pUserData);


typedef struct hiUPGRADE_INFO_S
{
	HI_CHAR sHost[32];
	HI_U32  u32Port;
	HI_CHAR sUser[16];
	HI_CHAR sPass[16];
	HI_CHAR sPath[256];
	HI_U32  u32Chn;
	HI_VOID *pUserData;
	OnUpgradeCallBack callBack;
}UPGRADE_INFO_S;


typedef struct HI_HTTPPORT
{
	HI_U32 u32HttpPort;
} HI_S_HTTPPORT;

#define HI_OSD_TIME 0
#define HI_OSD_NAME 1
typedef struct HI_OSD_EX
{
	HI_U32 u32Area;		/*����*/
	HI_U32 u32X;
	HI_U32 u32Y;
} HI_S_OSD_EX;

#define HI_NET_DEV_COVER_AREA_MAX  4
#define HI_NET_DEV_COVER_AREA_1    1
#define HI_NET_DEV_COVER_AREA_2    2
#define HI_NET_DEV_COVER_AREA_3    3
#define HI_NET_DEV_COVER_AREA_4    4

typedef struct HI_COVER_PARAM
{
	HI_U32 u32Area;            /* �ڵ����� */
	HI_BOOL bShow;
	HI_U32 u32X;
	HI_U32 u32Y;
	HI_U32 u32Width;
	HI_U32 u32Height;    
	HI_U32 u32Color;
} HI_S_COVER_PARAM;

typedef struct hiSERVERTIME_INFO_S
{
	HI_CHAR sTime[32];
} HI_S_SERVERTIME;

typedef struct hiSRVTIME_INFO_S
{
	HI_CHAR sTime[32];
	HI_CHAR sTimeZone[32];
	HI_U32 u32DstMode;
} HI_S_SRVTIME;

typedef struct HI_SNAPTIMER
{
	HI_U32 u32Interval;
	HI_U32 u32Operation;
	HI_U32 u32Enable;
}HI_S_SNAPTIMER;

typedef struct HI_EMAIL_PARAM
{
	HI_CHAR sServer[32];
	HI_U32  u32Port;
	HI_U32  u32Ssl;
	HI_U32  u32LoginType;
	HI_CHAR sUser[24];
	HI_CHAR sPass[24];
	HI_CHAR sFrom[32];
	HI_CHAR sTo[32];
	HI_CHAR sSubject[32];
	HI_CHAR sText[128];
}HI_S_EMAIL_PARAM;

typedef struct HI_FTP_PARAM
{
	HI_CHAR sServer[32];
	HI_U32  u32Port;
	HI_U32  u32Mode;
	HI_CHAR sUser[24];
	HI_CHAR sPass[24];
}HI_S_FTP_PARAM;

typedef struct HI_ATTR_EXT
{
	HI_U32  u32Enable;
	HI_U32  u32Flag;
}HI_S_ATTR_EXT;

typedef struct HI_PLATFORM
{
	HI_CHAR sServer[32];
	HI_U32  u32Port;
	HI_U32  u32Enable;
	HI_U32  u32Devid;
	HI_U32  u32MediaType;
	HI_U32  u32AlarmFlag;
}HI_S_PLATFORM;

typedef struct HI_DNS_PARAM
{
	HI_CHAR sServiceType[32];
	HI_CHAR sUser[24];
	HI_CHAR sPass[24];
	HI_CHAR sDomain[32];
	HI_U32  u32Enable;
}HI_S_DNS_PARAM;

typedef struct HI_OUR_DNS
{
	HI_CHAR sServer[32];
	HI_CHAR sUsername[32];
	HI_CHAR sPassword[32];
	HI_U32  u32Enable;
	HI_U32	u32Port;
	HI_CHAR sDomain[32];
}HI_S_OUR_DNS;

typedef struct HI_WIFI_PARAM
{
	HI_CHAR sSsID[32];
	HI_CHAR sKey[32];
	HI_U32 u32Enable;
	HI_U32 u32Auth;
	HI_U32 u32Enc;
	HI_U32 u32Mode;
}HI_S_WIFI_PARAM;

#define WIFI_NET_INFRA	0
#define WIFI_NET_ADHOC	1

#define WIFI_AUTH_NONE	0
#define WIFI_AUTH_WEP	1
#define WIFI_AUTH_WPA	2
#define WIFI_AUTH_WPA2	3

#define WIFI_ENC_TKIP	0
#define WIFI_ENC_AES	1

typedef struct HI_WFPT
{
	HI_CHAR sEssID[32];
	HI_S32 s32Chn;
	HI_S32 s32Rssi;
	HI_U32 u32Enc;
	HI_U32 u32Auth;
	HI_U32 u32Net;
}HI_S_WFPT;

#define MAX_WFPT 64
typedef struct HI_WIFI_INFO
{
	HI_S32 s32Num;
	HI_S_WFPT sWfPt[MAX_WFPT];
}HI_S_WIFI_INFO;

typedef struct HI_MD_TIMER
{
	HI_U32  u32Etm;
	HI_CHAR sWorkday[16];
	HI_CHAR sWeekend[16];
	HI_CHAR sWeek[7][16];
}HI_S_MD_TIMER;

typedef struct HI_NET_EXT
{
	HI_S_NETINFO  sNetInfo;
	HI_S_HTTPPORT sHttpPort;
}HI_S_NET_EXT;

typedef struct HI_RTSPINFO
{
	HI_U32 u32RtspPort;
	HI_U32 u32AuthFlag;
} HI_S_RTSPINFO;

typedef struct HI_USER
{
	HI_CHAR sUsername[32];
	HI_CHAR sPassword[32];
} HI_S_USER;

typedef struct HI_USERINFO
{
	HI_S_USER sUser[3];
} HI_S_USERINFO;

typedef struct HI_CHN_INFO
{
	HI_U32 u32Enable;
	HI_CHAR sHost[24];
	HI_BOOL bStream;
	HI_U32 u32Port;
	HI_U32 u32Chn;
	HI_CHAR sUsername[32];
	HI_CHAR sPassword[32];
}HI_S_CHN_INFO;

typedef struct hiNVR_CHN
{
	HI_CHAR sName[32];
	HI_S_CHN_INFO sChnInfo;
}HI_S_NVR_CHN;

typedef struct HI_CHN_STATE
{
	HI_U32 u32State;
	HI_U32 u32linkNum;
}HI_S_CHN_STATE;

typedef struct HI_DEVINFO
{
	HI_CHAR sHost[32];
	HI_U32 u32Port;
}HI_S_DEVINFO;

#define MAX_SEARCH_NUM 64
typedef struct HI_SEARCH_INFO
{
	HI_U32 u32Num;
	HI_S_DEVINFO sDevInfo[MAX_SEARCH_NUM];
}HI_S_SEARCH_INFO;

typedef struct HI_RECORD_INFO
{
	HI_BOOL bStream;
	HI_U32 u32SetupAlarm;
	HI_U32 u32InputAlarm;
	HI_U32 u32MdAlarm;
	HI_CHAR sRecInfo[7][48+1];
}HI_S_RECORD_INFO;

typedef struct HI_RECORD_SYS
{
	HI_U32 u32RecLen;
	HI_U32 u32AlarmLen;
	HI_U32 u32Cover;
	HI_U32 u32PlanRecFlag;
	HI_U32 u32PreRec;
	HI_U32 u32RecType;
	HI_U32 u32DiskRemain;
}HI_S_RECORD_SYS;


#define MAX_LOOP_CHN 36
typedef struct HI_LOOP_INFO
{
	HI_U32 u32LoopTime;
	HI_S_CHN_INFO  sChnInfo[MAX_LOOP_CHN];
}HI_S_LOOP_INFO;

typedef struct HI_TIME
{
	HI_U32  u32Year;
	HI_U32  u32Month;
	HI_U32  u32Day;
	HI_U32  u32Hour;
	HI_U32  u32Minute;
	HI_U32  u32Second;
}HI_S_TIME;


typedef struct HI_DEC_CHN_INFO
{
	HI_S_CHN_INFO  sChnInfo;
	HI_U32 u32DecState;
	HI_S_TIME sStartTime;
	HI_S_TIME sStopTime;
	HI_CHAR sFileName[128];
}HI_S_DEC_CHN_INFO;

typedef struct HI_REC_STATE
{
	HI_U32 u32link;
	HI_U32 u32Record;
}HI_S_REC_STATE;

typedef struct HiDISK
{
	HI_U32 u32Total;
	HI_U32 u32Free;
}HI_S_DISK;

#define MAX_DISK_NUM 20
typedef struct HI_DISK_INFO
{
	HI_S32 s32Num;
	HI_S_DISK sDisk[MAX_DISK_NUM];
}HI_S_DISK_INFO;

typedef struct HI_DISK_FORMAT
{
	HI_S32 s32DiskNum;
}HI_DISK_FORMAT;

typedef struct hiDEVID
{
	HI_CHAR sDevID[32];
}HI_S_DEVID;

typedef struct hiIMAGE
{
	HI_U32 u32Brightness;   //����
	HI_U32 u32Saturation;   //���Ͷ�
	HI_U32 u32Contrast;     //�Աȶ�
	HI_U32 u32Hue;          //ɫ��
	HI_U32 u32Shutter;		//����
	HI_U32 u32Dnt;			//ɫ��������
	HI_U32 u32Lumivalue;	//���ն�
	HI_U32 u32Sensor;		//���� 1--9d131��2--2643 
	HI_BOOL bFlip;			//���·�ת
	HI_BOOL bMirror;		//���ҷ�ת
	HI_BOOL bNight;			//ҹ��ģʽ
	HI_BOOL bWdr;			//��̬����
	HI_BOOL bLumiswitch;	//���նȿ���
} HI_S_IMAGE;






typedef enum{
	PICTURE_NUM_1 = 1,
	PICTURE_NUM_4 = 4,
	PICTURE_NUM_9 = 9,
	PICTURE_NUM_BUTT
}PICTURE_NUM_E;

typedef enum{  
	DISPLAY_TYPE_VGA = 0,
	DISPLAY_TYPE_CVBS = 2,  
	DISPLAY_TYPE_BUTT
}DISPLAY_TYPE_E;

typedef enum{  
	DISPLAY_MODE_PAL = 25,
	DISPLAY_MODE_NTSC= 30,  
	DISPLAY_MODE_BUTT
}DISPLAY_MODE_E;

typedef enum{
	VIDEO_MODE_PAL           = 0,
	VIDEO_MODE_NTSC          = 1,
		
	VIDEO_MODE_1280x1024_60  = 9,
	VIDEO_MODE_1366x768_60   = 10,
	VIDEO_MODE_BUTT
}VIDEO_MODE_E;/*CVBS[0-1],VGA[9-10]*/

typedef enum{
	DISPLAY_FLAG_AUTO = 0,
	DISPLAY_FLAG_FIXED = 1,
	DISPLAY_FLAG_BUTT
}DISPLAY_FLAG_E;


typedef struct hiDISPLAY_CFG
{
	VIDEO_MODE_E	eVideoMode;
	PICTURE_NUM_E	ePictureNum;
    DISPLAY_TYPE_E	eDpyType;
    DISPLAY_MODE_E	eDpyMode;
    DISPLAY_FLAG_E	eDpyFlag;
} HI_S_DISPLAY_CFG;

typedef struct hiFrameHeader
{
	HI_U32 u32AVFrameFlag;      /* ֡��־*/
    HI_U32 u32AVFrameLen;       /* ֡�ĳ���*/
    HI_U32 u32AVFramePTS;       /*ʱ��� */
    HI_U32 u32VFrameType;       /* ��Ƶ�����ͣ�I֡��P֡*/
	HI_U32 u32Width;			/* ��Ƶ�� */
	HI_U32 u32Height;			/* ��Ƶ�� */
	HI_U32 u32Format;			/*��Ƶ��ʽ*/
} HI_S_FrameHeader;


/* �������� */
#define HI_NET_DEV_GET_PRODUCT_VENDOR			0x1000
#define HI_NET_DEV_CMD_DISPLAY					0x1001
#define HI_NET_DEV_CMD_DISPLAY_EXT				0x1002
#define HI_NET_DEV_CMD_INFRARED					0x1003
#define HI_NET_DEV_CMD_VIDEO_PARAM				0x1004
#define HI_NET_DEV_CMD_OSD_PARAM				0x1005
#define HI_NET_DEV_CMD_AUDIO_PARAM				0x1006
#define HI_NET_DEV_CMD_AUDIO_INPUT				0x1007
#define HI_NET_DEV_CMD_RESOLUTION				0x1008
#define HI_NET_DEV_CMD_FREQUENCY				0x1009
#define HI_NET_DEV_CMD_PTZ_PARAM				0x1010
#define HI_NET_DEV_CMD_MD_PARAM				    0x1011
#define HI_NET_DEV_CMD_NET_INFO					0x1012
#define HI_NET_DEV_CMD_HTTP_PORT				0x1013
#define HI_NET_DEV_CMD_DEVICE_INFO				0x1014
#define HI_NET_DEV_CMD_PRODUCTID				0x1015
#define HI_NET_DEV_CMD_USERNUM					0x1016
#define HI_NET_DEV_CMD_SERVER_TIME				0x1017
#define HI_NET_DEV_CMD_REBOOT					0x1018
#define HI_NET_DEV_CMD_RESET					0x1019
#define HI_NET_DEV_CMD_COVER_PARAM				0x1020
#define HI_NET_DEV_CMD_OSDEX_PARAM				0x1021
#define HI_NET_DEV_CMD_NET_EXT					0x1022
#define HI_NET_DEV_CMD_SNAP_TIMER				0x1023
#define HI_NET_DEV_CMD_EMAIL_PARAM				0x1024
#define HI_NET_DEV_CMD_FTP_PARAM				0x1025
#define HI_NET_DEV_CMD_ATTR_EXT					0x1026
#define HI_NET_DEV_CMD_MD_TIMER					0x1027
#define HI_NET_DEV_CMD_PLATFORM					0x1028
#define HI_NET_DEV_CMD_DNS_PARAM				0x1029
#define HI_NET_DEV_CMD_WIFI_PARAM				0x1030
#define HI_NET_DEV_CMD_WIFI_SEARCH				0x1031
#define HI_NET_DEV_CMD_VIDEO_PARAMEX			0x1032
#define HI_NET_DEV_CMD_USER						0x1033
#define HI_NET_DEV_CMD_SYSTEM_INFO				0x1034
#define HI_NET_DEV_CMD_WIFI_CHECK				0x1035
#define HI_NET_DEV_CMD_SERVER_TIME_EX			0x1036

#define HI_NET_DEV_CMD_COLOR					0x1040
#define HI_NET_DEV_CMD_SYS						0x1041
#define HI_NET_DEV_CMD_DEVID					0x1042
#define HI_NET_DEV_CMD_IMAGE_DEFAULT			0x1043
#define HI_NET_DEV_CMD_OUR_DNS					0x1044
#define HI_NET_DEV_CMD_VIDEO_PARAM_EXT			0x1047
#define HI_NET_DEV_CMD_AUDIO_PARAM_EXT			0x1048

#define HI_NET_NVR_CMD_NET_EXT					0x1050
#define HI_NET_NVR_CMD_RTSP_INFO				0x1051
#define HI_NET_NVR_CMD_USER						0x1052
#define HI_NET_NVR_CMD_CHANNEL_INFO				0x1053
#define HI_NET_NVR_CMD_CHANNEL_STATE			0x1054
#define HI_NET_NVR_CMD_SEARCH					0x1055
#define HI_NET_NVR_CMD_RECORD_INFO				0x1056
#define HI_NET_NVR_CMD_RECORD_SYS				0x1057
#define HI_NET_NVR_CMD_TIME						0x1058
#define HI_NET_NVR_CMD_RESET					0x1059
#define HI_NET_NVR_CMD_REBOOT					0x1060
#define HI_NET_NVR_CMD_RECORD_STATE				0x1061
#define HI_NET_NVR_CMD_DISK_INFO				0x1062
#define HI_NET_NVR_CMD_DISK_FORMAT				0x1063
#define HI_NET_NVR_CMD_RECORD_STATE_EX			0x1064

#define HI_NET_DEV_CMD_AUDIO_VOLUME_IN			0x1070
#define HI_NET_DEV_CMD_AUDIO_VOLUME_OUT			0x1071
#define HI_NET_DEV_CMD_BUT						0x1072

/* ��̨���ƶ��� */
#define HI_NET_DEV_CTRL_PTZ_STOP			0x3000
#define HI_NET_DEV_CTRL_PTZ_UP				0x3001
#define HI_NET_DEV_CTRL_PTZ_DOWN			0x3002
#define HI_NET_DEV_CTRL_PTZ_LEFT			0x3003
#define HI_NET_DEV_CTRL_PTZ_RIGHT			0x3004
#define HI_NET_DEV_CTRL_PTZ_ZOOMIN			0x3005
#define HI_NET_DEV_CTRL_PTZ_ZOOMOUT			0x3006
#define HI_NET_DEV_CTRL_PTZ_FOCUSIN			0x3007
#define HI_NET_DEV_CTRL_PTZ_FOCUSOUT		0x3008
#define HI_NET_DEV_CTRL_PTZ_APERTUREIN		0x3009
#define HI_NET_DEV_CTRL_PTZ_APERTUREOUT		0x3010

#define HI_NET_DEV_CTRL_PTZ_GOTO_PRESET	0x3015
#define HI_NET_DEV_CTRL_PTZ_SET_PRESET	0x3016
#define HI_NET_DEV_CTRL_PTZ_CLE_PRESET	0x3017

#define HI_NET_DEV_CTRL_PTZ_LIGHT_ON	0x3021
#define HI_NET_DEV_CTRL_PTZ_LIGHT_OFF	0x3022
#define HI_NET_DEV_CTRL_PTZ_WIPER_ON	0x3023
#define HI_NET_DEV_CTRL_PTZ_WIPER_OFF	0x3024
#define HI_NET_DEV_CTRL_PTZ_AUTO_ON		0x3025
#define HI_NET_DEV_CTRL_PTZ_AUTO_OFF	0x3026
#define HI_NET_DEV_CTRL_PTZ_HOME		0x3027
#define HI_NET_DEV_CTRL_PTZ_CRUISE_V	0x3028
#define HI_NET_DEV_CTRL_PTZ_CRUISE_H	0x3029

#define HI_NET_DEV_CTRL_PTZ_GSPEED			0x3050
#define HI_NET_DEV_CTRL_PTZ_SSPEED			0x3051

#define HI_NET_DEV_CTRL_PTZ_TRAN	0x4000

#define HI_NET_DEV_CTRL_PTZ_SPEED_MAX	0x3F
#define HI_NET_DEV_CTRL_PTZ_SPEED_MIN	0x00
#define HI_NET_DEV_CTRL_PTZ_PRESET_MAX	255
#define HI_NET_DEV_CTRL_PTZ_PRESET_MIN	0
#define HI_NET_DEV_CTRL_PTZ_FT_BUF_LEN	64

#define HI_NET_DEV_SNAP_BUF_LEN_MIN		1024

#define HI_NET_DEV_CTRL_DEC_PLAY			0x4000
#define HI_NET_DEV_CTRL_DEC_STOP			0x4001
#define HI_NET_DEV_CTRL_DEC_BUT				0x4002

/* ������ */
/* ��ʼ����ȥ��ʼ��ֻ�����һ��  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_Init();
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_DeInit();
/*  �û���¼ */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_Login(HINET_OUT HI_U32* pu32Handle, const HI_CHAR* psUsername, const HI_CHAR* psPassword, const HI_CHAR* psHost, HI_U16 u16Port);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_LoginExt(HINET_OUT HI_U32* pu32Handle, const HI_CHAR* psUsername, const HI_CHAR* psPassword, const HI_CHAR* psHost, HI_U16 u16Port, HI_U32 u32TimeOut);
/* �û��ǳ�*/
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_Logout(HI_U32 u32Handle);
/* �������ӳ�ʱʱ�䣬Ĭ�ϳ�ʱ��5�룬��λ�Ǻ���*/
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetConnectTimeout(HI_U32 u32Handle, HI_U32 u32Timeout);
/* �����Զ��������ʱ�䣬Ĭ��Ϊ10�룬0Ϊ����������λ�Ǻ��� */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetReconnect(HI_U32 u32Handle, HI_U32 u32Interval);
/* ��ʼ������ */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StartStream(HI_U32 u32Handle, HI_S_STREAM_INFO*  pstruStreamInfo);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StartStreamExt(HI_U32 u32Handle, HI_S_STREAM_INFO_EXT*  pstruStreamInfo);

/* ֹͣ������  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StopStream(HI_U32 u32Handle);
/* �����������¼��ص�  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetEventCallBack(HI_U32 u32Handle, HI_ON_EVENT_CALLBACK cbEventCallBack, HI_VOID* pUserData);
/* ��������Ƶ���ݻص�  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetStreamCallBack(HI_U32 u32Handle, HI_ON_STREAM_CALLBACK cbStreamCallBack, HI_VOID* pUserData);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetStreamCallBackEx(HI_U32 u32Handle, HI_ON_NALU_CALLBACK cbNaluCallBack, HI_VOID* pUserData);
/* ���ñ������ݻص�  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetDataCallBack(HI_U32 u32Handle, HI_ON_DATA_CALLBACK cbDataCallBack, HI_VOID* pUserData);
/* �������úͻ�ȡ */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetConfig(HI_U32 u32Handle, HI_U32 u32Command, HI_VOID* pBuf, HI_U32 u32BufLen);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetConfig(HI_U32 u32Handle, HI_U32 u32Command, HINET_OUT HI_VOID* pBuf, HI_U32 u32BufLen);
/* ��̨����*/
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_PTZ_Ctrl_Standard(HI_U32 u32Handle, HI_U32 u32Command, HI_U32 u32Speed);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_PTZ_Ctrl_StandardEx(HI_U32 u32Handle, HI_U32 u32Command);
/* ��̨Ԥ��λ */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_PTZ_Ctrl_Preset(HI_U32 u32Handle, HI_U32 u32Command, HI_U32 u32Preset);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_PTZ_Ctrl_Extend(HI_U32 u32Handle, HI_U32 u32Command);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_PTZ_Ctrl_AutoSpeed(HI_U32 u32Handle, HI_U32 u32Command, HI_U32 u32Speed);
/* ��̨͸������  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_PTZ_Fully_Trans(HI_U32 u32Handle, const HI_CHAR* psBuf, HI_U32 u32BufLen);
/* �����Խ� */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StartVoice(HI_U32 u32Handle, HI_U32 u32AudioType);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StopVoice(HI_U32 u32Handle);
/* �����������ݣ�G.726����  */
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SendVoiceData(HI_U32 u32Handle, HI_CHAR* psBuf, HI_U32 u32BufLen, HI_U64 u64Pts);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SnapJpeg(HI_U32 u32Handle, HI_U8* pu8Data, HI_S32 s32BufLen, HI_S32 *pSize);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_Upgrade(HI_VOID *pParameter);

NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StartRecord(HI_U32 u32Handle, HI_CHAR* psPath, HI_U32 u32Type, HI_U32 u32Flag);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StopRecord(HI_U32 u32Handle);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetRecordState(HI_U32 u32Handle);

NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetChannel(HI_U32 u32Handle, HI_U32 u32Channel);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetChannel(HI_U32 u32Handle);

NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_MakeKeyFrame(HI_U32 u32Handle, HI_U32 u32Channel);

NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetDisplayCfg(HI_U32 u32Handle, HI_S_DISPLAY_CFG *pDisplayCfg);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetDisplayCfg(HI_U32 u32Handle, HI_S_DISPLAY_CFG *pDisplayCfg);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StartDec(HI_U32 u32Handle, HI_U32 u32Channel, HI_S_CHN_INFO *pDevInfo);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_StopDec(HI_U32 u32Handle, HI_U32 u32Channel);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetLoopDecChnInfo(HI_U32 u32Handle, HI_U32 u32Channel, HI_S_LOOP_INFO *pLoopInfo);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetLoopDecChnInfo(HI_U32 u32Handle, HI_U32 u32Channel, HI_S_LOOP_INFO *pLoopInfo);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetLoopDecChnEnable(HI_U32 u32Handle, HI_U32 u32Channel, HI_U32 *pu32Enable);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetLoopDecChnEnable(HI_U32 u32Handle, HI_U32 u32Channel, HI_U32 u32Enable);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetLoopDecEnable(HI_U32 u32Handle, HI_CHAR *psEnable, HI_U32 s32BufLen);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetChnInfo(HI_U32 u32Handle, HI_U32 u32Channel, HI_S_DEC_CHN_INFO *pDecChnInfo);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_GetDecChnEnable(HI_U32 u32Handle, HI_U32 u32Channel, HI_U32 *pu32Enable);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetDecChnEnable(HI_U32 u32Handle, HI_U32 u32Channel, HI_U32 u32Enable);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetRemotePlay(HI_U32 u32Handle, HI_U32 u32Channel, HI_S_DEC_CHN_INFO *pDecChnInfo);
NETSDK_API HI_S32 NETSDK_APICALL HI_NET_DEV_SetRemotePlayControl(HI_U32 u32Handle, HI_U32 u32Channel, HI_U32 u32Command, 
																 HI_U32 u32InValue, HI_U32 *pu32OutValue);

#define ERR_AVI_OK						0x00000000
#define ERR_AVI_INVALIDARG				0x80000001
#define ERR_AVI_MALLOC					0x80000003
#define ERR_AVI_OUTOFSEEKTIME			0x8000000a

#define ERR_AVI_OPEN_FILE				0x80000012
#define ERR_AVI_ENDFILE					0x80000013
#define ERR_AVI_READ_FRAME				0x80000014
#define ERR_AVI_INVALID_STREAM			0X80000015
#define ERR_AVI_WRITE_FRAME				0x80000016

#define ERR_AVI_READ_HEADER				0x80000017
#define ERR_AVI_WRITE_HEADER			0x80000018

#define AVI_VIDEO_FRAME_FLAG 0x1
#define AVI_AUDIO_FRAME_FLAG 0x2
#define AVI_FRAME_KEY_P 0
#define AVI_FRAME_KEY_I 1

typedef struct hiAVI_DURATION_S
{
	HI_S32 s32Hours;
	HI_S32 s32Mins;
	HI_S32 s32Secs;
}AVI_DURATION_S;

#define AVI_VIDEO_FORMAT_H264	0x00
typedef struct hiAVI_VSTREAM_S
{
	HI_U16 u16FormatTag;
	HI_U16 u32FrameRate;
	HI_U32 u32Width;
	HI_U32 u32Height;
} AVI_VSTREAM_S;

#define AVI_AUDIO_FORMAT_G711A	0x00
#define AVI_AUDIO_FORMAT_G726	0x01
typedef struct hiAVI_ASTREAM_S
{
	HI_U16 u16FormatTag;
	HI_U16 u16Channels;   
	HI_U32 u32SamplesPerSec;
	HI_U32 u32AvgBytesPerSec;
	HI_U16 u16BlockAlign;
	HI_U16 u16BitsPerSample;
} AVI_ASTREAM_S;

typedef struct hiAVI_INFO_S
{
	AVI_DURATION_S struDuration;
	AVI_VSTREAM_S struVStream;
    AVI_ASTREAM_S struAStream;
} AVI_INFO_S;

typedef enum _HI_E_AVIFRMTYPE
{
    AVIFRMTYPE_VKEY		= 0,
	AVIFRMTYPE_VNOR		= 1,
	AVIFRMTYPE_AUDIO	= 2,
	AVIFRMTYPE_BUTT
} HI_E_AVIFRMTYPE;

typedef struct hiAVI_FRAME_S
{
	HI_U64 u64Pts;
	HI_U64 u64Dts;
	HI_U8* pu8Data;
	HI_U32 u32Size;
	HI_U32 u32Type;
	HI_U32 u32KeyFlags;
	HI_U32 u32Duration;
}AVI_FRAME_S;

NETSDK_API HI_S32 NETSDK_APICALL AVI_CreateReader(HI_U32* pAVIHandle, HI_U8* pu8FileName);
NETSDK_API HI_S32 NETSDK_APICALL AVI_DestroyReader(HI_U32 u32Handle);
NETSDK_API HI_S32 NETSDK_APICALL AVI_ReadFrame(HI_U32 u32Handle, AVI_FRAME_S *pFrame);
NETSDK_API HI_S32 NETSDK_APICALL AVI_SeekFrame(HI_U32 u32Handle, HI_S32 s32Pts);
NETSDK_API HI_S32 NETSDK_APICALL AVI_ReadFileInfo(HI_U32 u32Handle, AVI_INFO_S *sAviInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

