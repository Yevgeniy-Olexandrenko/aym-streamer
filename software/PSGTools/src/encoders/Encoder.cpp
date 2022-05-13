#include "Encoder.h"

bool Encoder::CheckFileExt(const Stream& stream, const std::string& ext) const
{
    auto check_ext = "." + ext;
    auto extension = stream.file.extension().string();
    std::for_each(extension.begin(), extension.end(), [](char& c) { c = ::tolower(c); });
    return (extension == check_ext);
}
