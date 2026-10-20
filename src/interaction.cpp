#include "leapviewer/interaction.hpp"

namespace leapviewer {

RotationDelta rotation_between(const Vec3& previous, const Vec3& current) {
    return {(current.x - previous.x) * kDegreesPerNormalUnit,
            (current.y - previous.y) * kDegreesPerNormalUnit,
            (current.z - previous.z) * kDegreesPerNormalUnit};
}

int next_mode(int mode, int mode_count) {
    return (mode + 1) % mode_count;
}

void SampleMailbox::post(const HandSample& sample) {
    std::lock_guard lock(mutex_);
    latest_ = sample;
    pending_gestures_ += sample.new_gestures;
}

HandSample SampleMailbox::take() {
    std::lock_guard lock(mutex_);
    HandSample out = latest_;
    out.new_gestures = pending_gestures_;
    pending_gestures_ = 0;
    return out;
}

Interaction::Update Interaction::step(const HandSample& sample) {
    Update update;
    if (sample.valid) {
        for (int i = 0; i < sample.new_gestures; ++i) {
            mode_ = next_mode(mode_, mode_count_);
        }
        update.flash = sample.new_gestures > 0;

        if (!sample.second_hand) {
            update.rotation = rotation_between(previous_normal_, sample.palm_normal);
            update.move_camera = true;
            update.camera_z_offset = sample.palm_position.z * kCameraUnitsPerMillimetre;
        }
        // Track the hand even while frozen, so releasing the second hand
        // doesn't apply all the rotation that happened during the freeze.
        previous_normal_ = sample.palm_normal;
    }
    update.mode = mode_;
    return update;
}

}  // namespace leapviewer
