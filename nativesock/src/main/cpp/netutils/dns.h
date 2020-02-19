//
// Created by yutianzuo on 2019-08-02.
//

#ifndef SIMPLESOCKET_DNS_H
#define SIMPLESOCKET_DNS_H

#include <sstream>

#include "../socket/simpleudpclient.h"
#include "../toolbox/timeutils.h"
#include "../toolbox/string_x.h"
#include "../toolbox/miscs.h"
#include "errorhunter.h"

#pragma pack(1)
struct DnsHeader
{
    std::int16_t trans_id;
    std::int8_t flags[2];
    std::int16_t questions;
    std::int16_t answer_rrs;
    std::int16_t authority_rrs;
    std::int16_t additional_rrs;
};
#pragma pack()


class DNSQuery final : public SimpleUdpClient
{
public:
    enum
    {
        DNS_UDP_PORT = 53,
    };
#if(__cplusplus >= 201103L)

    DNSQuery(const DNSQuery &) = delete;

    DNSQuery(DNSQuery &&s) noexcept : SimpleUdpClient(std::move(s))
    {
    }

    void operator=(const DNSQuery &) = delete;

    void operator=(DNSQuery &&) = delete;

#endif

    DNSQuery() = default;

    ~DNSQuery() override = default;

    ///by defualt, first invoke get_ip_by_api use system default dns server
    ///if got nothing, then invoke get_ip_by_host search ip by a specific dns server
    ///if api_first == false, then reverse invoke
    bool combine_search_ip_by_host(const std::string &str_host_query, std::vector<int> &ips,
            bool api_first = true,  const std::string& str_dns_ip = "119.29.29.29", int retry_times = 50)
    {
        bool ret = false;
        FUNCTION_BEGIN;
        if (api_first)
        {
            ret = get_ip_by_api(str_host_query, ips);
            if (!ips.empty())
            {
                FUNCTION_LEAVE;
            }
            ret = get_ip_by_host(str_dns_ip, str_host_query, ips, retry_times);
        }
        else
        {
            ret = get_ip_by_host(str_dns_ip, str_host_query, ips, retry_times);
            if (!ips.empty())
            {
                FUNCTION_LEAVE;
            }
            ret = get_ip_by_api(str_host_query, ips);
        }
        FUNCTION_END;
        return ret;
    }

    ///get ip by default invoke getaddrinfo use default dnshost, block func
    bool get_ip_by_api(const std::string &str_host_query, std::vector<int> &ips)
    {
        static const int len = 128;
        bool ret = false;
        FUNCTION_BEGIN;
            addrinfo *answer, hint, *curr;
            char ipstr[len] = {0};
            memset(&hint, 0, sizeof(hint));
            hint.ai_family = AF_INET; //ipv4 only by far
            hint.ai_socktype = SOCK_STREAM;
            hint.ai_flags = AI_CANONNAME;

            int addr_result = getaddrinfo(str_host_query.c_str(), nullptr, &hint, &answer);
            if (addr_result != 0)
            {
                FUNCTION_LEAVE;
            }
            for (curr = answer; curr != nullptr; curr = curr->ai_next)
            {
//                memset(ipstr, 0, len);
//                inet_ntop(curr->ai_family, &(((struct sockaddr_in *) (curr->ai_addr))->sin_addr), ipstr, len);
//                stringxa tmp = stringxa(ipstr);
//                if (!tmp.empty())
//                {
//                    ips.emplace_back(std::move(tmp));
//                }
                ips.emplace_back(((struct sockaddr_in *) (curr->ai_addr))->sin_addr.s_addr);
            }
            freeaddrinfo(answer);

            ret = true;
        FUNCTION_END;
        return ret;
    }

