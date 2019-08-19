//
// Created by yutianzuo on 2019-05-22.
//

#ifndef SIMPLESOCKET_DATADEF_H
#define SIMPLESOCKET_DATADEF_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define PUT_LE_32(int32value, pbuff) pbuff[0] = (std::int8_t)(int32value & 0xff);\
pbuff[1] = (std::int8_t)((int32value >> 8) & 0xff);\
pbuff[2] = (std::int8_t)((int32value >> 16) & 0xff);\
pbuff[3] = (std::int8_t)((int32value >> 24) & 0xff)

#define PUT_LE_64(int64value, pbuff) pbuff[0] = (std::int8_t)(int64value & 0x00000000000000ff);\
pbuff[1] = (std::int8_t)((int64value >> 8) & 0x00000000000000ff);\
pbuff[2] = (std::int8_t)((int64value >> 16) & 0x00000000000000ff);\
pbuff[3] = (std::int8_t)((int64value >> 24) & 0x00000000000000ff);\
pbuff[4] = (std::int8_t)((int64value >> 32) & 0x00000000000000ff);\
pbuff[5] = (std::int8_t)((int64value >> 40) & 0x00000000000000ff);\
pbuff[6] = (std::int8_t)((int64value >> 48) & 0x00000000000000ff);\
pbuff[7] = (std::int8_t)((int64value >> 56) & 0x00000000000000ff)


#define GET_LE_32(pbuff, int32value) int32value = ((std::int32_t)pbuff[0] & 0xff) |\
(((std::int32_t)pbuff[1] & 0xff) << 8) |\
(((std::int32_t)pbuff[2] & 0xff) << 16) |\
(((std::int32_t)pbuff[3] & 0xff) << 24)


#define GET_LE_64(pbuff, int64value) int64value = ((std::int64_t)pbuff[0] & 0x00000000000000ff) |\
(((std::int64_t)pbuff[1] & 0x00000000000000ff) << 8) |\
(((std::int64_t)pbuff[2] & 0x00000000000000ff) << 16) |\
(((std::int64_t)pbuff[3] & 0x00000000000000ff) << 24) |\
(((std::int64_t)pbuff[4] & 0x00000000000000ff) << 32) |\
(((std::int64_t)pbuff[5] & 0x00000000000000ff) << 40) |\
(((std::int64_t)pbuff[6] & 0x00000000000000ff) << 48) |\
(((std::int64_t)pbuff[7] & 0x00000000000000ff) << 56)


struct picec_info
{
    picec_info(std::uint64_t pos, std::uint64_t len, std::uint64_t len_left, std::uint32_t id, char* p) :
            begin_pos(pos), lenth(len), mapping(p), lenth_left(len_left), sid(id)
    {}
    std::uint64_t begin_pos; //file offset from the beginning of the file
    std::uint64_t lenth; //piece len total
    std::uint64_t lenth_left; //piece len left
    std::uint32_t sid; //picec id
    char* mapping = nullptr; //write pointer pos
    bool is_mapping_valid()
    {
        return mapping != (char*)-1;
    }
};

inline
void deserialize_data(const char* ser_data, int len, std::vector<picec_info>& vec_infos)
{
    if (len < sizeof(std::uint32_t) * 3)
    {
        STDCERR("len < 12");
        return;
    }
    char* pbuff = (char*)ser_data;
    std::uint32_t crc32;
    GET_LE_32(pbuff, crc32);
    pbuff += 4;
    std::uint32_t version;
    GET_LE_32(pbuff, version);
    pbuff += 4;
    std::uint32_t blocks;
    GET_LE_32(pbuff, blocks);
    pbuff += 4;

    int callen = sizeof(std::uint32_t) * 3 + (blocks * (sizeof(std::uint64_t) * 3 + sizeof(std::uint32_t)));
    if (len != callen)
    {
        STDCERR("len error");
        return;
    }

    std::uint32_t calcrc = crc32_bitwise(ser_data + 4, len - 4);
    if (calcrc != crc32)
    {
        STDCERR("crc failed");
        return;
    }

    std::uint32_t sid;
    std::uint64_t being_pos;
    std::uint64_t lenth;
    std::uint64_t lenth_left;
    vec_infos.clear();
    int i = 0;
    while (i++ < blocks)
    {
        GET_LE_32(pbuff, sid);
        pbuff += 4;
        GET_LE_64(pbuff, being_pos);
        pbuff += 8;
        GET_LE_64(pbuff, lenth);
        pbuff += 8;
        GET_LE_64(pbuff, lenth_left);
        pbuff += 8;
        vec_infos.emplace_back(being_pos, lenth, lenth_left, sid, (char*)MAP_FAILED);
    }
}

#endif //SIMPLESOCKET_DATADEF_H
