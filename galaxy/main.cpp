/*
 * ICG: Final Project
 * Franklin Davenport
 */
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <math.h>


//OpenGL stuff
#include <GL/glew.h>
#if defined __APPLE__ || defined(MACOSX)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

//Our OpenCL Particle Systemclass
#include "cll.h"

#include "galaxy.cl" //std::string kernel_source is defined in this file

#define NUM_PARTICLES 5000
CL* example;

//GL related variables
int window_width = 800;
int window_height = 600;
int glutWindowHandle = 0;
float translate_z = -1.f;
// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;
//main app helper functions
void init_gl(int argc, char** argv);
void appRender();
void appDestroy();
void timerCB(int ms);
void appKeyboard(unsigned char key, int x, int y);
void appMouse(int button, int state, int x, int y);
void appMotion(int x, int y);


int paused = 0;
int num = NUM_PARTICLES;
std::vector<Vec4> pos(num);
std::vector<Vec4> vel(num);
std::vector<Vec4> color(num);

//----------------------------------------------------------------------
//quick random function to distribute our initial points
float rand_float(float mn, float mx)
{
    float r = random() / (float) RAND_MAX;
    return mn + (mx-mn)*r;
}


// Ranomly generates initial particle properties.
void gen_random(int num, std::vector<Vec4> &pos, std::vector<Vec4> &vel, std::vector<Vec4> &color)
{
    //fill our vectors with initial data
    for(int i = 0; i < num; i++)
    {
        float fmin = -1.0;
        float fmax =  1.0;

        pos[i] = Vec4(rand_float(fmin, fmax), rand_float(fmin, fmax), rand_float(fmin, fmax), 1.0f);

        float life_r = rand_float(0.0f, 10.0f);
        fmin = -500.0*sin(pos[i].x*100);
        fmax =  500.0*cos(pos[i].x*100);
        vel[i] = Vec4(rand_float(fmin, fmax), rand_float(fmin, fmax), rand_float(fmin, fmax), 1.0f);

        color[i] = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}


// generates a vortex
void gen_vortex(int num, std::vector<Vec4> &pos, std::vector<Vec4> &vel, std::vector<Vec4> &color)
{
    //fill our vectors with initial data
    for(int i = 0; i < num; i++)
    {
        float fmin = -1.0;
        float fmax =  1.0;

        pos[i] = Vec4(rand_float(fmin, fmax), rand_float(fmin, fmax), rand_float(fmin, fmax), 1.0f);

        float life_r = rand_float(0.0f, 10.0f);
        vel[i] = Vec4(0.0,0.0,10.0, life_r);

        color[i] = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}

// generates a ring
void gen_ring(int num, float z_size, std::vector<Vec4> &pos, std::vector<Vec4> &vel, std::vector<Vec4> &color)
{
    //fill our vectors with initial data
    for(int i = 0; i < num; i++)
    {
        //distribute the particles in a random circle around z axis
        float rad = rand_float(.05, .4);
        float ang = 2*3.14 * i/num;
        float x = rad*sin(ang);
        float z = rand_float(-z_size, z_size);// -.1 + .2f * i/num;
        float y = rad*cos(ang);
        pos[i] = Vec4(x, y, z, 1.0f);

        float s = 100.0*rad;

        float life_r = rand_float(0.f, 1.f);
        //life_r = 3.0*life_r*life_r;
        vel[i] = Vec4(-s*cos(ang), s*sin(ang), 0.0f, life_r);
        //vel[i] = Vec4(0.0,0.0,0.0,0.0);

        //just make them red and full alpha
        color[i] = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}

void generate_particles(int i)
{
    //initialize our particle system with positions, velocities and color
    //gen_random(num, pos, vel, color);
    switch (i)
    {
        case 1: gen_random(num, pos, vel, color); break;
        case 2: gen_ring(num, 0.1, pos, vel, color); break;
        case 3: gen_ring(num, 5.0, pos, vel, color); break;
    }

    example->loadData(pos, vel, color);
}

//----------------------------------------------------------------------
int main(int argc, char** argv)
{
    printf("Hello, OpenCL\n");
    //Setup our GLUT window and OpenGL related things
    //glut callback functions are setup here too
    init_gl(argc, argv);

    //initialize our CL object, this sets up the context
    example = new CL();

    //load and build our CL program from the file
    example->loadProgram(kernel_source);

    generate_particles(2);
   
    //initialize the kernel
    example->popCorn();

    //this starts the GLUT program, from here on out everything we want
    //to do needs to be done in glut callback functions
    glutMainLoop();

}

//----------------------------------------------------------------------
void appRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //this updates the particle system by calling the kernel
    if (!paused)
        example->runKernel();

    //render the particles from VBOs
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(5.);
    
    //printf("color buffer\n");
    glBindBuffer(GL_ARRAY_BUFFER, example->c_vbo);
    glColorPointer(4, GL_FLOAT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, example->p_vbo);
    glVertexPointer(4, GL_FLOAT, 0, 0);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glDrawArrays(GL_POINTS, 0, example->num);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glutSwapBuffers();
}


//----------------------------------------------------------------------
void init_gl(int argc, char** argv)
{

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition (glutGet(GLUT_SCREEN_WIDTH)/2 - window_width/2, 
                            glutGet(GLUT_SCREEN_HEIGHT)/2 - window_height/2);

    
    std::stringstream ss;
    ss << "Adventures in OpenCL: Part 2, " << NUM_PARTICLES << " particles" << std::ends;
    glutWindowHandle = glutCreateWindow(ss.str().c_str());

    glutDisplayFunc(appRender); //main rendering function
    glutTimerFunc(30, timerCB, 30); //determin a minimum time between frames
    glutKeyboardFunc(appKeyboard);
    glutMouseFunc(appMouse);
    glutMotionFunc(appMotion);

    glewInit();

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, window_width, window_height);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 1000.0);

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, translate_z);

}


