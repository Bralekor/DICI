/*
 * DICI - Dictionnary Index for Compressed Image
 * Lossless File Format
 * 
 * Copyright 2023 Arnaud Recchia and Kévin Passemard
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