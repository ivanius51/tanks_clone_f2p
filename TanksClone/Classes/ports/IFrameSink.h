#pragma once

#include <cstdint>
#include <string>

namespace game::ports
{
/**
 * Port: takes the rendered frames of a run and turns them into a file (a video, a set of pictures).
 * Frames are tightly packed RGBA rows, top row first. Close(false) throws the result away.
 */
class IFrameSink
{
public:
	virtual ~IFrameSink() = default;

	virtual bool Open(const std::string& InPath, int InWidth, int InHeight, int InFramesPerSecond) = 0;
	virtual bool PushFrame(const std::uint8_t* InRgba) = 0;
	virtual void Close(bool bInKeep) = 0;
	virtual bool IsOpen() const = 0;
};
} // namespace game::ports
