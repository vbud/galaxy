#define STRINGIFY(A) #A

std::string kernel_source = STRINGIFY(
__kernel void galaxy(__global float4* pos, __global float4* color, __global float4* vel, __global float4* pos_gen, __global float4* vel_gen, float dt, int num, int coloring)
{
    unsigned int i = get_global_id(0);

    float4 p = pos[i];
    float4 v = vel[i];

    float ax = 0.0;
    float ay = 0.0;
    float az = 0.0;

    float space = 10.0;

    for (int j=0; j<num; j++)
    {
        float4 p2 = pos[j];
        float dx = space * (p2.x - p.x);
        float dy = space * (p2.y - p.y);
        float dz = space * (p2.z - p.z);

        float invr = rsqrt(dx*dx + dy*dy + dz*dz + 0.001);
        float invr3 = invr*invr*invr;

        float f = invr;
        ax += f*p2.x;
        ay += f*p2.y;
        az += f*p2.z;
    }

    float dt2 = dt*dt;

    p.x -= dt*v.x + .5*dt2*ax;
    p.y -= dt*v.y + .5*dt2*ay;
    p.z -= dt*v.z + .5*dt2*az;

    v.x += dt*ax;
    v.y += dt*ay;
    v.z += dt*az;
    
    pos[i] = p;
    vel[i] = v;

    if (coloring == 1)
    {
        // Color by velocity
        color[i] = normalize(v)/2+.5;
        color[i].w = 1.0;
    } else if (coloring == 2)
    {
        // Color by speed
        color[i].x = 0.5;
        color[i].yz = 1/length(v);
        color[i].w = 1.0;
    } else if (coloring == 3)
    {
        // Color by angle
        color[i] = normalize(p)/2+.5;
        color[i].w = 1.0;
    } else
    {
        // Color by initial position
        color[i] = normalize(pos_gen[i])/2+.5;
        color[i].w = 1.0;
    }
}
);

void timeRate(float r);
void changeColor();