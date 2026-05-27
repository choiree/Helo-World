# Hazel Engine 层次审计报告

> 生成日期：2026-05-27 | 基准提交：6bce795

## 一、现有模块状况

```
Hazel/
├── Core/            ✅ 基础存在，有缺口
├── Serialization/   ✅ 新完成，三分支框架
├── Math/            ❌ 仅一个 DecomposeTransform 函数
├── Events/          ✅ 类型安全事件系统
├── Debug/           ✅ Chrome 兼容 profiler
├── ImGui/           ✅ ImGuiLayer
├── Renderer/        ✅ 抽象完整，仅 OpenGL 后端
├── Scene/           ✅ ECS + 组件 + 序列化器
├── Scripting/       ✅ Mono C# 互操作
├── Physics/         ⚠️ 仅 Physics2D.h（Box2D 薄封装）
├── Project/         ✅ 项目管理 + AssetManager
├── UI/              ❌ 35行空壳
├── Utils/           ⚠️ 仅 PlatformUtils
└── Platform/
    ├── Windows/     ✅ Window + Input + FileDialogs
    └── OpenGL/      ✅ 完整 OpenGL 后端
```

## 二、完全缺失的层次

### 2.1 数学封装层（Math）

**现状**：`Math.h` 9 行，仅 `DecomposeTransform()`。全引擎直接用 `glm::vec*` / `glm::mat*`。

**缺失**：
- `Vector2/3/4` 包装（或至少 typedef）
- `Matrix3/4` 包装
- `AABB` / `BoundingBox` / `BoundingSphere`
- `Ray` / `Plane` / `Frustum`
- `Rect` / `RectF`
- 颜色类型（`Color` / `LinearColor`）
- 数学常量（`HZ_PI`、`HZ_EPSILON` 等）
- 插值/缓动工具
- 随机数工具（UUID 里有个 `mt19937_64`，未暴露）

**影响**：序列化系统直接依赖 `glm::vec2/3/4`，换数学库要改 `Archive.h`。组件字段全是裸 glm 类型。

### 2.2 音频层（Audio）

**现状**：完全不存在。无任何音频相关文件。

**缺失**：
- AudioEngine（后端抽象）
- AudioSource / AudioListener 组件
- Sound / Music 资源类型
- 3D 空间化支持
- 音频 Asset 加载（WAV/OGG/MP3）

**阻塞**：任何需要音效/音乐的演示场景都无法实现。

### 2.3 UI 系统

**现状**：`UI.h` 35 行，定义了几个枚举值（`HorizAlignment`、`VertAlignment`、`ImageFilter`），无实现。

**缺失**：
- UI 控件体系（Text、Button、Image、Slider、ScrollView 等）
- 布局引擎
- 事件冒泡/路由
- 样式/主题系统
- Canvas / UIRenderer

**注意**：ImGui 仅用于编辑器调试面板，不是运行时 UI 方案。

### 2.4 资源/内容管线（Asset Pipeline）

**现状**：`AssetManager` 负责运行时加载缓存，`ProjectSerializer` 管理 `.hproj` 文件。但没有离线的资源编译/处理工具链。

**缺失**：
- Asset 注册表 / GUID 分配
- 资源导入器（FBX → mesh、PSD → texture atlas）
- 资源烘焙/压缩
- 热重载（检测文件变化 → 重新加载）
- 资源依赖图
- Asset Database / Content Browser 数据结构

### 2.5 网络层（Networking）

**现状**：完全不存在。

**缺失**：这是一个大层次，教学引擎可不急。

### 2.6 粒子/特效层（Particles/VFX）

**现状**：完全不存在。

**缺失**：
- ParticleSystem 组件
- ParticleEmitter
- GPU 粒子支持
- 粒子序列化

### 2.7 配置系统（Configuration）

**现状**：`ApplicationSpecification` 存储了几个启动参数，但无持久化引擎配置。

