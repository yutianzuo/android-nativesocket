//
// Created by yutianzuo on 2019/11/8.
//

#ifndef TESTPROJ_STLUTILS_H
#define TESTPROJ_STLUTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <map>
#include <functional>

class STLUtils
{
public:
    template<class CONTAINER>
    static void delete_from_container(CONTAINER &vec,
                                      const std::function<bool(const typename CONTAINER::value_type &value)> &condition)
    {
        for (auto it = vec.begin(); it != vec.end();)
        {
            if (condition(*it))
            {
                auto it_tmp = vec.erase(it);
                it = it_tmp;
            }
            else
            {
                ++it;
            }
        }
    }

    /**
     * find the first character which between U+10000-U+10FFFF
     * return it's index
     * if none return -1.
     */
    static int utf16_sequence_has_32bits_emoji(const std::u16string &utf16_string)
    {
        int current_index = 0;
        for (auto character : utf16_string)
        {
            //utf16,first character between 0xD800-0xDBFF, possibly a unicode in U+10000-U+10FFFF
            if ((character & 0xffff) >= 0xd800 && (character & 0xffff) <= 0xdbff)
            {
                //insure the next character between 0xDC00-0xDFFF
                if (current_index + 1 < utf16_string.size())
                {
                    decltype(character) tmpchar = utf16_string.at(current_index + 1);
                    if ((tmpchar & 0xffff) >= 0xdc00 && (tmpchar & 0xffff) <= 0xdfff)
                    {
                        break;
                    }
                }
            }
            ++current_index;
        }
        return current_index >= utf16_string.size() ? -1 : current_index;
    }

    /**
     * find the first character which between U+10000-U+10FFFF
     * return it's index
     * if none return -1.
     */
    static int utf8_sequence_has_32bits_emoji(const std::string &u8_string)
    {
        int current_index = 0;
        for (auto character : u8_string)
        {
            //first character between 1111 0000 to 1111 0111
            if ((character & 0xff) >= 240 && (character & 0xff) <= 247)
            {
                if (current_index + 3 < u8_string.size())
                {
                    bool following_good = true;
                    //the following 3 characters between 1000 0000 to 1011 1111
                    for (int i = current_index + 1; i < current_index + 4; ++i)
                    {
                        decltype(character) tmpchar = u8_string.at(i);
                        if ((tmpchar & 0xff) >= 128 && (tmpchar & 0xff) <= 191)
                        {
                            continue;
                        }
                        else
                        {
                            following_good = false;
                            break;
                        }
                    }
                    if (following_good)
                    {
                        break;
                    }
                }
            }
            ++current_index;
        }
        return current_index >= u8_string.size() ? -1 : current_index;
    }
};

#endif //TESTPROJ_STLUTILS_H
