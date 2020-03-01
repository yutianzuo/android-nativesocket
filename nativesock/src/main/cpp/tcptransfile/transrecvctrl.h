//
// Created by yutianzuo on 2019-05-22.
//

#ifndef SIMPLESOCKET_TRANSRECVCTRL_H
#define SIMPLESOCKET_TRANSRECVCTRL_H

#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include <cstdio>
#include <poll.h>
#include <atomic>
#include <mutex>

#include "../toolbox/stlutils.h"
#include "../toolbox/miscs.h"
#include "../toolbox/string_x.h"
#include "../toolbox/timeutils.h"
#include "recvmmap.h"
#include "transfiletcprecv.h"
#include "../transdata/CBMD5.h"
#include "../toolbox/logutils.h"

/**
 * transfer file through tcp in multi-thread,
 * receiver actually is a server
 * 1. sender connect to server send filename, size, md5
 * server sendback file piece info
 * 2. file begin transfer, recv record status for further retrans if necessary
 */
class TransRecvCtrl final
{
public:
    TransRecvCtrl(const TransRecvCtrl&) = delete;
    TransRecvCtrl(TransRecvCtrl&&) = delete;
    void operator = (const TransRecvCtrl&) = delete;
    void operator = (TransRecvCtrl&&) = delete;

    TransRecvCtrl(const std::shared_ptr<RecvMmap>& sp_filedata, const std::string& md5, std::uint64_t size,
            const std::string& name) : m_sp_filedata(sp_filedata), m_md5(md5), m_size(size),
            m_name(name), m_thread_count(0), m_start_update(false), m_can_clear(false)
    {
        init_threadfunc();
    }

    ~TransRecvCtrl()
    {
        STDCOUT("~TransRecvCtrl");
        quit();
        m_sp_filedata->uninit();
    }

    enum
    {
        LISTENING_PORT = 5896,
    };

    void add_socket(TransFileTcpRecv&& s)
    {
        ++m_thread_count;
        m_start_update = true;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_socks_trans.push_back(s.get_socket());
        }
        std::thread t(m_threadfunc, std::move(s));
        t.detach();
    }

    bool can_clear()
    {
        return m_can_clear;
    }

    std::string get_file_name()
    {
        return m_name;
    }


    void update_info()
    {
        if (!m_start_update)
        {
            return;
        }
        std::string str_persent;
        bool done;
        std::uint64_t left_total;
        m_sp_filedata->status(str_persent, done, left_total);
        if (done)
        {
            return;
        }

        if (last_time == 0)
        {
            last_time = TimeUtils::get_current_time();
            last_done_num = left_total;
            return;
        }

        if (last_done_num == left_total)
        {
            ++m_receive_timeout;
            if (m_receive_timeout > 30) //over 1 minute no data, quit
            {
                quit();
            }
        }
        else
        {
            m_receive_timeout = 0;
        }

        if (m_info_callback)
        {
            m_info_callback(str_persent.data(),last_done_num - left_total);
        }
        delta += (last_done_num - left_total);
        last_done_num = left_total;
        if (delta > 5 * 1024 * 1024 || (m_thread_count == 0 && !done))
        {
            delta = 0;
            std::string str_ser_data;
            m_sp_filedata->serialize_data(str_ser_data);
            if (!m_sp_filedata->is_uninit())
            {
                m_sp_filedata->write_info_file(str_ser_data);
            }
        }
        LogUtils::get_instance()->log_multitype("update_info(", m_name, ")", "done:", done, " m_thread_count:",
                m_thread_count, " timeout:", timeout, " persent:", str_persent, " leftsize:", left_total);
        if (!done && m_thread_count == 0 && ++timeout > 2)
        {
            m_can_clear = true;
            STDCERR("trans imcomplete, will quit, please trans again");
        }
    }

    std::shared_ptr<RecvMmap>& get_filedata()
    {
        return m_sp_filedata;
    }


    template <typename T>
    void set_info_callback(T&& func)
    {
        m_info_callback = std::forward<T>(func);
    }

    void quit()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_socks_trans.empty())
        {
            for (auto sock : m_socks_trans)
            {
                ::shutdown(sock, SHUT_RDWR);
                ::close(sock);
            }
        }
    }

