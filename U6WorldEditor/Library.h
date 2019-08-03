#pragma once
#include <fstream>
#include <sstream>
#include <vector>

class Library
{
public:
    enum TYPE { lib_16, lib_32, s_lib_16, s_lib_32 };

public:
    Library(void);
    ~Library(void);

    bool open(const char* filename, TYPE type, bool bCompressed);
    void close();

    int item_count() const { return m_item_count; }
    std::istream* get_item(uint32_t index, int& size);

private:
    void parse_file();

private:
    std::ifstream           m_file;
    std::stringstream       m_decoded_stream;
    std::istream*           m_current_stream;

    uint32_t                m_filesize;
    uint32_t                m_item_count;
    std::vector<uint32_t>   m_item_offsets;
    std::vector<uint32_t>   m_item_sizes;

    TYPE                    m_type;
};