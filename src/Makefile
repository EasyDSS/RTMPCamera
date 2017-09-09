

OBJS  = cJSON.o ehttpd.o einifile.o http_parser.o
OBJS += RTMPMgr.o RTMPPushExt.o 
OBJS += main.o
TAR = rtmpcam

CC ?= gcc
CXX ?= g++
CFLAGS += -g -Wall -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -DHI_OS_LINUX
CFLAGS += -I./libEasyRTMP/Include -I./libEasyAACEncoder/Include -I./libEasyRTSPClient/Include -I./HiNetDevSDK/Include
LDFLAGS += -L./libEasyRTMP/Lib/x64 -L./libEasyAACEncoder/Lib/x64 -L./libEasyRTSPClient/Lib/x64 -L./HiNetDevSDK/Lib
LDFLAGS += -leasyrtmp -leasyaacencoder -leasyrtspclient -lpthread

#CFLAGS += -m32
#LDFLAGS += -m32

all: $(TAR)

$(TAR) : $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ -lrt $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $^

%.o : %.cpp
	$(CXX) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(TAR)

