// Chapter2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Chapter2.h"
#include <iostream>
#include <windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v)
{
    XMFLOAT4 dst;
    XMStoreFloat4(&dst, v);

    os << "(" << dst.x << ", " << dst.y << ", " << dst.z << ", " << dst.w << ")";
    return os;
}

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMMATRIX m)
{
    for(int i = 0; i < 4; i++)
    {
        os << XMVectorGetX(m.r[i]) << "\t";
        os << XMVectorGetY(m.r[i]) << "\t";
        os << XMVectorGetZ(m.r[i]) << "\t";
        os << XMVectorGetW(m.r[i]) << std::endl;
    }

    return os;
}

int main()
{
    if(!XMVerifyCPUSupport())
    {
        std::cout << "DirectX math not supported" << std::endl;
        return 0;
    }

    XMMATRIX A(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 4.0f, 0.0f,
        1.0f, 2.0f, 3.0f, 1.0f
    );

    XMMATRIX B = XMMatrixIdentity();

    XMMATRIX C = A * B;

    XMMATRIX D = XMMatrixTranspose(A);

    XMVECTOR det = XMMatrixDeterminant(A);
    XMMATRIX E = XMMatrixInverse(&det, A);

    XMMATRIX F = A * E;

    std::cout << "A = " << std::endl << A << std::endl;
    std::cout << "B = " << std::endl << B << std::endl;
    std::cout << "C = A*B = " << std::endl << C << std::endl;
    std::cout << "D = transpose(A) = " << std::endl << D << std::endl;
    std::cout << "det = determinant(A) = " << std::endl << det << std::endl;
    std::cout << "E = inverse(A) = " << std::endl << E << std::endl;
    std::cout << "F = A*E = " << std::endl << F << std::endl;
}