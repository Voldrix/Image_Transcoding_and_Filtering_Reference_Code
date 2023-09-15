//standard bitmap run-length encoding
int packBits_encode(char* packedImg) {
  int dataSize = imageMetaData.width * imageMetaData.height * imageMetaData.colorspace;
  int pos = 0, maxRun = 127, totalSizeCompressed = 0;
  char byte, count, *raw = imageMetaData.pixelData;

  while(pos < dataSize) {
    byte = raw[pos];
    count = 0;
    maxRun = imageMetaData.width - (pos % imageMetaData.width) - 1;
    maxRun = (maxRun > 127) ? 127 : maxRun;
    maxRun = (dataSize - pos < 127) ? dataSize - pos : maxRun;

    if(raw[pos] == raw[pos + 1]) { //repeat bytes, pack 'em
      while(raw[++pos] == byte && count < maxRun)
        count++;
      *packedImg++ = -count;
      *packedImg++ = byte;
      totalSizeCompressed += 2;
    }
    else { //raw bytes, don't pack 'em
      while(raw[pos + count] != raw[pos + count + 1] || raw[pos + count + 1] != raw[pos + count + 2] && count < maxRun)
        count++;
      *packedImg++ = count - 1;
      memcpy(packedImg, &raw[pos], count);
      packedImg += count;
      pos += count;
      totalSizeCompressed += 1 + count;
    }
  }

  return totalSizeCompressed;
}


void packBits_decode(char* compressedImg, char* unpackedImg, int dataSize) {
  char byte;

  while(dataSize) {
    byte = *compressedImg++;

    if(byte < 0) { //repeat bytes, unpack 'em
      memset(unpackedImg, *compressedImg++, 1 - byte);
      unpackedImg += 1 - byte;
      dataSize -= 2;
    }
    else { //raw bytes, don't unpack 'em
      memcpy(unpackedImg, compressedImg, 1 + byte);
      unpackedImg += 1 + byte;
      compressedImg += 1 + byte;
      dataSize -= 2 + byte;
    }
  }
}


//Experimental RLE with sequences. Debugging counters show efficiency
int rij_compress(unsigned char* rawData, unsigned char* compressedData, int dataSize) {
  unsigned char byte, count;
  int pos = 0, maxRun = 63, totalSizeCompressed = 0;
int a=0,b=0,c=0,d=0,aa=0,bb=0,cc=0,dd=0;
  while(pos < dataSize) {
    byte = rawData[pos];
    count = 0; //number of dups, not bytes. (bytes - 1)
    maxRun = (dataSize - pos < 63) ? dataSize - pos - 1 : maxRun;

    if(rawData[pos] == rawData[pos + 1]) { //repeat bytes
a++;
      while(rawData[++pos] == byte && count < maxRun)
        count++;
aa+=count+1;
      *compressedData++ = 64 + count; //header. code: 01 | count
      *compressedData++ = byte;
      totalSizeCompressed += 2;
    }
    else if(rawData[pos] == rawData[pos + 1] + 1) { //negative sequence
b++;
      while(rawData[++pos] == byte - 1 - count && count < maxRun)
        count++;
bb+=count+1;
      *compressedData++ = 128 + count; //header. code: 10 | count
      *compressedData++ = byte;
      totalSizeCompressed += 2;
    }
    else if(rawData[pos] == rawData[pos + 1] - 1) { //positive sequence
c++;
      while(rawData[++pos] == byte + 1 + count && count < maxRun)
        count++;
cc+=count+1;
      *compressedData++ = 192 + count; //header. code: 11 | count
      *compressedData++ = byte;
      totalSizeCompressed += 2;
    }
    else { //raw bytes
d++;
      //while(rawData[pos + count] != rawData[pos + count + 1] && count < maxRun) //simple version
      while((rawData[pos + count] != rawData[pos + count + 1] || rawData[pos + count + 1] != rawData[pos + count + 2]) &&\
            (rawData[pos + count] != rawData[pos + count + 1] + 1 || rawData[pos + count + 1] != rawData[pos + count + 2] + 1) &&\
            (rawData[pos + count] != rawData[pos + count + 1] - 1 || rawData[pos + count + 1] != rawData[pos + count + 2] - 1) && count < maxRun)
        count++;
dd+=count+1;
      *compressedData++ = count++; //header. code: 00 | count
      memcpy(compressedData, &rawData[pos], count);
      compressedData += count;
      pos += count;
      totalSizeCompressed += 1 + count;
    }
  }
printf("raw\t%i\t%i\t%f\ndup\t%i\t%i\t%f\nns\t%i\t%i\t%f\nps\t%i\t%i\t%f\n\n",d,dd,(float)dd/d,a,aa,(float)aa/a,b,bb,(float)bb/b,c,cc,(float)cc/c);
  return totalSizeCompressed;
}


int rij_decompress(unsigned char* compressedImg, unsigned char* unpackedImg, int dataSize) {
  unsigned char byte, code, sequence;
  int totalBytes = 0;

  while(dataSize > 0) {
    byte = *compressedImg++;
    code = byte >> 6;
    byte &= 63;
    byte++;
    totalBytes += byte;

    switch(code) {
      case 0: //raw bytes
        memcpy(unpackedImg, compressedImg, byte);
        unpackedImg += byte;
        compressedImg += byte;
        dataSize -= 1 + byte;
        break;
      case 1: //repeat bytes
        memset(unpackedImg, *compressedImg++, byte);
        unpackedImg += byte;
        dataSize -= 2;
        break;
      case 2: //negative sequence
        sequence = *compressedImg++;
        while(byte--)
          *unpackedImg++ = sequence--;
        dataSize -= 2;
        break;
      case 3: //positive sequence
        sequence = *compressedImg++;
        while(byte--)
          *unpackedImg++ = sequence++;
        dataSize -= 2;
        break;
    }
  }
  return totalBytes;
}


