int jpeg_decode(unsigned char* srcImg, unsigned int srcImgSize) {
  struct jpeg_decompress_struct info;
  struct jpeg_error_mgr err;
  unsigned char* pixRowBuf[1];

  info.err = jpeg_std_error(&err);
  jpeg_create_decompress(&info);
  jpeg_mem_src(&info, srcImg, srcImgSize);
  jpeg_read_header(&info, TRUE);
  jpeg_start_decompress(&info);

  imageMetaData.colorspace = info.output_components;
  imageMetaData.width = info.output_width;
  imageMetaData.height = info.output_height;
  imageMetaData.rawImgSize = info.output_width * info.output_height * info.output_components;
  int subPixelsPerRow = info.output_width * info.output_components;
  imageMetaData.pixelData = malloc(imageMetaData.rawImgSize);

  /* Read scanlines */
  while(info.output_scanline < info.output_height) {
    *pixRowBuf = &imageMetaData.pixelData[subPixelsPerRow * info.output_scanline];
    jpeg_read_scanlines(&info, pixRowBuf, 1);
  }
  jpeg_finish_decompress(&info);
  jpeg_destroy_decompress(&info);
  return 0;
}


int jpeg_encode() {
  struct jpeg_compress_struct info;
  struct jpeg_error_mgr err;
  unsigned char* pixRowBuf[1];
  imageMetaData.destImgSize = imageMetaData.rawImgSize;
  imageMetaData.encodedImg = malloc(imageMetaData.destImgSize);

  info.err = jpeg_std_error(&err);
  jpeg_create_compress(&info);
  jpeg_mem_dest(&info, &imageMetaData.encodedImg, &imageMetaData.destImgSize);
  info.image_width = imageMetaData.width;
  info.image_height = imageMetaData.height;
  info.input_components = imageMetaData.colorspace;
  info.in_color_space = JCS_RGB;
  jpeg_set_defaults(&info);
  jpeg_set_quality(&info, 90, TRUE);
  jpeg_start_compress(&info, TRUE);
  int subPixelsPerRow = imageMetaData.width * imageMetaData.colorspace;

  /* Write scanlines */
  while(info.next_scanline < info.image_height) {
    *pixRowBuf = &imageMetaData.pixelData[subPixelsPerRow * info.next_scanline];
    jpeg_write_scanlines(&info, pixRowBuf, 1);
  }

  jpeg_finish_compress(&info);
  jpeg_destroy_compress(&info);
  return 0;
}


int png_decode(unsigned char* srcImg, unsigned int srcImgSize) {
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if(!png_ptr) return 1;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    return 1;
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if(!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    return 1;
  }
  if(setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return 1;
  }

  FILE *srcPNG = fmemopen(srcImg, srcImgSize, "rb");
  png_init_io(png_ptr, srcPNG);
  png_read_info(png_ptr, info_ptr);

  int color_type;
  png_get_IHDR(png_ptr, info_ptr, &imageMetaData.width, &imageMetaData.height, &imageMetaData.colorspace, &color_type, 0, 0, 0);
  if(imageMetaData.colorspace != 8) return 1;
  imageMetaData.colorspace = (color_type == PNG_COLOR_TYPE_RGB_ALPHA) ? 4 : 3;

  png_bytep *row_pointers = malloc(sizeof(png_bytep) * imageMetaData.height);
  imageMetaData.rawImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  imageMetaData.pixelData = malloc(imageMetaData.rawImgSize);

  for(int i = 0; i < imageMetaData.height; i++)
    row_pointers[i] = &imageMetaData.pixelData[i * imageMetaData.width * imageMetaData.colorspace];

  png_read_image(png_ptr, row_pointers);
  png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(row_pointers);
  fclose(srcPNG);
  return 0;
}


