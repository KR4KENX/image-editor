#include <iostream>
#include <fstream>
#include <vector>

#pragma pack(push, 1) // Wyłączanie wyrównania struktury w celu zgodności z formatem BMP

struct BITMAPFILEHEADER {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BITMAPINFOHEADER {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct RGBTRIPLE {
    uint8_t rgbtBlue;
    uint8_t rgbtGreen;
    uint8_t rgbtRed;
};

#pragma pack(pop) // Włączanie wyrównania struktury z powrotem

std::vector<std::vector<RGBTRIPLE>> readBMP(std::ifstream& file){
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // Czytanie nagłówków
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42) { // 'BM' w kodzie ASCII
        std::cerr << "This is not a BMP file" << std::endl;
        std::vector<std::vector<RGBTRIPLE>> blank_bmp;
        return blank_bmp;
    }

    // Ustawienie wskaźnika na początek danych pikseli
    file.seekg(fileHeader.bfOffBits, std::ios::beg);

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    std::vector<std::vector<RGBTRIPLE>> pixels(height, std::vector<RGBTRIPLE>(width));

    // Czytanie danych pikseli
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            file.read(reinterpret_cast<char*>(&pixels[i][j]), sizeof(RGBTRIPLE));
        }
        file.ignore(padding); // Pomijanie paddingu
    }
    return pixels; // Funkcja powinna zwrócić wektor pikseli
}

void saveBMP(std::ofstream& file, const std::vector<std::vector<RGBTRIPLE>>& pixels) {
    int height = pixels.size();
    int width = pixels[0].size();
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // Wypełnienie nagłówków
    fileHeader.bfType = 0x4D42; // 'BM' w kodzie ASCII
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBTRIPLE) * width + padding) * height;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24; // 24-bitowy obraz
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = (sizeof(RGBTRIPLE) * width + padding) * height;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    // Zapis nagłówków
    file.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    // Zapis danych pikseli
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            file.write(reinterpret_cast<const char*>(&pixels[i][j]), sizeof(RGBTRIPLE));
        }
        // Zapis paddingu
        char paddingBytes[3] = {0, 0, 0};
        file.write(paddingBytes, padding);
    }
}

int main(int argc, char *argv[]) {
    std::ifstream file(argv[1], std::ios::binary);
    std::ofstream ofile((std::string(argv[1]) + "-proccesed.bmp"), std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open input file" << std::endl;
        return 1;
    }
    if (!ofile) {
        std::cerr << "Unable to open output file" << std::endl;
        return 1;
    }

    std::vector<std::vector<RGBTRIPLE>> img = readBMP(file);
    if (img.size() < 1) return 1;
    std::vector<std::vector<RGBTRIPLE>> compressed_img;
    for (int x = 0; x < img.size()-1; x += 2) {
        compressed_img.push_back(std::vector<RGBTRIPLE>());
        auto& last_row = compressed_img.back();
        for (int y = 0; y < img[0].size()-1; y += 2) {
            RGBTRIPLE new_pixel;
            new_pixel.rgbtBlue = static_cast<uint8_t>((img[x][y].rgbtBlue + img[x+1][y].rgbtBlue + img[x][y+1].rgbtBlue + img[x+1][y+1].rgbtBlue) / 4);
            new_pixel.rgbtRed = static_cast<uint8_t>((img[x][y].rgbtRed + img[x+1][y].rgbtRed + img[x][y+1].rgbtRed + img[x+1][y+1].rgbtRed) / 4);
            new_pixel.rgbtGreen = static_cast<uint8_t>((img[x][y].rgbtGreen + img[x+1][y].rgbtGreen + img[x][y+1].rgbtGreen + img[x+1][y+1].rgbtGreen) / 4);
            last_row.push_back(new_pixel);
        }
    }

    saveBMP(ofile, compressed_img);

    file.close();
    ofile.close();

    return 0;
}
