
# DICI Image Format

The **DICI image format** is a lossless image compression format that offers an improved compression speed ratio compared to more popular formats. Developed by Arnaud Recchia and Kévin Passemard, DICI aims to provide file sizes similar to or better than the WebP format, with significantly faster encoding and decoding speeds especially for large images.

## 🎨 Supported Color Formats

- **24-bit RGB**
- **32-bit RGBA**
- **48-bit RGB**
- **8-bit Grayscale**

## 🛠 Usage Instructions

To utilize DICI, ensure that **openCV** is installed as a prerequisite.

### Encoding Process

```c++
#include "DICIencode.h"

...

DICIencode imageToEncode;
imageToEncode.setFileIN("c:\image.bmp"); // Input file path
imageToEncode.setFileOUT("c:\image.dici"); // Output file path
imageToEncode.setThreaded(1); // Enable multi-threading
imageToEncode.DICIcompress(); // Initiate compression
```

### Decoding Process

```c++
#include "DICIdecode.h"

...

DICIdecode imgToDecode;
imgToDecode.setFileIN("c:\image.dici"); // Input DICI file path
imgToDecode.setThreaded(1); // Enable multi-threading
imgToDecode.DICIdecompress(); // Initiate decompression
imgToDecode.save("c:\image.png"); // Save the image
imgToDecode.view(); // Display the image
```

## Documentation
You can find the documentation on this link : https://github.com/Bralekor/DICI/blob/master/docs/manual.md

## Benchmark

Image source : https://data.csail.mit.edu/graphics/fivek/

The first 3000 images were recovered and converted to 24-bit bmp format.

The conversion to PNG and webp format was done with benchmark software that uses openCV, the conversions are done by keeping the default parameters and activating multi-threading (if available).

Benchmark : https://docs.google.com/spreadsheets/d/1oLSE99_0oJzJbufkOxKr5TmmTNkSFzWYMvTQCkUXeHY/edit?usp=sharing

The benchmark results show equivalent or even better compression than webp with much faster encoding and decoding speeds for DICI, other benchmarks show that the larger the image, the more efficient the algorithm is in terms of compression and speed

## 📜 License

Distributed under the Apache 2.0 License. See `LICENSE` for more information.

---
