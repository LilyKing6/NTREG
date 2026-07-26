#include <algorithm>
// Minimal Criterion-compatible test runner (header-only)
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <exception>
#include <set>

struct SuiteOptions {
    std::function<void()> init = []{};
    std::function<void()> fini = []{};
};

struct TestCase {
    const char* suite;
    const char* name;
    SuiteOptions opts;
    std::function<void()> body;
};

inline std::vector<TestCase>& test_registry() {
    static std::vector<TestCase> reg;
    return reg;
}

inline int& test_failed_inner() { static int v = 0; return v; }
inline const char*& current_suite() { static const char* v = nullptr; return v; }
inline const char*& current_test() { static const char* v = nullptr; return v; }

#define cr_assert(expr, ...) do { \
    if (!(expr)) { \
        std::cerr << "  FAIL: " << current_suite() << "::" << current_test() \
                  << " (" << __FILE__ << ":" << __LINE__ << "): assertion failed: " #expr << "\n"; \
        test_failed_inner()++; \
        return; \
    } \
} while(0)

#define cr_assert_eq(a, b, ...) do { \
    auto _a = (a); auto _b = (b); \
    if (_a != _b) { \
        std::cerr << "  FAIL: " << current_suite() << "::" << current_test() \
                  << " (" << __FILE__ << ":" << __LINE__ << "): expected " #b " but got different\n"; \
        test_failed_inner()++; \
        return; \
    } \
} while(0)

#define cr_assert_neq(a, b, ...) do { \
    auto _a = (a); auto _b = (b); \
    if (_a == _b) { \
        std::cerr << "  FAIL: " << current_suite() << "::" << current_test() \
                  << " (" << __FILE__ << ":" << __LINE__ << "): expected != " #b "\n"; \
        test_failed_inner()++; \
        return; \
    } \
} while(0)

#define cr_assert_eq_w(want, got, ...) do { \
    if (_wcs_cmp_char16((want), (got)) != 0) { \
        std::cerr << "  FAIL: " << current_suite() << "::" << current_test() \
                  << " (" << __FILE__ << ":" << __LINE__ << "): wide string mismatch\n"; \
        test_failed_inner()++; \
        return; \
    } \
} while(0)

#define cr_assert_null(ptr, ...) cr_assert((ptr) == nullptr)
#define cr_assert_not_null(ptr, ...) cr_assert((ptr) != nullptr)

#define TestSuite(name, ...) namespace { SuiteOptions _so_##name = { __VA_ARGS__ }; }

#define Test(suite_name, test_name) \
    void _testfn_##suite_name##_##test_name(); \
    namespace { \
        struct _Reg_##suite_name##_##test_name { \
            _Reg_##suite_name##_##test_name() { \
                TestCase tc; tc.suite = #suite_name; tc.name = #test_name; \
                tc.opts = _so_##suite_name; \
                tc.body = _testfn_##suite_name##_##test_name; \
                test_registry().push_back(tc); \
            } \
        } _inst_##suite_name##_##test_name; \
    } \
    void _testfn_##suite_name##_##test_name()

inline int criterion_run_all() {
    int passed = 0, failed = 0;
    const char* last_suite = nullptr;
    std::set<std::string> init_done;

    for (auto& tc : test_registry()) {
        if (last_suite != tc.suite) {
            if (last_suite) {
                // fini previous suite
                auto it = std::find_if(test_registry().begin(), test_registry().end(),
                    [&](const TestCase& t) { return t.suite == last_suite; });
                if (it != test_registry().end()) {
                    try { it->opts.fini(); } catch (...) {}
                }
            }
            // init new suite
            if (init_done.find(tc.suite) == init_done.end()) {
                try { tc.opts.init(); } catch (const std::exception& e) {
                    std::cout << "==== " << tc.suite << " ====\n";
                    std::cout << "  INIT FAILED: " << e.what() << "\n";
                    last_suite = tc.suite;
                    init_done.insert(tc.suite);
                    continue;
                }
                init_done.insert(tc.suite);
            }
            std::cout << "==== " << tc.suite << " ====\n";
            last_suite = tc.suite;
        }
        current_suite() = tc.suite;
        current_test() = tc.name;
        test_failed_inner() = 0;

        std::cout << "  TEST: " << tc.name << " ... " << std::flush;
        try {
            tc.body();
            if (test_failed_inner() == 0) {
                std::cout << "PASS\n";
                passed++;
            } else {
                failed++;
            }
        } catch (const std::exception& e) {
            std::cout << "FAIL (exception: " << e.what() << ")\n";
            failed++;
        } catch (...) {
            std::cout << "FAIL (unknown exception)\n";
            failed++;
        }
    }

    // fini last suite
    if (last_suite) {
        auto it = std::find_if(test_registry().begin(), test_registry().end(),
            [&](const TestCase& t) { return t.suite == last_suite; });
        if (it != test_registry().end()) {
            try { it->opts.fini(); } catch (...) {}
        }
    }

    std::cout << "\n==============================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}

int main() {
    return criterion_run_all();
}