    ///query dns server, only A records; block func
    bool get_ip_by_host(const std::string &str_dnsip, const std::string &str_host_query,
            std::vector<int> &ips, int times = 50/*retry times, per time 100 millis, default is 50 * 100millis*/)
    {
        bool ret = false;
        FUNCTION_BEGIN ;
            if (!SimpleUdpClient::init_unconnected(str_dnsip, DNS_UDP_PORT))
            {
                m_last_errmsg = NORMAL_ERROR_MSG("init_unconnected failed");
                FUNCTION_LEAVE;
            }
            SimpleUdpClient::set_check_data(false);
            SimpleUdpClient::set_non_blocking(true);
            std::string str_query = dns_query(str_host_query);
            if (str_query.empty())
            {
                m_last_errmsg = NORMAL_ERROR_MSG("construct query failed, possibly domain label too long");
                FUNCTION_LEAVE;
            }
            if (!SimpleUdpClient::operator<<(str_query))
            {
                m_last_errmsg = NORMAL_ERROR_MSG("send msg failed");
                FUNCTION_LEAVE;
            }

            ////
            int counter = 0;
            std::string response;
            while (counter++ < times) //5000millis max
            {
                if (SimpleUdpClient::operator>>(response))
                {
                    break;
                }
                TimeUtils::sleep_for_millis(100);
            }

            if (!response.empty())
            {
                ret = analyze(response, ips);
            }
            else
            {
                m_last_errmsg = NORMAL_ERROR_MSG("response is empty");
            }
        FUNCTION_END;
        return ret;
    }

private:
    int trans_id = 0;
    std::vector<int> ipv4s;

    ///construct simple dns query request
    std::string dns_query(std::string const &str_host)
    {
        std::string request;
        const int buff_len = 256;
        char buff[buff_len] = {0};
        int index = 0;

        ::srand(::time(nullptr));
        trans_id = rand();

        stringxa strx_host(str_host);
        strx_host.trim();
        std::vector<stringxa> hosts;
        strx_host.split_string(".", hosts);

        ///header
        DnsHeader header = {0};
        header.trans_id = htons((std::int16_t)trans_id);

        //Do query recursively
        header.flags[0] = 1;
        header.flags[1] = 0;//flags  0 0000 0 1 0000 0000

        header.questions = htons(1);

        header.answer_rrs = htons(0);

        header.authority_rrs = htons(0);

        header.additional_rrs = htons(0);

        memcpy(buff, &header, sizeof(header));
        index += sizeof(header);
        ///header end

        ///queries
        for (const auto &host : hosts)
        {
            if (host.size() > 0x3f) //00xx xxxx domain label MUST NOT > 0x3f(0011 1111)
            {
                return "";
            }

            buff[index++] = host.size();
            for (auto c : host)
            {
                buff[index++] = c;
            }
        }
        index++; //end with a ZERO

        buff[index++] = 0;
        buff[index++] = 1; //type A --> 0x0001

        buff[index++] = 0;
        buff[index++] = 1; //class IN --> 0x0001

        ///queries end

        request.assign(buff, index > buff_len ? buff_len : index);
        return request;
    }
//////////////////////////////////////////////////////////////////////////////////////////////

    ///analyze response
    bool analyze(const std::string &str_response, std::vector<int> &str_ips)
    {
        bool ret = false;
        FUNCTION_BEGIN ;
            int index = 0;
            int answers = 0;
            int authority_rrs = 0;
            int add_rrs = 0;
            ///header
            if (!analyze_header(index, str_response, answers, authority_rrs, add_rrs))
            {
                FUNCTION_LEAVE;
            }
            ///header end

            ///queries
            std::string str_host;
            if (!analyze_queries(index, str_response, str_host))
            {
                m_last_errmsg = NORMAL_ERROR_MSG("analyze_queries failed");
                FUNCTION_LEAVE;
            }
            ///queries end

            ///answers
            if (answers > 0)
            {
                if (!analyze_rrs(index, str_response, answers))
                {
                    m_last_errmsg = NORMAL_ERROR_MSG("analyze_answers failed");
                    FUNCTION_LEAVE;
                }
                str_ips.swap(ipv4s);
            }
            ///answers end

//            if (authority_rrs > 0)
//            {
//                analyze_rrs(index, str_response, authority_rrs);
//            }
//
//            if (add_rrs > 0)
//            {
//                analyze_rrs(index, str_response, add_rrs);
//            }

            ret = true;
        FUNCTION_END;
        return ret;
    }

