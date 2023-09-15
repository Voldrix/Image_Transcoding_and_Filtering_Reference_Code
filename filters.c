unsigned char* rgb_to_monochrome(unsigned char *rgbImg, int weighted) { //RGB(A) -> Mono
  unsigned char *monoImg = malloc(imageMetaData.width * imageMetaData.height);
  int m = 0, n = 0;
  register r = imageMetaData.width * imageMetaData.height;

  //weighted averaging better matches human perception. Human eyes perceive green more strongly than red, and red more strongly than blue
  //the standard weights are about .3R + .6G + .1B
  //the weights here are adusted to make them faster to compute. The difference is minimal
  if(imageMetaData.colorspace == 3) { //RGB
    if(weighted) //weighted averaging
      while(r--)
        monoImg[m++] = (rgbImg[n++] >> 2) + (rgbImg[n++] >> 1) + (rgbImg[n++] >> 2);
    else //simple average method
      while(r--)
        monoImg[m++] = (rgbImg[n++] + rgbImg[n++] + rgbImg[n++]) / 3;
  }
  else { //RGBA
    if(weighted) //weighted averaging
      while(r--)
        monoImg[m++] = ((rgbImg[n++] >> 2) + (rgbImg[n++] >> 1) + (rgbImg[n++] >> 2)) * (rgbImg[n++] / 255);
    else //simple average method
      while(r--)
        monoImg[m++] = ((rgbImg[n++] + rgbImg[n++] + rgbImg[n++]) / 3) * (rgbImg[n++] / 255);
  }
  return monoImg;
}


unsigned char* monochrome_to_rgb(unsigned char *monoImg) { //Mono -> RGB
  unsigned char *rgbImg = malloc(imageMetaData.width * imageMetaData.height * 3);
  int m = 0, n = 0;
  register r = imageMetaData.width * imageMetaData.height;
  while(r--)
      rgbImg[n++] = rgbImg[n++] = rgbImg[n++] = monoImg[m++];
  return rgbImg;
}


