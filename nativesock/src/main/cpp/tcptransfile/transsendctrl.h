//
// Created by yutianzuo on 2019-05-22.
//

#ifndef SIMPLESOCKET_TRANSSENDCTRL_H
#define SIMPLESOCKET_TRANSSENDCTRL_H

#include <functional>
#include "../toolbox/miscs.h"
#include "transfiletcpsender.h"
#include "../transdata/CBMD5.h"
#include "transrecvctrl.h"
#include <mutex>

class SendCtrl final
{
public:
    bool init(const std::string &str_file)
    {
        init_thread_func();
        bool b_ret = false;
        FUNCTION_BEGIN ;
            if (str_file.empty())
            {
                FUNCTION_LEAVE;
            }
            m_file = str_file;
            m_size = get_filesize(str_file.c_str());
            if (m_size <= 0)
            {
                FUNCTION_LEAVE;
            }
            m_md5 = CMD5Checksum::GetMD5(str_file);
            if (m_md5.size() != 32)
            {
                FUNCTION_LEAVE;
            }
            b_ret = true;
        FUNCTION_END;
        STDCOUT("init file ok");
        STDCOUT(m_file);
        STDCOUT(m_md5);
        STDCOUT(m_size);
        return b_ret;
    }

    bool send_file(const std::string& str_ip, int port)
    {
        bool b_ret = false;
        FUNCTION_BEGIN ;
            TransFileTcpSender sender_neg;
            if (!sender_neg.init(str_ip, port))
            {
                STDCERR("sender_neg.init failed");
                FUNCTION_LEAVE;
            }
            m_ip = str_ip;
            m_port = port;
            std::string str_fileinfo = std::string(TRANS_FILE_TYPE_FILE_HANDSHAKE) + "|" + m_md5 + "|";
            str_fileinfo += std::to_string(m_size);
            str_fileinfo += "|";
            std::string::size_type pos = m_file.find_last_of('/');
            std::string strname = pos == std::string::npos ? m_file : m_file.substr(m_file.find_last_of('/') + 1);
            str_fileinfo += strname;
            STDCOUT(str_fileinfo);
            std::string str_build;
            SimpleTransDataUtil::build_trans_data(str_build, str_fileinfo.c_str(), str_fileinfo.size());
            if (!(sender_neg << str_build))
            {
                STDCERR("sender_neg failed");
                sender_neg.close();
                FUNCTION_LEAVE;
            }
            std::string str_piece_infos;
            if(!(sender_neg >> str_piece_infos))
            {
                STDCERR("sender_neg recv failed");
                sender_neg.close();
                FUNCTION_LEAVE;
            }
            deserialize_data(str_piece_infos.c_str(), str_piece_infos.size(), m_piece_infos);
            sender_neg.close();
            if (m_piece_infos.empty())
            {
                STDCERR("m_piece_infos.empty()");
                FUNCTION_LEAVE;
            }

            for (auto& piece : m_piece_infos)
            {
                m_size_left += piece.lenth_left;
            }

            for (int i = 0; i < m_piece_infos.size(); ++i)
            {
                if (m_piece_infos[i].lenth_left > 0)
                {
                    m_send_piece_threads.emplace_back(m_thread_function, i);
                }
            }

            for (int j = 0; j < m_send_piece_threads.size(); ++j)
            {
                m_send_piece_threads[j].join();
            }

            b_ret = true;
        FUNCTION_END;
        return b_ret;
    }

    template <typename T>
    void set_callback(T&& callback)
    {
        m_send_callback = std::forward<T>(callback);
    }

private:
    std::string m_ip;
    int m_port;
    std::string m_file;
    std::string m_md5;
    std::uint64_t m_size;
    std::uint64_t m_size_left = 0;
    std::uint64_t m_send_toll = 0;
    std::vector<picec_info> m_piece_infos;
    std::function<void(int)> m_thread_function;
    std::vector<std::thread> m_send_piece_threads;
    std::mutex m_mutex;
    std::function<void(std::uint64_t, std::uint64_t)> m_send_callback;


    void init_thread_func()
    {
        //this will be accessed by multi-threads
        m_thread_function = [this](int index)->void
        {
            TransFileTcpSender piece_sender;
            if (!piece_sender.init(m_ip, m_port))
            {
                STDCERR("piece sender init failed");
                return;
            }
            std::string str_build;
            std::string str_sid = std::to_string(m_piece_infos[index].sid);
            std::string str_sendsid = std::string(TRANS_FILE_TYPE_PIECE_HANDSHAKE) + "|" + m_md5 + "|" + str_sid;
            SimpleTransDataUtil::build_trans_data(str_build, str_sendsid.c_str(), str_sendsid.size());
            if (!(piece_sender << str_build))
            {
                STDCERR("send piece info failed");
                piece_sender.close();
                return;
            }
            std::string str_buff;
            if (!(piece_sender >> str_buff))
            {
                STDCERR("recv piece info failed");
                piece_sender.close();
                return;
            }
            if (strcmp(str_buff.c_str(), str_sid.c_str()) != 0)
            {
                STDCERR("piece nego info failed");
                piece_sender.close();
                return;
            }
            str_buff.resize(SimpleTransDataUtil::MAX_ORI_DATA_LEN);
            std::ifstream inputfile(m_file);
            if (!inputfile)
            {
                STDCERR("open file error");
                piece_sender.close();
                return;
            }
            piece_sender.set_check_data(false);
            inputfile.seekg(m_piece_infos[index].begin_pos + (m_piece_infos[index].lenth - m_piece_infos[index].lenth_left));
            bool loop = true;
            std::string str_receive_buff;
            str_receive_buff.resize(50);
            std::string str_send_data;
            while (loop)
            {
                int read_len = m_piece_infos[index].lenth_left > str_buff.size() ? str_buff.size() : m_piece_infos[index].lenth_left;
                if (read_len != str_buff.size())
                {
                    str_buff.resize(read_len);
                }
                if (!inputfile.read(&str_buff[0], read_len))
                {
                    //eof probably, if eof state will be set to (eof|fail)
                    //should not be here
                    LogUtils::get_instance()->log_multitype("read file error:", std::strerror
                            (errno));
                    loop = false;
                }
                int len_real_read = inputfile.gcount();
                if (len_real_read != read_len)
                {
                    //should never be here
                    LogUtils::get_instance()->log_multitype("read len != gcount:", std::strerror
                            (errno));
                    STDCERR("read file error");
                    break;
                }

                SimpleTransDataUtil::build_trans_data(str_send_data, str_buff.data(), str_buff.size());

                if (!(piece_sender << str_send_data))
                {
                    STDCERR("send file error -- send");
                    LogUtils::get_instance()->log_multitype("send file error:", std::strerror
                            (errno));
                    piece_sender.close();
                    break;
                }

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_send_toll += len_real_read;
                }
                if (m_send_callback)
                {
                    m_send_callback(m_send_toll, m_size_left);
                }
                m_piece_infos[index].lenth_left -= len_real_read;
                if (m_piece_infos[index].lenth_left == 0)
                {
                    //before eof break while
                    break;
                }

                if (!(piece_sender >> str_receive_buff)) //wait for server check data to continue
                {
                    STDCERR("send file error -- receive");
                    LogUtils::get_instance()->log_multitype("send file error-- receive back:", std::strerror
                            (errno));
                    piece_sender.close();
                    break;
                }
            }
            inputfile.close();
            piece_sender.close();
            STDCOUT("piece done");
        };
    }

};

#endif //SIMPLESOCKET_TRANSSENDCTRL_H
