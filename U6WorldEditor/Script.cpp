#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <stack>
#include <algorithm>
#include <cctype>
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
        assert(context[0] == -1 && (uint8_t)context[1] == npc_index);
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

std::string Script::get_script(uint32_t npc_index)
{
    int index = npc_index;
    if (m_game_type == "se")
    {
        index -= 2;
    }

    return get_context(index);
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



#define U6OP_GT         0x81
#define U6OP_GE         0x82
#define U6OP_LT         0x83
#define U6OP_LE         0x84
#define U6OP_NE         0x85
#define U6OP_EQ         0x86

#define U6OP_ADD        0x90
#define U6OP_SUB        0x91
#define U6OP_MUL        0x92
#define U6OP_DIV        0x93
#define U6OP_LOR        0x94
#define U6OP_LAND       0x95

#define U6OP_IF         0xa1
#define U6OP_ENDIF      0xa2
#define U6OP_ELSE       0xa3
#define U6OP_SETF       0xa4
#define U6OP_CLEARF     0xa5
#define U6OP_DECL       0xa6
#define U6OP_EVAL       0xa7
#define U6OP_ASSIGN     0xa8

#define U6OP_FLAG       0xab

#define U6OP_JUMP       0xb0

#define U6OP_BYE        0xb6

#define U6OP_PORTRAIT   0xbf

#define U6OP_INPARTY    0xc6

#define U6OP_JOIN       0xca
#define U6OP_WAIT       0xcb
#define U6OP_LEAVE      0xcc
#define U6OP_WORKTYPE   0xcd

#define U6OP_NUM32      0xd2
#define U6OP_NUM8       0xd3
#define U6OP_NUM16      0xd4

#define U6OP_ENDANSWER  0xee
#define U6OP_KEYWORDS   0xef

#define U6OP_LOOK       0xf1
#define U6OP_CONVERSE   0xf2

#define U6OP_ANSWER     0xf6
#define U6OP_ASK        0xf7
#define U6OP_ASKC       0xf8

#define U6OP_INPUT      0xfb
#define U6OP_INPUTNUM   0xfc

#define U6OP_ID         0xff







ScriptInterpreter::ScriptInterpreter(const std::string& script) : m_script(script)
{
    m_current = m_script.data();
    m_script_end = m_current + m_script.length();

    // test settings
    m_npc_flags[0x01] = 0x01;        // bit 0: rescue aiela
    m_npc_flags[0x02] = (0x01 << 5); // Aiela, bit 5: mate of avatar
    m_npc_flags[0x33] = (0x00 << 5); // Tristia, bit 5: mate of avatar
}

inline std::string get_string(const char* &p, const char* end)
{
    auto start = p;
    while (p < end && (uint8_t)*p < 0x80)
    {
        p++;
    }
    return std::string(start, p - start);
}

inline void get_keywords(const char* &p, const char* end, std::vector<std::string>& out)
{
    out.clear();

    auto start = p;
    while (p < end && (uint8_t)*p < 0x80)
    {
        if (*p == ',')
        {
            out.emplace_back(start, p - start);
            TRACE("%s, ", out.back().c_str());
            start = p + 1;
        }
        p++;
    }
    out.emplace_back(start, p - start);
    TRACE("%s, \n", out.back().c_str());
}

inline bool match_keybword(const std::vector<std::string>& keywords, const std::string& input)
{
    for (auto i = keywords.begin(); i != keywords.end(); ++i)
    {
        if (*i == "*")
            return true;

        if (i->length() <= input.length())
        {
            struct iequal
            {
                bool operator()(int c1, int c2) const
                {
                    return std::toupper(c1) == std::toupper(c2);
                }
            };
            if (std::equal(i->begin(), i->end(), input.begin(), iequal()))
            {
                return true;
            }
        }
    }
    return false;
}

ScriptInterpreter::status ScriptInterpreter::run(const std::string& input, std::string& output)
{
    output.clear();

    if (m_current >= m_script_end)
    {
        return status::END;
    }

    std::vector<std::string> keywords;
    int32_t result;

    while (m_current < m_script_end)
    {
        auto script_offset = m_current - m_script.data();

        int code = (uint8_t)*m_current;
        if (code < 0x80)
        { // printable
            std::string text = get_string(m_current, m_script_end);
            output += text;
        }
        else
        { // control code
            ++m_current;

            switch (code) {
            case U6OP_ID: // npc id and name
                m_npc_id = (uint8_t)*m_current++;
                m_npc_name = get_string(m_current, m_script_end);
                break;
            case U6OP_LOOK:
                m_npc_look = get_string(m_current, m_script_end);
                break;
            case U6OP_CONVERSE:
                TRACE("START CONVERSION\n");
                break;
            case U6OP_ASK:
                return status::INPUT;
            case U6OP_ASKC:
                output += '(';
                while ((uint8_t)*m_current != U6OP_KEYWORDS)
                {
                    output += *m_current++;
                }
                output += ')';
                return status::INPUT;;
            case U6OP_KEYWORDS:
                get_keywords(m_current, m_script_end, keywords);
                break;
            case U6OP_ANSWER:
                if (!match_keybword(keywords, input))
                {
                    skip_answer_block(m_current, m_script_end);
                }
                break;
            case U6OP_IF:
                result = evaluate();
                if (!result)
                {
                    skip_if_block(m_current, m_script_end, true);
                }
                break;
            case U6OP_ELSE:
            case U6OP_ENDIF:
                break;
            case U6OP_SETF:
                // TODO
                {
                int npc = evaluate();
                npc = (npc != 0xeb ? npc : m_npc_id);
                int flagIndex = evaluate();
                // set npc.flag
                }
                break;
            case U6OP_CLEARF:
                {
                int npc = evaluate();
                npc = (npc != 0xeb ? npc : m_npc_id);
                int flagIndex = evaluate();
                // clear npc.flag
                }
                break;
            case U6OP_DECL:
                // TODO:
                {
                int index = *(uint8_t*)m_current++;
                int type = *(uint8_t*)m_current++;
                }
                break;
            case U6OP_ASSIGN: // assign
                // TODO:
                {
                result = evaluate();
                }
                break;
            case U6OP_JUMP: // jump to
                result = *(int32_t*)m_current;
                m_current = m_script.data() + result;
                break;
            case U6OP_BYE: // bye
                m_current = m_script.data();
                return status::END;
            case U6OP_PORTRAIT:
                // TODO
                {
                int portrait_num = evaluate();
                }
                break;
            case U6OP_WAIT: // wait
                output += "(cont.)";
                return status::WAIT;
            case U6OP_WORKTYPE:
                {
                int npc = evaluate();
                npc = (npc != 0xeb ? npc : m_npc_id);
                int worktype = evaluate();
                // set npc worktype
                }
                break;
            default:
                assert(false && "unknown op code!");
                break;
            }
        }
    }

    return status::END;
}

int32_t ScriptInterpreter::evaluate()
{
    std::stack<int32_t> rstk;
    int32_t arg1, arg2;

    while (m_current < m_script_end)
    {
        auto value = (uint8_t)*m_current++;
        switch (value) {
        case U6OP_NUM8: // 8 bit value
            rstk.push(*m_current);
            m_current++;
            break;
        case U6OP_NUM32: // 32 bit value
            rstk.push(*(int32_t*)m_current);
            m_current += 4;
            break;
        case U6OP_NUM16: // 16 bit
            rstk.push(*(int16_t*)m_current);
            m_current += 2;
            break;
        case 0xa7: // result
            return rstk.top();
        case 0x81: // >
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 > arg2 ? 1 : 0);
            break;
        case 0x82: // >=
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 >= arg2 ? 1 : 0);
            break;
        case 0x83: // <
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 < arg2 ? 1 : 0);
            break;
        case 0x84: // <=
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 <= arg2 ? 1 : 0);
            break;
        case 0x85: // !=
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 != arg2 ? 1 : 0);
            break;
        case 0x86: // ==
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 == arg2 ? 1 : 0);
            break;
        case 0x90: // +
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 + arg2);
            break;
        case 0x91: // -
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 - arg2);
            break;
        case 0x92: // *
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 * arg2);
            break;
        case 0x93: // /
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 / arg2);
            break;
        case 0x94: // ||
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 || arg2 ? 1 : 0);
            break;
        case 0x95: // &&
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 && arg2 ? 1 : 0);
            break;

        case 0xab: // flag
            arg2 = rstk.top(); rstk.pop(); // flag index
            arg1 = (rstk.top() & 0xff); rstk.pop(); // npc id
            arg1 = (arg1 != 0xeb ? arg1 : m_npc_id);
            rstk.push((m_npc_flags[arg1] & (1 << arg2)) ? 1 : 0);
            break;

        case U6OP_INPARTY:
            arg1 = rstk.top(); rstk.pop(); // npc id
            arg1 = (arg1 != 0xeb ? arg1 : m_npc_id);
            rstk.push(0);
            // fake: not in the party
            break;
        case U6OP_JOIN:
            arg1 = rstk.top(); rstk.pop(); // npc id
            arg1 = (arg1 != 0xeb ? arg1 : m_npc_id);
            rstk.push(0);
            // 3: ALREADY IN PARTY
            // 2: PARTY TOO LARGE
            // 1: NOT ON LAND (vehicle)
            // 0: SUCCESS
            break;
        case U6OP_LEAVE: // leave
            arg1 = rstk.top(); rstk.pop(); // npc id
            arg1 = (arg1 != 0xeb ? arg1 : m_npc_id);
            rstk.push(0);
            // 2: NOT IN PARTY
            // 1: NOT ON LAND
            // 0: SUCCESS
            break;

        default:
            // check if it is a declared variable
            // check 'value' first if assert
            value;
            switch ((uint8_t)*m_current) {
            case 0xb2:
            case 0xb3:
            case 0xb4:
                rstk.push(0); // TODO: fake
                m_current++;
                break;
            default:
                assert(false);
                break;
            }
            break;
        }
    }

    assert(false); // should be stopped on the code 0xa7
    return 0;
}