/* monochrome Sobel convolution (x & y) */
unsigned char* sobel() {
  unsigned char (*p)[imageMetaData.width * imageMetaData.colorspace] = imageMetaData.pixelData; //Multidimensional array pointer, pointing to a single dimension array
  unsigned char (*gradient)[imageMetaData.width] = malloc(imageMetaData.height * imageMetaData.width); //Final filtered image, single channel
  int x, y, z, w = imageMetaData.width, h = imageMetaData.height;

  //greyscale copy of src img
  unsigned char (*grayscale)[imageMetaData.width] = rgb_to_monochrome(imageMetaData.pixelData, UNWEIGHTED_AVG);

  //start sobel convolution (x & y)

  //corners
  x = (p[0][1] - p[0][0] + p[1][1] - p[1][0]) >> 2;
  y = (p[1][0] - p[0][0] + p[1][1] - p[0][1]) >> 2;
  gradient[0][0] = sqrt(x*x + y*y); //TL

  x = (p[0][w-1] - p[0][w-2] + p[1][w-1] - p[1][w-2]) >> 2;
  y = (p[1][w-1] - p[0][w-1] + p[1][w-2] - p[0][w-2]) >> 2;
  gradient[0][w-1] = sqrt(x*x + y*y); //TR

  x = (p[h-2][1] - p[h-2][0] + p[h-1][1] - p[h-1][0]) >> 2;
  y = (p[h-1][0] - p[h-2][0] + p[h-1][1] - p[h-2][1]) >> 2;
  gradient[h-1][0] = sqrt(x*x + y*y); //BL

  x = (p[h-2][w-1] - p[h-2][w-2] + p[h-1][w-1] - p[h-1][w-2]) >> 2;
  y = (p[h-1][w-1] - p[h-2][w-1] + p[h-1][w-2] - p[h-2][w-2]) >> 2;
  gradient[h-1][w-1] = sqrt(x*x + y*y); //BR

  for(int j = 1; j < imageMetaData.width - 1; j++) {
    //top row
    x = grayscale[1][j+1] - grayscale[1][j-1] + 2 * grayscale[0][j+1] - 2 * grayscale[0][j-1];
    y = grayscale[1][j-1] + grayscale[1][j+1] - grayscale[0][j-1] - grayscale[0][j+1] + 2 * grayscale[1][j] - 2 * grayscale[0][j];
    x /= 6;
    y >>= 3;
    z = sqrt(x*x + y*y);
    gradient[0][j] = (z > 255) ? 255 : z;
    //bottom row
    x = grayscale[imageMetaData.height-2][j+1] - grayscale[imageMetaData.height-2][j-1] + 2 * grayscale[imageMetaData.height-1][j+1] - 2 * grayscale[imageMetaData.height-1][j-1];
    y = grayscale[imageMetaData.height-1][j-1] + grayscale[imageMetaData.height-1][j+1] - grayscale[imageMetaData.height-2][j-1] - grayscale[imageMetaData.height-2][j+1] + 2 * grayscale[imageMetaData.height-1][j] - 2 * grayscale[imageMetaData.height-2][j];
    x /= 6;
    y >>= 3;
    z = sqrt(x*x + y*y);
    gradient[imageMetaData.height-1][j] = (z > 255) ? 255 : z;
  }

  for(int i = 1; i < imageMetaData.height - 1; i++) {
    //left edge
    x = grayscale[i-1][1] + grayscale[i+1][1] - grayscale[i-1][0] - grayscale[i+1][0] + 2 * grayscale[i][1] - 2 * grayscale[i][0];
    y = grayscale[i+1][1] - grayscale[i-1][1] + 2 * grayscale[i+1][0] - 2 * grayscale[i-1][0];
    x >>= 3;
    y /= 6;
    z = sqrt(x*x + y*y);
    gradient[i][0] = (z > 255) ? 255 : z;
    //right edge
    x = grayscale[i-1][imageMetaData.width-1] + grayscale[i+1][imageMetaData.width-1] - grayscale[i-1][imageMetaData.width-2] - grayscale[i+1][imageMetaData.width-2] + 2 * grayscale[i][imageMetaData.width-1] - 2 * grayscale[i][imageMetaData.width-2];
    y = grayscale[i+1][imageMetaData.width-2] - grayscale[i-1][imageMetaData.width-2] + 2 * grayscale[i+1][imageMetaData.width-1] - 2 * grayscale[i-1][imageMetaData.width-1];
    x >>= 3;
    y /= 6;
    z = sqrt(x*x + y*y);
    gradient[i][imageMetaData.width-1] = (z > 255) ? 255 : z;
    //rest of the row
    for(int j = 1; j < imageMetaData.width - 1; j++) {
      x = grayscale[i-1][j+1] + grayscale[i+1][j+1] - grayscale[i-1][j-1] - grayscale[i+1][j-1] + 2 * grayscale[i][j+1] - 2 * grayscale[i][j-1];
      y = grayscale[i+1][j-1] + grayscale[i+1][j+1] - grayscale[i-1][j-1] - grayscale[i-1][j+1] + 2 * grayscale[i+1][j] - 2 * grayscale[i-1][j];
      x >>= 3;
      y >>= 3;
      z = sqrt(x*x + y*y);
      gradient[i][j] = (z > 255) ? 255 : z;
    }
  }

  free(grayscale);
  return gradient;
}


