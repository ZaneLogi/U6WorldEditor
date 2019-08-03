#pragma once
#include <vector>
#include <string>
#include "Configuration.h"

class Script
{
public:
    bool init(Configuration& config);
    std::string get_context(uint32_t index);
    std::string get_npc_name(uint32_t npc_index);

private:
    bool load_u6_script(Configuration& config);
    bool load_wou_script(Configuration& config);

private:
    std::string m_game_type;
    std::vector<std::string > m_scripts;
};