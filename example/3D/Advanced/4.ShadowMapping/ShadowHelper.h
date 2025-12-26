#pragma once 

struct OrthoProjInfo
{
    float r;        // right
    float l;        // left
    float b;        // bottom
    float t;        // top
    float n;        // z near
    float f;        // z far

    float Width;
    float Height;    

    void Print()
    {
        printf("Left %f   Right %f\n", l, r);
        printf("Bottom %f Top %f\n", b, t);
        printf("Near %f   Far %f\n", n, f);
    }
};

class AABB
{
public:
    AABB() {}

    void Add(const glm::vec3& v)
    {
        MinX = glm::min(MinX, v.x);
        MinY = glm::min(MinY, v.y);
        MinZ = glm::min(MinZ, v.z);

        MaxX = glm::max(MaxX, v.x);
        MaxY = glm::max(MaxY, v.y);
        MaxZ = glm::max(MaxZ, v.z);
    }

    float MinX = FLT_MAX;
    float MaxX = -FLT_MAX;
    float MinY = FLT_MAX;
    float MaxY = -FLT_MAX;
    float MinZ = FLT_MAX;
    float MaxZ = -FLT_MAX;

    void Print()
    {
        printf("X: [%f,%f]\n", MinX, MaxX);
        printf("Y: [%f,%f]\n", MinY, MaxY);
        printf("Z: [%f,%f]\n", MinZ, MaxZ);
    }

    void UpdateOrthoInfo(struct OrthoProjInfo& o)
    {
        o.r = MaxX;
        o.l = MinX;
        o.b = MinY;
        o.t = MaxY;
        o.n = MinZ;
        o.f = MaxZ;
    }
};


class Frustum
{
public:
    glm::vec4 NearTopLeft;
    glm::vec4 NearBottomLeft;
    glm::vec4 NearTopRight;
    glm::vec4 NearBottomRight;

    glm::vec4 FarTopLeft;
    glm::vec4 FarBottomLeft;
    glm::vec4 FarTopRight;
    glm::vec4 FarBottomRight;

    Frustum() {}

    void CalcCorners(const std::shared_ptr<RenderSys::PerspectiveCamera> persCamera)
    {
        float tanHalfFOV = tanf(glm::radians(persCamera->GetFOV() / 2.0f));

        float NearZ = persCamera->GetNearClip();
        float NearX = NearZ * tanHalfFOV;
        float NearY = NearZ * tanHalfFOV * persCamera->GetAspectRatio();

        NearTopLeft     = glm::vec4(-NearX, NearY, NearZ, 1.0f);
        NearBottomLeft  = glm::vec4(-NearX, -NearY, NearZ, 1.0f);
        NearTopRight    = glm::vec4(NearX, NearY, NearZ, 1.0f);
        NearBottomRight = glm::vec4(NearX, -NearY, NearZ, 1.0f);

        float FarZ = persCamera->GetFarClip();
        float FarX = FarZ * tanHalfFOV;
        float FarY = FarZ * tanHalfFOV * persCamera->GetAspectRatio();

        FarTopLeft     = glm::vec4(-FarX, FarY, FarZ, 1.0f);
        FarBottomLeft  = glm::vec4(-FarX, -FarY, FarZ, 1.0f);
        FarTopRight    = glm::vec4(FarX, FarY, FarZ, 1.0f);
        FarBottomRight = glm::vec4(FarX, -FarY, FarZ, 1.0f);
    }


    void Transform(const glm::mat4& m)
    {
         NearTopLeft     = m * NearTopLeft;
         NearBottomLeft  = m * NearBottomLeft;
         NearTopRight    = m * NearTopRight;
         NearBottomRight = m * NearBottomRight;

         FarTopLeft     = m * FarTopLeft;
         FarBottomLeft  = m * FarBottomLeft;
         FarTopRight    = m * FarTopRight;
         FarBottomRight = m * FarBottomRight;
    }


    void CalcAABB(AABB& aabb)
    {
        aabb.Add(NearTopLeft);
        aabb.Add(NearBottomLeft);
        aabb.Add(NearTopRight);
        aabb.Add(NearBottomRight);

        aabb.Add(FarTopLeft);
        aabb.Add(FarBottomLeft);
        aabb.Add(FarTopRight);
        aabb.Add(FarBottomRight);
    }


    void Print()
    {
        // printf("NearTopLeft "); NearTopLeft.Print();
        // printf("NearBottomLeft "); NearBottomLeft.Print();
        // printf("NearTopRight "); NearTopRight.Print();
        // printf("NearBottomLeft "); NearBottomRight.Print();

        // printf("FarTopLeft "); FarTopLeft.Print();
        // printf("FarBottomLeft "); FarBottomLeft.Print();
        // printf("FarTopRight "); FarTopRight.Print();
        // printf("FarBottomLeft "); FarBottomRight.Print();
    }
};