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

std::vector<uint8_t> Script::get_context(uint32_t index)
{
    if (index < m_scripts.size())
    {
        return m_scripts[index];
    }
    return std::vector<uint8_t>();
}

std::vector<uint8_t> Script::get_script(uint32_t npc_index)
{
    int index = npc_index;
    if (m_game_type == "se")
    {
        index -= 2;
    }

    return get_context(index);
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
        assert(context[0] == 0xff && (uint8_t)context[1] == npc_index);
        auto p = &context[2];
        while (1)
        {
            if (*p == 0xf1)
                break;
            p++;
        }
        return std::string((char*)&context[2], (size_t)(p - &context[2]));
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
                m_scripts.push_back(std::vector<uint8_t>());
                continue;
            }

            uint32_t size;
            is->read((char*)&size, sizeof(size));

            if (size == 0)
            {
                std::vector<uint8_t> tmp;
                tmp.resize(item_size);
                is->read((char*)tmp.data(), item_size);
                m_scripts.push_back(tmp);
            }
            else
            {
                // decode data
                std::stringstream decoded_stream;
                LZW().decode(*is, decoded_stream, 8);
                auto script_size = decoded_stream.tellp();
                m_scripts.emplace_back(script_size);
                decoded_stream.read((char*)m_scripts.back().data(), script_size);
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
            m_scripts.push_back(std::vector<uint8_t>());
            continue;
        }

        uint32_t size;
        is->read((char*)&size, sizeof(size));

        assert(size != 0);

        // decode data
        std::stringstream decoded_stream;
        LZW().decode(*is, decoded_stream, 8);
        auto script_size = decoded_stream.tellp();
        m_scripts.emplace_back(script_size);
        decoded_stream.read((char*)m_scripts.back().data(), script_size);
    }

    lib.close();
    return true;
}


#include "u6_opcode.h"












ScriptInterpreter::ScriptInterpreter()
{
    m_current = m_script_end = nullptr;

    // test settings
    m_npc_flags[0x01] = 0x01;        // bit 0: rescue aiela
    m_npc_flags[0x02] = (0x01 << 5); // Aiela, bit 5: mate of avatar
    m_npc_flags[0x33] = (0x00 << 5); // Tristia, bit 5: mate of avatar
    m_npc_flags[0x04] = (0x01 << 4); // Jimmy, bit 4: rescued
}

void ScriptInterpreter::load(const std::vector<uint8_t>& script)
{
    m_script = script;
    m_current = m_script.data();
    m_script_end = m_current + m_script.size();
}

std::string ScriptInterpreter::format_script()
{
    std::string result;
    const uint8_t* p = m_script.data();
    auto end = p + m_script.size();
    collect_format(result, p, p, end, 0);
    return result;
}

inline std::string get_string(const uint8_t* &p, const uint8_t* end)
{
    auto start = p;
    while (p < end && *p < 0x80)
    {
        p++;
    }
    return std::string((char*)start, p - start);
}

