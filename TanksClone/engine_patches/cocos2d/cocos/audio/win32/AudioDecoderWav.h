#pragma once

#include "audio/win32/AudioDecoder.h"
#include "base/CCData.h"

namespace cocos2d { namespace experimental {

/**
 * @brief Decodes uncompressed PCM wav files (8, 16, 24 or 32 bit integer, 32 bit float) to 16 bit PCM.
 */
class AudioDecoderWav : public AudioDecoder
{
public:
    virtual bool open(const char* path) override;
    virtual void close() override;
    virtual uint32_t read(uint32_t framesToRead, char* pcmBuf) override;
    virtual bool seek(uint32_t frameOffset) override;
    virtual uint32_t tell() const override;

protected:
    AudioDecoderWav();
    ~AudioDecoderWav();

    Data _fileData;
    const unsigned char* _samples = nullptr;
    uint32_t _sourceBytesPerSample = 0;
    bool _isFloat = false;
    uint32_t _position = 0;

    friend class AudioDecoderManager;
};

}} // namespace cocos2d { namespace experimental {
