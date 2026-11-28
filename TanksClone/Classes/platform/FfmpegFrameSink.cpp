#include "platform/FfmpegFrameSink.h"

#include <cstdlib>

namespace game
{
namespace
{
constexpr const char* QuietNull = "nul";
}

FfmpegFrameSink::~FfmpegFrameSink()
{
	Close(true);
}

bool FfmpegFrameSink::IsAvailable()
{
	const std::string Command = std::string("ffmpeg -version >") + QuietNull + " 2>&1";
	return std::system(Command.c_str()) == 0;
}

bool FfmpegFrameSink::Open(const std::string& InPath, const int InWidth, const int InHeight, const int InFramesPerSecond)
{
	Close(true);
	if ( InWidth <= 0 || InHeight <= 0 || !IsAvailable() )
	{
		return false;
	}
	Path = InPath;
	FrameBytes = static_cast<size_t>(InWidth) * static_cast<size_t>(InHeight) * 4;
	// The pad makes both sides even, which yuv420p needs.
	const std::string Command = "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s " + std::to_string(InWidth) + "x" + std::to_string(InHeight)
		+ " -r " + std::to_string(InFramesPerSecond) + " -i - -vf \"pad=ceil(iw/2)*2:ceil(ih/2)*2\" -c:v libx264 -preset veryfast -pix_fmt yuv420p \"" + Path + "\"";
#if defined(_WIN32)
	Pipe = _popen(Command.c_str(), "wb");
#else
	Pipe = popen(Command.c_str(), "w");
#endif
	return Pipe != nullptr;
}

bool FfmpegFrameSink::PushFrame(const std::uint8_t* const InRgba)
{
	return Pipe && std::fwrite(InRgba, 1, FrameBytes, Pipe) == FrameBytes;
}

void FfmpegFrameSink::Close(const bool bInKeep)
{
	if ( !Pipe )
	{
		return;
	}
#if defined(_WIN32)
	_pclose(Pipe);
#else
	pclose(Pipe);
#endif
	Pipe = nullptr;
	if ( !bInKeep )
	{
		std::remove(Path.c_str());
	}
}
} // namespace game
