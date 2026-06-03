// Criterion unit test common includes and helpers
#pragma once

#include <criterion/criterion.h>
#include <string>
#include "registry_api.hpp"

// Ensure wchar_t compatibility for string assertions
static inline int wcscmp__(const wchar_t* a, const wchar_t* b) {
    size_t i = 0;
    while (a[i] && b[i] && a[i] == b[i]) i++;
    return (int)(a[i] - b[i]);
}

#define cr_assert_eq_w(want, got) cr_assert(wcscmp__((want), (got)) == 0)
