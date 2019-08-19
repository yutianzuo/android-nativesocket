//
// Created by yutianzuo on 2018/4/24.
//

#ifndef SIMPLESOCKET_CLIENTSOCKET_H
#define SIMPLESOCKET_CLIENTSOCKET_H

#include "socketbase.h"
#include <string.h>

class ClinetSocket : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    ClinetSocket(const ClinetSocket &) = delete;

    ClinetSocket(ClinetSocket &&) = delete;

    void operator=(const ClinetSocket &) = delete;

    void operator=(ClinetSocket &&) = delete;

#endif

    ClinetSocket() : SocketBase()
    {

    }

    virtual ~ClinetSocket()
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
