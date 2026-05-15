# Requirements Document

## Introduction

本文档定义了基于 Cocos2d-x 的美术动画编辑器核心功能扩展的需求规格。功能范围包括：自定义 `.mov` 二进制文件格式（序列化/反序列化节点树及动画数据）、多时间线动画系统（关键帧、多条独立时间线、右键菜单创建属性轨道）、节点命名系统（任意节点重命名与唯一性保证）、以及 CMake 构建系统（从 VS 编译迁移到 CMake 直接编译运行）。

编辑器采用 Unity3D 风格面板布局（Hierarchy、Properties、Timeline、Viewport），技术栈包括 Cocos2d-x（渲染）、EnTT（ECS）、ImGui（编辑器 UI）、Behaviac（行为树）。

## Glossary

- **Editor**: 基于 Cocos2d-x 的美术动画编辑器应用程序
- **Serializer**: MovFileSerializer 组件，负责 .mov 文件的读写操作
- **Animation_System**: 多时间线动画播放与管理子系统
- **Naming_System**: 节点命名与唯一性管理子系统
- **Build_System**: CMake 构建配置系统
- **Timeline**: 一组动画轨道的集合，描述一种完整的动画行为
- **Track**: 单个属性的关键帧序列（AnimationTrack）
- **Keyframe**: 动画轨道中的时间-值对，定义属性在特定时刻的目标值
- **Registry**: EnTT ECS 注册表，存储所有实体和组件数据
- **Node_Tree**: 由 HierarchyComponent 维护的实体父子层级结构
- **SampledValues**: 在指定时间点对所有轨道插值后得到的属性值集合
- **WrapMode**: 动画播放到末尾时的行为模式（Once/Loop/PingPong）
- **EaseType**: 关键帧之间的缓动插值函数类型

## Requirements

### Requirement 1: .mov 文件序列化

**User Story:** As an animator, I want to save my node tree and animation data to a custom .mov binary file, so that I can persist my work and reload it in future editing sessions.

#### Acceptance Criteria

1. WHEN the user triggers a save operation, THE Serializer SHALL write the following Registry data to a .mov binary file at the specified path: NameComponent (displayName), TransformComponent (posX, posY, scaleX, scaleY, rotation, opacity), HierarchyComponent (parent-child relationships and sort order), and AnimatorComponent (all Timelines with their Tracks and Keyframes)
2. THE Serializer SHALL write a file header containing a magic number (0x4D4F5646), format version (currently 1), node count, flags, and a uint64 timestamp as the first 24 bytes of the .mov file
3. THE Serializer SHALL serialize node tree data using depth-first pre-order traversal, writing parent nodes before child nodes
4. THE Serializer SHALL serialize all Timeline and Keyframe data associated with each entity that has an AnimatorComponent, including for each Timeline its name, track list, duration, and looping flag, and for each Keyframe its time, value, and EaseType
5. IF the file path is invalid or the directory does not exist, THEN THE Serializer SHALL return false without creating or modifying any file on the file system
6. WHEN a save operation succeeds, THE Serializer SHALL produce a file that, when loaded back, reconstructs a Registry state with identical node displayNames, transform values, hierarchy relationships (parent-child and sort order), timeline names, track properties, and keyframe data (time, value, easeType); entity IDs and NodeRefComponent pointers are excluded from equivalence comparison
7. IF a write error occurs during serialization (e.g., disk full or I/O failure), THEN THE Serializer SHALL return false and remove any partially written file so that the file system is not left with a corrupt .mov file

### Requirement 2: .mov 文件反序列化

**User Story:** As an animator, I want to load a previously saved .mov file, so that I can resume editing my animation project.

#### Acceptance Criteria

