// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "String.h"
#include "Macros.h"

#include <limits.h>

namespace aga
{
    void StrCopy(char *_dst, const char *_src)
    {
        while ((*_dst++ = *_src++))
            ;
    }

    char *IToA(int number, char *dest, size_t dest_size, int base)
    {
        if (dest == nullptr || base < 2 || base > 36)
        {
            return nullptr;
        }

        char buf[sizeof number * CHAR_BIT + 2];  // worst case: itoa(INT_MIN,,,2)
        char *p = &buf[sizeof buf - 1];

        // Work with negative absolute value to avoid UB of `abs(INT_MIN)`
        int neg_num = number < 0 ? number : -number;

        // Form string
        *p = '\0';

        do
        {
            *--p = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[-(neg_num % base)];
            neg_num /= base;
        } while (neg_num);

        if (number < 0)
        {
            *--p = '-';
        }

        // Copy string
        size_t src_size = (size_t)(&buf[sizeof buf] - p);

        if (src_size > dest_size)
        {
            // Not enough room
            return nullptr;
        }

        return (char *)memcpy(dest, p, src_size);
    }

    String::String()
    {
        m_Length = 0;
        m_Data = new char[0];
    }

    String::String(char c)
    {
        m_Length = 1;
        m_Data = new char(c);
    }

    String::String(uint32_t c)
    {
        char dest[100];
        IToA(c, dest, 100, 10);

        uint32_t i = 0;
        while (dest[i] != '\0')
        {
            i++;
        }

        m_Length = i;
        m_Data = new char[i];

        for (uint32_t j = 0; j < i; j++)
        {
            m_Data[j] = dest[j];
        }
    }

    String::String(const char *c)
    {
        if (c)
        {
            uint32_t n = 0;

            while (c[n] != '\0')
            {
                n++;
            }

            m_Length = n;
            m_Data = new char[n];

            for (uint32_t j = 0; j < n; j++)
            {
                m_Data[j] = c[j];
            }
        }
        else
        {
            m_Length = 0;
            m_Data = new char[0];
        }
    }

    String::String(const String &s)
    {
        m_Length = s.Length();
        m_Data = new char[m_Length];

        for (uint32_t j = 0; j < m_Length; j++)
        {
            m_Data[j] = s[j];
        }
    }

    String::~String()
    {
        SAFE_DELETE_ARRAY(m_Data);
        m_Length = 0;
    }

    uint32_t String::Length() const
    {
        return m_Length;
    }

    int String::IndexOf(char c) const
    {
        for (uint32_t j = 0; j < m_Length; j++)
        {
            if (m_Data[j] == c)
            {
                return (int)j;
            }
        }

        return -1;
    }

    void String::ToUpperCase(uint32_t first, uint32_t last)
    {
        if ((first >= last) || (last > m_Length))
        {
            return;
        }

        for (uint32_t j = first; j < last; j++)
        {
            if ('a' <= m_Data[j] && m_Data[j] <= 'z')
            {
                m_Data[j] -= ('a' - 'A');
            }
        }
    }

    void String::ToLowerCase(uint32_t first, uint32_t last)
    {
        if ((first >= last) || (last > m_Length))
        {
            return;
        }

        for (uint32_t j = first; j < last; j++)
        {
            if ('A' <= m_Data[j] && m_Data[j] <= 'Z')
            {
                m_Data[j] += ('a' - 'A');
            }
        }
    }

    void String::ToggleCase(uint32_t first, uint32_t last)
    {
        if ((first >= last) || (last > m_Length))
        {
            return;
        }

        for (uint32_t j = first; j < last; j++)
        {
            if ('A' <= m_Data[j] && m_Data[j] <= 'Z')
            {
                m_Data[j] += ('a' - 'A');
            }
            else if ('a' <= m_Data[j] && m_Data[j] <= 'z')
            {
                m_Data[j] -= ('a' - 'A');
            }
        }
    }

    std::ostream &operator<<(std::ostream &os, const String &s)
    {
        if (s.Length() > 0)
        {
            for (uint32_t j = 0; j < s.Length(); j++)
            {
                os << s[j];
            }
        }
        else
        {
            os << "";
        }

        return os;
    }

    std::istream &operator>>(std::istream &is, String &s)
    {
        char *c = new char[1000];
        is >> c;
        s = String(c);

        SAFE_DELETE_ARRAY(c);

        return is;
    }

    char String::operator[](uint32_t j) const
    {
        return m_Data[j];
    }

    char &String::operator[](uint32_t j)
    {
        return m_Data[j];
    }

    String &String::operator=(const String &s)
    {
        if (this == &s)
        {
            return *this;
        }

        SAFE_DELETE_ARRAY(m_Data);

        m_Length = s.Length();
        m_Data = new char[m_Length];

        for (uint32_t j = 0; j < m_Length; j++)
        {
            m_Data[j] = s[j];
        }

        return *this;
    }

    const char *String::GetData()
    {
        return m_Data;
    }

