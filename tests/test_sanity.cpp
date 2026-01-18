#include <gtest/gtest.h>
#include "core/state.h"

TEST(Sanity, BuildWorks) {
    core::CpuState state;
    EXPECT_EQ(state.dummy, 0);
}