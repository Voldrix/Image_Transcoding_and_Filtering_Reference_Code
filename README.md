# Image Transcoding and Filtering Reference Code

#### Minimal reference code for encoding / decoding images of various formats, utilizing standard codec libraries. Also has code for various filters and transmutations, including seam carving.

This codebase does contain some experimental code (filters and formats).

### Files

- [main.c](main.c) :: Basic structural code to call necessary filtering functions
- [codecs.c](codecs.c) :: encoding and decoding functions for all real image formats
- [filters.c](filters.c) :: filters like monochrome, transpose, Sobel, etc.
- [compression.c](compression.c) :: PackBits RLE, custom RIJ RLE, LZW, Burrows-Wheeler
- [dither.c](dither.c) :: several dithering algorithms
- [scaling.c](scaling.c) :: quick functions to scale by specifically 1/2, 1/3, 1/4
- [rij.c](rij.c) :: RIJ and RIZ experimental / test codecs
- [seam_carving.c](seam_carving.c) :: seam carving function (not including requisite filters), seam drawing function (draws first seam instead of removing), and an exceptionally pointless seam expanding algorithm.

#### Seam Carving
Seam carving is a magical content-aware image resizing algorithm. It allows you to change the aspect ratio of an image without squishing content in the foreground by only removing pixels from the background.\
It accomplishes this by detecting edges, and drawing a continuous (but not straight) vertical line from top to bottom crossing through the least amount of edges.\
Removing horizontal seams is difficult from a programming perspective, so instead we transpose the image and do vertical seams while the image is sideways.

#### Seam Expanding
Seam expanding is exceptionally pointless because once you expand the first seam, that seam will forever be the lowest energy path. I've added a weight to increase the energy on every seam, but after a few rounds, this causes the seam to go through random higher energy areas, and basically just jump all over the image, giving it no advantage over a traditional upscale, but with more artifacts. This algorithm is also slower and uses more memory than the regular seam carving function due to having to expand multiple image arrays. This is just a test / (dis)proof of concept.

### Codecs Supported
- JPEG
- PNG
- WebP (Animations are decoded into a single static image using only the first frame)
- BMP
- TIFF (uncompressed only, LZW compression is WIP)
- PPM
- RIJ (A test codec that partitions the RGB channels, then uses simple RLE compression). decent compression, great speed, can't do scan lines
- RIZ (A test codec. RIJ, but utilizing Burrows-Wheeler transform). Not much benefit over RIJ

### Building
__prerequisites:__\
[libjpeg](https://libjpeg.sourceforge.net/)\
[libpng](http://www.libpng.org/pub/png/libpng.html)\
[zlib](https://www.zlib.net/)\
[libwebp](https://developers.google.com/speed/webp/download)\
[libvpx](https://chromium.googlesource.com/webm/libvpx/)

Compile and install prerequisite libraries, enabling static linking. Or install from Linux package manager.\
Run make, then run the executable (a.out) with no parameters to get a list of options.

## Contributing
#### ToDo
- Add GIF encoder/decoder
- Finish LZW compression
- Add compile options for prerequisite libraries to documentation
- Add GPU versions of seam carving functions

#### License
[MIT](LICENSE.txt)