1. WHEN the user triggers a load operation with a valid .mov file path, THE Serializer SHALL clear the provided Registry, reconstruct the complete node tree (NameComponent, TransformComponent, HierarchyComponent, AnimatorComponent) and animation data from the file into the Registry
2. THE Serializer SHALL validate the file header magic number equals 0x4D4F5646 before proceeding with deserialization
3. THE Serializer SHALL validate the file format version is less than or equal to the maximum supported version defined by MovFileSerializer::VERSION
4. WHEN loading succeeds, THE Serializer SHALL create Cocos2d-x Node instances and attach them to the provided parent node with correct parent-child relationships matching the HierarchyComponent data
5. IF the file does not exist or has an invalid magic number, THEN THE Serializer SHALL return false and leave the Registry and scene tree unmodified
6. IF the file version exceeds the maximum supported version, THEN THE Serializer SHALL return false and log a version incompatibility message indicating the file version and the maximum supported version
7. IF any parentEntityId reference in the file does not resolve to an entity previously read from the same file and does not equal zero, THEN THE Serializer SHALL abort the load operation, return false, and leave the Registry and scene tree unmodified
8. IF any AnimProperty enum value in track data is outside the range 0 to the maximum defined AnimProperty ordinal, THEN THE Serializer SHALL abort the load operation, return false, and leave the Registry and scene tree unmodified
9. IF the file is truncated or an I/O read error occurs before all declared nodes and animation data are fully read, THEN THE Serializer SHALL abort the load operation, return false, and leave the Registry and scene tree unmodified

### Requirement 3: 多时间线动画管理

**User Story:** As an animator, I want to create and manage multiple independent timelines on a single node, so that I can define different animation behaviors (e.g., movement, rotation, fade) separately.

#### Acceptance Criteria

1. THE Animation_System SHALL support multiple independent Timeline instances per entity through the AnimatorComponent, up to a maximum of 64 timelines per entity
2. WHEN the user adds a new Timeline to an entity, THE Animation_System SHALL create a Timeline with the name "Timeline_N" (where N is the smallest positive integer that does not duplicate an existing timeline name on that entity) and an empty track list
3. WHEN the user activates a Timeline by index, THE Animation_System SHALL set the activeTimelineIndex to the specified index, reset currentTime to zero, and set PlayState to Stopped
4. IF the user attempts to activate a Timeline with an index less than zero or greater than or equal to the timeline count, THEN THE Animation_System SHALL leave activeTimelineIndex, currentTime, and PlayState unchanged
5. WHILE a Timeline is active and in Playing state, THE Animation_System SHALL update currentTime each frame by adding delta time multiplied by speed, where speed is a positive value between 0.01 and 10.0 inclusive
6. THE Animation_System SHALL ensure that playing one Timeline does not modify the tracks, keyframes, name, duration, or isLooping flag of any other Timeline on the same entity

### Requirement 4: 关键帧插值与采样

**User Story:** As an animator, I want the animation system to smoothly interpolate between keyframes using easing functions, so that my animations have natural motion.

#### Acceptance Criteria

1. WHEN sampling a Track at a time value that falls between two adjacent keyframes kf0 and kf1, THE Animation_System SHALL normalize the time to t = (sampleTime - kf0.time) / (kf1.time - kf0.time), apply the easing function specified by kf1's EaseType to t, and return kf0.value + (kf1.value - kf0.value) * easedT
2. WHEN sampling a Track at a time value equal to a keyframe's time, THE Animation_System SHALL return that keyframe's exact value without interpolation
3. WHEN sampling a Track at a time before the first keyframe, THE Animation_System SHALL return the first keyframe's value
4. WHEN sampling a Track at a time after the last keyframe, THE Animation_System SHALL return the last keyframe's value
5. WHEN sampling an empty Track (zero keyframes), THE Animation_System SHALL return the default value of zero
6. WHEN sampling a Track with exactly one keyframe, THE Animation_System SHALL return that keyframe's value regardless of the sample time
7. THE Animation_System SHALL maintain keyframes within each Track in ascending order by time after any add or remove operation
8. IF addKeyframe is called with a time value equal to an existing keyframe's time in the same Track, THEN THE Animation_System SHALL replace that existing keyframe's value and EaseType with the new values rather than inserting a duplicate
9. WHEN finding adjacent keyframes for interpolation, THE Animation_System SHALL use binary search over the sorted keyframe list to locate the pair surrounding the sample time

### Requirement 5: 动画播放模式 (WrapMode)

**User Story:** As an animator, I want to choose how animations behave when they reach the end, so that I can create looping, one-shot, or ping-pong animations.

#### Acceptance Criteria

