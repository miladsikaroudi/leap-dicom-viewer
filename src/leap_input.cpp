// Leap Motion SDK v2 backend (the Orion SDK, v3+, removed the gesture API used here).
#include "leap_input.hpp"

#include <Leap.h>

namespace leapviewer {
namespace {

Vec3 to_vec3(const Leap::Vector& v) {
    return {v.x, v.y, v.z};
}

class Listener : public Leap::Listener {
public:
    explicit Listener(SampleMailbox& mailbox) : mailbox_(mailbox) {}

    void onFrame(const Leap::Controller& controller) override {
        const Leap::Frame frame = controller.frame();
        const Leap::Hand first = frame.hands()[0];

        HandSample sample;
        sample.valid = first.isValid();
        if (sample.valid) {
            sample.second_hand = frame.hands()[1].isValid();
            sample.palm_normal = to_vec3(first.palmNormal());
            sample.palm_position = to_vec3(first.palmPosition());
            // Count completed gestures only; a key tap arrives once, in STATE_STOP.
            const Leap::GestureList gestures = frame.gestures();
            for (auto it = gestures.begin(); it != gestures.end(); ++it) {
                if ((*it).isValid() && (*it).state() == Leap::Gesture::STATE_STOP) {
                    ++sample.new_gestures;
                }
            }
        }
        mailbox_.post(sample);
    }

private:
    SampleMailbox& mailbox_;
};

}  // namespace

struct LeapInput::Impl {
    explicit Impl(SampleMailbox& mailbox) : listener(mailbox) {
        controller.enableGesture(Leap::Gesture::TYPE_KEY_TAP, true);
        controller.addListener(listener);
    }
    ~Impl() { controller.removeListener(listener); }

    Listener listener;
    Leap::Controller controller;
};

LeapInput::LeapInput(SampleMailbox& mailbox) : impl_(std::make_unique<Impl>(mailbox)) {}
LeapInput::~LeapInput() = default;

}  // namespace leapviewer
