//
// Created by yutianzuo on 2018/4/24.
//

#ifndef SIMPLESOCKET_SERVERSOCKET_H
#define SIMPLESOCKET_SERVERSOCKET_H

#include "socketbase.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <vector>

class ServerSocket : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    ServerSocket(const ServerSocket &) = delete;

    ServerSocket(ServerSocket &&s) : SocketBase(std::move(s))
    {
    }

    void operator=(const ServerSocket &) = delete;

    void operator=(ServerSocket &&) = delete;

#endif

    enum
    {
        SERVER_PORT = 19840
    };

    ServerSocket() : SocketBase()
    {

    }

    ServerSocket(int sock) : SocketBase(sock)
    {

    }

    virtual ~ServerSocket()
    {
    }

    bool init(int port)
    {
        if (!create()) //socket
        {
            close();
            return false;
        }

        if (!bind(port)) //bind
        {
            close();
            return false;
        }

        if (!listen()) //listen
        {
            close();
            return false;
        }

        return true;
    }

    /**
     * can't combine polling_xxx
     * @param socket
     * @return
     */
    bool accept(ServerSocket &socket)
    {
        return SocketBase::accept(socket);
    }

    bool operator>>(std::string &buff)
    {
        return receive(buff) > 0;
//        return *this;
    }

    bool operator<<(const std::string &buff)
    {
        return send(buff);
//        return *this;
    }


    //-----------------------------------------------------------------------------------------------------------------

    /**
     * select polling
     * 单线程select模型。
     */
    fd_set m_read_fdset;
    fd_set m_all_fdset;

    void polling_select()
    {
//        std::string strbuffread;
        FD_ZERO(&m_all_fdset);
        FD_SET(m_sock, &m_all_fdset);
        while (true)
        {
            m_read_fdset = m_all_fdset;
            int total = ::select(FD_SETSIZE, &m_read_fdset, nullptr, nullptr, nullptr);
            if (total > 0)
            {
                if (FD_ISSET(m_sock, &m_read_fdset)) //accept
                {
                    int addr_length = sizeof(m_addr);
                    int socket_data = ::accept(m_sock, (sockaddr *) &m_addr, (socklen_t *) &addr_length);

                    if (socket_data <= 0)
                    {
                        std::cout << "polling_select()accept err:" << std::strerror(errno) << std::endl;
                        continue;
                    }

                    FD_SET(socket_data, &m_all_fdset);
                }
                else //read && write back
                {
                    for (int i = 0; i < FD_SETSIZE; ++i)
                    {
                        if (FD_ISSET(i, &m_read_fdset))
                        {
                            ServerSocket s;
                            s.m_sock = i;

                            //所有IO多路复用实际都不必然搭配non-blocking socket（epool ET模式除外）
                            //只要每次只调用一次recv，如果有剩余数据那么就等待系统再一次回调即可，业务编写者只需要做好数据的缓存和记录即可。
                            //下面注释掉的代码展示了这样的逻辑


//                            char buf[5] = {0};
//                            ssize_t status;
//                            status = ::recv(i, buf, 5, 0);
//                            if (status > 0)
//                            {
//                                strbuffread.append(buf, status);
//                                if (SimpleTransDataUtil::check_data(strbuffread))
//                                {
//                                    std::cout<<"bingo:"<<strbuffread<<std::endl;
//                                    std::string strbuff;
//                                    std::string strinput("server recieve client!");
//                                    SimpleTransDataUtil::build_trans_data(strbuff,
//                                                                          (unsigned char *) strinput.c_str(),
//                                                                          strinput.size());
//                                    s << strbuff;
//                                    strbuffread.clear();
//                                }
//                            }


                            std::string str_bff;

                            if (!(s >> str_bff))
                            {
                                s.close();
                                FD_CLR(i, &m_all_fdset);
                                std::cout << "one client offline" << std::endl;
                                continue;
                            }
                            std::cout << str_bff << std::endl;

                            if (std::string::npos != str_bff.find("exit"))
                            {
                                s.close();
                                FD_CLR(i, &m_all_fdset);
                                std::cout << "one client exit" << std::endl;
                                continue;
                            }

                            std::string strbuff;
                            std::string strinput("server recieve client!");
                            SimpleTransDataUtil::build_trans_data(strbuff,
                                                                  strinput.c_str(),
                                                                  strinput.size());
                            s << strbuff;
                            s.m_sock = -1;
                        }
                    }
                }
            }
            else
            {
                if (errno == EINTR)
                {
                    //忽略此信号
                }
                else
                {
                    std::cout << "polling_select()err:" << std::strerror(errno) << std::endl;
                    break;
                }
            }
        }
    }

    /**
    * use poll polling
    *
    */
    void polling_poll()
    {
        std::vector<pollfd> vec_pollfd;
        pollfd pfd_accept = {0};
        pfd_accept.fd = m_sock;
        pfd_accept.events = POLLIN;
        vec_pollfd.push_back(pfd_accept);

        while (true)
        {
            int nready = ::poll(&vec_pollfd[0], vec_pollfd.size(), -1);
            if (nready < 0)
            {
                if (errno == EINTR)
                {
                    //忽略此信号
                    continue;
                }
                else
                {
                    std::cout << "polling_poll()err:" << std::strerror(errno) << std::endl;
                    break;
                }
            }


            if (vec_pollfd[0].revents & POLL_IN)
            {
                int addr_length = sizeof(m_addr);
                int socket_data = ::accept(m_sock, (sockaddr *) &m_addr, (socklen_t *) &addr_length);

                if (socket_data <= 0)
                {
                    std::cout << "polling_select()accept err:" << std::strerror(errno) << std::endl;
                    continue;
                }

                pollfd pfd_rwfd = {0};
                pfd_rwfd.fd = socket_data;
                pfd_rwfd.events = POLL_IN;
                vec_pollfd.push_back(pfd_rwfd);
                continue;
            }


            for (auto item = vec_pollfd.begin(); item != vec_pollfd.end(); ++item)
            {
                if ((item->revents & POLL_IN) && item->fd != m_sock)
                {
                    ServerSocket s;
                    s.m_sock = item->fd;

                    std::string str_bff;

                    if (!(s >> str_bff))
                    {
                        s.close();
                        item->fd = -1;

                        std::cout << "one client offline" << std::endl;
                        continue;
                    }
                    std::cout << str_bff << std::endl;

                    if (std::string::npos != str_bff.find("exit"))
                    {
                        s.close();
                        item->fd = -1;
                        std::cout << "one client exit" << std::endl;
                        continue;
                    }

                    std::string strbuff;
                    std::string strinput("server recieve client!");
                    SimpleTransDataUtil::build_trans_data(strbuff,
                                                          strinput.c_str(),
                                                          strinput.size());
                    s << strbuff;
                    s.m_sock = -1;
                }
            }
        }


    }


private:
};

#endif //SIMPLESOCKET_SERVERSOCKET_H
