// File: key_gui.cpp
// Written by Joshua Green

#include <iostream>
#include <map>
#include <string>
#include <cmath>
#include <GL/glu.h>
#include <GL/glut.h>
using namespace std;

extern int _argc;
extern char** _argv;

float WIDTH = 600.0f;
float HEIGHT = 300.0f;

extern map<float, string> pie;

struct point2d {
  double x, y;

  point2d(double _x, double _y) {x = _x; y = _y;}
};
struct point3d {
  double x, y, z;

  point3d(double _x, double _y, double _z) {x = _x; y = _y; z = _z;}
};

void draw_circle(float r, const point2d pos, const point3d& color, bool filled=true) {
  float to_deg = 3.14159f/180.0f;
  glPushMatrix();
  glColor3f(color.x, color.y, color.z);
  if (filled) glBegin(GL_TRIANGLE_FAN);
  else glBegin(GL_LINE_LOOP);
  for (int theta=0;theta<360;theta+=5) {
    glVertex2f(pos.x + cos(theta*to_deg) * r, pos.y + sin(theta*to_deg) * r);
  }
  glEnd();
  glPopMatrix();
}

void draw_pie_piece(float percent, float r, point2d pos, point3d color, float offset) {
  float to_deg = 3.14159f/180.0f;

  glPushMatrix();
  glTranslatef(pos.x, pos.y, 0.0f);
  glRotatef(360*offset, 0.0f, 0.0f, 1.0f);
  glColor3f(color.x, color.y, color.z);
  glBegin(GL_POLYGON);
  glVertex2f(0, 0);
  glVertex2f(r, 0); // first boundary line
  for (int theta=0;theta<360*percent;theta+=5) {
    glVertex2f(cos(theta*to_deg)*r, sin(theta*to_deg)*r);
  }
  glVertex2f(r*cos(2*3.14159*percent), r*sin(2*3.14159*percent)); // second boundary line

  glEnd();
  glPopMatrix();
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT);         // reset background to black

  //draw_circle(100.0f, point2d(WIDTH/2.0, HEIGHT/2.0), point3d(0.3, 0.3, 0.3f));

  float offset = 0.0f;
  point2d pos = point2d(120.0f, HEIGHT/2.0);
  map<float, string>::iterator pie_it = pie.begin();
  while (pie_it != pie.end()) {
    draw_pie_piece(pie_it->first, 100.0f, pos, point3d(1.0*offset, 1.0*offset, 1.0f*offset), offset);
    offset += pie_it->first;
    pie_it++;
  }

  glFlush(); //Send the 2D scene to the screen
}

void init_opengl() {
  glClearColor(0.0, 0.0, 0.0, 1.0);    // black background

  // Set projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, WIDTH, 0.0, HEIGHT);  // Project into 2D.

  // Set modelview matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void glbranch(void*) {
  glutInit(&_argc, _argv);               // Initialize glut from argv[]
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); // Single buffer, rgb color.
  glutInitWindowSize(WIDTH, HEIGHT);         // 500 x 500 pixel window.
  glutCreateWindow("OpenGL");          // window title
  glutDisplayFunc(display);            // Set display callback function.
  init_opengl();                       // Set attributes.          

  glutMainLoop();                      // Enter event loop.
}
