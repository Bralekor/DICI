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

#include "Encode.h"


using namespace std;


static inline uint32_t ilog2(float x)
{
	uint32_t ix = (uint32_t&)x;
	uint32_t exp = (ix >> 23) & 0xFF;
	int32_t log2 = int32_t(exp) - 127;

	return log2;
}


void Compress_lzma(vector<unsigned char>& outBuf, vector<unsigned char>& inBuf)
{
	size_t propsSize = LZMA_PROPS_SIZE;
	size_t destLen = inBuf.size() + inBuf.size() / 3 + 128;
	outBuf.resize(propsSize + destLen);

	int res = LzmaCompress(&outBuf[LZMA_PROPS_SIZE], &destLen, &inBuf[0], inBuf.size(), &outBuf[0], &propsSize, -1, 0, -1, -1, -1, -1, -1);

	outBuf.resize(propsSize + destLen);
}


void sortRadix(vector<uint64_t>& vec, uint8_t bitToIgnore)
{
	const size_t base = 256; 
	vector<uint64_t> temp(vec.size());
	array<int, base> count;
	bool useTempAsSource = false;

	// Loop through each byte of the number, ignoring the last bits specified by bitToIgnore
	for (size_t digit = 0; digit < sizeof(uint64_t) - bitToIgnore; ++digit) 
	{
		fill(count.begin(), count.end(), 0);

		size_t shift = digit * 8;

		for (uint64_t num : vec) {
			size_t index = (num >> shift) & 0xFF;
			count[index]++;
		}

		for (size_t i = 1; i < base; ++i) {
			count[i] += count[i - 1];
		}

		for (int i = vec.size() - 1; i >= 0; --i) {
			size_t index = ((useTempAsSource ? temp[i] : vec[i]) >> shift) & 0xFF;
			if (useTempAsSource) {
				vec[--count[index]] = temp[i];
			}
			else {
				temp[--count[index]] = vec[i];
			}
		}

		useTempAsSource = !useTempAsSource;
	}

	if (useTempAsSource) {
		copy(temp.begin(), temp.end(), vec.begin());
	}
}


Encode::Encode(vector<uint8_t>* data, vector<uint8_t>* out, uint8_t bytePerValue)
{
	this->data = data;
	finalData = out;
	this->bytePerValue = bytePerValue;
	sizeData = data->size();

}


