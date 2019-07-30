#pragma once

//#pragma warning( disable : 4251 )

#include <iostream>
#include <vector>

class LZW
{
public:
    LZW(void);
    ~LZW(void);

    enum class RESULT: int {
        END_OF_CODE = 0,
        END_OF_STREAM = 1,
        UNKNOWN_ERROR = -1,
        FIRST_IS_NOT_CLEAR_CODE = -2,
        CLEAR_CODE_BUT_NOT_12BITS = -3,
        OUTPUT_ERROR = -4
    };

    bool    encode(std::istream& is, std::ostream& os, int bitcount);
    RESULT  decode(std::istream& is, std::ostream& os, int bitcount);


private:
    class Dictionary
    {
    public:
        int     CLEAR_CODE;
        int     END_CODE;
        int     BITCOUNT;

        void    init(int bitcount);
        int     get_position(int RM, int NC);
        bool    is_full();
        void    append(int RM, int NC);

        void    insert(int RM, int NC);
        int*    get_string(int code);

        void    dump();

    public:
        enum
        {
            MAX_DIC_SIZE    = 4096,
            EMPTY           = -1
        };

        struct NEW_STRING
        {
            int              prefix;
            unsigned char    character;
        };

    private:
        NEW_STRING          m_dic[MAX_DIC_SIZE];
        int                 m_dic_size;
        int                 m_dic_last_position;
        std::vector<int>    m_output_string;
    };

    Dictionary    m_dic;

    int            m_in_buffered_bits;
    int            m_in_buffer;

    int            m_out_buffered_bits;
    int            m_out_buffer;

    // Encode
    void    put_code(std::ostream& os, int code, int codesize);
    int     get_char(std::istream& is, int bitcount);

    // Decode
    // get a code from vis, error if return code is less than 0.
    int     get_code(std::istream& is, int codesize);
    // put a char to vos, error when false is returned.
    bool    put_char(std::ostream& os, int code, int bitcount);

};

void lzw_unit_tests();
