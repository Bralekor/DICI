#pragma once

#include <fstream>

#include "filter.h"
#include "Encode.h"

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