void ScriptInterpreter::skip_text(const char* &p, const char* end)
{
    auto start = p;
    while (p < end && ((uint8_t)*p < 0x80 || (uint8_t)*p == U6OP_WAIT))
    {
        p++;
    }
}

void ScriptInterpreter::skip_eval_block(const char* &p, const char* end)
{
    auto script_offset = p - m_script.data(); // for debug

    while (p < end)
    {
        auto value = (uint8_t)*p++;
        switch (value) {
        case U6OP_NUM8:         p++; break;
        case U6OP_NUM32:        p += 4; break;
        case U6OP_NUM16:        p += 2; break;
        case U6OP_EVAL:         return;
        case U6OP_GT:           break;
        case U6OP_GE:           break;
        case U6OP_LT:           break;
        case U6OP_LE:           break;
        case U6OP_NE:           break;
        case U6OP_EQ:           break;
        case U6OP_ADD:          break;
        case U6OP_SUB:          break;
        case U6OP_MUL:          break;
        case U6OP_DIV:          break;
        case U6OP_LOR:          break;
        case U6OP_LAND:         break;
        case U6OP_FLAG:         break;
        case U6OP_INPARTY:      break;
        case U6OP_JOIN:         break;
        case U6OP_LEAVE:        break;
        default:
            value; // for debug
            switch ((uint8_t)*p) {
            case 0xb2:
            case 0xb3:
            case 0xb4:
                p++;
                break;
            default:
                assert(false && "unknow op code!");
                break;
            }
            break;
        }
    }
    assert(false && "should not be here!");
}