inline void get_keywords(const uint8_t* &p, const uint8_t* end, std::vector<std::string>& out)
{
    out.clear();

    auto start = p;
    while (p < end && *p < 0x80)
    {
        if (*p == ',')
        {
            out.emplace_back((char*)start, p - start);
            TRACE("%s, ", out.back().c_str());
            start = p + 1;
        }
        p++;
    }
    out.emplace_back((char*)start, p - start);
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
                assert((uint8_t)*m_current == U6OP_ANSWER);
                m_current++;
                if (!match_keybword(keywords, input))
                {
                    skip_code_block(m_current, m_script_end, U6OP_ANSWER);
                    assert(*m_current == U6OP_KEYWORDS);
                }
                break;
            case U6OP_IF:
                result = evaluate();
                if (!result)
                {
                    skip_code_block(m_current, m_script_end, U6OP_IF);
                    assert(*(m_current - 1) == U6OP_ELSE || *(m_current - 1) == U6OP_ENDIF);
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
            case U6OP_NEW: // 0xb9
                // TODO
                {
                int npc = evaluate();
                int obj = evaluate();
                int qual = evaluate();
                int quant = evaluate();
                npc = (npc != 0xeb ? npc : m_npc_id);
                // create item for npc
                }
                break;
            case U6OP_DELETE: // 0xba
                // TODO
                {
                int npc = evaluate();
                int obj = evaluate();
                int qual = evaluate();
                int quant = evaluate();
                npc = (npc != 0xeb ? npc : m_npc_id);
                // remove item from npc
                }
                break;
            case U6OP_PORTRAIT:
                // TODO
                {
                int portrait_num = evaluate();
                }
                break;
            case U6OP_PAUSE: // wait
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
        auto script_offset = m_current - m_script.data();

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

        case U6OP_VAR:
            arg1 = rstk.top(); rstk.pop();
            rstk.push(0); // TODO
            break;
        case U6OP_SVAR:
            arg1 = rstk.top(); rstk.pop();
            rstk.push(0); // TODO
            break;
        case U6OP_DATA:
            arg1 = rstk.top(); rstk.pop();
            rstk.push(0); // TODO
            break;

        case 0xa7: // result
            assert(rstk.size() == 1);
            return rstk.top();

        case U6OP_GT: // 0x81
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 > arg2 ? 1 : 0);
            break;
        case U6OP_GE: // 0x82
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 >= arg2 ? 1 : 0);
            break;
        case U6OP_LT: // 0x83
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 < arg2 ? 1 : 0);
            break;
        case U6OP_LE: // 0x84
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 <= arg2 ? 1 : 0);
            break;
        case U6OP_NE: // 0x85
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 != arg2 ? 1 : 0);
            break;
        case U6OP_EQ: // 0x86
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 == arg2 ? 1 : 0);
            break;
        case U6OP_ADD: // 0x90
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 + arg2);
            break;
        case U6OP_SUB: // 0x91
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 - arg2);
            break;
        case U6OP_MUL: // 0x92
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 * arg2);
            break;
        case U6OP_DIV: // 0x93
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 / arg2);
            break;
        case U6OP_LOR: // 0x94
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 | arg2 ? 1 : 0);
            break;
        case U6OP_LAND: // 0x95
            arg2 = rstk.top(); rstk.pop();
            arg1 = rstk.top(); rstk.pop();
            rstk.push(arg1 & arg2 ? 1 : 0);
            break;
        case U6OP_FLAG: // 0xab
            arg2 = rstk.top(); rstk.pop(); // flag index
            arg1 = (rstk.top() & 0xff); rstk.pop(); // npc id
            arg1 = (arg1 != 0xeb ? arg1 : m_npc_id);
            rstk.push((m_npc_flags[arg1] & (1 << arg2)) ? 1 : 0);
            break;

        case U6OP_INPARTY: // 0xc6
            arg1 = rstk.top(); rstk.pop(); // npc id
            arg1 = (arg1 != 0xeb ? arg1 : m_npc_id);
            rstk.push(0);
            // fake: not in the party
            break;

        case U6OP_OBJINPARTY: // 0xc7
            arg2 = rstk.top(); rstk.pop(); // qual
            arg1 = rstk.top(); rstk.pop(); // obj
            rstk.push(0); // should push npc id who has this specific object.
            // fake: not in the party
            break;

        case U6OP_JOIN: // 0xca
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
            assert(value < 0x80);
            rstk.push(value);
            break;
        }
    }

    assert(false); // should be stopped on the code 0xa7
    return 0;
}

void ScriptInterpreter::skip_text(const uint8_t* &p, const uint8_t* end)
{
    auto start = p;
    while (p < end && *p < 0x80)
    {
        p++;
    }
}

