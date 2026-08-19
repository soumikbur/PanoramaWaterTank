#include "VolumeCalculator.h"

#include <algorithm>
#include <cmath>

namespace Panorama::VolumeCalculator {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kLitersPerCubicMeter = 1000.0;

} // namespace

double calculateArea(double radiusMeters)
{
    if (!std::isfinite(radiusMeters) || radiusMeters <= 0.0) {
        return 0.0;
    }
    return kPi * radiusMeters * radiusMeters;
}

double calculateVolume(double areaSquareMeters, double heightMeters)
{
    if (!std::isfinite(areaSquareMeters) || !std::isfinite(heightMeters) || areaSquareMeters <= 0.0
        || heightMeters <= 0.0) {
        return 0.0;
    }
    return areaSquareMeters * heightMeters * kLitersPerCubicMeter;
}

double calculateHeight(double volumeLiters, double areaSquareMeters)
{
    if (!std::isfinite(volumeLiters) || !std::isfinite(areaSquareMeters) || areaSquareMeters <= 0.0) {
        return 0.0;
    }
    const double clampedVolume = std::max(0.0, volumeLiters);
    return (clampedVolume / kLitersPerCubicMeter) / areaSquareMeters;
}

double calculateFillPercentageRaw(double currentVolumeLiters, double maximumVolumeLiters)
{
    if (!std::isfinite(currentVolumeLiters) || !std::isfinite(maximumVolumeLiters) || maximumVolumeLiters <= 0.0) {
        return 0.0;
    }
    return (currentVolumeLiters / maximumVolumeLiters) * 100.0;
}

double clampPercentageForDisplay(double rawPercentage)
{
    if (!std::isfinite(rawPercentage)) {
        return 0.0;
    }
    return std::clamp(rawPercentage, 0.0, 100.0);
}

double calculateRemainingVolume(double maximumVolumeLiters, double currentVolumeLiters)
{
    if (!std::isfinite(maximumVolumeLiters) || !std::isfinite(currentVolumeLiters)) {
        return 0.0;
    }
    return std::max(0.0, maximumVolumeLiters - currentVolumeLiters);
}

} // namespace Panorama::VolumeCalculator