1. WHILE WrapMode is Once and currentTime reaches or exceeds the Timeline duration, THE Animation_System SHALL clamp currentTime to the duration, apply the final sample, and set PlayState to Stopped
2. WHILE WrapMode is Loop and currentTime reaches or exceeds the Timeline duration, THE Animation_System SHALL repeatedly subtract the duration from currentTime until currentTime is less than the duration, and continue playing from the wrapped time
3. WHILE WrapMode is PingPong and currentTime reaches or exceeds the Timeline duration in forward direction, THE Animation_System SHALL set pingPongForward to false and reflect currentTime to (2 * duration - currentTime), clamping the result to the range [0, duration]
4. WHILE WrapMode is PingPong and currentTime reaches or falls below zero in reverse direction, THE Animation_System SHALL set pingPongForward to true and reflect currentTime to its absolute value, clamping the result to the range [0, duration]
5. WHILE WrapMode is Loop, THE Animation_System SHALL produce identical sample values for time t and time t + N * duration for any positive integer N (periodicity)
6. IF the active Timeline duration is zero or negative, THEN THE Animation_System SHALL set PlayState to Stopped without modifying currentTime
7. WHEN play() is invoked on an AnimatorComponent with WrapMode PingPong, THE Animation_System SHALL set pingPongForward to true and currentTime to zero

### Requirement 6: 动画轨道创建（右键菜单）

**User Story:** As an animator, I want to add property tracks to a timeline via a right-click context menu, so that I can quickly choose which properties to animate.

#### Acceptance Criteria

1. WHEN the user right-clicks in the Timeline Panel track area, THE Editor SHALL display a context menu with an "Add Property Track" submenu
2. WHEN the user selects a property from the submenu, THE Animation_System SHALL add a new Track with zero keyframes for that AnimProperty to the active Timeline
3. IF the active Timeline already contains a Track for the selected property, THEN THE Editor SHALL display that menu item in a disabled state to prevent duplicate tracks
4. THE Editor SHALL list all defined AnimProperty enum values (PositionX, PositionY, ScaleX, ScaleY, Rotation, Opacity, AnchorX, AnchorY, SkewX, SkewY, ColorR, ColorG, ColorB) in the submenu, one item per property
5. IF no Timeline is active on the selected entity when the user right-clicks, THEN THE Editor SHALL not display the "Add Property Track" submenu

### Requirement 7: 节点命名与重命名

**User Story:** As an animator, I want to rename any node in the hierarchy, so that I can organize my scene with meaningful names.

#### Acceptance Criteria

1. WHEN the user double-clicks a node name in the Hierarchy Panel, THE Editor SHALL enter inline edit mode displaying an input text field pre-filled with the current displayName
2. WHEN the user confirms a new name (presses Enter), THE Naming_System SHALL validate the name against naming rules before applying it
3. IF the validated name is empty, exceeds 128 characters, or contains any of the characters: / \\ : * ? " < > |, THEN THE Naming_System SHALL reject the rename, retain the previous displayName unchanged, and display an error message indicating the validation failure reason
4. IF the user enters a name that already exists in the Registry, THEN THE Naming_System SHALL generate a unique name by appending an underscore and the smallest available positive integer suffix
5. THE Naming_System SHALL ensure that all entity displayName values within the same Registry are unique at all times
6. WHEN a new node is created without a user-specified name, THE Naming_System SHALL assign a default name using the prefix "Node_" followed by the smallest available positive integer that produces a unique name within the Registry
7. IF the user cancels inline edit mode (presses Escape or moves focus away from the input field), THEN THE Editor SHALL discard the pending input and retain the previous displayName unchanged
8. IF the node's NameComponent has isRenamable set to false, THEN THE Editor SHALL not enter inline edit mode on double-click for that node

### Requirement 8: 节点树层级结构

**User Story:** As an animator, I want to organize nodes in a parent-child hierarchy, so that I can structure complex animations with grouped elements.

#### Acceptance Criteria

