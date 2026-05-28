# Hazel 教学引擎中长期路线图

> 状态：讨论完毕，待启动 | 日期：2026-05-28

## 已完成（不再投入）

| 模块 | 说明 |
|------|------|
| Serialization | 三分支递归框架 + ArchiveYAML |
| SystemGraph | entt::flow 建图 + Kahn 分层 |
| ComponentReflection | HZ_REGISTER_COMPONENT + FieldMeta |
| SceneSerializer → 新框架迁移 | 4 个特化保留，其余全反射 |
| YAML convert / ScriptField 宏 | 保留在 SceneSerializer.cpp |

---

## M1：Room — 数据与运行时分离

**目标**：`.hazel` 文件 ←→ Room (纯数据) ←→ Scene (运行时)，三者解耦。

**动机**：当前 SceneSerializer 直接写 registry → YAML，加 ctx 序列化无从下手。Scene 既当 Level 又当 World。

**做法**：

```
.hazel  ←→  RoomSerializer  ←→  Room (registry, 纯数据, 无 System/物理/ctx)
                                      │
                    Scene::LoadRoom(Room)  逐 entity 拷贝组件
                    Room Scene::Snapshot() 导出当前状态
```

**改动范围**：

| 文件 | 内容 |
|------|------|
| 新建 `Room.h` | `class Room { entt::registry m_Registry; CRUD; friend Serializer; };` |
| 新建 `RoomSerializer.h/.cpp` | 从 SceneSerializer 移 YAML 读写逻辑，只做 Room ↔ YAML |
| 修改 `SceneSerializer.cpp` | 改为创建 RoomSerializer + 调用 Room 接口 |
| 修改 `Scene.h/.cpp` | 新增 `LoadRoom(Room)`, `SnapshotRoom()`，利用已有 `CopyComponent` 逻辑 |
| `EditorLayer.cpp` | Play = `SnapshotRoom() → OnRuntimeStart()`，Stop = `LoadRoom(snapshot)` |

**预期**：~150 行新代码（Room ~60 + 拆分 ~90）。Room 加 ctx 序列化见 M3。

---

## M2：Core 补齐 — 阻塞其他模块的基础设施

**目标**：补上 M3/M4 会依赖的底层缺口。

| 文件 | 内容 | 行数 |
|------|------|------|
| `Core/NonCopyable.h` | 删拷贝/声明移动的便利类 | ~15 |
| `Base.h` 扩展 | `WeakRef<T>` = std::weak_ptr 别名 | 1 行 |
| `Core/FileSystem.h` | 扩展：`WriteText`, `ReadText`, `ListDirectory` | ~40 |
| `Math/Math.h` 扩展 | `HZ_PI`, `Lerp`, `Clamp`, `Random::Range/s/Unit` | ~40 |
| `Debug/Instrumentor.h` | `#define HZ_PROFILE 1` 打开 profiler | 1 行 |
| `Core/ThreadPool.h` 修复 | 合并 Application 的 `m_MainThreadQueue`，消除 Cancel 竞态 | ~30 |

**删除**：

| 文件 | 原因 |
|------|------|
| `Systems/ScriptSystem` + `ScriptableEntity.h` | 从未使用，C# 路径已够 |

**预期**：~130 行新代码，增量改动，不改架构。

---

## M3：ctx 序列化 + GameMode + Player 框架

**目标**：(1) GameState / GameModeConfig / PhysicsConfig 可存进 .hazel；(2) Player-Controller-Pawn 三层实体的 System 骨架跑通。

**前提**：M1 (Room 存在，ctx 有地方挂)、M2 (基础工具可用)

**实体结构**：

```
Controller 实体                    Pawn 实体
┌──────────────────────────┐      ┌──────────────────┐
│ LocalPlayer {Index,DevID}│      │ TransformComp     │
│ ControllerComp ──────────┼──────│ PossessableComp   │
│   PossessedPawn: entity  │ 双向 │   Controller: UUID│
│ PlayerState {Score,Lives}│      │ SpriteRenderer    │
└──────────────────────────┘      │ Rigidbody2D       │
                                  └──────────────────┘
```

