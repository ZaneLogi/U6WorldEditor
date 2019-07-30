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

    switch (type)
    {
    case lib_16:
    case lib_32:
        m_current_stream->seekg(0, std::ios_base::end);
        m_filesize = (uint32_t)m_current_stream->tellg();
        m_current_stream->seekg(0, std::ios_base::beg);
        break;
    case s_lib_16:
    case s_lib_32:
        m_current_stream->read((char*)&m_filesize, sizeof(m_filesize));
        break;
    default:
        close();
        return false;
    }

    m_type = type;

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

std::istream* Library::get_item(int index)
{
    if (nullptr == m_current_stream)
        return nullptr;

    switch (m_type)
    {
    case lib_16:
    {
        m_current_stream->seekg(index * sizeof(short), std::ios_base::beg);
        unsigned short off = 0;
        m_current_stream->read((char*)&off, sizeof(off));
        if (off == 0)
            return nullptr;
        m_current_stream->seekg(off, std::ios_base::beg);
        return m_current_stream;
    }
    break;
    case lib_32:
    {
        m_current_stream->seekg(index * sizeof(long), std::ios_base::beg);
        unsigned long off = 0;
        m_current_stream->read((char*)&off, sizeof(off));
        if (off == 0)
            return nullptr;
        m_current_stream->seekg(off, std::ios_base::beg);
        return m_current_stream;
    }
    break;
    case s_lib_16:
    {
        m_current_stream->seekg(4 + index * sizeof(short), std::ios_base::beg);
        unsigned short off = 0;
        m_current_stream->read((char*)&off, sizeof(off));
        if (off == 0)
            return nullptr;
        m_current_stream->seekg(off, std::ios_base::beg);
        return m_current_stream;
    }
    break;
    case s_lib_32:
    {
        m_current_stream->seekg(4 + index * sizeof(long), std::ios_base::beg);
        unsigned long off = 0;
        m_current_stream->read((char*)&off, sizeof(off));
        if (off == 0)
            return nullptr;
        m_current_stream->seekg(off, std::ios_base::beg);
        return m_current_stream;
    }
    break;
    default:
        close();
        return nullptr;
    }
}