/* this can detect color edges of equal luminosity 
   currently only supports RGB
*/
unsigned char* sobel_multiChannel(int color_output) {
  unsigned char (*p)[imageMetaData.width * imageMetaData.colorspace] = imageMetaData.pixelData;
  unsigned char (*gradient)[imageMetaData.width * imageMetaData.colorspace] = malloc(imageMetaData.height * imageMetaData.width * imageMetaData.colorspace);
  int w = imageMetaData.width, h = imageMetaData.height, c = imageMetaData.colorspace;
  int z, x, y;

  //apply Sobel convolution (x & y) to every color channel

  //corners
  for(int j = 0; j < c; j++) {
    x = (p[0][j+c] - p[0][j] + p[1][j+c] - p[1][j]) >> 2;
    y = (p[1][j] - p[0][j] + p[1][j+c] - p[0][j+c]) >> 2;
    gradient[0][j] = sqrt(x*x + y*y); //TL

    x = (p[0][w-j-1] - p[0][w-2*j-1] + p[1][w-j-1] - p[1][w-2*j-1]) >> 2;
    y = (p[1][w-j-1] - p[0][w-j-1] + p[1][w-2*j-1] - p[0][w-2*j-1]) >> 2;
    gradient[0][w-j-1] = sqrt(x*x + y*y); //TR

    x = (p[h-2][j+c] - p[h-2][j] + p[h-1][j+c] - p[h-1][j]) >> 2;
    y = (p[h-1][j] - p[h-2][j] + p[h-1][j+c] - p[h-2][j+c]) >> 2;
    gradient[h-1][j] = sqrt(x*x + y*y); //BL

    x = (p[h-2][w-j-1] - p[h-2][w-2*j-1] + p[h-1][w-j-1] - p[h-1][w-2*j-1]) >> 2;
    y = (p[h-1][w-j-1] - p[h-2][w-j-1] + p[h-1][w-2*j-1] - p[h-2][w-2*j-1]) >> 2;
    gradient[h-1][w-j-1] = sqrt(x*x + y*y); //BR
  }

  for(int j = c; j < imageMetaData.width * c - c; j++) {
    //top row
    x = p[1][j+c] - p[1][j-c] + 2 * p[0][j+c] - 2 * p[0][j-c];
    y = p[1][j-c] + p[1][j+c] - p[0][j-c] - p[0][j+c] + 2 * p[1][j] - 2 * p[0][j];
    x /= 6;
    y >>= 3;
    z = sqrt(x*x + y*y);
    gradient[0][j] = (z > 255) ? 255 : z;
    //bottom row
    x = p[h-2][j+c] - p[h-2][j-c] + 2 * p[h-1][j+c] - 2 * p[h-1][j-c];
    y = p[h-1][j-c] + p[h-1][j+c] - p[h-2][j-c] - p[h-2][j+c] + 2 * p[h-1][j] - 2 * p[h-2][j];
    x /= 6;
    y >>= 3;
    z = sqrt(x*x + y*y);
    gradient[h-1][j] = (z > 255) ? 255 : z;
  }

  for(int i = 1; i < imageMetaData.height - 1; i++) {
    //left edge
    for(int j = 0; j < imageMetaData.colorspace; j++) {
      x = p[i-1][j+c] + p[i+1][j+c] - p[i-1][j] - p[i+1][j] + 2 * p[i][j+c] - 2 * p[i][j];
      y = p[i+1][j+c] - p[i-1][j+c] + 2 * p[i+1][j] - 2 * p[i-1][j];
      x >>= 3;
      y /= 6;
      z = sqrt(x*x + y*y);
      gradient[i][j] = (z > 255) ? 255 : z;
    }
    //rest of the row
    for(int j = imageMetaData.colorspace; j < imageMetaData.width * imageMetaData.colorspace - imageMetaData.colorspace; j++) {
      x = p[i-1][j+c] + p[i+1][j+c] - p[i-1][j-c] - p[i+1][j-c] + 2 * p[i][j+c] - 2 * p[i][j-c];
      y = p[i+1][j-c] + p[i+1][j+c] - p[i-1][j-c] - p[i-1][j+c] + 2 * p[i+1][j] - 2 * p[i-1][j];
      x >>= 3;
      y >>= 3;
      z = sqrt(x*x + y*y);
      gradient[i][j] = (z > 255) ? 255 : z;
    }
    //right edge
    for(int j = 1; j <= imageMetaData.colorspace; j++) {
      x = p[i-1][w-c] + p[i+1][w-c] - p[i-1][w-2*c] - p[i+1][w-2*c] + 2 * p[i][w-c] - 2 * p[i][w-2*c];
      y = p[i+1][w-2*c] - p[i-1][w-2*c] + 2 * p[i+1][w-c] - 2 * p[i-1][w-c];
      x >>= 3;
      y /= 6;
      z = sqrt(x*x + y*y);
      gradient[i][w-c] = (z > 255) ? 255 : z;
    }
  }

  /* return filtered img */

  if(color_output == 0) { //greyscale the gradient, edges between different colors of equal luminosity are preserved
    unsigned char* monoGradient = rgb_to_monochrome(gradient, UNWEIGHTED_AVG);
    free(gradient);
    return monoGradient;
  }
  else { //return colored edges
    free(imageMetaData.pixelData);
    imageMetaData.pixelData = gradient;
    return gradient;
  }
}


//X,Y -> Y,X Mono
unsigned char* transpose_monochrome(unsigned char *img) {
  if(!img) return 0;
  int w = imageMetaData.width, h = imageMetaData.height;
  unsigned char *transposed = malloc(w * h);

  for(int r = 0; r < h; r++) {
    for(int c = 0; c < w; c++) {
      transposed[c * h + r] = img[r * w + c];
    }
  }
  free(img);
  return transposed;
}


