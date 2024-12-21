#include "tapah_log.h"

int tapahLogHandle;
int tapahLogDate;
const char *tapahReturnLine = "\n";
char *tapahLogHead = NULL;
char *tapahLogLine = NULL;
char *tapahTimeBuf = NULL;
char *tapahHexBuf = NULL;
int tapah__ = 0;

void initTapahLog()
{
    tapahLogHandle = 0;
    tapahLogDate = getTapahDateHour();
    tapahLogHead = malloc(TAPAH_LOGHEADLEN);
    tapahLogLine = malloc(TAPAH_LOGLINELEN);
    tapahTimeBuf = malloc(TAPAH_TIMEBUFLEN);
    tapahHexBuf = malloc(TAPAH_HEXBUFLEN);
    createTapahLogFile();
}

int getTapahDateHour()
{
    time_t t = time(NULL);
    struct tm *tmnow = gmtime(&t);
    return tmnow->tm_hour + tmnow->tm_mday * 100 + (1 + tmnow->tm_mon) * 10000 + (1900 + tmnow->tm_year) * 1000000;
}

void getTapahTime()
{
    memset(tapahTimeBuf, 0, TAPAH_TIMEBUFLEN);
    time_t t = time(NULL);
    struct tm *tmnow = localtime(&t);
    strftime(tapahTimeBuf, 24, "%Y-%m-%d %H:%M:%S.", tmnow);
}

int getTapahHexBuf(const char *buf, int length)
{
    memset(tapahHexBuf, 0, TAPAH_HEXBUFLEN - 1);
    unsigned char *p = (unsigned char *)tapahHexBuf;
    unsigned char *d = (unsigned char *)buf;
    int offset = 0, c = 0;
    for (int i = 0; i < length; i++)
    {
        if (offset + 3 >= TAPAH_HEXBUFLEN - 1)
            break;
        c = (d[i] >> 4);
        if (c >= 10)
        {
            *p++ = c + 0x41 - 10;
        }
        else
        {
            *p++ = c + 0x30;
        }
        c = (d[i] & 0x0F);
        if (c >= 10)
        {
            *p++ = c + 0x41 - 10;
        }
        else
        {
            *p++ = c + 0x30;
        }
        *p++ = ' ';
        offset += 3;
    }
    *p = '\0';
    return offset;
}

void createTapahLogFile()
{
    char logName[256] = {0};
    sprintf(logName, "/mnt/logs/%d.log", getTapahDateHour());
    tapahLogHandle = open(logName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}