    String &String::operator+=(const String &s)
    {
        uint32_t len = m_Length + s.Length();
        char *str = new char[len];

        for (uint32_t j = 0; j < m_Length; j++)
        {
            str[j] = m_Data[j];
        }

        for (uint32_t i = 0; i < s.Length(); i++)
        {
            str[m_Length + i] = s[i];
        }

        SAFE_DELETE_ARRAY(m_Data);

        m_Length = len;
        m_Data = str;

        return *this;
    }

    String operator+(const String &lhs, const String &rhs)
    {
        return String(lhs) += rhs;
    }

    String operator+(const String &lhs, char rhs)
    {
        return String(lhs) += String(rhs);
    }

    String operator+(const String &lhs, const char *rhs)
    {
        return String(lhs) += String(rhs);
    }

    String operator+(char lhs, const String &rhs)
    {
        return String(lhs) += rhs;
    }
    String operator+(const char *lhs, const String &rhs)
    {
        return String(lhs) += rhs;
    }

    bool operator==(const String &lhs, const String &rhs)
    {
        if (lhs.Length() != rhs.Length())
        {
            return false;
        }

        uint32_t cap = lhs.Length();
        uint32_t n = 0;
        while ((n < cap) && (lhs[n] == rhs[n]))
        {
            n++;
        }

        return (n == cap);
    }

    bool operator==(const String &lhs, char rhs)
    {
        return (lhs == String(rhs));
    }

    bool operator==(const String &lhs, const char *rhs)
    {
        return (lhs == String(rhs));
    }

    bool operator==(char lhs, const String &rhs)
    {
        return (String(lhs) == rhs);
    }

    bool operator==(const char *lhs, const String &rhs)
    {
        return (String(lhs) == rhs);
    }

    bool operator>(const String &lhs, const String &rhs)
    {
        uint32_t cap = (lhs.Length() < rhs.Length()) ? lhs.Length() : rhs.Length();
        uint32_t n = 0;
        while ((n < cap) && (lhs[n] == rhs[n]))
        {
            n++;
        }

        if (n == cap)
        {
            return (lhs.Length() > rhs.Length());
        }

        if ((('A' <= lhs[n] && lhs[n] <= 'Z') || ('a' <= lhs[n] && lhs[n] <= 'z')) &&
            (('A' <= rhs[n] && rhs[n] <= 'Z') || ('a' <= rhs[n] && rhs[n] <= 'z')))
        {
            char A = (lhs[n] & ~32);
            char B = (rhs[n] & ~32);

            if (A != B)
            {
                return (A > B);
            }
        }

        return lhs[n] > rhs[n];
    }

    bool operator>(const String &lhs, char rhs)
    {
        return (lhs > String(rhs));
    }

    bool operator>(const String &lhs, const char *rhs)
    {
        return (lhs > String(rhs));
    }

    bool operator>(char lhs, const String &rhs)
    {
        return (String(lhs) > rhs);
    }

    bool operator>(const char *lhs, const String &rhs)
    {
        return (String(lhs) > rhs);
    }

    bool operator!=(const String &lhs, const String &rhs)
    {
        return !(lhs == rhs);
    }

    bool operator!=(const String &lhs, char rhs)
    {
        return !(lhs == rhs);
    }

    bool operator!=(const String &lhs, const char *rhs)
    {
        return !(lhs == rhs);
    }

    bool operator!=(char lhs, const String &rhs)
    {
        return !(lhs == rhs);
    }

    bool operator!=(const char *lhs, const String &rhs)
    {
        return !(lhs == rhs);
    }

    bool operator<(const String &lhs, const String &rhs)
    {
        return !(lhs == rhs) && !(lhs > rhs);
    }

    bool operator<(const String &lhs, char rhs)
    {
        return !(lhs == rhs) && !(lhs > rhs);
    }

    bool operator<(const String &lhs, const char *rhs)
    {
        return !(lhs == rhs) && !(lhs > rhs);
    }

    bool operator<(char lhs, const String &rhs)
    {
        return !(lhs == rhs) && !(lhs > rhs);
    }

    bool operator<(const char *lhs, const String &rhs)
    {
        return !(lhs == rhs) && !(lhs > rhs);
    }

    bool operator<=(const String &lhs, const String &rhs)
    {
        return !(lhs > rhs);
    }

    bool operator<=(const String &lhs, char rhs)
    {
        return !(lhs > rhs);
    }

    bool operator<=(const String &lhs, const char *rhs)
    {
        return !(lhs > rhs);
    }

    bool operator<=(char lhs, const String &rhs)
    {
        return !(lhs > rhs);
    }

    bool operator<=(const char *lhs, const String &rhs)
    {
        return !(lhs > rhs);
    }

    bool operator>=(const String &lhs, const String &rhs)
    {
        return (lhs == rhs) || (lhs > rhs);
    }

    bool operator>=(const String &lhs, char rhs)
    {
        return (lhs == rhs) || (lhs > rhs);
    }

    bool operator>=(const String &lhs, const char *rhs)
    {
        return (lhs == rhs) || (lhs > rhs);
    }

    bool operator>=(char lhs, const String &rhs)
    {
        return (lhs == rhs) || (lhs > rhs);
    }

    bool operator>=(const char *lhs, const String &rhs)
    {
        return (lhs == rhs) || (lhs > rhs);
    }
}  // namespace aga