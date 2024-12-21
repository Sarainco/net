#pragma once

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/***********************************************
    make -DTAPAHLOGALL    打开所有log
    make -DTAPAHLOGI    打开Info
    make -DTAPAHLOGW    打开Warning
    make -DTAPAHLOGD    打开Debug

    initTapahLog();
    int aa = 123; const char* bb = "abc"; getTapahHexBuf(bb, strlen(bb));
    TapahLogE("%d %s", aa, bb);                //Log Error
    TapahLogI("%d %s", aa, bb);                //Log Info
    TapahLogW("%d %s", aa, tapahHexBuf);    //Log Warning
    TapahLogD("%d %s", aa, tapahHexBuf);    //Log Debug
**********************************************/


extern int tapahLogHandle;                                    //当前log文件的句柄
extern int tapahLogDate;                                    //当前log文件对应的时间，例如2022071315，精确到小时
extern const char* tapahReturnLine;                            //换行符
#define TAPAH_LOGHEADLEN 256
extern char* tapahLogHead;                                    //保存固定log信息的buffer，内容为[E]2022-07-13 15:01:23.[tapah_log.h:14] 
#define TAPAH_LOGLINELEN 1024
extern char* tapahLogLine;                                    //保存log行的buffer
#define TAPAH_TIMEBUFLEN 24
extern char* tapahTimeBuf;                                    //保存当前时间2022-07-13 15:01:23，调用getTapahTime后赋值
#define TAPAH_HEXBUFLEN 1024
extern char* tapahHexBuf;                                    //char* ==> hex，调用getTapahHexBuf后赋值

extern void initTapahLog();                                    //初始化log系统(函数全局应该只调用一次，否则会产生内存泄漏)
extern int getTapahDateHour();                                //获取当前时间，返回值精确到小时
extern void getTapahTime();                                    //获取当前时间，结果保存到tapahTimeBuf
extern int getTapahHexBuf(const char* buf, int length);        //将char*的内容逐个byte打印成16进制，保存到tapahHexBuf
extern void createTapahLogFile();                            //生成log文件句柄

extern int tapah__;                                            //Reserved

#define TapahLogE(LogFormat, ...)    {                                                                                               \
                                        memset(tapahLogHead, 0, TAPAH_LOGHEADLEN); memset(tapahLogLine, 0, TAPAH_LOGLINELEN);        \
                                        memset(tapahTimeBuf, 0, TAPAH_TIMEBUFLEN); getTapahTime();                                   \
                                        sprintf(tapahLogHead, "[E]%s[%s:%d] ", tapahTimeBuf, basename((char*)__FILE__), __LINE__);   \
                                        sprintf(tapahLogLine, LogFormat, ##__VA_ARGS__);                                             \
                                        tapah__ = write(tapahLogHandle, tapahLogHead, strlen(tapahLogHead));                         \
                                        tapah__ = write(tapahLogHandle, tapahLogLine, strlen(tapahLogLine));                         \
                                        tapah__ = write(tapahLogHandle, tapahReturnLine, strlen(tapahReturnLine));                   \
                                        int now = getTapahDateHour();                                                                \
                                        if (tapahLogDate != now) { close(tapahLogHandle); tapahLogDate = now; createTapahLogFile(); }\
                                    }

#if defined(TAPAHLOGALL) || defined(TAPAHLOGI)
#define TapahLogI(LogFormat, ...)    {                                                                                               \
                                        memset(tapahLogHead, 0, TAPAH_LOGHEADLEN); memset(tapahLogLine, 0, TAPAH_LOGLINELEN);        \
                                        memset(tapahTimeBuf, 0, TAPAH_TIMEBUFLEN); getTapahTime();                                   \
                                        sprintf(tapahLogHead, "[I]%s[%s:%d] ", tapahTimeBuf, basename((char*)__FILE__), __LINE__);   \
                                        sprintf(tapahLogLine, LogFormat, ##__VA_ARGS__);                                             \
                                        tapah__ = write(tapahLogHandle, tapahLogHead, strlen(tapahLogHead));                         \
                                        tapah__ = write(tapahLogHandle, tapahLogLine, strlen(tapahLogLine));                         \
                                        tapah__ = write(tapahLogHandle, tapahReturnLine, strlen(tapahReturnLine));                   \
                                        int now = getTapahDateHour();                                                                \
                                        if (tapahLogDate != now) { close(tapahLogHandle); tapahLogDate = now; createTapahLogFile(); }\
                                    }
#else
#define TapahLogI(LogFormat, ...)
#endif

#if defined(TAPAHLOGALL) || defined(TAPAHLOGW)
#define TapahLogW(LogFormat, ...)    {                                                                                               \
                                        memset(tapahLogHead, 0, TAPAH_LOGHEADLEN); memset(tapahLogLine, 0, TAPAH_LOGLINELEN);        \
                                        memset(tapahTimeBuf, 0, TAPAH_TIMEBUFLEN); getTapahTime();                                   \
                                        sprintf(tapahLogHead, "[W]%s[%s:%d] ", tapahTimeBuf, basename((char*)__FILE__), __LINE__);   \
                                        sprintf(tapahLogLine, LogFormat, ##__VA_ARGS__);                                             \
                                        tapah__ = write(tapahLogHandle, tapahLogHead, strlen(tapahLogHead));                         \
                                        tapah__ = write(tapahLogHandle, tapahLogLine, strlen(tapahLogLine));                         \
                                        tapah__ = write(tapahLogHandle, tapahReturnLine, strlen(tapahReturnLine));                   \
                                        int now = getTapahDateHour();                                                                \
                                        if (tapahLogDate != now) { close(tapahLogHandle); tapahLogDate = now; createTapahLogFile(); }\
                                    }
#else
#define TapahLogW(LogFormat, ...)
#endif

#if defined(TAPAHLOGALL) || defined(TAPAHLOGD)
#define TapahLogD(LogFormat, ...)    {                                                                                               \
                                        memset(tapahLogHead, 0, TAPAH_LOGHEADLEN); memset(tapahLogLine, 0, TAPAH_LOGLINELEN);        \
                                        memset(tapahTimeBuf, 0, TAPAH_TIMEBUFLEN); getTapahTime();                                   \
                                        sprintf(tapahLogHead, "[D]%s[%s:%d] ", tapahTimeBuf, basename((char*)__FILE__), __LINE__);   \
                                        sprintf(tapahLogLine, LogFormat, ##__VA_ARGS__);                                             \
                                        tapah__ = write(tapahLogHandle, tapahLogHead, strlen(tapahLogHead));                         \
                                        tapah__ = write(tapahLogHandle, tapahLogLine, strlen(tapahLogLine));                         \
                                        tapah__ = write(tapahLogHandle, tapahReturnLine, strlen(tapahReturnLine));                   \
                                        int now = getTapahDateHour();                                                                \
                                        if (tapahLogDate != now) { close(tapahLogHandle); tapahLogDate = now; createTapahLogFile(); }\
                                    }
#else
#define TapahLogD(LogFormat, ...)
#endif
