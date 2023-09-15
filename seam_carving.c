unsigned char* seam_carving(unsigned char *sobelImg, int resize) {
  if(!sobelImg) sobelImg = sobel();
  unsigned char *writePtr, *readPtr, *p = imageMetaData.pixelData;
  int *energyMap = malloc(sizeof(int) * imageMetaData.height * imageMetaData.width);
  int* seam = malloc(sizeof(int) * imageMetaData.height);
  int x __attribute__((aligned(32))), y __attribute__((aligned(32))), z, *energyPtr, width __attribute__((aligned(32))) = imageMetaData.width;
  register r;

  while(imageMetaData.width > resize) { //main seam removal loop

    energyPtr = &energyMap[width * (imageMetaData.height - 1)];
    readPtr = &sobelImg[width * (imageMetaData.height - 1)];
    r = width;
    while(r--) //copy bottom row from sobelImg to energy map
      *energyPtr++ = *readPtr++;

    for(int i = imageMetaData.height - 2; i >= 0; i--) { //build energy map, bottom-up
      x = width * i;
      y = x + width;
      energyMap[x] = sobelImg[x] + ((energyMap[y+1] < energyMap[y]) ? energyMap[y+1] : energyMap[y]); //left edge
      energyMap[y-1] = sobelImg[y-1] + ((energyMap[y+width-2] < energyMap[y+width-1]) ? energyMap[y+width-2] : energyMap[y+width-1]); //right edge
      for(int j = width - 2; j > 0; j--) {
        z = (energyMap[y+j-1] < energyMap[y+j]) ? energyMap[y+j-1] : energyMap[y+j];
        energyMap[x+j] = sobelImg[x+j] + ((energyMap[y+j+1] < z) ? energyMap[y+j+1] : z);
      }
    }

    /* start seam */
    r = 0;
    for(int i = 1; i < width; i++) //find lowest energy starting point in first row
      r = (energyMap[i] < energyMap[r]) ? i : r; //idx pos
    y = seam[0] = r;

    for(int i = 1; i < imageMetaData.height; i++) { //trace seam on energy map
      x = y + width;
      seam[i] = width;
      if(x == width * i) //left edge
        seam[i] += energyMap[x+1] < energyMap[x];
      else if(x == width * (i+1) - 1) //right edge
        seam[i] -= energyMap[x-1] < energyMap[x];
      else { //middle (not a side edge)
        r = (energyMap[x-1] < energyMap[x]) ? energyMap[x-1] : energyMap[x]; //relative advancement
        if(energyMap[x+1] < r) seam[i] += 1;
        else seam[i] -= energyMap[x-1] < energyMap[x];
      }
      y += seam[i]; //seam total (for finding the tail)
    }

    /* img array compacting */

    //filter/gradient img
    writePtr = readPtr = &sobelImg[seam[0]]; //trim seam from sobel filtered img
    readPtr++;
    for(int i = 1; i < imageMetaData.height; i++) { //compact array, deleting seam
      r = seam[i] - 1;
      memcpy(writePtr, readPtr, r);
      writePtr += r;
      readPtr += r + 1;
    }
    r = imageMetaData.height * width - y - 1; //last half of last line
    while(r--) //compact the tail
      *writePtr++ = *readPtr++;

    /* compact src img */

    writePtr = readPtr = &p[seam[0] * imageMetaData.colorspace]; //trim seam from src img
    readPtr += imageMetaData.colorspace;
    for(int i = 1; i < imageMetaData.height; i++) { //compact array, deleting seam
      r = (seam[i] - 1) * imageMetaData.colorspace;
      memcpy(writePtr, readPtr, r);
      writePtr += r;
      readPtr += r + imageMetaData.colorspace;
    }
    r = (imageMetaData.height * width - y - 1) * imageMetaData.colorspace; //last half of last line
    while(r--) //compact the tail
      *writePtr++ = *readPtr++;

    width = --imageMetaData.width;
  } //end seam carving loop

  free(seam);
  free(energyMap);
  return sobelImg;
}



