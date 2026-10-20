// Feeds Leap Motion hand data into a SampleMailbox. Built only with LEAP_DICOM_WITH_LEAP=ON.
#pragma once

#include <memory>

#include "leapviewer/interaction.hpp"

namespace leapviewer {

class LeapInput {
public:
    // Starts listening; the SDK calls back on its own thread.
    explicit LeapInput(SampleMailbox& mailbox);
    ~LeapInput();
    LeapInput(const LeapInput&) = delete;
    LeapInput& operator=(const LeapInput&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace leapviewer
