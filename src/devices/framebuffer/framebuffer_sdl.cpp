#include "devices/framebuffer.hpp"
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <iostream>

void FrameBuffer::init() {
    running_ = true;
    render_thread_ = std::thread(&FrameBuffer::render_loop, this);
}

void FrameBuffer::render_loop() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[framebuffer] SDL_Init failed: " << SDL_GetError() << "\n";
        return;
    }

    SDL_Window* window = SDL_CreateWindow("DOOM",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH * 2, HEIGHT * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    while (running_) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running_ = false; break; }
            // key events plug in here during Phase 3
        }

        SDL_UpdateTexture(texture, nullptr, pixels_.data(), WIDTH * BYTES_PER_PIXEL);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}