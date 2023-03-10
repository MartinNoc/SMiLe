#include "LoadImage.h"


std::vector<double> load_image(std::string pathToFile) {
    std::fstream fin(pathToFile);

    if (!fin.is_open()) {
        std::cout << "Could not open image file." << std::endl;
        std::exit(-1);
    }

    std::vector<double> img;
    std::string line, temp, entry;

    while (getline(fin, line)) {

        std::stringstream s(line);

        while (getline(s, entry, ';')) {
            img.push_back(std::stod(entry));
        }

    }

    fin.close();

    if (img.size() == 1023)
        img.push_back(.0);

    return img;
}
