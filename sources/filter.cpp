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

#include "filter.h"

using namespace std;

inline uint8_t Average2(uint8_t a, uint8_t b)
{
	return (a + b) >> 1;
}

inline int32_t Clamp(int32_t a)
{
	return (a < 0) ? 0 : (a > 255) ? 255 : a;
}

inline int32_t ClampAddSubtractFull(int32_t a, int32_t b, int32_t c)
{
	return Clamp(a + b - c);
}

inline uint8_t SemiPaeth5(uint8_t W, uint8_t N, uint8_t NW)
{
	int32_t pred = (5 * N + 5 * W - (NW << 1) + 3) >> 3;
	pred = Clamp(pred);
	return pred;
}

inline uint8_t SemiPaeth23(uint8_t W, uint8_t N, uint8_t NW)
{
	int32_t pred = ((N << 1) + 3 * W - NW + 1) >> 2;
	pred = Clamp(pred);
	return pred;
}

inline uint8_t SkewedGradientFilter(uint8_t a, uint8_t b, uint8_t c)
{
	int32_t pred = (3 * (b + a) - (c << 1)) >> 2;

	if (pred >= 255)
		return 255;

	if (pred <= 0)
		return 0;

	return pred;
}

Filter::Filter()
{

}

Filter::Filter(string fileName)
{

	Image = cv::imread(fileName, cv::IMREAD_UNCHANGED);

	if (Image.empty() || Image.rows <= 0 || Image.cols <= 0) {
		cout << "ERROR : The file is not an image or could not be loaded correctly. [" + fileName + "]" << endl;
		return;
	}


	width = Image.cols;
	height = Image.rows;
	canals = Image.channels();

	withThread = 1;

}

Filter::Filter(string fileName, bool thread)
{

	streambuf* original_stderr = cerr.rdbuf();
	cerr.rdbuf(nullptr);

	Image = cv::imread(fileName, cv::IMREAD_UNCHANGED);

	if (Image.empty() || Image.rows <= 0 || Image.cols <= 0) {
		cout << "ERROR : The file is not an image or could not be loaded correctly. [" + fileName + "]" << endl;
		return;
	}

	cerr.rdbuf(original_stderr);

	width = Image.cols;
	height = Image.rows;
	canals = Image.channels();


	if (Image.depth() == 0)
	{
		bitPerChannel = 8;

	}
	else if (Image.depth() == 2)
	{
		Part1 = cv::Mat(height, width, CV_8UC3);
		Part2 = cv::Mat(height, width, CV_8UC3);


		uint16_t* ptrImage = Image.ptr<uint16_t>();
		uint8_t* ptrPart1 = Part1.ptr<uint8_t>();
		uint8_t* ptrPart2 = Part2.ptr<uint8_t>();

		for (int a = 0; a < width * height * canals; a++)
		{
			
			*ptrPart1 = ((*ptrImage) & 65280) >> 8;
			*ptrPart2 = ((*ptrImage) & 255) ;
			ptrImage++;
			ptrPart1++;
			ptrPart2++;
		}

		bitPerChannel = 16;
	}

	width = Image.cols;
	height = Image.rows;
	canals = Image.channels();


	withThread = thread;

}

void Filter::Apply()
{
	blockSize = 48; // Each block is 48x48

	// We count the number of blocks 48x48 pixels
	nbBlockWidth = ceil((float)width / blockSize);
	nbBlockHeight = ceil((float)height / blockSize);

	uint8_t* startPixel = Image.ptr<uint8_t>(); // pointer to first pixel
	uint8_t* endPixel = startPixel + (width * height * canals) - 1; // pointer to last pixel
	 
	endBlock.resize(nbBlockWidth * nbBlockHeight);

	moduloBlockWidth = (width % blockSize);
	moduloBlockHeight = (height % blockSize);

	if (moduloBlockWidth == 0)
	{
		moduloBlockWidth = blockSize;
	}

	if (moduloBlockHeight == 0)
	{
		moduloBlockHeight = blockSize;
	}

	uint8_t* curseurPtr = endPixel;
	indexBlock = endBlock.size() - 1;
	
	// We get the last pixel of each block
	for (int a = 0; a < nbBlockHeight; a++)
	{
		for (int b = 0; b < nbBlockWidth; b++)
		{


			endBlock[indexBlock] = curseurPtr;
			curseurPtr = curseurPtr - (blockSize * canals);

			indexBlock--;

		}
		curseurPtr -= (width * (blockSize - 1) * canals) - ((blockSize - moduloBlockWidth) * canals);
	}



	
	// Calculation of the best filter
	typeFilter.resize(endBlock.size()); // Allows you to store which filter is used on each block
	
	if (withThread == 0)
	{
		StartSearchWithNoThread();
	}
	else {
		StartSearchWithThread();
	}




	
	// Applying the best filter
	indexBlock = endBlock.size() - 1;

	if (withThread == 0)
	{
	
		ApplyFilterWithNoThread();

	}
	else {
		ApplyFilterWithThread();

	}





}

