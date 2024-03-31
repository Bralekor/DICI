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

#include "DICIdecode.h"
#include "filter_reverse.h"

using namespace std;


void DICIdecode::getDataFromFile(string fileName, vector<uint8_t>* data)
{
    ifstream fileToRead(fileName, ios::binary);

    // Determine the file size
    fileToRead.seekg(0, ios::end);
    streampos fileSize = fileToRead.tellg();
    fileToRead.seekg(0, ios::beg);

    // Reserve space in the vector for file data
    data->resize(fileSize);

    // Reads data from file into vector
    fileToRead.read(reinterpret_cast<char*>(data->data()), fileSize);


    fileToRead.close();
}

void DICIdecode::vectorAssemble(vector<vector<uint8_t>>* in, vector<uint8_t>* out)
{
    for (auto vec : *in)
    {
        out->insert(out->end(), vec.begin(), vec.end());
    }
}

DICIdecode::DICIdecode()
{
    threaded = 1;
}

DICIdecode::DICIdecode(string fileIN, bool threaded)
{
    this->fileIN = fileIN;
    this->threaded = threaded;

}

void DICIdecode::DICIdecompress()
{
   
    tmpDataFilter.resize(0);

    getDataFromFile(fileIN, &data);

    uint32_t posData = 0; // Determines the position in the vector


    uint8_t dici[4];
    uint32_t version = 0;

    uint32_t sizeTypeFilter;
    vector<uint8_t> typeFilter;
    uint32_t width;
    uint32_t height;
    uint32_t canals;
    uint32_t bitPerChannel;

    memcpy(&(dici[0]), data.data() + posData, sizeof(uint32_t));
    posData += 4;
    memcpy(&version, data.data() + posData, sizeof(uint32_t));
    posData += 4;

    if (version > 1)
    {
        cout << "The file is in a newer version, update the software" << endl;
        return;
    }

    memcpy(&width, data.data() + posData, sizeof(uint32_t));
    posData += 4;
    memcpy(&height, data.data() + posData, sizeof(uint32_t));
    posData += 4;
    memcpy(&canals, data.data() + posData, sizeof(uint32_t));
    posData += 4;
    memcpy(&bitPerChannel, data.data() + posData, sizeof(uint32_t));
    posData += 4;

    memcpy(&sizeTypeFilter, data.data() + posData, sizeof(uint32_t));
    posData += 4;
    typeFilter.resize(sizeTypeFilter);
    memcpy(typeFilter.data(), data.data() + posData, sizeTypeFilter);
    posData += sizeTypeFilter;

    for (int a = 0; a < 2; a++)
    {
        threads.resize(0);
        splitData.resize(0);
        vecDecode.clear();
        dataFilter.resize(0);
        




        uint32_t sizeData = data.size();
        uint32_t sizeSplitData = 0;

        
        uint32_t nbSlice = 0;

        while (posData < sizeData)
        {
            memcpy(&sizeSplitData, data.data() + posData, sizeof(uint32_t)); // Recover the size of the first fraction of the data
            posData += 4;

            if (sizeSplitData == 0)
            {
                break;
            }

            splitData.emplace_back(vector<uint8_t>(sizeSplitData));
            memcpy((splitData[nbSlice].data()), data.data() + posData, sizeSplitData * sizeof(uint8_t));
            posData += (sizeSplitData * sizeof(uint8_t));
            nbSlice++;

        }


        out.resize(nbSlice);

        if (threaded == 1)
        {

            

            // Create an Encode object for each fraction of data
 
            for (int i = 0; i < nbSlice; i++)
            {
                vecDecode.emplace_back(Decode(&(splitData[i]), &(out[i])));
            }



            // Launch the threads
            for (int i = 0; i < nbSlice; i++)
            {
                threads.emplace_back(async(launch::async, &Decode::Decompression, &vecDecode[i]));
            }

            for (auto& t : threads)
            {
                t.wait();
            }

        }
        else {

            for (int i = 0; i < nbSlice; i++)
            {
                Decode vecData(&(splitData[i]), &(out[i]));
                vecData.Decompression();

            }


        }



        // Reassemble the vectors

        dataFilter.reserve(out.size() * out[0].size());

        for (auto& subvec : out) {
            dataFilter.insert(dataFilter.end(), make_move_iterator(subvec.begin()), make_move_iterator(subvec.end()));
        }


        // Reverse the filter
        Filter_reverse Image(&dataFilter, &finalData, width, height, canals, &typeFilter, threaded);
        Image.Reverse();

        if (bitPerChannel == 8)
        {

            if (canals == 3)
            {
                ImageFinal = cv::Mat(height, width, CV_8UC3, dataFilter.data());

            }
            else if (canals == 4)
            {
                ImageFinal = cv::Mat(height, width, CV_8UC4, dataFilter.data());
            }
            else if (canals == 1)
            {
                ImageFinal = cv::Mat(height, width, CV_8UC1, dataFilter.data());
            }
            
            break;
        }
        
        if (bitPerChannel == 16 && a == 0)
        {
            tmpDataFilter = dataFilter;

        }
        else {

            dataFilter16.resize(dataFilter.size());

            for (int a = 0; a < dataFilter.size(); a++)
            {
                dataFilter16[a] = (tmpDataFilter[a] << 8) | dataFilter[a];
            }

            ImageFinal = cv::Mat(height, width, CV_16UC3, dataFilter16.data());

        }


    }

}

void DICIdecode::view()
{
    imshow("Image", ImageFinal);
    cv::waitKey(0);
}

void DICIdecode::save(string fileOUT)
{
    imwrite(fileOUT, ImageFinal);
}


void DICIdecode::setFileIN(string fileIN)
{
    this->fileIN = fileIN;
}

void DICIdecode::setThreaded(bool threaded)
{
    this->threaded = threaded;
}

cv::Mat* DICIdecode::getMat()
{
    return &ImageFinal;
}

