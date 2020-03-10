//
// Created by yutianzuo on 2018/4/24.
//

#ifndef SIMPLESOCKET_CLIENTSOCKET_H
#define SIMPLESOCKET_CLIENTSOCKET_H

#include "socketbase.h"
#include <string.h>

class ClientSocket : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    ClientSocket(const ClientSocket &) = delete;

    ClientSocket(ClientSocket &&) = delete;

    void operator=(const ClientSocket &) = delete;

    void operator=(ClientSocket &&) = delete;

#endif

    ClientSocket() : SocketBase()
    {

    }

    virtual ~ClientSocket()
    {

    }

    bool init(const std::string &host, int port)
    {
        if (!create()) //socket
        {
            close();
            return false;
        }

        if (!connect(host, port)) //connection
        {
            close();
            return false;
        }

        return true;
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

};

#endif //SIMPLESOCKET_CLIENTSOCKET_H