//X,Y -> Y,X RGB(A)
void transpose_image() {
  int w = imageMetaData.width, h = imageMetaData.height;
  unsigned char *transposed = malloc(w * h * imageMetaData.colorspace);

  if(imageMetaData.colorspace == 3) { //RGB
    register x,y;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        x = 3*w * r + 3*c;
        y = 3*h * c + 3*r;
        transposed[y] = imageMetaData.pixelData[x];
        transposed[y + 1] = imageMetaData.pixelData[x + 1];
        transposed[y + 2] = imageMetaData.pixelData[x + 2];
      }
    }
  }

  if(imageMetaData.colorspace == 4) { //RGBA
    int *p = imageMetaData.pixelData;
    int *t = transposed;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        t[c * h + r] = p[r * w + c];
      }
    }
  }
  imageMetaData.width = h;
  imageMetaData.height = w;
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = transposed;
}


//Experimental filter I made to try to match the effect I got from a broken Burrows Wheeler function while I was writing it
void horizontal_blocking_color(int blockSize) {
  int imgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  unsigned char pix, *blockyImg = malloc(imgSize);
  int mid = (imageMetaData.colorspace * blockSize) >> 1;

/*  for(int i = 0; i < imgSize; i += (imageMetaData.colorspace*blockSize)) {
    for(int c = 0; c < imageMetaData.colorspace; c++) {
      pix = imageMetaData.pixelData[i + c + mid];
      for(int l = 0; l < blockSize - 1; l++)
        blockyImg[i + c + (imageMetaData.colorspace*l)] = pix;
      blockyImg[i + c + (imageMetaData.colorspace*(blockSize-1))] = imageMetaData.pixelData[i + c + (imageMetaData.colorspace*(blockSize-1))];
    }
  }
*/
  for(int h = 1; h < imageMetaData.height; h += 2) {
    for(int i = 0; i < imageMetaData.width * imageMetaData.colorspace; i += (imageMetaData.colorspace*blockSize)) {
      for(int c = 0; c < imageMetaData.colorspace; c++) {
        pix = imageMetaData.pixelData[h*imageMetaData.width *imageMetaData.colorspace + i + c + mid];
        for(int l = 0; l < blockSize - 1; l++)
          blockyImg[h*imageMetaData.width*imageMetaData.colorspace + i + c + (imageMetaData.colorspace*l)] = pix;
        blockyImg[h*imageMetaData.width*imageMetaData.colorspace + i + c + (imageMetaData.colorspace*(blockSize-1))] = imageMetaData.pixelData[h*imageMetaData.width*imageMetaData.colorspace + i + c + (imageMetaData.colorspace*(blockSize-1))];
      }
    }
    memcpy(&blockyImg[(h-1)*imageMetaData.width * imageMetaData.colorspace], &blockyImg[h*imageMetaData.width * imageMetaData.colorspace], imageMetaData.width * imageMetaData.colorspace);

  }
  //if(imgSize % (imageMetaData.colorspace*blockSize)) memset(&blockyImg[imgSize - (imgSize % blockSize)], &imageMetaData.pixelData[imgSize - (imgSize % blockSize)], imgSize % blockSize);

  free(imageMetaData.pixelData);
  imageMetaData.pixelData = blockyImg;
}


void horizontal_blocking_mono(int blockSize, int minMidMax) {
  //minMidMax = 0 min, 1 mid, 2 max
  int imgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  unsigned char *mono = rgb_to_monochrome(imageMetaData.pixelData, UNWEIGHTED_AVG);
  int avg;

  for(int h = 1; h < imageMetaData.height; h += 2) {
    int hw = h * imageMetaData.width;
    for(int w = 0; w < imageMetaData.width; w += blockSize) {
      if(minMidMax == 0) { //min
        avg = 255;
        for(int k = 0; k < blockSize; k++) //find min
          avg = (avg > mono[hw + w + k]) ? mono[hw + w + k] : avg;
      }
      if(minMidMax == 2) { //max
        avg = 0;
        for(int k = 0; k < blockSize; k++) //find max
          avg = (avg < mono[hw + w + k]) ? mono[hw + w + k] : avg;
      }
      if(minMidMax == 1) { //mid/avg
        avg = 0;
        for(int k = 0; k < blockSize; k++) //calc avg
          avg += mono[hw + w + k];
        avg /= blockSize;
      }

      for(int k = 0; k < blockSize; k++) //set block to val
        mono[hw + w + k] = avg;
      
    }
    memcpy(&mono[(h-1)*imageMetaData.width], &mono[hw], imageMetaData.width);
  }

  free(imageMetaData.pixelData);
  imageMetaData.pixelData = monochrome_to_rgb(mono);
  imageMetaData.colorspace = 3;
}


