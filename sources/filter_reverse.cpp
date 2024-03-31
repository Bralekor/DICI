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

#include "filter_reverse.h"

using namespace std;

inline uint8_t Average2(uint8_t a, uint8_t b)
{
	return (a + b) >> 1;
}

inline int32_t Clamp(int32_t a)
{
	return (a < 0) ? 0 : (a > 255) ? 255 : a;
}

inline uint8_t ClampAddSubtractFull(int32_t a, int32_t b, int32_t c)
{
	return Clamp(a + b - c);
}

inline uint8_t SemiPaeth5(uint8_t W, uint8_t N, uint8_t NW)
{
	int32_t pred = ((5 * (N + W) - (NW << 1)) + 3) >> 3;
	return Clamp(pred);
}

inline uint8_t SemiPaeth23(uint8_t W, uint8_t N, uint8_t NW)
{
	int32_t pred = (((N << 1) + 3 * W - NW) + 1) >> 2;
	return Clamp(pred);
}

inline uint8_t SkewedGradientFilter(uint8_t a, uint8_t b, uint8_t c)
{
	int32_t pred = (3 * (b + a) - (c << 1)) >> 2;
	return Clamp(pred);
}

Filter_reverse::Filter_reverse(vector<uint8_t>* in, vector<uint8_t>* out, uint32_t width, uint32_t height, uint32_t canals, vector<uint8_t>* typeFilter)
{
	this->width = width;
	this->height = height;
	this->canals = canals;
	this->data = in;
	this->typeFilter = typeFilter;

	withThread = 1;


}

Filter_reverse::Filter_reverse(vector<uint8_t>* in, vector<uint8_t>* out, uint32_t width, uint32_t height, uint32_t canals, vector<uint8_t>* typeFilter, bool thread)
{
	this->width = width;
	this->height = height;
	this->canals = canals;
	this->data = in;
	this->typeFilter = typeFilter;

	withThread = thread;


}

void Filter_reverse::Reverse()
{

	blockSize = 48; // Each block is 48x48

	// Count the number of 48x48 blocks possible on the X and Y axis
	nbBlockWidth = ceil((float)width / blockSize);
	nbBlockHeight = ceil((float)height / blockSize);


	uint8_t* startPixel = &(*data)[0]; // pointer to first pixel
	uint8_t* endPixel = startPixel + (width * height * canals) - 1; // pointer to last pixel

	endBlock.resize(nbBlockWidth * nbBlockHeight);

	moduloBlockWidth = width % blockSize;
	moduloBlockHeight = height % blockSize;

	if (moduloBlockWidth == 0)
	{
		moduloBlockWidth = blockSize;
	}

	if (moduloBlockHeight == 0)
	{
		moduloBlockHeight = blockSize;
	}

	uint8_t* curseurPtr = startPixel; // we place the cursor at the end because we will perform filters identifying the top and left pixel
	indexBlock = 0; // we take the last block because we have to start from the bottom left for the filters


	// We recover the first pixel of each block

	//First line
	endBlock[indexBlock] = curseurPtr;

	curseurPtr = curseurPtr + ((moduloBlockWidth) * canals);


	indexBlock++;

	for (int b = 1; b < nbBlockWidth; b++)
	{


		endBlock[indexBlock] = curseurPtr;

		curseurPtr = curseurPtr + (blockSize * canals);

		indexBlock++;

	}

	curseurPtr += (width * (moduloBlockHeight - 1) * canals);


	// Other line
	for (int a = 0; a < nbBlockHeight - 1; a++)
	{
		endBlock[indexBlock] = curseurPtr;


		indexBlock++;

		curseurPtr = curseurPtr + ((moduloBlockWidth) * canals);
		
		for (int b = 1; b < nbBlockWidth; b++)
		{


			endBlock[indexBlock] = curseurPtr;

			curseurPtr = curseurPtr + (blockSize * canals);

			indexBlock++;

		}
		curseurPtr += (width * ((blockSize) - 1) * canals);
	}

	//-------------------------

	// We apply the inverse filter on each block

	if (withThread == 0)
	{
		ApplyFilterWithNoThread();
	}
	else {
		ApplyFilterWithThread();

	}
	
	


}

