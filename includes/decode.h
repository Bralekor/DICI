/*
 * DICI - Dictionnary Index for Compressed Image
 * Lossless File Format
 * 
 * Copyright 2024 Arnaud Recchia and Kévin Passemard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <lzma/LzmaLib.h>

#include <cstdint>
#include <cmath>
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