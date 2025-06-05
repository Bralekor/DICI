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

#include <fstream>

#include "filter.h"
#include "encode.h"

using namespace std;

class DICIencode {

public:

	DICIencode();
	DICIencode(string fileIN, string fileOUT, bool threaded);
	bool DICIcompress();

	void function_splitData(vector <vector<uint8_t>>* splitData, span<uint8_t>* data, unsigned char nbSplit, uint8_t bytePerValue);

	void setFileIN(string fileIN);
	void setFileOUT(string fileOUT);
	void setThreaded(bool threaded);
	void clear();


private:

	span<uint8_t> data; // vector that we want to compress
	Filter image;

	vector <vector<uint8_t>> splitData;
	vector<vector<uint8_t>> out;
	vector<thread> threads;


	vector<Encode> vecEncode;

	string fileIN;
	string fileOUT;
	bool threaded;

};