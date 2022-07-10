#pragma once

#ifndef D3D12BOOK_GEOMETRYGENERATOR_H
#define D3D12BOOK_GEOMETRYGENERATOR_H

#ifndef HLSLTYPE_USE_VECTORS
#define HLSLTYPE_USE_VECTORS
#endif

#include "hlsltype.h"
#include <vector>
#include <type_traits>

using namespace HLSLType;

template <typename Number>
concept IndexType = std::is_same<std::uint16_t, Number>::value || std::is_same<std::uint32_t, Number>::value;

class GeometryGenerator
{
public:
    struct Vertex
    {
        float3 Position;
        float3 Normal;
        float3 TangentU;
        float2 TexC;

        Vertex() = default;
        Vertex(const Vertex&) = default;
        Vertex& operator=(const Vertex&) = default;

        Vertex(
            const float3& p,
            const float3& n,
            const float3& t,
            const float2& uv
        ) : Position(p), Normal(n), TangentU(t), TexC(uv)
        {
        }

        Vertex(
            float px, float py, float pz,
            float nx, float ny, float nz,
            float tx, float ty, float tz,
            float u, float v) :
            Position(px, py, pz),
            Normal(nx, ny, nz),
            TangentU(tx, ty, tz),
            TexC(u, v)
        {
        }
    };

    template <typename Index> requires IndexType<Index>
    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<Index> Indices;

        std::vector<std::uint16_t>& GetIndices16()
        {
            if(mIndices16.empty())
            {
                mIndices16.resize(Indices.size());
                for(size_t i = 0; i < Indices.size(); i++)
                {
                    mIndices16[i] = (std::uint16_t)Indices[i];
                }
            }
            return mIndices16;
        }

        std::vector<std::uint32_t>& GetIndices32()
        {
            if(mIndices32.empty())
            {
                mIndices32.resize(Indices.size());
                for(size_t i = 0; i < Indices.size(); i++)
                {
                    mIndices32[i] = (std::uint32_t)Indices[i];
                }
            }
            return mIndices32;
        }

    private:
        std::vector<std::uint16_t> mIndices16;
        std::vector<std::uint32_t> mIndices32;