void ScriptInterpreter::skip_if_block(const char* &p, const char* end, bool stop_on_else)
{
    auto script_offset = p - m_script.data(); // for debug

    // if stop_on_else == false, it means all if block should be skipped including the evaluation,
    // otherwise, only the content of the if statement should be skipped.
    if (!stop_on_else)
        skip_eval_block(p, end);

    while (p < end)
    {
        int code = (uint8_t)*p;
        if (code < 0x80)
        {
            skip_text(p, end);
        }
        else
        {
            p++;
            switch (code) {
            case U6OP_ELSE:     if (stop_on_else) return; else break; // exit the function if stop_on_else
            case U6OP_ENDIF:    return; // exit the function
            case U6OP_SETF:
            case U6OP_CLEARF:
            case U6OP_WORKTYPE:
                skip_eval_block(p, end);
                skip_eval_block(p, end);
                break;
            case U6OP_DECL:     p += 2; break;
            case U6OP_ASSIGN:
                skip_eval_block(p, end);
                break;
            case U6OP_JUMP:     p += 4; break;
            case U6OP_BYE:      break;
            case U6OP_PORTRAIT:
                skip_eval_block(p, end);
                break;
            case U6OP_WAIT:     break;
            default:
                assert(false);
                break;
            }
        }
    }
}

void ScriptInterpreter::skip_answer_block(const char* &p, const char* end)
{
    auto script_offset = p - m_script.data(); // for debug

    while (p < end)
    {
        int code = (uint8_t)*p;
        if (code < 0x80)
        {
            skip_text(p, end);
        }
        else if (code == U6OP_KEYWORDS)
        {
            return;
        }
        else
        {
            p++;
            switch (code) {
            case U6OP_IF:
                skip_if_block(p, end);
                break;
            case U6OP_SETF:
            case U6OP_CLEARF:
            case U6OP_WORKTYPE:
                skip_eval_block(p, end);
                skip_eval_block(p, end);
                break;
            case U6OP_DECL:     p += 2; break;
            case U6OP_ASSIGN:
                skip_eval_block(p, end);
                break;
            case U6OP_JUMP:     p += 4; break; // exit the function
            case U6OP_BYE:      break;
            case U6OP_PORTRAIT:
                skip_eval_block(p, end);
                break;
            case U6OP_WAIT:     break;
            default:
                assert(false);
                break;
            }
        }
    }
}