//----------------------------------------------------------------------
void appDestroy()
{
    //this makes sure we properly cleanup our OpenCL context
    delete example;
    if(glutWindowHandle)glutDestroyWindow(glutWindowHandle);
    printf("about to exit!\n");

    exit(0);
}


//----------------------------------------------------------------------
void timerCB(int ms)
{
    //this makes sure the appRender function is called every ms miliseconds
    glutTimerFunc(ms, timerCB, ms);
    glutPostRedisplay();
}


//----------------------------------------------------------------------
void appKeyboard(unsigned char key, int x, int y)
{
    //this way we can exit the program cleanly
    switch(key)
    {
        case '\033': // escape quits
        case '\015': // Enter quits    
        case 'Q':    // Q quits
        case 'q':    // q (or escape) quits
            // Cleanup up and quit
            appDestroy();
            break;
        case '1': generate_particles(1); break;
        case '2': generate_particles(2); break;
        case '3': generate_particles(3); break;
        case 'a': timeRate(0.0001); break;
        case 'z': timeRate(-0.0001); break;
        case 's': timeRate(0.001); break;
        case 'x': timeRate(-0.001); break;
        case 'c': changeColor(); break;
        default :
            if (paused)
                paused = 0;
            else
                paused = 1;
    }
}


//----------------------------------------------------------------------
void appMouse(int button, int state, int x, int y)
{
    //handle mouse interaction for rotating/zooming the view
    if (state == GLUT_DOWN) {
        mouse_buttons |= 1<<button;
    } else if (state == GLUT_UP) {
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;
}


//----------------------------------------------------------------------
void appMotion(int x, int y)
{
    //hanlde the mouse motion for zooming and rotating the view
    float dx, dy;
    dx = x - mouse_old_x;
    dy = y - mouse_old_y;

    if (mouse_buttons & 1) {
        rotate_x += dy * 0.2;
        rotate_y += dx * 0.2;
    } else if (mouse_buttons & 4) {
        translate_z += dy * 0.1;
    }

    mouse_old_x = x;
    mouse_old_y = y;

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, translate_z);
    glRotatef(rotate_x, 1.0, 0.0, 0.0);
    glRotatef(rotate_y, 0.0, 1.0, 0.0);
}



