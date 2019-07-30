#pragma once
#include <fstream>
#include <sstream>

class Library
{
public:
    enum TYPE { lib_16, lib_32, s_lib_16, s_lib_32 };

private:
    std::ifstream       m_file;
    std::stringstream   m_decoded_stream;
    std::istream*       m_current_stream;

    uint32_t            m_filesize;

    TYPE                m_type;

public:
    Library(void);
    ~Library(void);

    bool open(const char* filename, TYPE type, bool bCompressed);
    void close();

    std::istream* get_item(int index);
};