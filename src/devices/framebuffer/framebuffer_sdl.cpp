#include "devices/framebuffer.hpp"
#include "doom/doomkeys.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <iostream>

void FrameBuffer::init(InputDevice* input) {
    running_ = true;
    render_thread_ = std::thread(&FrameBuffer::render_loop, this);
    input_ = input;
}

static uint8_t translate_sdl_key(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_UP:     return KEY_UPARROW;
        case SDLK_DOWN:   return KEY_DOWNARROW;
        case SDLK_LEFT:   return KEY_LEFTARROW;
        case SDLK_RIGHT:  return KEY_RIGHTARROW;
        case SDLK_RETURN: return KEY_ENTER;
        case SDLK_ESCAPE: return KEY_ESCAPE;
        case SDLK_LCTRL:
        case SDLK_RCTRL:  return KEY_FIRE;
        case SDLK_SPACE:  return KEY_USE;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT: return KEY_RSHIFT;
        default:
            if (sym >= 'a' && sym <= 'z') return (uint8_t)sym;
            if (sym >= '0' && sym <= '9') return (uint8_t)sym;
            return 0;
    }
}

void FrameBuffer::render_loop() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[framebuffer] SDL_Init failed: " << SDL_GetError() << "\n";
        return;
    }

    SDL_Window* window = SDL_CreateWindow("RISC-V Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH * 2, HEIGHT * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "[framebuffer] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "[framebuffer] SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        // try software fallback
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            std::cerr << "[framebuffer] software renderer also failed: " << SDL_GetError() << "\n";
            return;
        }
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture) {
        std::cerr << "[framebuffer] SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        return;
    }

    std::cerr << "[framebuffer] window/renderer/texture created OK, entering render loop\n";
    while (running_) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running_ = false; break; }
            
            if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
                uint8_t doom_key = translate_sdl_key(ev.key.keysym.sym);
                if (doom_key != 0)
                    input_->push_event(doom_key, ev.type == SDL_KEYDOWN);
            }

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