        friend class GeometryGenerator;
    };

    template <typename Index> requires IndexType<Index>
    static MeshData<Index> CreateBox(
        float width, float height, float depth, std::uint32_t numSubdivisions)
    {
        MeshData<Index> meshData;

        if(numSubdivisions > 13)
            numSubdivisions = 13;

        Vertex v[24];

        float w2 = 0.5f * width;
        float h2 = 0.5f * height;
        float d2 = 0.5f * depth;

        // Fill in the front face vertex data.
        v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

        // Fill in the back face vertex data.
        v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        // Fill in the top face vertex data.
        v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

        // Fill in the bottom face vertex data.
        v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        // Fill in the left face vertex data.
        v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
        v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
        v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
        v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

        // Fill in the right face vertex data.
        v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

        meshData.Vertices.assign(&v[0], &v[24]);

        Index i[36];

        // Fill in the front face index data
        i[0] = 0; i[1] = 1; i[2] = 2;
        i[3] = 0; i[4] = 2; i[5] = 3;

        // Fill in the back face index data
        i[6] = 4; i[7] = 5; i[8] = 6;
        i[9] = 4; i[10] = 6; i[11] = 7;

        // Fill in the top face index data
        i[12] = 8; i[13] = 9; i[14] = 10;
        i[15] = 8; i[16] = 10; i[17] = 11;

        // Fill in the bottom face index data
        i[18] = 12; i[19] = 13; i[20] = 14;
        i[21] = 12; i[22] = 14; i[23] = 15;

        // Fill in the left face index data
        i[24] = 16; i[25] = 17; i[26] = 18;
        i[27] = 16; i[28] = 18; i[29] = 19;

        // Fill in the right face index data
        i[30] = 20; i[31] = 21; i[32] = 22;
        i[33] = 20; i[34] = 22; i[35] = 23;

        meshData.Indices.assign(&i[0], &i[36]);

        for(std::uint32_t i = 0; i < numSubdivisions; i++)
            Subdivide<Index>(meshData);

        return meshData;
    }

    template <typename Index> requires IndexType<Index>
    static MeshData<Index> CreateCylinder(
        float bottomRadius, float topRadius,
        float height, std::uint32_t sliceCount, std::uint32_t stackCount
    )
    {
        MeshData<Index> meshData;

        float stackHeight = height / stackCount;
        float radiusStep = (topRadius - bottomRadius) / stackCount;

        for(std::uint32_t i = 0; i < stackCount + 1; i++)
        {
            float y = -0.5f * height + i * stackHeight;
            float r = bottomRadius + i * radiusStep;

            float dTheta = 2 * XM_PI / sliceCount;

            for(std::uint32_t j = 0; j <= sliceCount; j++)
            {
                Vertex vertex;

                float c = cosf(j * dTheta);
                float s = sinf(j * dTheta);

                vertex.Position = float3(r * c, y, r * s);
                vertex.TexC.x = (float)j / sliceCount;
                vertex.TexC.y = 1 - (float)i / stackCount;
                vertex.TangentU = float3(-s, 0, c);

                float dr = bottomRadius - topRadius;
                float3 bitangent(dr * c, -height, dr * s);

                XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
                XMVECTOR B = XMLoadFloat3(&bitangent);
                XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
                XMStoreFloat3(&vertex.Normal, N);

                meshData.Vertices.push_back(vertex);
            }
        }

        std::uint32_t n = sliceCount + 1;

        for(std::uint32_t i = 0; i < stackCount; i++)
        {
            for(std::uint32_t j = 0; j < sliceCount; j++)
            {
                meshData.Indices.push_back((Index)(i * n + j));
                meshData.Indices.push_back((Index)((i + 1) * n + j));
                meshData.Indices.push_back((Index)((i + 1) * n + j + 1));

                meshData.Indices.push_back((Index)(i * n + j));
                meshData.Indices.push_back((Index)((i + 1) * n + j + 1));
                meshData.Indices.push_back((Index)(i * n + j + 1));
            }
        }



        std::uint32_t baseIndex = (std::uint32_t)meshData.Vertices.size();

        float y = 0.5f * height;
        float dTheta = 2 * XM_PI / sliceCount;

        meshData.Vertices.push_back(Vertex(
            float3(0, y, 0), float3(0, 1, 0), float3(1, 0, 0), float2(0.5f, 0.5f)
        ));
        
        for(std::uint32_t i = 0; i <= sliceCount; i++)
        {
            float x = topRadius * cosf(i * dTheta);
            float z = topRadius * sinf(i * dTheta);

            float u = x / height + 0.5f;
            float v = z / height + 0.5f;

            meshData.Vertices.push_back(Vertex(
                float3(x, y, z), float3(0, 1, 0), float3(1, 0, 0), float2(u, v)
            ));
        }

        for(std::uint32_t i = 0; i < sliceCount; i++)
        {
            meshData.Indices.push_back((Index)baseIndex);
            meshData.Indices.push_back((Index)baseIndex + 1 + i + 1);
            meshData.Indices.push_back((Index)baseIndex + 1 + i);
        }



        baseIndex = (std::uint32_t)meshData.Vertices.size();
        y = -0.5f * height;

        meshData.Vertices.push_back(Vertex(
            float3(0, y, 0), float3(0, -1, 0), float3(1, 0, 0), float2(0.5f, 0.5f)
        ));

        for(std::uint32_t i = 0; i <= sliceCount; i++)
        {
            float x = bottomRadius * cosf(i * dTheta);
            float z = bottomRadius * sinf(i * dTheta);

            float u = x / height + 0.5f;
            float v = z / height + 0.5f;

            meshData.Vertices.push_back(Vertex(
                float3(x, y, z), float3(0, -1, 0), float3(1, 0, 0), float2(u, v)
            ));
        }

        for(std::uint32_t i = 0; i < sliceCount; i++)
        {
            meshData.Indices.push_back((Index)baseIndex);
            meshData.Indices.push_back((Index)baseIndex + 1 + i);
            meshData.Indices.push_back((Index)baseIndex + 1 + i + 1);
        }

        return meshData;
    }

    template <typename Index> requires IndexType<Index>
    static MeshData<Index> CreateSphere(
        float radius, std::uint32_t sliceCount, std::uint32_t stackCount
    )
    {
        MeshData<Index> meshData;

        for(std::uint32_t i = 0; i <= sliceCount; i++)
        {
            for(std::uint32_t j = 0; j <= stackCount; j++)
            {
                Vertex vertex;

                float theta = XM_PI * j / stackCount;
                float phi = XM_PI * 2 * i / sliceCount;

                vertex.Position.x = radius * sinf(theta) * cosf(phi);
                vertex.Position.y = radius * cosf(theta);
                vertex.Position.z = radius * sinf(theta) * sinf(phi);

                vertex.Normal = float3(sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi));

                vertex.TangentU = float3(-sinf(phi), 0, cosf(phi));

                vertex.TexC.x = (float)i / sliceCount;
                vertex.TexC.y = (float)j / stackCount;

                meshData.Vertices.push_back(vertex);
            }
        }

        std::uint32_t sliceVertexCount = stackCount + 1;

        for(std::uint32_t i = 0; i < sliceCount; i++)
        {
            for(std::uint32_t j = 0; j < stackCount; j++)
            {
                Index v0 = (Index)(i * sliceVertexCount + j);
                Index v1 = (Index)(i * sliceVertexCount + j + 1);
                Index v2 = (Index)((i + 1) * sliceVertexCount + j + 1);
                Index v3 = (Index)((i + 1) * sliceVertexCount + j);

                meshData.Indices.push_back(v0);
                meshData.Indices.push_back(v3);
                meshData.Indices.push_back(v1);

                meshData.Indices.push_back(v3);
                meshData.Indices.push_back(v2);
                meshData.Indices.push_back(v1);
            }
        }

        return meshData;
    }

    template <typename Index> requires IndexType<Index>
    static MeshData<Index> CreateGeosphere(
        float radius, std::uint32_t numSubdivisions
    )
    {
        MeshData<Index> meshData;

        if(numSubdivisions > 13)
            numSubdivisions = 13;

        constexpr float X = 0.525731f;
        constexpr float Z = 0.850651f;

        float3 pos[12] =
        {
            float3(-X, 0.0f, Z),  float3(X, 0.0f, Z),
            float3(-X, 0.0f, -Z), float3(X, 0.0f, -Z),
            float3(0.0f, Z, X),   float3(0.0f, Z, -X),
            float3(0.0f, -Z, X),  float3(0.0f, -Z, -X),
            float3(Z, X, 0.0f),   float3(-Z, X, 0.0f),
            float3(Z, -X, 0.0f),  float3(-Z, -X, 0.0f)
        };

        Index k[60] = 
        {
            1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
            1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
            3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
            10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
        };

        meshData.Vertices.resize(12);
        meshData.Indices.assign(&k[0], &k[60]);

        for(std::uint32_t i = 0; i < 12; i++)
        {
            meshData.Vertices[i].Position = pos[i];
        }

        for(std::uint32_t i = 0; i < numSubdivisions; i++)
        {
            Subdivide<Index>(meshData);
        }

        for(std::uint32_t i = 0; i < meshData.Vertices.size(); i++)
        {
            XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));
            XMVECTOR p = radius * n;

            XMStoreFloat3(&meshData.Vertices[i].Position, p);
            XMStoreFloat3(&meshData.Vertices[i].Normal, n);

            float phi = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);
            if(phi < 0)
                phi += XM_2PI;

            float theta = acosf(meshData.Vertices[i].Position.y / radius);

            meshData.Vertices[i].TexC.x = phi / XM_2PI;
            meshData.Vertices[i].TexC.y = theta / XM_PI;

            meshData.Vertices[i].TangentU.x = -sinf(phi);
            meshData.Vertices[i].TangentU.y = 0;
            meshData.Vertices[i].TangentU.z = cosf(phi);
        }

        return meshData;
    }

    template <typename Index> requires IndexType<Index>
    static MeshData<Index> CreateGrid(
        float width, float depth, std::uint32_t m, std::uint32_t n
    )
    {
        MeshData<Index> meshData;

        std::uint32_t vertexCount = m * n;
        std::uint32_t faceCount = (m - 1) * (n - 1) * 2;

        float halfWidth = 0.5f * width;
        float halfDepth = 0.5f * depth;

        float dx = width / (n - 1);
        float dz = depth / (m - 1);

        meshData.Vertices.resize(vertexCount);

        for(std::uint32_t i = 0; i < m; i++)
        {
            float z = halfDepth - i * dz;
            for(std::uint32_t j = 0; j < n; j++)
            {
                float x = -halfWidth + j * dx;

                meshData.Vertices[i * n + j].Position = float3(x, 0, z);
                meshData.Vertices[i * n + j].Normal = float3(0, 1, 0);
                meshData.Vertices[i * n + j].TangentU = float3(1, 0, 0);
                meshData.Vertices[i * n + j].TexC = float2((float)j / (n-1), (float)i / (m - 1));
            }
        }

        meshData.Indices.resize(faceCount * 3);

        std::uint32_t k = 0;

        for(std::uint32_t i = 0; i < m - 1; i++)
        {
            for(std::uint32_t j = 0; j < n - 1; j++)
            {
                meshData.Indices[k] = (Index)(i * n + j);
                meshData.Indices[k + 1] = (Index)(i * n + j + 1);
                meshData.Indices[k + 2] = (Index)((i + 1) * n + j);

                meshData.Indices[k + 3] = (Index)((i + 1) * n + j);
                meshData.Indices[k + 4] = (Index)(i * n + j + 1);
                meshData.Indices[k + 5] = (Index)((i + 1) * n + j + 1);

                k += 6;
            }
        }

        return meshData;
    }

    template <typename Index> requires IndexType<Index>
    static MeshData<Index> CreateQuad(
        float x, float y, float w, float h, float depth
    )
    {
        MeshData<Index> meshData;

        meshData.Vertices.resize(4);
        meshData.Indices.resize(6);

        meshData.Vertices[0] = Vertex(
            x, y - h, depth,
            0.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f);

        meshData.Vertices[1] = Vertex(
            x, y, depth,
            0.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 0.0f);

        meshData.Vertices[2] = Vertex(
            x + w, y, depth,
            0.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f);

        meshData.Vertices[3] = Vertex(
            x + w, y - h, depth,
            0.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f);

        meshData.Indices[0] = (Index)0;
        meshData.Indices[1] = (Index)1;
        meshData.Indices[2] = (Index)2;
                              (Index)
        meshData.Indices[3] = (Index)0;
        meshData.Indices[4] = (Index)2;
        meshData.Indices[5] = (Index)3;

        return meshData;
    }

    template <typename Index> requires IndexType<Index>
    static void Subdivide(MeshData<Index>& meshData)
    {
        MeshData<Index> inputCopy = meshData;

        meshData.Vertices.resize(0);
        meshData.Indices.resize(0);
        meshData.mIndices16.resize(0);
        meshData.mIndices32.resize(0);

        //       v1
        //       *
        //      / \
	    //     /   \
	    //  m0*-----*m1
        //   / \   / \
	    //  /   \ /   \
	    // *-----*-----*
        // v0    m2     v2

        std::uint32_t numTris = (std::uint32_t)inputCopy.Indices.size() / 3;
        for(std::uint32_t i = 0; i < numTris; i++)
        {
            Vertex v0 = inputCopy.Vertices[inputCopy.Indices[i * 3]];
            Vertex v1 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 1]];
            Vertex v2 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 2]];

            Vertex m0 = MidPoint(v0, v1);
            Vertex m1 = MidPoint(v1, v2);
            Vertex m2 = MidPoint(v0, v2);

            meshData.Vertices.push_back(v0); // 0
            meshData.Vertices.push_back(v1); // 1
            meshData.Vertices.push_back(v2); // 2
            meshData.Vertices.push_back(m0); // 3
            meshData.Vertices.push_back(m1); // 4
            meshData.Vertices.push_back(m2); // 5

            meshData.Indices.push_back(i * 6);
            meshData.Indices.push_back(i * 6 + 3);
            meshData.Indices.push_back(i * 6 + 5);

            meshData.Indices.push_back(i * 6 + 3);
            meshData.Indices.push_back(i * 6 + 4);
            meshData.Indices.push_back(i * 6 + 5);

            meshData.Indices.push_back(i * 6 + 5);
            meshData.Indices.push_back(i * 6 + 4);
            meshData.Indices.push_back(i * 6 + 2);

            meshData.Indices.push_back(i * 6 + 3);
            meshData.Indices.push_back(i * 6 + 1);
            meshData.Indices.push_back(i * 6 + 4);
        }
    }

    static Vertex MidPoint(const Vertex& v0, const Vertex& v1)
    {
        XMVECTOR p0 = XMLoadFloat3(&v0.Position);
        XMVECTOR p1 = XMLoadFloat3(&v1.Position);

        XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
        XMVECTOR n1 = XMLoadFloat3(&v1.Normal);

        XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
        XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);

        XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
        XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

        XMVECTOR p = 0.5f * (p0 + p1);
        XMVECTOR n = XMVector3Normalize(0.5f * (n0 + n1));
        XMVECTOR tan = XMVector3Normalize(0.5f * (tan0 + tan1));
        XMVECTOR tex = 0.5f * (tex0 + tex1);

        Vertex v;
        XMStoreFloat3(&v.Position, p);
        XMStoreFloat3(&v.Normal, n);
        XMStoreFloat3(&v.TangentU, tan);
        XMStoreFloat2(&v.TexC, tex);

        return v;
    }
};

#endif