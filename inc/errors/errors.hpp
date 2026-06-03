// inc/errors/errors.hpp

#pragma once

#include <stdexcept>
#include <cstdint>

struct BusAccessError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct InstructionPageError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct LoadPageError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct StorePageError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ProgramExit : std::runtime_error {
    explicit ProgramExit(int code)
        : std::runtime_error("program exit"), code(code) {}

    int code;
};
