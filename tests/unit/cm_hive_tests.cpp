#include "common.hpp"

TestSuite(Hive, .init=[]{ Registry::initialize("SYSTEM"); }, .fini=[]{ Registry::shutdown(); })

Test(Hive, SYSTEMRootAccessible) {
    auto sys = Registry::open_key(u"\\NTReg\\Local\\SYSTEM");
    cr_assert(sys.valid());
    sys.close();
}

Test(Hive, MultipleHiveInit) {
    Registry::shutdown();
    Registry::initialize("SYSTEM,SOFTWARE");
    auto sys = Registry::open_key(u"\\NTReg\\Local\\SYSTEM");
    auto sw = Registry::open_key(u"\\NTReg\\Local\\SOFTWARE");
    cr_assert(sys.valid());
    cr_assert(sw.valid());
    sys.close();
    sw.close();
}

Test(Hive, CreateAcrossHives) {
    auto sysKey = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\HvCross");
    sysKey.set_string(u"Source", u"SYSTEM hive");
    sysKey.close();

    auto checkSys = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\HvCross");
    cr_assert(checkSys.valid());
    auto val = checkSys.get_string(u"Source");
    cr_assert(val.has_value());
    cr_assert_eq(*val, std::u16string(u"SYSTEM hive"));
    checkSys.close();
}

Test(Hive, ShutdownCleanup) {
    Registry::shutdown();
    Registry::initialize("SYSTEM");
    auto key = Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\HvShutdownTest");
    key.set_dword(u"Data", 999);
    key.close();
    Registry::shutdown();
    Registry::initialize("SYSTEM");
    auto reopened = Registry::open_key(u"\\NTReg\\Local\\SYSTEM\\HvShutdownTest");
    auto val = reopened.get_dword(u"Data");
    cr_assert(val.has_value());
    cr_assert_eq(*val, u32(999));
    reopened.close();
}