1. THE Editor SHALL maintain a tree structure through HierarchyComponent where each entity has at most one parent and zero or more children
2. THE Editor SHALL ensure the node tree is acyclic: traversing the parent chain from any entity reaches entt::null in a number of steps less than or equal to the total entity count in the Registry
3. IF a drag operation would create a circular reference (a node becoming a descendant of itself) or would reparent a node to itself, THEN THE Editor SHALL reject the operation, leave the hierarchy unchanged, and display a "Cannot create circular hierarchy" message
4. WHEN serializing the node tree, THE Serializer SHALL write parent nodes before their children (depth-first pre-order)
5. WHEN a node is successfully reparented via drag-and-drop, THE Editor SHALL remove the node from its previous parent's children list, add it to the new parent's children list as the last child, set the node's parent to the new parent, and recalculate the depth field for the moved node and all its descendants to equal their parent's depth plus one
6. WHEN a node is reparented to become a root node (dragged to the top level), THE Editor SHALL set the node's parent to entt::null and recalculate the depth field of the node to zero and all its descendants accordingly

### Requirement 9: ECS 组件与 Cocos2d-x 同步

**User Story:** As an animator, I want changes to ECS component data to be reflected in the Cocos2d-x viewport in real time, so that I can see my edits immediately.

#### Acceptance Criteria

1. WHEN TransformComponent data changes (posX, posY, scaleX, scaleY, rotation, opacity), THE Editor SHALL apply the updated values to the associated Cocos2d-x Node referenced by NodeRefComponent within the same frame's update loop
2. WHILE an animation is in Playing state, THE Animation_System SHALL apply SampledValues directly to the Cocos2d-x Node each frame without overwriting the entity's TransformComponent data
3. WHEN the user stops an animation, THE Animation_System SHALL restore the Cocos2d-x Node properties to the values stored in TransformComponent (which remains unmodified during playback)
4. IF NodeRefComponent::node is null when a sync operation is triggered, THEN THE Editor SHALL skip the sync for that entity without modifying any state or interrupting other entities' sync operations
5. WHILE an animation is in Playing state, THE Animation_System SHALL preserve the TransformComponent values as they were before playback began, so that stopping restores the correct initial state

### Requirement 10: CMake 构建系统

**User Story:** As a developer, I want to build the project using CMake directly, so that I can compile and run without depending on Visual Studio IDE project files.

#### Acceptance Criteria

1. THE Build_System SHALL define a CMakeLists.txt that compiles all source files under Classes/Editor/ and Classes/AppDelegate.cpp, links against Cocos2d-x, ImGui, EnTT, and Behaviac libraries, and produces a Windows executable as the output artifact
2. THE Build_System SHALL set the C++ language standard to C++17 for all compiled targets
3. THE Build_System SHALL support both Debug and Release build configurations such that each configuration produces a successful build with the corresponding compiler optimization and debug symbol settings
4. THE Build_System SHALL support the Windows platform as the primary build target, generating a build system usable with MSVC
5. THE Build_System SHALL require CMake version 3.16 or higher via cmake_minimum_required
6. IF a required dependency (Cocos2d-x, ImGui, EnTT, or Behaviac) is not found at its expected path under Classes/External/, THEN THE Build_System SHALL halt configuration with a fatal error message that names the missing library and its expected directory path

### Requirement 11: 文件格式安全验证

**User Story:** As a developer, I want the file loader to validate all input data strictly, so that malformed or malicious .mov files cannot cause crashes or memory corruption.

#### Acceptance Criteria

1. WHEN loading a .mov file, THE Serializer SHALL validate that nodeCount does not exceed 10,000 before allocating memory
2. WHEN loading a .mov file, THE Serializer SHALL validate that string length fields (nameLength) do not exceed 128 characters
3. WHEN loading a .mov file, THE Serializer SHALL validate that keyframe time values are non-negative and in ascending order within each Track
4. IF any validation check fails during loading, THEN THE Serializer SHALL abort the load operation, return false, and leave the Registry unmodified
5. WHEN loading or saving a .mov file, THE Serializer SHALL reject file paths that contain path traversal sequences ("..") or that begin with an absolute path prefix (drive letter or root slash) when the expected base directory is relative
6. WHEN loading a .mov file, THE Serializer SHALL validate that timelineCount per entity does not exceed 64, trackCount per Timeline does not exceed 32, and keyframeCount per Track does not exceed 10,000
7. IF the file stream reaches end-of-file before all data indicated by count fields (nodeCount, timelineCount, trackCount, keyframeCount) has been read, THEN THE Serializer SHALL abort the load operation, return false, and leave the Registry unmodified
