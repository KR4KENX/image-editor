#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#pragma pack(push, 1)

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

#pragma pack(pop)

int max(std::vector<int> nums){
    int res = -1;
    for (int num : nums){
        res = std::max(res, num);
    }
    return res;
}

int clamp(int value, int min, int max) {
    return std::max(min, std::min(max, value));
}

std::vector<std::vector<RGBTRIPLE>> readBMP(std::ifstream& file){
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42) {
        std::cerr << "This is not a BMP file" << std::endl;
        std::vector<std::vector<RGBTRIPLE>> blank_bmp;
        return blank_bmp;
    }

    file.seekg(fileHeader.bfOffBits, std::ios::beg);

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    std::vector<std::vector<RGBTRIPLE>> pixels(height, std::vector<RGBTRIPLE>(width));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            file.read(reinterpret_cast<char*>(&pixels[i][j]), sizeof(RGBTRIPLE));
        }
        file.ignore(padding); 
    }
    return pixels;
}
void colorBMP(std::vector<std::vector<RGBTRIPLE>>& img, bool inverseColors, int totalPixels, int x, int y, int sumBlue, int sumGreen, int sumRed){
            if (inverseColors){
                img[x][y].rgbtBlue = static_cast<uint8_t>(abs(max({sumRed, clamp(sumBlue+10, 0, 255), sumGreen})) / totalPixels);
                img[x][y].rgbtGreen = static_cast<uint8_t>(abs(max({sumRed, sumBlue, clamp(sumGreen+10, 0, 255)})) / totalPixels);
                img[x][y].rgbtRed = static_cast<uint8_t>(abs(max({clamp(sumRed+10, 0, 255), sumBlue, sumGreen})) / totalPixels);
            }
            else{
                img[x][y].rgbtBlue = static_cast<uint8_t>(sumBlue / totalPixels);
                img[x][y].rgbtGreen = static_cast<uint8_t>(sumGreen / totalPixels);
                img[x][y].rgbtRed = static_cast<uint8_t>(sumRed / totalPixels);
            }
}

void sepiaBMP(std::vector<std::vector<RGBTRIPLE>>& img){
    const std::vector<std::vector<double>> transform_vector = {
        {0.393, 0.769, 0.189},
        {0.349, 0.686, 0.168},
        {0.272, 0.534, 0.131}
    };
    for (int x = 0; x < img.size()-1; x++) {
        for (int y = 0; y < img[0].size()-1; y++) {
                double newRed = 0.0, newGreen = 0.0, newBlue = 0.0;
                for (int i = 0; i < 3; i++) {
                newRed += img[x][y].rgbtRed * transform_vector[i][0];
                newGreen += img[x][y].rgbtGreen * transform_vector[i][1];
                newBlue += img[x][y].rgbtBlue * transform_vector[i][2];
                }
            img[x][y].rgbtRed = static_cast<uint8_t>(clamp(int(newRed), 0, 255));
            img[x][y].rgbtGreen = static_cast<uint8_t>(clamp(int(newGreen), 0, 255));
            img[x][y].rgbtBlue = static_cast<uint8_t>(clamp(int(newBlue), 0, 255));
        }
    }
}

void blurBMP(std::vector<std::vector<RGBTRIPLE>>& img){
    const std::vector<std::vector<double>> gauss_blur_matrix = {
        {0.015, 0.125, 0.015},
        {0.25, 0.0625, 0.25},
        {0.015, 0.125, 0.015}
    };
    std::vector<std::vector<RGBTRIPLE>> temp_img = img; 

    for (int x = 0; x < img.size()-1; x++) {
        for (int y = 0; y < img[0].size()-1; y++) {
            double blue = 0.0, green = 0.0, red = 0.0;

            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    int xi = x + i;
                    int yj = y + j;
                    if (xi >= 0 && xi < img.size() && yj >= 0 && yj < img[0].size()) {
                        blue += img[xi][yj].rgbtBlue * gauss_blur_matrix[i + 1][j + 1];
                        green += img[xi][yj].rgbtGreen * gauss_blur_matrix[i + 1][j + 1];
                        red += img[xi][yj].rgbtRed * gauss_blur_matrix[i + 1][j + 1];
                    }
                }
            }

            temp_img[x][y].rgbtBlue = static_cast<unsigned char>(blue);
            temp_img[x][y].rgbtGreen = static_cast<unsigned char>(green);
            temp_img[x][y].rgbtRed = static_cast<unsigned char>(red);
        }
    }
    img = temp_img;

}

std::vector<std::vector<RGBTRIPLE>> compressBMP(std::vector<std::vector<RGBTRIPLE>> img, float compressionScale, bool inverseColors, bool blur, bool sepia){
    std::vector<std::vector<RGBTRIPLE>> compressed_img;

    int new_width = int(img[0].size() / compressionScale);
    int new_height = int(img.size() / compressionScale);
    std::cout << new_width << " : " << new_height << std::endl;
    compressed_img.resize(new_height);
    if (compressionScale >= 1){
        for (int x = 0; x < new_height; ++x) {
            compressed_img[x].resize(new_width);
            for (int y = 0; y < new_width; ++y) {
                int sumBlue = 0, sumGreen = 0, sumRed = 0;
                for (int i = 0; i < compressionScale; ++i) {
                    for (int j = 0; j < compressionScale; ++j) {
                        sumBlue += img[x * compressionScale + i][y * compressionScale + j].rgbtBlue;
                        sumGreen += img[x * compressionScale + i][y * compressionScale + j].rgbtGreen;
                        sumRed += img[x * compressionScale + i][y * compressionScale + j].rgbtRed;
                    }
                }
                int totalPixels = compressionScale * compressionScale;
                colorBMP(compressed_img, inverseColors, totalPixels, x, y, sumBlue, sumGreen, sumRed);
            }
        }
    }
    else{
        for (int x = 0; x < new_height; ++x) {
            compressed_img[x].resize(new_width);
            for (int y = 0; y < new_width; ++y) {
                RGBTRIPLE origin_pixel = img[clamp(int(x*compressionScale), 0, img.size()-1)][clamp(int(y*compressionScale), 0, img.size()-1)];
                compressed_img[x][y].rgbtBlue = origin_pixel.rgbtBlue;
                compressed_img[x][y].rgbtGreen = origin_pixel.rgbtGreen;
                compressed_img[x][y].rgbtRed = origin_pixel.rgbtRed;
            }
        }
    }
    if (blur){
        blurBMP(compressed_img);
    }
    if (sepia){
        sepiaBMP(compressed_img);
    }

    return compressed_img;
}

void saveBMP(std::ofstream& file, const std::vector<std::vector<RGBTRIPLE>>& pixels) {
    int height = pixels.size();
    int width = pixels[0].size();
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBTRIPLE) * width + padding) * height;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = (sizeof(RGBTRIPLE) * width + padding) * height;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    file.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            file.write(reinterpret_cast<const char*>(&pixels[i][j]), sizeof(RGBTRIPLE));
        }
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

    const bool inverseColors = std::strcmp(argv[3], "y") ? false : true;
    const bool blur = std::strcmp(argv[4], "y") ? false : true;
    const bool sepia = std::strcmp(argv[5], "y") ? false : true;

    std::vector<std::vector<RGBTRIPLE>> compressed_img = compressBMP(img, std::stof(argv[2]), inverseColors, blur, sepia);

    saveBMP(ofile, compressed_img);

    file.close();
    ofile.close();

    return 0;
}
