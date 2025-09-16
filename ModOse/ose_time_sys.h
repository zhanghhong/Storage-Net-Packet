#ifndef TIME_SYS_H
#define TIME_SYS_H
#include <atomic>
#include <string>
#include <signal.h>
#include <time.h>

#if defined (__WIN32) || defined (__WIN64)
#include <winsock2.h>
#include <windows.h>
#include <winbase.h>
#include <processthreadsapi.h>
//#include <realtimeapiset.h>
#endif

#ifdef __unix__
#include <time.h>
#include <signal.h>
#include <errno.h>
#define NDEBUG
#endif

#ifndef SECS_IN_DAY
#define SECS_IN_DAY (86400) // 24*3600
#endif


// #define TIMER_EN

class OSE_TIME_SYS
{
public:
    // 时间系统初始化
    static void Init();

    // 时间系统退出
    static void Exit();

    static void SetExternalTriggerEn(const bool value);

    static void SetSpeedUp( float ratio = 1.0);

    /**
     * @brief TimeSync 系统对时处理，设置系统时间
     * @param _t 1970年至今的秒计数
     */
    static double TimeSync(double _t, bool force = false);

    /**
     * @brief GetOriginSysTime 获取原始系统当前UTC时间
     * @return 1970-01-01至今的秒数
     * @note 时区修正值在此处获取
     */
    static double GetOriginUTCTime();

    /**
     * @brief GetUTCTime 获取对时后的系统当前UTC时间
     * @return 1970-01-01至今的秒数
     */
    static double GetUTCTime();

    /**
     * @brief GetUTCTime 获取指定当地时间对应的UTC时间
     * @param days 基日，1970年1月1日至今的天数
     * @param LocalDayTime 本地当日时间，sec
     * @return 1970-01-01至当前时间的秒数
     */
    static double GetUTCTime(unsigned int days, double LocalDayTime);
    /**
     * @brief GetUTCTime 获取指定当地时间对应的UTC时间
     * @param year 年
     * @param month 月
     * @param day 日
     * @param LocalDayTime 本地当日时间，sec
     * @return 1970-01-01至当前时间的秒数
     */
    static double GetUTCTime(unsigned short year, unsigned char month, unsigned char day,
                             double LocalDayTime);

    /**
     * @brief GetShortTimeStr 获取时间字符串（短）
     * @return 时间字符串
     */
    static std::string GetShortTimeStr();

    /**
     * @brief GetLongTimeStr 获取时间字符串（长）
     * @param Formate 输出标准时间格式标志
     * @return 时间字符串
     */
    static std::string GetLongTimeStr(bool Formate = false);

    /**
     * @brief ToString UTC时间转字符串
     * @param _t 1970-01-01至当前时间的秒数
     * @param Formate 输出标准时间格式标志
     * @return 时间字符串
     */
    static std::string ToString(double _t, bool Formate = false);

    /**
     * @brief LocalToUTC 本地时间转UTC时间
     * @param _T 本地时间（1970-01-01至当前时间的秒数）
     * @return UTC时间（1970-01-01至当前时间的秒数）
     */
    static double LocalToUTC( double _T);

    /**
     * @brief UTCToLocal UTC时间转本地时间
     * @param _T UTC时间（1970-01-01至当前时间的秒数）
     * @return 本地时间（1970-01-01至当前时间的秒数）
     */
    static double UTCToLocal( double _T);

    static double GetTreadRunTime();


private:
#ifdef TIMER_EN
#if defined (__WIN32__)||  defined (__WIN64__)
    static void CALLBACK TimerProc(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime);
#endif
#endif
    // void SetTimeZoneRevise();

private:
    // GPS时间与系统时间的差值 sec
    static std::atomic<double> s_mtx_deltaT;
    // 时区修正值
    static std::atomic<short> s_mtx_timezone;
#ifdef TIMER_EN
    // 当前OS时间
    static std::atomic<double> s_crt_time;
#endif

    // 原始点更新的时间
    static std::atomic<double> s_mtx_SystemUTCTime;
    static std::atomic<bool>  s_Inited;
    static std::atomic<bool>  s_ExternalTriggerEn;

    static std::atomic<double> s_mtx_os_start_time;
    static std::atomic<double> s_mtx_utc_start_time;
    static std::atomic<float> s_mtx_speed_up_factor;
    static std::atomic<double> s_mtx_last_print_time;
};

#endif // TIME_SYS_H