int png_encode() {
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if(!png_ptr)
    return 1;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return 1;
  }
  if(setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 2;
  }

  imageMetaData.encodedImg = malloc(imageMetaData.rawImgSize + 128);
  FILE *destImg = fmemopen(imageMetaData.encodedImg, imageMetaData.rawImgSize + 128, "wb");
  png_init_io(png_ptr, destImg);

  int color_type = (imageMetaData.colorspace == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA;
  png_set_IHDR(png_ptr, info_ptr, imageMetaData.width, imageMetaData.height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);

  png_bytep *row_pointers = malloc(sizeof(png_bytep) * imageMetaData.height);
  for(int i = 0; i < imageMetaData.height; i++)
    row_pointers[i] = &imageMetaData.pixelData[i * imageMetaData.width * imageMetaData.colorspace];

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, 0);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(row_pointers);
  imageMetaData.destImgSize = ftell(destImg);
  fclose(destImg);
  return 0;
}


int webp_decode(unsigned char* srcImg, unsigned int srcImgSize) {
  if(!WebPGetInfo(srcImg, srcImgSize, &imageMetaData.width, &imageMetaData.height)) return 1; //lib version mismatch

  WebPBitstreamFeatures features;
  if(WebPGetFeatures(srcImg, srcImgSize, &features)) return 2;

  WebPDecoderConfig config;
  WebPInitDecoderConfig(&config);
  imageMetaData.colorspace = (features.has_alpha | features.has_animation) ? 4 : 3;
  config.output.colorspace = (features.has_alpha | features.has_animation) ? MODE_RGBA : MODE_RGB;
  config.options.no_fancy_upsampling = 1;
  config.options.bypass_filtering = 1;
  config.options.use_scaling = 0;
  config.options.dithering_strength = 0;
  config.options.alpha_dithering_strength = 0;

  //if animated img, grab first frame
  if(features.has_animation) {
    WebPData webp_data;
    webp_data.bytes = srcImg;
    webp_data.size = srcImgSize;
    WebPDemuxer* demux = WebPDemux(&webp_data);
    WebPIterator framePtr;
    if(!WebPDemuxGetFrame(demux, 1, &framePtr)) return 3;
    //config.input.height = framePtr.height;
    //config.input.width = framePtr.width;
    //config.input.has_alpha = framePtr.has_alpha;
    //config.input.has_animation = 1;

    if(WebPDecode(framePtr.fragment.bytes, framePtr.fragment.size, &config)) return 4;
    WebPDemuxReleaseIterator(&framePtr);
    WebPDemuxDelete(demux);
  }
  else //not animated
    if(WebPDecode(srcImg, srcImgSize, &config)) return 5;
  imageMetaData.pixelData = config.output.u.RGBA.rgba;
  imageMetaData.rawImgSize = config.output.u.RGBA.size;

  if(config.output.u.RGBA.stride != imageMetaData.width * imageMetaData.colorspace) {
    printf("stride error: end of row padding inserted\nW*C %i\nStride %i\n",imageMetaData.width * imageMetaData.colorspace, config.output.u.RGBA.stride);
    return 6;
  }
  return 0;
}


int webp_encode() {
  WebPConfig config;
  WebPConfigInit(&config);
  config.quality = 90; //0-100
  config.method = 5; //0-6 0=fast, 6=slow,higher-quality
  config.filter_strength = 0; //off
  config.preprocessing = 0; //off

  WebPPicture pic;
  if(!WebPPictureInit(&pic)) return 1; //lib version mismatch
  WebPMemoryWriter writer;
  WebPMemoryWriterInit(&writer);

  pic.use_argb = imageMetaData.colorspace - 3;
  pic.colorspace = (imageMetaData.colorspace == 4) ? WEBP_YUV420A : WEBP_YUV420;
  pic.width = imageMetaData.width;
  pic.height = imageMetaData.height;
  pic.writer = WebPMemoryWrite;
  pic.custom_ptr = (void*)&writer;
  if(imageMetaData.colorspace == 4)
    WebPPictureImportRGBA(&pic, imageMetaData.pixelData, imageMetaData.width * imageMetaData.colorspace);
  else
    WebPPictureImportRGB(&pic, imageMetaData.pixelData, imageMetaData.width * imageMetaData.colorspace);

  if(!WebPPictureAlloc(&pic)) return 2;
  if(!WebPEncode(&config, &pic)) return 3;
  WebPPictureFree(&pic);

  imageMetaData.encodedImg = writer.mem;
  imageMetaData.destImgSize = writer.size;
  return 0;
}


