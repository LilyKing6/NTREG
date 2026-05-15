# NTREG

模仿 Windows 注册表机制实现的类注册表数据库 — 提供类似 Windows 注册表的键值树状存储结构。

## 功能特性

- **Hive 文件操作** — 创建、加载、保存注册表 Hive 文件（二进制格式）
- **键/值操作** — 创建、打开、删除、枚举注册表键和值
- **INF 导入** — 从 Windows INF 文件批量导入注册表数据
- **数据验证与修复** — Hive 完整性检查（`CmCheckRegistry`）和自愈修复
- **现代 C++20 API** — RAII 包装的 `registry::Key` 和 `registry::Registry` 类
- **跨平台** — 支持 Windows（MSVC/MinGW）和 Unix（GCC）

## 构建

**依赖：** CMake 3.20+, C++20 编译器

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j8
```

或使用快捷脚本：

```bash
bd.cmd      # 构建
clean.cmd   # 清理
```

## 项目结构

```
reg/
├── core/           # 核心库源文件
├── include/        # 头文件
├── tools/          # 工具程序
│   ├── mkhive      # 从 INF 创建 Hive 文件
│   ├── inithive    # 版本信息初始化
│   ├── regexplorer # 交互式注册表浏览器
│   ├── test_api    # 现代 API 集成测试
│   └── test_fileload # Hive 文件加载测试
├── reginit/        # INF 初始化文件
├── CMakeLists.txt  # 构建配置
└── CLAUDE.md       # 开发文档
```

## 使用示例

```cpp
#include "registry_api.hpp"

// 初始化注册表
auto reg = registry::Registry::initialize("SYSTEM,SOFTWARE,DEFAULT", false);

// 创建键
auto key = reg.open_key(L"NTReg\\Local\\SYSTEM\\MyKey");

// 设置值
key.set_string(L"Name", L"Hello");
key.set_dword(L"Count", 42);

// 读取值
auto name = key.get_string(L"Name);    // "Hello"
auto count = key.get_dword(L"Count");  // 42

// 枚举
for (const auto& name : reg.enum_keys(L"NTReg\\Local\\SYSTEM")) {
    // ...
}

// 关闭
reg.shutdown();
```

## 测试

```bash
cd build
./test_api.exe       # 现代 API 测试 (13 项)
./test_fileload.exe  # Hive 文件加载测试
./mkhive.exe -h:SYSTEM,SOFTWARE,DEFAULT -d:output reginit/hivesys.inf reginit/hivesft.inf reginit/hivedef.inf
```

## 支持的注册表类型

| 类型 | 常量 | 说明 |
|------|------|------|
| `REG_SZ` | 1 | 字符串 |
| `REG_BINARY` | 3 | 二进制数据 |
| `REG_DWORD` | 4 | 32 位整数 |
| `REG_MULTI_SZ` | 7 | 多字符串 |
| `REG_QWORD` | 11 | 64 位整数 |

## 许可证

BSD-2-Clause，详见 [LICENSE](LICENSE)。
