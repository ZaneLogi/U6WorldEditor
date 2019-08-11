#pragma once
#include <vector>
#include <string>
#include "Configuration.h"


class Script
{
public:
    bool init(Configuration& config);
    std::vector<uint8_t> get_context(uint32_t index);
    std::vector<uint8_t> get_script(uint32_t npc_index);
    std::string get_npc_name(uint32_t npc_index);

private:
    bool load_u6_script(Configuration& config);
    bool load_wou_script(Configuration& config);

private:
    std::string m_game_type;
    std::vector<std::vector<uint8_t>> m_scripts;
};

class ScriptInterpreter
{
public:
    ScriptInterpreter();

    void load(const std::vector<uint8_t>& script);

    enum status { END, WAIT, INPUT, INPUT_NUM };
    status run(const std::string& input, std::string& output);

    std::string format_script();

private:
    int32_t evaluate();

    void skip_text(const uint8_t* &p, const uint8_t* end);
    void skip_eval_block(const uint8_t* &p, const uint8_t* end);
    void skip_code_block(const uint8_t* &p, const uint8_t* end, int block_type);

    void collect_eval(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end);
    void collect_format(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end, int block_type);

private:
    std::vector<uint8_t> m_script;
    const uint8_t* m_current;
    const uint8_t* m_script_end;

    uint8_t m_npc_flags[256]; // fake

    int m_npc_id;
    std::string m_npc_name;
    std::string m_npc_look;

};