void Filter_reverse::ApplyFilterWithThread()
{

	vector<future<void>> futures(nbBlockWidth);

	indexBlock = 0;

	uint32_t indexBlock2 = indexBlock;


	uint32_t numThread = 0;
	uint32_t remaining = 0;



	if (nbBlockWidth >= nbBlockHeight)
	{
		FilterCornerUpLeftBLock(endBlock[indexBlock2], (*typeFilter)[indexBlock2]);
		indexBlock++;
		indexBlock2++;

		for (int a = 0; a < nbBlockHeight - 1; a++)
		{
			remaining++;

			futures[numThread] = (async(launch::async, &Filter_reverse::FilterUpBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
			numThread++;
			indexBlock2 += nbBlockWidth - 1;

			for (int b = 0; b < remaining - 1; b++)
			{
				futures[numThread] = (async(launch::async, &Filter_reverse::FilterBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
				numThread++;

				indexBlock2 += nbBlockWidth - 1;
			}

			futures[numThread] = (async(launch::async, &Filter_reverse::FilterLeftBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
			numThread++;

			indexBlock += 1;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}


		for (int a = 0; a < nbBlockWidth - nbBlockHeight; a++)
		{

			futures[numThread] = (async(launch::async, &Filter_reverse::FilterUpBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
			numThread++;
			indexBlock2 += nbBlockWidth - 1;

			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter_reverse::FilterBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
				numThread++;

				indexBlock2 += nbBlockWidth - 1;
			}


			indexBlock += 1;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}

		indexBlock += (nbBlockWidth - 1);
		indexBlock2 = indexBlock;
		remaining++;

		for (int a = 0; a < nbBlockHeight - 1; a++)
		{
			remaining--;

			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter_reverse::FilterBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
				numThread++;

				indexBlock2 += nbBlockWidth - 1;
			}


			indexBlock += nbBlockWidth;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}

	}
	else {

		FilterCornerUpLeftBLock(endBlock[indexBlock2], (*typeFilter)[indexBlock2]);
		indexBlock += nbBlockWidth;
		indexBlock2 = indexBlock;
		
		for (int a = 0; a < nbBlockWidth - 1; a++)
		{
			remaining++;

			futures[numThread] = (async(launch::async, &Filter_reverse::FilterLeftBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
			numThread++;
			indexBlock2 += (1 - nbBlockWidth);
			
			for (int b = 0; b < remaining - 1; b++)
			{
				futures[numThread] = (async(launch::async, &Filter_reverse::FilterBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
				numThread++;

				indexBlock2 += (1 - nbBlockWidth);
			}

			futures[numThread] = (async(launch::async, &Filter_reverse::FilterUpBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
			numThread++;

			indexBlock += nbBlockWidth;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}

		for (int a = 0; a < nbBlockHeight - nbBlockWidth; a++)
		{

			futures[numThread] = (async(launch::async, &Filter_reverse::FilterLeftBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
			numThread++;
			indexBlock2 += (1 - nbBlockWidth);

			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter_reverse::FilterBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
				numThread++;

				indexBlock2 += (1 - nbBlockWidth);
			}



			indexBlock += nbBlockWidth;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}

		indexBlock -= (nbBlockWidth - 1);
		indexBlock2 = indexBlock;
		remaining++;


		for (int a = 0; a < nbBlockWidth - 1; a++)
		{
			remaining--;


			for (int b = 0; b < remaining; b++)
			{
				futures[numThread] = (async(launch::async, &Filter_reverse::FilterBLock, this, endBlock[indexBlock2], (*typeFilter)[indexBlock2]));
				numThread++;

				indexBlock2 -= (nbBlockWidth - 1);
			}



			indexBlock++;
			indexBlock2 = indexBlock;

			for (int b = 0; b < numThread; b++)
			{
				futures[b].wait();
			}


			numThread = 0;


		}

	}


	

}

void Filter_reverse::ApplyFilterWithThreadRight(uint8_t* curPos, uint32_t indexBlock, int32_t lenght)
{

	int add = (blockSize * canals);

	for (int a = 0; a < lenght - 1; a++)
	{
		curPos += add;
		indexBlock++;

		FilterBLock(curPos, (*typeFilter)[indexBlock]);

	}


}



void Filter_reverse::ApplyFilterWithThreadDown(uint8_t* curPos, uint32_t indexBlock, int32_t lenght)
{

	int add = (width * blockSize * canals);

	for (int a = 0; a < lenght - 1; a++)
	{
		curPos += add;
		indexBlock += nbBlockWidth;



		FilterBLock(curPos, (*typeFilter)[indexBlock]);


	}


}



void Filter_reverse::ApplyFilterWithNoThread()
{
	indexBlock = 0;

	FilterCornerUpLeftBLock(endBlock[indexBlock], (*typeFilter)[indexBlock]);

	indexBlock++;
	
	for (int a = 0; a < nbBlockWidth - 1; a++)
	{
		FilterUpBLock(endBlock[indexBlock], (*typeFilter)[indexBlock]);

		indexBlock++;
	}


	
	for (int a = 0; a < nbBlockHeight - 1; a++)
	{

		FilterLeftBLock(endBlock[indexBlock], (*typeFilter)[indexBlock]);

		indexBlock++;

		for (int a = 0; a < nbBlockWidth - 1; a++)
		{
			FilterBLock(endBlock[indexBlock], (*typeFilter)[indexBlock]);
			indexBlock++;
		}

	}
	

}


void Filter_reverse::ApplyFilterOnBlock(uint8_t* curPos, uint8_t filter)
{
	uint8_t* pixLeft = curPos - canals;
	uint8_t* pixUp = curPos - (width * canals);
	uint8_t* pixUpLeft = curPos - (width * canals) - canals;


	switch (filter)
	{
	case 0:
		*curPos += Average2(*pixLeft, *pixUp);
		break;
	case 1:
		*curPos += *pixUp;
		break;
	case 2:
		*curPos += *pixUpLeft;
		break;
	case 3:
		*curPos += *pixLeft;
		break;
	case 4:
		*curPos += ClampAddSubtractFull(*pixLeft, *pixUp, *pixUpLeft);
		break;
	case 5:
		*curPos += SemiPaeth23(*pixLeft, *pixUp, *pixUpLeft);
		break;
	case 6:
		*curPos += SemiPaeth5(*pixLeft, *pixUp, *pixUpLeft);

		break;
	case 7:
		*curPos += SkewedGradientFilter(*pixLeft, *pixUp, *pixUpLeft);
		break;
	case 8:
		*curPos += Average2(*pixUp, *pixUpLeft);
		break;
	case 9:

		break;
	}
}

void Filter_reverse::FilterBLock(uint8_t* curPos, const uint8_t filter)
{
	int blockSizeCanals = blockSize * canals;
	int rowOffset = (width * canals) - blockSizeCanals;

	for (int a = 0; a < blockSize; a++)
	{
	
		for (int b = 0; b < blockSize; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);
				curPos++;
			}

		}

		curPos += rowOffset;

	}


}

void Filter_reverse::FilterCornerUpLeftBLock(uint8_t* curPos, uint8_t filter)
{

	curPos += (width * canals);
	curPos += canals;


	int add = (width * canals) - ((moduloBlockWidth - 1) * canals);

	for (int a = 0; a < moduloBlockHeight - 1; a++)
	{
		for (int b = 0; b < moduloBlockWidth - 1; b++) // Ignore the pixels to the left of the block
		{
			
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);
				curPos++;
			}
		}
		curPos += add; // Ignore the pixels to the left of the block
		
	}

}

void Filter_reverse::FilterUpBLock(uint8_t* curPos, uint8_t filter)
{

	curPos += (width * canals);

	int add = (width * canals) - ((blockSize)*canals);

	for (int a = 0; a < moduloBlockHeight - 1; a++) // -1 because we ignore the top line
	{
		for (int b = 0; b < blockSize; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);
				curPos++;
			}
		}
		curPos += add;

	}

}

void Filter_reverse::FilterLeftBLock(uint8_t* curPos, uint8_t filter)
{
	curPos += canals;

	int add = (width * canals) - ((moduloBlockWidth - 1) * canals);

	for (int a = 0; a < blockSize; a++)
	{
		for (int b = 0; b < moduloBlockWidth - 1; b++)
		{
			for (int a = 0; a < canals; a++)
			{
				ApplyFilterOnBlock(curPos, filter);
				curPos++;
			}
		}
		curPos += add;

	}

}
