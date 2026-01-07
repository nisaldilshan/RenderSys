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
};

struct Triangle {
    glm::vec3 pt[3];
    bool culled = false;
};

void ComputeNearAndFar(float& fNearPlane, float& fFarPlane, 
                       const glm::vec3& vLightCameraOrthographicMin, 
                       const glm::vec3& vLightCameraOrthographicMax, 
                       const std::vector<glm::vec3>& pvPointsInCameraView) 
{
    // Initialize the near and far planes
    fNearPlane = FLT_MAX;
    fFarPlane = -FLT_MAX;

    // AABB Triangle Indices (standard cube triangulation)
    static const int iAABBTriIndexes[] = {
        0, 1, 2,  1, 2, 3,
        4, 5, 6,  5, 6, 7,
        0, 2, 4,  2, 4, 6,
        1, 3, 5,  3, 5, 7,
        0, 1, 4,  1, 4, 5,
        2, 3, 6,  3, 6, 7
    };

    // We need a buffer to hold split triangles. 
    // Starting with 1 (from the AABB face) and growing as we clip.
    Triangle triangleList[16];
    int iTriangleCnt = 0;

    for (int AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter) {
        
        // Reset count for this specific AABB face
        iTriangleCnt = 1;
        triangleList[0].pt[0] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 0]];
        triangleList[0].pt[1] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 1]];
        triangleList[0].pt[2] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 2]];
        triangleList[0].culled = false;

        // Clip each triangle against the 4 frustum planes (MinX, MaxX, MinY, MaxY)
        for (int frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter) {
            
            float fEdge = 0.0f;
            int iComponent = 0;

            if (frustumPlaneIter == 0) { fEdge = vLightCameraOrthographicMin.x; iComponent = 0; }
            else if (frustumPlaneIter == 1) { fEdge = vLightCameraOrthographicMax.x; iComponent = 0; }
            else if (frustumPlaneIter == 2) { fEdge = vLightCameraOrthographicMin.y; iComponent = 1; }
            else { fEdge = vLightCameraOrthographicMax.y; iComponent = 1; }

            for (int triIter = 0; triIter < iTriangleCnt; ++triIter) {
                if (!triangleList[triIter].culled) {
                    
                    int iInsideVertCount = 0;
                    bool iPointPassesCollision[3] = { false };

                    // Test points against plane
                    for (int triPtIter = 0; triPtIter < 3; ++triPtIter) {
                        float val = (iComponent == 0) ? triangleList[triIter].pt[triPtIter].x : triangleList[triIter].pt[triPtIter].y;
                        
                        // Check Min planes (0, 2) > edge, Max planes (1, 3) < edge
                        if (frustumPlaneIter == 0 || frustumPlaneIter == 2) {
                            if (val > fEdge) iPointPassesCollision[triPtIter] = true;
                        } else {
                            if (val < fEdge) iPointPassesCollision[triPtIter] = true;
                        }
                        
                        if (iPointPassesCollision[triPtIter]) iInsideVertCount++;
                    }

                    // Move inside points to the beginning of the array to standardise logic
                    if (iPointPassesCollision[1] && !iPointPassesCollision[0]) {
                        std::swap(triangleList[triIter].pt[0], triangleList[triIter].pt[1]);
                        iPointPassesCollision[0] = true; iPointPassesCollision[1] = false;
                    }
                    if (iPointPassesCollision[2] && !iPointPassesCollision[1]) {
                        std::swap(triangleList[triIter].pt[1], triangleList[triIter].pt[2]);
                        iPointPassesCollision[1] = true; iPointPassesCollision[2] = false;
                    }
                    if (iPointPassesCollision[1] && !iPointPassesCollision[0]) {
                        std::swap(triangleList[triIter].pt[0], triangleList[triIter].pt[1]);
                        iPointPassesCollision[0] = true; iPointPassesCollision[1] = false;
                    }

                    // Clipping Logic
                    if (iInsideVertCount == 0) {
                        triangleList[triIter].culled = true; // All out
                    } else if (iInsideVertCount == 1) {
                        // 1 in, 2 out -> Clip to 1 smaller triangle
                        triangleList[triIter].culled = false;
                        
                        glm::vec3 vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
                        glm::vec3 vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

                        float hitRatio = fEdge - ((iComponent == 0) ? triangleList[triIter].pt[0].x : triangleList[triIter].pt[0].y);
                        float comp1 = (iComponent == 0) ? vVert0ToVert1.x : vVert0ToVert1.y;
                        float comp2 = (iComponent == 0) ? vVert0ToVert2.x : vVert0ToVert2.y;

                        triangleList[triIter].pt[1] = triangleList[triIter].pt[0] + vVert0ToVert1 * (hitRatio / comp1);
                        triangleList[triIter].pt[2] = triangleList[triIter].pt[0] + vVert0ToVert2 * (hitRatio / comp2);
                    } else if (iInsideVertCount == 2) {
                        // 2 in, 1 out -> Quad -> Split into 2 triangles
                        
                        // Copy next triangle out of the way
                        triangleList[iTriangleCnt] = triangleList[triIter + 1];
                        triangleList[triIter].culled = false;
                        triangleList[triIter + 1].culled = false;

                        glm::vec3 vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
                        glm::vec3 vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

                        float hitRatio = fEdge - ((iComponent == 0) ? triangleList[triIter].pt[2].x : triangleList[triIter].pt[2].y);
                        float comp0 = (iComponent == 0) ? vVert2ToVert0.x : vVert2ToVert0.y;
                        float comp1 = (iComponent == 0) ? vVert2ToVert1.x : vVert2ToVert1.y;

                        glm::vec3 vert0 = triangleList[triIter].pt[2] + vVert2ToVert0 * (hitRatio / comp0);
                        glm::vec3 vert1 = triangleList[triIter].pt[2] + vVert2ToVert1 * (hitRatio / comp1);

                        // New Triangle 1
                        triangleList[triIter + 1].pt[0] = triangleList[triIter].pt[0];
                        triangleList[triIter + 1].pt[1] = triangleList[triIter].pt[1];
                        triangleList[triIter + 1].pt[2] = vert0;

                        // Modified Triangle 0
                        triangleList[triIter].pt[0] = triangleList[triIter + 1].pt[1];
                        triangleList[triIter].pt[1] = triangleList[triIter + 1].pt[2];
                        triangleList[triIter].pt[2] = vert1;

                        iTriangleCnt++;
                        triIter++; // Skip the newly inserted one
                    }
                }
            }
        }

        // Update Near/Far with the Z values of the surviving triangles
        for (int index = 0; index < iTriangleCnt; ++index) {
            if (!triangleList[index].culled) {
                for (int vertind = 0; vertind < 3; ++vertind) {
                    float fTriangleCoordZ = triangleList[index].pt[vertind].z;
                    if (fNearPlane > fTriangleCoordZ) fNearPlane = fTriangleCoordZ;
                    if (fFarPlane < fTriangleCoordZ) fFarPlane = fTriangleCoordZ;
                }
            }
        }
    }
}