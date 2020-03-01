//
// Created by yutianzuo on 2018/4/24.
//

#ifndef SIMPLESOCKET_SOCKETBASE_H
#define SIMPLESOCKET_SOCKETBASE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <cstring>
#include "../transdata/transdata.h"

#define STDCERR(msg) std::cerr<<"("<<__FILE__<<"--line"<<std::to_string(__LINE__)<<\
")"<<msg<<":"<<std::strerror(errno)<<std::endl

#define STDCOUT(msg) std::cout<<msg<<std::endl


class SocketBase
{
public:
    enum
    {
        /**
         * bsd:
         *      The backlog argument defines the maximum length the queue of pending con-
         * nections may grow to.  The	real maximum queue length will be 1.5 times
         * more than the value specified in the backlog argument.  A subsequent
         * listen() system call on the listening socket allows the caller to change
         * the maximum queue length using a new backlog argument.  If	a connection
         * request arrives with the queue full the client may	receive	an error with
         * an	indication of ECONNREFUSED, or,	in the case of TCP, the	connection
         * will be silently dropped.
         *  Note that before FreeBSD 4.5 and the introduction of the syncache,	the
         * backlog argument also determined the length of the	incomplete connection
         * queue, which held TCP sockets in the process of completing	TCP's 3-way
         * handshake.	 These incomplete connections are now held entirely in the
         * syncache, which is	unaffected by queue lengths.  Inflated backlog values
         * to	help handle denial of service attacks are no longer necessary.
         */


        /**
         * linux:
         *        The backlog argument defines the maximum length to which the queue of
         * pending connections for sockfd may grow.  If a connection request
         * arrives when the queue is full, the client may receive an error with
         * an indication of ECONNREFUSED or, if the underlying protocol supports
         * retransmission, the request may be ignored so that a later reattempt
         * at connection succeeds.
         *          The behavior of the backlog argument on TCP sockets changed with
         * Linux 2.2.  Now it specifies the queue length for completely
         * established sockets waiting to be accepted, instead of the number of
         * incomplete connection requests.  The maximum length of the queue for
         * incomplete sockets can be set using
         * /proc/sys/net/ipv4/tcp_max_syn_backlog.  When syncookies are enabled
         * there is no logical maximum length and this setting is ignored.  See
         * tcp(7) for more information.
         * If the backlog argument is greater than the value in
         * /proc/sys/net/core/somaxconn, then it is silently truncated to that
         * value; the default value in this file is 128.  In kernels before
         * 2.4.25, this limit was a hard coded value, SOMAXCONN, with the value
         * 128.
         */

        MAXCONNECTIONS = 128,
        MAXRECEIVEBUFF = 65536 * 3    //if test, = 4
    };

    int close()
    {
        ::shutdown(m_sock, SHUT_RDWR);
        int n_ret = ::close(m_sock);
        m_sock = -1;
        return n_ret;
    }

    void set_non_blocking(bool b)
    {
        if (!is_valid_socket())
        {
            return;
        }
        int opts;
        opts = fcntl(m_sock, F_GETFL);
        if (opts < 0)
        {
            return;
        }
        if (b)
            opts = (opts | O_NONBLOCK);
        else
            opts = (opts & ~O_NONBLOCK);

        fcntl(m_sock, F_SETFL, opts);
    }

    void set_check_data(bool b_check = true)
    {
        b_check_data = b_check;
    }

#if(__cplusplus >= 201103L)

    SocketBase(const SocketBase &) = delete;

    SocketBase(SocketBase &&s) noexcept
    {
        this->m_sock = s.m_sock;
        memcpy(&this->m_addr, &s.m_addr, sizeof(s.m_addr));
        s.m_sock = -1;
        memset((void *) (&s.m_addr), 0, sizeof(s.m_addr));
        this->b_check_data = s.b_check_data;
    }

    void operator=(const SocketBase &) = delete;

    void operator=(SocketBase &&) = delete;

#endif
    bool is_valid_socket()
    { return m_sock != -1; }

    int get_socket()
    {
        return m_sock;
    }

    sockaddr_in get_addr()
    {
        return m_addr;
    }

    virtual ~SocketBase()
    {
        close();
    }

    const std::string& get_last_err_msg()
    {
        return m_last_errmsg;
    }
protected:
    std::string m_last_errmsg;
    SocketBase() : SocketBase(-1)
    {

    }

    SocketBase(int sock) : m_sock(sock), m_last_errmsg("no last err msg")
    {
        memset(&m_addr, 0, sizeof(m_addr));
    }



