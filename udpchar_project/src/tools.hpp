#pragma once
#include <string.h>
#include <iostream>
#include <string>
#include <time.h>
using namespace std;

enum LogLevel
{
    INFO = 0,
    WARNING,
    ERROR,
    FATAL,
    DEBUG

};

const char* LevelInfo[] = 
{
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL",
    "DEBUG"
};


// loglevel:日志等级
// file：源码文件名称
// line：源码当中对应的代码行
// msg：需要记录的具体信息
//
// [时间戳 等级 源文件名称 ： 行号]  具体信息
// __FILE__ __LINE__

class TimeStamp
{
    public:
        // 年-月-日 时：分：秒
        static void GetTimeStamp(string* timestamp)
        {
            // time函数
            // struct tm* localtime(const time_t *timep)
            time_t sys_time;
            time(&sys_time);

            struct tm* t = localtime(&sys_time);

            char buf[30] = {'\0'};
            snprintf(buf,sizeof(buf)-1, "%04d-%02d-%02d %02d:%02d:%02d",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
            timestamp->assign(buf,strlen(buf));
            
        }
};


ostream& Log(LogLevel loglevel,const char* file, int line, const string& msg)
{
    string timestamp;
    TimeStamp::GetTimeStamp(&timestamp);

    string level = LevelInfo[loglevel];

    cout<<"[" <<timestamp <<" "<<level <<" " <<file <<":"<<line<<"]"<<" "<<msg;
    return cout;
}


#define LOG(loglevel, msg) Log(loglevel,__FILE__, __LINE__, msg)
