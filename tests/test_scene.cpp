// Builds the volume pipeline from a synthetic CT-like image (no DICOM files,
// no window) and checks the reader rejects a directory with no series.
#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkRTAnalyticSource.h>
#include <vtkVolumeProperty.h>

#include "leapviewer/scene.hpp"

int main() {
    int failures = 0;

    vtkNew<vtkRTAnalyticSource> source;  // a 21^3 analytic volume, values roughly 40..280
    source->Update();
    auto volume = leapviewer::make_volume(source->GetOutputPort());
    double bounds[6];
    volume->GetBounds(bounds);
    if (!(bounds[1] > bounds[0]) || !volume->GetProperty()->GetShade()) {
        std::fputs("make_volume produced an empty or unshaded volume\n", stderr);
        ++failures;
    }

    const auto empty = std::filesystem::temp_directory_path() / "leap-dicom-viewer-empty";
    std::filesystem::create_directories(empty);
    try {
        leapviewer::read_dicom_series(empty.string());
        std::fputs("read_dicom_series accepted an empty directory\n", stderr);
        ++failures;
    } catch (const std::runtime_error&) {
    }

    if (failures == 0) std::puts("all scene tests passed");
    return failures == 0 ? 0 : 1;
}
