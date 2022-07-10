#pragma once

#include <type_traits>

#ifndef D3D12BOOK_CONCEPTS
#define D3D12BOOK_CONCEPTS

template <class Child, class Parent>
concept Derived = std::is_base_of<Parent, Child>::value;

template <class Parent, class Child>
concept Super = std::is_base_of<Parent, Child>::value;

template <class Num>
concept Floating = std::is_floating_point<Num>::value;

template <class Num>
concept Integer = std::is_integral<Num>::value;

template <class Type>
concept Number = std::is_arithmetic<Type>::value;

#endif