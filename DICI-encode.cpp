#include <filesystem>
#include "DICIencode.h"


using namespace std;

int main(int argc, char* argv[])
{


    if (argc < 3)
    {
        cout << "Missing argument :"<< endl;
        cout << "DICI-encode input output" << endl << endl;
        cout << "Example : DICI-encode cat.png cat.dici" << endl;
        cout << "        : DICI-encode c:/folder/ c:/folder2" << endl;
        cout << "For single thread use -nt or -noThread" << endl;
        return 0;
    }

    bool withThread = 1;


    for (int i = 3; i < argc; i++)
    {
        if (strcmp(argv[i], "-nt") == 0 || strcmp(argv[i], "-noThread") == 0)
        {
            withThread = false;
        }
    }


    bool fileOrFolder = 0;

    const string pathINstr = argv[1];
    filesystem::path pathIN(pathINstr);

    const string pathOUTstr = argv[2];
    filesystem::path pathOUT(pathOUTstr);

    if (filesystem::exists(pathIN)) {

        if (filesystem::is_regular_file(pathIN))
        {
            fileOrFolder = 1;
            
        }
        else if (filesystem::is_directory(pathIN)) 
        {
            if (filesystem::is_directory(pathOUT))
            {
                fileOrFolder = 0;
            }
            else {

                cout << "The output path is invalid." << endl;
                return 1;
            }

        }
        else 
        {
            cerr << "The specified path does not exist." << endl;
            return 1;
        }
    }
    else {
        cerr << "The specified path does not exist." << endl;
        return 1;
    }


    if (fileOrFolder == 1)
    {
        DICIencode imageToEncode;
        imageToEncode.setFileIN(argv[1]);
        imageToEncode.setFileOUT(argv[2]);
        imageToEncode.setThreaded(withThread);
        imageToEncode.DICIcompress();
    }
    else {

        static const vector<string> supportedExtensions = { ".bmp", ".png", ".jpg", ".jpeg", ".tif", ".tiff", ".webp" };

        for (const auto& entry : filesystem::directory_iterator(pathINstr)) 
        {
            if (entry.is_regular_file())
            {
                const string extension = entry.path().extension().string();
                if (find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end()) 
                {

                    DICIencode imageToEncode;
                    imageToEncode.setFileIN(entry.path().string());
                    imageToEncode.setFileOUT(pathOUTstr + "\\" + entry.path().stem().string() + ".dici");
                    imageToEncode.setThreaded(withThread);
                    imageToEncode.DICIcompress();

                }
            }
        }

    }

    return 0;
}