/* draw seam line with the lowest energy (seam carving) */
unsigned char* draw_seam(unsigned char *sobelImg) {
  if(!sobelImg) sobelImg = sobel();
  unsigned char *p = imageMetaData.pixelData;
  int *energyMap = malloc(sizeof(int) * imageMetaData.height * imageMetaData.width);
  int x, z, y = imageMetaData.width, width = imageMetaData.width, c = imageMetaData.colorspace;

    while(y--) //copy bottom row from sobelImg to energy map
      energyMap[width * imageMetaData.height - y] = sobelImg[width * imageMetaData.height - y];

    for(int i = imageMetaData.height - 2; i >= 0; i--) { //build energy map, bottom-up
      x = width * i;
      y = x + width;
      energyMap[x] = sobelImg[x] + ((energyMap[y+1] < energyMap[y]) ? energyMap[y+1] : energyMap[y]); //left edge
      energyMap[y-1] = sobelImg[y-1] + ((energyMap[y+width-2] < energyMap[y+width-1]) ? energyMap[y+width-2] : energyMap[y+width-1]); //right edge
      for(int j = width - 2; j > 0; j--) {
        z = (energyMap[y+j-1] < energyMap[y+j]) ? energyMap[y+j-1] : energyMap[y+j];
        energyMap[x+j] = sobelImg[x+j] + ((energyMap[y+j+1] < z) ? energyMap[y+j+1] : z);
      }
    }

    /* start seam */
    y = 0;
    for(int i = 1; i < width; i++) //find lowest energy starting point in first row
      y = (energyMap[i] < energyMap[y]) ? i : y; //idx pos
    p[y*c] = p[y*c+1] = p[y*c+2] = 255;

    for(int i = 1; i < imageMetaData.height; i++) { //trace seam on energy map
      y += width;
      if(y == width * i) //left edge
        y += energyMap[y+1] < energyMap[y];
      else if(y == width * (i+1) - 1) //right edge
        y -= energyMap[y-1] < energyMap[y];
      else { //middle (not a side edge)
        x = (energyMap[y-1] < energyMap[y]) ? energyMap[y-1] : energyMap[y]; //relative advancement
        if(energyMap[y+1] < x) y += 1;
        else y -= energyMap[y-1] < energyMap[y];
      }
      p[y*c+2] = p[y*c+1] = p[y*c] = 255;
    }

  free(energyMap);
  return sobelImg;
}



