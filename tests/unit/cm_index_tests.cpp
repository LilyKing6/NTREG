#include "common.hpp"

TestSuite(Index, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{ Registry::shutdown(); })

Test(Index, ManySubkeysEnum) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\IdxMany");
    const int N = 50;
    for (int i = 0; i < N; ++i) {
        char16_t name[8];
        for (int j = 0; j < 7; ++j) name[j] = u'0' + ((i >> (4*(6-j))) & 0xF);
        name[7] = u'\0';
        key.create_subkey(name).close();
    }
    int count = 0;
    key.enum_keys([&](std::u16string_view) { ++count; return true; });
    cr_assert_eq(count, N);
    key.close();
}

Test(Index, SubkeyExistsCheck) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\IdxExists");
    key.create_subkey(u"Found").close();

    bool found = false;
    key.enum_keys([&](std::u16string_view name) {
        if (name == u"Found") found = true;
        return true;
    });
    cr_assert(found);
    key.close();
}

Test(Index, ValueByNameConsistent) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\IdxByName");
    key.set_string(u"Alpha", u"A");
    key.set_string(u"Beta", u"B");
    key.set_string(u"Gamma", u"C");

    cr_assert_eq(*key.get_string(u"Alpha"), std::u16string(u"A"));
    cr_assert_eq(*key.get_string(u"Beta"), std::u16string(u"B"));
    cr_assert_eq(*key.get_string(u"Gamma"), std::u16string(u"C"));
    key.close();
}

Test(Index, UnicodeKeyName) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\IdxUnicode");
    auto sub = key.create_subkey(u"Name");
    sub.close();
    int count = 0;
    key.enum_keys([&](std::u16string_view name) {
        if (name == u"Name") ++count;
        return true;
    });
    cr_assert_eq(count, 1);
    key.close();
}
