#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <jpeglib.h>
#include <jerror.h>
#include <png.h>
#include <webp/types.h>
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>
#include <vpx/vpx_decoder.h>

#define WEIGHTED_AVG 1
#define UNWEIGHTED_AVG 0


//This global structure holds all the working data about the image.
//The decoder will set these values. filters may alter them. most functions use it
struct imageMetaData_t {
  int width, height, colorspace, rawImgSize, destImgSize;
  unsigned char *pixelData, *encodedImg;
};
struct imageMetaData_t __attribute__((aligned(32))) imageMetaData;

#include "compression.c"
#include "filters.c"
#include "scaling.c"
#include "dither.c"
#include "seam_carving.c"
#include "codecs.c"
#include "rij.c"


//timer for benchmarking
static unsigned long long rdtsc(void) {
  unsigned int ax, dx;
  __asm__ __volatile__ ("rdtsc" : "=a"(ax), "=d"(dx));
  return ((((unsigned long long)dx) << 32) | ax);
}


int main(int argc, const char* argv[]) {
  if(argc < 4) {
    printf("\nUsage: %s <image source file> <ouput filename> <filter name> [opt param]\n", argv[0]);
    printf("\nwidth and height only required for seam carving/expanding filter\n");
    printf("set width or height to zero to keep original dimension\n");
    printf("\nAvailable Filters:\n");
    printf("\tsc [width] [height]\tseam carving. also used for seam expansion, if height/width parameter is larger than img\n");
    printf("\tds\tdraw seam\n");
    printf("\tgs[w]\tgreyscale. standard averaging (R+B+G)/3 or [weighted] averaging R/4 + B/2 + G/4\n");
    printf("\ttp\ttranspose image. X,Y -> Y,X (90deg rotation mirrored over Y axis)\n");
    printf("\tsb\tgreyscale sobel edge detection\n");
    printf("\tsbm\tmultichannel sobel edge detection. detects color boundaries of equil luminosity, but then outputs greyscale\n");
    printf("\tsbc\tcolor sobel edge detection. edges from each color channel combined into color img output\n");
    printf("\t8bit\t8-bit color quantization (3-3-2). output in sRGB(A)\n");
    printf("\t8pal\t8-bit color palettized (256). output in sRGB(A)\n");
    printf("\thb[c/m] [blockSize]\thorizontal blocking [color/monochrome]\n");
    printf("\thbr[m] [blockSize]\thorizontal blocking random. [color/monochrome]\n");
    printf("\tdfs\terror diffusion dithering. floyd-steinberg. (first and most common method)\n");
    printf("\tdb[8/16]\tordered dithering. Bayer matrix 8x8 or 16x16. (8-bit / hash style)\n");
    printf("\tdh[8/16]\tordered dithering. Halftone matrix 8x8 or 16x16. (screen printing / comicbook dots style)\n");
    printf("\tscale [n]\tscaled by 1/n in both dimensions. (n = 2, 2a, 3, 4). (a = anti-aliased). for n=3, h&w must be divisible by 3. for now. /todo\n");
    return 1;
  }

  //name input parameters
  const char* srcFile = argv[1];
  const char* destFile = argv[2];
  const char* filter = argv[3];

  //src file perms and size
  if(access(srcFile, F_OK|R_OK|W_OK) != 0) return 2;
  int fd = open(srcFile, O_RDONLY);
  if(fd == -1) return 4;
  struct stat sb;
  fstat(fd,&sb); //file size
  if(sb.st_size < 14) return 5;

  //read src img file
  int rc = 0;
  unsigned char* srcImg = malloc(sb.st_size);
  while(rc < sb.st_size)
    rc += read(fd, srcImg + rc, sb.st_size - rc);
  close(fd);

  /* decode img */

  if((srcImg[0] == 0xFF && srcImg[1] == 0xD8 && srcImg[2] == 0xFF) ||
     (srcImg[6] == 'J'  && srcImg[7] == 'F'  && srcImg[8] == 'I' ) ||
     (srcImg[6] == 'E'  && srcImg[7] == 'x'  && srcImg[8] == 'i' )) { //JPEG
    jpeg_decode(srcImg, sb.st_size);
  }
  else if(srcImg[1] == 'P' && srcImg[2] == 'N' && srcImg[3] == 'G') { //PNG
    int err = png_decode(srcImg, sb.st_size);
    if(err) {printf("png decompress failed\n"); return err;}
  }
  else if(srcImg[8] == 'W' && srcImg[9] == 'E' && srcImg[10] == 'B') { //WEBP
    int err = webp_decode(srcImg, sb.st_size);
    if(err) {printf("webp decompress failed\n"); return err;}
  }
  else if(srcImg[0] == 'P' && srcImg[1] == '6') { //PPM
    int err = ppm_decode(srcImg, sb.st_size);
    if(err) {printf("ppm decompress failed\n"); return err;}
  }
  else if(srcImg[0] == 'I' && srcImg[1] == 'I') { //TIFF
    int err = tiff_decode(srcImg, sb.st_size);
    if(err == 1) printf("compression type not supported\n");
    if(err) {printf("tiff decompress failed\n"); return err;}
  }
  else if(srcImg[0] == 'B' && srcImg[1] == 'M') { //BMP
    int err = bmp_decode(srcImg, sb.st_size);
    if(err == 1) {printf("bitmap compression not supported\n"); return err;}
    if(err == 2) {printf("bitmap colorspace only supports 24/32bpp\n"); return err;}
  }
  else if(srcImg[0] == 'V' && srcImg[1] == 'S') { //RIJ
    int err = rij_decode(srcImg, sb.st_size);
    if(err) {printf("rij decompress failed\n"); return err;}
  }
  else if(srcImg[0] == 'V' && srcImg[1] == 'Z') { //RIZ
    int err = riz_decode(srcImg, sb.st_size);
    if(err) {printf("riz decompress failed\n"); return err;}
  }
  else return 7;

  free(srcImg);

unsigned long long bench = rdtsc(); //start timer. times filters, not encoding/decoding. optional benchmarking

  /* run filters */

  if(strncmp(filter,"sc\0",3) == 0) { //seam carving or expansion
    if(argc < 5) return 18;
    int resizeX = atoi(argv[4]);
    int resizeY = (argc > 5) ? atoi(argv[5]) : 0;
    unsigned char* gradientImg = 0;
    //carving
    if(imageMetaData.width > resizeX && resizeX > 7) gradientImg = seam_carving(gradientImg, resizeX);
    if(imageMetaData.height > resizeY && resizeY > 7) {
      gradientImg = transpose_monochrome(gradientImg);
      transpose_image();
      gradientImg = seam_carving(gradientImg, resizeY);
      transpose_image();
      free(gradientImg);
    }
    //expanding
    if(imageMetaData.width < resizeX && resizeX < 100000) seam_expansion(resizeX);
    if(imageMetaData.height < resizeY && resizeY < 100000) {
      transpose_image();
      seam_expansion(resizeY);
      transpose_image();
    }
  }

  if(strncmp(filter,"ds",2) == 0) { //draw seam
    unsigned char* gradientImg = NULL;
    gradientImg = draw_seam(gradientImg);
    gradientImg = transpose_monochrome(gradientImg);
    transpose_image();
    gradientImg = draw_seam(gradientImg);
    free(gradientImg);
    transpose_image();
  }

  if(strncmp(filter,"gs",2) == 0) { //greyscale
    int weighted = (filter[2] == 'w') ? WEIGHTED_AVG : UNWEIGHTED_AVG;
    unsigned char* x = monochrome_to_rgb(rgb_to_monochrome(imageMetaData.pixelData, weighted));
    imageMetaData.colorspace = 3;
    free(imageMetaData.pixelData);
    imageMetaData.pixelData = x;
  }

  if(strncmp(filter,"sb\0",3) == 0) { //sobel
    unsigned char* mono = sobel();
    free(imageMetaData.pixelData);
    imageMetaData.pixelData = monochrome_to_rgb(mono);
  }

  if(strncmp(filter,"sbm",3) == 0) { //sobel multichannel
    unsigned char* mono = sobel_multiChannel(0);
    free(imageMetaData.pixelData);
    imageMetaData.pixelData = monochrome_to_rgb(mono);
  }

  if(strncmp(filter,"sbc",3) == 0) //sobel color
    sobel_multiChannel(1);

  if(strncmp(filter,"tp",2) == 0) //transpose
    transpose_image();

  if(filter[0] == '8') { //8-bit
    if(filter[1] == 'b') f_8bit();
    if(filter[1] == 'p') f_8bit_palette();
  }

  if(strncmp(filter,"hbc",3) == 0) //horizontal blocking color
    horizontal_blocking_color(atoi(argv[4]));

  if(strncmp(filter,"hbm",3) == 0) //horizontal blocking mono
    horizontal_blocking_mono(atoi(argv[4]), 0);

  if(strncmp(filter,"hbr",3) == 0) { //horizontal blocking random
    int color = 1;
    if(filter[3] == 'm') color = 0; //monochrome
    horizontal_blocking_random(atoi(argv[4]), color);
  }

  if(strncmp(filter,"dfs",3) == 0) //dither floyd steinberg
    BW_dither_floyd_steinberg();

  if(strncmp(filter,"db",2) == 0) { //dither bayer
    int matrixSize = (filter[2] == '1') ? 16 : 8;
    BW_dither_bayer(matrixSize);
  }

  if(strncmp(filter,"dh",2) == 0) { //dither halftone
    int matrixSize = (filter[2] == '1') ? 16 : 8;
    BW_dither_halftone(matrixSize);
  }

  if(strncmp(filter,"sca",3) == 0) { //scale
    int scaleFactor = argv[4][0] - 48;
    int antiAliasing = (argv[4][1] == 'a');
    if(scaleFactor == 2) scale_by_half(antiAliasing);
    if(scaleFactor == 3) scale_by_third();
    if(scaleFactor == 4) scale_by_quarter();
  }

bench = rdtsc() - bench; //end benchmark timer
printf("%lli\n",bench>>18);

  /* encode */

  char *ext = strrchr(destFile, '.');
  int err = 0;

  if(strncmp(ext,".jpg",4) == 0 || strncmp(ext,".jpeg",5) == 0) err = jpeg_encode();
  if(strncmp(ext,".png",4) == 0) err = png_encode();
  if(strncmp(ext,".webp",5) == 0) err = webp_encode();
  if(strncmp(ext,".ppm",4) == 0) err = ppm_encode();
  if(strncmp(ext,".tif",4) == 0) err = tiff_encode();
  if(strncmp(ext,".bmp",4) == 0) err = bmp_encode();
  if(strncmp(ext,".rij",4) == 0) err = rij_encode();
  if(strncmp(ext,".riz",4) == 0) err = riz_encode();

  if(err) {
    printf("%s encoding err: %i\n", &ext[1], err);
  }
  else {
    /* write file */
    fd = open(destFile, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    write(fd, imageMetaData.encodedImg, imageMetaData.destImgSize);
    close(fd);
  }
  free(imageMetaData.pixelData);
  free(imageMetaData.encodedImg);
  return err;
}
