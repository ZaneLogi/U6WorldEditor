#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include "LZW.h"

using namespace std;

LZW::LZW(void)
{
}

LZW::~LZW(void)
{
}

// LZW encoding
// for example:
// input:
//        a b c d ab bc
// output:
//        [256] 97(a) 98(b) 99(c) 100(d) 258(ab) 259(bc) [257]
// dictionary looks like:
//        258:ab
//        259:bc
//        260:cd
//        261:da
//        262:abb
// steps:
//        a
//        b => out a   => dic258 ab
//        c => out b   => dic259 bc
//        d => out c   => dic260 cd
//        a => out d   => dic261 da
//        b
//        b => out 258 => dic262 abb
//        c => out 259

bool LZW::encode( istream& is, ostream& os, int bitcount )
{
    int NC; // current code word
    int RM; // keep the previous code word

    // clear buffer data
    m_in_buffered_bits = 0;
    m_in_buffer = 0;

    m_out_buffered_bits = 0;
    m_out_buffer = 0;

    // initialize dictionary
    m_dic.init(bitcount);

    // put CLEAR_CODE
    put_code( os, m_dic.CLEAR_CODE, m_dic.BITCOUNT);

    RM = Dictionary::EMPTY;
    NC = get_char(is,bitcount);

    while ( NC >= 0 ) {
        int pos = m_dic.get_position( RM, NC );
        if ( pos < 0 )
        { // not hit
            if ( m_dic.is_full() )
            {
                put_code( os, RM, m_dic.BITCOUNT);
                put_code( os, m_dic.CLEAR_CODE, m_dic.BITCOUNT);
                m_dic.init(bitcount);
            }
            else
            {
                put_code( os, RM, m_dic.BITCOUNT);
                m_dic.append( RM, NC );
            }
            RM = NC;
        }
        else
        {
            RM = pos;
        }
        NC = get_char(is,bitcount);
    }
    put_code( os, RM, m_dic.BITCOUNT);
    put_code( os, m_dic.END_CODE, m_dic.BITCOUNT);

    if ( m_out_buffered_bits > 0 )
        os.put( (char)m_out_buffer ); // flush buffer

    return true;
}

void LZW::put_code( ostream& os, int code, int codesize )
{
    m_out_buffer |= ( code << m_out_buffered_bits ); // add code to high bits
    m_out_buffered_bits += codesize; // buffered bits increases m_bitcount

    // we need to write to output stream?
    while ( m_out_buffered_bits >= 8 )
    {
        os.put( m_out_buffer & 0xff );
        m_out_buffer >>= 8;
        m_out_buffered_bits -= 8;
    }
}

int LZW::get_char( istream& is, int bitcount )
{
    int ch = 0;
    int unfilled_bits = bitcount;

    while ( unfilled_bits > 0 )
    {
        if ( unfilled_bits >= m_in_buffered_bits )
        {
            ch = (ch << m_in_buffered_bits) | m_in_buffer;
            unfilled_bits -= m_in_buffered_bits;

            m_in_buffer = is.get();
            if ( m_in_buffer == -1 ) // EOF
            {
                if ( unfilled_bits == 0 )
                    m_in_buffered_bits = 0;
                else
                    return -1;
            }
            m_in_buffered_bits = 8;
        }
        else
        {
            m_in_buffered_bits -= unfilled_bits;
            ch = (ch << unfilled_bits) | ( m_in_buffer >> m_in_buffered_bits );
            m_in_buffer &= ((1 << m_in_buffered_bits ) - 1);
            unfilled_bits = 0;
        }
    }
    return ch;
}

// LZW decoding
// for example:
// input:
//        [256] 97(a) 98(b) 99(c) 100(d) 258(ab) 259(bc) [257]
// output:
//        a b c d ab bc
// dictionary looks like:
//        258:ab
//        259:bc
//        260:cd
//        261:da
//        262:abb
// steps:
//        97  => out a
//        98  => RM = 97(a),  NC = 98(b)  => dic258 ab => out b
//        99  => RM = 98(b),  NC = 99(c)  => dic259 bc => out c
//        100 => RM = 99(c),  NC =100(d)  => dic260 cd => out d
//        258 => RM =100(d),  NC =258(ab) => dic261 da => out ab
//        259 => RM =258(ab), NC =259(bc) => dic262 abb=> out bc

LZW::RESULT LZW::decode( istream& is, ostream& os, const int bitcount )
{
    int NC; // current code word
    int RM; // keep the previous code word

    // clear buffer data
    m_in_buffered_bits = 0;
    m_in_buffer = 0;

    m_out_buffered_bits = 0;
    m_out_buffer = 0;

    // initialize dictionary
    m_dic.init(bitcount);

    // get CLEAR_CODE
    NC = get_code( is, m_dic.BITCOUNT);
    if ( NC != m_dic.CLEAR_CODE )
        return RESULT::FIRST_IS_NOT_CLEAR_CODE;

    // get the first real data
    NC = get_code( is, m_dic.BITCOUNT);
    if ( NC == EOF )
        return RESULT::END_OF_STREAM;

    put_char( os, NC, bitcount );

    RM = NC;

    NC = get_code( is, m_dic.BITCOUNT);
    if ( NC == EOF )
        return RESULT::END_OF_STREAM;

    RESULT ret = RESULT::END_OF_CODE;

    while ( NC != m_dic.END_CODE )
    {
        if ( NC == m_dic.CLEAR_CODE )
        {
            if (m_dic.BITCOUNT != 12 )
            {
                ret = RESULT::CLEAR_CODE_BUT_NOT_12BITS;
                break;
            }

            m_dic.init(bitcount);

            // get the first useful data
            NC = get_code( is, m_dic.BITCOUNT);
            if ( NC == EOF )
            {
                ret = RESULT::END_OF_STREAM;
                break;
            }

            put_char( os, NC, bitcount );
        }
        else
        {
            m_dic.insert(RM,NC);
            if (!put_char( os, NC, bitcount ) )
                return RESULT::OUTPUT_ERROR;
        }

        // shift roles - the current NC becomes the RM
        RM = NC;

        NC = get_code( is, m_dic.BITCOUNT);
        if ( NC == EOF )
        {
            ret = RESULT::END_OF_STREAM;
            break;
        }
    }

    if ( m_out_buffered_bits > 0 ) // flush buffer
    {
        os.put( m_out_buffer );
    }

    return ret;
}

