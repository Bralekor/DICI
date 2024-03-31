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

#include <lzma/Lzmalib.h>
#include <ska/bytell_hash_map.hpp>

#include <vector>

using namespace std;

struct HashFct {

	size_t operator()(uint32_t value) const {

		return value;

	}
};

class Encode {
public:

	Encode(vector<uint8_t>* in, vector<uint8_t>* out, uint8_t bytePerValue);

	
	void CreateTablesFrequency(); // Creates a pixel frequency table
	void Compression();
	void Compress();


private:
	vector<uint8_t>* data;  // Pointer to data vector
	vector<uint32_t> groupedData; // Data separated based on bytePerValue
	
	vector<uint32_t> frequency_vector;  // Vector for value frequencies
	ska::bytell_hash_map<uint32_t, uint32_t, HashFct> frequency_hash_map; // The hashmap linked to it allowing you to find a value by its frequency classification


	vector<uint8_t> bitToRead;
	vector<uint8_t> dataCompressed;
	vector<uint8_t>* finalData;


	uint8_t bytePerValue;
	uint64_t sizeData;

};