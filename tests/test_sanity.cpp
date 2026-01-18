#include <gtest/gtest.h>
#include "core/state.hpp"

TEST(Sanity, BuildWorks) {
    core::CpuState state;
    EXPECT_EQ(state.dummy, 0);
}