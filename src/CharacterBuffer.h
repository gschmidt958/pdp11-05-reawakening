#ifndef CHARACTERBUFFER_H
#define CHARACTERBUFFER_H

#include <string.h>

class CharacterBuffer
{
public:
    static const unsigned int CharacterBufferSize = 128;

    CharacterBuffer()
    {
        Clear();
    }

    void AppendCharacter(char c)
    {
        if (c == '\r') return; // normalize \r\n — only \n drives state transitions
        if (index < CharacterBufferSize - 1)
        {
            str[index++] = c;
            str[index]   = '\0';
        }
    }

    void        Clear()                      { index = 0; str[0] = '\0'; }
    bool        IsEmpty()              const { return index == 0; }
    bool        StartsWith(char c)     const { return str[0] == c; }
    bool        StartsWithToken(const char* s) const { return str[0] == s[0] && str[1] == s[1]; }
    const char* ToString()             const { return str; }

    bool operator==(const char* other) const { return strcmp(str, other) == 0; }
    bool operator!=(const char* other) const { return strcmp(str, other) != 0; }

private:
    char         str[CharacterBufferSize];
    unsigned int index;
};

#endif // CHARACTERBUFFER_H