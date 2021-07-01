#include <iostream>
#include <cstring>
//#include "simulator.hpp"

using namespace std;

#ifndef RISCV_MEMORY_HPP
#define RISCV_MEMORY_HPP

class memoryManager
{
    friend class Simulator;

private:
    unsigned char memory[1000000];

    int hex_to_dec(char c)
    {
        if ('0' <= c && c <= '9') return c - '0';
        if ('A' <= c && c <= 'F') return c - 'A' + 10;
        else return 0;
    }

public:
    memoryManager()
    {
        memset(memory, 0, sizeof(memory));
    }

    void init()
    {
        char ch = cin.get();
        while (!cin.eof())
        {
            int pos = 0;
            if (ch == '@')
            {
                for (int i = 0; i < 8; ++i)
                {
                    ch = cin.get();
                    pos <<= 4;
                    pos += hex_to_dec(ch);
                }
                ch = cin.get();
            }
            while (!cin.eof())
            {
                ch = cin.get();
                if (ch == '@') break;
                if (!(('0' <= ch && ch <= '9') || ('A' <= ch && ch <= 'F'))) continue;
                memory[pos] = hex_to_dec(ch);
                ch = cin.get();
                memory[pos] <<= 4;
                memory[pos++] += hex_to_dec(ch);
            }
        }
    }

    unsigned load(int pos, int len)
    {
        unsigned val = 0;
        for (int i = pos + len - 1; i >= pos; --i)
        {
            val <<= 8;
            val += memory[i];
        }
        return val;
    }

    void store(int pos, unsigned val, int len)
    {
        for (int i = 0; i < len; ++i)
        {
            memory[pos + i] = val & 0xFF;
            val >>= 8;
        }
    }
};

#endif //RISCV_MEMORY_HPP