int ppm_decode(unsigned char* srcImg, unsigned int srcImgSize) {
  unsigned char* whitespace = " \r\n\t\v\f";
  unsigned char* header = strtok(srcImg + 3, whitespace);
  imageMetaData.width = atoi(header);
  header = strtok(0, whitespace);
  imageMetaData.height = atoi(header);
  header = strtok(0, whitespace);
  int maxPixValue = atoi(header);
  if(maxPixValue > 255 || maxPixValue < 9) return maxPixValue;
  unsigned char* pixData = header + strlen(header) + 1;
  imageMetaData.colorspace = 3;
  imageMetaData.rawImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  imageMetaData.pixelData = malloc(imageMetaData.rawImgSize);
  memcpy(imageMetaData.pixelData, pixData, imageMetaData.rawImgSize);
  return 0;
}


int ppm_encode() {
  char header[128];
  sprintf(header, "P6 %d %d 255\n", imageMetaData.width, imageMetaData.height);
  int headerLength = strlen(header);
  imageMetaData.destImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace + headerLength;
  imageMetaData.encodedImg = malloc(imageMetaData.destImgSize);
  memcpy(imageMetaData.encodedImg, header, headerLength);
  memcpy(imageMetaData.encodedImg + headerLength, imageMetaData.pixelData, imageMetaData.destImgSize - headerLength);
  return 0;
}


int tiff_decode(unsigned char* srcImg, unsigned int srcImgSize) {
  int ifd = *(int*)(srcImg + 4); //first image file directory location. (tiff supports multiple images in one file, but this decoder does not)
  unsigned short numOfTags = *(unsigned short*)(srcImg + ifd);
  struct field {
    unsigned short id, dataType;
    int dataLength, value;
  };
  struct field *f = srcImg + ifd + 2;
  int numOfStrips, rowsPerStrip, compression, bytesWritten = 0;
  int *stripOffsets, *stripByteCounts;

  while(numOfTags--) {
    switch(f->id) {
      case 256: imageMetaData.width = f->value; break;
      case 257: imageMetaData.height = f->value; break;
      case 259: compression = f->value;
                if(f->value != 1 /*&& f->value != 5*/ && f->value != 0x8005) return 1; break; //compression, 1 = none
      case 262: if(f->value != 2) return 2; break; //full color. palettes and mono not supported
      case 273: numOfStrips = f->dataLength;
                stripOffsets = srcImg + f->value; break;
      case 277: if(f->value < 3 || f->value > 4) return 3; //only RGB(A) supported
                imageMetaData.colorspace = f->value; break;
      case 278: if(f->dataType == 4) rowsPerStrip = f->value;
                else rowsPerStrip = (short)f->value; break;
      case 279: stripByteCounts = srcImg + f->value; break;
    }
    f++;
  }
  imageMetaData.rawImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  imageMetaData.pixelData = malloc(imageMetaData.rawImgSize);

  for(int i = 0; i < numOfStrips; i++) { //assemble strips
    memcpy(imageMetaData.pixelData + bytesWritten, srcImg + stripOffsets[i], stripByteCounts[i]);
    bytesWritten += stripByteCounts[i];
  }

  if(compression > 1) { //compression
    char* x = malloc(imageMetaData.rawImgSize);
    //if(compression == 5) lzw_decode(imageMetaData.pixelData, x, bytesWritten); //LZW compression
    if(compression == 0x8005) packBits_decode(imageMetaData.pixelData, x, bytesWritten); //PackBits compression
    free(imageMetaData.pixelData);
    imageMetaData.pixelData = x;
  }
  return 0;
}


