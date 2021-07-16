#pragma once
#include <stdint.h>

#include "stb_image.h"

typedef  uint32_t COLORREF;

#define SUCCEEDED(x)  x == 0
#define RGB(r,g,b) ( ((COLORREF)(uint8_t)r)|((COLORREF)((uint8_t)g)<<8)|((COLORREF)((uint8_t)b)<<16) )


class CImage
{
    private:
    int width;
    int height;
    int channels;
    stbi_uc *img;

public:
    CImage():
       width(0)
     , height(0)
     , channels(0)
     , img(nullptr)
    { }

    ~CImage() { Destroy(); }

    int Load(char const *fname) {
        img = stbi_load(fname, &width, &height, &channels, 0);
        if (img != nullptr) {
            return 0;
        } else {
            return 1;
        }
    }

    int GetHeight() { return height; }
    int GetWidth() { return width; }
    COLORREF GetPixel(const int x, const int y) {
        if (img)
            return RGB(img[y * width * channels + x * channels + 0],
                       img[y * width * channels + x * channels + 1],
                       img[y * width * channels + x * channels + 2]);
    }

    void Destroy() {
        if(img)
            stbi_image_free(img);
        width = 0;
        height = 0;
        img = nullptr;
    }

};


void Usage();
int ConvertBin(uint8_t *dest);
int ConvertImage(uint8_t *dest);
void convertBitmap1(uint8_t *destp, CImage *img, bool t);
void convertBitmap2(uint8_t *destp, CImage *img, bool t);
