#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "read_image.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX(a,b) ((a)>(b))?a:b
#define MIN(a,b) ((a)<(b))?a:b
/* find distance between two points */
#define distance(x1,y1,x2,y2) sqrt(((x1)-(x2))*((x1)-(x2)) + \
                                  ((y1)-(y2))*((y1)-(y2)))
                                   
/* A list of all the file formats accepted,
   and the functions to call for that type
   of image.*/
static struct{
    char *type;
    int(*is)(char *detectFile);
    Image *(*load)(char *infile);
} supported[] = {
    { "GIF", IMG_isGIF, image_from_gif  },
    { "JPG", IMG_isJPG, image_from_jpg  }
};

Image *image_load(char *filename){
    FILE *infile;
    int i;
    
    /* make sure filename is valid */
    if(filename==NULL)
        return NULL;
    if((infile=fopen(filename,"rb"))==NULL){
        perror("Couldn't open file");
        return NULL;
    }
    fclose(infile);
    
    /* determine image type */
    for( i=0; i < ARRAYSIZE(supported);i++){
        if(supported[i].is(filename))
            return supported[i].load(filename);
    }
    
    fprintf(stderr,"Unsupported image format\n");
    return NULL;
}


#ifdef WITHOPENGL
int image_load_for_openGL(char*filename, int *internalformat, GLsizei *width, 
                          GLsizei *height, GLenum *format, GLenum *type, void **pixels){
    Image *img;
    img = image_load(filename);
    if(img==NULL)
        return 0;
    
    if(image_stretch_to_power_of_2(img,STRETCH_NEAREST,0)==NULL)
        return 0;
        
    image_flip_x(img);
    
    *width = img->width;
    *height = img->height;
    *pixels = (void*)img->pixels;
    
    *internalformat = GL_RGB;
    *format = GL_RGB;
    *type   = GL_UNSIGNED_BYTE;
    
    return 1;
}
#endif


void image_free(Image *img){
    if(img!=NULL){
        if(img->pixels!=NULL){
            free(img->pixels);
        }
        free(img);
    }
}
/*----------------------------------------------------------------------*/
/* image processing functions */
/*----------------------------------------------------------------------*/

Image *image_flip_x(Image *img){
    int x,y;
    int row1,row2;
    RGBTriple t;
    
    /* row1 starts from the front and goes to the back,
       row2 starts from the back and goes to the front. */
    row1 = 0;
    row2 = ((img->height)-1)*img->width;
    for(y=0;y<img->height/2;y++){
        /* swap the rows */
        for(x=0;x<img->width;x++){
            t = img->pixels[row1];
            img->pixels[row1] = img->pixels[row2];
            img->pixels[row2] = t;
            row1++, row2++;
        }
        /* adjust row2 to the next row up, row1 is
           adjusted automatically. */
        row2 = ((img->height-y)-2)*img->width;
    }
    return img;
}

Image *image_stretch_to_size(Image *img, int newWidth,int newHeight){
    int oldWidth,oldHeight;
    double xScaleFactor,yScaleFactor;
    RGBTriple *newPixels;
    int x,y,t,x1,y1;
    if(img==NULL) return NULL;
    if((newWidth<=0) && (newHeight<=0)) return NULL;
    
    /* calculate new dimension if one is 0 */
    if(newWidth <= 0)
        newWidth = newHeight*img->width/img->height;
    else if(newHeight <= 0)
        newHeight = newWidth*img->height/img->width;
    
    /* calculate scale factor: this scale factor
        is inverted for efficiency */
    oldWidth = img->width;
    oldHeight = img->height;
    xScaleFactor = (double)oldWidth/(double)newWidth;
    yScaleFactor = (double)oldHeight/(double)newHeight;
    
    /* allocate memory for new image */
    newPixels = (RGBTriple*)malloc(sizeof(RGBTriple)*newWidth*newHeight);
    if(newPixels == NULL)
        return NULL;
    
    /* copy and scale the image. */
    /* to find the pixel in the old image that will fill the
       pixel in the new image, use the equation:
            oldPixelLocation = 1/scaleFactor * newPixelLocation
            y1: old Y location * pixelsPerRowInOldImage
            x1: old X location
    */
    for(y=0;y<newHeight;y++){
        for(x=0;x<newWidth;x++){
            y1 = ((int)(yScaleFactor*(double)y))*oldWidth;
            x1= (((double)x)*xScaleFactor);
            t = x1+y1;
            newPixels[(y*newWidth)+x] = img->pixels[t];
        }
    }
    
    /* replace old data in img */
    free(img->pixels);
    img->pixels = newPixels;
    img->width  = newWidth;
    img->height = newHeight;
    return img;
}

