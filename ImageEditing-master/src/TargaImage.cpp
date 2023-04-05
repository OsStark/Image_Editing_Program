///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// constants
const int           RED             = 0;                // red channel
const int           GREEN           = 1;                // green channel
const int           BLUE            = 2;                // blue channel
const unsigned char BACKGROUND[3]   = { 0, 0, 0 };      // background color


// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1 ; i <= s ; i++)
        res = (n - i + 1) * res / i ;

    return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
   data = new unsigned char[width * height * 4];
   ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char *d)
{
    int i;

    width = w;
    height = h;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
	    data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image) 
{
   width = image.width;
   height = image.height;
   data = NULL; 
   if (image.data != NULL) {
      data = new unsigned char[width * height * 4];
      memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char   *rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (! data)
	    return NULL;

    // Divide out the alpha
    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = i * width * 4;
	    int out_offset = i * width * 3;

	    for (j = 0 ; j < width ; j++)
        {
	        RGBA_To_RGB(data + (in_offset + j*4), rgb + (out_offset + j*3));
	    }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char *filename)
{
    TargaImage	*out_image = Reverse_Rows();

    if (! out_image)
	    return false;

    if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
	    cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
	    return false;
    }

    delete out_image;

    return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char *filename)
{
    unsigned char   *temp_data;
    TargaImage	    *temp_image;
    TargaImage	    *result;
    int		        width, height;

    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
	    width = height = 0;
	    return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);

    result = temp_image->Reverse_Rows();

    delete temp_image;

    return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{
    int intensity;

    for (int i = 0; i < width * height * 4; i = i + 4) {
        intensity = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
        data[i] = intensity;
        data[i + 1] = intensity;
        data[i + 2] = intensity;
    }
    return false;
}// To_Grayscale



