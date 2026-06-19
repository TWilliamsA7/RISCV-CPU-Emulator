// inc/types.hpp

#pragma once

enum PrivilegeLevel {
    USER = 0,
    SUPERVISOR = 1,
    MACHINE = 3,
};

enum AccessType {
    FETCH,
    LOAD,
    STORE
};