void ScriptInterpreter::skip_eval_block(const uint8_t* &p, const uint8_t* end)
{
    while (p < end)
    {
        auto script_offset = p - m_script.data(); // for debug

        auto value = *p++;
        switch (value) {
        case U6OP_NUM8:         p++; break;
        case U6OP_NUM32:        p += 4; break;
        case U6OP_NUM16:        p += 2; break;
        case U6OP_VAR:          break;
        case U6OP_SVAR:         break;
        case U6OP_DATA:         break;
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
        case U6OP_OBJINPARTY:   break;
        case U6OP_JOIN:         break;
        case U6OP_LEAVE:        break;
        default:
            assert(value < 0x80);
            break;
        }
    }
    assert(false && "should not be here!");
}

void ScriptInterpreter::skip_code_block(const uint8_t* &p, const uint8_t* end, int block_type)
{
    while (p < end)
    {
        auto script_offset = p - m_script.data(); // for debug
        const uint8_t* addr1;
        const uint8_t* addr2;

        int code = *p;
        if (code < 0x80)
        {
            skip_text(p, end);
        }
        else
        {
            p++;
            switch (code) {
            case U6OP_ASK:      break; // does it need to do as U6OP_ASKC?
            case U6OP_ASKC:
                skip_text(p, end); // skip possible answers
                while (p < end)
                {
                    assert(*p == U6OP_KEYWORDS);
                    p++;
                    skip_text(p, end); // skip keywords
                    assert(*p == U6OP_ANSWER);
                    p++;
                    skip_code_block(p, end, U6OP_ANSWER);
                    if (*p == U6OP_ENDANSWER)
                        break;
                }
                assert(*p == U6OP_ENDANSWER);
                p++;
                break;
            case U6OP_KEYWORDS:
            case U6OP_ENDANSWER:
                p--;
                assert(block_type == U6OP_ANSWER);
                return;
            case U6OP_IF:
                addr1 = addr2 = nullptr;
                skip_eval_block(p, end);                // skip the conditional expression
                skip_code_block(p, end, U6OP_IF);       // skip the if-true block
                if (*(p - 6) == U6OP_JUMP)
                {
                    addr1 = p - 5;
                }
                if (*(p - 1) == U6OP_ELSE)
                {
                    skip_code_block(p, end, U6OP_ELSE); // skip the if-false block
                    if (*(p - 6) == U6OP_JUMP)
                    {
                        addr2 = p - 5;
                    }
                }
                assert(*(p - 1) == U6OP_ENDIF);         // should be end with U6OP_ENDIF
                if (addr1 && addr2 && *(uint32_t*)addr1 == *(uint32_t*)addr2 && block_type == U6OP_ANSWER)
                {
                    // this is the case that there is no U6OP_ENDANSWER after the last answer '*' in aiela's script
                    // if the handling of if-true and if-false jumps to the same address, treat it as the end of the answer
                    return;
                }
                break;
            case U6OP_ELSE:
                assert(block_type == U6OP_IF);          // should be from U6OP_IF block, then hit U6OP_ELSE
                return;
            case U6OP_ENDIF:                            // should be from U6OP_IF or U6OP_ELSE, then hit U6OP_ENDIF
                assert(block_type == U6OP_IF || block_type == U6OP_ELSE);
                return;
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
            case U6OP_NEW:
            case U6OP_DELETE:
                skip_eval_block(p, end); // npc number
                skip_eval_block(p, end); // obj
                skip_eval_block(p, end); // quality
                skip_eval_block(p, end); // quantity
                break;
            case U6OP_PORTRAIT:
                skip_eval_block(p, end);
                break;
            case U6OP_PAUSE:     break;
            default:
                assert(false);
                break;
            }
        }
    }

}


void ScriptInterpreter::collect_text(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end)
{
    auto script_offset = p - script_start; // for debug
    auto start = p;
    while (p < script_end && *p < 0x80)
    {
        p++;
    }
    result += std::string((const char*)start, p - start);
}

