#include "FluidSolver2D.h"
#include <cassert>

#define IX(x, y) ((x) + (y) * N)

constexpr uint32_t linearSolveIterations = 4;

// Check of Nan propagation
// if(std::isnan(x[IX(0, 0)]))
//     x[IX(0, 0)] = 0.000001f;

static void set_bnd(int boundaryCondition, std::vector<float>& x, int N)
{
    //set edges
    for(int i = 1; i < N - 1; i++) {
        x[IX(0  , i)] = boundaryCondition == 1 ? -x[IX(1  , i)] : x[IX(1  , i)];
        x[IX(N-1, i)] = boundaryCondition == 1 ? -x[IX(N-2, i)] : x[IX(N-2, i)];
        x[IX(i, 0  )] = boundaryCondition == 2 ? -x[IX(i, 1  )] : x[IX(i, 1  )];
        x[IX(i, N-1)] = boundaryCondition == 2 ? -x[IX(i, N-2)] : x[IX(i, N-2)];
    }
    
    // set corners
    x[IX(0, 0)]         = 0.5f * (x[IX(1, 0)]
                                + x[IX(0, 1)]);
    x[IX(0, N-1)]       = 0.5f * (x[IX(1, N-1)]
                                + x[IX(0, N-2)]);
    x[IX(N-1, 0)]       = 0.5f * (x[IX(N-2, 0)]
                                + x[IX(N-1, 1)]);
    x[IX(N-1, N-1)]     = 0.5f * (x[IX(N-2, N-1)]
                                + x[IX(N-1, N-2)]);                        
}

static void lin_solve_gauss_seidel(int boundaryCondition, std::vector<float>& x, std::vector<float>& x0, float a, float c, int N)
{
    float cRecip = 1.0 / c;
    for (int k = 0; k < linearSolveIterations; k++) {
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                x[IX(i, j)] =
                    (x0[IX(i, j)]
                        + a*(    x[IX(i+1, j  )]
                                +x[IX(i-1, j  )]
                                +x[IX(i  , j+1)]
                                +x[IX(i  , j-1)]
                        )) * cRecip;
            }
        }
        set_bnd(boundaryCondition, x, N);
    }
}

static void lin_solve_jacobi_iteration(int boundaryCondition, std::vector<float>& x, std::vector<float>& x0, float a, float c, int N)
{
    float cRecip = 1.0 / c;
    std::vector<float> x_new(x.size(), 0.0f); // Create a new vector to store updated values
    
    for (int k = 0; k < linearSolveIterations; k++) {
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                x_new[IX(i, j)] =
                    (x0[IX(i, j)]
                        + a*(    x[IX(i+1, j  )]
                                + x[IX(i-1, j  )]
                                + x[IX(i  , j+1)]
                                + x[IX(i  , j-1)]
                        )) * cRecip;
            }
        }
        
        // Update the x vector after each iteration
        x = x_new;
        
        set_bnd(boundaryCondition, x, N);
    }
}

FluidSolver2D::FluidSolver2D(FluidPlane& fluid)
    : m_fluid(fluid)
{}

void FluidSolver2D::FluidSolveStep()
{
    float dt       = 0.01f;
    
    Diffuse(1, m_fluid.Vx0, m_fluid.Vx, m_fluid.viscosity, dt);
    Diffuse(2, m_fluid.Vy0, m_fluid.Vy, m_fluid.viscosity, dt);
    
    Project(m_fluid.Vx0, m_fluid.Vy0, m_fluid.Vx, m_fluid.Vy);
    
    Advect(1, m_fluid.Vx, m_fluid.Vx0, m_fluid.Vx0, m_fluid.Vy0, dt);
    Advect(2, m_fluid.Vy, m_fluid.Vy0, m_fluid.Vx0, m_fluid.Vy0, dt);
    
    Project(m_fluid.Vx, m_fluid.Vy, m_fluid.Vx0, m_fluid.Vy0);
    
    Diffuse(0, m_fluid.density0, m_fluid.density, m_fluid.diffusion, dt);
    Advect(0, m_fluid.density, m_fluid.density0, m_fluid.Vx, m_fluid.Vy, dt);
}

