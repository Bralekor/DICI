#pragma once

#include <vector>
#include <string>
#include <thread>
#include <future>

using namespace std;

class Filter_reverse
{
public:

    Filter_reverse(vector<uint8_t>* in, vector<uint8_t>* out, uint32_t width, uint32_t height, uint32_t canals, vector<uint8_t>* typerFilter);
    Filter_reverse(vector<uint8_t>* in, vector<uint8_t>* out, uint32_t width, uint32_t height, uint32_t canals, vector<uint8_t>* typerFilter, bool thread);

    void Reverse();
    void ReverseVerySmall();

    void ApplyFilterWithNoThread();
    void ApplyFilterWithThread();
    void ApplyFilterWithThreadRight(uint8_t* curPos, uint32_t indexBlock, int32_t lenght);
    void ApplyFilterWithThreadDown(uint8_t* curPos, uint32_t indexBlock, int32_t lenght);


    void ApplyFilterOnBlock(uint8_t* curPos, uint8_t filter);
    void FilterBLock(uint8_t* curPos, uint8_t filter);
    void FilterCornerUpLeftBLock(uint8_t* curPos, uint8_t filter);
    void FilterUpBLock(uint8_t* curPos, uint8_t filter);
    void FilterLeftBLock(uint8_t* curPos, uint8_t filter);



private:
    vector<uint8_t>* data;
    vector<uint8_t> dataReverse;
    vector<uint8_t>* typeFilter;

    uint32_t width;
    uint32_t height;
    uint32_t canals;

    uint32_t blockSize;

    uint32_t indexBlock;

    uint32_t moduloBlockWidth;
    uint32_t moduloBlockHeight;

    int32_t nbBlockWidth;
    int32_t nbBlockHeight;

    vector<uint8_t*> endBlock;

    bool withThread;

};