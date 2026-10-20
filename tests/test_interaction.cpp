// Unit tests for the hand-tracking logic. No framework: each CHECK failure
// prints the expression and the test exits non-zero.
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

#include "leapviewer/interaction.hpp"

using namespace leapviewer;

static int failures = 0;
#define CHECK(expr)                                                             \
    do {                                                                        \
        if (!(expr)) {                                                          \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #expr); \
            ++failures;                                                         \
        }                                                                       \
    } while (0)

static bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

static HandSample hand(Vec3 normal, float depth = 0, bool second = false, int gestures = 0) {
    HandSample s;
    s.valid = true;
    s.second_hand = second;
    s.palm_normal = normal;
    s.palm_position = {0, 0, depth};
    s.new_gestures = gestures;
    return s;
}

static void rotation_uses_each_axis() {
    const RotationDelta d = rotation_between({0, 0, -1}, {0.1f, -0.2f, -0.9f});
    CHECK(near(d.x, 0.1f * kDegreesPerNormalUnit));
    CHECK(near(d.y, -0.2f * kDegreesPerNormalUnit));
    CHECK(near(d.z, (-0.9f + 1.0f) * kDegreesPerNormalUnit));  // the 2018 code rotated X here
}

static void no_hand_means_no_motion() {
    Interaction ix;
    const auto u = ix.step(HandSample{});
    CHECK(near(u.rotation.x, 0) && near(u.rotation.y, 0) && near(u.rotation.z, 0));
    CHECK(!u.move_camera);
}

static void second_hand_freezes_without_a_jump_on_release() {
    Interaction ix;
    ix.step(hand({0, 0, -1}));
    const auto frozen = ix.step(hand({0.5f, 0, -1}, 0, /*second=*/true));
    CHECK(near(frozen.rotation.x, 0));
    CHECK(!frozen.move_camera);
    // Releasing the second hand with the palm where it was during the freeze: no rotation.
    const auto released = ix.step(hand({0.5f, 0, -1}));
    CHECK(near(released.rotation.x, 0));
}

static void palm_depth_moves_camera() {
    Interaction ix;
    const auto u = ix.step(hand({0, 0, -1}, 25.0f));
    CHECK(u.move_camera);
    CHECK(near(u.camera_z_offset, 25.0 * kCameraUnitsPerMillimetre));
}

static void gestures_cycle_modes_and_flash() {
    Interaction ix(3);
    CHECK(ix.step(hand({0, 0, -1}, 0, false, 1)).mode == 1);
    const auto u = ix.step(hand({0, 0, -1}, 0, false, 2));
    CHECK(u.mode == 0);  // 1 -> 2 -> 0
    CHECK(u.flash);
    CHECK(!ix.step(hand({0, 0, -1})).flash);
}

static void mailbox_accumulates_gestures_between_takes() {
    SampleMailbox box;
    box.post(hand({0, 0, -1}, 0, false, 1));
    box.post(hand({0.2f, 0, -1}, 0, false, 1));
    const HandSample s = box.take();
    CHECK(s.new_gestures == 2);
    CHECK(near(s.palm_normal.x, 0.2f));
    CHECK(box.take().new_gestures == 0);
}

static void mailbox_is_safe_across_threads() {
    SampleMailbox box;
    constexpr int kPosts = 20000;
    std::thread producer([&] {
        for (int i = 0; i < kPosts; ++i) box.post(hand({0, 0, -1}, 0, false, 1));
    });
    int seen = 0;
    while (seen < kPosts) seen += box.take().new_gestures;
    producer.join();
    CHECK(seen == kPosts);
}

int main() {
    rotation_uses_each_axis();
    no_hand_means_no_motion();
    second_hand_freezes_without_a_jump_on_release();
    palm_depth_moves_camera();
    gestures_cycle_modes_and_flash();
    mailbox_accumulates_gestures_between_takes();
    mailbox_is_safe_across_threads();
    if (failures == 0) std::puts("all interaction tests passed");
    return failures == 0 ? 0 : 1;
}
