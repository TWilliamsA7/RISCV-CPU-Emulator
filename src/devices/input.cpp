#include "devices/input.hpp"

void InputDevice::push_event(uint8_t key, bool pressed) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push({key, pressed});
}

uint32_t InputDevice::read32(uint32_t offset) {
    if (offset != REG_POP) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return 0xFFFFFFFF;

    Event e = queue_.front();
    queue_.pop();
    return (e.pressed ? (1u << 8) : 0) | e.key;
}

void InputDevice::write32(uint32_t offset, uint32_t val) {
    // reserved — no op
}