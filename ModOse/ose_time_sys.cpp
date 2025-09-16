#include "ose_time_sys.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
//#include <sys/time.h>
#include <sys/timeb.h>
#include <math.h>


#if defined (__WIN32__)||  defined (__WIN64__)
#define ID_TIMER 100
#endif

#ifndef _MSG_LENGTH_MACRO
#define _MSG_LENGTH_MACRO 256
#endif

using namespace std;

/************************************************************
 * 定时从GPS获取时间信息
 * 重置时间变量
 * 秒脉冲清空毫秒计数
 *
 *
 *
 * *********************************************************/
std::atomic<double>     OSE_TIME_SYS::s_mtx_deltaT(0);        // GPS时间与系统时间的差值 sec

std::atomic<short>      OSE_TIME_SYS::s_mtx_timezone(0);

std::atomic<double> OSE_TIME_SYS::s_mtx_SystemUTCTime(1);
std::atomic<bool>   OSE_TIME_SYS::s_Inited(false);
std::atomic<bool>   OSE_TIME_SYS::s_ExternalTriggerEn(false);
#ifdef TIMER_EN
std::atomic<double> OSE_TIME_SYS::s_crt_time(0);
#endif

std::atomic<double> OSE_TIME_SYS::s_mtx_os_start_time(0);
std::atomic<double> OSE_TIME_SYS::s_mtx_utc_start_time(0);
std::atomic<float>  OSE_TIME_SYS::s_mtx_speed_up_factor(1.0);
std::atomic<double> OSE_TIME_SYS::s_mtx_last_print_time(-1);


// 初始化
// 上电获取系统时间
// 初始化互斥量
void OSE_TIME_SYS::Init()
{
#ifdef TIMER_EN
    SetTimer ( NULL, ID_TIMER, 1, TimerProc) ;
#endif
    s_mtx_deltaT        = 0;

    // 获取时区修正信息
    struct timeb SysTime;
    ftime(&SysTime);
    s_mtx_timezone = SysTime.timezone * 60;

    //    s_mtx_SystemUTCTime = GetOriginUTCTime();
}

void OSE_TIME_SYS::Exit()
{
}

void OSE_TIME_SYS::SetExternalTriggerEn(const bool value)
{
    s_ExternalTriggerEn = value;
}

void OSE_TIME_SYS::SetSpeedUp(float ratio)
{
    if( ratio > 0.1 && ratio < 10)
    {
        //        s_mtx_speed_up_factor = ratio;
        //#ifdef FILE_MNG_H
        //        char strMsg[256];
        //        sprintf(strMsg, "[MSG] Func:%s, Time speed up from %.3f to %.3f\n",
        //                __FUNCTION__, s_mtx_speed_up_factor, ratio);
        //        FILE_MNG::SaveToLog(strMsg, false, true);
        //#endif
        s_mtx_speed_up_factor = ratio;
    }
    else
        s_mtx_speed_up_factor = 1.0;
}

