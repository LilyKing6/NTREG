#include "common.hpp"

TestSuite(API, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{/* skip shutdown; OS reclaims at exit */})

Test(API, OpenKeyRoot) {
    auto key = Registry::open_key(u"\\NTReg\\Local\\SYSTEM");
    cr_assert(key.valid());
    key.close();
}

Test(API, OpenKeyNested) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\APINested");
    auto sub = key.create_subkey(u"A");
    sub.set_dword(u"v", 1);
    sub.close();
    key.close();

    auto opened = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\APINested\\A");
    auto val = opened.get_dword(u"v");
    cr_assert(val.has_value());
    cr_assert_eq(*val, u32(1));
    opened.close();
}

Test(API, CreateKey) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\APICreate");
    cr_assert(key.valid());
    key.set_dword(u"Test", 100);
    key.close();

    auto opened = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\APICreate");
    auto val = opened.get_dword(u"Test");
    cr_assert(val.has_value());
    cr_assert_eq(*val, u32(100));
    opened.close();
}

Test(API, DeleteKey) {
    Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\APIDel").close();
    Registry::delete_key(u"\\NTReg\\Local\\SYSTEM\\APIDel");
    bool threw = false;
    try { Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\APIDel"); }
    catch (const RegistryException&) { threw = true; }
    cr_assert(threw);
}

Test(API, MoveSemantics) {
    auto k1 = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\APIMove");
    k1.set_dword(u"X", 1);
    auto k2 = std::move(k1);
    cr_assert(k2.valid());
    cr_assert(!k1.valid());
    cr_assert_eq(*k2.get_dword(u"X"), u32(1));
    k2.close();
}
