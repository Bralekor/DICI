#include "decode.h"

using namespace std;


static inline uint32_t ilog2(float x)
{
    uint32_t ix = (uint32_t&)x;
    uint32_t exp = (ix >> 23) & 0xFF;
    int32_t log2 = int32_t(exp) - 127;

    return log2;
}

void Uncompress(vector<uint8_t>& outBuf, vector<uint8_t>& inBuf, int32_t size)
{
    outBuf.resize(size);
    size_t dstLen = outBuf.size();
    size_t srcLen = inBuf.size() - LZMA_PROPS_SIZE;
    SRes res = LzmaUncompress(&outBuf[0], &dstLen, &inBuf[LZMA_PROPS_SIZE], &srcLen, &inBuf[0], LZMA_PROPS_SIZE);
}


Decode::Decode(vector<uint8_t>* in, vector<uint8_t>*out)
{
    dataIn = in;
    uncompressedData = out;   
}

void Decode::readCompressedFile()
{

    uint32_t size = 0;

    vector<uint8_t> frequency_vector_compressed;
    vector<uint8_t> bitToRead_compressed;

    uint32_t size_frequency_vector;
    uint32_t size_bitToRead;


    memcpy(&size, dataIn->data(), sizeof(uint32_t));
    frequency_vector_compressed.resize(size);
    memcpy(&size, dataIn->data() + 4, sizeof(uint32_t));
    bitToRead_compressed.resize(size);
    memcpy(&size, dataIn->data() + 8, sizeof(uint32_t));
    dataCompressed.resize(size);

    memcpy(&size_frequency_vector, dataIn->data() + 12, sizeof(uint32_t));
    memcpy(&size_bitToRead, dataIn->data() + 16, sizeof(uint32_t));


    memcpy(&bytePerValue, dataIn->data() + 20, sizeof(uint8_t));
    memcpy(&sizeData, dataIn->data() + 21, sizeof(uint64_t));


    memcpy(frequency_vector_compressed.data(), dataIn->data() + 29, frequency_vector_compressed.size());
    memcpy(bitToRead_compressed.data(), dataIn->data() + 29 + (frequency_vector_compressed.size() ), bitToRead_compressed.size());
    memcpy(dataCompressed.data(), dataIn->data() + 29 + (frequency_vector_compressed.size() ) + bitToRead_compressed.size(), dataCompressed.size());

    
    vector<uint8_t> frequency_vector_char;

    Uncompress(frequency_vector_char, frequency_vector_compressed, size_frequency_vector);
    Uncompress(bitToRead, bitToRead_compressed, size_bitToRead);

    
    frequency_vector.reserve(frequency_vector_char.size() / 4);

    for (int a = 0; a < frequency_vector_char.size(); a = a + 4)
    {
        frequency_vector.emplace_back((frequency_vector_char[a + 3] << 24) | (frequency_vector_char[a + 2] << 16) | (frequency_vector_char[a + 1] << 8) | (frequency_vector_char[a + 0]));
    }

    // Reverse the filter applied to frequency_vector
    for (int a = 1; a < frequency_vector.size(); a++)
    {
        frequency_vector[a] += frequency_vector[a - 1];
    }

   



    uncompressedData->reserve(sizeData);

}

