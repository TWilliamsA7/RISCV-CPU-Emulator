#include "devices/framebuffer.hpp"
#include <cstring>

FrameBuffer::FrameBuffer() : pixels_(SIZE, 0) {}

FrameBuffer::~FrameBuffer() {
    running_ = false;
    if (render_thread_.joinable())
        render_thread_.join();
}

uint8_t FrameBuffer::read8(uint32_t offset) const {
    if (offset >= SIZE) return 0;
    return pixels_[offset];
}

void FrameBuffer::write8(uint32_t offset, uint8_t val) {
    if (offset >= SIZE) return;
    pixels_[offset] = val;
}

uint32_t FrameBuffer::read32(uint32_t offset) const {
    if (offset + 4 > SIZE) return 0;
    uint32_t val;
    std::memcpy(&val, &pixels_[offset], 4);
    return val;
}

void FrameBuffer::write32(uint32_t offset, uint32_t val) {
    if (offset + 4 > SIZE) return;
    std::memcpy(&pixels_[offset], &val, 4);
}