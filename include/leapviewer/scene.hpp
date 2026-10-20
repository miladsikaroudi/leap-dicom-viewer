// VTK volume-rendering pipeline for a CT series.
#pragma once

#include <string>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkSmartPointer.h>
#include <vtkVolume.h>

namespace leapviewer {

// Reads every DICOM slice in `directory`. Throws std::runtime_error if the
// directory holds no readable series. Keep the returned reader alive for as
// long as the volume is rendered.
vtkSmartPointer<vtkAlgorithm> read_dicom_series(const std::string& directory);

// A composite-blended, shaded volume with a CT-style greyscale transfer
// function (air transparent, opaque from 400 HU), centred on its own origin
// so rotations turn it in place.
vtkSmartPointer<vtkVolume> make_volume(vtkAlgorithmOutput* image);

}  // namespace leapviewer
