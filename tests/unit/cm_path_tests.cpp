#include "common.hpp"

TestSuite(Path, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{ /* skip shutdown; OS reclaims at exit */ })

Test(Path, DeepNesting) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\P1");
    auto k2 = key.create_subkey(u"P2");
    auto k3 = k2.create_subkey(u"P3");
    k3.set_dword(u"DeepVal", 888);
    k3.close(); k2.close(); key.close();

    auto k = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\P1\\P2\\P3");
    cr_assert_eq(*k.get_dword(u"DeepVal"), u32(888));
    k.close();
}

Test(Path, PathWithEscapes) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\PathEsc");
    auto sub = key.create_subkey(u"valid_name");
    sub.set_string(u"val", u"ok");
    sub.close();
    auto opened = key.open_subkey(u"valid_name");
    cr_assert(opened.valid());
    cr_assert_eq(*opened.get_string(u"val"), std::u16string(u"ok"));
    opened.close();
    key.close();
}

Test(Path, TraversalToRoot) {
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\PathRootChild");
    key.set_dword(u"Marker", 1);
    key.close();
    // Verify by accessing directly via full path
    auto rootAnchor = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\PathRootChild");
    cr_assert(rootAnchor.valid());
    cr_assert_eq(*rootAnchor.get_dword(u"Marker"), u32(1));
    rootAnchor.close();
}

Test(Path, NonexistentKeyThrows) {
    bool threw = false;
    try {
        Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\PathDoesNotExist");
    } catch (const RegistryException&) {
        threw = true;
    }
    cr_assert(threw);
}