void Encode::CreateTablesFrequency()
{
	

	
	// Groups bytes into a vector<uint32_t> based on bytePerValue
	if (bytePerValue == 1)
	{
		groupedData.resize(data->size());
		vector<uint32_t>::iterator itGroupedData = groupedData.begin();

		for (auto it = data->cbegin(); it != data->cend(); it += 1)
		{
			*itGroupedData = *it;
			itGroupedData++;
		}


	}
	else if (bytePerValue == 2)
	{
		groupedData.resize(data->size() / 2);
		vector<uint32_t>::iterator itGroupedData = groupedData.begin();

		for (auto it = data->cbegin(); it < data->cend(); it += 2)
		{
			*itGroupedData = (*it) << 8 | (*(it + 1));
			itGroupedData++;

		}
	}
	else if (bytePerValue == 3)
	{

		groupedData.resize(data->size() / 3);

		vector<uint32_t>::iterator itGroupedData = groupedData.begin();

		for (auto it = data->cbegin(); it < data->cend(); it += 3)
		{
			*itGroupedData = ((*it) << 16 | (*(it + 1)) << 8 | (*(it + 2)));
			itGroupedData++;
		}

	}
	else if (bytePerValue == 4)
	{

		groupedData.resize(data->size() / 4);
		vector<uint32_t>::iterator itGroupedData = groupedData.begin();

		for (auto it = data->cbegin(); it < data->cend(); it += 4)
		{
			*itGroupedData = (*it) << 24 | (*(it + 1)) << 16 | (*(it + 2)) << 8 | (*(it + 3));
			itGroupedData++;
		}

	}

	// Process the remaining data in case it is not a multiple of bytePerValue
	uint32_t value = 0;
	int remainingData = data->size() % bytePerValue;


	if (remainingData != 0)
	{
		for (int a = remainingData; a != 0; a--)
		{
			value = value << 8;
			value += (*data)[data->size() - a];


		}
		value = value << (8 * (4 - remainingData));
		groupedData.emplace_back(value);
	}



	// Create a hashmap which gives the frequency of appearance of a value
	frequency_hash_map = ska::bytell_hash_map<uint32_t, uint32_t, HashFct>(0);
	frequency_hash_map.max_load_factor(0.8);

	for (int value : groupedData)
		frequency_hash_map[value]++;


	vector<uint64_t> sorted;
	sorted.reserve(frequency_hash_map.size());

	uint32_t maxFreq = 0;


	for (auto& it : frequency_hash_map)
	{
		
		// The loss of precision allows better compression of the frequency table and potentially (depends on maxFreq) to speed up sorting
		it.second >>= 2;

		// Finds the highest frequency value
		if (it.second > maxFreq)
		{
			maxFreq = it.second;
		}


		// Stores in 32 bits the frequency of appearance of the value and in the remaining 32 bits the value itself
		uint64_t tmp = ~(it.second); // Inversion of the bits so that the sorting is done in the other direction for the part which stores the frequency
		sorted.emplace_back(((uint64_t)tmp << ( (8 * bytePerValue) )  ) | (it.first));

	}



	
	// Defines the number of bytes to ignore for radix sorting in order to speed up sorting
	uint8_t bitToIgnore = (4 - bytePerValue);

	if (maxFreq > 16777215)
	{

	}
	else if (maxFreq > 65535)
	{
		bitToIgnore++;
	}
	else if (maxFreq > 255)
	{
		bitToIgnore += 2;
	}
	else {
		bitToIgnore += 3;
	}

	
	
	// Sort the table
	sortRadix(sorted, bitToIgnore); 


	// Stores the results in 2 tables, one which allows you to retrieve the value using the frequency
	// and the other table (hashmap) allows you to recover the frequency using the value
	uint32_t i = 1;
	frequency_vector.reserve(sorted.size());
	frequency_hash_map.reserve(sorted.size());


	uint32_t mask = ((uint64_t)1 << (8 * bytePerValue)) - 1;

	for (auto it = sorted.begin(); it != sorted.end(); it++)
	{
		frequency_vector.emplace_back((*it) & mask);
		frequency_hash_map[(*it) & mask] = i;
		i++;
	}




}