private:
    std::function<void(const char*, std::uint64_t)> m_info_callback;
    std::uint64_t m_size;
    std::string m_md5;
    std::string m_name;
    std::function<void(TransFileTcpRecv)> m_threadfunc;
    std::shared_ptr<RecvMmap> m_sp_filedata;
    std::atomic_int m_thread_count;
    std::mutex m_mutex;
    int m_receive_timeout = 0;

    std::uint64_t last_done_num = 0;
    std::chrono::milliseconds::rep last_time = 0;
    std::uint64_t delta = 0;
    int timeout = 0;
    bool m_start_update;
    bool m_can_clear;

    std::vector<int> m_socks_trans;

    ///run in a working thread
    bool update_file_piece_data(const char* buff, int len, picec_info* info)
    {
        bool b_ret = false;
        std::unique_lock<std::mutex> lock(m_sp_filedata->get_mutex(), std::defer_lock);
        lock.lock();
        if (info->is_mapping_valid())
        {
            if (len > info->lenth_left)
            {
                len = info->lenth_left;
            }
            memcpy(info->mapping, buff, len);
            info->mapping += len;
            info->lenth_left -= len;

            if (info->lenth_left == 0)
            {
                b_ret = true; //piece done
            }
        }
        lock.unlock();
        return b_ret;
    }

    ///run in a working thread
    void one_piece_done()
    {
        STDCOUT("one piece done");
        std::string persent;
        std::uint64_t left_total;
        bool done;
        m_sp_filedata->status(persent, done, left_total);
        if (done)
        {
            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
            lock.lock();
            if (m_sp_filedata->is_uninit())
            {
                return;
            }
            m_sp_filedata->uninit();
            lock.unlock();
            STDCOUT("all done");
            if (check_file())
            {
                STDCOUT("file check ok");
            }
            else
            {
                STDCOUT("file check not ok");
            }
            m_can_clear = true;
        }
    }

    ///run in a working thread
    bool check_file()
    {
        bool b_ret = false;
        long size = get_filesize(m_sp_filedata->get_full_file_path().c_str());
        if (size == 0)
        {
            size = m_size;
        }
        if (size != m_size)
        {
            ::truncate(m_sp_filedata->get_full_file_path().c_str(), m_size);
        }
        std::string md5 = CMD5Checksum::GetMD5(m_sp_filedata->get_full_file_path());
        if (strcmp(md5.c_str(), m_md5.c_str()) == 0)
        {
            if (!m_name.empty())
            {
                ::rename(m_sp_filedata->get_full_file_path().c_str(),
                        (m_sp_filedata->get_dir() + m_name).c_str());
            }
            int ctrl = 0;
            while (::remove(m_sp_filedata->get_full_infofile_path().c_str()) != 0)
            {
                if (ctrl++ > 5)
                {
                    LogUtils::get_instance()->log_multitype("non-timer----remove info file failed:", m_name);
                    break;
                }
                TimeUtils::sleep_for_millis(200);
            }
            b_ret = true;
            LogUtils::get_instance()->log_multitype("non-timer----check md5 OK, all done:", m_name);
        }
        else
        {
            LogUtils::get_instance()->log_multitype("non-timer----check md5 failed delete all data:", m_name);
            ::remove(m_sp_filedata->get_full_file_path().c_str());
            ::remove(m_sp_filedata->get_full_infofile_path().c_str());
        }
        return b_ret;
    }

    void init_threadfunc()
    {
        //this pointer will be accessed by multi-threads
        m_threadfunc = [this](TransFileTcpRecv s) -> void
        {
            if (s.piece_neg_ok())
            {
                stringxa str_buff;
                str_buff.resize(SimpleTransDataUtil::MAX_DATA_LEN);
                while (true)
                {
                    int recv_size = s.receive(str_buff);
                    if (recv_size <= 0)
                    {
                        LogUtils::get_instance()->log_multitype("recv piece data err, maybe close:", m_name);
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            STLUtils::delete_from_container(m_socks_trans, [&s](const int& sock) -> bool
                            {
                                return s.get_socket() == sock;
                            });
                        }
                        s.close();
                        break;
                    }
                    if (str_buff.empty())
                    {
                        //上层数据校验失败
                        LogUtils::get_instance()->log_multitype("data check failed in one piece:", m_name);
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            STLUtils::delete_from_container(m_socks_trans, [&s](const int& sock) -> bool
                            {
                                return s.get_socket() == sock;
                            });
                        }
                        s.close();
                        break;
                    }
                    if (this->update_file_piece_data(str_buff.c_str(), recv_size, s.get_piece_info()))
                    {
                        //piece done
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            STLUtils::delete_from_container(m_socks_trans, [&s](const int& sock) -> bool
                            {
                                return s.get_socket() == sock;
                            });
                        }
                        s.receive(str_buff);//avoid time_wait
                        s.close();
                        this->one_piece_done();
                        break;
                    }
                    if (!s.send("continue"))
                    {
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            STLUtils::delete_from_container(m_socks_trans, [&s](const int& sock) -> bool
                            {
                                return s.get_socket() == sock;
                            });
                        }
                        s.close();
                        LogUtils::get_instance()->log_multitype("sendback failed in one piece:", m_name);
                        break;
                    }

                }
            }
            --m_thread_count;
        };
    }
};

#endif //SIMPLESOCKET_TRANSRECVCTRL_H