**组件**：

| 组件 | 挂载实体 | 说明 |
|------|---------|------|
| `LocalPlayer` | Controller | PlayerIndex，DeviceID — 标记哪个玩家 |
| `ControllerComp` | Controller | PossessedPawn 引用 — 当前控制谁 |
| `PlayerState` | Controller | Score/Deaths/Lives — Pawn 销毁不丢 |
| `PossessableComp` | Pawn | ControllerEntity UUID + PlayerIndex — 被谁控制 |

**ctx 格式扩展**：

```yaml
Room: Example
Context:
  GameModeConfig:
    MaxScore: 10
    MaxLives: 3
    RespawnDelay: 3.0
  PhysicsConfig:
    Gravity: [0, -9.8]
    SubSteps: 4
Entities:
  - Entity: ...
```

**改动范围**：

| 文件 | 内容 |
|------|------|
| 新建 `CtxTypes.h` | `using SerializableCtx = std::tuple<GameModeConfig, GameState, PhysicsConfig>` |
| 新建 `Components/PlayerComponents.h` | `LocalPlayer`, `ControllerComp`, `PlayerState`, `PossessableComp` + 反射注册 |
| 新建 `Core/GameState.h` | `GameModeConfig`, `GameState`, `PhysicsConfig` 结构体 + 反射注册 |
| 新建 `Systems/GameModeSystem.h/.cpp` | Phase 过渡, 胜利条件, 分数更新 |
| 新建 `Systems/PlayerControllerSystem.h/.cpp` | 读 InputActions → 写 PossessedPawn 的 Rigidbody2D |
| 新建 `Systems/SpawnSystem.h/.cpp` | 读 PlayerState.Lives → 复活倒计时 → 创建/销毁 Pawn |
| `CameraSystem` 修改 | 读 `ControllerComp.PossessedPawn` 的 Transform 替代 Primary camera |
| `RoomSerializer` 加 ctx 段 | 遍历 `SerializableCtx` tuple，反射读写 |
| `Scene::OnRuntimeStart` | ctx 初始化 + 创建 Controller 实体 |
| `Scene.cpp` 构造 | 注册 GameMode / PlayerController / Spawn 到 SystemGraph |
| 删除 `CameraComponent::Primary` | 相机跟 Controller 走，不要标记相机 |

**执行序列（Runtime）**：

```
PreUpdate: GameModeSystem (Phase 检查)
            → “Playing” ? 什么也不做
            → “PostMatch” ? 3秒后切换回菜单

Update:    PlayerControllerSystem (InputActions → Pawn.Velocity)
           SpawnSystem (PlayerState.Lives>0 && !有Pawn ? RespawnTimer -= dt)
           
Physics:   PhysicsSystem (已有)

PostPhys:  TransformSyncSystem (已有)

PreRender: CameraSystem (读 ControllerComp.PossessedPawn 的 Transform → 设置相机)

Render:    RenderSystem2D (已有)
```

**预期**：~300 行，打通”序列化 ctx → 三层实体生命周期 → Controller 驱动相机”。

---

## M4：输入系统 — 从原始键鼠到 Action

**目标**：InputSystem 生成 `InputActions`，M3 的 `PlayerControllerSystem` 读 Action 而非裸键码。

**前提**：M3 (PlayerControllerSystem 已存在)

**做法**：

```
GLFW 回调 → WindowsInput 更新 → InputSystem (PreUpdate)
  → 写入 registry.ctx<InputActions>
    → Jump: bool, Horizontal: float, Fire: bool...
                                                 ↓
                                    PlayerControllerSystem (M3)
                                    读 InputActions → 写 Pawn 速度
```

**改动范围**：

