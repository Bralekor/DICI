#include "DICIdecode.h"
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cout << "Incorrect command, use: " << endl;
        cout << "DICI-encode-image file_name output_file" << endl << endl;
        cout << "Exemple : DICI-decode-image cat.dici cat.png" << endl << endl;
        cout << "Or for the viewer use : " << endl;
        cout << "DICI-encode-image file_name -v" << endl << endl;
        cout << "Exemple : DICI-decode-image cat.dici" << endl;
        cout << "To decompress a folder: : " << endl;
        cout << "DICI-decode-image input_folder output_folder" << endl;
        return 0;
    }

    bool withThread = true;
    bool isDirectory = false;

    // Checking arguments for threading options
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-nt") == 0 || strcmp(argv[i], "-noThread") == 0)
        {
            withThread = false;
        }
    }

    // Checks if the first argument is a folder
    fs::path inputPath(argv[1]);
    if (fs::is_directory(inputPath))
    {
        isDirectory = true;
    }

    if (isDirectory)
    {
        
        // Ensures there is a second argument for the output folder
        if (argc < 3)
        {
            cout << "Missing output folder." << endl;
            return 1;
        }

        fs::path outputPath(argv[2]);
        // Create the output folder if it does not exist
        fs::create_directories(outputPath);

        // Processes each file in the input folder
        for (const auto& entry : fs::directory_iterator(inputPath))
        {
            const auto& path = entry.path();
            if (path.extension() == ".dici") // Make sure the file is a DICI image
            {
                string outputFile = (outputPath / path.stem()).string() + ".bmp"; // Builds the exit path

                DICIdecode imgToDecode;
                imgToDecode.setFileIN(path.string());
                imgToDecode.setThreaded(withThread);
                imgToDecode.DICIdecompress();
                imgToDecode.save(outputFile);
            }
        }
    }
    else
    {
        DICIdecode imgToDecode;
        imgToDecode.setFileIN(argv[1]);
        imgToDecode.setThreaded(withThread);
        imgToDecode.DICIdecompress();

        if (argc > 2)
        {
            imgToDecode.save(argv[2]);
        }
        else
        {
            imgToDecode.view();
        }
    }

    return 0;
}