int LZW::get_code( istream& is, int codesize )
{
    int code = 0;

    int requested_bits = codesize;

    while ( requested_bits > 0 )
    {
        // read more 1 byte = 8 bits
        if ( requested_bits > m_in_buffered_bits )
        {
            int data = is.get();
            if ( data < 0 ) // error
                return EOF;
            m_in_buffer |= ( data << m_in_buffered_bits );
            m_in_buffered_bits += 8;
        }
        // retrieve code from bufferedData
        else
        {
            code                = m_in_buffer & ((1 << requested_bits) - 1);
            m_in_buffer            >>= requested_bits;
            m_in_buffered_bits    -= requested_bits;
            requested_bits    = 0;
        }
    }
    return code;
}

bool LZW::put_char( ostream& os, int code, int bitcount )
{
    for (int* pCh = m_dic.get_string(code); *pCh >= 0; --pCh)
    {
        os.put(*pCh);
        if (os.fail())
            return false;
        // output the data whose bit counts is same as 'bitcount'.
        // ie. 5bits=>0x1f, 8bits=>0xff.
        // if there is a request to pack the bits, do it in the ostream derived class
    }
    return true;
}


//
// Dictionary
//
void LZW::Dictionary::init( int bitcount ) // ex. 8 bits
{
    if ( bitcount == 1 )
    {
        bitcount = 2;
    }

    BITCOUNT = bitcount;
    m_dic_size = 1 << BITCOUNT;    // ex. size = 256;

    for ( int i = 0; i <= m_dic_size + 1; i++ )
    {
        m_dic[i].prefix     = EMPTY;
        m_dic[i].character  = i;
    }

    CLEAR_CODE  = m_dic_size;        // ex. 0x100
    END_CODE    = m_dic_size + 1;    // ex. 0x101

    m_dic_last_position = m_dic_size + 1;
    BITCOUNT++;
    m_dic_size = 1 << BITCOUNT;    // ex. 512

    m_output_string.resize(MAX_DIC_SIZE);
}

// helper function for encoding
int LZW::Dictionary::get_position( int RM, int NC )
{
    for ( int i = 0; i <= m_dic_last_position; i++ )
    {
        if (( RM == m_dic[i].prefix) && ( NC == m_dic[i].character))
            return i;
    }
    return EMPTY;
}

// helper function for encoding
bool LZW::Dictionary::is_full()
{
    return ( m_dic_last_position == MAX_DIC_SIZE - 1 );
}

// helper function for encoding
void LZW::Dictionary::append( int RM, int NC )
{
    m_dic_last_position++;
    m_dic[ m_dic_last_position ].prefix     = RM;
    m_dic[ m_dic_last_position ].character  = NC;

    if ( m_dic_last_position == m_dic_size && m_dic_last_position + 1 != MAX_DIC_SIZE )
    {
        // when we meet the max size, don't need to expand it
        BITCOUNT++;
        m_dic_size <<= 1;
    }
}

// helper function for decoding
void LZW::Dictionary::insert( int RM, int NC )
{
    int code;

    if ( NC <= m_dic_last_position ) // codeword is already in the dictionary
    {
        code = NC;
    }
    else // codeword is not yet defined
    {
        code = RM;
    }

    while( m_dic[code].prefix != EMPTY )
    {
        code = m_dic[code].prefix;
    }

    m_dic_last_position++;
    m_dic[ m_dic_last_position ].prefix     = RM;
    m_dic[ m_dic_last_position ].character  = m_dic[code].character;

//    char buf[256];
//    wsprintf( buf, "DIC[%4d] - prefix(%4d), character(%02Xh)\r\n",
//        m_dic_last_position, m_dic[ m_dic_last_position ].prefix, m_dic[ m_dic_last_position ].character );
//    OutputDebugString(buf);

    if ( m_dic_last_position == m_dic_size - 1 && m_dic_last_position != MAX_DIC_SIZE - 1 )
    {
        // when we meet the max size, don't need to expand it
        BITCOUNT++;
        m_dic_size <<= 1;
    }
}

// helper function for decoding
int* LZW::Dictionary::get_string( int code )
{
    int* p = m_output_string.data();
    *p = -1;

    if (code > m_dic_last_position)
        return p;

    do
    {
        NEW_STRING& NS = m_dic[code];
        *(++p) = NS.character;
        code = NS.prefix;
    } while (code != EMPTY);

    return p;
}

void LZW::Dictionary::dump()
{
    for (int code = 256; code <= m_dic_last_position; code++)
    {
        for (int* p = get_string(code); *p >= 0; --p)
        {
            //TRACE( "CODE(%4d):", code );
            int character = *p;
            //TRACE( " %2x", character );
        }
        //TRACE("\n");
    }
}

void lzw_unit_tests()
{
    std::stringstream data;
    data.write("abcdabbc", 8);

    std::stringstream encoded_ss;
    std::stringstream decoded_ss;

    LZW().encode(data, encoded_ss, 8);
    LZW().decode(encoded_ss, decoded_ss, 8);

    auto encoded_data = encoded_ss.str();
    auto decoded_data = decoded_ss.str();
}