    int calc_message_name_size(const std::string &str_response, int index)
    {
        if (!check_index(index, str_response))
        {
            return INT_MAX;
        }

        //caution:
        //this equals 0xffff ffc0 & 0x0000 00c0 -> 0x000000c0 -> > 0 -> true
        if ((str_response[index] & 0xc0) == 0xc0) //11xx xxxx-->represents a pointer
        {
            return 2;
        }
        else //this may be a label sequence, or label sequence + pointer(pointer must be the end of domain name)
        {
            int count = 0;
            auto c = str_response[index];
            while (true)
            {
                ++count;
                if (c == 0) // a label sequence
                {
                    break;
                }
                else if ((c & 0xc0) == 0xc0) //end with a pointer
                {
                    ++count;
                    break;
                }
                if (!check_index(index + 1, str_response))
                {
                    return INT_MAX;
                }
                c = str_response[++index];
            }
            return count;
        }
    }

    bool analyze_rrs(int &index, const std::string &str_response, int answers)
    {
        bool ret = false;
        int index_inner = index;
        FUNCTION_BEGIN ;
            if (!check_index(index_inner, str_response))
            {
                FUNCTION_LEAVE;
            }

            //2bytes-->Name; 2bytes-->type; 2bytes-->class; 4bytes-->time; 2bytes-->lenth; 4bytes-->ipv4;
            //answer的最短字节数为16，as above shows，Name是可变的，遵循message compression
            //The compression scheme allows a domain name in a message to be
            //represented as either:
            //
            //   - a sequence of labels ending in a zero octet
            //
            //   - a pointer
            //
            //   - a sequence of labels ending with a pointer
            //The pointer takes the form of a two octet sequence:
            //
            //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //    | 1  1|                OFFSET                   |
            //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //The first two bits are ones.  This allows a pointer to be distinguished
            //from a label, since the label must begin with two zero bits because
            //labels are restricted to 63 octets or less.  (The 10 and 01 combinations
            //are reserved for future use.)  The OFFSET field specifies an offset from
            //the start of the message (i.e., the first octet of the ID field in the
            //domain header).  A zero offset specifies the first byte of the ID field,
            //etc.
            //more:https://tools.ietf.org/rfc/rfc1035.txt
            if (index_inner + answers * 16 > str_response.size()) //check A record * count bytes
            {
                m_last_errmsg = NORMAL_ERROR_MSG("answer's size error");
                FUNCTION_LEAVE;
            }

            for (int i = 0; i < answers; ++i)
            {
                index_inner += calc_message_name_size(str_response, index_inner); //jump name

                if (!check_index(index_inner, str_response))
                {
                    continue;
                }

                std::uint16_t type = ntohs(* ((std::int16_t*) &str_response[index_inner]));//type
                index_inner += 2;

                if (!check_index(index_inner, str_response))
                {
                    continue;
                }
                std::uint16_t query_class = ntohs(*((std::int16_t*) &str_response[index_inner]));//class
                index_inner += 2;

                index_inner += 4; //jump live time

                if (!check_index(index_inner, str_response))
                {
                    continue;
                }

                std::uint16_t len = ntohs(* ((std::int16_t*) &str_response[index_inner]));//length
                index_inner += 2;
                if (query_class != 1) //NOT IN type, no encounter yet
                {
                    index_inner += len;
                    continue;
                }

                if (!check_index(index_inner, str_response))
                {
                    continue;
                }

                if (type == 1) //A-> ipv4 addr
                {
                    if (len != 4)
                    {
                        index_inner += len;
                        continue;
                    }
                    int ip = *((int *) (&str_response[index_inner]));
                    ipv4s.emplace_back(ip);
                }
                else if (type == 2) //Authoritative Name Server
                {
                    std::string str_host_name;
                    int result = get_message_name(str_response, index_inner, str_host_name);
                    std::cout<< "host name is:" << str_host_name << std::endl;
                    if (result - index_inner != len)
                    {
                        std::cout<< "host name data len error" << std::endl;
                    }
                    else
                    {
                        std::cout<< "host name data len right" << std::endl;
                    }
                }
                else if (type == 5) //CNAME
                {
                    std::string str_cname;
                    int result = get_message_name(str_response, index_inner, str_cname);
                    std::cout<< "cname is:" << str_cname << std::endl;
                    if (result - index_inner != len)
                    {
                        std::cout<< "cname data len error" << std::endl;
                    }
                    else
                    {
                        std::cout<< "cname data len right" << std::endl;
                    }
                }
                else if (type == 28) //AAAA->ipv6
                {
                    ///
                }
                else if (type == 6) //SOA start of a zone of authority
                {
                    ///
                }
                else //other type, just jump
                {
                    index_inner += len;
                    continue;
                }
                index_inner += len;
            }
            index = index_inner;
            ret = true;
        FUNCTION_END;
        return ret;
    }

