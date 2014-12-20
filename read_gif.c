#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "read_image.h"

//GIF block special codes
#define TRAILER     0x3B
#define EXTENSION   0x21
#define APPLICATION 0xFF
#define PLAINTEXT   0x01
#define COMMENT     0xFE
#define GCONTROL    0xF9
#define DESCRIPTOR  0x2C

#define CLEARCODE   -2
#define EOI         -1
#define NOPARENT    -3

#define ERROR      0xFFFFFFFF


typedef struct {
  int parentCode;
  int index;
} TableEntry;

static RGBTriple GcolorTable[256];              //global color table
static RGBTriple ScolorTable[256];               //local color table
static RGBTriple *AcolorTable = GcolorTable;      //active color table
static RGBTriple *image=NULL;                    //the image

static TableEntry codeTable[5000];    //the code table for LZW


static int Xpos=0,Ypos=0;      //last pixels drawn

static unsigned char version[4];
static unsigned int  width;           //image width
static unsigned int  height;          //image height
static unsigned int  GcolorTableSize;
static unsigned int  sortFlag;            //are the colors sorted by importance?
static unsigned int  colorResolution;    //number of colors in original image
static unsigned int  GcolorTableFlag;    //is a global color table present?
static unsigned int  background;         //index of background color
static unsigned char aspectRatio;      //pixel aspect ratio (whatever that is)
static unsigned int  Gcolors;         
  

static unsigned int  SimageLeftPos=0;     //left side of a sub-image
static unsigned int  SimageTopPos=0;
static unsigned int  SimageWidth=0;
static unsigned int  SimageHeight=0;
static unsigned int  ScolorTableFlag=0;     //Indicates local color table follows
static unsigned int  SinterlaceFlag=0;    //S prefix means local, G means global
static unsigned int  SsortFlag=0;           
static unsigned int  ScolorTableSize=0;
static unsigned int  Scolors=0;

static int pass=1;
static int empty;
static FILE *in;

#define read8() getbits(8)

static unsigned int getbits(int number); //number must be <= # of bits in an int
static int bitsLeft=0;
static unsigned int storageInt=0;

static int Pow(int base, int exponent);
static void readHeader(void);
static void readLogicalScreenDescriptor(void);
static void readGlobalColorTable(void);
static void readImageDescriptor(void);
static void readLocalColorTable(void);
static void skipControlExtension(void);
static void readCommentExtension(void);
static void skipPlainTextExtension(void);
static void skipApplicationExtension(void);
static void codeTableInit(int bitsPerPixel);
static void readImageData(void);
static void outputCode(int code);
static int  getFirstChar(int code);
static void addToCodeList(int code, int c);
static void drawPoint(int colorIndex);
static int readImage(void);
static void ungetbits(int numberOfBits,unsigned int theBits);
static void clearVars(void);

static int numberReadIn=0;

static int Pow(int base, int exponent){
  int rv=1;
  int i;
  for(i=0;i<exponent;i++)
    rv *= base;
  return rv;
}

/*read 'number' bits from stdin.  If number != 8,16, or 24 then it remembers*/
/*the bits and saves them for the next time around.*/
static unsigned int getbits(int number){
  unsigned int rv=0;
  unsigned int c=0;
  int t;
  unsigned int temp;
  if (number>24){
    number=24;
  }

  while(bitsLeft<number){
    t = fgetc(in);
    numberReadIn++;
    if(t == EOF){
      fprintf(stderr,"unexpected EOF\n");
      return ERROR;
    }
    c = t;
    c = c << bitsLeft;
     storageInt = storageInt | c;
    bitsLeft+=8;
  }
  temp = 1;
  temp = (temp << number)-1;  //to get "number" ones
  rv = temp & storageInt;
  storageInt = storageInt >> number;
  bitsLeft -= number;
  return rv;
}


//You can't do this very often, and it may not work for more than
//12 bits!
static void ungetbits(int numberOfBits,unsigned int theBits){
  if(numberOfBits > 12){
    numberOfBits=12;
  }
  storageInt = storageInt << numberOfBits;
  storageInt = storageInt | theBits;
  bitsLeft += numberOfBits;
  if(numberOfBits>32){
    numberOfBits = 32;
  }
}

