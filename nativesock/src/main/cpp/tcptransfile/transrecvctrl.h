//
// Created by yutianzuo on 2019-05-22.
//

#ifndef SIMPLESOCKET_TRANSRECVCTRL_H
#define SIMPLESOCKET_TRANSRECVCTRL_H

#include <thread>
#include <vector>
#include <functional>
#include <stdio.h>
#include <poll.h>


#include "../toolbox/miscs.h"
#include "../toolbox/string_x.h"
#include "../toolbox/timeutils.h"
#include "recvmmap.h"
#include "transfiletcprecv.h"
#include "../transdata/CBMD5.h"
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
    TransRecvCtrl(const std::string& str_dir) : m_filedata(str_dir)
    {}
    enum
    {
        LISTENING_PORT = 5896,
        MEMORY_BUFF_LEN = 2 * 1024 * 1024
    };
    bool init(int pieces)
    {
        if (pieces <= 0 || pieces > 16)
        {
            pieces = RecvMmap::PIECES_DEFAULT;
        }
        m_pieces = pieces;
        m_size = 0;
        m_thread_count = 0;
        init_threadfunc();
        return m_sock_listen.init(LISTENING_PORT);
    }

//    bool start()
//    {
//        bool b_ret = true;
//        do
//        {
//            TransFileTcpRecv sock;
//            b_ret = m_sock_listen.accept(sock); //sock maybe a full file negotiating socket; maybe a file piece data transfer socket
//            if (b_ret)
//            {
//                ++m_thread_count;
//                std::thread t(m_threadfunc, std::move(sock));
//                t.detach();
//            }
//            else
//            {
//                if (errno == EINTR)
//                {
//                    b_ret = true;
//                }
//            }
//        } while (b_ret);
//    }

    void start_poll()
    {
        std::vector<pollfd> vec_pollfd;
        pollfd pfd_accept = {0};
        pfd_accept.fd = m_sock_listen.get_socket();
        pfd_accept.events = POLLIN;
        vec_pollfd.push_back(pfd_accept);

        bool start_update = false;
        while (true)
        {
            int nready = ::poll(&vec_pollfd[0], vec_pollfd.size(), 1000);
            if (start_update)
            {
                start_update_info();
            }
            if (nready < 0)
            {
                if (errno == EINTR)
                {
                    //忽略此信号
                    continue;
                }
                else
                {
                    break;
                }
            }
            if (nready == 0) //timeout
            {
                if (!m_sock_listen.is_valid_socket())
                {
                    break;
                }
                continue;
            }

            //android中 close掉了listen的socket，poll一直返回1。
            //并且返回的revents是32。
            if (!(vec_pollfd[0].revents & POLLIN) && !m_sock_listen.is_valid_socket())
            {
                break;
            }

            if (vec_pollfd[0].revents & POLLIN)
            {
                STDCOUT("trans thread create");
                start_update = true;
                TransFileTcpRecv sock;
                bool b_ret = m_sock_listen.accept(sock);
                if (b_ret)
                {
                    ++m_thread_count;
                    std::thread t(m_threadfunc, std::move(sock));
                    t.detach();
                }
                else
                {
                    STDCERR("poll-accept-error");
                }
            }
        }
    }

    template <typename T>
    void set_callback(T&& func)
    {
        m_callback = func;
    }

