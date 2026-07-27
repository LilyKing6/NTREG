#include "common.hpp"

TestSuite(ValueBasic, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{/* skip shutdown; OS reclaims at exit */})

Test(ValueBasic, DwordRoundtrip) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBDword");
    key.set_dword(u"Val", 0xDEADBEEF);
    auto val = key.get_dword(u"Val");
    cr_assert(val.has_value());
    cr_assert_eq(*val, u32(0xDEADBEEF));
    key.close();
}

Test(ValueBasic, StringRoundtrip) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBString");
    key.set_string(u"Val", u"Hello World");
    auto val = key.get_string(u"Val");
    cr_assert(val.has_value());
    cr_assert_eq(*val, std::u16string(u"Hello World"));
    key.close();
}

Test(ValueBasic, BinaryRoundtrip) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBBinary");
    u8 data[] = {0x00, 0x01, 0x02, 0xFF, 0xFE};
    key.set_binary(u"Val", data, 5);
    auto val = key.get_binary(u"Val");
    cr_assert(val.has_value());
    cr_assert_eq(val->size(), size_t(5));
    cr_assert_eq((*val)[3], u8(0xFF));
    key.close();
}

Test(ValueBasic, MissingValueNullopt) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBMissing");
    cr_assert(!key.get_string(u"Nonexistent").has_value());
    cr_assert(!key.get_dword(u"Nonexistent").has_value());
    cr_assert(!key.get_binary(u"Nonexistent").has_value());
    key.close();
}

Test(ValueBasic, EmptyString) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBEmpty");
    key.set_string(u"Empty", u"");
    auto val = key.get_string(u"Empty");
    cr_assert(val.has_value());
    cr_assert(val->empty());
    key.close();
}

Test(ValueBasic, DwordZero) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBZero");
    key.set_dword(u"Zero", 0);
    auto val = key.get_dword(u"Zero");
    cr_assert(val.has_value());
    cr_assert_eq(*val, u32(0));
    key.close();
}

Test(ValueBasic, LargeString) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBLargeStr");
    std::u16string large(2000, u'X');
    key.set_string(u"Big", large);
    auto val = key.get_string(u"Big");
    cr_assert(val.has_value());
    cr_assert_eq(val->size(), size_t(2000));
    key.close();
}

Test(ValueBasic, DeleteValue) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBDel");
    key.set_dword(u"Val", 42);
    key.delete_value(u"Val");
    cr_assert(!key.get_dword(u"Val").has_value());
    key.close();
}