    int get_message_name(const std::string &str_response, int index_inner, std::string &str_host)
    {
        if ((str_response[index_inner] & 0xc0) == 0xc0) //just a pointer
        {
            int index_pointer = (((~0xc0 & 0xff) & str_response[index_inner]) << 8) |
                                (str_response[index_inner + 1] & 0xff);
            get_message_name(str_response, index_pointer, str_host);
            index_inner += 2;
        }
        else //labels OR labels ends with a pointer
        {
            int jump = 0;
            while (true)
            {
                if (!check_index(index_inner, str_response))
                {
                    index_inner = str_response.size();
                    break;
                }
                jump = str_response[index_inner++];
                if ((jump & 0xc0) == 0xc0)
                {
                    int index_pointer = (((~0xc0 & 0xff) & jump) << 8) |
                                        (str_response[index_inner] & 0xff);
                    get_message_name(str_response, index_pointer, str_host);
                    ++index_inner;
                    break;
                }
                else
                {
                    if (jump == 0 )
                    {
                        break;
                    }
                    else if (index_inner + jump <= str_response.size())
                    {
                        str_host.empty() ? "" : str_host.append(".");
                        str_host.append(&str_response[index_inner], jump);
                    }
                    index_inner += jump;
                }
            }
        }
        return index_inner;
    }

    bool analyze_queries(int &index, const std::string &str_response, std::string &str_host)
    {
        bool ret = false;
        int index_inner = index;
        str_host.clear();
        FUNCTION_BEGIN ;
            if (!check_index(index_inner, str_response))
            {
                FUNCTION_LEAVE;
            }

            index_inner = get_message_name(str_response, index_inner, str_host);

            if (!check_index(index_inner + 4, str_response))
            {
                FUNCTION_LEAVE;
            }

            std::uint16_t type = ntohs(* ((std::int16_t*) &str_response[index_inner]));//type shoud be an A(ipv4)
            index_inner += 2;
            std::uint16_t query_class = ntohs(*((std::int16_t*) &str_response[index_inner]));//class should be IN
            index_inner += 2;

            if (type != 1 || query_class != 1)
            {
                FUNCTION_LEAVE;
            }
            index = index_inner;
            ret = true;
        FUNCTION_END;
        return ret;
    }