static void clearVars(void){
    image = NULL;
    SimageLeftPos=0;  
    SimageTopPos=0;
    SimageWidth=0;
    SimageHeight=0;
    ScolorTableFlag=0;
    SinterlaceFlag=0;    
    SsortFlag=0;           
    ScolorTableSize=0;
    Scolors=0;
    pass=1;
    bitsLeft=0;
    storageInt=0;
    numberReadIn=0;
}

static void readHeader(void){
  int a,b,c;
  a = read8(); b = read8(); c = read8();
  if(a!='G' || b!='I' || c!='F'){
    fprintf(stderr,"WARNING!!!!!WARNING!!!!!!WARNING!!!!!!WARNING!!!!!!WARNING!!!!\n");
    fprintf(stderr,"File does not appear to be a GIF.  We will try to continue,\n");
    fprintf(stderr,"but you have been warned!!\n");
  }

  version[0]=read8();
  version[1]=read8();
  version[2]=read8();
  version[3]='\0';

  if(version[0]!='8' || version[1]!='9' || version[2]!='a'){
    fprintf(stderr,"WARNING!!!!!WARNING!!!!!!WARNING!!!!!!WARNING!!!!!!WARNING!!!!\n");
    fprintf(stderr,"This GIF is version %s which is not supported ",version);
    fprintf(stderr,"by this program.\nThis program only supports version 89a, but ");
    fprintf(stderr,"we will try to read\nthis GIF anyway...\n");
    fprintf(stderr,"%X,%X,%X\n",version[0],version[1],version[2]);
  }
}
 

static void readLogicalScreenDescriptor(void){
  width = getbits(16);
  height= getbits(16);

  GcolorTableSize = getbits(3);
  sortFlag = getbits(1);
  colorResolution = getbits(3)+1;
  GcolorTableFlag = getbits(1);
  background = getbits(8);
  aspectRatio = getbits(8);
}

static void readGlobalColorTable(void){
  Gcolors = Pow (2, GcolorTableSize + 1);
  int i;
  if (Gcolors>256){
    fprintf(stderr,"colors are too big for color table (%d), so we ",Gcolors);
    fprintf(stderr,"will probably\n segmentation fault veeery soon...\n");
  }

  for(i=0;i<Gcolors;i++){
    GcolorTable[i].red=getbits(8);
    GcolorTable[i].green=getbits(8);
    GcolorTable[i].blue=getbits(8);
  }
}

static void readImageDescriptor(void){
  int trash;
  SimageLeftPos = getbits(16);
  SimageTopPos  = getbits(16);
  SimageWidth   = getbits(16);
  SimageHeight  = getbits(16);
  
  Xpos = SimageLeftPos;
  Ypos = SimageTopPos;


  ScolorTableSize = getbits(3);
  trash = getbits(2);
  SsortFlag     = getbits(1);
  SinterlaceFlag  = getbits(1);
  ScolorTableFlag = getbits(1);
}

static void readLocalColorTable(void){
  int i;
  Scolors = Pow(2,ScolorTableSize+1);
  if(Scolors>256){
    fprintf(stderr,"Colors are too big for local color table (%d), so ",Scolors);
    fprintf(stderr,"we will probably\n segmentation fault veeery soon...\n");
    Scolors = 256;
  }
  for(i=0;i<Scolors;i++){
    ScolorTable[i].red = getbits(8);
    ScolorTable[i].green = getbits(8);
    ScolorTable[i].blue = getbits(8);
  }
}

static void skipControlExtension(void){
  int size,i ;

    size = getbits(8);
  for(i=0;i<size;i++){
    getbits(8);
  }
  int temp = getbits(8);
  if(temp != 0){
    fprintf(stderr,"Expected to be at the end of a ");
    fprintf(stderr,"\ncontrol block, but I'm not.   Probably about to cause a \n");
    fprintf(stderr,"segmentation fault.  Goodbye in case I never see you again...\n");
  }
}


static void readCommentExtension(void){
  unsigned int c;
  int size,j;
  
  printf("Comment:\n");
  
  while(1){
    size = getbits(8);
    if(size==0)
      break;
    for(j=0;j<size;j++){
      c = getbits(8);
      putchar(c);
    }
  } 
  putchar('\n');
}


static void skipPlainTextExtension(void){
  int length,i;

  length = getbits(8);
  for(i=0;i<length;i++)
    getbits(8);

  int temp = getbits(8);
  if(temp!= 0){
    fprintf(stderr,"In Plain Text Extension...Expected zero termination of block ");
    fprintf(stderr,"but got %d.\nI may be leaving soon.\n",temp);
  }
}

