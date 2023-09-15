int scale_by_half(int antiAliasing) {
  //rounds down
  unsigned char (*p)[imageMetaData.width * imageMetaData.colorspace] = imageMetaData.pixelData;
  unsigned char *s = malloc((imageMetaData.width * imageMetaData.height * imageMetaData.colorspace) >> 2);
  int z = 0, w = imageMetaData.width, h = imageMetaData.height, c = imageMetaData.colorspace, oddColumns = w & 1, oddRows = h & 1;
  register r;

  if(antiAliasing == 0) { //4 block avg
    for(int i = 0; i < h - 1; i+=2) {
      for(int j = 0; j < (w - 1) * c; j += c*2) {
        for(int k = 0; k < c; k++) {
          r = j+k;
          s[z++] = (p[i][r] + p[i][r+c] + p[i+1][r] + p[i+1][r+c]) >> 2;
        }
      }
    }
  }
  else { //9 block weighted avg
    for(int i = 0; i < h - 2; i+=2) {
      for(int j = 0; j < (w - 2) * c; j += c*2) {
        for(int k = 0; k < c; k++) {
          r = j+k;
          s[z++] = (p[i][r] + p[i][r+c] + p[i][r+c+c] + p[i+1][r] + 5 * p[i+1][r+c] + (p[i+1][r+c+c] << 1) + p[i+2][r] + (p[i+2][r+c] << 1) + (p[i+2][r+c+c] << 1)) >> 4;
          //slightly weaker anti-aliasing
          //s[z++] = (p[i][r] + p[i][r+c] + p[i][r+c+c] + p[i+1][r] + (p[i+1][r+c] << 3) + p[i+1][r+c+c] + p[i+2][r] + p[i+2][r+c] + p[i+2][r+c+c]) >> 4;
        }
      }
      for(int j = w*c - c*(1-oddColumns); j < w*c; j++) { //last column, only if even columns
        s[z++] = (p[i][j-c] + p[i][j] + p[i+1][j-c] + p[i+1][j]) >> 2;
      }
    }
    for(int i = w*c*oddRows; i < (w-2)*c; i+=c*2) { //last row, only if even rows
      for(int j = 0; j < c; j++) {
        s[z++] = (p[h-2][i+j] + p[h-2][i+j+c] + p[h-2][i+j+c+c] + p[h-1][i+j] + 3 * p[h-1][i+j+c] + p[h-1][i+j+c+c]) >> 3;
      }
    }
    if(!(oddRows | oddColumns)) { //last pixel, only if even rows AND even columns
      for(int i = w*c-c; i < w*c; i++) {
        s[z++] = (p[h-2][i-c] + p[h-2][i] + p[h-1][i-c] + p[h-1][i]) >> 2;
      }
    }
  }

  imageMetaData.height >>= 1;
  imageMetaData.width >>= 1;
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = s;
  return 0;
}


int scale_by_third() {
  //rounds down
  unsigned char (*p)[imageMetaData.width * imageMetaData.colorspace] = imageMetaData.pixelData;
  unsigned char *s = malloc(imageMetaData.width * imageMetaData.height * imageMetaData.colorspace / 9);
  int z = 0, w = imageMetaData.width, h = imageMetaData.height, c = imageMetaData.colorspace;
  register r;

  for(int i = 0; i < h - 2; i+=3) {
    for(int j = 0; j < (w - 2) * c; j += c*3) {
      for(int k = 0; k < c; k++) {
        r = j+k;
        s[z++] = (p[i][r] + p[i][r+c] + p[i][r+c+c] + p[i+1][r] + (p[i+1][r+c] << 3) + p[i+1][r+c+c] + p[i+2][r] + p[i+2][r+c] + p[i+2][r+c+c]) >> 4;
      }
    }
  }

  imageMetaData.height /= 3;
  imageMetaData.width /= 3;
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = s;
  return 0;
}


int scale_by_quarter() {
  //rounds down
  unsigned char (*p)[imageMetaData.width * imageMetaData.colorspace] = imageMetaData.pixelData;
  unsigned char *s = malloc((imageMetaData.width * imageMetaData.height * imageMetaData.colorspace) >> 2);
  int z = 0, w = imageMetaData.width, h = imageMetaData.height, c = imageMetaData.colorspace;
  register r;

  for(int i = 0; i < h - 3; i+=4) {
    for(int j = 0; j < (w - 3) * c; j += c*4) {
      for(int k = 0; k < c; k++) {
        r = j+k+c;
        s[z++] = ((p[i][r-c]   >> 1) + (p[i][r]   >> 2) + (p[i][r+c]   >> 2) + (p[i][r+c+c]   >> 1) +\
                  (p[i+1][r-c] >> 2) +  p[i+1][r]       +  p[i+1][r+c]       + (p[i+1][r+c+c] >> 2) +\
                  (p[i+2][r-c] >> 2) +  p[i+2][r]       +  p[i+2][r+c]       + (p[i+2][r+c+c] >> 2) +\
                  (p[i+3][r-c] >> 1) + (p[i+3][r] >> 2) + (p[i+3][r+c] >> 2) + (p[i+3][r+c+c] >> 1)) >> 3;
      }
    }
  }

  imageMetaData.height >>= 2;
  imageMetaData.width >>= 2;
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = s;
  return 0;
}