    bool analyze_header(int &index, const std::string &str_response, int &answers, int &authority, int& additional)
    {
        bool ret = false;
        int index_inner = index;
        FUNCTION_BEGIN ;
            if (str_response.size() < sizeof(DnsHeader))
            {
                FUNCTION_LEAVE;
            }
            DnsHeader* header = (DnsHeader*)(&str_response[index_inner]);
            if ((std::int16_t)(ntohs(header->trans_id)) != (std::int16_t)trans_id)
            {
                m_last_errmsg = NORMAL_ERROR_MSG("transid not match");
                FUNCTION_LEAVE;
            } //check trans_id

            if (!analyze_flags((const char*)header->flags))
            {
                FUNCTION_LEAVE;
            } //check flags

            int questions = ntohs(header->questions);
            if (questions != 1)
            {
                m_last_errmsg = NORMAL_ERROR_MSG("question count is not 1");
                FUNCTION_LEAVE;
            }

            answers = ntohs(header->answer_rrs);
            authority = ntohs(header->authority_rrs);
            additional = ntohs(header->additional_rrs);

            index_inner += sizeof(DnsHeader); //authority rrs, additional rrs

            ret = true;
        FUNCTION_END;
        if (ret)
        {
            index = index_inner;
        }

        return ret;
    }

    bool analyze_flags(const char *buff)
    {
        bool ret = false;
        FUNCTION_BEGIN ;
            //x0000000
            int response_type = (buff[0] & 0x80) >> 7; //response type: 1,message is a response 0,message is a request
            if (response_type != 1)
            {
                m_last_errmsg = NORMAL_ERROR_MSG("flag is not a response");
                FUNCTION_LEAVE;
            }
            //0xxxx000
            int opcode =
                    (buff[0] & 0x78) >> 3; //opcode: 0,is a standard query 1,is a reverse query 2, server status request
            //00000x00
            int authorit_answer =
                    (buff[0] & 0x04) >> 2; //AA, Authoritative answer. 0,server is not an authority for domain
            //000000x0
            int truncated = (buff[0] & 0x02) >> 1; //TC, 0, message is not truncated. 1, truncated.
            //0000000x
            int recursion_desired = buff[0] & 0x01; //RD, 1,do query recursively 0, not recursively

            //x0000000
            int recursion_available = (buff[1] & 0x80) >> 7; //RA, 1,server can do recursive queries, 2, not
            //0x000000
            //Z(reserved 0)

            if (recursion_desired == 0 || (recursion_desired == 1 && recursion_available != 1))
            {
                m_last_errmsg = NORMAL_ERROR_MSG("server cant do recursive search");
                FUNCTION_LEAVE;
            }

            //00x00000
            int answer_authenticated =
                    (buff[1] & 0x20) >> 5; //Answer authenticated: 0, Answer/authority portion was not
            //authenticated by the server

            //000x0000
            int non_authenticated_data = (buff[1] & 0x10) >> 4; //0, Unacceptable

            //0000xxxx
            int reply_code = (buff[1] & 0x0f); //reply code: 0,no error 2,server failure 3,name error
            if (reply_code != 0)
            {
                std::string error = "reply code error:";
                error += std::to_string(reply_code);
                m_last_errmsg = NORMAL_ERROR_MSG(error);
                FUNCTION_LEAVE;
            }
            ret = true;
        FUNCTION_END;
        return ret;
    }


