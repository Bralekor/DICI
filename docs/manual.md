# File Header:

**DICI Signature**: 4 bytes for "DICI" in ASCII (68 73 67 73).

**Algorithm Version**: 4 bytes.

**Image Dimensions**:
- Width: 4 bytes
- Height: 4 bytes

**Color Information**:
- Number of Channels: 4 bytes
- Number of Bits per Channel: 4 bytes

## Algorithm Operation

The first step is to apply a filter on the different colorimetric values of the image, which are then combined into a single value per pixel.

![img4](https://github.com/Bralekor/DICI/assets/53519025/7e58df05-0ad0-4699-82be-e1a830454d13)

A frequency table of each colorimetric value is then created. This table is sorted by frequency order (the most frequent value being first).

| Index | Chromatic Value                | Frequency |
|-------|--------------------------------|-----------|
| 0     | 00000000 00000000 00000000     | 0.172545  |
| 1     | 00110100 11111100 01001010     | 0.121223  |
| 2     | 01000101 10000011 11111010     | 0.075659  |
| 3     | 01110010 01001001 10000101     | 0.048765  |
| 4     | 10010110 00100000 11000000     | 0.040456  |
| 5     | 00001100 01000110 00100100     | 0.038987  |
| ...   | ...                            | ...       |

A hashmap is then created containing a key/value pair of the pixel's colorimetric value and its position index in the frequency table. This step allows for quicker retrieval of the frequency order relative to a pixel's colorimetric value. Each pixel of the image is then replaced by its position index within the frequency table.

To allow for efficient compression at low cost, the data will then be written within a file, bypassing the classic rules of data writing in bytes. Indeed, all numbers will be represented by two distinct bit code: the first being the number itself after optimization (`optValue`), and the second being the number of bits used in writing this number (`nbBit`).

`nbBit = log2(x + 2)`

`optValue = (x + 2) - 2^nbDigit`

`x`: Value to optimize
`nbBit`: Number of bits the value will take in memory
`optValue`: Optimized value

Since the size of the data is variable, the sequences 0 and 00 will represent two distinct values. This method allows values to be written with the least necessary bits. The table below represents this optimization, showing that the numbers 4 and 5 use 2 bits instead of 3, and the values from 8 to 12 move from 4 bits to 3.

| Base 10 Number | Base 2 Number | Optimization | Number of Bits to Read (Base 2) |
|----------------|---------------|--------------|---------------------------------|
| 0              | 0             | 0            | 001                             |
| 1              | 1             | 1            | 001                             |
| 2              | 10            | 00           | 010                             |
| 3              | 11            | 01           | 010                             |
| 4              | 100           | 10           | 010                             |
| 5              | 101           | 11           | 010                             |
| 6              | 110           | 000          | 011                             |
| 7              | 111           | 001          | 011                             |
| 8              | 1000          | 010          | 011                             |
| 9              | 1001          | 011          | 011                             |
| 10             | 1010          | 100          | 011                             |
| 11             | 1011          | 101          | 011                             |
| 12             | 1100          | 110          | 011                             |
| ...            | ...           | ...          | ...                             |


The image below shows how to obtain the transformation of a value using this method.

![img1](https://github.com/Bralekor/DICI/assets/53519025/0f6c7069-e97d-43c8-8571-feed59f57a8a)

Three tables will be stored when writing the file. The first represents the list of chromatic values sorted by frequency. The second is the index (in optimized value) to the previous table for each pixel. The last is the bit size of each element in the second table.

Table 1 : List of chromatic values sorted by frequency

Table 2 : For each pixel, the index (in optimized value) to table 1

Table 3 : Bit size of each element in the second table

The tables can be compressed using another compression algorithm before being saved to further reduce the file size. However, it's noted that only the first and third tables have sufficiently low entropy for the compression to have a significant impact on the file size.

Two optimizations are performed on the third table (indicating the number of bits to read). Thanks to table 1, we can retrieve the number of bits needed by the last value (the least frequent, thus the maximum value) and thus reduce the number of bits inserted into table 3; for example, if the last value needs a maximum of 5 bits, each value will be inserted every 5 bits in the third table. The second optimization consists of checking if a value on these 3 bits is not used (and is therefore available); knowing that on 3 bits it is possible to enter numbers from 1 to 8, if no number needs 8 bits, it can be used. This free value will be used to indicate that it will be the most frequent value; thus, there is no need to insert it in the second value table, which also allows to shift the first table because the most frequent value no longer needs a representation.

![img2](https://github.com/Bralekor/DICI/assets/53519025/0c857714-baeb-4738-942b-d94b8c95862c)

During decompression, the process is reversed but is significantly less complex. The different tables are retrieved. The algorithm will decompress the second table (value of indexes) thanks to the third (number of bits per data). Then, the indexes will be replaced by the chromatic values combined thanks to the first table. Once this step is finished, the chromatic values will be recut and then reinserted to form the image.

![img3](https://github.com/Bralekor/DICI/assets/53519025/f4bb79d0-dc0f-46e8-9fb1-70c19802a757)
