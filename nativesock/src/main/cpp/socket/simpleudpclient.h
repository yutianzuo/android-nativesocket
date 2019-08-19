//
// Created by yutianzuo on 2018/10/15.
//

#ifndef SIMPLESOCKET_SIMPLEUDPCLIENT_H
#define SIMPLESOCKET_SIMPLEUDPCLIENT_H

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include "socketbase.h"


class SimpleUdpClient : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    SimpleUdpClient(const SimpleUdpClient &) = delete;

    SimpleUdpClient(SimpleUdpClient &&s) noexcept : SocketBase(std::move(s))
    {
    }

    void operator=(const SimpleUdpClient &) = delete;

    void operator=(SimpleUdpClient &&) = delete;

#endif

    SimpleUdpClient() = default;

    ~SimpleUdpClient() override = default;

    bool init_multicast(int port, const char* addr)
    {
        if (!create_udp()) //socket
        {
            close();
            return false;
        }

        m_addr.sin_addr.s_addr = ::inet_addr(addr);
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);

        struct in_addr localInterface = {0};
        localInterface.s_addr = ::inet_addr("0.0.0.0");

        return (setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) == 0);
    }


    bool init_broadcast(int port)
    {
        if (!create_udp(1)) //socket
        {
            close();
            return false;
        }

        uint32_t broad_cast_addr = INADDR_BROADCAST;//limited broadcast 255.255.255.255

#ifndef __APPLE__ //macOS not work, linux, android ok

        ifreq ifrs[16];
        ifconf ifconfig = {0};
        ifconfig.ifc_len = sizeof(ifrs);
        ifconfig.ifc_buf = (caddr_t) ifrs;

        if (::ioctl(m_sock, SIOCGIFCONF, &ifconfig) != -1)
        {
            int len = ifconfig.ifc_len / sizeof(ifreq);

            for (int i = 0; i < len; ++i)
            {
                if (::ioctl(m_sock, SIOCGIFFLAGS, &ifrs[i]) != -1)
                {
                    uint16_t flags = (uint16_t) ifrs[i].ifr_flags;
                    if (flags & IFF_UP &&
                        flags & IFF_BROADCAST &&
                        flags & IFF_RUNNING)
                    {
                        if (::ioctl(m_sock, SIOCGIFBRDADDR, &ifrs[i]) != -1)
                        {
                            char *sz_broadcast = ::inet_ntoa(((sockaddr_in *) (&ifrs[i].ifr_broadaddr))->sin_addr);
                            std::cout << "broadcast addr:" << sz_broadcast << std::endl;
                            in_addr addr_tmp = {0};
                            ::inet_aton(sz_broadcast, &addr_tmp);
                            broad_cast_addr = addr_tmp.s_addr;
                            sz_broadcast = ::inet_ntoa(addr_tmp);
                            std::cout << "broadcast addr convert:" << sz_broadcast << std::endl;

                            if (broad_cast_addr != 0)
                            {
                                break; //just find the first one
                            }
                        }
                        else
                        {
                            std::cout << "error:" << std::strerror(errno) << std::endl;
                        }
                    }
                }
            }
        }
#else

        ifreq broad_cast_info = {0};
        strncpy(broad_cast_info.ifr_name, "en0", IF_NAMESIZE);
        if (::ioctl(m_sock, SIOCGIFFLAGS, &broad_cast_info) != -1)
        {
            uint16_t flags = (uint16_t) broad_cast_info.ifr_flags;
            if (flags & IFF_UP &&
                flags & IFF_BROADCAST &&
                flags & IFF_RUNNING)
            {
                if (::ioctl(m_sock, SIOCGIFBRDADDR, &broad_cast_info) != -1)
                {
                    char *sz_broadcast = ::inet_ntoa(((sockaddr_in *) (&broad_cast_info.ifr_broadaddr))->sin_addr);
                    std::cout << "broadcast addr:" << sz_broadcast << std::endl;
                    in_addr addr_tmp = {0};
                    ::inet_aton(sz_broadcast, &addr_tmp);
                    broad_cast_addr = addr_tmp.s_addr;
                    sz_broadcast = ::inet_ntoa(addr_tmp);
                    std::cout << "broadcast addr convert:" << sz_broadcast << std::endl;

                }
                else
                {
                    STDCERR("init_broadcast()err");
                }
            }
        }

#endif

        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = broad_cast_addr;
        m_addr.sin_port = htons (port);
        return true;
    }

    /**
     * peer to peer upd,unconnected
     * @param port
     * @return
     */
    bool init_unconnected(const std::string &str_host, int port)
    {
        if (!create_udp()) //socket
        {
            close();
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

        return true;
    }

    void reset_host_port(const std::string& str_ip, int port)
    {
        m_addr.sin_port = htons(port);
        ::inet_pton(AF_INET, str_ip.c_str(), &m_addr.sin_addr);
    }

    /**
     * peer to peer upd,connected
     * @param port
     * @return
     */
    bool init_connected(const std::string &str_host, int port)
    {
        if (!init_unconnected(str_host, port))
        {
            return false;
        }
        m_b_connected = true;


        sockaddr_in add_local = {0};
        add_local.sin_family = AF_INET;
        add_local.sin_addr.s_addr = INADDR_ANY;
        add_local.sin_port = htons (8899);

        //可选
        //通过bind指定端口
        //固定发送端口
        int n_ret = ::bind(m_sock, (struct sockaddr *) &add_local, sizeof(add_local));
        if (n_ret == -1)
        {
            STDCERR("bind()err");
        }

        //connect 可以将发送地址关联到socket，这样可以后续调用send，而不用调用sendto了。
        return ::connect(m_sock, (const sockaddr *) &m_addr, sizeof(m_addr)) == 0;
    }

    bool operator<<(const std::string &str_buff)
    {
        if (m_b_connected)
        {
            return ::send(m_sock, str_buff.c_str(), str_buff.size(), 0) != -1;
        }
        else
        {
            return ::sendto(m_sock, str_buff.c_str(), str_buff.size(), 0, (const sockaddr *) &m_addr, sizeof(m_addr)) !=
                   -1;
        }
    }

    bool operator>>(std::string &data)
    {
        return receivefrom(data) > 0;
    }

private:
    bool m_b_connected = false;
};

#endif //SIMPLESOCKET_SIMPLEUDPCLIENT_H