void ScriptInterpreter::collect_eval(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end)
{
    result += "[";
    while (p < script_end)
    {
        auto script_offset = p - script_start; // for debug

        auto value = *p++;
        switch (value) {
        case U6OP_NUM8:         result += format_string("N8(0x%02x) ", *p); p++; break;
        case U6OP_NUM32:        result += format_string("N32(0x%08x) ", *(uint32_t*)p); p += 4; break;
        case U6OP_NUM16:        result += format_string("N16(0x%04x) ", *(uint16_t*)p); p += 2; break;
        case U6OP_VAR:          result += "B2 "; break;
        case U6OP_SVAR:         result += "B3 "; break;
        case U6OP_DATA:         result += "B4 "; break;
        case U6OP_EVAL:         result += "]"; return;
        case U6OP_GT:           result += "> "; break;
        case U6OP_GE:           result += ">= "; break;
        case U6OP_LT:           result += "< "; break;
        case U6OP_LE:           result += "<= "; break;
        case U6OP_NE:           result += "!= "; break;
        case U6OP_EQ:           result += "== "; break;
        case U6OP_ADD:          result += "+ "; break;
        case U6OP_SUB:          result += "- "; break;
        case U6OP_MUL:          result += "* "; break;
        case U6OP_DIV:          result += "/ "; break;
        case U6OP_LOR:          result += "| "; break;
        case U6OP_LAND:         result += "& "; break;
        case U6OP_CANCARRY:     result += "CANCARRY "; break;
        case U6OP_WEIGHT:       result += "WEIGHT "; break;
        case U6OP_RAND:         result += "RAND "; break;
        case U6OP_FLAG:         result += "FLAG "; break;
        case U6OP_OBJCOUNT:     result += "OBJCOUNT "; break;
        case U6OP_INPARTY:      result += "INPARTY "; break;
        case U6OP_OBJINPARTY:   result += "OBJINPARTY "; break; // arg1: obj, arg2: qual, return: should push npc id who has this specific object.
        case U6OP_JOIN:         result += "JOIN "; break;
        case U6OP_LEAVE:        result += "LEAVE "; break;
        case U6OP_WOUNDED:      result += "WOUNDED "; break;
        case U6OP_POISONED:     result += "POISONED "; break;
        case U6OP_NPC:          result += "NPC "; break;
        case U6OP_EXP:          result += "EXP. "; break;
        case U6OP_LVL:          result += "LVL. "; break;
        case U6OP_STR:          result += "STR. "; break;
        case U6OP_INT:          result += "INT. "; break;
        case U6OP_DEX:          result += "DEX. "; break;
        default:
            assert(value < 0x80);
            result += format_string("0x%02x ", value);
            break;
        }
    }
    assert(false && "should not be here!");
}