    bool check_index(int index, const std::string &str_response)
    {
        if (index < 0 || (index >= 0 && index >= str_response.size()))
        {
            return false;
        }
        return true;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    void run_test_case()
    {
        int index1 = 0;
        int index2 = 12;
        int index3 = 18;
        std::string data = generate_name_test_data();

        int result = calc_message_name_size(data, index1);
        std::cout << "size1:" << result << std::endl;
        result = calc_message_name_size(data, index2);
        std::cout << "size2:" << result << std::endl;
        result = calc_message_name_size(data, index3);
        std::cout << "size3:" << result << std::endl;

        std::string str_host;
        result = get_message_name(data, index1, str_host);
        std::cout << "name1:" << str_host << " index:" << result << std::endl;
        str_host.clear();
        result = get_message_name(data, index2, str_host);
        std::cout << "name2:" << str_host << " index:" << result << std::endl;
        str_host.clear();
        result = get_message_name(data, index3, str_host);
        std::cout << "name3:" << str_host << " index:" << result << std::endl;

        data = generate_all_test_data();
        std::vector<int> ips;
        trans_id = 0x0662;
        analyze(data, ips);
    }

private:
    std::string generate_name_test_data()
    {
        ///3 type compression data
        std::string data;
        //F.ISI.APAR 1st, no compression, labels
        data.push_back(1); //offset 0
        data.push_back('F');
        data.push_back(3);
        data.push_back('I');
        data.push_back('S');
        data.push_back('I');
        data.push_back(4);//offset 6
        data.push_back('A');
        data.push_back('P');
        data.push_back('A');
        data.push_back('R');
        data.push_back(0);
        //FOO.F.ISI.APAR, lebels with labels(in which case, pointer MUST be the end of domain name)
        data.push_back(3); //ofsset 12
        data.push_back('F');
        data.push_back('O');
        data.push_back('O');
        data.push_back(0xc0); //pointer-> offset 0
        data.push_back(0);
        //APAR, only pointer
        data.push_back(0xc0); //offset 18, pointer->offset 6
        data.push_back(6);
        return data;
    }

    std::string generate_all_test_data()
    {
        std::string data("\x06\x62\x85\x80\x00\x01\x00\x01\x00\x03\x00\x09\x09\x79\x75\x74" \
                            "\x69\x61\x6e\x7a\x75\x6f\x03\x63\x6f\x6d\x00\x00\x01\x00\x01\xc0" \
                            "\x0c\x00\x01\x00\x01\x00\x02\xa3\x11\x00\x04\x41\x31\xdb\xe1\xc0" \
                            "\x0c\x00\x02\x00\x01\x00\x02\xa3\x00\x00\x0d\x03\x6e\x73\x31\x06" \
                            "\x64\x6e\x73\x6f\x77\x6c\xc0\x16\xc0\x0c\x00\x02\x00\x01\x00\x02" \
                            "\xa3\x00\x00\x06\x03\x6e\x73\x32\xc0\x3f\xc0\x0c\x00\x02\x00\x01" \
                            "\x00\x02\xa3\x00\x00\x06\x03\x6e\x73\x33\xc0\x3f\xc0\x3b\x00\x01" \
                            "\x00\x01\x00\x00\xa8\xc0\x00\x04\x68\xcf\x8d\x8a\xc0\x3b\x00\x01" \
                            "\x00\x01\x00\x00\x54\x60\x00\x04\xb9\x22\xd8\x9f\xc0\x3b\x00\x01" \
                            "\x00\x01\x00\x00\x7e\x90\x00\x04\xc6\xfb\x54\x10\xc0\x54\x00\x01" \
                            "\x00\x01\x00\x00\x54\x60\x00\x04\xa8\xeb\x4b\x34\xc0\x54\x00\x01" \
                            "\x00\x01\x00\x00\x7e\x90\x00\x04\x2d\x20\xed\x80\xc0\x54\x00\x01" \
                            "\x00\x01\x00\x00\xa8\xc0\x00\x04\x40\x20\x16\x64\xc0\x66\x00\x01" \
                            "\x00\x01\x00\x00\x54\x60\x00\x04\xd1\x8d\x27\x96\xc0\x66\x00\x01" \
                            "\x00\x01\x00\x00\xa8\xc0\x00\x04\x2d\x3f\x6a\x3f\xc0\x66\x00\x01" \
                            "\x00\x01\x00\x00\x7e\x90\x00\x04\x2d\x3f\x05\xea", 252);

        return data;
    }


};

#endif //SIMPLESOCKET_DNS_H
