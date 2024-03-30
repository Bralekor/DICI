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