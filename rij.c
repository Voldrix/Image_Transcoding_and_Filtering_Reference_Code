int rij_decode(void* srcImg, unsigned int srcImgSize) {
  unsigned int *header = srcImg;
  int w = imageMetaData.width = header[1];
  int h = imageMetaData.height = header[2];
  int c = imageMetaData.colorspace = header[3];
  imageMetaData.rawImgSize = w * h * c;
  unsigned int *colorChannels = &header[4];
  imageMetaData.pixelData = malloc(w * h * c);
  unsigned char* decompressedChannelBlocks = malloc(w * h * c);

  //decompress each color channel.
  for(int i = 0; i < c; i++) {
    int dataSize = (i < c - 1) ? colorChannels[i+1] - colorChannels[i] : srcImgSize - colorChannels[i];
    rij_decompress(srcImg + colorChannels[i], decompressedChannelBlocks + (w * h * i), dataSize);
  }

  //interleave color channels into RGB(A) order
  if(c == 1) return 0; //single channel greyscale does not need interleaving
  unsigned char *writePtr = imageMetaData.pixelData;
  unsigned char *red = decompressedChannelBlocks;
  unsigned char *green = decompressedChannelBlocks + (w * h);
  unsigned char *blue = decompressedChannelBlocks + (w * h * 2);
  unsigned char *alpha = decompressedChannelBlocks + (w * h * 3);
  int count = w * h;

  if(c == 3) { //RGB
    while(count--) {
      *writePtr++ = *red++;
      *writePtr++ = *green++;
      *writePtr++ = *blue++;
    }
  }
  else if(c == 4) { //RGBA
    while(count--) {
      *writePtr++ = *red++;
      *writePtr++ = *green++;
      *writePtr++ = *blue++;
      *writePtr++ = *alpha++;
    }
  }

  free(decompressedChannelBlocks);
  return 0;
}


int rij_encode() {
  imageMetaData.encodedImg = malloc(imageMetaData.width * imageMetaData.height * imageMetaData.colorspace + 32);
  unsigned char* partitionedChannels = malloc(imageMetaData.width * imageMetaData.height * imageMetaData.colorspace);
  unsigned int *header = imageMetaData.encodedImg;
  header[0] = 'V' | ('S' << 8);
  int w = header[1] = imageMetaData.width;
  int h = header[2] = imageMetaData.height;
  int c = header[3] = imageMetaData.colorspace;
  unsigned int *colorChannels = &header[4];
  colorChannels[0] = 32; //fixed header size
  int totalBytesWritten = 32; //output file size.

  //segregate color channels into continuous blocks
  for(int i = 0; i < c; i++) {
    int z = w * h * i;
    for(int j = i; j < w * h * c; j += c)
      partitionedChannels[z++] = imageMetaData.pixelData[j];
  }

  //compress each color channel block.
  for(int i = 0; i < c; i++) {
    totalBytesWritten += rij_compress(partitionedChannels + (w * h * i), imageMetaData.encodedImg + totalBytesWritten, w * h);
    if(i < c - 1) colorChannels[i+1] = totalBytesWritten; //channel block offsets
  }

  imageMetaData.destImgSize = totalBytesWritten;
  free(partitionedChannels);
  return 0;
}


#define RIZ_STRIP_WIDTH 24

