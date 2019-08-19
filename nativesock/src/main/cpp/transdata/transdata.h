//
// Created by yutianzuo on 2018/4/25.
//

#ifndef SIMPLESOCKET_TRANSDATA_H
#define SIMPLESOCKET_TRANSDATA_H

#include <string>
#include <cstdint>
#include "Crc32.h"

class SimpleTransDataUtil
{
public:
#define MAGICNUM (char)0x77

    //data_len < 0x80000000
    static void build_trans_data(std::string& buff, const char* data, std::int32_t data_len)
    {
        if (!data || data_len <= 0)
        {
            return;
        }
        //0--maigc,1-4--len,5-8--crc32,9--reserved
        std::int8_t head[10] = {0};
        head[0] = MAGICNUM;
        //little end
        std::int32_t n_len = data_len + 10;
        head[1] = (std::int8_t)(n_len & 0xff);
        head[2] = (std::int8_t)(n_len >> 8 & 0xff);
        head[3] = (std::int8_t)(n_len >> 16 & 0xff);
        head[4] = (std::int8_t)(n_len >> 24 & 0xff);

        std::uint32_t crc = crc32_bitwise(data, data_len);

        head[5] = (std::int8_t)(crc & 0xff);
        head[6] = (std::int8_t)(crc >> 8 & 0xff);
        head[7] = (std::int8_t)(crc >> 16 & 0xff);
        head[8] = (std::int8_t)(crc >> 24 & 0xff);

        head[9] = 0; //reserved

        buff.assign((char*)head, 10);
        buff.append(data, data_len);
    }

    static bool check_data(std::string& str_buff)
    {
        if (str_buff.size() < 10)
        {
            return false;
        }

        std::int8_t magic = str_buff[0];
        if (magic != MAGICNUM)
        {
            return false;
        }

        std::int32_t len =
                ((std::int32_t)str_buff[1] & 0xff) |
                (((std::int32_t)str_buff[2] & 0xff) << 8) |
                (((std::int32_t)str_buff[3] & 0xff) << 16) |
                (((std::int32_t)str_buff[4] & 0xff) << 24);
        if (len != str_buff.size())
        {
            return false;
        }

        std::uint32_t crc32 = ((std::uint32_t) str_buff[5] & 0xff) |
                              (((std::uint32_t) str_buff[6] & 0xff) << 8) |
                              (((std::uint32_t) str_buff[7] & 0xff) << 16) |
                              (((std::uint32_t) str_buff[8] & 0xff) << 24);

        std::uint32_t crc = crc32_bitwise(str_buff.c_str() + 10, str_buff.size() - 10);

        if (crc != crc32)
        {
            return false;
        }
        str_buff.assign((str_buff.c_str() + 10), str_buff.size() - 10);
        return true;
    }
};

#endif //SIMPLESOCKET_TRANSDATA_H
