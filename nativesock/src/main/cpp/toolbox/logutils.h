#ifndef TESTPROJ_LOGUTILS_H
#define TESTPROJ_LOGUTILS_H

#include <thread>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <sstream>
//--------------------------------------
#include "timeutils.h"

#define __FILE_LINE__ "(", __FILE__", ", std::to_string(__LINE__), ") "

class LogUtils
{
public:

    LogUtils(const LogUtils &) = delete;

    LogUtils(LogUtils &&) = delete;

    void operator=(const LogUtils &) = delete;

    void operator=(LogUtils &&) = delete;

    //better invoke at the begining of app
    static void init(const std::string &str_logfile_path)
    {
        uninit();
        if (!g_instance)
        {
            g_instance = new LogUtils(str_logfile_path);
        }
    }

    //not thread safe
    static LogUtils *get_instance()
    {
        if (!g_instance)
        {
            g_instance = new LogUtils("log.txt");
        }
        return g_instance;
    }

    //not thread safe
    static void uninit()
    {
        if (g_instance)
        {
            g_instance->uninit_thread();
            delete g_instance;
            g_instance = nullptr;
        }
    }

    //better use this func
    template <typename...T>
    void log_multitype(T &&...params)
    {
        std::ostringstream ss;
        log_multitypeinner(ss, std::forward<T>(params)...);
    }

    //DO NOT log long info, if do so , us log()
    //C like format "%d..."
    void log_format(const char *sz_log, ...)
    {
        if (!m_log_stream.is_open())
            return;
        const int len = 512;
        char txt_buff[len] = {0};

        va_list argList2;
        va_start(argList2, sz_log);
        vsnprintf(txt_buff, len, sz_log, argList2);
        va_end(argList2);


        std::string str_logformat;
        build_outstring(str_logformat, txt_buff);

        {
            std::lock_guard<std::mutex> tmp_lock(m_mutex);
            m_logs.emplace_back(str_logformat);
            m_cv.notify_all();
        }
    }

    //log only a string,no lenth limit
    void log(const std::string& str_log)
    {
        if (!m_log_stream.is_open())
            return;

        std::string str_logformat;
        build_outstring(str_logformat, str_log);

        {
            std::lock_guard<std::mutex> tmp_lock(m_mutex);
            m_logs.emplace_back(str_logformat);
            m_cv.notify_all();
        }
    }

private:
    //C++11 like log multi-type
    template<typename HEAD, typename... TAIL>
    void log_multitypeinner(std::ostringstream& ss, HEAD &&first, TAIL &&...tails)
    {
        ss << first;
        log_multitypeinner(ss, std::forward<TAIL>(tails)...);
    }

    template<typename LAST>
    void log_multitypeinner(std::ostringstream& ss, LAST &&value)
    {
        ss << value;
        log(ss.str());
    }

    explicit LogUtils(const std::string &str_logfile_path)
    {
        init_file(str_logfile_path);
        init_thread();
    }

    void init_file(const std::string &str_logfile_path)
    {
        if (!str_logfile_path.empty())
        {
            m_log_stream.open(str_logfile_path.c_str(), std::ios::out | std::ios::app);
        }
        else
        {
            m_log_stream.open("log.txt", std::ios::out | std::ios::app);
        }
    }

    void init_thread()
    {
        //init thread use operator = (&&)
        m_thread = std::thread(
                [this]() -> void
                {
                    std::string str_logs;
                    std::unique_lock<std::mutex> tmplock(m_mutex, std::defer_lock_t());
                    while (true)
                    {
                        tmplock.lock();
                        m_cv.wait(tmplock,
                                  [this]() -> bool
                                  {
                                      return !m_logs.empty();
                                  });


                        str_logs.clear();
                        while (!m_logs.empty())
                        {
                            str_logs.append(m_logs.front());
                            m_logs.pop_front();
                        }

                        if (m_quit_thread)
                        {
                            m_log_stream << str_logs;
                            break;
                        }

                        tmplock.unlock();
                        m_log_stream << str_logs;
                    }
                    uninit_file();
                }
        );
    }

    void build_outstring(std::string& str_output, const std::string str_log)
    {
        str_output.assign(TimeUtils::get_zhCN_time());
        str_output.append(": ");
        str_output.append(str_log);
        str_output.append("\n");
    }


    void uninit_file()
    {
        if (m_log_stream.is_open())
        {
            m_log_stream.flush();
            m_log_stream.clear();
            m_log_stream.close();
        }
    }

    void uninit_thread()
    {
        {
            std::lock_guard<std::mutex> tmp_lock(m_mutex);
            m_logs.emplace_back("-----------the end--------------\n");
            m_quit_thread = true;
            m_cv.notify_all();
        }
        m_thread.join();
    }

    static LogUtils *g_instance;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::string> m_logs;
    std::ofstream m_log_stream;
    volatile bool m_quit_thread = false;
};

class TestLogUtils
{
public:
    static void execute()
    {
        LogUtils::init("logtest.log");

        LogUtils::get_instance()->log_format("金泰你是个%s:%d", "123", 33333);
        LogUtils::get_instance()->log_format("second line");
        LogUtils::get_instance()->log("123");

        LogUtils::get_instance()->log_multitype(__FILE_LINE__, "singleline");
        LogUtils::get_instance()->log_multitype(__FILE_LINE__, 2.3f, " ", "haha ", 123, " ", 89898);

        LogUtils::uninit();
    }
};

LogUtils *LogUtils::g_instance = nullptr;

#endif //TESTPROJ_LOGUTILS_H