    //server
    bool create()
    {
        m_sock = ::socket(PF_INET, SOCK_STREAM, 0);
        if (!is_valid_socket())
        {
            STDCERR("socket()err");
            return false;
        }

        int on = 1;
        if (::setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) == -1)
        {
            STDCERR("setsockopt()err");
            close();
            return false;
        }

#if defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL)//MACOS || IOS
        if (::setsockopt(m_sock, SOL_SOCKET, SO_NOSIGPIPE, (const char *) &on, sizeof(on)) == -1)
        {
            STDCERR("setsockopt()pipe err");
            close();
            return false;
        }
#endif
        return true;
    }

    //upd broadcast server
    /*
     * type:
     * 0,default udp socket
     * 1,broadcast udp socket
     */
    bool create_udp(int type = 0)
    {
        m_sock = ::socket(PF_INET, SOCK_DGRAM, 0);
        if (!is_valid_socket())
        {
            STDCERR("setsockopt()err");
            return false;
        }

        int on = 1;
        if (::setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) == -1)
        {
            STDCERR("setsockopt()err");
            close();
            return false;
        }

        if (type == 1)
        {
            if (::setsockopt(m_sock, SOL_SOCKET, SO_BROADCAST, (const char *) &on, sizeof(on)) == -1)
            {
                STDCERR("setsockopt()err");
                close();
                return false;
            }
        }

        return true;
    }

    /**
     * 加入组播
     * @return
     */
    bool add_to_multicast(const char *multicast_ip)
    {
        struct ip_mreqn group = {0};
        inet_pton(AF_INET, multicast_ip, &group.imr_multiaddr);

        struct in_addr local = {0};
        local.s_addr = INADDR_ANY;
        char *local_str = ::inet_ntoa(local);

        std::cout << "local addr is:" << local_str << std::endl;

        inet_pton(AF_INET, local_str, &group.imr_address);
//        group.imr_ifindex = if_nametoindex("eth0");

        return (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) == 0);
    }

