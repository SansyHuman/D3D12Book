#include "Chapter1.h"

#include <iostream>
#include <windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v)
{
    XMFLOAT3 dst;
    XMStoreFloat3(&dst, v);

    os << "(" << dst.x << ", " << dst.y << ", " << dst.z << ")";
    return os;
}

int main()
{
    std::cout.setf(std::ios_base::boolalpha);

    if(!XMVerifyCPUSupport())
    {
        std::cout << "DirectX math not supported" << std::endl;
        return 0;
    }
    
    XMVECTOR n = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR u = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
    XMVECTOR v = XMVectorSet(-2.0f, 1.0f, -3.0f, 0.0f);
    XMVECTOR w = XMVectorSet(0.707f, 0.707f, 0.0f, 0.0f);

    XMVECTOR a = u + v;
    XMVECTOR b = u - v;
    XMVECTOR c = 10.0f * u;
    XMVECTOR L = XMVector3Length(u);
    XMVECTOR d = XMVector3Normalize(u);
    XMVECTOR s = XMVector3Dot(u, v);
    XMVECTOR e = XMVector3Cross(u, v);
    XMVECTOR projW, perpW;
    XMVector3ComponentsFromNormal(&projW, &perpW, w, n);
    bool equal = XMVector3Equal(projW + perpW, w);
    bool notEqual = XMVector3NotEqual(projW + perpW, w);

    XMVECTOR angleVec = XMVector3AngleBetweenVectors(projW, perpW);
    float angleRadians = XMVectorGetX(angleVec);
    float angleDegrees = XMConvertToDegrees(angleRadians);

    std::cout << "u = " << u << std::endl;
    std::cout << "v = " << v << std::endl;
    std::cout << "w = " << w << std::endl;
    std::cout << "n = " << n << std::endl;
    std::cout << "a = u + v = " << a << std::endl;
    std::cout << "b = u - v = " << b << std::endl;
    std::cout << "c = 10 * u = " << c << std::endl;
    std::cout << "L = ||u|| = " << L << std::endl;
    std::cout << "d = u / ||u|| = " << d << std::endl;
    std::cout << "s = u.v = " << s << std::endl;
    std::cout << "projW = " << projW << std::endl;
    std::cout << "perpW = " << perpW << std::endl;
    std::cout << "projW + perpW == w = " << equal << std::endl;
    std::cout << "projW + perpW != w = " << notEqual << std::endl;
    std::cout << "angle = " << angleDegrees << std::endl;

    return 0;
}
