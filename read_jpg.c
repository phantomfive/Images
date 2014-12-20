#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "jpeglib.h"
#include "read_image.h"

/* most of this code is modified from the libjpg example file */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  /* Always display the message. */
  (*cinfo->err->output_message) (cinfo);
  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


/*-----------------------------------------------*/
/* read in the jpeg */
/*-----------------------------------------------*/
Image *image_from_jpg(char *filename)
{
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  int row_stride;		/* physical row width in output buffer */
  unsigned char *data;
  JSAMPROW row_pointer[1];
  FILE *infile;
  Image *pic = (Image*)malloc(sizeof(Image));
  
  
  infile = fopen(filename,"rb");
  if(!infile) return NULL;
  
  /* Step 1: allocate and initialize JPEG decompression object */
  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */
  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */
  (void) jpeg_read_header(&cinfo, TRUE);

  /* Step 4: set parameters for decompression */
  cinfo.out_color_space = JCS_RGB;
  cinfo.quantize_colors = FALSE;
  
  /* Step 5: Start decompressor */
  (void) jpeg_start_decompress(&cinfo);

  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  data = (unsigned char*)malloc(sizeof(RGBTriple)*cinfo.output_width * cinfo.output_height);
  pic->height = cinfo.output_height;
  pic->width  = cinfo.output_width;
  
  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
      row_pointer[0]=&data[cinfo.output_scanline*cinfo.output_width*3];
      jpeg_read_scanlines(&cinfo,row_pointer, 1);
  }
      
  /* Step 7: Finish decompression */
  (void) jpeg_finish_decompress(&cinfo);

  /* Step 8: Release JPEG decompression object */
  jpeg_destroy_decompress(&cinfo);

  /* And we're done! */
  pic->pixels = (RGBTriple*)data;

  fclose (infile);
  return pic;
}

/*-------------------------------------------------*/
/* Test to see if it is really a jpeg */
/*-------------------------------------------------*/
int IMG_isJPG(char *filename)
{
    /* This code was ripped from SDL_Image */
    int is_JPG;
    FILE *infile;
    unsigned char magic[4];
    infile = fopen(filename, "rb");
    if(!infile)return 0;

    is_JPG = 0;
    if ( fgets(magic,3,infile) ) {
        if ( (magic[0] == 0xFF) && (magic[1] == 0xD8) ) {
            fgets(magic,5,infile);
            fgets(magic,5,infile);
            if ( memcmp((char *)magic, "JFIF", 4) == 0 ||
                 memcmp((char *)magic, "Exif", 4) == 0 ) {
                is_JPG = 1;
            }
        }
    }
    fclose(infile);
    return(is_JPG);
}
