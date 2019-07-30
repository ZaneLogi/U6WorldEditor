#pragma once
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <cassert>

class Configuration
{
public:
    bool load(const char* filename)
    {
        std::ifstream ifs(filename);
        if (!ifs.is_open())
            return false;

        ifs.seekg(0, std::ios::end);
        auto fsize = ifs.tellg();

        ifs.seekg(0, std::ios::beg);

        std::string content(fsize, 0);
        ifs.read(&content[0], fsize);
        ifs.close();

        load_string(content);
        return true;
    }

    std::string get_property(const char* property_name) const
    {
        auto i = m_properties.find(property_name);
        if (i != m_properties.end())
            return i->second;
        return std::string();
    }

    void set_property(const std::string& property_name, const std::string& property_value)
    {
        m_properties[property_name] = property_value;
    }

    std::string get_path(const char* filename)
    {
        std::string game_type = m_properties["game_type"];
        std::string root_path_key(game_type + "_root_path");
        assert(!game_type.empty());
        assert(!root_path_key.empty());
        auto i = m_properties.find(game_type + "_" + filename);
        if (i != m_properties.end())
            return m_properties[root_path_key] + "\\" + i->second;
        i = m_properties.find(filename);
        if (i != m_properties.end())
            return m_properties[root_path_key] + "\\" + i->second;

        return std::string();
    }

private:
    void load_string(const std::string file_content)
    {
        std::istringstream stream(file_content);
        load_stream(stream);
    }

    void load_stream(std::istringstream& stream)
    {
        std::regex expression(s_regex_string, std::regex_constants::icase);

        std::string line;
        while (std::getline(stream, line))
        {
            std::smatch match;
            if (std::regex_match(line, match, expression))
            {
                auto m1 = match[1];
                auto m2 = match[2];
                m_properties[m1] = m2;
            }
        }
    }
private:
    const std::string s_regex_string = R"(([a-z0-9]+[a-z0-9 ._]*[a-z0-9]+)\s*=\s*(.+)\s*)";
    std::map<std::string, std::string> m_properties;
};
