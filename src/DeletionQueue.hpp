#pragma once

#include <deque>
#include <functional>

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()> &&function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        // 反向遍历，最后创建的最先销毁
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)();
        }
        deletors.clear();
    }
};