private:
    std::function<void(const char*)> m_callback;
    int m_pieces;
    std::uint64_t m_size;
    std::string m_md5;
    std::string m_name;
    std::function<void(TransFileTcpRecv)> m_threadfunc;
    RecvMmap m_filedata;
    TransFileTcpRecv m_sock_listen;
    std::atomic_int m_thread_count;

    std::uint64_t last_done_num = 0;
    std::chrono::milliseconds::rep last_time = 0;
    std::uint64_t delta = 0;
    int timeout = 0;

    bool update_file_piece_data(const char* buff, int len, picec_info* info)
    {
        bool b_ret = false;
//        std::unique_lock<std::mutex> lock(m_filedata.get_mutex(), std::defer_lock);
//        lock.lock();
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
//        lock.unlock();
        return b_ret;
    }

    void start_update_info()
    {
        std::string str_persent;
        bool done;
        std::uint64_t left_total;
        this->m_filedata.status(str_persent, done, left_total);
        if (last_time == 0)
        {
            last_time = TimeUtils::get_current_time();
            last_done_num = left_total;
            return;
        }
        std::ostringstream oss;
        oss << "\rprogress:"<<str_persent<<" speed:"<<(last_done_num - left_total) / 1000 <<"KB/s"<<"      ";
        std::string strout = oss.str();
        if (m_callback)
        {
            m_callback(strout.data());
        }
        delta += (last_done_num - left_total);
        last_done_num = left_total;
        if (delta > 5 * 1024 * 1024 || (m_thread_count == 0 && !done))
        {
            delta = 0;
            std::string str_ser_data;
            m_filedata.serialize_data(str_ser_data);
            m_filedata.write_info_file(str_ser_data);
        }
        if (!done && m_thread_count == 0 && ++timeout > 4)
        {
            STDCERR("trans imcomplete, will quit, please trans again");
            m_sock_listen.close();
        }
    }

    void one_piece_done()
    {
        STDCOUT("one piece done");
        std::string persent;
        std::uint64_t left_total;
        bool done;
        m_filedata.status(persent, done, left_total);
        if (done)
        {
            m_filedata.uninit();
            STDCOUT("all done");
            if (check_file())
            {
                STDCOUT("file check ok");
            }
            else
            {
                STDCOUT("file check not ok");
            }
            m_sock_listen.close();
        }
    }


    bool check_file()
    {
        bool b_ret = false;
        long size = get_filesize(m_filedata.get_full_file_path().c_str());
        if (size == 0)
        {
            size = m_size;
        }
        if (size != m_size)
        {
            ::truncate(m_filedata.get_full_file_path().c_str(), m_size);
        }
        std::string md5 = CMD5Checksum::GetMD5(m_filedata.get_full_file_path());
        if (strcmp(md5.c_str(), m_md5.c_str()) == 0)
        {
            if (!m_name.empty())
            {
                ::rename(m_filedata.get_full_file_path().c_str(), (m_filedata.get_dir() + m_name).c_str());
            }
            ::remove(m_filedata.get_full_infofile_path().c_str());
            b_ret = true;
        }
        return b_ret;
    }

    bool neg_ok() //first time negotiate, file md5, name ,size, divided
    {
        if (!m_size || m_md5.empty())
        {
            return false;
        }
        return true;
    }

    std::string neg_backmsg()
    {
        std::string str_ser_data;
        m_filedata.serialize_data(str_ser_data);
        std::string str_out;
        SimpleTransDataUtil::build_trans_data(str_out, str_ser_data.c_str(), str_ser_data.size());
        return str_out;
    }

    void init_threadfunc()
    {
        //this pointer will be accessed by multi-threads
        m_threadfunc = [this](TransFileTcpRecv s) -> void
        {
            if (neg_ok()) //transfer file data
            {
                stringxa str_buff;
                str_buff.resize(MEMORY_BUFF_LEN);
                while (true)
                {
                    if (s.piece_neg_ok()) //piece already negotiated, transfer data
                    {
                        int recv_size = s.receive(str_buff);
                        if (recv_size <= 0)
                        {
                            STDCERR("recv piece data err, maybe close");
                            s.close();
                            break;
                        }
                        if (this->update_file_piece_data(str_buff.c_str(), recv_size, s.get_piece_info()))
                        {
                            //piece done
                            s.close();
                            this->one_piece_done();
                            break;
                        }
                    }
                    else //piece not negotiated , this is first info come, sendback piece info(file offset, lenth)
                    {
                        std::string str_neg;
                        int recv_size = s.receive(str_neg);
                        if (recv_size <= 0)
                        {
                            STDCERR("recv piece info err, maybe close");
                            s.close();
                            break;
                        }
                        if (str_neg.empty())
                        {
                            s.close();
                            break;
                        }
                        std::uint32_t sid = 0xffffffff;
                        try
                        {
                            sid = std::stoi(str_neg);
                        }
                        catch (...)
                        {

                        }
                        if (sid == 0xffffffff)
                        {
                            s.close();
                            break;
                        }
                        s.init_piece_info(m_filedata.get_piece_info_by_sid(sid));
                        s.set_check_data(false);
                        std::string str_sendack;
                        SimpleTransDataUtil::build_trans_data(str_sendack, str_neg.c_str(), str_neg.size());
                        s.send(str_sendack);
                    }
                }
            }
            else //negotiating
            {
                stringxa str_buff;
                s.receive(str_buff);
                if (!str_buff.empty())
                {
                    std::vector<stringxa> vec_info;
                    str_buff.split_string("|", vec_info);
                    if (vec_info.size() >= 2)
                    {
                        if (vec_info[0].size() == 32) //md5
                        {
                            m_md5 = vec_info[0];
                        }
                        try
                        {
                            m_size = std::stoul(vec_info[1]);

                        }
                        catch (...)
                        {

                        }
                        if (vec_info.size() > 2)
                        {
                            m_name = vec_info[2];
                        }

                    }
                }
                if (neg_ok())
                {
                    if (m_filedata.init(m_md5, m_size, m_pieces))
                    {
                        s.send(neg_backmsg());
                    }
                    else
                    {
                        m_md5.clear();
                        m_size = 0;
                        m_name.clear();
                    }
                }
                s.close();

            }
            --m_thread_count;
        };
    }
};

#endif //SIMPLESOCKET_TRANSRECVCTRL_H