int powers_of_two[] = {2,4,8,16,32,64,128,256,512,1024,
                      2048,4096,8192,16384,32768,65536};
Image *image_stretch_to_power_of_2(Image *img, int transformType, 
                                   int maxSize){
    int x,y;
    int toFindX,toFindY;
    if(img == NULL) return NULL;
        
    /* take care of maxSize */
    if((0<maxSize) && (maxSize<MAX(img->width,img->height))){
        /* clamp the largest dimension to maxSize,
           then scale the other dimension */
        if(img->height > img->width){
            toFindY = maxSize;
            toFindX = img->width * maxSize/img->height;
        }
        else{
            toFindX = maxSize;
            toFindY = img->height * maxSize/img->width;
        }
    }
    else{
        toFindX = img->width;
        toFindY = img->height;
    }

    /* First find the correct values for STRETCH_UP,
    later adjust them for STRETCH_DOWN and STRETCH_NEAREST */
    for(x = 1;x<ARRAYSIZE(powers_of_two)-1;x++){
        if(toFindX < powers_of_two[x])
            break;
    }
    for(y = 1;y<ARRAYSIZE(powers_of_two)-1;y++){
        if(toFindY < powers_of_two[y])
            break;
    }
    /* Make sure (again) it is not too big */
    if(maxSize>0){
        if(powers_of_two[x]>maxSize)
            x--;
        if(powers_of_two[y]>maxSize)
            y--;
    }
    
    /* stretch it */
    if(transformType == STRETCH_UP)
        return image_stretch_to_size(img, powers_of_two[x], powers_of_two[y]);
    
    if(transformType == STRETCH_DOWN){
        return image_stretch_to_size(img,powers_of_two[x-1], powers_of_two[y-1]);
    }
    
    if(transformType == STRETCH_NEAREST){
        int both_up, both_down, x_up_y_down, x_down_y_up;
        
        /* of all possible stretches, find the nearest */
        both_up = distance(img->width,img->height, powers_of_two[x],powers_of_two[y]);
        both_down = distance(img->width,img->height, powers_of_two[x-1],powers_of_two[y-1]);
        x_up_y_down = distance(img->width,img->height,powers_of_two[x],powers_of_two[y-1]);
        x_down_y_up = distance(img->width,img->height,powers_of_two[x-1],powers_of_two[y]);

        /* some sort of cheap sort */
        if(both_up<both_down && both_up<x_up_y_down && both_up<x_down_y_up)
            return image_stretch_to_size(img,powers_of_two[x],powers_of_two[y]);
        
        else if(both_down<x_up_y_down && both_down<x_down_y_up)
            return image_stretch_to_size(img,powers_of_two[x-1],powers_of_two[y-1]);
        
        else if(x_up_y_down<x_down_y_up)
            return image_stretch_to_size(img,powers_of_two[x],powers_of_two[y-1]);
        
        else
            return image_stretch_to_size(img,powers_of_two[x-1],powers_of_two[y]);
    }
    
    /* shouldn't get here */
    fprintf(stderr,"invalid transformType to image_stretch_to_power_of_2\n");
    exit(-1);
}