//broken burrows–wheeler transform
void horizontal_blocking_random(int length, int monochrome) {
  unsigned char (*suffix)[length] = calloc(length + 1, length);
  const int imgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;

  for(int block = 0; block < imgSize; block += length) {
    memcpy(&suffix[0][0], &imageMetaData.pixelData[block], length);
    for(int i = 1; i < length; i++) {
      memcpy(&suffix[i][1], &suffix[i-1][0], length - 1);
      suffix[i][0] = suffix[i-1][length - 1];
    }

    int z = 1;
    while(z) {
      z = 0;
      for(int i = 0; i < length - 1; i++) {
        if(cmpStr(&suffix[i], &suffix[i+1], length) > 0) {
          memcpy(&suffix[length], &suffix[i], length);
          memcpy(&suffix[i], &suffix[i+1], length);
          memcpy(&suffix[i+1], &suffix[length], length);
          z = 1;
        }
      }
    }

    for(int i = 0; i < length; i++)
      imageMetaData.pixelData[block + i] = suffix[0][length - 1];
  }
  free(suffix);

  if(monochrome) {
    unsigned char *mono = rgb_to_monochrome(imageMetaData.pixelData, UNWEIGHTED_AVG);
    unsigned char *rgb = monochrome_to_rgb(mono);
    free(mono);
    free(imageMetaData.pixelData);
    imageMetaData.pixelData = rgb;
    imageMetaData.colorspace = 3;
  }
}


void f_8bit() { //quantization (3-3-2)
  const int imgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;

  for(int i = 0; i < imgSize; i += imageMetaData.colorspace) {
    //could be rounded instead of truncated
    imageMetaData.pixelData[i] &= 224;
    imageMetaData.pixelData[i+1] &= 224;
    imageMetaData.pixelData[i+2] &= 192;
  }
}


void f_8bit_palette() {
  const int imgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  int pixel, palette[256][3], maxTally, idx, colorDeviation, closestColor, brightestChannel;
  int *tally = calloc(4, 16777216);

  for(int i = 0; i < imgSize; i += imageMetaData.colorspace) { //pixels -> ints
    pixel = imageMetaData.pixelData[i] << 16;
    pixel |= imageMetaData.pixelData[i+1] << 8;
    pixel |= imageMetaData.pixelData[i+2];
    tally[pixel]++;
  }

  for(int i = 0; i < 256; i++) { //generate palette. find 256 most common colors
    maxTally = 0;
    for(int j = 0; j < 16777216; j++) {
      idx = (maxTally < tally[j]) ? j : idx;
      maxTally = (maxTally < tally[j]) ? tally[j] : maxTally;
      //todo ignore close colors already in palette
    }

    palette[i][0] = idx >> 16;
    palette[i][1] = (idx >> 8) & 255;
    palette[i][2] = idx & 255;
    tally[idx] = 0;

    //Ignore colors nearly identical to ones already in the pallet, to increase color variance
    closestColor = 0;
    for(int j = 0; j < i; j++) { //find closest color in palette
      //if(1 == abs(palette[i][0] - palette[j][0]) + abs(palette[i][1] - palette[j][1]) + abs(palette[i][2] - palette[j][2])) {
      //  i--;
      //  break;
      //}
      closestColor = (1 == abs(palette[i][0] - palette[j][0]) + abs(palette[i][1] - palette[j][1]) + abs(palette[i][2] - palette[j][2])) ? 1 : closestColor;
    }
    i -= closestColor;
  }

  for(int i = 0; i < imgSize; i += imageMetaData.colorspace) { //set src pixels to closest palette color
    closestColor = 16777216;
    for(int j = 0; j < 256; j++) { //find closest color in palette
      colorDeviation = abs(imageMetaData.pixelData[i] - palette[j][0]) + abs(imageMetaData.pixelData[i+1] - palette[j][1]) + abs(imageMetaData.pixelData[i+2] - palette[j][2]);
      idx = (colorDeviation < closestColor) ? j : idx;
      closestColor = (colorDeviation < closestColor) ? colorDeviation : closestColor;
    }
    imageMetaData.pixelData[i] = palette[idx][0];
    imageMetaData.pixelData[i+1] = palette[idx][1];
    imageMetaData.pixelData[i+2] = palette[idx][2];
  }

  free(tally);
}

