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

#include "Decode.h"

#include <opencv2/opencv.hpp>

#include <vector>
#include <fstream>
#include <future>


using namespace std;

class DICIdecode
{

public:

    DICIdecode();
    DICIdecode(string fileIN, bool threaded);

    void DICIdecompress();
 

    void getDataFromFile(string fileName, vector<uint8_t>* data);
    void vectorAssemble(vector<vector<uint8_t>>* in, vector<uint8_t>* out);

    void view();
    void save(string fileOUT);

    void setFileIN(string fileIN);
    void setThreaded(bool threaded);



    cv::Mat* getMat();


private:

    string fileIN;


    bool threaded;


    cv::Mat ImageFinal;

    vector<uint8_t> data;
    vector<uint8_t> tmpDataFilter; // for 48 bits images
    vector <vector<uint8_t>> splitData; // vector which stores the different parts of compressed data
    vector<vector<uint8_t>> out; // vector which recovers the decompressed data from the different parts
    vector<future<void>> threads;
    vector<Decode> vecDecode;
    vector<uint8_t> dataFilter;
    vector<uint8_t> finalData;
    vector<uint16_t> dataFilter16;

};