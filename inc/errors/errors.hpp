// inc/errors/errors.hpp

#pragma once

#include <stdexcept>

struct BusAccessError : std::runtime_error {
    using std::runtime_error::runtime_error;
};