
// stdafx.cpp : source file that includes just the standard includes
// U6WorldEditor.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"


std::string format_string(const std::string fmt, ...)
{
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}

std::string format_hex_string(const uint8_t* p_src, const uint8_t* p_end)
{
    std::string result((p_end - p_src) * 3 + 1, ' ');
    char* p_dst = (char*)result.data();
    while (p_src < p_end)
    {
        sprintf_s(p_dst, 4, "%02x ", *p_src++);
        p_dst += 3;
    }
    *p_dst = '\0';
    return result;
}



