// inc/errors/errors.hpp

#pragma once

#include <stdexcept>

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