//for lzw
int codeCheck(int (*dict)[2], int dictSize, unsigned char *string, int stringCount) {
  int prev_char_idx, char_idx;

  for(int i = 258; i < dictSize; i++) {
    prev_char_idx = i;
    char_idx = stringCount;
    while(prev_char_idx > 257 && char_idx > 1) {
      if(dict[prev_char_idx][0] != string[char_idx--])
        goto end; //double continue
      prev_char_idx = dict[i][1];
    }
    if(char_idx == 1 && prev_char_idx < 256) return i;
    end: continue;
  }
  return 0;
}


//WIP does not work yet
#define TABLE_SIZE 4096
int lzw_encode(unsigned char* rawImg, unsigned char* destImg, int dataSize) {
  int (*dict)[2] = calloc(2, TABLE_SIZE * sizeof(int));
  unsigned char *string = malloc(TABLE_SIZE);

  int stringCount, dictSize = 258, x, prevStrPtr, codeLength = 9, output_bit_count = 1;
  unsigned int output_bit_buffer = 0, totalBytesWritten = 0;

  for(int i = 0; i < 256; i++) { //all single char strings. tiff standard, not required by lzw
    dict[i][0] = i;
    dict[i][1] = -1;
  }

  *destImg++ = 128; //write clear code first
  string[stringCount] = *rawImg++;

  while(dataSize > 0) {
    stringCount = 1;
    //string[stringCount] = *rawImg++;
    prevStrPtr = string[stringCount];
    string[++stringCount] = *rawImg++;
    //code check
    while(dataSize > stringCount && (x = codeCheck(dict, dictSize, string, stringCount))) {
      prevStrPtr = x;
      string[++stringCount] = *rawImg++;
    }
    //add code to dict
    dict[dictSize][0] = string[1] = string[stringCount]; //last char
    dict[dictSize][1] = prevStrPtr; //ptr to second to last char
//if(dictPos > 258) printf("\t%i - %i\n",dictPos, string[stringCount]);

    //output code + last char
    //dictSize is the idx pos of the new code, thus also the output code value
    output_bit_buffer |= dictSize++ << (32 - codeLength - output_bit_count);
    output_bit_buffer |= string[stringCount] << (24 - output_bit_count);
    output_bit_count += codeLength + 8;
    while(output_bit_count >= 8) {
      *destImg++ = output_bit_buffer >> 24;
      output_bit_buffer <<= 8;
      output_bit_count -= 8;
      totalBytesWritten++;
    }
    dataSize -= stringCount;
    codeLength += (dictSize & (dictSize - 1)) == 0; //variable length codes. if power of 2

    if(dictSize == 4095) { //write clear code and reset code length
      output_bit_buffer |= 256 << (32 - codeLength - output_bit_count);
      output_bit_count += codeLength;
      while(output_bit_count >= 8) {
        *destImg++ = output_bit_buffer >> 24;
        output_bit_buffer <<= 8;
        output_bit_count -= 8;
        totalBytesWritten++;
      }
      dictSize = 258;
      codeLength = 9;
    }
  }

  //write EOF code and flush buffer
  output_bit_buffer |= 257 << (32 - codeLength - output_bit_count);
  output_bit_count += codeLength;
   while(output_bit_count > 0) {
    *destImg++ = output_bit_buffer >> 24;
    output_bit_buffer <<= 8;
    output_bit_count -= 8;
    totalBytesWritten++;
  }

  return totalBytesWritten;
}


//binary safe string comparison. Does not stop on null
static inline int strIdent(register char* s1, register char* s2, register int length) {
  while(*s1++ == *s2++ && --length)
    ;
  return length; // bool. 0 = identical
}


//binary safe string comparison. Does not stop on null
static inline int cmpStr(unsigned char* s1, unsigned char* s2, register int length) {
  register int r = 0;
  while(r == 0 && length--)
    r = *s1++ - *s2++;
  return r; // 0 = identical, r > 0 -> s1 > s2, r < 0 -> s1 < s2
}


int Burrows_Wheeler_Transform(unsigned char* text, unsigned char* output, int length) {
  unsigned char (*suffix)[length] = calloc(length + 1, length);
  int *idx = malloc(length * sizeof(int));

  idx[0] = 1;
  memcpy(*suffix, text, length);
  for(int i = 1; i < length; i++) {
    idx[i] = 0;
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
        z = idx[i];
        idx[i] = idx[i+1];
        idx[i+1] = z;
        z = 1;
      }
    }
  }

  for(int i = 0; i < length; i++) {
    output[i] = suffix[i][length - 1];
    if(idx[i]) z = i;
  }

  free(idx);
  free(suffix);
  return z;
}


void Burrows_Wheeler_Inverse(unsigned char* text, unsigned char* output, int length, int idx) {
  unsigned char (*suffix)[length] = calloc(length + 1, length);
  int z;

  for(int i = length - 1; i >= 0; i--) {
    for(int j = 0; j < length; j++)
      suffix[j][i] = text[j];

    z = 1;
    while(z) {
      z = 0;
      for(int k = 0; k < length - 1; k++) {
        if(cmpStr(&suffix[k], &suffix[k+1], length) > 0) {
          memcpy(&suffix[length], &suffix[k], length);
          memcpy(&suffix[k], &suffix[k+1], length);
          memcpy(&suffix[k+1], &suffix[length], length);
          z = 1;
        }
      }
    }
  }

  memcpy(output, &suffix[idx], length);
  free(suffix);
}

