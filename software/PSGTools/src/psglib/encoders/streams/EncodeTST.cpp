#include "EncodeTST.h"

////////////////////////////////////////////////////////////////////////////////

void EncodeTST::BitStream::Open(std::ostream& stream)
{
    m_stream = &stream;
    m_buffer = m_count = 0;
}

void EncodeTST::BitStream::Write(uint32_t data, uint8_t size)
{
    if (m_stream)
    {
        auto maxSize = uint8_t(sizeof(data) * 8);
        size = std::min(size, maxSize);
        data <<= (maxSize - size);

        for (int i = 0; i < size; ++i)
        {
            m_buffer <<= 1;
            m_buffer |= (data >> (maxSize - 1) & 1);
            data <<= 1;
            m_count++;

            if (m_count == 8)
            {
                (*m_stream) << m_buffer;
                m_buffer = m_count = 0;
            }
        }
    }
}

void EncodeTST::BitStream::Close()
{
    if (m_stream && m_count)
    {
        m_buffer <<= (8 - m_count);
        (*m_stream) << m_buffer;
        m_buffer = m_count = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

std::vector<CharType> operator + (std::vector<CharType> vc, CharType c)
{
    vc.push_back(c);
    return vc;
}



bool EncodeTST::Open(const Stream& stream)
{
    if (CheckFileExt(stream, "tst"))
    {
        m_output.open(stream.file, std::fstream::binary);
        if (m_output)
        {
            //
            m_bitStream.Open(m_output);
            return true;
        }
    }
    return false;
}

void EncodeTST::Encode(const Frame& frame)
{
    for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
    {
        if (frame[0].IsChanged(reg))
        {
            int8_t delta = (frame[0].Read(reg) - m_frame[0].Read(reg));
            c = reg << 8 | delta;
            compress_char();
        }
    }
    m_frame = frame;
}

void EncodeTST::Close(const Stream& stream)
{
    compress_last();
    //
    m_bitStream.Close();
    m_output.close();
}

void EncodeTST::reset_dictionary()
{
    dictionary.clear();
    //
}

//list<Node> encodeLZ78(string s) :
//    string buffer = ""                              // текущий префикс             
//    map<string, int> dict = {}                      // словарь
//    list<Node> ans = []                             // ответ
//    for i = 0 ..s.length - 1 :
//        if dict.hasKey(buffer + s[i]) :              // можем ли мы увеличить префикс
//            buffer += s[i]
//        else :
//            ans.push({ dict[buffer], s[i] })          // добавляем пару в ответ
//            dict[buffer + s[i]] = dict.length + 1   // добавляем слово в словарь
//            buffer = ""                             // сбрасываем буфер
    //if not (buffer is empty) : // если буффер не пуст - этот код уже был, нужно его добавить в конец словаря
    //    last_ch = buffer.peek() // берем последний символ буффера, как "новый" символ
    //    buffer.pop() // удаляем последний символ из буфера
    //    ans.push({ dict[buffer], last_ch }) // добавляем пару в ответ 
    //    return ans

void EncodeTST::compress_char()
{
    // dictionary's maximum size was reached
    if (dictionary.size() == (1 << CodeTypeSize) - 1)
        reset_dictionary();

    if (dictionary.count(s + c) != 0)
    {
        s.push_back(c);
    }
    else
    {
        CodeType code = dictionary.count(s) != 0 ? dictionary.at(s) : 0;
        m_bitStream.Write(code, CodeTypeSize);
        m_bitStream.Write(c, CharTypeSize);

        CodeType newCode = dictionary.size() + 1;
        dictionary[s + c] = newCode;

        s.clear();
    }
}

void EncodeTST::compress_last()
{
    if (!s.empty())
    {
        CharType last_ch = s.back();
        s.pop_back();

        CodeType code = dictionary.count(s) != 0 ? dictionary.at(s) : 0;
        m_bitStream.Write(code, CodeTypeSize);
        m_bitStream.Write(last_ch, CharTypeSize);
    }
}
