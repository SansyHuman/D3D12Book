#pragma once

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;

#ifndef D3D12BOOK_HLSLTYPE_H
#define D3D12BOOK_HLSLTYPE_H

namespace HLSLType
{

#ifdef HLSLTYPE_USE_VECTORS

    struct bool2
    {
        bool x;
        bool y;

        bool2() = default;

        bool2(const bool2&) = default;
        bool2& operator=(const bool2&) = default;

        bool2(bool2&&) = default;
        bool2& operator=(bool2&&) = default;

        XM_CONSTEXPR bool2(bool _x, bool _y) : x(_x), y(_y) {}
        explicit bool2(const bool* pArray) : x(pArray[0]), y(pArray[1]) {}
    };

    struct bool3
    {
        bool x;
        bool y;
        bool z;

        bool3() = default;

        bool3(const bool3&) = default;
        bool3& operator=(const bool3&) = default;

        bool3(bool3&&) = default;
        bool3& operator=(bool3&&) = default;

        XM_CONSTEXPR bool3(bool _x, bool _y, bool _z) : x(_x), y(_y), z(_z) {}
        explicit bool3(const bool* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}
    };

    struct bool4
    {
        bool x;
        bool y;
        bool z;
        bool w;

        bool4() = default;

        bool4(const bool4&) = default;
        bool4& operator=(const bool4&) = default;

        bool4(bool4&&) = default;
        bool4& operator=(bool4&&) = default;

        XM_CONSTEXPR bool4(bool _x, bool _y, bool _z, bool _w) : x(_x), y(_y), z(_z), w(_w) {}
        explicit bool4(const bool* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}
    };

    using int2 = XMINT2;
    using int3 = XMINT3;
    using int4 = XMINT4;

    using uint2 = XMUINT2;
    using uint3 = XMUINT3;
    using uint4 = XMUINT4;

    using float2 = XMFLOAT2;
    using float3 = XMFLOAT3;
    using float4 = XMFLOAT4;

    struct double2
    {
        double x;
        double y;

        double2() = default;

        double2(const double2&) = default;
        double2& operator=(const double2&) = default;

        double2(double2&&) = default;
        double2& operator=(double2&&) = default;

        XM_CONSTEXPR double2(double _x, double _y) : x(_x), y(_y) {}
        explicit double2(const double* pArray) : x(pArray[0]), y(pArray[1]) {}
    };

    struct double3
    {
        double x;
        double y;
        double z;

        double3() = default;

        double3(const double3&) = default;
        double3& operator=(const double3&) = default;

        double3(double3&&) = default;
        double3& operator=(double3&&) = default;

        XM_CONSTEXPR double3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
        explicit double3(const double* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}
    };

    struct double4
    {
        double x;
        double y;
        double z;
        double w;

        double4() = default;

        double4(const double4&) = default;
        double4& operator=(const double4&) = default;

        double4(double4&&) = default;
        double4& operator=(double4&&) = default;

        XM_CONSTEXPR double4(double _x, double _y, double _z, double _w) : x(_x), y(_y), z(_z), w(_w) {}
        explicit double4(const double* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}
    };

#endif

#ifdef HLSLTYPE_USE_MATRICES

    using float3x3 = XMFLOAT3X3;
    using float3x4 = XMFLOAT3X4;
    using float4x3 = XMFLOAT4X3;
    using float4x4 = XMFLOAT4X4;

#endif

}

#endif