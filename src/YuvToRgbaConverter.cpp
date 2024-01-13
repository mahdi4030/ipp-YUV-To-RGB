#include "YuvToRgbaConverter.h"

#include <ippi.h>
#include <ipps.h>
#include <ippcore.h>
#include <ippcc.h>
#include <iostream>
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

namespace ui_engine {

    YuvToRgbaConverter::YuvToRgbaConverter(int inputWidth,
                                           int inputHeight,
                                           int outputWidth,
                                           int outputHeight)
          : m_inputWidth(inputWidth),
            m_inputHeight(inputHeight),
            m_outputWidth(outputWidth),
            m_outputHeight(outputHeight),
            m_inputStep(inputWidth*YUV_BPP),
            m_outputStep(outputWidth*RGBA_BPP),
            m_convertedStep(0),
            m_scaledStep(0),
            m_pConverted(nullptr),
            m_pScaled(nullptr)
    {
    }

    bool YuvToRgbaConverter::Initialize() {
        m_pConverted = ippiMalloc_8u_C3(m_outputWidth, m_outputHeight, &m_convertedStep);
        if (m_pConverted == nullptr) {
            std::cerr << "Initialize error: Allocting converting buffer" << '\n';
            return false;
        }

        m_pScaled = ippiMalloc_8u_C3(m_outputWidth, m_outputHeight, &m_scaledStep);
        if (m_pScaled == nullptr) {
            std::cerr << "Initialize error: Allocatigin scaling buffer" << '\n';
            ippFree(m_pConverted);
            return false;
        }

        if (!CalculateResizeSpec()) {
            std::cerr << "Initialize error: CalculateResizeSpec" << '\n';
            ippFree(m_pConverted);
            ippFree(m_pScaled);
            return false;
        }

        return true;
    }

    void YuvToRgbaConverter::Finalize() {
        ippFree(m_pOneBuffer);
        ippFree(m_pScaled);
        ippFree(m_pConverted);
    }

    bool YuvToRgbaConverter::Convert(char *input, char *output) {
        if (!m_pScaled || !m_pConverted) {
            return false;
        }

        // Resize YUV422 to FHD
        ippiResizeYCbCr422_8u_C2R(
            reinterpret_cast<Ipp8u *>(input), {m_inputWidth, m_inputHeight},
            m_inputStep, {0,0, m_inputWidth, m_inputHeight},
            reinterpret_cast<Ipp8u*>(m_pScaled), m_scaledStep,
            {m_outputWidth, m_outputHeight},
            ippNearest, reinterpret_cast<Ipp8u*>(m_pOneBuffer));


        // Convert YUV FHD to RGB FHD
        IppStatus status = ippiYUV422ToRGB_8u_C2C3R(reinterpret_cast<Ipp8u*>(m_pScaled), m_scaledStep,
                                                    reinterpret_cast<Ipp8u*>(m_pConverted), m_convertedStep,
                                                    {m_outputWidth, m_outputHeight});


        // Copy the scaled image back to output as RGBA
        ippiCopy_8u_C3AC4R(reinterpret_cast<Ipp8u*>(m_pConverted), m_convertedStep,
                           reinterpret_cast<Ipp8u*>(output), m_outputStep,
                           {m_outputWidth, m_outputHeight});

        return (status == ippStsNoErr);
    }

    bool YuvToRgbaConverter::CalculateResizeSpec() {
        Ipp32s bufSize = 0;

        ippiResizeYCbCr422GetBufSize({0, 0, m_inputWidth, m_inputHeight}, {m_outputWidth, m_outputHeight}, ippNearest, &bufSize);
        m_pOneBuffer = ippsMalloc_8u(bufSize);
        if (!m_pOneBuffer) {
            return false;
        }

        return true;
    }
}
