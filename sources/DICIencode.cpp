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

#include "DICIencode.h"

using namespace std;

void DICIencode::function_splitData(vector <vector<uint8_t>>* splitData, span<uint8_t>* data, unsigned char nbSplit, uint8_t bytePerValue)
{

    size_t chunk_size = data->size() / nbSplit;

    chunk_size += (bytePerValue - (chunk_size % bytePerValue));

    for (size_t i = 0; i < nbSplit; ++i)
    {
        if (i == nbSplit - 1)
        {
            splitData->push_back(vector<uint8_t>(data->begin() + i * chunk_size, data->end()));
        }
        else {
            splitData->push_back(vector<uint8_t>(data->begin() + i * chunk_size, data->begin() + (i + 1) * chunk_size));
        }
    }

}

DICIencode::DICIencode()
{
    this->threaded = 1;
}

DICIencode::DICIencode(string fileIN, string fileOUT, bool threaded)
{
    this->fileIN = fileIN;
    this->fileOUT = fileOUT;
    this->threaded = threaded;
}

bool DICIencode::DICIcompress()
{

    image = Filter(fileIN, threaded);

    if (image.getMat()->empty())
    {
        return 1;
    }

    data = span<uint8_t>(image.getMat()->data, image.getMat()->total() * image.getMat()->channels());

    if (image.getHeight() == 0)
    {
        return 1;
    }

    if (image.getBitPerChannel() == 8)
    {
        image.Apply();
    }
    else {

        image.Apply16(0);
    }

    ofstream file(fileOUT, ios::binary | ios::out);

    uint8_t dici[4];
    dici[0] = 68;
    dici[1] = 73;
    dici[2] = 67;
    dici[3] = 73;
    uint32_t version = 1;

    file.write(reinterpret_cast<char*>(&dici), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&version), sizeof(uint32_t));

    uint32_t sizeTypeFilter = image.getTypeFilter()->size();

    uint32_t width = image.getWidth();
    uint32_t height = image.getHeight();
    uint32_t canals = image.getCanals();
    uint32_t bitPerChannel = image.getBitPerChannel();

    file.write(reinterpret_cast<char*>(&width), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&height), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&canals), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&bitPerChannel), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&sizeTypeFilter), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(image.getTypeFilter()->data()), sizeTypeFilter);
    image.getTypeFilter()->clear();

    for (int a = 0; a < image.getBitPerChannel() / 8; a++) // Performs 2 loops if the image is in 48 bits per pixels
    {
        splitData.resize(0);
        threads.resize(0);
        vecEncode.clear();


        if (a == 1)
        {
            image.Apply16(1);
        }

        // We cut the image into pieces to use several threads
        int nbSlice;

        uint32_t sizeImage = image.getWidth() * image.getHeight() * image.getCanals();

        if (sizeImage <= 172800) //320 X 180 in 3 channels
        {
            nbSlice = 1;

        }
        else if (sizeImage <= 2764800) // 1280 X 720 in 3 channels
        {
            nbSlice = 4;

        }
        else { // au dessus

            nbSlice = 16;

        }
 
        uint8_t bytePerValue = image.getCanals(); // Variable that allows each pixel data to be grouped together


        if (bytePerValue == 1) // For 1 channel images we make an exception
        {
            bytePerValue = 2;
        }

        if (a == 1) // For 48-bit images
        {
            bytePerValue = 1;
        }


        function_splitData(&splitData, &data, nbSlice, bytePerValue);

        out.resize(nbSlice);
        
        if (threaded == 1)
        {

            // Creates an Encode object for each slice
            
            for (int i = 0; i < nbSlice; i++)
            {
                vecEncode.emplace_back(Encode(&(splitData[i]), &(out[i]), bytePerValue));
            }

            // We launch the threads
            for (int i = 0; i < nbSlice; i++)
            {
                threads.push_back(thread(&Encode::Compress, &vecEncode[i]));
            }

            for_each(threads.begin(), threads.end(), mem_fn(&thread::join)); // Wait for all threads to finish

        }
        else {
            for (int i = 0; i < nbSlice; i++)
            {
                Encode vecData(&(splitData[i]), &(out[i]), bytePerValue);
                vecData.Compress();
            }
        }


        // Save compressed data

        for (int a = 0; a < nbSlice; a++)
        {
            uint32_t size = out[a].size();
            file.write(reinterpret_cast<char*>(&size), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(out[a].data()), out[a].size()); // Save the tables
        }

        uint32_t zero = 0;
        file.write(reinterpret_cast<char*>(&zero), sizeof(uint32_t)); // end of image



    }

    file.close();

    return 0;
}

void DICIencode::setFileIN(string fileIN)
{
    this->fileIN = fileIN;
}

void DICIencode::setFileOUT(string fileOUT)
{
    this->fileOUT = fileOUT;
}

void DICIencode::setThreaded(bool threaded)
{
    this->threaded = threaded;
}

void DICIencode::clear()
{
    delete &image;

    splitData.clear();
    out.clear();
    threads.clear();

    vecEncode.clear();
}