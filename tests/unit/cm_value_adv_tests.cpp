// Registry Advanced Value Operations Unit Tests
// Licensed under Apache-2.0

#include "common.hpp"
#include "registry_api.hpp"

TestSuite(Value, .init=[]{ registry::Registry::initialize(L"SYSTEM"); }, .fini=[]{ registry::Registry::shutdown(); })

Test(Value, MultiString) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VMultiTest");
  std::vector<std::wstring> ms = { L"a", L"b", L"c", L"" };
  key.set_multi_string(L"MultiSz", ms);
  auto val = key.get_multi_string(L"MultiSz");
  cr_assert(val.has_value(), "Multi-string value should exist");
  cr_assert(val->size() == ms.size(), "Multi-string count should match");
  for (size_t i = 0; i < ms.size(); ++i)
      cr_assert_eq_w(ms[i], (*val)[i], "Multi-string element %zu mismatch", i);
  key.close();
}

Test(Value, Qword) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VQwordTest");
  key.set_qword(L"QwVal", 0xCAFEBABELL0);
  auto val = key.get_qword(L"QwVal");
  cr_assert(val.has_value());
  cr_assert_eq(*val, 0xCAFEBABELL0u);
  key.close();
}

Test(Value, Link) {
  auto link_key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\Links");
  auto target_key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\Target");
  target_key.set_dword(L"Keep", 1);

  link_key.set_link(L"SymLink", L"\\NTReg\\Local\\SYSTEM\\Target");
  auto val = link_key.get_link(L"SymLink");
  cr_assert(val.has_value());
  cr_assert_eq_w(L"\\NTReg\\Local\\SYSTEM\\Target", val->c_str());

  link_key.close();
  target_key.close();
}

Test(Value, OverwriteTypes) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VOverType");

  key.set_string(L"X", L"was string");
  auto after_s = key.get_string(L"X");
  cr_assert(after_s.has_value());
  cr_assert_eq_w(L"was string", after_s->c_str());

  key.set_dword(L"X", 42);
  auto after_d = key.get_dword(L"X");
  cr_assert(after_d.has_value());
  cr_assert_eq(*after_d, 42u);

  std::vector<uint8_t> bin = {0x0,'T','e','s','t',0x1,0};
  key.set_binary(L"X", bin.data(), bin.size());
  auto after_b = key.get_binary(L"X");
  cr_assert(after_b.has_value());
  cr_assert_eq(after_b->size(), 5u);

  std::vector<std::wstring> ms = {L"new", L"multi"};
  key.set_multi_string(L"X", ms);
  auto after_m = key.get_multi_string(L"X");
  cr_assert(after_m.has_value());
  cr_assert_eq(after_m->size(), 2u);

  key.set_qword(L"X", 123456789u);
  auto after_q = key.get_qword(L"X");
  cr_assert(after_q.has_value());
  cr_assert_eq(*after_q, 123456789u);

  key.close();
}

Test(Value, DeleteRecreateAdvanced) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VDelRecAdv");
  key.set_multi_string(L"X", std::vector<std::wstring>{L"a"});
  key.delete_value(L"X");
  cr_assert(!key.get_multi_string(L"X").has_value());

  key.set_qword(L"X", 7);
  auto val = key.get_qword(L"X");
  cr_assert(val.has_value());
  cr_assert_eq(*val, 7u);

  key.close();
}

Test(Value, MultiStringEmpty) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VMultiEmpty");
  key.set_multi_string(L"Empty", std::vector<std::wstring>{});
  auto val = key.get_multi_string(L"Empty");
  cr_assert(val.has_value());
  cr_assert(val->empty());
  key.close();
}

Test(Value, QwordZeroLarge) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VQwZLge");
  key.set_qword(L"Zero", 0u);
  key.set_qword(L"Max", ~static_cast<u64>(0));
  cr_assert_eq(*key.get_qword(L"Zero"), 0u);
  cr_assert_eq(*key.get_qword(L"Max"), ~static_cast<u64>(0));
  key.close();
}

Test(Value, LinkRoundtrip) {
  auto base = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\LinkBase");
  auto link = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\LinkKey");
  base.set_string(L"TT", L"\\NTReg\\Local\\SYSTEM\\LinkBase");
  link.set_link(L"Target", L"\\NTReg\\Local\\SYSTEM\\LinkBase");

  auto got = link.get_link(L"Target");
  cr_assert(got.has_value());
  cr_assert_eq_w(L"\\NTReg\\Local\\SYSTEM\\LinkBase", got->c_str());

  link.close();
  base.close();
}

Test(Value, LargeMultiString) {
  auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\VBigMultiTest");
  std::vector<std::wstring> big(512, L"123456789");
  key.set_multi_string(L"BigMS", big);
  auto val = key.get_multi_string(L"BigMS");
  cr_assert(val.has_value());
  cr_assert(val->size() == 512u);
  for (const auto& s: *val) cr_assert_neq_w(L"", s);
  key.close();
}
