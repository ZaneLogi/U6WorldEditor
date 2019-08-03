#include "stdafx.h"
#include <fstream>
#include <sstream>
#include "Script.h"
#include "Library.h"
#include "LZW.h"

bool Script::init(Configuration& config)
{
    m_game_type = config.get_property("game_type");
    m_scripts.clear();

    if (m_game_type == "u6")
    {
        load_u6_script(config);
    }
    else
    {
        load_wou_script(config);
    }

    return true;
}

std::string Script::get_context(uint32_t index)
{
    if (index < m_scripts.size())
    {
        return m_scripts[index];
    }
    return std::string();
}

std::string Script::get_npc_name(uint32_t npc_index)
{
    int index = npc_index;
    if (m_game_type == "se")
    {
        index -= 2;
    }

    auto context = get_context(index);
    if (!context.empty())
    {
        assert(context[0] == -1 && context[1] == npc_index);
        auto p = &context[2];
        while (1)
        {
            if (*p == -15)
                break;
            p++;
        }
        return std::string(&context[2], (size_t)(p - &context[2]));
    }
    return std::string();
}

bool Script::load_u6_script(Configuration& config)
{
    auto script_filename_a = config.get_path("script_a");
    auto script_filename_b = config.get_path("script_b");

    std::string script_filenames[2] = { config.get_path("script_a") , config.get_path("script_b") };
    for (int i = 0; i < 2; i++)
    {
        Library lib;
        lib.open(script_filenames[i].c_str(), Library::TYPE::lib_32, false);
        auto item_count = lib.item_count();
        for (int i = 0; i < item_count; i++)
        {
            int item_size = 0;
            auto is = lib.get_item(i, item_size);
            if (is == nullptr)
            {
                // no script
                m_scripts.push_back(std::string());
                continue;
            }

            uint32_t size;
            is->read((char*)&size, sizeof(size));

            if (size == 0)
            {
                std::string tmp;
                tmp.resize(item_size);
                is->read((char*)tmp.data(), item_size);
                m_scripts.push_back(tmp);
            }
            else
            {
                // decode data
                std::stringstream decoded_stream;
                LZW().decode(*is, decoded_stream, 8);
                m_scripts.push_back(decoded_stream.str());
            }
        }
        lib.close();
    }

    return true;
}

bool Script::load_wou_script(Configuration& config)
{
    auto script_filename = config.get_path("script");

    Library lib;
    if (!lib.open(script_filename.c_str(), Library::TYPE::s_lib_32, false))
    {
        assert(false && "Couldn't open source file TALK.LZC!");
        return false;
    }

    auto item_count = lib.item_count();
    for (int i = 0; i < item_count; i++)
    {
        int item_size = 0;
        auto is = lib.get_item(i, item_size);
        if (is == nullptr)
        {
            // no script
            m_scripts.push_back(std::string());
            continue;
        }

        uint32_t size;
        is->read((char*)&size, sizeof(size));

        assert(size != 0);

        // decode data
        std::stringstream decoded_stream;
        LZW().decode(*is, decoded_stream, 8);
        m_scripts.push_back(decoded_stream.str());
    }

    lib.close();
    return true;
}