//
// Created by yutianzuo on 2019-05-21.
//

#ifndef SIMPLESOCKET_RECVMMAP_H
#define SIMPLESOCKET_RECVMMAP_H

#include <string>
#include <vector>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <mutex>
#include <iostream>
#include "../toolbox/miscs.h"
#include "../transdata/Crc32.h"

#include "datadef.h"

#define DATA_EXTRA ".data"
#define INFO_EXTRA ".info"

/**
 * when process exit, automaticlly unmap the addresses
 * so no need to munmap
 */
class RecvMmap final
{
public:
    enum
    {
        FILE_SIZE_THRESHOLD = 10 * 1024 * 1024,
        PIECES_DEFAULT = 4,
        INFO_VERSION = 1
    };

    RecvMmap(const std::string& dir) : m_filedir(dir)
    {
        if (!m_filedir.empty() && m_filedir[m_filedir.size() - 1] != '/')
        {
            m_filedir.append("/");
        }
    }
    bool init(const std::string& strmd5, std::uint64_t size, int pieces = PIECES_DEFAULT)
    {
        m_pieces = pieces;
        bool b_ret = true;
        FUNCTION_BEGIN;
        m_md5 = strmd5;
        m_size = size;
        m_name = m_md5 + DATA_EXTRA;
        m_info_name = m_md5 + INFO_EXTRA;
        if (dir_file_exist(get_full_file_path().c_str()) != 1 ||
                dir_file_exist(get_full_infofile_path().c_str()) != 1)//info not exist, first transfer
        {
            ::remove(get_full_infofile_path().c_str());
            ::remove(get_full_file_path().c_str());
            std::uint32_t piece_id = 0;
            long page_size = ::sysconf(_SC_PAGE_SIZE);
            long left = m_size % page_size;
            m_trunc_size = left > 0 ? m_size + (page_size - left) : m_size;
            long pages = m_trunc_size / page_size;
            int pieces = 1;
            if (m_trunc_size > FILE_SIZE_THRESHOLD) //> 10M, divide file
            {
                pieces = m_pieces;
            }
            int fd_data = ::open(get_full_file_path().c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd_data < 0)
            {
                b_ret = false;
                FUNCTION_LEAVE;
            }
            if (::ftruncate(fd_data, m_size) < 0)
            {
                ::close(fd_data);
                b_ret = false;
                FUNCTION_LEAVE;
            }
            std::uint64_t offset = 0;
            for (int i = 0; i < pieces; ++i)
            {
                std::uint64_t len = (pages / pieces) * page_size;
                if (i == pieces - 1)
                {
                    len = m_size - offset;
                }
                char* tmp = (char*)::mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, offset);
                if (tmp == MAP_FAILED)
                {
                    std::cerr << "mmap failed" << std::endl;
                }
                m_piece_infos.emplace_back(offset, len, len, piece_id++, tmp);
                offset += len;
            }
            ::close(fd_data);
            std::string str_out;
            serialize_data(str_out);
            write_info_file(str_out);
        }
        else //info exists, continue trans
        {
            long size = get_filesize(get_full_infofile_path().c_str());
            if (size <= 0 || size > FILE_SIZE_THRESHOLD)
            {
                b_ret = false;
                FUNCTION_LEAVE;
            }
            std::string strbuff;
            strbuff.resize(size);
            std::ifstream in_info(get_full_infofile_path().c_str());
            if (!in_info)
            {
                b_ret = false;
                FUNCTION_LEAVE;
            }
            in_info.read(&strbuff[0], strbuff.size());
            if (in_info.gcount() != size)
            {
                b_ret = false;
                FUNCTION_LEAVE;
            }
            deserialize_data(strbuff.c_str(), size, m_piece_infos);
            if (m_piece_infos.empty())
            {
                b_ret = false;
                FUNCTION_LEAVE;
            }
            int fd_data = ::open(get_full_file_path().c_str(), O_RDWR);
            if (fd_data < 0)
            {
                b_ret = false;
                FUNCTION_LEAVE;
            }
            if (::ftruncate(fd_data, m_size) < 0)
            {
                ::close(fd_data);
                b_ret = false;
                FUNCTION_LEAVE;
            }
            for (int i = 0; i < m_piece_infos.size(); ++i)
            {
                char* tmp = (char*)::mmap(nullptr, m_piece_infos[i].lenth, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd_data, m_piece_infos[i].begin_pos);
                if (tmp == MAP_FAILED)
                {
                    std::cerr << "mmap failed" << std::endl;
                }
                else
                {
                    tmp += (m_piece_infos[i].lenth - m_piece_infos[i].lenth_left);
                    m_piece_infos[i].mapping = tmp;
                }
            }
            ::close(fd_data);
        }
        FUNCTION_END;
        return b_ret;
    }
    std::vector<picec_info>& get_pieces_infos()
    {
        return m_piece_infos;
    }

    void uninit()
    {
        for (int i = 0; i < m_piece_infos.size(); ++i)
        {
            if (m_piece_infos[i].mapping != MAP_FAILED)
            {
                ::munmap(m_piece_infos[i].mapping - (m_piece_infos[i].lenth - m_piece_infos[i].lenth_left),
                        m_piece_infos[i].lenth);
            }
        }
    }

    void write_info_file(const std::string& str_out)
    {
        if (!str_out.empty())
        {
            std::ofstream output(get_full_infofile_path().c_str());
            output.write(str_out.c_str(), str_out.size());
            output.close();
        }
    }

    void serialize_data(std::string& str_ser_data)
    {
        int bufflen = sizeof(std::uint32_t) * 3; //crc32 + version + blocks
        bufflen += ((sizeof(std::uint64_t) * 3 + sizeof(std::uint32_t)) * m_piece_infos.size()); //blocks len
        // block data includes:id, pos, lentotal, lenleft
        str_ser_data.clear();
        str_ser_data.resize(bufflen, 0);
        char* pbuff = &str_ser_data[4];
        std::uint32_t tmp32 = INFO_VERSION;
        std::uint64_t tmp64 = 0;
        PUT_LE_32(tmp32, pbuff); //version
        pbuff += 4;
        tmp32 = m_piece_infos.size();
        PUT_LE_32(tmp32, pbuff); //blocks
        pbuff += 4;
        for (int i = 0; i < m_piece_infos.size(); ++i)
        {
            tmp32 = m_piece_infos[i].sid;
            PUT_LE_32(tmp32, pbuff);
            pbuff += 4;

            tmp64 = m_piece_infos[i].begin_pos;
            PUT_LE_64(tmp64, pbuff);
            pbuff += 8;

            tmp64 = m_piece_infos[i].lenth;
            PUT_LE_64(tmp64, pbuff);
            pbuff += 8;

            tmp64 = m_piece_infos[i].lenth_left;
            PUT_LE_64(tmp64, pbuff);
            pbuff += 8;
        }

        tmp32 = crc32_bitwise(&str_ser_data[4], str_ser_data.size() - 4);
        pbuff = &str_ser_data[0];
        PUT_LE_32(tmp32, pbuff);
    }

    picec_info* get_piece_info_by_sid(std::uint32_t sid)
    {
        for (int i = 0; i < m_piece_infos.size(); ++i)
        {
            if (m_piece_infos[i].sid == sid)
            {
                return &m_piece_infos[i];
            }
        }
        return nullptr;
    }

    void status(std::string& str_progress, bool& done, std::uint64_t& left_total)
    {
//        std::lock_guard<std::mutex> lock(m_mutex);
        std::uint64_t total = 0;
        std::uint64_t left = 0;
        for (auto it : m_piece_infos)
        {
            total += it.lenth;
            left += it.lenth_left;
        }
        done = (left == 0);
        double persent = (double)(total - left) / total;
        std::ostringstream output_persent;
        output_persent.precision(2);
        output_persent << std::fixed << persent * 100 << "%";
        str_progress = output_persent.str();
        left_total = left;
    }

    std::mutex& get_mutex()
    {
        return m_mutex;
    }

    std::string get_full_file_path()
    {
        if (m_filedir.empty())
        {
            return m_name;
        }
        else
        {
            return m_filedir + m_name;
        }
    }

    std::string get_dir()
    {
        return m_filedir;
    }

    std::string get_full_infofile_path()
    {
        if (m_filedir.empty())
        {
            return m_info_name;
        }
        else
        {
            return m_filedir + m_info_name;
        }
    }
private:
    std::string m_md5;
    std::string m_name;
    std::string m_info_name;
    std::string m_filedir;
    std::uint64_t m_size;
    std::uint64_t m_trunc_size;
    std::vector<picec_info> m_piece_infos;
    std::mutex m_mutex;
    int m_pieces;

};

#endif //SIMPLESOCKET_RECVMMAP_H
