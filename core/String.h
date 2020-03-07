// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "Common.h"

namespace aga
{
    void StrCopy(char *_dst, const char *_src);
    char *IToA(int number, char *dest, size_t dest_size, int base);

    class String
    {
    public:
        String();
        String(char c);
        String(uint32_t c);
        String(const char *c);
        String(const String &s);
        ~String();

        uint32_t Length() const;

        int IndexOf(char c) const;

        void ToUpperCase(uint32_t first, uint32_t last);
        void ToLowerCase(uint32_t first, uint32_t last);
        void ToggleCase(uint32_t first, uint32_t last);

        friend std::ostream &operator<<(std::ostream &so, const String &s);
        friend std::istream &operator>>(std::istream &so, String &s);

        char operator[](uint32_t j) const;
        char &operator[](uint32_t j);

        String &operator=(const String &s);

        String &operator+=(const String &s);

        const char *GetData();

        operator const char *() const;

        friend String operator+(const String &lhs, const String &rhs);
        friend String operator+(const String &lhs, char rhs);
        friend String operator+(const String &lhs, const char *rhs);
        friend String operator+(char lhs, const String &rhs);
        friend String operator+(const char *lhs, const String &rhs);

        friend bool operator==(const String &lhs, const String &rhs);
        friend bool operator==(const String &lhs, char rhs);
        friend bool operator==(const String &lhs, const char *rhs);
        friend bool operator==(char lhs, const String &rhs);
        friend bool operator==(const char *lhs, const String &rhs);

        friend bool operator>(const String &lhs, const String &rhs);
        friend bool operator>(const String &lhs, char rhs);
        friend bool operator>(const String &lhs, const char *rhs);
        friend bool operator>(char lhs, const String &rhs);
        friend bool operator>(const char *lhs, const String &rhs);

        friend bool operator!=(const String &lhs, const String &rhs);
        friend bool operator!=(const String &lhs, char rhs);
        friend bool operator!=(const String &lhs, const char *rhs);
        friend bool operator!=(char lhs, const String &rhs);
        friend bool operator!=(const char *lhs, const String &rhs);

        friend bool operator<(const String &lhs, const String &rhs);
        friend bool operator<(const String &lhs, char rhs);
        friend bool operator<(const String &lhs, const char *rhs);
        friend bool operator<(char lhs, const String &rhs);
        friend bool operator<(const char *lhs, const String &rhs);

        friend bool operator<=(const String &lhs, const String &rhs);
        friend bool operator<=(const String &lhs, char rhs);
        friend bool operator<=(const String &lhs, const char *rhs);
        friend bool operator<=(char lhs, const String &rhs);
        friend bool operator<=(const char *lhs, const String &rhs);

        friend bool operator>=(const String &lhs, const String &rhs);
        friend bool operator>=(const String &lhs, char rhs);
        friend bool operator>=(const String &lhs, const char *rhs);
        friend bool operator>=(char lhs, const String &rhs);
        friend bool operator>=(const char *lhs, const String &rhs);

    private:
        char *m_Data;
        uint32_t m_Length;
    };
}  // namespace aga
