#pragma once

#include <cstdint>
#include <queue>
#include <mutex>

class InputDevice {
public:
    static constexpr uint32_t BASE = 0x10004000;
    static constexpr uint32_t SIZE = 0x10;

    static constexpr uint32_t REG_POP = 0x00; // read: pop next event

    struct Event {
        uint8_t key;
        bool pressed;
    };

    // Called from the SDL/render thread on key events.
    void push_event(uint8_t key, bool pressed);

    // Called from the CPU thread via Bus.
    uint32_t read32(uint32_t offset);
    void     write32(uint32_t offset, uint32_t val);

private:
    std::queue<Event> queue_;
    std::mutex        mutex_;
};