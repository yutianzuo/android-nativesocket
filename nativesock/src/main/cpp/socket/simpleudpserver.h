//
// Created by yutianzuo on 2018/10/12.
//

#ifndef SIMPLESOCKET_UPDBRAODCASTSERVER_H
#define SIMPLESOCKET_UPDBRAODCASTSERVER_H

#include "socketbase.h"
#include <string.h>

class SimpleUdpServer : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    SimpleUdpServer(const SimpleUdpServer &) = delete;

    SimpleUdpServer(SimpleUdpServer &&s) : SocketBase(std::move(s))
    {
    }

    void operator=(const SimpleUdpServer &) = delete;

    void operator=(SimpleUdpServer &&) = delete;

#endif

    enum
    {
        SERVER_PORT = 19842
    };

    SimpleUdpServer() : SocketBase()
    {

    }

    virtual ~SimpleUdpServer()
    {
    }


    bool init(int port)
    {
        if (!create_udp()) //socket
        {
            close();
            return false;
        }

        if (!bind(port)) //bind
        {
            close();
            return false;
        }
        return true;
    }

    bool init_multicast(int port, const char* multicast_addr)
    {
        bool ret = init(port);
        if (ret)
        {
            ret = add_to_multicast(multicast_addr);
        }
        return ret;
    }

    bool operator>>(std::string &data)
    {
        return receivefrom(data) > 0;
    }

    bool operator<<(const std::string &strdata)
    {
        return ::sendto(m_sock, strdata.c_str(), strdata.size(), 0, (const sockaddr *) &m_udp_addr_peer,
                        sizeof(m_udp_addr_peer)) !=
               -1;
    }


};

#endif //SIMPLESOCKET_UPDBRAODCASTSERVER_H