void Encode::Compression()
{



	uint32_t PosValue;
	uint32_t valBitToRead;

	// Variables allowing you to put the bits one after the other
	uint16_t currentValue = 0; // for bitToRead
	int bitsInCurrentValue = 0;
	uint64_t currentValue_2 = 0; // for dataCompressed
	int bitsInCurrentValue_2 = 0;

	uint8_t numberOfDigit; // Number of digits to insert for the vector bitToRead
	uint8_t maxValue; // The maximum possible value

	uint32_t mostFrequentNum = frequency_vector[0]; // We determine the most frequent value (Allows optimization)

	// We determine how many bits are necessary to store the bit values ​​to be read

	if (frequency_vector.size() < pow(2, 8) * 2 - 2)
	{
		numberOfDigit = 3;
		maxValue = 8;

	}
	else if (frequency_vector.size() < pow(2, 16) * 2 - 2) {

		numberOfDigit = 4;
		maxValue = 16;

	}
	else if (frequency_vector.size() < pow(2, 32) * 2 - 2) {

		numberOfDigit = 5;
		maxValue = 32;

	}
	else if (frequency_vector.size() < pow(2, 64) * 2 - 2) {

		numberOfDigit = 6;
		maxValue = 64;

	}

	uint32_t nbValue = floor(ilog2(frequency_vector.size() + 2));


	// We check if a value of numberOfDigit is available to allow optimization
	if (nbValue >= maxValue) // if it's not the case
	{
		numberOfDigit++;
		maxValue *= 2;
	}



	const uint8_t nbDigit = numberOfDigit;

	bitToRead.resize(groupedData.size() * (nbDigit / 8.0));
	dataCompressed.reserve(sizeData / 3);

	vector<uint8_t>::iterator itBitToRead = bitToRead.begin();



	uint32_t* ptrLastValue = &(frequency_hash_map[0]);
	uint32_t lastValue = 0;



	

	for (const auto& value : groupedData)
	{

		PosValue = frequency_hash_map[value]; // We recover its position in the frequency vector using the hashmap

		valBitToRead = ilog2(PosValue);

		// Partie bitToRead
		currentValue = (currentValue << nbDigit) | valBitToRead;  // Shift and add value


		bitsInCurrentValue += nbDigit;
		while (bitsInCurrentValue >= 8)
		{
			*itBitToRead = currentValue >> (bitsInCurrentValue - 8);
			itBitToRead++;
			bitsInCurrentValue -= 8;

		}

		// DataCompressed part
		if (mostFrequentNum != value)
		{

			currentValue_2 = (currentValue_2 << valBitToRead);
			currentValue_2 += (PosValue - (1 << valBitToRead));  // Shift and add value
			bitsInCurrentValue_2 += valBitToRead;

			while (bitsInCurrentValue_2 >= 8)
			{
				bitsInCurrentValue_2 -= 8;
				dataCompressed.emplace_back(currentValue_2 >> bitsInCurrentValue_2);
			}
		}



	}

	


	// Add the last byte if there are bits left for bitToRead
	if (bitsInCurrentValue > 0)
	{
		bitToRead.emplace_back(currentValue << (8 - bitsInCurrentValue));
	}

	// Add the last byte if there are bits left for dataCompressed
	if (bitsInCurrentValue_2 > 0)
	{
		dataCompressed.emplace_back(currentValue_2 << (8 - bitsInCurrentValue_2));
	}



	// Just before recording we apply a filter on the frequency vector to obtain better compression
	for (int a = frequency_vector.size() - 1; a >= 1; a--)
	{
		frequency_vector[a] -= frequency_vector[a - 1];
	}



	vector<uint8_t> frequency_vector_char; // for lzma compression
	frequency_vector_char.reserve(frequency_vector.size() * sizeof(uint32_t));

	for (uint32_t x : frequency_vector)
	{
		uint8_t bytes[sizeof(uint32_t)];
		memcpy(bytes, &x, sizeof(uint32_t));

		// Add each byte to the 8-bit vector
		for (uint8_t b : bytes)
		{
			frequency_vector_char.push_back(b);
		}
	}




	// We do lzma compression on bitToRead and frequency_vector
	vector<uint8_t> bitToRead_compressed;
	vector<uint8_t> frequency_vector_compressed;

	Compress_lzma(bitToRead_compressed, bitToRead);
	Compress_lzma(frequency_vector_compressed, frequency_vector_char);

	uint32_t size_frequency_vector_compressed = frequency_vector_compressed.size();
	uint32_t size_bitToRead_compressed = bitToRead_compressed.size();
	uint32_t size_dataCompressed = dataCompressed.size();

	uint32_t size_frequency_vector = frequency_vector_char.size();
	uint32_t size_bitToRead = bitToRead.size();


	finalData->resize(size_frequency_vector_compressed + size_bitToRead_compressed + size_dataCompressed + 29);

	memcpy(finalData->data(), &size_frequency_vector_compressed, sizeof(uint32_t));
	memcpy(finalData->data() + 4, &size_bitToRead_compressed, sizeof(uint32_t));
	memcpy(finalData->data() + 8, &size_dataCompressed, sizeof(uint32_t));

	memcpy(finalData->data() + 12, &size_frequency_vector, sizeof(uint32_t));
	memcpy(finalData->data() + 16, &size_bitToRead, sizeof(uint32_t));

	memcpy(finalData->data() + 20, &bytePerValue, sizeof(uint8_t));
	memcpy(finalData->data() + 21, &sizeData, sizeof(uint64_t));

	memcpy(finalData->data() + 29, (frequency_vector_compressed.data()), size_frequency_vector_compressed);
	memcpy(finalData->data() + 29 + size_frequency_vector_compressed, (bitToRead_compressed.data()), size_bitToRead_compressed);
	memcpy(finalData->data() + 29 + size_frequency_vector_compressed + size_bitToRead_compressed, (dataCompressed.data()), size_dataCompressed);

}


void Encode::Compress()
{
	CreateTablesFrequency();
	Compression();
}

