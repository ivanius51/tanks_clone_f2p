#include "audio/win32/AudioDecoderWav.h"
#include "audio/win32/AudioMacros.h"
#include "platform/CCFileUtils.h"

#include <cstring>

#define LOG_TAG "AudioDecoderWav"

namespace cocos2d { namespace experimental {

namespace
{
    const uint16_t kFormatPcm = 1;
    const uint16_t kFormatFloat = 3;
    const uint16_t kFormatExtensible = 0xFFFE;

    uint32_t readU32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24); }
    uint16_t readU16(const unsigned char* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
}

AudioDecoderWav::AudioDecoderWav()
{
}

AudioDecoderWav::~AudioDecoderWav()
{
    close();
}

bool AudioDecoderWav::open(const char* path)
{
    _fileData = FileUtils::getInstance()->getDataFromFile(path);
    const unsigned char* bytes = _fileData.getBytes();
    const size_t size = static_cast<size_t>(_fileData.getSize());
    if (size < 12 || memcmp(bytes, "RIFF", 4) != 0 || memcmp(bytes + 8, "WAVE", 4) != 0)
    {
        ALOGE("%s is not a RIFF/WAVE file", path);
        return false;
    }

    uint16_t format = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    size_t offset = 12;
    while (offset + 8 <= size)
    {
        const unsigned char* chunk = bytes + offset;
        const uint32_t chunkSize = readU32(chunk + 4);
        const unsigned char* body = chunk + 8;
        const size_t available = size - offset - 8;
        if (memcmp(chunk, "fmt ", 4) == 0 && chunkSize >= 16 && available >= 16)
        {
            format = readU16(body);
            channels = readU16(body + 2);
            sampleRate = readU32(body + 4);
            bitsPerSample = readU16(body + 14);
            if (format == kFormatExtensible && chunkSize >= 26 && available >= 26)
                format = readU16(body + 24);
        }
        else if (memcmp(chunk, "data", 4) == 0)
        {
            _samples = body;
            dataSize = static_cast<uint32_t>(chunkSize < available ? chunkSize : available);
            break;
        }
        offset += 8 + static_cast<size_t>(chunkSize) + (chunkSize & 1);
    }

    _isFloat = format == kFormatFloat && bitsPerSample == 32;
    const bool isPcm = format == kFormatPcm && (bitsPerSample == 8 || bitsPerSample == 16 || bitsPerSample == 24 || bitsPerSample == 32);
    if (!_samples || channels == 0 || sampleRate == 0 || (!isPcm && !_isFloat))
    {
        ALOGE("%s: unsupported wav (format %u, %u bits, %u channels)", path, format, bitsPerSample, channels);
        close();
        return false;
    }

    _sourceBytesPerSample = bitsPerSample / 8;
    _sampleRate = sampleRate;
    _channelCount = channels;
    _bytesPerFrame = channels * sizeof(short);
    _totalFrames = dataSize / (_sourceBytesPerSample * channels);
    _position = 0;
    _isOpened = true;
    return true;
}

void AudioDecoderWav::close()
{
    _fileData.clear();
    _samples = nullptr;
    _isOpened = false;
}

uint32_t AudioDecoderWav::read(uint32_t framesToRead, char* pcmBuf)
{
    const uint32_t frames = framesToRead < _totalFrames - _position ? framesToRead : _totalFrames - _position;
    const uint32_t sampleCount = frames * _channelCount;
    const unsigned char* source = _samples + static_cast<size_t>(_position) * _channelCount * _sourceBytesPerSample;
    short* target = reinterpret_cast<short*>(pcmBuf);
    for (uint32_t i = 0; i < sampleCount; ++i, source += _sourceBytesPerSample)
    {
        if (_isFloat)
        {
            float value;
            memcpy(&value, source, sizeof(value));
            value = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
            target[i] = static_cast<short>(value * 32767.0f);
        }
        else if (_sourceBytesPerSample == 1)
        {
            target[i] = static_cast<short>((static_cast<int>(source[0]) - 128) << 8);
        }
        else
        {
            // Little-endian signed; the two most significant bytes are the 16 bit sample.
            target[i] = static_cast<short>(source[_sourceBytesPerSample - 2] | (source[_sourceBytesPerSample - 1] << 8));
        }
    }
    _position += frames;
    return frames;
}

bool AudioDecoderWav::seek(uint32_t frameOffset)
{
    if (frameOffset > _totalFrames)
        return false;
    _position = frameOffset;
    return true;
}

uint32_t AudioDecoderWav::tell() const
{
    return _position;
}

}} // namespace cocos2d { namespace experimental {
