//
// Created by NBT22 on 8/6/25.
//

#pragma once

#include <vector>
#include <cstddef>

template<typename T> static inline void MoveBack(std::vector<T> &vector, size_t index)
{
    if (index > 0)
    {
        std::iter_swap(vector.begin() + index, vector.begin() + index - 1);
    }
}

template<typename T> static inline void MoveForward(std::vector<T> &vec, size_t index)
{
    if (index + 1 < vec.size())
    {
        std::iter_swap(vec.begin() + index, vec.begin() + index + 1);
    }
}
