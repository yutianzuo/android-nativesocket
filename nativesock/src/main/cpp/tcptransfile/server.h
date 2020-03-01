//
// Created by yutianzuo on 2020/2/25.
//

#ifndef SIMPLESOCKET_SERVER_H
#define SIMPLESOCKET_SERVER_H

#include <map>
#include <memory>
#include "../toolbox/stlutils.h"
#include "transfiletcprecv.h"
#include "transrecvctrl.h"
#include "../toolbox/logutils.h"


///接收文件服务，接收多线程上载文件。
///顶层类解析协议，将解析协议通过的socket分发给多线程接收处理类处理。
class FileRecvServer final
{
public:
    bool start_server(int file_piece, const std::string& dir)
    {
        bool ret = false;
        FUNCTION_BEGIN ;
            ret = m_listen_s.init(TransRecvCtrl::LISTENING_PORT);
            if (!ret)
            {
                FUNCTION_LEAVE;
            }
            std::vector<pollfd> vec_pollfd;
            pollfd pfd_accept = {0};
            pfd_accept.fd = m_listen_s.get_socket();
            pfd_accept.events = POLLIN;
            vec_pollfd.push_back(pfd_accept);
            while (true)
            {
                int nready = ::poll(&vec_pollfd[0], vec_pollfd.size(), 2000);
                if (m_quit)
                {
                    uninit();
                    break;
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
                    if (!m_listen_s.is_valid_socket())
                    {
                        break;
                    }
                    timer();
                    continue;
                }

                //android中 close掉了listen的socket，poll一直返回1。
                //并且返回的revents是32。
                if (!(vec_pollfd[0].revents & POLLIN) && !m_listen_s.is_valid_socket())
                {
                    break;
                }

                if (vec_pollfd[0].revents & POLLIN)
                {
                    TransFileTcpRecv sock_acc;
                    bool inner_ret = m_listen_s.accept(sock_acc);
                    if (!inner_ret)
                    {
                        LogUtils::get_instance()->log_multitype("non-timer----poll-accept-error");
                        continue;
                    }

                    stringxa buff;
                    sock_acc.receive(buff);
                    std::vector<stringxa> vec_info;
                    buff.split_string("|", vec_info);
                    if (vec_info.empty())
                    {
                        LogUtils::get_instance()->log_multitype("non-timer----unknown request:", buff);
                        sock_acc.close();
                        continue;
                    }
                    if (0 == vec_info[0].compare_no_case(TRANS_FILE_TYPE_FILE_HANDSHAKE))
                    {
                        std::string md5;
                        std::string name;
                        std::uint64_t size = 0;
                        if (vec_info.size() >= 3)
                        {
                            if (vec_info[1].size() == 32)
                            {
                                md5 = vec_info[1];
                            }
                            try
                            {
                                size = std::stoul(vec_info[2]);

                            }
                            catch (...)
                            {

                            }
                            if (vec_info.size() > 3)
                            {
                                name = vec_info[3];
                            }
                            if (!md5.empty() && size > 0)
                            {
                                if (m_map_trans.find(md5) == m_map_trans.end())
                                {
                                    if (file_piece <= 0 || file_piece > 16)
                                    {
                                        file_piece = RecvMmap::PIECES_DEFAULT;
                                    }
                                    std::shared_ptr<RecvMmap> sp_filedata = std::make_shared<RecvMmap>(dir);
                                    if (!sp_filedata->init(md5, size, file_piece))
                                    {
                                        LogUtils::get_instance()->log_multitype("non-timer----", name,
                                                " init file data failed");
                                        sock_acc.close();
                                        continue;
                                    }
                                    std::string str_ser_data;
                                    sp_filedata->serialize_data(str_ser_data);
                                    std::string str_out;
                                    SimpleTransDataUtil::build_trans_data(str_out, str_ser_data.c_str(),
                                                                          str_ser_data.size());
                                    if (sock_acc.send(str_out))
                                    {
                                        std::shared_ptr<TransRecvCtrl> sp_trans_recv_ctrl = std::make_shared<TransRecvCtrl>(
                                                sp_filedata, md5, size, name);
#ifdef CONSOL_DEBUG
                                        sp_trans_recv_ctrl->set_info_callback([](
                                                const char *persent, std::uint64_t bytes) -> void
                                          {
                                              std::ostringstream oss;
                                              oss << "\rprogress:" << persent
                                                  << " speed:"
                                                  << bytes / 1000 << "KB/s" << "      ";
                                              std::string strout = oss.str();
                                              std::cout << strout;
                                              fflush(stdout);
                                          });
#endif
                                        m_map_trans.insert(
                                                std::pair<std::string, std::shared_ptr<TransRecvCtrl>>(
                                                        std::move(md5), std::move(sp_trans_recv_ctrl)));
                                        sock_acc.receive(buff); //avoid time_wait
                                    }
                                }
                                else
                                {
                                    LogUtils::get_instance()->log_multitype("non-timer----", name, "is in trans map");
                                }

                            }
                        }
                        sock_acc.close();
                    }
                    else if (0 == vec_info[0].compare_no_case(TRANS_FILE_TYPE_PIECE_HANDSHAKE))
                    {
                        std::string md5;
                        std::uint32_t sid = 0xffffffff;

                        if (vec_info.size() >= 3)
                        {
                            if (vec_info[1].size() == 32)
                            {
                                md5 = vec_info[1];
                            }
                            try
                            {
                                sid = std::stoi(vec_info[2]);
                            }
                            catch (...)
                            {

                            }
                        }
                        if (!md5.empty() && sid != 0xffffffff && m_map_trans.find(md5) != m_map_trans.end())
                        {
                            sock_acc.init_piece_info(
                                    m_map_trans.find(md5)->second->get_filedata()->get_piece_info_by_sid(sid));
                            std::string str_sendback;
                            SimpleTransDataUtil::build_trans_data(str_sendback,
                                                                  vec_info[2].c_str(), vec_info[2].size());
//                            sock_acc.set_check_data(false);
                            if (sock_acc.send(str_sendback))
                            {
                                m_map_trans.find(md5)->second->add_socket(std::move(sock_acc));
                            }
                        }
                        else
                        {
                            LogUtils::get_instance()->log_multitype("non-timer----piece handshake err:",
                                    md5, "  ", sid, "  ",  m_map_trans.find(md5) == m_map_trans.end());
                            sock_acc.close();
                        }
                    }
                    else
                    {
                        LogUtils::get_instance()->log_multitype("non-timer----unknown handshake:", vec_info[0]);
                        sock_acc.close();
                    }
                }
            }
            ret = true;
            FUNCTION_END;
        return ret;
    }

    void quit()
    {
        m_quit = true;
    }

    template <typename T>
    void set_callback(T&& callback)
    {
        m_callback = std::forward<T>(callback);
    }

private:
    std::map<std::string, std::shared_ptr<TransRecvCtrl> > m_map_trans;
    using MAP = std::map<std::string, std::shared_ptr<TransRecvCtrl> >;
    TransFileTcpRecv m_listen_s;
    bool m_quit = false;
    std::function<void(const char* out)> m_callback;

    void uninit()
    {
        m_listen_s.close();
        m_map_trans.clear();
    }

    void timer()
    {
        STLUtils::delete_from_container(m_map_trans,
                [this](const std::pair<std::string, const std::shared_ptr<TransRecvCtrl> > &kv) ->
                bool
        {
            kv.second->update_info();
            bool ret = kv.second->can_clear();
            if (ret)
            {
                if (m_callback)
                {
                    m_callback(kv.second->get_file_name().data());
                }
            }
            return ret;
        });
    }
};

#endif //SIMPLESOCKET_SERVER_H
