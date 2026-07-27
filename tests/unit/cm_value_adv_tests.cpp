// Registry Advanced Value Operations Unit Tests
#include "common.hpp"

TestSuite(Value, .init=[]{ registry::Registry::initialize("SYSTEM"); }, .fini=[]{/* skip shutdown; OS reclaims at exit */})

Test(Value, MultiString) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VMultiTest");
  std::vector<std::u16string> ms = { u"a", u"b", u"c" };
  key.set_multi_string(u"MultiSz", ms);
  auto val = key.get_multi_string(u"MultiSz");
  cr_assert(val.has_value());
  cr_assert_eq(val->size(), ms.size());
  for (size_t i = 0; i < ms.size(); ++i)
      cr_assert_eq(ms[i], (*val)[i]);
  key.close();
}

Test(Value, Qword) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VQwordTest");
  key.set_qword(u"QwVal", 0xCAFEBABELLU);
  auto val = key.get_qword(u"QwVal");
  cr_assert(val.has_value());
  cr_assert_eq(*val, 0xCAFEBABELLU);
  key.close();
}

Test(Value, Link) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VLinkTest");
  key.set_link(u"LinkVal", u"\\NTReg\\Local\\SYSTEM\\Target");
  auto val = key.get_link(u"LinkVal");
  cr_assert(val.has_value());
  cr_assert_eq(*val, std::u16string(u"\\NTReg\\Local\\SYSTEM\\Target"));
  key.close();
}

Test(Value, OverwriteTypes) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VOverType");

  key.set_string(u"X", u"was string");
  auto after_s = key.get_string(u"X");
  cr_assert(after_s.has_value());
  cr_assert_eq(std::u16string(u"was string"), *after_s);

  key.set_dword(u"X", 42);
  auto after_d = key.get_dword(u"X");
  cr_assert(after_d.has_value());
  cr_assert_eq(*after_d, 42u);

  std::vector<u8> bin = {0x0,'T','e','s','t',0x1,0};
  key.set_binary(u"X", bin.data(), 5);
  auto after_b = key.get_binary(u"X");
  cr_assert(after_b.has_value());
  cr_assert_eq(after_b->size(), 5u);

  std::vector<std::u16string> ms = {u"new", u"multi"};
  key.set_multi_string(u"X", ms);
  auto after_m = key.get_multi_string(u"X");
  cr_assert(after_m.has_value());
  cr_assert_eq(after_m->size(), 2u);

  key.set_qword(u"X", 123456789u);
  auto after_q = key.get_qword(u"X");
  cr_assert(after_q.has_value());
  cr_assert_eq(*after_q, u64(123456789));

  key.close();
}

Test(Value, DeleteRecreateAdvanced) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VDelRecAdv");
  key.set_dword(u"X", 42);
  key.delete_value(u"X");
  cr_assert(!key.get_dword(u"X").has_value());

  key.set_dword(u"X", 7);
  auto val = key.get_dword(u"X");
  cr_assert(val.has_value());
  cr_assert_eq(*val, u64(7));

  key.close();
}

Test(Value, MultiStringEmpty) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VMultiEmpty");
  key.set_multi_string(u"Empty", std::vector<std::u16string>{});
  auto val = key.get_multi_string(u"Empty");
  cr_assert(val.has_value());
  cr_assert(val->empty());
  key.close();
}

Test(Value, QwordZeroLarge) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VQwZLge");
  key.set_qword(u"Zero", 0u);
  key.set_qword(u"Max", ~u64(0));
  cr_assert_eq(*key.get_qword(u"Zero"), u64(0));
  cr_assert_eq(*key.get_qword(u"Max"), ~u64(0));
  key.close();
}

Test(Value, LinkRoundtrip) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VLinkRT");
  key.set_link(u"From", u"\\NTReg\\Local\\SYSTEM\\VLinkRT");
  key.set_link(u"From", u"\\NTReg\\Local\\SYSTEM\\Elsewhere");
  auto val = key.get_link(u"From");
  cr_assert(val.has_value());
  cr_assert_eq(*val, std::u16string(u"\\NTReg\\Local\\SYSTEM\\Elsewhere"));
  key.close();
}

Test(Value, LargeMultiString) {
  auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\VBigMultiTest");
  std::u16string big(1024, u'X');
  key.set_string(u"BigStr", big);
  auto val = key.get_string(u"BigStr");
  cr_assert(val.has_value());
  cr_assert_eq(val->size(), size_t(1024));
  key.close();
}
