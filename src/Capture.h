#pragma once

#include "ImageView.h"

#include <filesystem>

bool WritePngRgb(const std::filesystem::path& path, const ImageView& image);
void SaveCapture(const ImageView& image);
