//increases fidelity through quantization error diffusion
int BW_dither_floyd_steinberg() {
  int quant_error, row, oldPixel, w = imageMetaData.width, h = imageMetaData.height;
  unsigned char* mono = rgb_to_monochrome(imageMetaData.pixelData, UNWEIGHTED_AVG);
  int byteWidth = (w+2)*sizeof(int); //+2 because the first and last columns overflow 1 pixel each
  int* nextRowErr = calloc(1,byteWidth); //This gets rid of all 3 edge cases; first column, last column, last row
  nextRowErr++; //to allow index position [-1] for first column's pixels' error diffusion.
  float coefficients[4] = {7.0/16, 3.0/16, 5.0/16, 1.0/8}; //saves expensive/slow division in main loop

  for(int i = 0; i < h; i++) {
    row = w * i;
    quant_error = 0;
    memset(&nextRowErr[-1], 0, byteWidth);
    for(int j = 0; j < w; j++) {
      oldPixel = mono[row+j] + nextRowErr[j] + (coefficients[0] * quant_error);
      mono[row+j] = (oldPixel > 127) ? 255 : 0;
      quant_error = oldPixel - mono[row+j];
      //diffuse quantization error
      nextRowErr[j-1] += coefficients[1] * quant_error;
      nextRowErr[j]   += coefficients[2] * quant_error;
      nextRowErr[j+1] += coefficients[3] * quant_error;
    }
  }

  unsigned char* rgb = monochrome_to_rgb(mono); //grey back to rgb

  free(--nextRowErr);
  free(mono);
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = rgb;
  imageMetaData.colorspace = 3;
  return 0;
}


//ordered dithering with a bayer matrix (8-bit hash style)
int BW_dither_bayer(matrixSize) {
  int row, col, w = imageMetaData.width, h = imageMetaData.height;
  unsigned char* mono = rgb_to_monochrome(imageMetaData.pixelData, UNWEIGHTED_AVG);
  const int bayer_matrix16[16][16] = {
	{   0,191, 48,239, 12,203, 60,251,  3,194, 51,242, 15,206, 63,254 },
	{ 127, 64,175,112,139, 76,187,124,130, 67,178,115,142, 79,190,127 },
	{  32,223, 16,207, 44,235, 28,219, 35,226, 19,210, 47,238, 31,222 },
	{ 159, 96,143, 80,171,108,155, 92,162, 99,146, 83,174,111,158, 95 },
	{   8,199, 56,247,  4,195, 52,243, 11,202, 59,250,  7,198, 55,246 },
	{ 135, 72,183,120,131, 68,179,116,138, 75,186,123,134, 71,182,119 },
	{  40,231, 24,215, 36,227, 20,211, 43,234, 27,218, 39,230, 23,214 },
	{ 167,104,151, 88,163,100,147, 84,170,107,154, 91,166,103,150, 87 },
	{   2,193, 50,241, 14,205, 62,253,  1,192, 49,240, 13,204, 61,252 },
	{ 129, 66,177,114,141, 78,189,126,128, 65,176,113,140, 77,188,125 },
	{  34,225, 18,209, 46,237, 30,221, 33,224, 17,208, 45,236, 29,220 },
	{ 161, 98,145, 82,173,110,157, 94,160, 97,144, 81,172,109,156, 93 },
	{  10,201, 58,249,  6,197, 54,245,  9,200, 57,248,  5,196, 53,244 },
	{ 137, 74,185,122,133, 70,181,118,136, 73,184,121,132, 69,180,117 },
	{  42,233, 26,217, 38,229, 22,213, 41,232, 25,216, 37,228, 21,212 },
	{ 169,106,153, 90,165,102,149, 86,168,105,152, 89,164,101,148, 85 }
  };
  const int bayer_matrix8[8][8] = {
	{   0,128, 32,160,  8,136, 40,168 },
	{ 192, 64,224, 96,200, 72,232,104 },
	{  48,176, 16,144, 56,184, 24,152 },
	{ 240,112,208, 80,248,120,216, 88 },
	{  12,140, 44,172,  4,132, 36,164 },
	{ 204, 76,236,108,196, 68,228,100 },
	{  60,188, 28,156, 52,180, 20,148 },
	{ 252,124,220, 92,244,116,212, 84 }
  };
  matrixSize = (matrixSize != 16) ? 8 : matrixSize; //invalid input -> 8
  const int (*bayer_matrix)[matrixSize] = (matrixSize == 16) ? bayer_matrix16 : bayer_matrix8;
  --matrixSize; //to convert modulo operation -> faster bitwise and

  for(int y = 0; y < h; y++) {
    row = y & matrixSize; //y % (8 || 16)
    for(int x = 0; x < w; x++) {
      col = x & matrixSize; //x % (8 || 16)
      mono[y * w + x] = (mono[y * w + x] > bayer_matrix[row][col]) ? 255 : 0;
    }
  }

  unsigned char* rgb = monochrome_to_rgb(mono); //grey back to rgb

  free(mono);
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = rgb;
  imageMetaData.colorspace = 3;
  return 0;
}