void ScriptInterpreter::collect_format(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end, int block_type)
{
    while (p < script_end)
    {
        auto script_offset = p - script_start;
        result += format_string(":%04X\r\n", script_offset);

        collect_strange(result, p, script_start, script_end);

        int code = (uint8_t)*p;
        if (code < 0x80)
        { // printable
            result += "    {";
            collect_text(result, p, script_start, script_end);
            result += "}\r\n";
        }
        else
        { // control code
            ++p;

            switch (code) {
            case U6OP_ID: // npc id and name
            {
                m_npc_id = *p++;
                m_npc_name.clear();
                collect_text(m_npc_name, p, script_start, script_end);
                result += format_string("    NPC_ID: npc id: %d, npc name: %s\r\n",
                    m_npc_id, m_npc_name.c_str());
                break;
            }
            case U6OP_LOOK:
            {
                result += "    NPC_LOOK: ";
                collect_text(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_CONVERSE:
            {
                result += "    START CONVERSION\r\n";
                break;
            }
            case U6OP_ASK:
            {
                result += "    ASK\r\n";
                bool force_to_end_the_answer = false;
                while (p < script_end)
                {
                    script_offset = p - script_start;
                    if (*p != U6OP_KEYWORDS)
                    {
                        // it is possible that there is something before KEYWORDS
                        collect_format(result, p, script_start, script_end, U6OP_ASK);
                    }

                    script_offset = p - script_start;
                    assert(*p == U6OP_KEYWORDS);
                    p++;

                    // force_to_end_the_answer is used to solve the last answer has no the code U6OP_ENDANSWER
                    // but it would be not perfect because it is a guess where is the end of the answer.
                    // check how U6OP_IF and U6OP_JUMP handle this situation.

                    if (*p == '*')
                        force_to_end_the_answer = true;

                    result += "    KEYWORDS ";
                    collect_text(result, p, script_start, script_end);
                    result += "\r\n";

                    script_offset = p - script_start;
                    assert(*p == U6OP_ANSWER);
                    p++;
                    result += "    ANSWER\r\n";
                    collect_format(result, p, script_start, script_end, U6OP_ANSWER);
                    if (*p == U6OP_ENDANSWER || force_to_end_the_answer)
                        break;
                }
                assert(*p == U6OP_ENDANSWER || force_to_end_the_answer);
                if (*p == U6OP_ENDANSWER)
                    p++;
                result += "    END_ANSWER\r\n";
                break;
            }
            case U6OP_KEYWORDS:
                p--;
                assert(block_type == U6OP_ANSWER || block_type == U6OP_ASK);
                return;
            case U6OP_ENDANSWER:
                p--;
                assert(block_type == U6OP_ANSWER);
                return;
            case U6OP_ASKC:
            {
                auto anchor = p;
                skip_text(p, script_end);
                auto possible_answer_count = p - anchor;
                result += format_string("    ASKC [%s]\r\n", std::string((char*)anchor, p - anchor).c_str());
                while (p < script_end)
                {
                    script_offset = p - script_start;
                    assert(*p == U6OP_KEYWORDS);
                    p++;
                    auto anchor = p;
                    skip_text(p, script_end);
                    result += format_string("    KEYWORDS %s\r\n", std::string((char*)anchor, p - anchor).c_str());

                    script_offset = p - script_start;
                    assert(*p == U6OP_ANSWER);
                    possible_answer_count--;
                    if (*anchor == '*') // wildcard, 056_Kunawo
                    {
                        possible_answer_count = 0;
                    }

                    p++;
                    result += "    ANSWER\r\n";
                    collect_format(result, p, script_start, script_end, U6OP_ANSWER);
                    if (*p == U6OP_ENDANSWER)
                    {
                        p++;
                        break;
                    }

                    // Atlipacta (SE) script offset 0x0aa8 does not have U6OP_ENDANSWER but U6OP_BYE
                    if (*p == U6OP_BYE)
                    {
                        p++;
                        break;
                    }

                    // 035 Katalkotl (SE) script offset 0x10d7 does not have U6OP_ENDANSWER
                    if (*p != U6OP_KEYWORDS && possible_answer_count == 0)
                        break;
                }
                assert(possible_answer_count == 0);
                result += "    END_ANSWER\r\n";
                break;
            }
            case U6OP_IF:
            {
                const uint8_t* addr1 = nullptr;
                const uint8_t* addr2 = nullptr;
                auto anchor = p;
                result += "    IF ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                collect_format(result, p, script_start, script_end, U6OP_IF);
                if (*(p - 6) == U6OP_JUMP)
                {
                    addr1 = p - 5;
                }
                if (*(p - 1) == U6OP_ELSE)
                {
                    auto anchor_else = p;
                    result += "    ELSE\r\n";
                    collect_format(result, p, script_start, script_end, U6OP_ELSE);
                    if (*(p - 6) == U6OP_JUMP)
                    {
                        addr2 = p - 5;
                    }
                }
                script_offset = p - script_start;
                assert(*(p - 1) == U6OP_ENDIF);         // should be end with U6OP_ENDIF
                result += "    ENDIF\r\n";
                if (addr1 && addr2 &&
                    *(uint32_t*)addr1 == *(uint32_t*)addr2 &&
                    *(uint32_t*)addr1 < (anchor - script_start) && // it should be the jump to the address close to the beginning
                    block_type == U6OP_ANSWER)
                {
                    // this is the case that there is no U6OP_ENDANSWER after the last answer '*' in aiela's script
                    // if the handling of if-true and if-false jumps to the same address, treat it as the end of the answer
                    return;
                }
                break;
            }
            case U6OP_ELSE:
                script_offset = p - script_start;
                assert(block_type == U6OP_IF);          // should be from U6OP_IF block, then hit U6OP_ELSE
                return;
            case U6OP_ENDIF:                            // should be from U6OP_IF or U6OP_ELSE, then hit U6OP_ENDIF
                script_offset = p - script_start;
                assert(block_type == U6OP_IF || block_type == U6OP_ELSE);
                return;
            case U6OP_INPUT:
            {
                auto var_index = *p++;
                auto var_type = *p++;
                result += format_string("    INPUT (vi=%02x, vt=%02x) ", var_index, var_type);
                collect_text(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_INPUTNUM:
            {
                auto var_index = *p++;
                auto var_type = *p++;
                result += format_string("    INPUTNUM (vi=%02x, vt=%02x) ", var_index, var_type);
                collect_text(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_SETF:
            {
                result += "    SET_FLAG ";
                collect_eval(result, p, script_start, script_end);
                result += ", ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_CLEARF:
            {
                result += "    CLEAR_FLAG ";
                collect_eval(result, p, script_start, script_end);
                result += ", ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            break;
            case U6OP_DECL:
            {
                auto index = *p++;
                auto type = *p++;
                result += format_string("    DECLARE [0x%02x, 0x%02x] ", index, type);
                assert(*p == U6OP_ASSIGN);
                p++;
                result += "= ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
            }
            break;
            case U6OP_JUMP: // jump to
            {
                auto address = *(int32_t*)p;
                p += 4;
                result += format_string("    JUMP 0x%04x\r\n", address);

                if (block_type == U6OP_ANSWER)
                {
                    // this is the case that there is no U6OP_ENDANSWER after the last answer '*' in jimmy's and rafkin's script
                    // if the handling of the jump to an address, treat it as the end of the answer
                    return;
                }

                break;
            }
            case U6OP_BYE: // bye
            {
                result += "    BYE\r\n";

                if (block_type == U6OP_ANSWER && *(p-2) == U6OP_ENDIF)
                {
                    // this is the case that there is no U6OP_ENDANSWER after the last answert '*' in Denys' script 0x094f
                    // if it is BYE after U6OP_ENDIF, treat is as the end of the answer
                    return;
                }
                break;
            }
            case U6OP_GIVE:
            {
                result += "    GIVE obj ";
                collect_eval(result, p, script_start, script_end);
                result += ", qual ";
                collect_eval(result, p, script_start, script_end);
                result += ", from ";
                collect_eval(result, p, script_start, script_end);
                result += ", to ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_NEW: // 0xb9
            {
                result += "    NEW npc ";
                collect_eval(result, p, script_start, script_end);
                result += ", obj ";
                collect_eval(result, p, script_start, script_end);
                result += ", quality ";
                collect_eval(result, p, script_start, script_end);
                result += ", quantity ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_DELETE: // 0xba
            {
                result += "    DELETE npc ";
                collect_eval(result, p, script_start, script_end);
                result += ", obj ";
                collect_eval(result, p, script_start, script_end);
                result += ", quality ";
                collect_eval(result, p, script_start, script_end);
                result += ", quantity ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            break;
            case U6OP_PORTRAIT:
            {
                result += "    PORTRAIT ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_PAUSE: // wait
            {
                result += "    PAUSE\r\n";
                break;
            }
            case U6OP_WORKTYPE:
            {
                result += "    WORKTYPE ";
                collect_eval(result, p, script_start, script_end);
                result += ", ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_SET_Y:
            {
                result += "    SET_$Y ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_HEAL:
            {
                result += "    HEAL ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            case U6OP_CURE:
            {
                result += "    CURE ";
                collect_eval(result, p, script_start, script_end);
                result += "\r\n";
                break;
            }
            default:
            {
                if (!collect_unknown(result, p, script_start, script_end, code))
                {
                    code;
                    script_offset = p - script_start;
                    assert(false && "unknown op code!");
                }
                break;
            }
            }
        }
    }
}

bool ScriptInterpreter::collect_unknown(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end, int unknown_code)
{
    auto script_offset = p - script_start;

    switch (unknown_code) {
    case U6OP_FUNC:
    {
        // unknown op code for Yunapotli (SE)
        // should be used for opeing the door of the city
        if (m_npc_name == "Yunapotli" && *(uint32_t*)p == 0xa700ded4)
        {
            // func 222:
            result += "    U6OP_FUNC ";
            collect_eval(result, p, script_start, script_end);
            result += "\r\n";
            p += 4;
            return true;
        }
        else if (m_npc_name == "Fabozz" && *(uint32_t*)p == 0xa700afd4)
        {
            // func 175:
            result += "    U6OP_FUNC ";
            collect_eval(result, p, script_start, script_end);
            result += "\r\n";
            return true;
        }
        else if (m_npc_name == "Intanya" && *(uint32_t*)p == 0xb6a70ad3)
        {
            // func 010: kick out to DOS
            result += "    U6OP_FUNC ";
            collect_eval(result, p, script_start, script_end);
            result += " kick out to DOS.\r\n";
            return true;
        }
        else
        {
            // func 001: ask guards to kill the player (055_Zipactriotl)(031_Huitlapacti)
            // func 002: ask guards to kill the player (021_Chizzztl)
            // func 003: ask guards to kill the player (036_Kipotli)
            // func 004: ask guards to kill the player (049_Tlapatla)
            // func 005: ask guards to kill the player (054_Xyxxxtl)
            // func 150: 052_Tuomaxx made a biggest drum on the Hill of Drum
            result += "    U6OP_FUNC ";
            collect_eval(result, p, script_start, script_end);
            result += "\r\n";
            return true;
        }
    }

    case U6OP_REVIVE:
    {
        // something related to revive people
        // op code for Balakai (SE), Intanya (SE)
        // it needs to remove the dead body from the evaluation, npc_id who has the dead body
        // the npc id of the dead body will be assigned to [0x1b B2],
        // also set $Y as the npc name who is revived.
        assert(m_npc_name == "Balakai" ||
               m_npc_name == "Intanya" ||
               m_npc_name == "Kunawo");
        result += "    REVIVE ";
        collect_eval(result, p, script_start, script_end);
        result += "\r\n";
        return true;
    }

    }

    return false;
}

void ScriptInterpreter::collect_strange(std::string& result, const uint8_t* &p, const uint8_t* script_start, const uint8_t* script_end)
{
    auto offset = p - script_start;
    switch (offset) {
        // maybe a bug on Yunapotli (SE) script offset 0x12d7
        // also on Aloron (SE) script offset 0x12ec
        // after BYE, the code A2 is following which is meaningless
    case 0x12d7:
        if (*p == 0xa2 && m_npc_name == "Yunapotli")
        {
            p++;
            result += "    // NOTE: a strange A2 is here\r\n";
        }
        break;

    case 0x12ec:
        if (*p == 0xa2 && m_npc_name == "Aloron")
        {
            p++;
            result += "    // NOTE: a strange A2 is here\r\n";
        }
        break;

    case 0x171e:
        if (*p == 0xee && m_npc_name == "Chafblum")
        {
            p++;
            result += "    // NOTE: a strange EE is here\r\n";
        }
        break;

    case 0x00bd:
        if (*p == 0xa2 && m_npc_name == "Kunawo")
        {
            p++;
            result += "    // NOTE: a strange A2 is here\r\n";
        }
        break;
    }

}