int riz_decode(void* srcImg, unsigned int srcImgSize) {
  unsigned int *header = srcImg;
  int w = imageMetaData.width = header[1];
  int h = imageMetaData.height = header[2];
  int c = imageMetaData.colorspace = header[3];
  imageMetaData.rawImgSize = w * h * c;
  unsigned int *colorChannels = &header[4];
  imageMetaData.pixelData = malloc(imageMetaData.rawImgSize);
  unsigned char* decompressedChannelBlocks = malloc(imageMetaData.rawImgSize + (imageMetaData.rawImgSize / RIZ_STRIP_WIDTH) + RIZ_STRIP_WIDTH);

  //decompress each color channel.
  int totalBytes = 0;
  for(int i = 0; i < c; i++) {
    int dataSize = (i < c - 1) ? colorChannels[i+1] - colorChannels[i] : srcImgSize - colorChannels[i];
    totalBytes += rij_decompress(srcImg + colorChannels[i], decompressedChannelBlocks + totalBytes, dataSize);
  }

if(totalBytes != imageMetaData.rawImgSize + (imageMetaData.rawImgSize / RIZ_STRIP_WIDTH)) printf("error\ntotal bytes: %i\nmalloc size: %i\n", totalBytes, imageMetaData.rawImgSize + (imageMetaData.rawImgSize / RIZ_STRIP_WIDTH) + 1);
else printf("decode pass 1\n");

  //Burrows Wheeler Transform
  int idx, BWTsize = 0;
  for(int i = 0; i < totalBytes; i += RIZ_STRIP_WIDTH + 1) {
    idx = decompressedChannelBlocks[i];
    Burrows_Wheeler_Inverse(decompressedChannelBlocks + i + 1, decompressedChannelBlocks + BWTsize, RIZ_STRIP_WIDTH, idx);
    BWTsize += RIZ_STRIP_WIDTH;
  }
  //remainder
  int BWTremainder = totalBytes % (RIZ_STRIP_WIDTH + 1);
  if(BWTremainder == 1) {decompressedChannelBlocks[BWTsize] = decompressedChannelBlocks[totalBytes - 1];}
  if(BWTremainder > 1) {
    idx = decompressedChannelBlocks[totalBytes - BWTremainder];
    Burrows_Wheeler_Inverse(decompressedChannelBlocks + (totalBytes - BWTremainder) + 1, decompressedChannelBlocks + BWTsize, BWTremainder, idx);
    BWTsize += BWTremainder;
  }

if(BWTsize != imageMetaData.rawImgSize) printf("error\nbtw size: %i\nraw size: %i\n", BWTsize, imageMetaData.rawImgSize);
else printf("decode pass 2\n");

  //interleave color channels into RGB(A) order
  if(c == 1) return 0; //single channel greyscale does not need interleaving
  unsigned char *writePtr = imageMetaData.pixelData;
  unsigned char *red = decompressedChannelBlocks;
  unsigned char *green = decompressedChannelBlocks + (w * h);
  unsigned char *blue = decompressedChannelBlocks + (w * h * 2);
  unsigned char *alpha = decompressedChannelBlocks + (w * h * 3);
  int count = w * h;

  if(c == 3) { //RGB
    while(count--) {
      *writePtr++ = *red++;
      *writePtr++ = *green++;
      *writePtr++ = *blue++;
    }
  }
  else if(c == 4) { //RGBA
    while(count--) {
      *writePtr++ = *red++;
      *writePtr++ = *green++;
      *writePtr++ = *blue++;
      *writePtr++ = *alpha++;
    }
  }

  free(decompressedChannelBlocks);
  return 0;
}

int riz_encode() {
  imageMetaData.rawImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  imageMetaData.encodedImg = malloc(imageMetaData.rawImgSize * 1.25 + 32);
  unsigned char* partitionedChannels = malloc(imageMetaData.rawImgSize + (imageMetaData.rawImgSize / RIZ_STRIP_WIDTH) + RIZ_STRIP_WIDTH);
  unsigned int *header = imageMetaData.encodedImg;

  header[0] = 'V' | ('Z' << 8);
  header[1] = imageMetaData.width;
  header[2] = imageMetaData.height;
  int c = header[3] = imageMetaData.colorspace;
  unsigned int *colorChannels = &header[4];
  colorChannels[0] = 32; //fixed header size
  int totalBytesWritten = 32; //output file size.

  //segregate color channels into continuous blocks
  for(int i = 0; i < c; i++) {
    int z = imageMetaData.width * imageMetaData.height * i + 32;
    for(int j = i; j < imageMetaData.rawImgSize; j += c)
      imageMetaData.encodedImg[z++] = imageMetaData.pixelData[j];
  }

  //Burrows Wheeler Transform
  int BWTsize = 0;
  for(int i = 32; i < imageMetaData.rawImgSize; i += RIZ_STRIP_WIDTH) {
    partitionedChannels[BWTsize] = Burrows_Wheeler_Transform(imageMetaData.encodedImg + i, partitionedChannels + BWTsize + 1, RIZ_STRIP_WIDTH);
    BWTsize += RIZ_STRIP_WIDTH + 1;
  }
  //bwt remainder
  int BWTremainder = imageMetaData.rawImgSize % RIZ_STRIP_WIDTH;
  if(BWTremainder == 1) {partitionedChannels[BWTsize] = 1; partitionedChannels[BWTsize] = imageMetaData.encodedImg[imageMetaData.rawImgSize - 1];}
  if(BWTremainder > 1) partitionedChannels[BWTsize] = Burrows_Wheeler_Transform(&imageMetaData.encodedImg[imageMetaData.rawImgSize - BWTremainder], &partitionedChannels[BWTsize + 1], BWTremainder);
  BWTsize += BWTremainder + !!BWTremainder;

  //RLE compress each color channel block.
  int stripSize = BWTsize / c;
  for(int i = 0; i < c; i++) {
    int remainder = (i == c - 1) ? BWTsize % c : 0; //add remainder to last strip
    totalBytesWritten += rij_compress(partitionedChannels + (stripSize * i), imageMetaData.encodedImg + totalBytesWritten, stripSize + remainder);
    if(i < c - 1) colorChannels[i+1] = totalBytesWritten; //channel block offsets
  }

  imageMetaData.destImgSize = totalBytesWritten;
  free(partitionedChannels);
  return 0;
}