| 文件 | 内容 |
|------|------|
| 新建 `Core/InputActions.h` | Action 枚举 + 按键绑定表 |
| 新建 `Systems/InputSystem.h/.cpp` | PreUpdate，读 Input → 写 ctx `InputActions` |
| `Scene.cpp` | 注册 InputSystem 到 SystemGraph |
| `PlayerControllerSystem` 修改 | `KeyCode::W` → `InputActions.Horizontal > 0` |
| `EditorLayer` | 调用 `InputSystem::MapAction()` 绑定按键 |

**预期**：~100 行，游戏逻辑层不再依赖 `KeyCode::W/A/S/D`。AISystem 在同阶段写另一个 Controller 的目标方向——和 Player 走同一套 Pawn 控制接口。

---

## M5：事件总线 + Gameplay 事件

**目标**：System 可以订阅/发布事件，碰撞响应不再靠组件轮询。

**做法**：不扩展现有 Event 类体系。新建独立的轻量总线：

```cpp
// 单例，无虚函数
class EventBus {
public:
    template<typename T>
    using Handler = std::function<void(const T&)>;

    template<typename T>
    void Subscribe(System* owner, Handler<T> fn);

    template<typename T>
    void Publish(const T& event);  // 遍历处理器立即调用

    void Clear(System* owner);     // owner 销毁时解绑
};
```

**改动范围**：

| 文件 | 内容 |
|------|------|
| 新建 `Core/EventBus.h` | ~80 行，模板 + type_index key 的 handler map |
| 新建 `Events/GameplayEvents.h` | `CollisionBeginEvent`, `TriggerEvent`, `HealthChangedEvent` 等轻量 struct |
| `PhysicsSystem` 扩展 | `b2World_Step` 后 query contact → `EventBus::Publish(CollisionBeginEvent{...})` |
| `GameModeSystem` 扩展 | `Subscribe<HealthChangedEvent>()` → 检测 Player 死亡、更新分数 |

**预期**：~150 行，System 间通信不再靠 ctx 组件数据。

---

## M6：编辑器 Panel 拆分

**目标**：EditorLayer 800 行拆成 `ViewportPanel`, `InspectorPanel`, `ProjectPanel`, `ToolbarController`。

**前提**：M2 (共享上下文)、M5 (EventBus 通信)

**不做**：不引入 ImGuiArchive（ImGui 手写绑定的代码量大于反射收益，教学引擎不值得）。

---

## M7+：暂缓

| 模块 | 原因 |
|------|------|
| 音频 | 教学引擎不刻意需要，M1-M6 做完 demo 游戏如有音频需求再进 |
| UI 运行时系统 | 目前无用例，ImGui 编辑器面板够用 |
| 多渲染后端 | OpenGL 够教学，Vulkan 是单独一个大版本 |
| 跨平台 | 现阶段只有 Windows 编译器环境 |
| 网络/多人 | 远超出教学引擎范围 |

---

## 总览

```
  M1 (Room)        M2 (Core 补)
     │                 │
     └────┬────────────┘
          ↓
   M3 (ctx 序列化 + GameMode)
          │
   M4 (Input 抽象) ─── M5 (事件总线)
          │                 │
          └────┬────────────┘
               ↓
        M6 (编辑器拆 Panel)
```

| 里程碑 | 新增代码 | 可验证效果 |
|--------|---------|-----------|
| M1 Room | ~150 行 | Play/Stop 走 Room 快照，不再逐组件复制 |
| M2 Core | ~130 行 | 编译器无警告，FileSystem 可写文件 |
| M3 Player+GameMode | ~300 行 | Controller→Pawn→Camera 链路跑通，GameModeConfig 存 .hazel |
| M4 Input | ~100 行 | `InputActions.Jump` 替代 `KeyCode::W`，AISystem 同接口控制 Pawn |
| M5 EventBus | ~150 行 | Physics 碰撞 → GameMode 得分 链路跑通 |
| M6 Editor | ~200 行 | EditorLayer 分割为 4 个 Panel 文件 |

**总增量**：~1000 行。M1 之后 M2/M3 可并行，M4 依赖 M3，M5/M6 可并行。
