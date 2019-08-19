//
// Created by yutianzuo on 2019-05-22.
//

#ifndef SIMPLESOCKET_TRANSFILETCPSENDER_H
#define SIMPLESOCKET_TRANSFILETCPSENDER_H


#include "../socket/socketbase.h"

class TransFileTcpSender final : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    TransFileTcpSender(const TransFileTcpSender &) = delete;

    TransFileTcpSender(TransFileTcpSender &&s) noexcept : SocketBase(std::move(s))
    {
    }

    void operator=(const TransFileTcpSender &) = delete;

    void operator=(TransFileTcpSender &&) = delete;
#endif

    TransFileTcpSender() : TransFileTcpSender(-1)
    {}

    explicit TransFileTcpSender(int sock) : SocketBase(sock)
    {}

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
    }

    bool operator<<(const std::string &buff)
    {
        return send(buff);
    }

private:
};
#endif //SIMPLESOCKET_TRANSFILETCPSENDER_H
