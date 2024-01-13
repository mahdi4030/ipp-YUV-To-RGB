#ifndef YUVTORGB_CONVERTER_H
#define YUVTORGB_CONVERTER_H

#include <cstdint>

namespace ui_engine {

    class YuvToRgbaConverter final {
        public:
            YuvToRgbaConverter() = delete;
            YuvToRgbaConverter(int inputWidth,
                              int inputHeight,
                              int outputWidth,
                              int outputHeight);
            YuvToRgbaConverter(const YuvToRgbaConverter& other) = delete;
            YuvToRgbaConverter& operator=(const YuvToRgbaConverter& other) = delete;
            YuvToRgbaConverter(YuvToRgbaConverter&& other) = delete;
            YuvToRgbaConverter& operator=(YuvToRgbaConverter&& other) = delete;
            ~YuvToRgbaConverter() = default;

            [[nodiscard]] bool Initialize();
            void Finalize();

            [[nodiscard]] bool Convert(char *input, char *output);

        private:
            bool CalculateResizeSpec();

            const int m_inputWidth;
            const int m_inputHeight;
            const int m_outputWidth;
            const int m_outputHeight;
            const int m_inputStep;
            const int m_outputStep;
            int m_convertedStep;
            int m_scaledStep;
            void* m_pConverted;
            void* m_pScaled;
            void* m_pOneBuffer;

            static constexpr uint32_t YUV_BPP = 2;
            static constexpr uint32_t RGBA_BPP = 4;
    };

}

#endif