void Filter::Apply16(bool part)
{
	if (part == false)
	{
		Image = Part1;

	}
	else {

		Image = Part2;
	}

	blockSize = 48;

	nbBlockWidth = ceil((float)width / blockSize);
	nbBlockHeight = ceil((float)height / blockSize);


	uint8_t* startPixel = Image.ptr<uint8_t>();
	uint8_t* endPixel = startPixel + (width * height * canals) - 1;

	endBlock.resize(nbBlockWidth * nbBlockHeight);

	moduloBlockWidth = (width % blockSize);
	moduloBlockHeight = (height % blockSize);

	if (moduloBlockWidth == 0)
	{
		moduloBlockWidth = blockSize;
	}

	if (moduloBlockHeight == 0)
	{
		moduloBlockHeight = blockSize;
	}


	uint8_t* curseurPtr = endPixel;
	indexBlock = endBlock.size() - 1; 

	// On r�cup�re le dernier pixel de chaque blocs
	for (int a = 0; a < nbBlockHeight; a++)
	{
		for (int b = 0; b < nbBlockWidth; b++)
		{


			endBlock[indexBlock] = curseurPtr;
			curseurPtr = curseurPtr - (blockSize * canals);

			indexBlock--;

		}
		curseurPtr -= (width * (blockSize - 1) * canals) - ((blockSize - moduloBlockWidth) * canals);
	}



	typeFilter.resize(endBlock.size()); 
	if (withThread == 0)
	{
		StartSearchWithNoThread();
	}
	else {
		StartSearchWithThread();
	}



	indexBlock = endBlock.size() - 1;
	if (withThread == 0)
	{

		ApplyFilterWithNoThread();

	}
	else {
		ApplyFilterWithThread();

	}

}