static void skipApplicationExtension(void){
  int length,i;
  
  length = getbits(8);
  
  for( i=0;i<length;i++)
    getbits(8);

  int t = getbits(8);

  if (t != 0){
    fprintf(stderr,"In Application extension, expected 0 Block Terminator but got ");
    fprintf(stderr,"%d.  Will continue processing, but without much hope...\n",t);
  }
}

 
static void codeTableInit(int bitsPerPixel){
  int i;
  int size = Pow(2,bitsPerPixel)-1;
  for(i=0;i<=size;i++){
    codeTable[i].parentCode = NOPARENT;
    codeTable[i].index = i;
  }
}

static void readImageData(void){
  int blockSize;
  int minCodeSize;
  int currentCodeSize,clear,eoi,oldCode, curCode;
  int first;
  int bitsRead;
  int leftOverBits,tempBits;
  int full = 0,fromClear=0;
  minCodeSize = getbits(8);
  blockSize = getbits(8);

  bitsRead=0;
  currentCodeSize = minCodeSize+1;
  codeTableInit(minCodeSize);
  clear = Pow(2,minCodeSize);
  eoi = clear + 1;
  empty = eoi + 1;
  oldCode = -1;


  while(blockSize>0){
    while(bitsRead+currentCodeSize<=blockSize*8){
      fflush(stdout);
      curCode = getbits(currentCodeSize);
      bitsRead+=currentCodeSize;
      if(curCode == ERROR){
	return;
      }
      if(curCode == clear){
CLEAR:  full = 0; 
	codeTableInit(minCodeSize);
	currentCodeSize = minCodeSize+1;
	clear = Pow(2,minCodeSize);
	eoi = clear + 1;
	empty = eoi + 1;
	oldCode = -1;
	if(bitsRead+currentCodeSize>blockSize*8){
	  fromClear=1;
	  break;
	}
CLEAR2: fromClear = 0;
	curCode = getbits(currentCodeSize);
	bitsRead+=currentCodeSize;
	if(curCode == clear)
	  goto CLEAR;
	if(curCode == eoi)
	  goto END;
	if(curCode == ERROR) 
	  return;
	outputCode(curCode);
	oldCode = curCode;
      }
      else if(curCode == eoi){
END:
      if(blockSize*8 >bitsRead){
	getbits(blockSize*8-bitsRead);
      }
      if((curCode=getbits(8))!=0)
	fprintf(stderr,"terminator (%0X) is not zero, veery strange...\n",curCode);
      return;
      }
      else if(curCode<empty){
	outputCode(curCode);
	first = getFirstChar(curCode);
	if(!full){
	  addToCodeList(oldCode, first);
	}
	oldCode = curCode;
      }
      else{
	if(curCode != empty)
	  fprintf(stderr,"POSSIBLE ERROR!!!! empty=%d, curCode=%d\n",empty,curCode);
	first = getFirstChar(oldCode);
	if(!full){
	  addToCodeList(oldCode,first);
	}
	outputCode(empty-1);
	oldCode = curCode;
      }
      if(empty >= Pow(2,currentCodeSize) && currentCodeSize<12){
	currentCodeSize++;
      }else if (empty>=Pow(2,currentCodeSize)){
	full = 1;
      }
    }
    leftOverBits = blockSize*8-bitsRead;
    tempBits = getbits(leftOverBits);
    blockSize=getbits(8);
    if(blockSize != 0){
      ungetbits(leftOverBits,tempBits);
    }else
      fprintf(stderr,"This shouldn't be zero here.We should get the end code first\n");
    if (blockSize == ERROR)
      return;
    bitsRead=0-leftOverBits;
    if(fromClear){
      goto CLEAR2;
    }
  }
}


static void addToCodeList(int code, int c){
  codeTable[empty].parentCode = code;
  codeTable[empty].index = c;
  empty++;
}


static void outputCode(int code){
  int pixelList[5001];          //longest possible code
  int lastPixel=0;  
  int tempCode=code;
  do{
    pixelList[lastPixel]=codeTable[tempCode].index;
    lastPixel++;
    tempCode=codeTable[tempCode].parentCode;
  }while(tempCode!=NOPARENT && lastPixel<5001);
  for(lastPixel--;lastPixel>=0;lastPixel--){
    drawPoint(pixelList[lastPixel]);
  }
}


