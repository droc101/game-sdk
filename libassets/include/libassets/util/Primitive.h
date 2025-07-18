//
// Created by droc101 on 7/18/25.
//

#ifndef PRIMITIVE_H
#define PRIMITIVE_H
#include <type_traits>

template<typename T> concept Primitive = std::is_trivial_v<T> && std::is_fundamental_v<T> && std::is_arithmetic_v<T>;

#endif //PRIMITIVE_H