///////////////////////////////////////////////////////////////////////////////
//
// Convert the image to an 8 bit image using uniform quantization. Return
// success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
    for (int i = 0; i < width * height * 4; i = i + 4) {
        data[i] = data[i] / 36 * 36;
        data[i + 1] = data[i + 1] / 36 * 36;
        data[i + 2] = data[i + 2] / 85 * 85;
    }

    return false;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Quant_Populosity()
{
    //uniform quantization to 32 shades of each primary
    for (int i = 0; i < width * height * 4; i = i + 4) {
        data[i] = data[i] / 8 * 8; //data[i] is the red color
        data[i + 1] = data[i + 1] / 8 * 8; //data[i+1] is the green color
        data[i + 2] = data[i + 2] / 8 * 8; //data[i+2] is the blue color
    }

    //do histogram
    int** colors_histogram = new int* [4];
    for (int i = 0; i < 4; i++) {
        colors_histogram[i] = new int[32768];
    }
    for (int i = 0; i < 32768; i++) {
        colors_histogram[0][i] = 0; //fill the counting row with 0s
    }

    for (int i = 0; i < width * height * 4; i = i + 4) {
        int red = data[i];
        int green = data[i+1];
        int blue = data[i+2];
        int index = red * 1024 / 8 + green * 32 / 8 + blue / 8; //index for the color of the current pixel
        colors_histogram[0][index]++; //add + 1
        colors_histogram[1][index] = red; //save color info
        colors_histogram[2][index] = green; //save color info
        colors_histogram[3][index] = blue; //save color info
    }

    //sort histogram. using insertion sort in ascending order. descending order would be better.
    int i, key, keyr, keyg, keyb, j;
    for (i = 1; i < 32768; i++)
    {
        key = colors_histogram[0][i];
        keyr = colors_histogram[1][i];
        keyg = colors_histogram[2][i];
        keyb = colors_histogram[3][i];
        j = i - 1;

        while (j >= 0 && colors_histogram[0][j] > key)
        {
            colors_histogram[0][j + 1] = colors_histogram[0][j];
            colors_histogram[1][j + 1] = colors_histogram[1][j];
            colors_histogram[2][j + 1] = colors_histogram[2][j];
            colors_histogram[3][j + 1] = colors_histogram[3][j];
            j = j - 1;
        }
        colors_histogram[0][j + 1] = key;
        colors_histogram[1][j + 1] = keyr;
        colors_histogram[2][j + 1] = keyg;
        colors_histogram[3][j + 1] = keyb;
    }

    //get top 256 in descending order. this step is useless. it is here because i am lazy
    int top256[4][256];
    for (int i = 0; i < 256; i++) {
        top256[0][i] = colors_histogram[0][32767 - i];
        top256[1][i] = colors_histogram[1][32767 - i];
        top256[2][i] = colors_histogram[2][32767 - i];
        top256[3][i] = colors_histogram[3][32767 - i];
    }

    //assign pixels to closest color in the top 256.
    for (int i = 0; i < width * height * 4; i = i + 4) {
        int closest_distance = top256[0][0];
        int closest_red = top256[1][0];
        int closest_green = top256[2][0];
        int closest_blue = top256[3][0];

        for (int j = 1; j < 256; j++) { //here the distance from the pixel to all the possible colors is calculated
            int distance = (data[i] - top256[1][j]) * (data[i] - top256[1][j]) 
                            + (data[i + 1] - top256[2][j]) * (data[i + 1] - top256[2][j])
                            + (data[i + 2] - top256[3][j]) * (data[i + 2] - top256[3][j]);

            if (distance < closest_distance) {
                closest_distance = distance; //the information of the closest color is saved
                closest_red = top256[1][j];
                closest_green = top256[2][j];
                closest_blue = top256[3][j];
            }
        }
        data[i] = closest_red; //the pixel is painted with the closest color
        data[i + 1] = closest_green;
        data[i + 2] = closest_blue;
    }

    for (int i = 0; i < 4; i++) {
        delete[] colors_histogram[i];
    }
    delete[] colors_histogram;

    return false;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
    int intensity;
    int pixelcolor;

    for (int i = 0; i < width * height * 4; i = i + 4) {
        intensity = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
        pixelcolor = (intensity >= 127) ? 255 : 0;

        data[i] = pixelcolor;
        data[i + 1] = pixelcolor;
        data[i + 2] = pixelcolor;
    }
    return false;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
    To_Grayscale();
    int noise;
    int intensity;
    int pixelcolor;

    for (int i = 0; i < width * height * 4; i = i + 4) {
        noise = rand() % 102 - 51;
        intensity = data[i] + noise;
        pixelcolor = (intensity > 127) ? 255 : 0;

        data[i] = pixelcolor;
        data[i + 1] = pixelcolor;
        data[i + 2] = pixelcolor;
    }
    return false;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
    float intensity;
    int pixelcolor;
    int size = 4 * (width * height + width + 1);
    float *floating_point = new float[size];

    float propagation[4] = { 0.4375, 0.0625, 0.3125, 0.1875 };

    //copy all the image data to a pointing float array
    for (int i = 0; i < width * height * 4; i = i + 4) {
        for (int j = 0; j < 3; j++) {
            float number = data[i + j];
            floating_point[i + j] = number / 255;
        }
    }
    
    for (int i = 0; i < width * height * 4; i = i + 4) {
        int current_height = i / (4 * width);
        if (current_height % 2 == 0) {
            intensity = 0.299 * floating_point[i] + 0.587 * floating_point[i + 1] + 0.114 * floating_point[i + 2];
            pixelcolor = (intensity >= 0.5) ? 255 : 0;

            data[i] = pixelcolor;
            data[i + 1] = pixelcolor;
            data[i + 2] = pixelcolor;
        }
        else {
            intensity = 0.299 * floating_point[4 * width * (current_height + 1) - i % (4 * width)] + 0.587 * floating_point[4 * width * (current_height + 1) - i % (4 * width) + 1] + 0.114 * floating_point[4 * width * (current_height + 1) - i % (4 * width) + 2];
            pixelcolor = (intensity >= 0.5) ? 255 : 0;

            data[4 * width * (current_height + 1) - i % (4 * width)] = pixelcolor;
            data[4 * width * (current_height + 1) - i % (4 * width) + 1] = pixelcolor;
            data[4 * width * (current_height + 1) - i % (4 * width) + 2] = pixelcolor;
        }

        for (int j = 0; j < 3; j++){
            if (current_height % 2 == 0) {
                float error = (pixelcolor == 255) ? floating_point[i + j] - 1 : floating_point[i + j];
                if (i % (4 * width) != 0) floating_point[i + 4 + j] = floating_point[i + 4 + j] + error * propagation[0];
                if (i % (4 * width) != 0) floating_point[i + 4 * width + 4 + j] = floating_point[i + 4 * width + 4 + j] + error * propagation[1];
                floating_point[i + 4 * width + j] = floating_point[i + 4 * width + j] + error * propagation[2];
                if (i % (4 * width) != 1) floating_point[i + 4 * width - 4 + j] = floating_point[i + 4 * width - 4 + j] + error * propagation[3];
            }
            else {
                float error = (pixelcolor == 255) ? floating_point[4 * width * (current_height + 1) - i % (4 * width) + j] - 1 : floating_point[4 * width * (current_height + 1) - i % (4 * width) + j];
                if (i % (4 * width) != 1) floating_point[4 * width * (current_height + 1) - i % (4 * width) - 4 + j] = floating_point[4 * width * (current_height + 1) - i % (4 * width) - 4 + j] + error * propagation[0];
                if (i % (4 * width) != 1) floating_point[4 * width * (current_height + 1) - i % (4 * width) + 4 * width - 4 + j] = floating_point[4 * width * (current_height + 1) - i % (4 * width) + 4 * width - 4 + j] + error * propagation[1];
                floating_point[4 * width * (current_height + 1) - i % (4 * width) + 4 * width + j] = floating_point[4 * width * (current_height + 1) - i % (4 * width) + 4 * width + j] + error * propagation[2];
                if (i % (4 * width) != 0) floating_point[4 * width * (current_height + 1) - i % (4 * width) + 4 * width + 4 + j] = floating_point[4 * width * (current_height + 1) - i % (4 * width) + 4 * width + 4 + j] + error * propagation[3];
            }
        }
    }

    //free memory
    delete[] floating_point;

    return false;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
    int intensity;
    int pixelcolor;
    int size = width * height;
    int *array_of_intensities = new int[size];
    int sum_of_intensities = 0;

    for (int i = 0; i < width * height * 4; i = i + 4) {
        intensity = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
        array_of_intensities[i/4] = intensity;
        sum_of_intensities = sum_of_intensities + intensity;
    }

    int average_intensity = sum_of_intensities / (width * height);

    sort(array_of_intensities, array_of_intensities + size);
    int threshold_intensity = array_of_intensities[size * (255-average_intensity) / 255];

    //printf("%d %d", average_intensity, threshold_intensity);

    for (int i = 0; i < width * height * 4; i = i + 4) {
        intensity = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
        pixelcolor = (intensity >= threshold_intensity) ? 255 : 0;

        data[i] = pixelcolor;
        data[i + 1] = pixelcolor;
        data[i + 2] = pixelcolor;
    }

    //free memory
    delete[] array_of_intensities;

    return false;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
    float cluster[4][4] = { {0.7059, 0.3529, 0.5882, 0.2353},
                            {0.0588, 0.9412, 0.8235, 0.4118},
                            {0.4706, 0.7647, 0.8824, 0.1176},
                            {0.1765, 0.5294, 0.2941, 0.6471} };
    int intensity;
    int pixelcolor;

    for (int j = 0; j < height; j = j + 1) {
        for (int i = 0; i < width; i = i + 1) {
            intensity = 0.299 * data[4*width*j + 4*i] + 0.587 * data[4*width*j + 4*i + 1] + 0.114 * data[4*width*j + 4*i + 2];
            pixelcolor = (intensity >= cluster[j%4][i%4]*255) ? 255 : 0;

            data[4*width*j + 4*i] = pixelcolor;
            data[4*width*j + 4*i + 1] = pixelcolor;
            data[4*width*j + 4*i + 2] = pixelcolor;
        }
    }

    return false;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    int original_color, closest_color, color_up, color_down;
    float error;
    int size = 4 * (width * height + width + 1);
    float* floating_point = new float[size];

    float propagation[4] = { 0.4375, 0.0625, 0.3125, 0.1875 };

    //copy all the image data to a pointing float array
    for (int i = 0; i < width * height * 4; i = i + 4) {
        for (int j = 0; j < 3; j++) {
            float number = data[i + j];
            floating_point[i + j] = number / 255;
        }
    }

    //work all pixels 1 by 1
    for (int i = 0; i < width * height * 4; i += 4) {
        int current_height = i / (4 * width);
        int mirrored_i = 4 * width * (current_height + 1) - i % (4 * width);
        int direction = (current_height % 2 == 0) ? 1 : -1; //on even heights the pointer is moving to the right and direction is 1
                                                            //on odd heights the pointer is moving to the left and direction is -1
        int used_i = (direction == 1) ? i : mirrored_i; //when moving to the right "i" is used. when moving to the left "mirrored_i" is used.
        
        for (int j = 0; j < 3; j++) {
            //calculate value for the color channel
            int step = (j == 2) ? 85 : 36;

            original_color = floating_point[used_i + j] * 255;
            color_down = original_color / step * step;
            color_up = ((original_color + step) / step * step) % 256;
            closest_color = (abs(original_color - color_down) < abs(original_color - color_up)) ? color_down : color_up;

            data[used_i + j] = closest_color;

            //calculate error
            error = original_color - closest_color;
            error = error / 255;

            //do error propagation for the color channel
            floating_point[used_i + 4 * direction + j] += error * propagation[0];
            floating_point[used_i + 4 * width + 4 * direction + j] += error * propagation[1];
            floating_point[used_i + 4 * width + j] += error * propagation[2];
            floating_point[used_i + 4 * width - 4 * direction + j] += error * propagation[3];
        }
    }

    //free memory
    delete[] floating_point;

    return false;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout <<  "Comp_Over: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0 ; i < width * height * 4 ; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i+1] = abs(rgb1[1] - rgb2[1]);
        data[i+2] = abs(rgb1[2] - rgb2[2]);
        data[i+3] = 255;
    }

    return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box()
{
    int sumValue;
    int sumWeight = 25;

    int box[5][5] = { {1,1,1,1,1},
                     {1,1,1,1,1},
                     {1,1,1,1,1},
                     {1,1,1,1,1},
                     {1,1,1,1,1}, };

    //do matrices
    int** image_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    int** result_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    for (int i = 0; i < width * 4 + 16; i++) {
        image_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
        result_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
    }
    //put 0 on the entire matrix
    for (int i = 0; i < 4 * width + 16; i++) {
        for (int j = 0; j < height + 16; j++) {
            image_matrix[i][j] = 0;
        }
    }

    //put values on their places
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {
            image_matrix[i + 8][j + 8] = data[i + j * width * 4];;
        }
    }

    //do the filter
    for (int i = 8; i < width * 4 + 8; i++) {
        for (int j = 8; j < height + 8; j++) {

            sumValue = 0;
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 5; y++) {
                    sumValue += box[x][y] * image_matrix[i + x * 4 - 8][j + y - 2];
                }
            }

            result_matrix[i][j] = sumValue / sumWeight;
        }
    }

    //put results in data
    for (int i = 0; i < width * 4; i++) {
        for (int j = 0; j < height; j++) {
            if ((i + j * width * 4) % 4 != 3) {
                data[i + j * width * 4] = result_matrix[i + 8][j + 8];
            }
        }
    }

    //free memory
    for (int i = 0; i < width * 4 + 16; i++) {
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
    int sumValue;
    int sumWeight = 81;

    int box[5][5] = { {1,2,3,2,1},
                    {2,4,6,4,2},
                    {3,6,9,6,3},
                    {2,4,6,4,2},
                    {1,2,3,2,1}, };

    //do matrices
    int** image_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    int** result_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    for (int i = 0; i < width * 4 + 16; i++) {
        image_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
        result_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
    }
    //put 0 on the entire matrix
    for (int i = 0; i < 4 * width + 16; i++) {
        for (int j = 0; j < height + 16; j++) {
            image_matrix[i][j] = 0;
        }
    }

    //put values on their places
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {
            image_matrix[i + 8][j + 8] = data[i + j * width * 4];;
        }
    }

    //do the filter
    for (int i = 8; i < width * 4 + 8; i++) {
        for (int j = 8; j < height + 8; j++) {

            sumValue = 0;
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 5; y++) {
                    sumValue += box[x][y] * image_matrix[i + x * 4 - 8][j + y - 2];
                }
            }

            result_matrix[i][j] = sumValue / sumWeight;
        }
    }

    //put results in data
    for (int i = 0; i < width * 4; i++) {
        for (int j = 0; j < height; j++) {
            if ((i + j * width * 4) % 4 != 3) {
                data[i + j * width * 4] = result_matrix[i + 8][j + 8];
            }
        }
    }

    //free memory
    for (int i = 0; i < width * 4 + 16; i++) {
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Filter_Bartlett

///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
    int sumValue;
    int sumWeight = 256;

    int box[5][5] = { {1,4,6,4,1},
                    {4,16,24,16,4},
                    {6,24,36,24,6},
                    {4,16,24,16,4},
                    {1,4,6,4,1}, };

    //do matrices
    int** image_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    int** result_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    for (int i = 0; i < width * 4 + 16; i++) {
        image_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
        result_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
    }
    //put 0 on the entire matrix
    for (int i = 0; i < 4 * width + 16; i++) {
        for (int j = 0; j < height + 16; j++) {
            image_matrix[i][j] = 0;
        }
    }

    //put values on their places
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {
            image_matrix[i + 8][j + 8] = data[i + j * width * 4];;
        }
    }

    //do the filter
    for (int i = 8; i < width * 4 + 8; i++) {
        for (int j = 8; j < height + 8; j++) {

            sumValue = 0;
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 5; y++) {
                    sumValue += box[x][y] * image_matrix[i + x * 4 - 8][j + y - 2];
                }
            }

            result_matrix[i][j] = sumValue / sumWeight;
        }
    }

    //put results in data
    for (int i = 0; i < width * 4; i++) {
        for (int j = 0; j < height; j++) {
            if ((i + j * width * 4) % 4 != 3) {
                data[i + j * width * 4] = result_matrix[i + 8][j + 8];
            }
        }
    }

    //free memory
    for (int i = 0; i < width * 4 + 16; i++) {
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N( unsigned int N )
{
    if (N > 29) return false; //if i use n bigger than 29, the numbers are too big and the calculations are wrong 
                              //(because "unsigned long long int" is not big enough for the numbers).
                              //the code should be written in a way that the calculations are done with aproximations
                              //instead of the exact number.

    unsigned long long int sumValue;
    unsigned long long int sumWeight = 0;

    //do the gauss NxN filter
    unsigned long long int** gauss = new unsigned long long int* [N - 1];
    for (int row = 0; row < N - 1; row++) {
        gauss[row] = new unsigned long long int[N];
        gauss[row][0] = 1;
        gauss[row][row + 1] = 1;
        //printf("1 ");
        for (int column = 1; column < row + 1; column++) {
            gauss[row][column] = gauss[row - 1][column - 1] + gauss[row - 1][column];
            //printf("%d ", gauss[row][column]);
        }
        //printf("1\n");
    }

    //for (int i = 0; i < N; i++) {
        //printf("%d ", gauss[N - 2][i]);
    //}

    //n by n matrix for the filter
    unsigned long long int** box = new unsigned long long int* [N];
    for (int i = 0; i < N; i++) {
        box[i] = new unsigned long long int[N];
    }
    for (int i = 0; i < N; i++) {
        box[0][i] = gauss[N - 2][i];
        box[i][0] = gauss[N - 2][i];
        box[N - 1][i] = gauss[N - 2][i];
        box[i][N - 1] = gauss[N - 2][i];
    }
    for (int row = 1; row < N - 1; row++) {
        for (int column = 1; column < N - 1; column++) {
            box[row][column] = box[row][0] * box[0][column];
        }
    }

    //printf("\n");

    //calculate total weight of the filter
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            //cout << box[row][column] << "\t";
            sumWeight += box[row][column];
        }
        //cout << endl;
    }
    //cout << "sum weight = " << sumWeight << endl;

    //create matrices
    unsigned long long int** image_matrix = new unsigned long long int* [4 * width];
    unsigned long long int** result_matrix = new unsigned long long int* [4 * width];
    for (int i = 0; i < width * 4; i++) {
        image_matrix[i] = new unsigned long long int[height];
        result_matrix[i] = new unsigned long long int[height];
    }

    //put data in image matrix
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {
            image_matrix[i][j] = data[i + j * width * 4];;
        }
    }

    //do the filter
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {

            sumValue = 0;
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < N; y++) {
                    int row = i + x * 4 - (N - 1) * 2;
                    int column = j + y - (N - 1) / 2;
                    if (row >= 0 && row < 4 * width && column >=0 && column < height)
                        sumValue += box[x][y] * image_matrix[row][column];
                }
            }

            result_matrix[i][j] = sumValue / sumWeight;
        }
    }

    //put results in data
    for (int i = 0; i < width * 4; i++) {
        for (int j = 0; j < height; j++) {
            if ((i + j * width * 4) % 4 != 3) {
                data[i + j * width * 4] = result_matrix[i][j];
            }
        }
    }

    //free memory
    for (int i = 0; i < N - 1; i++) {
        delete[] gauss[i];
    }
    for (int i = 0; i < N; i++) {
        delete[] box[i];
    }

    for (int i = 0; i < width * 4; i++) {
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] gauss;
    delete[] box;
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
    int sumValue;
    int sumWeight = 256;

    int box[5][5] = { {-1,-4,-6,-4,-1},
                    {-4,-16,-24,-16,-4},
                    {-6,-24,220,-24,-6},
                    {-4,-16,-24,-16,-4},
                    {-1,-4,-6,-4,-1}, };

    //do matrices
    int** image_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    int** result_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    for (int i = 0; i < width * 4 + 16; i++) {
        image_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
        result_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
    }
    //put 0 on the entire matrix
    for (int i = 0; i < 4 * width + 16; i++) {
        for (int j = 0; j < height + 16; j++) {
            image_matrix[i][j] = 0;
        }
    }

    //put values on their places
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {
            image_matrix[i + 8][j + 8] = data[i + j * width * 4];;
        }
    }

    //do the filter
    for (int i = 8; i < width * 4 + 8; i++) {
        for (int j = 8; j < height + 8; j++) {

            sumValue = 0;
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 5; y++) {
                    sumValue += box[x][y] * image_matrix[i + x * 4 - 8][j + y - 2];
                }
            }

            result_matrix[i][j] = sumValue / sumWeight;
        }
    }

    //put results in data
    for (int i = 0; i < width * 4; i++) {
        for (int j = 0; j < height; j++) {
            if ((i + j * width * 4) % 4 != 3) {
                if (result_matrix[i + 8][j + 8] > 0)
                    data[i + j * width * 4] = result_matrix[i + 8][j + 8];
                else
                    data[i + j * width * 4] = 0;
            }
        }
    }

    //free memory
    for (int i = 0; i < width * 4 + 16; i++) {
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
    int sumValue;
    int sumWeight = 256;

    int box[5][5] = { {-1,-4,-6,-4,-1},
                    {-4,-16,-24,-16,-4},
                    {-6,-24,220,-24,-6},
                    {-4,-16,-24,-16,-4},
                    {-1,-4,-6,-4,-1}, };

    //do matrices
    int** image_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    int** result_matrix = new int* [4 * width + 16]; //+ 16 so we can have 0s on the sides
    for (int i = 0; i < width * 4 + 16; i++) {
        image_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
        result_matrix[i] = new int[height + 16]; //+ 16 so we can have 0s on the sides
    }
    //put 0 on the entire matrix
    for (int i = 0; i < 4 * width + 16; i++) {
        for (int j = 0; j < height + 16; j++) {
            image_matrix[i][j] = 0;
        }
    }

    //put values on their places
    for (int i = 0; i < 4 * width; i++) {
        for (int j = 0; j < height; j++) {
            image_matrix[i + 8][j + 8] = data[i + j * width * 4];;
        }
    }

    //do the filter
    for (int i = 8; i < width * 4 + 8; i++) {
        for (int j = 8; j < height + 8; j++) {

            sumValue = 0;
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 5; y++) {
                    sumValue += box[x][y] * image_matrix[i + x * 4 - 8][j + y - 2];
                }
            }

            result_matrix[i][j] = sumValue / sumWeight;
        }
    }

    //put results in data
    for (int i = 0; i < width * 4; i++) {
        for (int j = 0; j < height; j++) {
            if ((i + j * width * 4) % 4 != 3) {
                if (result_matrix[i + 8][j + 8] > 0 && data[i + j * width * 4] + result_matrix[i + 8][j + 8] < 256)
                    data[i + j * width * 4] += result_matrix[i + 8][j + 8];
            }
        }
    }

    //free memory
    for (int i = 0; i < width * 4 + 16; i++) {
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{
    ClearToBlack();
    return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
    unsigned int N = 3;
    unsigned int sumValue;
    unsigned int sumWeight = 64;

    int box[3][3] = { {4,  8, 4},
                      {8, 16, 8},
                      {4,  8, 4}, };

    //create matrices
    unsigned int*** image_matrix = new unsigned int** [width];
    for (int i = 0; i < width; i++) {
        image_matrix[i] = new unsigned int*[height];
        for (int j = 0; j < height; j++) {
            image_matrix[i][j] = new unsigned int[3];
        }
    }

    unsigned int*** result_matrix = new unsigned int** [width / 2];
    for (int i = 0; i < width / 2; i++) {
        result_matrix[i] = new unsigned int* [height / 2];
        for (int j = 0; j < height / 2; j++) {
            result_matrix[i][j] = new unsigned int[3];
        }
    }

    //put data in image matrix
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < 3; k++) {
                image_matrix[i][j][k] = data[j * width * 4 + i * 4 + k];
            }
        }
    }

    //do the filter
    for (int i = 0; i < width / 2; i++) { //2*width is half of 4*width
        for (int j = 0; j < height / 2; j++) {
            for (int k = 0; k < 3; k++) {
                sumValue = 0;

                for (int x = 0; x < N; x++) {
                    for (int y = 0; y < N; y++) {

                        int row = 2 * i + x - (N - 1) / 2;
                        int column = 2 * j + y - (N - 1) / 2;

                        if (row >= 0 && row < width && column >= 0 && column < height)
                            sumValue += box[x][y] * image_matrix[row][column][k];
                    }
                }

                result_matrix[i][j][k] = sumValue / sumWeight;
            }        
        }
    }

    //half the image width and height
    int original_width = width;
    int original_height = height;
    width /= 2;
    height /= 2;

    //put results in data
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < 3; k++) {
                if ((j * width * 4 + i * 4 + k) % 4 != 3) {
                    data[j * width * 4 + i * 4 + k] = result_matrix[i][j][k];
                }
            }
        }
    }

    //free memory
    for (int i = 0; i < original_width; i++) {
        for (int j = 0; j < original_height; j++) {
            delete[] image_matrix[i][j];
        }
        delete[] image_matrix[i];
    }
    delete[] image_matrix;
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            delete[] result_matrix[i][j];
        }
        delete[] result_matrix[i];
    }
    delete[] result_matrix;

    return false;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
    unsigned int sumValue;
    const int sumWeight = 64;

    int box1[4][4] = {  {4,  8,  4,  0},
                        {8, 16,  8,  0},
                        {4,  8,  4,  0}, 
                        {0,  0,  0,  0}, };

    int box2[4][4] = {  {1,  3,  3,  1},
                        {3,  9,  9,  3},
                        {3,  9,  9,  3},
                        {1,  3,  3,  1}, };

    int box3[4][4] = {  {2,  4,  2,  0},
                        {6, 12,  6,  0},
                        {6, 12,  6,  0},
                        {2,  4,  2,  0}, };

    int box4[4][4] = {  {2,  6,  6,  2},
                        {4, 12, 12,  4},
                        {2,  6,  6,  2},
                        {0,  0,  0,  0}, };

    //create matrices
    unsigned int*** image_matrix = new unsigned int** [width];
    for (int i = 0; i < width; i++) {
        image_matrix[i] = new unsigned int* [height];
        for (int j = 0; j < height; j++) {
            image_matrix[i][j] = new unsigned int[3];
            for (int k = 0; k < 3; k++) {
                image_matrix[i][j][k] = data[j * width * 4 + i * 4 + k]; //put data in image matrix
            }
        }
    }

    unsigned int*** result_matrix = new unsigned int** [width * 2];
    for (int i = 0; i < width * 2; i++) {
        result_matrix[i] = new unsigned int* [height * 2];
        for (int j = 0; j < height * 2; j++) {
            result_matrix[i][j] = new unsigned int[3];
        }
    }

    //apply the filter
    for (int i = 0; i < width * 2; i++) {
        for (int j = 0; j < height * 2; j++) {
            for (int k = 0; k < 3; k++) {
                sumValue = 0;

                for (int x = -1; x <= 2; x += 1) { //4x4 filter matrix
                    for (int y = -1; y <= 2; y += 1) {
                        int row = i / 2 + x;
                        int column = j / 2 + y;

                        if (row >= 0 && row < width && column >= 0 && column < height) {

                            if (i % 2 == 0 && j % 2 == 0) { //i and j are even
                                sumValue += box1[x + 1][y + 1] * image_matrix[row][column][k]; //box1
                            }

                            if (i % 2 == 1 && j % 2 == 1) { //i and j are odd
                                sumValue += box2[x + 1][y + 1] * image_matrix[row][column][k]; //box2
                            }

                            if (i % 2 == 1 && j % 2 == 0) { //i is even and j is odd
                                sumValue += box3[x + 1][y + 1] * image_matrix[row][column][k]; //box3
                            }

                            if (i % 2 == 0 && j % 2 == 1) { //i is odd and j is even
                                sumValue += box4[x + 1][y + 1] * image_matrix[row][column][k]; //box4
                            }
                        }
                    }
                }

                result_matrix[i][j][k] = sumValue / sumWeight;
            }
        }
    }

    //double the image width and height
    int original_width = width;
    int original_height = height;
    width *= 2;
    height *= 2;
    delete[] data;
    data = new unsigned char[width * height * 4];

    //put results in data
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < 4; k++) {
                if ((j * width * 4 + i * 4 + k) % 4 != 3) {
                    data[j * width * 4 + i * 4 + k] = result_matrix[i][j][k];
                }
                else {
                    data[j * width * 4 + i * 4 + k] = 255;
                }
            }
        }
    }

    //free memory
    for (int i = 0; i < original_width; i++) {
        for (int j = 0; j < original_height; j++) {
            delete[] image_matrix[i][j];
        }
        delete[] image_matrix[i];
    }
    delete[] image_matrix;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            delete[] result_matrix[i][j];
        }
        delete[] result_matrix[i];
    }
    delete[] result_matrix;

    return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
    ClearToBlack();
    return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
    unsigned int sumValue;
    const int sumWeight = 64;
    float rad = angleDegrees * acos(0.0) / 90;

    int box1[4][4] = { {4,  8,  4,  0},
                        {8, 16,  8,  0},
                        {4,  8,  4,  0},
                        {0,  0,  0,  0}, };

    int box2[4][4] = { {1,  3,  3,  1},
                        {3,  9,  9,  3},
                        {3,  9,  9,  3},
                        {1,  3,  3,  1}, };

    int box3[4][4] = { {2,  4,  2,  0},
                        {6, 12,  6,  0},
                        {6, 12,  6,  0},
                        {2,  4,  2,  0}, };

    int box4[4][4] = { {2,  6,  6,  2},
                        {4, 12, 12,  4},
                        {2,  6,  6,  2},
                        {0,  0,  0,  0}, };

    //create matrices
    unsigned int*** image_matrix = new unsigned int** [width];
    unsigned int*** result_matrix = new unsigned int** [width];
    for (int i = 0; i < width; i++) {
        image_matrix[i] = new unsigned int* [height];
        result_matrix[i] = new unsigned int* [height];

        for (int j = 0; j < height; j++) {
            image_matrix[i][j] = new unsigned int[3];
            result_matrix[i][j] = new unsigned int[3];

            for (int k = 0; k < 3; k++) {
                image_matrix[i][j][k] = data[j * width * 4 + i * 4 + k]; //put data in image matrix
                result_matrix[i][j][k] = 0; //make result matrix all black
            }
        }
    }

    //apply the filter
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < 3; k++) {
                sumValue = 0;
                
                int offset_i = i - width / 2;
                int offset_j = j - height / 2;

                for (int x = -1; x <= 2; x += 1) { //4x4 filter matrix
                    for (int y = -1; y <= 2; y += 1) {

                        int row = offset_i * cos(-rad) - offset_j * sin(-rad) + x + width / 2; //+ width / 2 to remove the offset
                        int column = offset_i * sin(-rad) + offset_j * cos(-rad) + y + height / 2; //+ height / 2 to remove the offset
                        
                        if (row >= 0 && row < width && column >= 0 && column < height) {
                            if (i % 2 == 0 && j % 2 == 0) { //i and j are even
                                sumValue += box1[x + 1][y + 1] * image_matrix[row][column][k]; //box1
                            }

                            if (i % 2 == 1 && j % 2 == 1) { //i and j are odd
                                sumValue += box2[x + 1][y + 1] * image_matrix[row][column][k]; //box2
                            }

                            if (i % 2 == 1 && j % 2 == 0) { //i is even and j is odd
                                sumValue += box3[x + 1][y + 1] * image_matrix[row][column][k]; //box3
                            }

                            if (i % 2 == 0 && j % 2 == 1) { //i is odd and j is even
                                sumValue += box4[x + 1][y + 1] * image_matrix[row][column][k]; //box4
                            }
                        }
                    }
                }

                result_matrix[i][j][k] = sumValue / sumWeight;
            }
        }
    }

    //put results in data
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < 3; k++) {
                data[j * width * 4 + i * 4 + k] = result_matrix[i][j][k];
            }
        }
    }

    //free memory
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            delete[] image_matrix[i][j];
            delete[] result_matrix[i][j];
        }
        delete[] image_matrix[i];
        delete[] result_matrix[i];
    }
    delete[] image_matrix;
    delete[] result_matrix;

    return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
	    float	alpha_scale = (float)255 / (float)alpha;
	    int	val;
	    int	i;

	    for (i = 0 ; i < 3 ; i++)
	    {
	        val = (int)floor(rgba[i] * alpha_scale);
	        if (val < 0)
		    rgb[i] = 0;
	        else if (val > 255)
		    rgb[i] = 255;
	        else
		    rgb[i] = val;
	    }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char   *dest = new unsigned char[width * height * 4];
    TargaImage	    *result;
    int 	        i, j;

    if (! data)
    	return NULL;

    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = (height - i - 1) * width * 4;
	    int out_offset = i * width * 4;

	    for (j = 0 ; j < width ; j++)
        {
	        dest[out_offset + j * 4] = data[in_offset + j * 4];
	        dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
	        dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
	        dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
   int radius_squared = (int)s.radius * (int)s.radius;
   for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
      for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
         int x_loc = (int)s.x + x_off;
         int y_loc = (int)s.y + y_off;
         // are we inside the circle, and inside the image?
         if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
            int dist_squared = x_off * x_off + y_off * y_off;
            if (dist_squared <= radius_squared) {
               data[(y_loc * width + x_loc) * 4 + 0] = s.r;
               data[(y_loc * width + x_loc) * 4 + 1] = s.g;
               data[(y_loc * width + x_loc) * 4 + 2] = s.b;
               data[(y_loc * width + x_loc) * 4 + 3] = s.a;
            } else if (dist_squared == radius_squared + 1) {
               data[(y_loc * width + x_loc) * 4 + 0] = 
                  (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
               data[(y_loc * width + x_loc) * 4 + 1] = 
                  (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
               data[(y_loc * width + x_loc) * 4 + 2] = 
                  (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
               data[(y_loc * width + x_loc) * 4 + 3] = 
                  (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
            }
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
               unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
   radius(iradius),x(ix),y(iy),r(ir),g(ig),b(ib),a(ia)
{
}