//ordered dithering with a halftone matrix. (comicbook dots style)
int BW_dither_halftone(matrixSize) {
  int row, col, w = imageMetaData.width, h = imageMetaData.height;
  unsigned char* mono = rgb_to_monochrome(imageMetaData.pixelData, UNWEIGHTED_AVG);
  const int halftone_matrix16[16][16] = {
	{ 255,250,239,235,227,207,167,149,142,168,203,223,231,243,247,252 },
	{ 246,219,211,195,187,164,147,140, 88,136,162,188,196,212,216,251 },
	{ 242,215,183,175,155,145,132,116, 64,112,128,159,176,180,208,236 },
	{ 230,199,179,152,144,119,108, 84, 44, 80,104,124,153,172,192,232 },
	{ 222,191,158,143, 99, 95, 75, 60, 28, 56, 72, 92,100,156,184,224 },
	{ 202,161,146,123, 91, 67, 51, 40, 16, 36, 48, 68, 96,120,165,204 },
	{ 169,148,127,103, 71, 47, 31, 24,  8, 20, 32, 52, 76,105,129,170 },
	{ 150,135,111, 79, 55, 35, 19, 11,  4, 12, 21, 37, 57, 81,113,137 },
	{ 141, 87, 63, 43, 27, 15,  7,  3,  0,  1,  5, 13, 25, 41, 61, 85 },
	{ 166,139,115, 83, 59, 39, 23, 10,  2,  9, 17, 33, 53, 77,109,133 },
	{ 206,163,131,107, 74, 50, 30, 18,  6, 22, 29, 45, 69,101,125,200 },
	{ 226,186,154,118, 94, 66, 46, 34, 14, 38, 49, 65, 89,121,189,220 },
	{ 234,194,174,151, 98, 90, 70, 54, 26, 58, 73, 93, 97,177,197,228 },
	{ 238,210,182,178,157,122,102, 78, 42, 82,106,117,173,181,213,240 },
	{ 249,218,214,198,190,160,126,110, 62,114,130,185,193,209,217,244 },
	{ 254,245,241,229,221,201,171,134, 86,138,205,225,233,237,248,253 }
  };
  const int halftone_matrix8[8][8] = {
	{ 248,236,204, 64,128,188,220,252 },
	{ 216,168, 96, 32, 80,120,172,224 },
	{ 184, 76, 44, 16, 48, 84,124,192 },
	{  60, 28, 12,  0,  4, 20, 52,100 },
	{ 140, 92, 40,  8, 36, 68,112,144 },
	{ 200,136, 72, 24, 88,108,148,176 },
	{ 232,164,132, 56,116,152,160,208 },
	{ 244,212,180,104,156,196,228,240 }
  };
  matrixSize = (matrixSize != 16) ? 8 : matrixSize; //invalid input -> 8
  const int (*halftone_matrix)[matrixSize] = (matrixSize == 16) ? halftone_matrix16 : halftone_matrix8;
  --matrixSize; //to convert modulo op -> faster bitwise and

  for(int y = 0; y < h; y++) {
    row = y & matrixSize; //y % (8 || 16)
    for(int x = 0; x < w; x++) {
      col = x & matrixSize; //x % (8 || 16)
      mono[y * w + x] = (mono[y * w + x] > halftone_matrix[row][col]) ? 255 : 0;
    }
  }

  unsigned char* rgb = monochrome_to_rgb(mono); //grey back to rgb

  free(mono);
  free(imageMetaData.pixelData);
  imageMetaData.pixelData = rgb;
  imageMetaData.colorspace = 3;
  return 0;
}

