// Hand-tracking logic, independent of both VTK and the Leap SDK so it can be unit-tested.
#pragma once

#include <mutex>

namespace leapviewer {

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

// One reading from the hand tracker. Leap reports palm positions in millimetres
// above the device and palm normals as unit vectors (pointing down = {0, -1, 0}).
struct HandSample {
    bool valid = false;        // a first hand is in view
    bool second_hand = false;  // a second hand is in view: freeze the volume
    Vec3 palm_normal{0, 0, -1};
    Vec3 palm_position{};
    int new_gestures = 0;      // gestures completed since the previous sample
};

struct RotationDelta {
    double x = 0, y = 0, z = 0;  // degrees about each world axis
};

// How strongly a change in palm normal rotates the volume.
inline constexpr double kDegreesPerNormalUnit = 200.0;
// How far the camera moves per millimetre of palm depth.
inline constexpr double kCameraUnitsPerMillimetre = 10.0;

RotationDelta rotation_between(const Vec3& previous, const Vec3& current);
int next_mode(int mode, int mode_count);

// Hands samples from the tracker's callback thread to the render thread.
// Gesture counts accumulate until taken, so none are lost between render ticks.
class SampleMailbox {
public:
    void post(const HandSample& sample);
    HandSample take();

private:
    std::mutex mutex_;
    HandSample latest_;
    int pending_gestures_ = 0;
};

// Turns successive samples into view updates. Runs on the render thread.
class Interaction {
public:
    struct Update {
        RotationDelta rotation;     // apply to the volume
        bool move_camera = false;
        double camera_z_offset = 0; // added to the camera's starting z position
        int mode = 0;
        bool flash = false;         // a gesture changed the mode this tick
    };

    explicit Interaction(int mode_count = 3) : mode_count_(mode_count) {}

    Update step(const HandSample& sample);
    int mode() const { return mode_; }

private:
    int mode_count_;
    int mode_ = 0;
    Vec3 previous_normal_{0, 0, -1};
};

}  // namespace leapviewer
