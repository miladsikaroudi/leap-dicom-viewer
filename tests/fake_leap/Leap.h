// Minimal stand-in for the subset of the Leap Motion SDK v2 API that
// src/leap_input.cpp uses. It lets CI compile the Leap backend without the
// proprietary SDK; it does not track hands.
#pragma once

#include <vector>

namespace Leap {

struct Vector {
    float x = 0, y = 0, z = 0;
};

class Gesture {
public:
    enum Type { TYPE_INVALID = -1, TYPE_SWIPE = 1, TYPE_CIRCLE = 4, TYPE_SCREEN_TAP = 5, TYPE_KEY_TAP = 6 };
    enum State { STATE_INVALID = -1, STATE_START = 1, STATE_UPDATE = 2, STATE_STOP = 3 };
    bool isValid() const { return false; }
    State state() const { return STATE_INVALID; }
};

class GestureList {
public:
    using const_iterator = std::vector<Gesture>::const_iterator;
    const_iterator begin() const { return items_.begin(); }
    const_iterator end() const { return items_.end(); }

private:
    std::vector<Gesture> items_;
};

class Hand {
public:
    bool isValid() const { return false; }
    Vector palmNormal() const { return {0, -1, 0}; }
    Vector palmPosition() const { return {}; }
};

class HandList {
public:
    Hand operator[](int) const { return {}; }
};

class Frame {
public:
    HandList hands() const { return {}; }
    GestureList gestures() const { return {}; }
};

class Controller;

class Listener {
public:
    virtual ~Listener() = default;
    virtual void onFrame(const Controller&) {}
};

class Controller {
public:
    Frame frame(int = 0) const { return {}; }
    bool addListener(Listener&) { return true; }
    bool removeListener(Listener&) { return true; }
    void enableGesture(Gesture::Type, bool = true) const {}
};

}  // namespace Leap
