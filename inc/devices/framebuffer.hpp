#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>

class FrameBuffer {
public:
    static constexpr uint32_t BASE = 0x10003000;
    static constexpr uint32_t WIDTH  = 640;
    static constexpr uint32_t HEIGHT = 400;
    static constexpr uint32_t BYTES_PER_PIXEL = 4;
    static constexpr uint32_t SIZE = WIDTH * HEIGHT * BYTES_PER_PIXEL;

    FrameBuffer();
    ~FrameBuffer();

    void init(); // starts the render thread

    uint8_t  read8(uint32_t offset) const;
    void     write8(uint32_t offset, uint8_t val);
    uint32_t read32(uint32_t offset) const;
    void     write32(uint32_t offset, uint32_t val);

private:
    std::vector<uint8_t> pixels_;

    std::thread       render_thread_;
    std::atomic<bool> running_{false};

    void render_loop();
};