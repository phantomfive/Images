#include <stdio.h>
#include <stdlib.h>
#include <GLUT/glut.h>
#include "read_image.h"

void drawCube(void);
void RenderScene(void);

void drawCube(void){
    glBegin(GL_QUADS);
    /* top */
    glTexCoord2f(0,0);
    glVertex3f( 1, 1, 1);
    glTexCoord2f(0,1);    
    glVertex3f( 1, 1,-1);
    glTexCoord2f(1,1);
    glVertex3f( 1,-1,-1);
    glTexCoord2f(1,0);
    glVertex3f(-1, 1, 1);
    
    /* front */
    glVertex3f( 1,-1, 1);
    glVertex3f( 1, 1, 1);
    glVertex3f(-1, 1, 1);
    glVertex3f(-1,-1, 1);
    
    /* bottom */
    glVertex3f( 1,-1, 1);
    glVertex3f(-1,-1, 1);
    glVertex3f(-1,-1,-1);
    glVertex3f( 1,-1,-1);
    
    /* back */
    glVertex3f( 1,-1,-1);
    glVertex3f(-1,-1,-1);
    glVertex3f(-1, 1,-1);
    glVertex3f( 1, 1,-1);
    
    /* left */
    glVertex3f(-1, 1, 1);
    glVertex3f(-1, 1,-1);
    glVertex3f(-1,-1,-1);
    glVertex3f(-1,-1, 1);
    
    /* right */
    glVertex3f( 1, 1,-1);
    glVertex3f( 1, 1, 1);
    glVertex3f( 1,-1, 1);
    glVertex3f( 1,-1,-1);
    glEnd();
}

void RenderScene(void){
    
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0,.2,1);
    glScalef(10,10,10);
    //glRotatef(45,1,1,1);
    drawCube();
    glLoadIdentity();
        
    glutSwapBuffers();

}

void SetupRC(void)
{
    char *fileName = "/Users/andrew/Desktop/char2.gif";
    GLsizei width;
    GLsizei height;
    GLenum  format;
    GLenum  type;
    int internalformat;
    void   *pixels;
    
    glClearColor(0,0,0,0);
   // glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    glFrontFace(GL_CW);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    image_load_for_openGL(fileName,&internalformat, &width, &height,
                          &format, &type, &pixels);
    
    glTexImage2D(GL_TEXTURE_2D,0,internalformat,width,height,0,format,type,pixels);
    free(pixels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
}

void ChangeSize(GLsizei w, GLsizei h)
{
    GLfloat aspectRatio;
    
    if(h==0) h = 1;
    
    glViewport(0,0,w,h);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    aspectRatio = (GLfloat)w / (GLfloat)h;
    if(w <= h)
        glOrtho(-100,100, -100/aspectRatio, 100.0/ aspectRatio,
                25.0, -25.0);
    else
        glOrtho (-100.0*aspectRatio, 100.0 * aspectRatio,
                 -100.0, 100.0, 25.0, -25.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main (int argc, const char * argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE |GLUT_RGB);
    glutCreateWindow("GLCube");
    glutDisplayFunc(RenderScene);
    glutReshapeFunc(ChangeSize);
    
    SetupRC();
    
    glutMainLoop();
    return 0;
}
