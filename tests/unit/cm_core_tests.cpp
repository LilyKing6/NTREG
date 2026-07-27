#include "common.hpp"

TestSuite(Core, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{/* skip shutdown; OS reclaims at exit */})

Test(Core, KeyNativeHandle) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CoreNH");
    cr_assert_not_null(key.native_handle());
    key.close();
    cr_assert(!key.valid());
}

Test(Core, EnumerateSubkeys) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CoreEnum");
    key.create_subkey(u"SK1").close();
    key.create_subkey(u"SK2").close();
    key.create_subkey(u"SK3").close();

    int count = 0;
    key.enum_keys([&](std::u16string_view) { ++count; return true; });
    cr_assert_eq(count, 3);
    key.close();
}

Test(Core, EnumerateValues) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CoreEnumV");
    key.set_dword(u"V1", 1);
    key.set_string(u"V2", u"two");
    key.set_dword(u"V3", 3);

    int count = 0;
    key.enum_values([&](std::u16string_view, ValueType, usize) { ++count; return true; });
    cr_assert_eq(count, 3);
    key.close();
}

Test(Core, EnumEarlyExit) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CoreEarlyExit");
    key.create_subkey(u"A").close();
    key.create_subkey(u"B").close();

    int count = 0;
    key.enum_keys([&](std::u16string_view) { ++count; return count < 2; });
    cr_assert_eq(count, 2);
    key.close();
}

Test(Core, SubkeyRoundtrip) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CoreSubRT");
    auto sub = key.create_subkey(u"Nested");
    sub.set_string(u"Name", u"Value");
    sub.close();

    auto opened = key.open_subkey(u"Nested");
    auto val = opened.get_string(u"Name");
    cr_assert(val.has_value());
    cr_assert_eq(*val, std::u16string(u"Value"));
    opened.close();
    key.close();
}

Test(Core, DeleteSubkey) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CoreDelSK");
    key.create_subkey(u"ToDel").close();
    key.create_subkey(u"Keep").close();
    key.delete_subkey(u"ToDel");

    int count = 0;
    key.enum_keys([&](std::u16string_view) { ++count; return true; });
    cr_assert_eq(count, 1);
    key.close();
}