void Filter::ApplyFilterWithThread()
{
	

	indexBlock = endBlock.size() - 1;

	uint32_t indexBlock2 = indexBlock;


	uint32_t numThread = 0;
	uint32_t remaining = 0;

	if (nbBlockWidth >= nbBlockHeight)
	{

		vector<future<void>> futures(nbBlockWidth);

		for (int a = 0; a < nbBlockHeight - 1; a++)
		{
			remaining++;

			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter::FilterBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
				numThread++;

				indexBlock2 -= nbBlockWidth - 1;
			}

			indexBlock -= 1;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}
		
		for (int a = 0; a < nbBlockWidth - nbBlockHeight; a++)
		{
			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter::FilterBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
				numThread++;
				indexBlock2 -= nbBlockWidth - 1;
			}

			futures[numThread] = (async(launch::async, &Filter::FilterUpBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
			numThread++;


			indexBlock -= 1;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;

		}

		for (int a = 0; a < nbBlockHeight - 1; a++)
		{
			futures[numThread] = (async(launch::async, &Filter::FilterLeftBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
			numThread++;
			indexBlock2 -= nbBlockWidth - 1;

			remaining--;

			for (int b = 0; b < remaining; b++)
			{

				futures[numThread] = (async(launch::async, &Filter::FilterBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
				numThread++;

				indexBlock2 -= nbBlockWidth - 1;
			}

			futures[numThread] = (async(launch::async, &Filter::FilterUpBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
			numThread++;

			indexBlock -= nbBlockWidth;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;
		}


		FilterCornerUpLeftBLock(endBlock[indexBlock2], typeFilter[indexBlock2]);
	
	}
	else {

		vector<future<void>> futures(nbBlockHeight);

		for (int a = 0; a < nbBlockWidth - 1; a++)
		{
			remaining++;

			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter::FilterBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
				numThread++;

				indexBlock2 -= nbBlockWidth - 1;
			}

			indexBlock -= 1;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}
		
		
		remaining++;

		for (int a = 0; a < nbBlockHeight - nbBlockWidth; a++)
		{
			futures[numThread] = (async(launch::async, &Filter::FilterLeftBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
			numThread++;
			indexBlock2 -= nbBlockWidth - 1;

			for (int b = 0; b < remaining - 1; b++)
			{
				futures[numThread] = (async(launch::async, &Filter::FilterBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
				numThread++;
				indexBlock2 -= nbBlockWidth - 1;
			}



			indexBlock -= nbBlockWidth;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;

		}

		remaining--;

		for (int a = 0; a < nbBlockHeight - (nbBlockHeight - nbBlockWidth) - 1; a++)
		{
			futures[numThread] = (async(launch::async, &Filter::FilterLeftBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
			numThread++;
			indexBlock2 -= nbBlockWidth - 1;

			remaining--;

			for (int b = 0; b < remaining; b++)
			{

				futures[numThread] = (async(launch::async, &Filter::FilterBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
				numThread++;

				indexBlock2 -= nbBlockWidth - 1;
			}

			futures[numThread] = (async(launch::async, &Filter::FilterUpBLock, this, endBlock[indexBlock2], typeFilter[indexBlock2]));
			numThread++;

			indexBlock -= nbBlockWidth;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;
		}


		FilterCornerUpLeftBLock(endBlock[indexBlock2], typeFilter[indexBlock2]);



	}


}

void Filter::ApplyFilterWithThreadLeft(uint8_t* curPos, uint32_t indexBlock, int32_t lenght)
{
	for (int a = 0; a < lenght - 1; a++)
	{
		curPos -= (blockSize * canals);
		indexBlock--;

		FilterBLock(curPos, typeFilter[indexBlock]);

	}

	curPos -= (blockSize * canals);
	indexBlock--;

	FilterLeftBLock(curPos, typeFilter[indexBlock]);

}

void Filter::ApplyFilterWithThreadUp(uint8_t* curPos, uint32_t indexBlock, int32_t lenght)
{

	for (int a = 0; a < lenght - 1; a++)
	{
		curPos -= (width * blockSize * canals);
		indexBlock -= nbBlockWidth ;



		FilterBLock(curPos, typeFilter[indexBlock]);


	}
	
	curPos -= (width * blockSize * canals);
	indexBlock -= nbBlockWidth ;

	FilterUpBLock(curPos, typeFilter[indexBlock]);

}

void Filter::ApplyFilterWithNoThread()
{

	indexBlock = endBlock.size() - 1; // On remet l'index sur le dernier block

	// on ne prend pas le premier et dernier block de la ligne car il faut appeller une fonction sp�cifique pour eux
	for (int a = 0; a < nbBlockHeight - 1; a++) 
	{
		for (int b = 0; b < nbBlockWidth - 1; b++)
		{
			FilterBLock(endBlock[indexBlock], typeFilter[indexBlock]);
			indexBlock--;
		}
	
		FilterLeftBLock(endBlock[indexBlock], typeFilter[indexBlock]);
		indexBlock--;
		
	}

	
	for (int b = 0; b < nbBlockWidth - 1; b++)
	{
		
		FilterUpBLock(endBlock[indexBlock], typeFilter[indexBlock]);
		indexBlock--;

	}

	FilterCornerUpLeftBLock(endBlock[indexBlock], typeFilter[indexBlock]);

	
}


void Filter::ApplyFilterOnBlock(uint8_t* curPos, uint8_t filter)
{
	uint8_t* pixLeft = curPos - canals;
	uint8_t* pixUp = curPos - (width * canals);
	uint8_t* pixUpLeft = curPos - (width * canals) - canals;


	switch (filter)
	{
	case 0:
		*curPos -= Average2(*pixLeft, *pixUp);
		
		break;
	case 1:
		*curPos -= *pixUp;
		break;
	case 2:
		*curPos -= *pixUpLeft;
		break;
	case 3:
		*curPos -= *pixLeft;
		break;
	case 4:
		*curPos -= ClampAddSubtractFull(*pixLeft, *pixUp, *pixUpLeft);
		break;
	case 5:
		*curPos -= SemiPaeth23(*pixLeft, *pixUp, *pixUpLeft);
		break;
	case 6:
		*curPos -= SemiPaeth5(*pixLeft, *pixUp, *pixUpLeft);
		
		break;
	case 7:
		*curPos -= SkewedGradientFilter(*pixLeft, *pixUp, *pixUpLeft);
		break;
	case 8:
		*curPos -= Average2(*pixUp, *pixUpLeft);
		break;
	case 9:
		//Aucun filtre
		break;
	}
}

void Filter::FilterBLock(uint8_t* curPos, const uint8_t filter)
{

	
	for (int a = 0; a < blockSize; a++)
	{
		for (int b = 0; b < blockSize; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);

				curPos--;
			}


		}
		curPos -= (width * canals) - (blockSize * canals);

	}
	

}

void Filter::FilterCornerUpLeftBLock(uint8_t* curPos, uint8_t filter)
{


	for (int a = 0; a < moduloBlockHeight - 1; a++)
	{
		// we ignore the pixels to the left of the block
		for (int b = 0; b < moduloBlockWidth - 1; b++) 
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);

				curPos--;
			}
		}
		curPos -= (width * canals) - ((moduloBlockWidth - 1) * canals);

	}

}

void Filter::FilterUpBLock(uint8_t* curPos, uint8_t filter)
{

	for (int a = 0; a < moduloBlockHeight - 1; a++) // -1 because we ignore the top line
	{
		for (int b = 0; b < blockSize; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);

				curPos--;
			}
		}
		curPos -= (width * canals) - ((blockSize)*canals);

	}

}

void Filter::FilterLeftBLock(uint8_t* curPos, uint8_t filter)
{

	for (int a = 0; a < blockSize; a++)
	{
		for (int b = 0; b < moduloBlockWidth - 1; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);

				curPos--;
			}
		}
		curPos -= (width * canals) - ((moduloBlockWidth - 1) * canals);

	}

}



void Filter::StartSearchWithNoThread()
{
	//Finds the most effective filter for each block

	indexBlock = endBlock.size() - 1; // We put the index back on the last block

	// we do not take the first and last block of the line because we must call a specific function for them
	for (int a = 0; a < nbBlockHeight - 1; a++) 
	{
		for (int b = 0; b < nbBlockWidth - 1; b++)
		{
			typeFilter[indexBlock] = SearchFilterBLock(endBlock[indexBlock]);
			
			indexBlock--;
		}



		typeFilter[indexBlock] = SearchFilterLeftBLock(endBlock[indexBlock]);
		indexBlock--;
	}


	// Specific function for the last blocks
	for (int b = 0; b < nbBlockWidth - 1; b++)
	{
		typeFilter[indexBlock] = SearchFilterUpBLock(endBlock[indexBlock]);
		indexBlock--;
	}

	typeFilter[indexBlock] = SearchFilterCornerUpLeftBLock(endBlock[indexBlock]);

}

void Filter::StartSearchWithThread()
{

	indexBlock = endBlock.size() - 1; // Return the index to the last block

	vector<future<uint8_t>> futures;

	// we do not take the first and last block of the line because we must call a specific function for them
	for (int a = 0; a < nbBlockHeight - 1; a++) 
	{
		for (int b = 0; b < nbBlockWidth - 1; b++)
		{
			futures.emplace_back(async(launch::async, &Filter::SearchFilterBLock, this, endBlock[indexBlock]));
			indexBlock--;
		}



		futures.emplace_back(async(launch::async , &Filter::SearchFilterLeftBLock, this, endBlock[indexBlock]));
		indexBlock--;
	}



	// Specific function for the last blocks
	for (int b = 0; b < nbBlockWidth - 1; b++)
	{
		futures.emplace_back(async(launch::async , &Filter::SearchFilterUpBLock, this, endBlock[indexBlock]));
		indexBlock--;
	}

	futures.emplace_back(async(launch::async , &Filter::SearchFilterCornerUpLeftBLock, this, endBlock[indexBlock]));

	indexBlock = endBlock.size() - 1;

	for (auto& t : futures)
	{
		typeFilter[indexBlock] = t.get();
		indexBlock--;
	}


}

uint8_t Filter::SearchFilterBLock(uint8_t* curPos)
{
	vector<uint32_t> totalPixel(10);

	uint8_t* pixLeft = curPos - canals;
	uint8_t* pixUp = curPos - (width * canals);
	uint8_t* pixUpLeft = curPos - (width * canals) - canals;


	for (int a = 0; a < blockSize; a++)
	{
		for (int b = 0; b < blockSize; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				totalPixel[0] += abs((*curPos - Average2(*pixLeft, *pixUp)));
				totalPixel[1] += abs((*curPos - *pixUp));
				totalPixel[2] += abs((*curPos - *pixUpLeft));
				totalPixel[3] += (abs((*curPos - *pixLeft)));
				totalPixel[4] += abs((*curPos - ClampAddSubtractFull(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[5] += abs((*curPos - SemiPaeth23(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[6] += abs((*curPos - SemiPaeth5(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[7] += abs((*curPos - SkewedGradientFilter(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[8] += abs((*curPos - Average2(*pixUp, *pixUpLeft)));
				totalPixel[9] += abs(*curPos);
				curPos--;
				pixLeft--;
				pixUp--;
				pixUpLeft--;
			}
		}
		curPos -= (width * canals) - (blockSize * canals);
		pixLeft = curPos - canals;
		pixUp = curPos - (width * canals);
		pixUpLeft = curPos - (width * canals) - canals;

	}


	uint32_t min_val = totalPixel[0];
	size_t min_index = 0;



	for (size_t i = 1; i < totalPixel.size(); ++i)
	{

		if (totalPixel[i] < min_val)
		{
			min_val = totalPixel[i];
			min_index = i;
		}
	}

	
	return min_index;
	


}

uint8_t Filter::SearchFilterCornerUpLeftBLock(uint8_t* curPos)
{
	vector<uint32_t> totalPixel(9);

	uint8_t* pixLeft = curPos - canals;
	uint8_t* pixUp = curPos - (width * canals);
	uint8_t* pixUpLeft = curPos - (width * canals) - canals;


	for (int a = 0; a < moduloBlockHeight - 1; a++)
	{
		for (int b = 0; b < moduloBlockWidth - 1; b++) // Ignore pixels to the left of the block
		{
			for (int a = 0; a < canals; a++)
			{

				totalPixel[0] += abs((*curPos - Average2(*pixLeft, *pixUp)));
				totalPixel[1] += abs((*curPos - *pixUp));
				totalPixel[2] += abs((*curPos - *pixUpLeft));
				totalPixel[3] += (abs((*curPos - *pixLeft)));
				totalPixel[4] += abs((*curPos - ClampAddSubtractFull(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[5] += abs((*curPos - SemiPaeth23(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[6] += abs((*curPos - SemiPaeth5(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[7] += abs((*curPos - SkewedGradientFilter(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[8] += abs((*curPos - Average2(*pixUp, *pixUpLeft)));
				totalPixel[9] += abs(*curPos);
				curPos--;
				pixLeft--;
				pixUp--;
				pixUpLeft--;
			}
		}
		curPos -= (width * canals) - ((moduloBlockWidth - 1) * canals); // Ignore pixels to the left of the block
		pixLeft = curPos - canals;
		pixUp = curPos - (width * canals);
		pixUpLeft = curPos - (width * canals) - canals;

	}

	uint32_t min_val = totalPixel[0];
	size_t min_index = 0;

	for (size_t i = 1; i < totalPixel.size(); ++i)
	{
		if (totalPixel[i] < min_val)
		{
			min_val = totalPixel[i];
			min_index = i;
		}
	}




	return min_index;

}

uint8_t Filter::SearchFilterUpBLock(uint8_t* curPos)
{
	vector<uint32_t> totalPixel(9);

	uint8_t* pixLeft = curPos - canals;
	uint8_t* pixUp = curPos - (width * canals);
	uint8_t* pixUpLeft = curPos - (width * canals) - canals;


	for (int a = 0; a < moduloBlockHeight - 1; a++) // -1 because we ignore the top line
	{
		for (int b = 0; b < blockSize; b++)
		{
			for (int a = 0; a < canals; a++)
			{

				totalPixel[0] += abs((*curPos - Average2(*pixLeft, *pixUp)));
				totalPixel[1] += abs((*curPos - *pixUp));
				totalPixel[2] += abs((*curPos - *pixUpLeft));
				totalPixel[3] += (abs((*curPos - *pixLeft)));
				totalPixel[4] += abs((*curPos - ClampAddSubtractFull(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[5] += abs((*curPos - SemiPaeth23(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[6] += abs((*curPos - SemiPaeth5(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[7] += abs((*curPos - SkewedGradientFilter(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[8] += abs((*curPos - Average2(*pixUp, *pixUpLeft)));
				totalPixel[9] += abs(*curPos);
				curPos--;
				pixLeft--;
				pixUp--;
				pixUpLeft--;
			}
		}
		curPos -= (width * canals) - ((blockSize)*canals);
		pixLeft = curPos - canals;
		pixUp = curPos - (width * canals);
		pixUpLeft = curPos - (width * canals) - canals;

	}


	uint32_t min_val = totalPixel[0];
	size_t min_index = 0;

	for (size_t i = 1; i < totalPixel.size(); ++i)
	{
		if (totalPixel[i] < min_val)
		{
			min_val = totalPixel[i];
			min_index = i;
		}
	}




	return min_index;

}

uint8_t Filter::SearchFilterLeftBLock(uint8_t* curPos)
{
	vector<uint32_t> totalPixel(9);

	uint8_t* pixLeft = curPos - canals;
	uint8_t* pixUp = curPos - (width * canals);
	uint8_t* pixUpLeft = curPos - (width * canals) - canals;

	for (int a = 0; a < blockSize; a++)
	{
		for (int b = 0; b < moduloBlockWidth - 1; b++)
		{
			for (int a = 0; a < canals; a++)
			{

				totalPixel[0] += abs((*curPos - Average2(*pixLeft, *pixUp)));
				totalPixel[1] += abs((*curPos - *pixUp));
				totalPixel[2] += abs((*curPos - *pixUpLeft));
				totalPixel[3] += (abs((*curPos - *pixLeft)));
				totalPixel[4] += abs((*curPos - ClampAddSubtractFull(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[5] += abs((*curPos - SemiPaeth23(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[6] += abs((*curPos - SemiPaeth5(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[7] += abs((*curPos - SkewedGradientFilter(*pixLeft, *pixUp, *pixUpLeft)));
				totalPixel[8] += abs((*curPos - Average2(*pixUp, *pixUpLeft)));
				totalPixel[9] += abs(*curPos);
				curPos--;
				pixLeft--;
				pixUp--;
				pixUpLeft--;
			}
		}
		curPos -= (width * canals) - ((moduloBlockWidth - 1) * canals);
		pixLeft = curPos - canals;
		pixUp = curPos - (width * canals);
		pixUpLeft = curPos - (width * canals) - canals;

	}

	uint32_t min_val = totalPixel[0];
	size_t min_index = 0;

	for (size_t i = 1; i < totalPixel.size(); ++i)
	{
		if (totalPixel[i] < min_val)
		{
			min_val = totalPixel[i];
			min_index = i;
		}
	}

	return min_index;

}

vector<uint8_t>* Filter::getTypeFilter()
{
	return &typeFilter;
}

uint32_t Filter::getWidth()
{
	return width;
}

uint32_t Filter::getHeight()
{
	return height;
}

uint32_t Filter::getCanals()
{
	return canals;
}

uint32_t Filter::getBitPerChannel()
{
	return bitPerChannel;
}

cv::Mat* Filter::getMat()
{
	return &Image;
}