double OSE_TIME_SYS::TimeSync(double _t, bool force)
{
    // 外部触发使能
    if(s_ExternalTriggerEn)
    {
        if(s_Inited == false) // 未初始化时
        {
            s_Inited = true;

            s_mtx_SystemUTCTime = _t;
        }
        else
        {
            // 防止时间突变
            if( fabs( s_mtx_SystemUTCTime - _t) < 100 )
                s_mtx_SystemUTCTime = _t;
        }
        return 0;
    }
    else
    {
        double LocalTime    = GetOriginUTCTime();
        double DeltaT       = _t - LocalTime;
        s_mtx_os_start_time = LocalTime;
        s_mtx_utc_start_time = _t;

        if( (!force) && fabs( DeltaT - s_mtx_deltaT) < 10 )//|| DeltaT > 10
            return DeltaT;

        if( fabs(LocalTime - s_mtx_deltaT) < 0.1)
            return DeltaT;
        else
        {
            s_mtx_deltaT        = DeltaT;

#ifdef FILE_MNG_H
            time_t UniSecs = (time_t)_t;
            double UniBSec = _t - UniSecs;
            struct tm * UniTime = localtime( &UniSecs);
            struct tm SyncT = *UniTime;

            time_t LocSecs = (time_t)LocalTime;
            double LocBSec = LocalTime - LocSecs;
            struct tm * LocTime = localtime( &LocSecs);
            struct tm SysT = *LocTime;
            if( (LocalTime - s_mtx_last_print_time) > 5)
            {
                char strMsg[256];
                sprintf(strMsg, "[MSG] Func:%s\n\tSync Time: %d-%d-%d %d:%d:%.3f\n"
                                "\tSys Time: %d-%d-%d %d:%d:%.3f\n\tDeltaT:%.3f sec",
                        __FUNCTION__, SyncT.tm_year + 1900, SyncT.tm_mon + 1, SyncT.tm_mday,
                        SyncT.tm_hour, SyncT.tm_min, SyncT.tm_sec + UniBSec,
                        SysT.tm_year + 1900, SysT.tm_mon + 1, SysT.tm_mday, SysT.tm_hour, SysT.tm_min, SysT.tm_sec + LocBSec,
                        s_mtx_deltaT*1.0);
                FILE_MNG::SaveToLog(strMsg, false, true);
                s_mtx_last_print_time = LocalTime;
            }
#endif
            return (s_mtx_deltaT * 1000.0);
        }
    }
}

double OSE_TIME_SYS::LocalToUTC(double _T)
{
    return ( _T + s_mtx_timezone);
}

double OSE_TIME_SYS::UTCToLocal(double _T)
{
    return ( _T - s_mtx_timezone);
}

double OSE_TIME_SYS::GetOriginUTCTime()
{
#ifdef TIMER_EN
#if defined (__WIN32) || defined (__WIN64)
    MSG msg;

    int code = GetMessage( &msg, NULL, 0, 0);
    if( code > 0)
    {
        if (msg.message == WM_TIMER)
        {
            DispatchMessage(&msg);
        }
    }
    return s_crt_time;
#endif
#else
    s_mtx_timezone = 0;//SysTime.timezone * 60;
    double ret;
    // 获取系统当前时间
#if 1
    struct timeb SysTime;
    ftime(&SysTime);
    ret = SysTime.time + SysTime.millitm / 1000.0;
#else
    struct timeval Time;
    gettimeofday( &Time, NULL);
    ret = Time.tv_sec + Time.tv_usec/1.0e6;
#endif
    return ret;
#endif
}

double OSE_TIME_SYS::GetUTCTime()
{
    if(s_ExternalTriggerEn)
        return s_mtx_SystemUTCTime;
    else
    {
        return GetOriginUTCTime() + s_mtx_deltaT;
        // return (( GetOriginUTCTime() - s_mtx_os_start_time) * s_mtx_speed_up_factor + s_mtx_utc_start_time);
    }
}

double OSE_TIME_SYS::GetUTCTime(unsigned int days, double LocalDayTime)
{
    // 利用基日+日时间获取本地系统时间
    double LocalTime = days * SECS_IN_DAY + LocalDayTime;

    // 本地时间转UTC时间
    double UTCTime = LocalToUTC( LocalTime);

    // 对时时间修正
    //    double ret = UTCTime + s_mtx_deltaT;

    return UTCTime;
}

double OSE_TIME_SYS::GetUTCTime(unsigned short year,
                                unsigned char month,
                                unsigned char day,
                                double LocalDayTime)
{
    struct tm curTime;
    memset( &curTime, 0, sizeof(struct tm));
    curTime.tm_sec  = 0;
    curTime.tm_min  = 0;
    curTime.tm_hour = 0;
    curTime.tm_mday = day;
    curTime.tm_mon  = month - 1;
    curTime.tm_year = year - 1900;
    //    curTime.tm_wday; // 星期几
    //    curTime.tm_yday;  // 当年已过天数
    time_t LocalDaySec = mktime(&curTime);

    // 利用基日+日时间获取本地系统时间
    double LocalTime = LocalDaySec + LocalDayTime;

    // 本地时间转UTC时间
    double UTCTime = LocalToUTC( LocalTime);

    // 对时时间修正
    return UTCTime;
}

