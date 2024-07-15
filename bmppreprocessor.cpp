#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stack>
#include <cmath>
#include <array>
#include <variant>
#include<functional>
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

struct CurveContainer {
    int lineLenght;
    std::vector<std::array<int, 2>> linePixels;
};

#pragma pack(pop)

int max(std::vector<int> nums) {
    int res = -1;
    for (int num : nums) {
        res = std::max(res, num);
    }
    return res;
}

int clamp(int value, int min, int max) {
    return std::max(min, std::min(max, value));
}

std::vector<std::vector<RGBTRIPLE>> readBMP(std::ifstream& file) {
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

void colorBMP(std::vector<std::vector<RGBTRIPLE>>& img, bool inverseColors, int totalPixels, int x, int y, int sumBlue, int sumGreen, int sumRed) {
    if (inverseColors) {
        img[x][y].rgbtBlue = static_cast<uint8_t>(abs(max({sumRed, clamp(sumBlue + 10, 0, 255), sumGreen})) / totalPixels);
        img[x][y].rgbtGreen = static_cast<uint8_t>(abs(max({sumRed, sumBlue, clamp(sumGreen + 10, 0, 255)})) / totalPixels);
        img[x][y].rgbtRed = static_cast<uint8_t>(abs(max({clamp(sumRed + 10, 0, 255), sumBlue, sumGreen})) / totalPixels);
    }
    else {
        img[x][y].rgbtBlue = static_cast<uint8_t>(sumBlue / totalPixels);
        img[x][y].rgbtGreen = static_cast<uint8_t>(sumGreen / totalPixels);
        img[x][y].rgbtRed = static_cast<uint8_t>(sumRed / totalPixels);
    }
}

void sepiaBMP(std::vector<std::vector<RGBTRIPLE>>& img) {
    const std::vector<std::vector<double>> transform_vector = {
        {0.393, 0.769, 0.189},
        {0.349, 0.686, 0.168},
        {0.272, 0.534, 0.131}
    };
    for (int x = 0; x < img.size(); x++) {
        for (int y = 0; y < img[0].size(); y++) {
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

void blurBMP(std::vector<std::vector<RGBTRIPLE>>& img) {
    const std::vector<std::vector<double>> gauss_blur_matrix = {
        {0.015, 0.125, 0.015},
        {0.25, 0.0625, 0.25},
        {0.015, 0.125, 0.015}
    };
    std::vector<std::vector<RGBTRIPLE>> temp_img = img;

    for (int x = 0; x < img.size(); x++) {
        for (int y = 0; y < img[0].size(); y++) {
            double blue = 0.0, green = 0.0, red = 0.0;

            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
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

std::vector<std::vector<RGBTRIPLE>> shapeDetectorBMP(std::vector<std::vector<RGBTRIPLE>>& img, int diffToleration, int shapeOrigin[2]) {
    int height = img.size(), width = img[0].size();
    int originX = shapeOrigin[0], originY = shapeOrigin[1];
    std::vector<std::vector<RGBTRIPLE>> shape_detected_img(height, std::vector<RGBTRIPLE>(width));

    auto isWithinTolerance = [&](RGBTRIPLE a, RGBTRIPLE b) {
        return std::abs(a.rgbtBlue - b.rgbtBlue) <= diffToleration &&
               std::abs(a.rgbtGreen - b.rgbtGreen) <= diffToleration &&
               std::abs(a.rgbtRed - b.rgbtRed) <= diffToleration;
    };

    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::stack<std::pair<int, int>> stack;
    stack.push({ originX, originY });
    RGBTRIPLE originColor = img[originX][originY];

    while (!stack.empty()) {
        auto [x, y] = stack.top();
        stack.pop();

        if (x < 0 || y < 0 || x >= height || y >= width || visited[x][y] || !isWithinTolerance(img[x][y], originColor)) {
            continue;
        }

        visited[x][y] = true;

        shape_detected_img[x][y].rgbtRed = img[x][y].rgbtRed;
        shape_detected_img[x][y].rgbtGreen = img[x][y].rgbtGreen;
        shape_detected_img[x][y].rgbtBlue = img[x][y].rgbtBlue;

        stack.push({ x + 1, y });
        stack.push({ x - 1, y });
        stack.push({ x, y + 1 });
        stack.push({ x, y - 1 });
    }
    return shape_detected_img;
}

std::vector<std::vector<RGBTRIPLE>> contourBMP(std::vector<std::vector<RGBTRIPLE>>& img, int diffToleration, int skipRadius) {
    int height = img.size(), width = img[0].size();
    std::vector<std::vector<RGBTRIPLE>> mirror_img = img;
    blurBMP(mirror_img);
    auto isWithinTolerance = [&](RGBTRIPLE a, RGBTRIPLE b) {
        return std::abs(b.rgbtBlue - a.rgbtBlue) > diffToleration ||
               std::abs(b.rgbtRed - a.rgbtRed) > diffToleration ||
               std::abs(b.rgbtGreen - a.rgbtGreen) > diffToleration;
    };
    auto isEdge = [&](std::array<int, 2> p1, std::array<int, 2> p2){
    if (p1[0] < 0 || p2[0] < 0 || p1[0] >= height || p2[0] >= height || 
        p1[1] < 0 || p2[1] < 0 || p1[1] >= width || p2[1] >= width) return false;
    return isWithinTolerance(mirror_img[p1[0]][p1[1]], mirror_img[p2[0]][p2[1]]);
    };
    std::vector<std::vector<RGBTRIPLE>> contour_img(height, std::vector<RGBTRIPLE>(width));
    std::vector<std::vector<bool>> skipMatrix(height, std::vector<bool>(width, false));
    auto ignoreRadiusMatrixAgent = [height, width, skipRadius, &skipMatrix](std::array<int, 2> cords){
        for (int x = cords[0]-skipRadius; x <= cords[0]+skipRadius; x++){
            for (int y = cords[1]-skipRadius; y <= cords[1]+skipRadius; y++){
                if (x < 0 || x >= height || y < 0 || y >= width) continue;
                else{
                    skipMatrix[x][y] = true;
                }
            }
        }
    };
    for(int x = 0; x < height; x++){
        for (int y = 0; y < width; y++){
            if (
            !skipMatrix[x][y] &&
            (isEdge({x, y}, {x, y+1}) || 
            isEdge({x, y}, {x+1, y}) ||
            isEdge({x, y}, {x-1, y}) ||
            isEdge({x, y}, {x, y-1}) || 
            isEdge({x, y}, {x-1, y-1}) || 
            isEdge({x, y}, {x+1, y-1}) || 
            isEdge({x, y}, {x-1, y+1}) || 
            isEdge({x, y}, {x+1, y+1}))) {
                contour_img[x][y] = {255, 255, 255};
                ignoreRadiusMatrixAgent({x, y});
            }
    }
    }
    return contour_img;
}

std::vector<std::vector<RGBTRIPLE>> lineDetectorBMP(std::vector<std::vector<RGBTRIPLE>>& img, int maxBlankStreak, int sprayRadius, int lineLengthMinimum) {
    int height = img.size(), width = img[0].size();
    
    auto contoured_img = contourBMP(img, 35, 0);
    
    auto isPixelWhite = [&contoured_img, height, width](int x, int y) -> bool {
        if (x < 0 || x >= height || y < 0 || y >= width) return false;
        return (contoured_img[x][y].rgbtBlue == 255); // Assuming white pixels are (255, 255, 255)
    };
    
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    
    std::function<CurveContainer(std::array<int, 2>, int, std::vector<std::array<int, 2>>, int, std::array<int, 2>)> dfs;
    dfs = [&](std::array<int, 2> cords, int actualBlankStreak, std::vector<std::array<int, 2>> path, int actualPathSum, std::array<int, 2> slope) -> CurveContainer {
        if (actualBlankStreak >= maxBlankStreak) {
            CurveContainer curveInfo = { actualPathSum, path };
            return curveInfo;
        }
        
        bool isLineContinuous = false;
        for (int x = cords[0] + slope[0] - sprayRadius; x <= cords[0] + slope[0] + sprayRadius; x++) {
            for (int y = cords[1] + slope[1] - sprayRadius; y <= cords[1] + slope[1] + sprayRadius; y++) {
                if (isPixelWhite(x, y)) {
                    isLineContinuous = true;
                    visited[cords[0]][cords[1]] = true;
                    actualBlankStreak = 0;
                    break;
                }
            }
            if (isLineContinuous) break;
        }
        
        path.push_back(cords);
        
        return dfs({ cords[0] + slope[0], cords[1] + slope[1] }, actualBlankStreak + (isLineContinuous ? 0 : 1), path, actualPathSum + (isLineContinuous ? 1 : 0), slope);
    };
    
    std::vector<std::array<int, 2>> lines;
    
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            if (isPixelWhite(x, y) && !visited[x][y]) {
                std::vector<CurveContainer> searchedLines;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (!(dx == 0 && dy == 0)){
                            CurveContainer curveInfo = dfs({ x, y }, 0, { {x, y} }, 0, { dx, dy });
                            searchedLines.push_back(curveInfo);
                        }
                    }
                }
                CurveContainer bestCurve = {0, {}};

                bool foundLongestPair = false;
                int maxSumLength = 0;
                std::array<int, 2> bestPairIndexes;

                if (searchedLines[0].lineLenght + searchedLines[7].lineLenght > maxSumLength) {
                    maxSumLength = searchedLines[0].lineLenght + searchedLines[7].lineLenght;
                    bestPairIndexes = {0, 7};
                    foundLongestPair = true;
                }

                if (searchedLines[1].lineLenght + searchedLines[6].lineLenght > maxSumLength) {
                    maxSumLength = searchedLines[1].lineLenght + searchedLines[6].lineLenght;
                    bestPairIndexes = {1, 6};
                    foundLongestPair = true;
                }

                if (searchedLines[2].lineLenght + searchedLines[5].lineLenght > maxSumLength) {
                    maxSumLength = searchedLines[2].lineLenght + searchedLines[5].lineLenght;
                    bestPairIndexes = {2, 5};
                    foundLongestPair = true;
                }
                if (searchedLines[3].lineLenght + searchedLines[4].lineLenght > maxSumLength) {
                    maxSumLength = searchedLines[3].lineLenght + searchedLines[4].lineLenght;
                    bestPairIndexes = {3, 4};
                    foundLongestPair = true;
                }

                if (foundLongestPair && maxSumLength >= lineLengthMinimum) {
                    bestCurve.lineLenght = maxSumLength;
                    bestCurve.linePixels.clear();
                    for(auto pixel : searchedLines[bestPairIndexes[0]].linePixels){
                        bestCurve.linePixels.push_back(pixel);
                    }
                    for(auto pixel : searchedLines[bestPairIndexes[1]].linePixels){
                        bestCurve.linePixels.push_back(pixel);
                    }
                }
                std::cout << bestCurve.linePixels.size() << std::endl;

                for (auto pixel : bestCurve.linePixels){
                    lines.push_back(pixel);
                }
            }
        }
    }

    for (const auto& coords : lines) {
        int x = coords[0];
        int y = coords[1];
        if (x < 0 || x >= height || y < 0 || y >= width) continue;
        contoured_img[x][y].rgbtBlue = 0;
        contoured_img[x][y].rgbtGreen = 0;

    }
    
    return contoured_img;
}

std::vector<std::vector<RGBTRIPLE>> compressBMP(std::vector<std::vector<RGBTRIPLE>>& img, float compressionScale, bool inverseColors, bool blur, bool sepia) {
    std::vector<std::vector<RGBTRIPLE>> compressed_img;

    int new_width = int(img[0].size() / compressionScale);
    int new_height = int(img.size() / compressionScale);
    std::cout << new_width << " : " << new_height << std::endl;
    compressed_img.resize(new_height);
    if (compressionScale >= 1) {
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
    else {
        for (int x = 0; x < new_height; ++x) {
            compressed_img[x].resize(new_width);
            for (int y = 0; y < new_width; ++y) {
                RGBTRIPLE origin_pixel = img[clamp(int(x * compressionScale), 0, img.size() - 1)][clamp(int(y * compressionScale), 0, img[0].size() - 1)];
                compressed_img[x][y].rgbtBlue = origin_pixel.rgbtBlue;
                compressed_img[x][y].rgbtGreen = origin_pixel.rgbtGreen;
                compressed_img[x][y].rgbtRed = origin_pixel.rgbtRed;
            }
        }
    }
    if (blur) {
        blurBMP(compressed_img);
    }
    if (sepia) {
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
        char paddingBytes[3] = { 0, 0, 0 };
        file.write(paddingBytes, padding);
    }
}


void displayMenu() {
    std::cout << "=== BMP Image Processor ===" << std::endl;
    std::cout << "Enter the path to the BMP file: ";
}

int main() {
    displayMenu();

    std::string inputPath;
    std::cin >> inputPath;

    std::ifstream file(inputPath, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open input file" << std::endl;
        return 1;
    }

    std::vector<std::vector<RGBTRIPLE>> img = readBMP(file);
    file.close();

    if (img.size() < 1) {
        return 1;
    }

    float compressionScale;
    std::cout << "Enter compression scale: ";
    std::cin >> compressionScale;

    char inverseColorsChar, blurChar, sepiaChar, detectShapesChar, contourChar, lineDetectChar;
    std::cout << "Inverse colors? (y/n): ";
    std::cin >> inverseColorsChar;
    std::cout << "Apply blur? (y/n): ";
    std::cin >> blurChar;
    std::cout << "Apply sepia? (y/n): ";
    std::cin >> sepiaChar;
    std::cout << "Detect shapes? (y/n): ";
    std::cin >> detectShapesChar;
    std::cout << "Draw contour? (y/n): ";
    std::cin >> contourChar;
    std::cout << "Detect lines? (y/n): ";
    std::cin >> lineDetectChar;
    bool inverseColors = (inverseColorsChar == 'y');
    bool blur = (blurChar == 'y');
    bool sepia = (sepiaChar == 'y');
    bool detectShapes = (detectShapesChar == 'y');
    bool contour = (contourChar == 'y');
    bool detectLine = (lineDetectChar == 'y');

    std::vector<std::vector<RGBTRIPLE>> compressed_img = compressBMP(img, compressionScale, inverseColors, blur, sepia);

    if (detectShapes) {
        int diffTolerance;
        int origin[2];
        std::cout << "Enter difference tolerance: ";
        std::cin >> diffTolerance;
        std::cout << "Enter origin X: ";
        std::cin >> origin[0];
        std::cout << "Enter origin Y: ";
        std::cin >> origin[1];

        std::vector<std::vector<RGBTRIPLE>> shape_detected_img = shapeDetectorBMP(compressed_img, diffTolerance, origin);
        std::ofstream ofile_shape_detected(inputPath + "-shape-processed.bmp", std::ios::binary);
        if (!ofile_shape_detected) {
            std::cerr << "Unable to open output file" << std::endl;
            return 1;
        }
        saveBMP(ofile_shape_detected, shape_detected_img);
        ofile_shape_detected.close();
    }
    if (contour){
        int diffTolerance;
        int skipRadius;
        std::cout << "Enter difference tolerance: ";
        std::cin >> diffTolerance;
        std::cout << "Enter skip radius: ";
        std::cin >> skipRadius;

        std::vector<std::vector<RGBTRIPLE>> contour_img = contourBMP(compressed_img, diffTolerance, skipRadius);
        std::ofstream ofile_contour(inputPath + "-contour.bmp", std::ios::binary);
        if (!ofile_contour) {
            std::cerr << "Unable to open output file" << std::endl;
            return 1;
        }
        saveBMP(ofile_contour, contour_img);
        ofile_contour.close();
    }
    if(detectLine){
        std::vector<std::vector<RGBTRIPLE>> lineDetected_img = lineDetectorBMP(compressed_img, 40, 1, 100);
        std::ofstream ofile_line(inputPath + "-line.bmp", std::ios::binary);
        if (!ofile_line) {
            std::cerr << "Unable to open output file" << std::endl;
            return 1;
        }
        saveBMP(ofile_line, lineDetected_img);
        ofile_line.close();
    }

    std::ofstream ofile(inputPath + "-processed.bmp", std::ios::binary);
    if (!ofile) {
        std::cerr << "Unable to open output file" << std::endl;
        return 1;
    }

    saveBMP(ofile, compressed_img);
    ofile.close();

    return 0;
}