public:

    bool bind(int port)
    {
        if (!is_valid_socket())
        {
            return false;
        }

        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = INADDR_ANY;
        m_addr.sin_port = htons (port);

        int n_ret = ::bind(m_sock, (struct sockaddr *) &m_addr, sizeof(m_addr));
        if (n_ret == -1)
        {
            STDCERR("bind()err");
            close();
            return false;
        }

        return true;
    }

    bool listen()
    {
        if (!is_valid_socket())
        {
            return false;
        }
        int n_et = ::listen(m_sock, MAXCONNECTIONS);

        if (n_et == -1)
        {
            STDCERR("listen()err");
            close();
            return false;
        }

        return true;
    }

    bool accept(SocketBase &socket)
    {
        if (!is_valid_socket())
        {
            return false;
        }
        int addr_length = sizeof(m_addr);
        int socket_data = ::accept(m_sock, (sockaddr *) &m_addr, (socklen_t *) &addr_length);

        if (socket_data <= 0)
        {
            STDCERR("accept()err");
            if (errno != EINTR)
            {
                close();
            }
            return false;
        }
        else
        {
            sockaddr_in addr_bind_sock;
            int len_bind_sock = sizeof(addr_bind_sock);
            ::getsockname(m_sock, (sockaddr *) &addr_bind_sock, (socklen_t *) &len_bind_sock);
            std::cout << "listen sock addr:" << ::inet_ntoa(addr_bind_sock.sin_addr) << " port:"
                      << ntohs(addr_bind_sock.sin_port) << std::endl;

            sockaddr_in addr_data_sock;
            int len_data_sock = sizeof(addr_data_sock);
            ::getsockname(socket_data, (sockaddr *) &addr_data_sock, (socklen_t *) &len_data_sock);
            std::cout << "data sock host addr:" << ::inet_ntoa(addr_data_sock.sin_addr) << " port:"
                      << ntohs(addr_data_sock.sin_port) << std::endl;

            sockaddr_in addr_data_sock_peer;
            int len_data_sock_peer = sizeof(addr_data_sock_peer);
            ::getpeername(socket_data, (sockaddr *) &addr_data_sock_peer, (socklen_t *) &len_data_sock_peer);
            std::cout << "data sock peer addr:" << ::inet_ntoa(addr_data_sock_peer.sin_addr) << " port:"
                      << ntohs(addr_data_sock_peer.sin_port) << std::endl;


            socket.m_sock = socket_data;
            return true;
        }
    }


    //client
    bool connect(const std::string &str_host, int port)
    {
        if (!is_valid_socket())
        {
            STDCERR("connect()err");
            return false;
        }

        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);

        ::inet_pton(AF_INET, str_host.c_str(), &m_addr.sin_addr);

        if (errno == EAFNOSUPPORT)
        {
            STDCERR("errno == EAFNOSUPPORT");
            close();
            return false;
        }

        int n_ret = ::connect(m_sock, (sockaddr *) &m_addr, sizeof(m_addr));
        if (n_ret != 0)
        {
            STDCERR("connect()err");
        }
        return n_ret == 0;
    }


    bool connect_timeout(const std::string &str_host, int port, int secs)
    {
        if (!is_valid_socket())
        {
            STDCERR("connect()err");
            return false;
        }
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);

        ::inet_pton(AF_INET, str_host.c_str(), &m_addr.sin_addr);

        if (errno == EAFNOSUPPORT)
        {
            STDCERR("errno == EAFNOSUPPORT");
            close();
            return false;
        }

        set_non_blocking(true);
        int con_ret = ::connect(m_sock, (sockaddr *) &m_addr, sizeof(m_addr));
        if (con_ret == -1 && errno != EINPROGRESS)
        {
            STDCERR("connect_timeout--errno != EINPROGRESS");
            close();
            return false;
        }

        pollfd pfd_listen = {0};
        pfd_listen.fd = m_sock;
        pfd_listen.events = POLLIN | POLLOUT;
        int ret = ::poll(&pfd_listen, 1, secs * 1000);
        if (ret <= 0)
        {
            STDCERR("connect timeout OR error occured");
            close();
            return false;
        }

        set_non_blocking(false);
        return true;
    }


    //data trans
    bool send(const std::string &data)
    {
        if (!is_valid_socket())
        {
            return false;
        }
        int n_flags = 0;
#if defined(MSG_NOSIGNAL)
        n_flags = MSG_NOSIGNAL;
#endif
        int status = ::send(m_sock, data.c_str(), data.size(), n_flags);
        //std::cout << "send()status:" << status << std::endl;
        return status != -1;
    }

    int receive(std::string &buff)
    {
        if (!is_valid_socket())
        {
            return -1;
        }
        if (b_check_data)
        {
            buff = "";
        }
        char buf[MAXRECEIVEBUFF] = {0};
        char* pbuff = &buf[0];
        int buff_len = MAXRECEIVEBUFF;

        if (!b_check_data)
        {
            if (buff.size() == 0)
            {
                buff.resize(MAXRECEIVEBUFF);
            }
            pbuff = &buff[0];
            buff_len = buff.size();
        }

        ssize_t status;
        do
        {
            status = ::recv(m_sock, pbuff, buff_len, 0);
            if (status > 0)
            {
//                std::cout << "recv bytes is:" << status << std::endl;
                if (b_check_data)
                {
                    buff.append(pbuff, status);
                    //check data completion
                    if (SimpleTransDataUtil::check_data(buff))
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else if (status == 0)
            {
                STDCERR("receive()status == 0");
                break; //possible connection closed
            }
            else
            {
                STDCERR("receive()err");
                if (errno == EINTR)
                {
                    //忽略此信号
                    status = 0;
                }
            }
        } while (status != -1);
        return status;
    }


    sockaddr_in m_udp_addr_peer;

    int receivefrom(std::string &buff)
    {
        if (!is_valid_socket())
        {
            return -1;
        }
        buff.clear();
        socklen_t addr_len = sizeof(m_udp_addr_peer);
        bzero(&m_udp_addr_peer, sizeof(m_udp_addr_peer));
        ssize_t status = 0;
        char buf[MAXRECEIVEBUFF] = {0};
        do
        {
            status = ::recvfrom(m_sock, buf, MAXRECEIVEBUFF, 0, (sockaddr *) &m_udp_addr_peer, &addr_len);
//            std::cout << "datagram:" << ::inet_ntoa(m_udp_addr_peer.sin_addr) << " port:"
//                      << ntohs(m_udp_addr_peer.sin_port) << std::endl;
            if (status > 0)
            {
                buff.append(buf, status);
                break;
            }
            else
            {
                STDCERR("recvfrom()err");
                if (errno == EINTR)
                {
                    //忽略此信号
                    status = 0;
                }
            }
        } while (status != -1);

        return status;

    }

    bool b_check_data = true;
    int m_sock;
    sockaddr_in m_addr;
};


#endif //SIMPLESOCKET_SOCKETBASE_H
