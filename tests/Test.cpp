#include <YuvToRgbaConverter.h>

#include <gtk/gtk.h>
#include <jpeglib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

namespace {

    constexpr uint32_t FHD_WIDTH = 1920;
    constexpr uint32_t FHD_HEIGHT = 1080;
    constexpr uint32_t UHD_WIDTH = FHD_WIDTH * 2;
    constexpr uint32_t UHD_HEIGHT = FHD_HEIGHT * 2;

    constexpr uint32_t RGBA_BITS_PER_CHANNEL = 8;
    constexpr uint32_t RGBA_BYTES_PER_CHANNEL = 4;
    constexpr size_t RGBA_FHD_FRAME_SIZE = FHD_WIDTH * FHD_HEIGHT * RGBA_BYTES_PER_CHANNEL;

    constexpr uint32_t YUV_BYTES_PER_CHANNEL = 2;
    constexpr size_t YUV_UHD_FRAME_SIZE = UHD_WIDTH * UHD_HEIGHT * YUV_BYTES_PER_CHANNEL;

    constexpr uint8_t PixelToY(uint8_t r, uint8_t g, uint8_t b)  {
        return 16 + ((66 * r + 129 * g + 25 * b) >> 8);
    }

    constexpr uint32_t PixelToU(uint32_t r, uint32_t g, uint32_t b)  {
        return 128 + ((-38 * r - 74 * g + 112 * b) >> 8);
    }

    constexpr uint32_t PixelToV(uint32_t r, uint32_t g, uint32_t b)  {
        return 128 + ((127 * r - 106 * g - 21 * b) >> 8);
    }
}

std::vector<char> ReadJPEG(const char *filename) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE *infile;
    JSAMPARRAY buffer;
    int row_stride;

    if ((infile = fopen(filename, "rb")) == NULL) {
        std::cerr << "Error opening JPEG file " << filename << "!" << std::endl;
        return {};
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    (void) jpeg_read_header(&cinfo, TRUE);

    (void) jpeg_start_decompress(&cinfo);
    row_stride = cinfo.output_width * cinfo.output_components;

    buffer = (*cinfo.mem->alloc_sarray)
             ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    std::vector<char> rgbData;
    rgbData.reserve(cinfo.output_width * cinfo.output_height * 3);

    while (cinfo.output_scanline < cinfo.output_height) {
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        for (JDIMENSION col = 0; col < cinfo.output_width; col++) {
            rgbData.push_back(buffer[0][col * 3]);       // R
            rgbData.push_back(buffer[0][col * 3 + 1]);   // G
            rgbData.push_back(buffer[0][col * 3 + 2]);   // B
        }
    }

    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return rgbData;
}

void DisplayRgbFrame(const char* buffer, uint32_t width, uint32_t height) {
    GtkWidget* window;
    GtkWidget* image;

    gtk_init(NULL, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
        (const guchar*)buffer,          // data
        GDK_COLORSPACE_RGB,             // colorspace
        true,                           // has alpha?
        RGBA_BITS_PER_CHANNEL,          // bits per sample
        width,                          // width
        height,                         // height
        width * RGBA_BYTES_PER_CHANNEL, // rowstride
        NULL,                           // destroy function
        NULL                            // destroy data
    );

    gdk_pixbuf_save (pixbuf, "ConvertYUV2RGBA.jpg", "jpeg", NULL, "quality", "100", NULL);

    // image = gtk_image_new_from_pixbuf(pixbuf);
    // gtk_container_add(GTK_CONTAINER(window), image);

    // gtk_widget_show_all(window);

    // gtk_main();

}

