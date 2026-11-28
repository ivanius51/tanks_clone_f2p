#pragma once

#include <cstdio>
#include <string>

#include "ports/IFrameSink.h"

namespace game
{
/** Pipes raw frames into an ffmpeg process that writes an MP4; Open fails when ffmpeg is not installed. */
class FfmpegFrameSink final : public ports::IFrameSink
{
public:
	~FfmpegFrameSink() override;

	static bool IsAvailable();

	bool Open(const std::string& InPath, int InWidth, int InHeight, int InFramesPerSecond) override;
	bool PushFrame(const std::uint8_t* InRgba) override;
	void Close(bool bInKeep) override;
	bool IsOpen() const override { return Pipe != nullptr; }

private:
	std::FILE* Pipe = nullptr;
	std::string Path;
	size_t FrameBytes = 0;
};
} // namespace game