/* Seam Expansion */
void seam_expansion(int resize) {
  unsigned char *convolution = sobel();
  unsigned char *writePtr, *readPtr, *tmp;
  unsigned char *p = malloc(imageMetaData.height * resize * imageMetaData.colorspace);
  memcpy(p, imageMetaData.pixelData, imageMetaData.height * imageMetaData.width * imageMetaData.colorspace);
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = p;
  unsigned char *sobelImg = malloc(imageMetaData.height * resize + imageMetaData.height);
  memcpy(sobelImg, convolution, imageMetaData.height * imageMetaData.width);
  free(convolution);
  int *energyMap = malloc(sizeof(int) * imageMetaData.height * resize);
  int *seam = malloc(sizeof(int) * imageMetaData.height);
  unsigned char *newPixels = malloc(imageMetaData.height * imageMetaData.colorspace + 4);
  unsigned char *rawImgTmp = malloc(imageMetaData.height * resize * imageMetaData.colorspace);
  unsigned char *sobelTmp = malloc(imageMetaData.height * resize + imageMetaData.height);
  int x __attribute__((aligned(32))), y __attribute__((aligned(32))), z, *energyPtr, width __attribute__((aligned(32))) = imageMetaData.width, c = imageMetaData.colorspace;
  register r;

  while(imageMetaData.width < resize) { //main seam removal loop

    energyPtr = &energyMap[width * (imageMetaData.height - 1)];
    readPtr = &sobelImg[width * (imageMetaData.height - 1)];
    r = width;
    while(r--) //copy bottom row from sobelImg to energy map
      *energyPtr++ = *readPtr++;

    for(int i = imageMetaData.height - 2; i >= 0; i--) { //build energy map, bottom-up
      x = width * i;
      y = x + width;
      energyMap[x] = sobelImg[x] + ((energyMap[y+1] < energyMap[y]) ? energyMap[y+1] : energyMap[y]); //left edge
      energyMap[y-1] = sobelImg[y-1] + ((energyMap[y+width-2] < energyMap[y+width-1]) ? energyMap[y+width-2] : energyMap[y+width-1]); //right edge
      for(int j = width - 2; j > 0; j--) {
        z = (energyMap[y+j-1] < energyMap[y+j]) ? energyMap[y+j-1] : energyMap[y+j];
        energyMap[x+j] = sobelImg[x+j] + ((energyMap[y+j+1] < z) ? energyMap[y+j+1] : z);
      }
    }

    /* start seam */
    r = 0;
    for(int i = 1; i < width; i++) //find lowest energy starting point in first row
      r = (energyMap[i] < energyMap[r]) ? i : r; //idx pos
    seam[0] = r;

    for(int i = 1; i < imageMetaData.height; i++) { //trace seam on energy map
      seam[i] = x = seam[i-1] + width; //img array index
      if(x == width * i) //left edge
        seam[i] += energyMap[x+1] < energyMap[x];
      else if(x == width * (i+1) - 1) //right edge
        seam[i] -= energyMap[x-1] < energyMap[x];
      else { //middle (not a side edge)
        r = (energyMap[x-1] < energyMap[x+1]) ? x-1 : x+1;
        seam[i] = (energyMap[r] < seam[i]) ? r : seam[i];
      }
    }

    /* interpolate new pixels */

    for(int i = 1; i < imageMetaData.height - 1; i++) {
      r = seam[i] * imageMetaData.colorspace;
      x = width * imageMetaData.colorspace;
      y = i * imageMetaData.colorspace;

      if(r % x == width - 1) { //right edge
        newPixels[y] = p[r];
        newPixels[y+1] = p[r+1];
        newPixels[y+2] = p[r+2];
        newPixels[y+3] = p[r+3]; //if colorspace = 3, this will be overwritten. no need for a branch/if
        continue;
      }
      newPixels[y] = (p[r] + p[r+c]) >> 1;
      newPixels[y+1] = (p[r+1] + p[r+1+c]) >> 1;
      newPixels[y+2] = (p[r+2] + p[r+2+c]) >> 1;
      newPixels[y+3] = (p[r+3] + p[r+3+c]) >> 1; //if colorspace = 3, this will be overwritten. no need for a branch/if
    }

    /* img array expanding */

    //filter/gradient img
    writePtr = sobelTmp;
    readPtr = sobelImg;
    memcpy(writePtr, readPtr, seam[0]);
    writePtr += seam[0];
    readPtr += seam[0];
    *writePtr++ = ++*writePtr++; //plus one prevents all proceeding seams from following the same path

    for(int i = 1; i < imageMetaData.height; i++) {
      r = seam[i] - seam[i-1];
      memcpy(writePtr, readPtr, r);
      writePtr += r;
      readPtr += r;
      *writePtr++ = ++*writePtr++; //increment seam pixel then set inserted pixel to same value
    }
    r = imageMetaData.height * width - seam[imageMetaData.height-1] - 1; //last half of last line
    while(r--) //copy the tail
      *writePtr++ = *readPtr++;

    //flip old and new images
    tmp = sobelTmp;
    sobelTmp = sobelImg;
    sobelImg = tmp;

    /* expand src img */
    z = 0;
    writePtr = rawImgTmp;
    readPtr = p;
    r = seam[0] * imageMetaData.colorspace;// + imageMetaData.colorspace;
    memcpy(writePtr, readPtr, r);
    writePtr += r;
    readPtr += r;
    r = imageMetaData.colorspace;
    while(r--)
      *writePtr++ = newPixels[z++];

    for(int i = 1; i < imageMetaData.height; i++) {
      r = (seam[i] - seam[i-1]) * imageMetaData.colorspace;
      memcpy(writePtr, readPtr, r);
      writePtr += r;
      readPtr += r;
      r = imageMetaData.colorspace;
      while(r--)
        *writePtr++ = newPixels[z++];
    }
    r = (imageMetaData.height * width - seam[imageMetaData.height-1] - 1) * imageMetaData.colorspace; //last half of last line
    while(r--) //copy the tail
      *writePtr++ = *readPtr++;

    //flip old and new images
    tmp = rawImgTmp;
    rawImgTmp = p;
    imageMetaData.pixelData = p = tmp;

    width = ++imageMetaData.width;
  } //end seam expanding loop

  free(seam);
  free(energyMap);
  free(sobelTmp);
  free(sobelImg);
  free(newPixels);
  free(rawImgTmp);
  return;
}