string OSE_TIME_SYS::GetLongTimeStr(bool Formate)
{
    return ToString(GetOriginUTCTime(), Formate);
}

string OSE_TIME_SYS::ToString(double _t, bool Formate)
{
    string ret;

    double TotalSecs = _t;
    time_t SecCnt = (time_t)TotalSecs;

    //    assert( SecCnt > 0);
    struct tm * unitime = localtime( &SecCnt);
    unsigned short year      = unitime->tm_year + 1900;
    unsigned char  month     = unitime->tm_mon + 1;
    unsigned char  day       = unitime->tm_mday;
    unsigned char  hour      = unitime->tm_hour;
    unsigned char  minute    = unitime->tm_min;
    unsigned char  secs      = unitime->tm_sec;

    char tempMsg[_MSG_LENGTH_MACRO];
    if( Formate)
        sprintf( tempMsg, "%04d-%02d-%02d %02d:%02d:%02d",  year, month, day,
                hour, minute, secs);
    else
        sprintf( tempMsg, "%04d%02d%02d%02d%02d%02d",  year, month, day,
                hour, minute, secs);
    ret.assign( tempMsg);

    return ret;
}

string OSE_TIME_SYS::GetShortTimeStr()
{
    string ret;
    double TotalSecs = GetUTCTime(); // GetOriginUTCTime();//
    time_t SecCnt = (time_t)TotalSecs;

    unsigned short msecCnt = (unsigned short)((TotalSecs - SecCnt)*1000);
    struct tm * unitime = localtime( &SecCnt);

    float tempSec = unitime->tm_sec + msecCnt / 1000.0;
    char tempMsg[_MSG_LENGTH_MACRO];
    sprintf( tempMsg, "%02d:%02d:%.3f",  unitime->tm_hour, unitime->tm_min,  tempSec);
    ret.assign(tempMsg);
    return ret;
}

double OSE_TIME_SYS::GetTreadRunTime()
{
    double total = -1;

#if defined (__WIN32) || defined (__WIN64)
    FILETIME CreationTime, ExitTime, KernelTime, UserTime;
    if( GetThreadTimes ( GetCurrentThread(),  &CreationTime, &ExitTime, &KernelTime, &UserTime))
    {
        double dKernelTime = (((ULONGLONG)KernelTime.dwHighDateTime << 32) + KernelTime.dwLowDateTime) * 0.1;
        double dUserTime = (((ULONGLONG)UserTime.dwHighDateTime << 32) + UserTime.dwLowDateTime) * 0.1;
        total = (dKernelTime + dUserTime)/1e6;
    }
#endif

#ifdef __unix__
    struct timespec RunTime;
    clock_gettime( CLOCK_THREAD_CPUTIME_ID,  &RunTime);
    total = RunTime.tv_sec + RunTime.tv_nsec * 1e-9;
#endif

    return  total;
}
#ifdef TIMER_EN
#if defined (__WIN32__)||  defined (__WIN64__)
void CALLBACK OSE_TIME_SYS::TimerProc(HWND hwnd,
                                      UINT message,
                                      UINT iTimerID,
                                      DWORD dwTime)
{
    // 获取系统当前时间
    struct timeb SysTime;
    ftime(&SysTime);

    s_mtx_timezone = 0;//SysTime.timezone * 60;

    struct timeval Time;
    gettimeofday( &Time, NULL);
    s_crt_time = Time.tv_sec + Time.tv_usec/1.0e6;
}
#endif
#endif
