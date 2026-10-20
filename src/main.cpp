// leap-dicom-viewer: render a DICOM CT series and turn it with your hand.
//
//   leap-dicom-viewer <dicom-directory> [--size WIDTHxHEIGHT] [--screenshot out.png]
//
// With a Leap Motion controller (build option LEAP_DICOM_WITH_LEAP):
//   tilt your palm      rotate the volume
//   move palm up/down   move the camera closer or further
//   second hand in view freeze the volume
//   key-tap gesture     cycle the on-screen mode label
// Without one, the mouse rotates and zooms as in any VTK viewer.
// Press q or Escape to quit. --screenshot renders one frame to a PNG and exits.

#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkWindowToImageFilter.h>

#include "leapviewer/interaction.hpp"
#include "leapviewer/scene.hpp"
#ifdef LEAP_DICOM_WITH_LEAP
#include "leap_input.hpp"
#endif

namespace {

using namespace leapviewer;

const std::array<std::string, 3> kModeLabels{"Mode 1", "Mode 2", "Mode 3"};

struct Options {
    std::string dicom_dir;
    int width = 1000;
    int height = 1000;
    std::string screenshot;  // if set, render once to this PNG and exit
};

void usage(const char* argv0) {
    std::cerr << "usage: " << argv0 << " <dicom-directory> [--size WIDTHxHEIGHT] [--screenshot out.png]\n";
}

bool parse_args(int argc, char** argv, Options& out) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            return false;
        }
        if (arg == "--size" && i + 1 < argc) {
            if (std::sscanf(argv[++i], "%dx%d", &out.width, &out.height) != 2 || out.width <= 0 || out.height <= 0) {
                std::cerr << "--size expects WIDTHxHEIGHT, e.g. 1280x800\n";
                return false;
            }
        } else if (arg == "--screenshot" && i + 1 < argc) {
            out.screenshot = argv[++i];
        } else if (out.dicom_dir.empty() && arg.rfind("--", 0) != 0) {
            out.dicom_dir = arg;
        } else {
            std::cerr << "unexpected argument: " << arg << "\n";
            return false;
        }
    }
    return !out.dicom_dir.empty();
}

// Everything the render-thread callbacks need.
struct Viewer {
    SampleMailbox mailbox;
    Interaction interaction{static_cast<int>(kModeLabels.size())};
    vtkVolume* volume = nullptr;
    vtkCamera* camera = nullptr;
    vtkTextActor* label = nullptr;
    double camera_start_z = 0;
};

#ifdef LEAP_DICOM_WITH_LEAP
constexpr int kTimerMilliseconds = 10;

void on_timer(vtkObject* caller, unsigned long, void* client, void*) {
    auto& viewer = *static_cast<Viewer*>(client);
    const Interaction::Update update = viewer.interaction.step(viewer.mailbox.take());

    viewer.volume->RotateX(update.rotation.x);
    viewer.volume->RotateY(update.rotation.y);
    viewer.volume->RotateZ(update.rotation.z);
    if (update.move_camera) {
        double x, y, z;
        viewer.camera->GetPosition(x, y, z);
        viewer.camera->SetPosition(x, y, viewer.camera_start_z + update.camera_z_offset);
    }

    viewer.label->SetInput(kModeLabels[update.mode].c_str());
    if (update.flash) {
        viewer.label->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    } else {
        viewer.label->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
    }
    static_cast<vtkRenderWindowInteractor*>(caller)->Render();
}
#endif

void on_key(vtkObject* caller, unsigned long, void*, void*) {
    auto* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    const std::string key = interactor->GetKeySym() ? interactor->GetKeySym() : "";
    if (key == "q" || key == "Q" || key == "Escape") {
        interactor->TerminateApp();
    }
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_args(argc, argv, options)) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    vtkSmartPointer<vtkAlgorithm> reader;
    try {
        reader = read_dicom_series(options.dicom_dir);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    auto viewer = std::make_unique<Viewer>();
    vtkSmartPointer<vtkVolume> volume = make_volume(reader->GetOutputPort());

    vtkNew<vtkRenderer> renderer;
    renderer->AddViewProp(volume);
    renderer->ResetCamera();
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetClippingRange(1, 5000);

    vtkNew<vtkTextActor> label;
    label->SetInput(kModeLabels[0].c_str());
    label->SetPosition(10, 100);
    label->GetTextProperty()->SetFontSize(48);
    label->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
    renderer->AddActor2D(label);

    vtkNew<vtkRenderWindow> window;
    window->SetSize(options.width, options.height);
    window->SetWindowName("leap-dicom-viewer");
    window->AddRenderer(renderer);

    if (!options.screenshot.empty()) {
        window->SetOffScreenRendering(true);
        window->Render();
        vtkNew<vtkWindowToImageFilter> grab;
        grab->SetInput(window);
        vtkNew<vtkPNGWriter> writer;
        writer->SetFileName(options.screenshot.c_str());
        writer->SetInputConnection(grab->GetOutputPort());
        writer->Write();
        return EXIT_SUCCESS;
    }

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(window);

    viewer->volume = volume;
    viewer->camera = camera;
    viewer->label = label;
    viewer->camera_start_z = camera->GetPosition()[2];

#ifdef LEAP_DICOM_WITH_LEAP
    // The hand drives the view, so disable mouse camera control.
    interactor->SetInteractorStyle(nullptr);
    LeapInput leap(viewer->mailbox);
#else
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    interactor->SetInteractorStyle(style);
#endif
    interactor->Initialize();

    vtkNew<vtkCallbackCommand> key_callback;
    key_callback->SetCallback(on_key);
    interactor->AddObserver(vtkCommand::KeyPressEvent, key_callback);

#ifdef LEAP_DICOM_WITH_LEAP
    vtkNew<vtkCallbackCommand> timer_callback;
    timer_callback->SetCallback(on_timer);
    timer_callback->SetClientData(viewer.get());
    interactor->AddObserver(vtkCommand::TimerEvent, timer_callback);
    interactor->CreateRepeatingTimer(kTimerMilliseconds);
#endif

    window->Render();
    interactor->Start();
    return EXIT_SUCCESS;
}
