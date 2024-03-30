#pragma once

#include <lzma/Lzmalib.h>

#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

class Decode
{
public:
    Decode(vector<uint8_t>* in, vector<uint8_t>* out);

    void readCompressedFile();
    void Decompression();
    void SaveFile(const string& fileName);


private:
    vector<uint32_t> frequency_vector;
    vector<uint8_t> bitToRead;
    vector<uint8_t> dataCompressed;

    uint8_t bytePerValue;
    uint64_t sizeData;

    vector<uint8_t>* uncompressedData;
    vector<uint8_t>* dataIn;


};