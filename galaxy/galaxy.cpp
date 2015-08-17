#include <stdio.h>

#include "cll.h"
#include "util.h"

#include <vector>

float dt = .001f;
int coloring = 1;

void CL::loadData(std::vector<Vec4> pos, std::vector<Vec4> vel, std::vector<Vec4> col)
{
    static int do_init = 1;

    num = pos.size();
    array_size = num * sizeof(Vec4);

    if (do_init)
    {   do_init = 0;
        // This is the first time we are loading particle data.
        // VBOs and OpenCL arrays must be initiialized

        // Create Position and Color VBOs for GL & CL
        p_vbo = createVBO(&pos[0], array_size, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
        c_vbo = createVBO(&col[0], array_size, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
        glFinish();

        // Create OpenCL buffer from GL VBO
        cl_vbos.push_back(cl::BufferGL(context, CL_MEM_READ_WRITE, p_vbo, &err));
        cl_vbos.push_back(cl::BufferGL(context, CL_MEM_READ_WRITE, c_vbo, &err));

        //Create the OpenCL only arrays
        cl_velocities = cl::Buffer(context, CL_MEM_WRITE_ONLY, array_size, NULL, &err);
        cl_pos_gen = cl::Buffer(context, CL_MEM_WRITE_ONLY, array_size, NULL, &err);
        cl_vel_gen = cl::Buffer(context, CL_MEM_WRITE_ONLY, array_size, NULL, &err);
    } else {
        // Reloading the position and color VBOs with new values.
        reload_buffer(p_vbo, &pos[0], array_size, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
        reload_buffer(c_vbo, &col[0], array_size, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    }

    // Push data to the GPU
    printf("Pushing data to the GPU\n");
    err = queue.enqueueWriteBuffer(cl_velocities, CL_TRUE, 0, array_size, &vel[0], NULL, &event);
    err = queue.enqueueWriteBuffer(cl_pos_gen, CL_TRUE, 0, array_size, &pos[0], NULL, &event);
    err = queue.enqueueWriteBuffer(cl_vel_gen, CL_TRUE, 0, array_size, &vel[0], NULL, &event);
    queue.finish();
}

void CL::popCorn()
{
    // Initialize our kernel from the program
    try{
        kernel = cl::Kernel(program, "galaxy", &err);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
    }

    //set the arguements of our kernel
    try
    {
        err = kernel.setArg(0, cl_vbos[0]);   //position vbo
        err = kernel.setArg(1, cl_vbos[1]);   //color vbo
        err = kernel.setArg(2, cl_velocities);//velocity
        err = kernel.setArg(3, cl_pos_gen);   //initial positions
        err = kernel.setArg(4, cl_vel_gen);   //initial velocities
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
    }
    queue.finish();
}

void CL::runKernel()
{
    glFinish();
    // map OpenGL buffer object for writing from OpenCL
    // this passes in the vector of VBO buffer objects (position and color)
    err = queue.enqueueAcquireGLObjects(&cl_vbos, NULL, &event);
    queue.finish();

    kernel.setArg(5, dt);  //pass in the timestep
    kernel.setArg(6, num); //pass in the number of particles
    kernel.setArg(7, coloring); //pass in the number of particles

    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event); 
    queue.finish();

    //Release the VBOs so OpenGL can play with them
    err = queue.enqueueReleaseGLObjects(&cl_vbos, NULL, &event);
    queue.finish();
}

void changeColor()
{
    coloring++;
    if (coloring > 4) coloring = 1;
}

void timeRate(float r)
{
    dt += r;
}