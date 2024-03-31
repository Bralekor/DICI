
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

## Benchmark

Coming soon !

## 💡 Contributing

We warmly welcome contributions! If you're passionate about enhancing the DICI format or wish to introduce new features, we encourage you to submit pull requests or initiate discussions through issues.

## 📜 License

Distributed under the Apache 2.0 License. See `LICENSE` for more information.

---