void Decode::Decompression()
{

    readCompressedFile();

    uint32_t mostFrequentNum = frequency_vector[0]; // Determine the most frequent value (optimization)

    uint8_t numberOfDigit; // Number of digits to insert for the vector bitToRead
    uint8_t maxValue; // The maximum possible value
    uint8_t mask;

    if (frequency_vector.size() < pow(2, 8) * 2 - 2)
    {
        numberOfDigit = 3;
        maxValue = 8;
        mask = 7;

    }
    else if (frequency_vector.size() < pow(2, 16) * 2 - 2) {

        numberOfDigit = 4;
        maxValue = 16;
        mask = 15;

    }
    else if (frequency_vector.size() < pow(2, 32) * 2 - 2) {

        numberOfDigit = 5;
        maxValue = 32;
        mask = 31;

    }
    else if (frequency_vector.size() < pow(2, 64) * 2 - 2) {

        numberOfDigit = 6;
        maxValue = 64;
        mask = 63;

    }

    // We detect if the optimization has increased numberOfDigit by 1
    if (floor(ilog2(frequency_vector.size() + 2)) >= maxValue)
    {
        numberOfDigit++; //si c'est le cas on incrémente
        maxValue *= 2;
        mask = (mask * 2) + 1;
    }

    const uint8_t nbDigit = numberOfDigit;

    // Precalculate the masks (to avoid repetitive calculations)
    uint32_t maskList[32];

    for (int a = 0; a < 32; a++)
    {
        maskList[a] = ((1 << a) - 1);
    }

    uint32_t numberOfDataToRead = ceil(bitToRead.size());

    uint16_t currentValue = 0;
    int bitsInCurrentValue = 0;
    uint64_t currentValue_2 = 0;
    int bitsInCurrentValue_2 = 0;
    uint8_t digit;

    vector<uint8_t>::iterator itDataCompressed = dataCompressed.begin();


    uint32_t compressedVal = 0;
    uint32_t uncompressedVal = 0;


    uncompressedData->resize(sizeData);
    vector<uint8_t>::iterator itUncompressedData = uncompressedData->begin();


    uint64_t stackValue = 0;
    uint8_t nbValueInStackValue = nbDigit * 7;

    int loop = (nbDigit - (bitToRead.size() % nbDigit));


    if (bitToRead.size() % nbDigit != 0)
    {

        for (int a = 0; a < loop; a++)
        {
            bitToRead.emplace_back(0);
        }
    

    }

    for (vector<uint8_t>::iterator it = bitToRead.begin(); it != bitToRead.end() - nbDigit; it = it + nbDigit)
    {
        
            if (nbDigit == 4)
            {
                stackValue = (*it << 24) | (*(it + 1) << 16) | (*(it + 2) << 8) | *(it + 3);
            }
            else if (nbDigit == 5)
            {
                stackValue = ((uint64_t)*it << 32) | ((uint64_t) *(it + 1) << 24) | ((uint64_t) *(it + 2) << 16) | ((uint64_t) *(it + 3) << 8) | (uint64_t) *(it + 4);

            }
            else if (nbDigit == 3)
            {
                stackValue = (*it << 16) | (*(it + 1) << 8) | *(it + 2);
            }
            
            for (int32_t a = nbValueInStackValue; a >= 0; a = a - nbDigit)
            {

                digit = (stackValue >> a) & mask;


                if (digit != 0) // optimisation
                {
                    // We now know how many digits to read, so we read dataCompressed
                    while (bitsInCurrentValue_2 < digit)
                    {

                        currentValue_2 = (currentValue_2 << 8) | *itDataCompressed;  // Shift and add the byte
                        itDataCompressed++;
                        bitsInCurrentValue_2 += 8;
                    }


                    bitsInCurrentValue_2 -= digit;
                    compressedVal = ((1 << digit) | ((currentValue_2 >> (bitsInCurrentValue_2)) & maskList[digit])) - 1;
                    uncompressedVal = frequency_vector[compressedVal];


                    if (bytePerValue == 4)
                    {
                        *itUncompressedData = (uncompressedVal >> 24) & 255;
                        itUncompressedData++;

                        *itUncompressedData = (uncompressedVal >> 16) & 255;
                        itUncompressedData++;

                        *itUncompressedData = (uncompressedVal >> 8) & 255;
                        itUncompressedData++;

                        *itUncompressedData = uncompressedVal & 255;
                        itUncompressedData++;

                    }else if (bytePerValue == 3)
                    {
                        *itUncompressedData = (uncompressedVal >> 16) & 255;
                        itUncompressedData++;

                        *itUncompressedData = (uncompressedVal >> 8) & 255;
                        itUncompressedData++;

                        *itUncompressedData = uncompressedVal & 255;
                        itUncompressedData++;

                    }
                    else if (bytePerValue == 2)
                    {
                        *itUncompressedData = (uncompressedVal >> 8) & 255;
                        itUncompressedData++;

                        *itUncompressedData = uncompressedVal & 255;
                        itUncompressedData++;
                    }
                    else {

                        *itUncompressedData = uncompressedVal & 255;
                        itUncompressedData++;

                    }

                }
                else {

                    if (bytePerValue == 4)
                    {

                        *itUncompressedData = (mostFrequentNum >> 24) & 255;
                        itUncompressedData++;
                        *itUncompressedData = (mostFrequentNum >> 16) & 255;
                        itUncompressedData++;
                        *itUncompressedData = (mostFrequentNum >> 8) & 255;
                        itUncompressedData++;
                        *itUncompressedData = mostFrequentNum & 255;
                        itUncompressedData++;

                    }else if (bytePerValue == 3)
                    {


                        *itUncompressedData = (mostFrequentNum >> 16) & 255;
                        itUncompressedData++;
                        *itUncompressedData = (mostFrequentNum >> 8) & 255;
                        itUncompressedData++;
                        *itUncompressedData = mostFrequentNum & 255;
                        itUncompressedData++;

                    }
                    else if (bytePerValue == 2)
                    {
                        *itUncompressedData = (mostFrequentNum >> 8) & 255;
                        itUncompressedData++;
                        *itUncompressedData = mostFrequentNum & 255;
                        itUncompressedData++;
                    }
                    else {

                        *itUncompressedData = mostFrequentNum & 255;
                        itUncompressedData++;

                    }


                }
                
            }
            
        
    }

    // Process the latest data
    vector<uint8_t>::iterator lastData = bitToRead.end() - nbDigit;

    if (nbDigit == 4)
    {
        stackValue = (*lastData << 24) | (*(lastData + 1) << 16) | (*(lastData + 2) << 8) | *(lastData + 3);
    }
    else if (nbDigit == 5)
    {
        stackValue = ((uint64_t)*lastData << 32) | ((uint64_t) * (lastData + 1) << 24) | ((uint64_t) * (lastData + 2) << 16) | ((uint64_t) * (lastData + 3) << 8) | (uint64_t) * (lastData + 4);

    }
    else if (nbDigit == 3)
    {
        stackValue = (*lastData << 16) | (*(lastData + 1) << 8) | *(lastData + 2);
    }
    
    if (loop != 0)
    {
        for (int32_t a = nbValueInStackValue; a >= 0; a = a - nbDigit)
        {
            
            if (itUncompressedData == uncompressedData->end())
            {
                break;
            }

            digit = (stackValue >> a) & mask;

            if (digit != 0) // optimisation
            {
                // We now know how many digits to read, so we read dataCompressed
                while (bitsInCurrentValue_2 < digit)
                {

                    currentValue_2 = (currentValue_2 << 8) | *itDataCompressed;  // Shift and add the byte
                    itDataCompressed++;
                    bitsInCurrentValue_2 += 8;
                }


                bitsInCurrentValue_2 -= digit;
                compressedVal = ((1 << digit) | ((currentValue_2 >> (bitsInCurrentValue_2)) & maskList[digit])) - 1;
                uncompressedVal = frequency_vector[compressedVal];


                if (bytePerValue == 4)
                {
                    *itUncompressedData = (uncompressedVal >> 24) & 255;
                    itUncompressedData++;

                    *itUncompressedData = (uncompressedVal >> 16) & 255;
                    itUncompressedData++;

                    *itUncompressedData = (uncompressedVal >> 8) & 255;
                    itUncompressedData++;

                    *itUncompressedData = uncompressedVal & 255;
                    itUncompressedData++;

                }else if (bytePerValue == 3)
                {
                    *itUncompressedData = (uncompressedVal >> 16) & 255;
                    itUncompressedData++;

                    *itUncompressedData = (uncompressedVal >> 8) & 255;
                    itUncompressedData++;

                    *itUncompressedData = uncompressedVal & 255;
                    itUncompressedData++;

                }
                else if (bytePerValue == 2)
                {
                    *itUncompressedData = (uncompressedVal >> 8) & 255;
                    itUncompressedData++;

                    *itUncompressedData = uncompressedVal & 255;
                    itUncompressedData++;
                }
                else {

                    *itUncompressedData = uncompressedVal & 255;
                    itUncompressedData++;

                }

            }
            else {

                if (bytePerValue == 4)
                {

                    *itUncompressedData = (mostFrequentNum >> 24) & 255;
                    itUncompressedData++;
                    *itUncompressedData = (mostFrequentNum >> 16) & 255;
                    itUncompressedData++;
                    *itUncompressedData = (mostFrequentNum >> 8) & 255;
                    itUncompressedData++;
                    *itUncompressedData = mostFrequentNum & 255;
                    itUncompressedData++;

                }else if (bytePerValue == 3)
                {


                    *itUncompressedData = (mostFrequentNum >> 16) & 255;
                    itUncompressedData++;
                    *itUncompressedData = (mostFrequentNum >> 8) & 255;
                    itUncompressedData++;
                    *itUncompressedData = mostFrequentNum & 255;
                    itUncompressedData++;

                }
                else if (bytePerValue == 2)
                {
                    *itUncompressedData = (mostFrequentNum >> 8) & 255;
                    itUncompressedData++;
                    *itUncompressedData = mostFrequentNum & 255;
                    itUncompressedData++;
                }
                else {

                    *itUncompressedData = mostFrequentNum & 255;
                    itUncompressedData++;
                }
            }

        }

    }

    // Delete the data added in the encode which allowed it to end right at the end of a byte
    for (int a = uncompressedData->size() - sizeData; a > 0; a--)
    {
        uncompressedData->pop_back();
    }
    


}



void Decode::SaveFile(const string& fileName)
{
    ofstream file(fileName, ios::binary | ios::out);


    if (file.is_open()) {

        // Writes the contents of the vector to the file
        file.write(reinterpret_cast<const char*>(uncompressedData->data()), uncompressedData->size() * sizeof(uint8_t));
        file.close();
    }
    else {

        cout << "Error : Can't create the file" << endl;

    }
}