static void drawPoint(int colorIndex){
  int point,i;
  //If there is no image yet, create space for one
  if(image == NULL){
    image = (RGBTriple *)malloc (sizeof(RGBTriple)*width*height);
    if(GcolorTableFlag){
      for(i=0;i<width*height;i++){
	image[i].red = GcolorTable[background].red;
	image[i].green=GcolorTable[background].green;
	image[i].blue=GcolorTable[background].blue;
      }
    }//if there is no global color table...
    else{
      for(i=0;i<width*height;i++){
          image[i].red=0;
          image[i].green=0;
          image[i].blue=0;
      }
    }
  }
  //figure out which point to draw on and increment the pointers
  point = Ypos*width + Xpos;
  image[point].red=AcolorTable[colorIndex].red;
  image[point].green=AcolorTable[colorIndex].green;
  image[point].blue=AcolorTable[colorIndex].blue;

  Xpos++;
  if(Xpos>=SimageWidth+SimageLeftPos){
    Xpos=SimageLeftPos;
    
    if(SinterlaceFlag){         //if interlaced...
      switch(pass){
      case 1: Ypos+=8; break;
      case 2: Ypos+=8; break;
      case 3: Ypos+=4; break;
      case 4: Ypos+=2; break;
      default: fprintf(stderr,"Unexpected case (%d) in Ypos switch\n",pass);
      }
      
      if(Ypos>=height){
	pass++;
	switch(pass){
	case 1: Ypos = 0; break;
	case 2: Ypos = 4; break;
	case 3: Ypos = 2; break;
	case 4: Ypos = 1; break;
	}
      }
    }
    else{               //not interlaced
       Ypos++;
    }
  } 
}


static int getFirstChar(int code){
  int rv;
  while(codeTable[code].parentCode != NOPARENT)
    code = codeTable[code].parentCode;
  rv = codeTable[code].index;
  return rv;
}

  

static int readImage(void){
  int code;
  clearVars();
  readHeader();
  readLogicalScreenDescriptor();
  if(GcolorTableFlag)
    readGlobalColorTable();
  
  while(1){
    code = getbits(8);
    if(code == EXTENSION){
      code = getbits(8);
      if(code == GCONTROL){
	skipControlExtension();
	continue;
      }
      else if(code == APPLICATION){
	skipApplicationExtension();  //I'm lazy...
	continue;
      }
      else if(code == COMMENT){
	readCommentExtension();
	continue;
      }
      else if(code == PLAINTEXT){
	skipPlainTextExtension();
	continue;
      }
    }
    else if(code == DESCRIPTOR){
      readImageDescriptor();
      if(ScolorTableFlag){       //if there is a local color table
	readLocalColorTable();
	AcolorTable = ScolorTable; //activate local color table
      }
      readImageData();
      AcolorTable=GcolorTable;
      SinterlaceFlag=0;
      pass=1;
      SimageLeftPos=0;
      SimageTopPos=0;
      SimageHeight=0;
      SimageWidth=0;
      ScolorTableFlag=0;
      Scolors=0;
      Xpos = 0;
      Ypos = 0;
      continue;
    }
    else if (code == TRAILER){
      return 0;
    }
    else if(code == ERROR){
      fprintf(stderr,"Unexpected end of file: will display what we have.\n");
      return 1;
    }
    else
      fprintf(stderr,"Unknown code %X: what should we do?\n",code);
  }
}
    
/*-------------------------------------------------*/
/* The main function */
/*-------------------------------------------------*/
Image *image_from_gif(char *filename){
  Image *pic = (Image*)malloc(sizeof(Image));
  in = fopen(filename,"rb");

  if(readImage()){
    fprintf(stderr,"There was an error reading the image.\n");
    fclose(in);
    return NULL;
  }

  pic->height = height;
  pic->width  = width;
  pic->pixels = image;
  fclose(in);
  return pic;
}

/*--------------------------------------------------*/
/* another important function */
/*--------------------------------------------------*/
int IMG_isGIF(char *filename)
{
    /* This function was ripped from SDL_image */
    int is_GIF;
    char magic[6];
    FILE *infile = fopen(filename, "rb");
    
    is_GIF = 0;
    if ( fgets(magic,7,infile) ) {
        if ( (strncmp(magic, "GIF", 3) == 0) &&
             ((memcmp(magic + 3, "87a", 3) == 0) ||
              (memcmp(magic + 3, "89a", 3) == 0)) ) {
            is_GIF = 1;
        }
    }
    
    return(is_GIF);
}