#include "stdafx.h"
#include "Library.h"
#include "LZW.h"

Library::Library(void) : m_current_stream(nullptr)
{
}

Library::~Library(void)
{
    close();
}

bool Library::open(const char* filename, Library::TYPE type, bool bCompressed)
{
    close();

    m_file.open(filename, std::ios_base::in | std::ios_base::binary);
    if (!m_file.is_open())
    {
        return false;
    }

    if (bCompressed)
    {
        LZW().decode(m_file, m_decoded_stream, 8);
        m_current_stream = &m_decoded_stream;
    }
    else
    {
        m_current_stream = &m_file;
    }

    m_type = type;
    parse_file();

    return true;
}

void Library::close()
{
    if (m_file.is_open())
    {
        m_file.close();
    }

    m_decoded_stream.seekp(0);
    m_decoded_stream.seekg(0);

    m_current_stream = nullptr;
}

std::istream* Library::get_item(uint32_t index, int& size)
{
    if (nullptr == m_current_stream)
        return nullptr;

    if (index >= m_item_count)
    {
        size = -1;
        return nullptr;
    }

    if (m_item_sizes[index] == 0)
    {
        size = 0;
        return nullptr;
    }

    m_current_stream->seekg((m_item_offsets[index] & 0x00ffffff), std::ios_base::beg);
    size = m_item_sizes[index];
    return m_current_stream;
}

void Library::parse_file()
{
    m_current_stream->seekg(0, std::ios_base::end);
    m_filesize = (uint32_t)m_current_stream->tellg();
    m_current_stream->seekg(0, std::ios_base::beg);

    uint32_t modified_size = 0;
    if (m_type == s_lib_16 || m_type == s_lib_32)
    {
        uint32_t size;
        m_current_stream->read((char*)&size, sizeof(size));
        assert(size == m_filesize);

        modified_size = 4;
    }

    m_item_offsets.clear();
    m_item_sizes.clear();
    m_item_count = 0;

    if (m_type == lib_16 || m_type == s_lib_16)
    {
        uint32_t first_valid_item_offset = 0;
        // read the first item whose offset is not 0
        for (uint32_t i = modified_size; i < m_filesize; i += sizeof(uint16_t))
        {
            if (first_valid_item_offset != 0 && i >= first_valid_item_offset)
                break;

            uint16_t off = 0;
            m_current_stream->read((char*)&off, sizeof(off));
            m_item_offsets.push_back(off);
            if (m_item_count == 0 && off != 0)
            {
                first_valid_item_offset = off;
                m_item_count = (first_valid_item_offset - modified_size) / 2;
            }
        }
        // append the file size for the last item
        m_item_offsets.push_back(m_filesize);
    }
    else if (m_type == lib_32 || m_type == s_lib_32)
    {
        uint32_t first_valid_item_offset = 0;
        // read the first item whose offset is not 0
        for (uint32_t i = modified_size; i < m_filesize; i += sizeof(uint32_t))
        {
            if (first_valid_item_offset != 0 && i >= first_valid_item_offset)
                break;

            uint32_t off = 0;
            m_current_stream->read((char*)&off, sizeof(off));
            m_item_offsets.push_back(off);
            if (m_item_count == 0 && off != 0)
            {
                auto flag = (off >> 24);
                first_valid_item_offset = (off & 0x00ffffff);
                m_item_count = (first_valid_item_offset - modified_size) / 4;
            }
        }
        // append the file size for the last item
        m_item_offsets.push_back(m_filesize);
    }
    else
    {
        assert(false); // should not be here
    }

    assert(m_item_count > 0);

    for (uint32_t i = 0; i < m_item_count; i++)
    {
        auto flag = (m_item_offsets[i] >> 24);
        int offset = (m_item_offsets[i] & 0x00ffffff);
        if (offset == 0)
        {
            m_item_sizes.push_back(0);
        }
        else
        {
            int next_offset = 0;
            for (uint32_t j = i + 1; j < m_item_count + 1; j++)
            {
                if ((m_item_offsets[j] & 0x00ffffff) != 0)
                {
                    next_offset = (m_item_offsets[j] & 0x00ffffff);
                    break;
                }
            }

            m_item_sizes.push_back(next_offset - offset);
        }
    }
}