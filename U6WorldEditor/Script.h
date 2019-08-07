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
    std::string get_script(uint32_t npc_index);

private:
    bool load_u6_script(Configuration& config);
    bool load_wou_script(Configuration& config);

private:
    std::string m_game_type;
    std::vector<std::string> m_scripts;
};

class ScriptInterpreter
{
public:
    ScriptInterpreter(const std::string& script);

    enum status { END, WAIT, INPUT, INPUT_NUM };
    status run(const std::string& input, std::string& output);

private:
    int32_t evaluate();

    void skip_text(const char* &p, const char* end);
    void skip_eval_block(const char* &p, const char* end);
    void skip_if_block(const char* &p, const char* end, bool stop_on_else = false);
    void skip_answer_block(const char* &p, const char* end);


private:
    std::string m_script;
    const char* m_current;
    const char* m_script_end;

    uint8_t m_npc_flags[256]; // fake

    int m_npc_id;
    std::string m_npc_name;
    std::string m_npc_look;

};
