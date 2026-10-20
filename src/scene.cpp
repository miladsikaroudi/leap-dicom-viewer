#include "leapviewer/scene.hpp"

#include <stdexcept>

#include <vtkColorTransferFunction.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolumeProperty.h>

namespace leapviewer {

vtkSmartPointer<vtkAlgorithm> read_dicom_series(const std::string& directory) {
    auto reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName(directory.c_str());
    reader->Update();
    int dims[3];
    reader->GetOutput()->GetDimensions(dims);
    if (dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0) {
        throw std::runtime_error("no readable DICOM series in " + directory);
    }
    return reader;
}

vtkSmartPointer<vtkVolume> make_volume(vtkAlgorithmOutput* image) {
    vtkNew<vtkSmartVolumeMapper> mapper;
    mapper->SetBlendModeToComposite();
    mapper->SetInputConnection(image);
    mapper->Update();

    vtkNew<vtkPiecewiseFunction> opacity;
    opacity->AddPoint(0.0, 0.0);
    opacity->AddPoint(400.0, 1.0);

    vtkNew<vtkColorTransferFunction> color;
    color->AddRGBPoint(-10.0, 0.0, 0.0, 0.0);
    color->AddRGBPoint(200.0, 0.5, 0.5, 0.5);
    color->AddRGBPoint(400.0, 1.0, 1.0, 1.0);

    vtkNew<vtkVolumeProperty> property;
    property->SetColor(color);
    property->SetScalarOpacity(opacity);
    property->SetInterpolationTypeToLinear();
    property->SetScalarOpacityUnitDistance(5.0);
    property->ShadeOn();
    property->SetAmbient(0.2);
    property->SetDiffuse(0.8);
    property->SetSpecular(0.5);
    property->SetSpecularPower(5.0);

    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);
    volume->SetProperty(property);
    double center[3];
    mapper->GetCenter(center);
    volume->SetOrigin(center);
    return volume;
}

}  // namespace leapviewer
