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

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <vector>
#include <span>
#include <future>


using namespace std;

class Filter {
public:

	Filter();
	Filter(string fileName);
	Filter(string fileName, bool thread);
	void Apply();
	void Apply16(bool part);

	//Applying filters for a given block
	void ApplyFilterWithThread();

	void ApplyFilterWithThreadLeft(uint8_t* curPos, uint32_t indexBlock, int32_t lenght);
	void ApplyFilterWithThreadUp(uint8_t* curPos, uint32_t indexBlock, int32_t lenght);

	void ApplyFilterWithNoThread();

	void ApplyFilterOnBlock(uint8_t* curPos, uint8_t filter);

	void FilterBLock(uint8_t* curPos, const uint8_t filter);
	void FilterCornerUpLeftBLock(uint8_t* curPos, uint8_t filter);
	void FilterUpBLock(uint8_t* curPos, uint8_t filter);
	void FilterLeftBLock(uint8_t* curPos, uint8_t filter);

	
	//Calculation of the best filters for a given block
	void StartSearchWithThread();
	void StartSearchWithNoThread();

	uint8_t SearchFilterBLock(uint8_t* curPos);
	uint8_t SearchFilterCornerUpLeftBLock(uint8_t* curPos);
	uint8_t SearchFilterUpBLock(uint8_t* curPos);
	uint8_t SearchFilterLeftBLock(uint8_t* curPos);

	vector<uint8_t>* getTypeFilter();
	uint32_t getWidth();
	uint32_t getHeight();
	uint32_t getCanals();
	uint32_t getBitPerChannel();
	cv::Mat* getMat();

	bool withThread;


private:

	cv::Mat Image;

	// for 16 bits per channels
	cv::Mat Part1; 
	cv::Mat Part2;

	vector<vector<uint8_t>> pixels;
	vector<vector<uint8_t>> pixelsWithFilter;

	vector<uint8_t> typeFilter;
	vector<uint8_t*> endBlock; //contains the pointer of the last pixel of each block

	uint32_t width;
	uint32_t height;
	uint32_t canals;
	uint32_t bitPerChannel;

	uint32_t blockSize;

	uint32_t indexBlock;

	uint32_t moduloBlockWidth;
	uint32_t moduloBlockHeight;

	int32_t nbBlockWidth;
	int32_t nbBlockHeight;

	uint32_t nbAssembleBlock;

	uint32_t lastFilter;

};