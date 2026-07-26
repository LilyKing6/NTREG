#include "common.hpp"

TestSuite(Hive, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{ /* skip shutdown; OS reclaims at exit */ })

Test(Hive, SYSTEMRootAccessible) {
    auto sys = Registry::open_key(u"\\NTReg\\Local\\SYSTEM");
    cr_assert(sys.valid());
    sys.close();
}

Test(Hive, CreateAndReadKey) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\HvCreateTest");
    key.set_string(u"Source", u"hive test");
    key.close();

    auto reopened = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\HvCreateTest");
    cr_assert(reopened.valid());
    auto val = reopened.get_string(u"Source");
    cr_assert(val.has_value());
    cr_assert_eq(*val, std::u16string(u"hive test"));
    reopened.close();
}

Test(Hive, EnumRootSubkeys) {
    auto sys = Registry::open_key(u"\\NTReg\\Local\\SYSTEM");
    int count = 0;
    sys.enum_keys([&](std::u16string_view) { count++; return true; });
    cr_assert(count > 0);
    sys.close();
}

Test(Hive, KeyMoveSemantics) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\HvMoveTest");
    key.set_dword(u"Val", 42);
    auto moved = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\HvMoveTest");
    cr_assert(moved.valid());
    auto v = moved.get_dword(u"Val");
    cr_assert(v.has_value());
    cr_assert_eq(*v, u32(42));
    moved.close();
    key.close();
}
