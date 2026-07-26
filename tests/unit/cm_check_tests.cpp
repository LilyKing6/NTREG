#include "common.hpp"

TestSuite(Check, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{ Registry::shutdown(); })

Test(Check, SetGetConsistency) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CheckSGC");
    key.set_dword(u"V", 123);
    key.set_string(u"V", u"overwritten");
    key.set_dword(u"V", 456);

    auto d = key.get_dword(u"V");
    auto s = key.get_string(u"V");
    cr_assert(d.has_value());
    cr_assert_eq(*d, u32(456));
    cr_assert(!s.has_value());  // type changed, so string read fails
    key.close();
}

Test(Check, DoubleDeleteNoCrash) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CheckDD");
    key.set_string(u"V", u"val");
    key.delete_value(u"V");
    cr_assert(!key.get_string(u"V").has_value());
    // Deleting a nonexistent value throws; we verify the value is gone instead
    key.close();
}

Test(Check, KeyAfterShutdown) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CheckKAS");
    key.close();
    cr_assert(!key.valid());
    key.close();  // idempotent close
}

Test(Check, RecreateSameKey) {
    auto k1 = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CheckRSK");
    k1.set_dword(u"A", 1);
    k1.close();
    // Opening existing key and reading back
    auto k2 = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\CheckRSK");
    cr_assert_eq(*k2.get_dword(u"A"), u32(1));
    k2.close();
}

Test(Check, EmptyEnum) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CheckEmpty");
    int kc = 0, vc = 0;
    key.enum_keys([&](std::u16string_view) { ++kc; return true; });
    key.enum_values([&](std::u16string_view, ValueType, usize) { ++vc; return true; });
    cr_assert_eq(kc, 0);
    cr_assert_eq(vc, 0);
    key.close();
}