int tiff_encode() {
  unsigned char* x = calloc(1,1.5 * imageMetaData.width * imageMetaData.height * imageMetaData.colorspace);
  struct field {
    unsigned short id, dataType;
    int dataLength, value;
  };
  struct field *f = x + 10;

  x[0] = x[1] = 'I'; x[2] = 42; x[4] = 8; x[8] = 9; x[122] = x[124] = x[126] = 8;
  f->id = 256; f->dataType = 4; f->dataLength = 1; f->value = imageMetaData.width; f++;
  f->id = 257; f->dataType = 4; f->dataLength = 1; f->value = imageMetaData.height; f++;
  f->id = 258; f->dataType = 3; f->dataLength = 3; f->value = 122; f++;
  f->id = 259; f->dataType = 4; f->dataLength = 1; f->value = 5; f++;//0x8005; f++;
  f->id = 262; f->dataType = 4; f->dataLength = 1; f->value = 2; f++;
  f->id = 273; f->dataType = 4; f->dataLength = 1; f->value = 128; f++;
  f->id = 277; f->dataType = 4; f->dataLength = 1; f->value = imageMetaData.colorspace; f++;
  f->id = 278; f->dataType = 4; f->dataLength = 1; f->value = imageMetaData.height; f++;
  f->id = 279; f->dataType = 4; f->dataLength = 1;

  f->value = imageMetaData.destImgSize = lzw_encode(imageMetaData.pixelData, x + 128, imageMetaData.width * imageMetaData.height * imageMetaData.colorspace);
  imageMetaData.destImgSize += 128;
  imageMetaData.encodedImg = x;
  return 0;
}


int bmp_decode(void* srcImg, unsigned int srcImgSize) {
  int* dib = srcImg + 2;
  unsigned short *colorspace = srcImg+28;
  if(dib[7]) return 1; //img compression not supported
  switch(*colorspace) {
    case 24: imageMetaData.colorspace = 3; break;
    case 32: imageMetaData.colorspace = 4; break;
    default: return 2; //1/4/8/16 bit not supported
  }
  imageMetaData.width = dib[4];
  imageMetaData.height = abs(dib[5]);
  imageMetaData.rawImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  imageMetaData.pixelData = malloc(imageMetaData.rawImgSize);

  //reverse row order and remove line padding
  int rowWidth = imageMetaData.width * imageMetaData.colorspace;
  int rowPadded = rowWidth;
  rowPadded += (rowPadded % 4) ? 4 - (rowPadded % 4) : 0;
  unsigned char *s = imageMetaData.pixelData, *d = srcImg + dib[2];

  register r = imageMetaData.height - 1;
  if(dib[5] < 0) //rows already top-to-bottom (reversed)
    for(int i = 0; i < imageMetaData.height; i++)
      memcpy(&s[i * rowWidth], &d[rowPadded * i], rowWidth);
  else //rows are bottom-up
    for(int i = 0; i < imageMetaData.height; i++)
      memcpy(&s[i * rowWidth], &d[rowPadded * r--], rowWidth);

  //BGR -> RGB
  unsigned char b;
  for(int i = 0; i < imageMetaData.rawImgSize; i+=imageMetaData.colorspace) {
    b = s[i];
    s[i] = s[i+2];
    s[i+2] = b;
  }
  return 0;
}


int bmp_encode() {
  //bmp can encode but RGBA, but doesn't actually support transparency
  int rowWidth = imageMetaData.width * imageMetaData.colorspace;
  int rowPadded = rowWidth;
  rowPadded += (rowPadded % 4) ? 4 - (rowPadded % 4) : 0; //rows padded to 4-byte boundaries

  imageMetaData.rawImgSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  imageMetaData.destImgSize = rowPadded * imageMetaData.height + 54;
  imageMetaData.encodedImg = calloc(1,imageMetaData.destImgSize);
  unsigned char *dib = imageMetaData.encodedImg;

  dib[0] = 'B'; dib[1] = 'M';
  int *x = &dib[2];
  *x = imageMetaData.destImgSize;
  x = dib+10;
  *x = 54;
  *++x = 40;
  *++x = imageMetaData.width;
  *++x = imageMetaData.height;
  dib[26] = 1;
  dib[28] = imageMetaData.colorspace * 8;

  //RGB -> BGR
  unsigned char tmp, *d = imageMetaData.pixelData, *s = imageMetaData.encodedImg + 54;
  for(int i = 0; i < imageMetaData.rawImgSize; i+=imageMetaData.colorspace) {
    tmp = d[i];
    d[i] = d[i+2];
    d[i+2] = tmp;
  }

  //reverse row order (bottom-up)
  register r = imageMetaData.height - 1;
  for(int i = 0; i < imageMetaData.height; i++)
    memcpy(&s[rowPadded * r--], &d[i * rowWidth], rowWidth);

  return 0;
}