bool ConvertYUV2RGBA_IPP(std::vector<char> &input, std::vector<char> &output) {
    ui_engine::YuvToRgbaConverter converter(UHD_WIDTH, UHD_HEIGHT, FHD_WIDTH, FHD_HEIGHT);

    if (!converter.Initialize()) {
        std::cerr << "Converter initialization failed" << '\n';
        return false;
    }

    const uint32_t REPEATS = 100;
    uint64_t total_microseconds = 0;
    uint64_t max_microseconds = 0;

    bool res = true;
    /*
    auto t1 = high_resolution_clock::now();
    bool bConvert = converter.Convert(input.data(), output.data());
    auto t2 = high_resolution_clock::now();
    auto current_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "YuvToRgbaConverter::Convert = " << current_microseconds/1000 << "." << current_microseconds%1000 << "ms\n";
    if (!bConvert) {
        std::cerr << "Conversion failed" << '\n';
        res = false;
    }
    */
    

    for (uint32_t r = 0; r < REPEATS ; ++r) {
        auto tic = std::chrono::high_resolution_clock::now();
        if (!converter.Convert(input.data(), output.data())) {
            std::cerr << "Conversion failed" << '\n';
            res = false;
        }

        auto toc = std::chrono::high_resolution_clock::now();
        auto current_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();

        total_microseconds += current_microseconds;
        max_microseconds = (max_microseconds > current_microseconds) ? max_microseconds : current_microseconds;
    }

    total_microseconds /= REPEATS;
    long long avg_ms = total_microseconds / 1000;
    long long avg_us = total_microseconds % 1000;
    long long max_ms = max_microseconds / 1000;
    long long max_us = max_microseconds % 1000;

    std::cout << "Conversion done in " << avg_ms << '.' << avg_us << " average time" << '\n';
    std::cout << "Maximum:           " << max_ms << '.' << max_us << " average time" << '\n';

    converter.Finalize();

    return res;
}

std::vector<char> ConvertRGB2YUV422_UHD_Naive(const std::vector<char> &rgbaData) {
    std::vector<char> yuvData(rgbaData.size()*2/3);

    char *ptr = &yuvData[0];
    for (size_t i = 0; i < rgbaData.size(); i += 6) {
        char R1 = rgbaData[i];
        char G1 = rgbaData[i + 1];
        char B1 = rgbaData[i + 2];
        char R2 = rgbaData[i + 3];
        char G2 = rgbaData[i + 4];
        char B2 = rgbaData[i + 5];

        *ptr++ = PixelToY(R1, G1, B1);
        *ptr++ = (PixelToU(R1, G1, B1) + PixelToU(R2, G2, B2)) / 2;
        *ptr++ = PixelToY(R2, G2, B2);
        *ptr++ = (PixelToV(R1, G1, B1) + PixelToV(R2, G2, B2)) / 2;
    }

    return yuvData;
}

std::vector<char> Convert_YUV422_UHD_to_RGBA_FHD_Naive(const std::vector<char>& yuy2) {
	const int A = 255;
    std::vector<char> rgba(RGBA_BYTES_PER_CHANNEL * FHD_WIDTH * FHD_HEIGHT);

    for (size_t y = 0; y < UHD_HEIGHT; y += 2) {
        for (size_t x = 0; x < UHD_WIDTH; x++) {
            const size_t i = y*UHD_WIDTH*2 + x*2;
            uint8_t Y0 = yuy2[i];
            uint8_t U = yuy2[i + 1];
            uint8_t Y1 = yuy2[i + 2];
            uint8_t V = yuy2[i + 3];

            int R0 = Y0 + 1.402 * (V - 128);
            int G0 = Y0 - 0.344136 * (U - 128) - 0.714136 * (V - 128);
            int B0 = Y0 + 1.772 * (U - 128);

            int R1 = Y1 + 1.402 * (V - 128);
            int G1 = Y1 - 0.344136 * (U - 128) - 0.714136 * (V - 128);
            int B1 = Y1 + 1.772 * (U - 128);

            // clamp values to [0, 255]
            const size_t j = x*2 + y*FHD_WIDTH*4/2;
            rgba[j] = std::clamp(R0, 0, 255);
            rgba[j + 1] = std::clamp(G0, 0, 255);
            rgba[j + 2] = std::clamp(B0, 0, 255);
            rgba[j + 3] = std::clamp(A, 0, 255);
        }
    }

    return rgba;
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << '\n';
        std::cout << "{} <jpeg>" << argv[0] << '\n';
        return 0;
    }

    std::vector<char> originalRgbFrame = ReadJPEG(argv[1]);
    std::vector<char> yuvFrame = ConvertRGB2YUV422_UHD_Naive(originalRgbFrame);
#ifdef NAIVE_CONVERSION
    std::vector<char> resultRgbaFrame = Convert_YUV422_UHD_to_RGBA_FHD_Naive(yuvFrame);
#else
    std::vector<char> resultRgbaFrame(RGBA_FHD_FRAME_SIZE, char(255));
    bool bYUB2RGBA = ConvertYUV2RGBA_IPP(yuvFrame, resultRgbaFrame);
    if (!bYUB2RGBA) {
        std::cerr << "Conversion failed" << '\n';
        return 1;
    }

#endif

    DisplayRgbFrame(resultRgbaFrame.data(), FHD_WIDTH, FHD_HEIGHT);

    return 0;
}
