// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "Logger.h"
#include "Common.h"

#include <memory>

namespace aga
{
    struct MemoryTracker
    {
    public:
        static MemoryTracker &getInstance()
        {
            static MemoryTracker instance;
            return instance;
        }

    private:
        MemoryTracker()
        {
        }

    public:
        MemoryTracker(MemoryTracker const &) = delete;
        void operator=(MemoryTracker const &) = delete;

        void IncreaseAllocationsCount(uint32_t by)
        {
            m_AllocationsCount += by;
        }

        void PrintStatistics()
        {
            LOG_INFO("Allocations count: " + String(m_AllocationsCount) + "\n");
        }

    private:
        uint32_t m_AllocationsCount;
    };  // namespace aga
}  // namespace aga

void *operator new(size_t size)
{
    //   aga::MemoryTracker::getInstance().IncreaseAllocationsCount(1);

    return std::malloc(size);
}

void *operator new[](size_t size)
{
    //  aga::MemoryTracker::getInstance().IncreaseAllocationsCount(1);

    return std::malloc(size);
}

void operator delete(void *memory)
{
    //   aga::MemoryTracker::getInstance().IncreaseAllocationsCount(-1);

    std::free(memory);
}

void operator delete[](void *memory)
{
    //   aga::MemoryTracker::getInstance().IncreaseAllocationsCount(-1);

    std::free(memory);
}