void FluidSolver2D::FluidPlaneAddDensity(int x, int y, float amount)
{
    const int N = m_fluid.size;
    m_fluid.density[IX(x, y)] += amount;
}

void FluidSolver2D::FluidPlaneAddVelocity(int x, int y, float amountX, float amountY)
{
    const int N = m_fluid.size;
    int index = IX(x, y);
    
    m_fluid.Vx[index] += amountX;
    m_fluid.Vy[index] += amountY;
}

void FluidSolver2D::Advect(int b, std::vector<float> &d, std::vector<float> &d0, std::vector<float> &velocX, std::vector<float> &velocY, float dt)
{
    const int N = m_fluid.size;    
    const float dt0 = dt * N;
    const float Nfloat = N;    

    for(int i = 1; i < N - 1; i++) {
        for(int j = 1; j < N - 1; j++) { 
            float x = float(i) - (dt0 * velocX[IX(i, j)]); 
            float y = float(j) - (dt0 * velocY[IX(i, j)]);
            
            if(x < 0.5f) 
                x = 0.5f; 
            if(x > Nfloat + 0.5f) 
                x = Nfloat + 0.5f; 
            if(y < 0.5f) 
                y = 0.5f; 
            if(y > Nfloat + 0.5f) 
                y = Nfloat + 0.5f; 
            
            float i0 = floorf(x); 
            float i1 = i0 + 1.0f;  
            float j0 = floorf(y);
            float j1 = j0 + 1.0f; 
            
            float s1 = x - i0; 
            float s0 = 1.0f - s1; 
            float t1 = y - j0; 
            float t0 = 1.0f - t1;
            
            int i0i = i0;
            int i1i = i1;
            int j0i = j0;
            int j1i = j1;
            
            d[IX(i, j)] =   s0 * ( t0 * d0[IX(i0i, j0i)]  +  t1 * d0[IX(i0i, j1i)])
                          + s1 * ( t0 * d0[IX(i1i, j0i)]  +  t1 * d0[IX(i1i, j1i)]);
        }
    }
    set_bnd(b, d, N);
}

void FluidSolver2D::Diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt)
{
    const int N = m_fluid.size;
    float a = dt * diff * (N - 2) * (N - 2);
    lin_solve_gauss_seidel(b, x, x0, a, 1 + 4 * a, N);
}

void FluidSolver2D::Project(std::vector<float>& velocX, std::vector<float>& velocY, std::vector<float>& p, std::vector<float>& div)
{
    const int N = m_fluid.size;
    float h = 1.0/N;
    for (int i = 1; i < N - 1; i++) {
        for (int j = 1; j < N - 1; j++) {
            div[IX(i, j)] = -0.5f * h *(
                     velocX[IX(i+1, j  )]
                    -velocX[IX(i-1, j  )]
                    +velocY[IX(i  , j+1)]
                    -velocY[IX(i  , j-1)]
                );
            p[IX(i, j)] = 0;
        }
    }
    set_bnd(0, div, N); 
    set_bnd(0, p, N);
    lin_solve_gauss_seidel(0, p, div, 1, 4, N);
    
    for (int i = 1; i < N - 1; i++) {
        for (int j = 1; j < N - 1; j++) {
            velocX[IX(i, j)] -= 0.5f * (  p[IX(i+1, j)]
                                         -p[IX(i-1, j)]) * N;
            velocY[IX(i, j)] -= 0.5f * (  p[IX(i, j+1)]
                                         -p[IX(i, j-1)]) * N;
        }
    }
    set_bnd(1, velocX, N);
    set_bnd(2, velocY, N);
}
