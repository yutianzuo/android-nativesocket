//
// Created by yutianzuo on 2019-08-07.
//

#ifndef SIMPLESOCKET_ERRORHUNTER_H
#define SIMPLESOCKET_ERRORHUNTER_H

#include <sstream>
#include <string>
#include <iostream>

////////////////////////////////////
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


///////////////////////////////////
#include "../toolbox/string_x.h"


inline std::string get_error_info(const std::string& msg)
{
    std::ostringstream ss;
    ss << "error(";
    ss << msg;
    ss << ") in";
    ss << __FILE__;
    ss << ", line:";
    ss << __LINE__;
    return ss.str();
}

inline std::string get_errorno_info(const std::string& msg)
{
    std::ostringstream ss;
    ss << "error(";
    ss << msg;
    ss << ") errorno(";
    ss << std::strerror(errno);
    ss << ") in";
    ss << __FILE__;
    ss << ", line:";
    ss << __LINE__;
    return ss.str();
}




///////////////////////////////////
#ifndef NORMAL_ERROR_MSG
#define NORMAL_ERROR_MSG(msg) get_error_info(msg)
#endif

#ifndef NORMAL_ERROR_NUM
#define NORMAL_ERROR_NUM(msg) get_errorno_info(msg)
#endif

#ifndef STD_COUT_ERRORMSG
#define STD_COUT_ERRORMSG(msg) std::cout<< NORMAL_ERROR_MSG(msg) << std::endl
#endif

#ifndef STD_COUT_ERRORMSG_NUM
#define STD_COUT_ERRORMSG_NUM(msg) std::cout<< NORMAL_ERROR_NUM(msg) << std::endl
#endif

#ifndef STD_CERR_ERRORMSG
#define STD_CERR_ERRORMSG(msg) std::cerr << NORMAL_ERROR_MSG(msg) << std::endl
#endif

#ifndef STD_CERR_ERRORMSG_NUM
#define STD_CERR_ERRORMSG_NUM(msg) std::cerr << NORMAL_ERROR_NUM(msg) << std::endl
#endif

class NetHelper final
{
public:
    NetHelper() = delete;
    static std::string ipv4_to_string_addr(int addr)
    {
        in_addr addrin = {0};
        addrin.s_addr = addr;
        return stringxa(::inet_ntoa(addrin));
    }

    static std::string safe_ipv4_to_string_addr(int addr)
    {
        const int len = 32;
        char buff[len] = {0};
        return stringxa(::inet_ntop(AF_INET, &addr, buff, len));
    }

    static std::string safe_ipv6_to_string_addr(const in6_addr* addr)
    {
        const int len = 128;
        char buff[len] = {0};
        return stringxa(::inet_ntop(AF_INET6, addr, buff, len));
    }

};

#endif //SIMPLESOCKET_ERRORHUNTER_H
