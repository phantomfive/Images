#ifndef ACT_READ_IMAGE
#define ACT_READ_IMAGE 1

#include "editMe.h"

typedef struct{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
}RGBTriple;
typedef struct{
    RGBTriple *pixels;
    int height;
    int width;
}Image;
enum Stretch{ STRETCH_UP, STRETCH_DOWN, STRETCH_NEAREST };

/* This function loads any supported image format */
Image *image_load(char *filename);

/* frees img->pixels, and frees img, if they are not null */
void image_free(Image *img);

#ifdef WITHOPENGL
/* loads an image in a format to be used as an openGL texture. This can
   be used with glDrawPixels, or glTexImage2D. Returns 0 on failure, or
   1 on success.  The parameters will not be changed on failure. Don't
   forget to free() *pixels! */
int image_load_for_openGL(char*filename,GLsizei *width, GLsizei *height,
                          GLenum *format, GLenum *type, void **pixels);
#endif


/* stretches img to newWidth and newHeight. img->pixels is replaced
and freed. Returns img or NULL on error.  If newWidth or newHeight is 0,
the other dimension will be scaled proportionately */
Image *image_stretch_to_size(Image *img, int newWidth,int newHeight);

/* stretches the dimensions of the image to a power of 2, suitable for
an openGL texture. img is the Image to be stretched.  The pixels element
of the Image struct will be replaced and freed. transformType is
STRETCH_UP      - round up to a power of 2,
STRETCH_DOWN    - round down to a power of 2
STRETCH_NEAREST - round  to the nearest power of 2
maxSize is the maximum size for the width or height.  This can be useful
for shrinking an image. A maxSize value of 0 will disable this feature.
Returns img or NULL on error. */
Image *image_stretch_to_power_of_2(Image *img, int transformType, int maxSize);

/* flips the image over the x axis.  Returns img or crashes on error.
   (There should never be an error)*/
Image *image_flip_x(Image *img);


/* These functions to load particular image formats */
Image *image_from_gif(char *filename);
Image *image_from_jpg(char *filename);

/* These functions check to see what type the image is*/
int IMG_isGIF(char *filename);
int IMG_isJPG(char *filename);

#endif
