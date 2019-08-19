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



///////////////////////////////////
#ifndef NORMAL_ERROR_MSG
#define NORMAL_ERROR_MSG(msg) (std::ostringstream() << "error(" << msg << ") in " << __FILE__ << ", line:" << __LINE__).str()
#endif

#ifndef NORMAL_ERROR_NUM
#define NORMAL_ERROR_NUM(msg) (std::ostringstream() << "error(" << msg << ") errorno(" << std::strerror(errno) << ") in "\
<< __FILE__ << ", line:" << __LINE__).str()
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
    static std::string int32_to_string_addr(int addr)
    {
        in_addr addrin = {0};
        addrin.s_addr = addr;
        return stringxa(::inet_ntoa(addrin));
    }
};

#endif //SIMPLESOCKET_ERRORHUNTER_H