**缺失**：
- EngineConfig（图形质量、分辨率、VSync 等）
- UserSettings（键位绑定、音量、语言）
- CVar 系统（运行时调试变量）
- 配置文件序列化

### 2.8 AI/导航层

**现状**：完全不存在。只有 Box2D 物理，无寻路/行为树。

### 2.9 本地化（Localization）

**现状**：无字符串表、无本地化管道。

### 2.10 动画层

**现状**：`AnimationSystem` + `AnimationClip` + `AnimationClipSerializer` 存在，但仅覆盖精灵帧动画。

**缺失**：
- 骨骼动画
- 动画状态机/混合树
- AnimationBlueprint 资源

## 三、现有层次的具体缺口

### 3.1 Core 层

| 缺口 | 说明 |
|------|------|
| `NonCopyable` / 移动宏 | 删除拷贝/声明移动的便利工具 |
| `WeakRef<T>` | `Ref<T>` = shared_ptr，缺 weak_ptr 别名 |
| `StringID` | 编译期字符串哈希，替代字符串比较 |
| `Result<T,E>` | 错误处理模式（目前只有断言） |
| `FileSystem` 扩展 | 仅有 ReadFileBinary，缺写文件/文本读取/目录遍历 |
| `Buffer` 改进 | 当前用 raw new[]/delete[]，无对齐保证 |
| `ThreadPool` 修复 | Cancel() 竞态；主线程回调队列与 Application 重复 |

### 3.2 平台层

| 缺口 | 说明 |
|------|------|
| `WindowsWindow` 命名 | 实际是 GLFWWindow，与 Win32 无关 |
| `Time::GetTime()` 位置 | 在 Platform/Windows/ 但用 glfwGetTime()，跨平台 |
| `FileDialogs` | Win32 commdlg，非 Windows 需替代实现 |
| `EntryPoint.h` | main() 被 #ifdef HZ_PLATFORM_WINDOWS 包裹 |
| 无 Wayland/SDL 备选 | GLFW 仅有的窗口后端 |

### 3.3 渲染层

| 缺口 | 说明 |
|------|------|
| 无 Vulkan/DX12 后端 | 仅 OpenGL，但抽象接口已准备好 |
| 无 RenderGraph | 帧资源管理/Pass 编排 |
| 无批处理抽象 | Renderer2D 849 行全是具体实现 |
| 无材质系统 | 纹理/Shader 参数直接用，无 Material Asset |

### 3.4 物理层

| 缺口 | 说明 |
|------|------|
| 仅 2D | Box2D，无 3D 物理 |
| 无 PhysicsScene | 碰撞回调/射线检测/重叠查询在 PhysicsSystem 里硬编码 |
| 无 Joint/Constraint | Box2D 有能力，未封装 |

## 四、优先级建议（教学引擎视角）

### 立即可做

1. **Core 补基础设施** — `NonCopyable`、`WeakRef`、`FileSystem` 扩展（<200 行）
2. **平台层整理** — 重命名 `WindowsWindow` → `GLFWindow`，移 `Time` 到 Core（无行为变化）
3. **数学层起步** — 常量定义、`Rect`/`AABB`、颜色类型、随机数封装

### 近期需要

4. **音频层** — 选后端（MiniAudio / SoLoud），做 Stream/Source 抽象
5. **UI 系统设计** — 运行时 UI，不是编辑器面板
6. **配置系统** — CVar + 配置文件持久化

### 后续迭代

7. **Asset Pipeline** — 资源 GUID、热重载
8. **动画扩展** — 骨骼动画、状态机
9. **多渲染后端** — Vulkan/DX12

## 五、关键架构决策

**当前依赖关系是健康的**：

```
Editor / Game
    ↓
Scene / Scripting / Physics
    ↓
Renderer / Serialization / Events / Math
    ↓
Core (Base, Assert, Log, Timer, Buffer)
    ↓
Platform (Window, Input, FileDialogs)
```

各层之间通过抽象接口通信，没有循环依赖。补充缺失层次时保持这个方向即可。
