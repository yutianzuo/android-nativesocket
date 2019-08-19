//
// Created by yutianzuo on 2019-05-21.
//

#ifndef SIMPLESOCKET_TRANSFILETCPRECV_H
#define SIMPLESOCKET_TRANSFILETCPRECV_H


#include "../socket/socketbase.h"
#include "../toolbox/string_x.h"
#include "datadef.h"

class TransFileTcpRecv final : public SocketBase
{
public:
#if(__cplusplus >= 201103L)

    TransFileTcpRecv(const TransFileTcpRecv &) = delete;

    TransFileTcpRecv(TransFileTcpRecv &&s) : SocketBase(std::move(s))
    {
        m_piece_info = s.m_piece_info;
    }

    void operator=(const TransFileTcpRecv &) = delete;

    void operator=(TransFileTcpRecv &&) = delete;

#endif

    TransFileTcpRecv() : TransFileTcpRecv(-1)
    {}

    explicit TransFileTcpRecv(int sock) : SocketBase(sock)
    {
        m_piece_info = nullptr;
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

    picec_info* get_piece_info()
    {
        return m_piece_info;
    }

    bool piece_neg_ok() //piece negotiate, begin file offset, transfer lenth.
    {
        return m_piece_info != nullptr;
    }

    void init_piece_info(picec_info* p)
    {
        m_piece_info = p;
    }
private:
    picec_info* m_piece_info;

};

#endif //SIMPLESOCKET_TRANSFILETCPRECV_H
