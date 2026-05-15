# Implementation Plan: Animation Editor

## Overview

Implement the core animation editor feature set for a Cocos2d-x based editor using EnTT (ECS), ImGui (UI), and a custom .mov binary file format. The implementation covers ECS component definitions, multi-timeline animation system, .mov file serialization/deserialization, node naming system, hierarchy management, ECS-Cocos2d-x sync, and CMake build configuration.

## Tasks

- [ ] 1. Define ECS components and core data types
  - [ ] 1.1 Create AnimTypes.h with all ECS component structs and enums
    - Define `AnimProperty` enum (PositionX, PositionY, ScaleX, ScaleY, Rotation, Opacity, AnchorX, AnchorY, SkewX, SkewY, ColorR, ColorG, ColorB)
    - Define `EaseType` enum (Linear, EaseIn, EaseOut, EaseInOut, etc.)
    - Define `PlayState` enum (Stopped, Playing, Paused)
    - Define `WrapMode` enum (Once, Loop, PingPong)
    - Define `Keyframe` struct (time, value, ease)
    - Define `AnimationTrack` struct with keyframes vector and property field
    - Define `Timeline` struct with name, tracks, duration, isLooping
    - Define `NameComponent` struct (name, displayName, isRenamable)
    - Define `TransformComponent` struct (posX, posY, scaleX, scaleY, rotation, opacity)
    - Define `HierarchyComponent` struct (parent, children, depth, sortOrder)
    - Define `NodeRefComponent` struct (cocos2d::Node* node)
    - Define `AnimatorComponent` struct (timelines, activeTimelineIndex, speed, wrapMode, state, currentTime, pingPongForward)
    - Define `SampledValues` struct with all animatable property fields
    - Define `NodeNamingRules` with MAX_NAME_LENGTH, DEFAULT_PREFIX, isValidName(), generateUniqueName()
    - _Requirements: 1.1, 3.1, 4.7, 7.2, 7.3, 7.5_

  - [ ] 1.2 Create MovFileSerializer.h with file format structs and class declaration
    - Define `MovFileHeader` struct (magic, version, nodeCount, flags, timestamp)
    - Define `MovNodeData`, `MovAnimationData`, `MovTimelineData`, `MovTrackData`, `MovKeyframeData` structs
    - Declare `MovFileSerializer` class with static save/load methods and private read/write helpers
    - Define MAGIC (0x4D4F5646) and VERSION (1) constants
    - _Requirements: 1.2, 2.2, 2.3, 11.1, 11.2, 11.6_

- [ ] 2. Implement keyframe interpolation and animation track logic
  - [ ] 2.1 Implement AnimationTrack methods in Animator.cpp
    - Implement `addKeyframe()` that inserts in sorted order by time, replacing existing keyframe at same time
    - Implement `removeKeyframe()` by index
    - Implement `sample(float time)` using binary search for adjacent keyframes, normalize time, apply easing, and linear interpolation
    - Handle edge cases: empty track returns 0, single keyframe returns its value, before-first returns first value, after-last returns last value
    - Implement `getDuration()` returning last keyframe time or 0
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9_

  - [ ] 2.2 Write property test for keyframe ordering invariant
    - **Property 5: Keyframe Ordering Invariant**
    - Generate random sequences of addKeyframe/removeKeyframe operations and verify keyframes remain sorted by time after each operation
    - **Validates: Requirements 4.7**

  - [ ] 2.3 Write property test for sampling boundary correctness
    - **Property 6: Sampling Boundary Correctness**
    - Generate random tracks with 1+ keyframes, verify: exact keyframe time returns exact value, before-first returns first value, after-last returns last value
    - **Validates: Requirements 4.2, 4.3, 4.4, 4.6**

  - [ ] 2.4 Implement easing functions
    - Implement `applyEase(float t, EaseType ease)` for all EaseType variants
    - Ensure all easing functions return 0 at t=0 and 1 at t=1
    - _Requirements: 4.1_

- [ ] 3. Implement multi-timeline animation system
  - [ ] 3.1 Implement Timeline methods
    - Implement `addTrack(AnimProperty)` returning pointer to new track or nullptr if already exists
    - Implement `getTrack(AnimProperty)` returning pointer or nullptr
    - Implement `removeTrack(AnimProperty)`
    - Implement `getDuration()` returning max duration across all tracks
    - Implement `sample(float time)` iterating all tracks and populating SampledValues
    - _Requirements: 3.1, 3.2, 6.2_

  - [ ] 3.2 Implement AnimatorComponent update logic
    - Implement `update(float dt)` that advances currentTime based on speed and handles WrapMode
    - WrapMode::Once: clamp to duration, apply final sample, set Stopped
    - WrapMode::Loop: wrap currentTime by subtracting duration repeatedly
    - WrapMode::PingPong: reflect at boundaries, toggle pingPongForward
    - Handle zero/negative duration by setting Stopped
    - Implement `play()` that sets Playing state and resets pingPongForward for PingPong mode
    - Implement `stop()` that sets Stopped state
    - Implement `switchTimeline(int index)` that validates index, sets activeTimelineIndex, resets currentTime to 0, sets Stopped
    - Reject invalid timeline index (< 0 or >= count) by leaving state unchanged
    - _Requirements: 3.3, 3.4, 3.5, 3.6, 5.1, 5.2, 5.3, 5.4, 5.6, 5.7_

  - [ ] 3.3 Write property test for timeline independence
    - **Property 7: Timeline Independence**
    - Generate entity with multiple timelines, play one timeline for random duration, verify other timelines' data is unchanged
    - **Validates: Requirements 3.5**

  - [ ] 3.4 Write property test for timeline switch state reset
    - **Property 8: Timeline Switch State Reset**
    - Generate AnimatorComponent in random state, switch timeline, verify activeTimelineIndex, currentTime=0, state=Stopped
    - **Validates: Requirements 3.3**

  - [ ] 3.5 Write property test for WrapMode loop periodicity
    - **Property 9: WrapMode Loop Periodicity**
    - Generate timeline with Loop mode and duration > 0, verify sample(t) == sample(t + N*duration) for random t and N
    - **Validates: Requirements 5.5**

- [ ] 4. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 5. Implement node naming system
  - [ ] 5.1 Implement NodeNamingRules methods
    - Implement `isValidName()` checking empty, length > 128, and forbidden characters (/ \\ : * ? " < > |)
    - Implement `generateUniqueName()` that returns baseName if unique, otherwise appends _N with smallest available N
    - Implement default name generation using "Node_" prefix with smallest available integer
    - _Requirements: 7.2, 7.3, 7.4, 7.5, 7.6_

  - [ ] 5.2 Write property test for node name uniqueness invariant
    - **Property 10: Node Name Uniqueness Invariant**
    - Generate random sequences of node creation and rename operations, verify all displayNames are pairwise distinct
    - **Validates: Requirements 7.4, 7.5**

  - [ ] 5.3 Write property test for name validation determinism
    - **Property 11: Name Validation Determinism and Correctness**
    - Generate random strings, verify isValidName returns false iff empty, >128 chars, or contains forbidden characters
    - **Validates: Requirements 7.2, 7.3**

- [ ] 6. Implement node hierarchy management
  - [ ] 6.1 Implement HierarchyComponent operations
    - Implement `reparentNode()` that removes from old parent's children, adds to new parent's children, updates parent field, recalculates depth for moved node and all descendants
    - Implement `reparentToRoot()` that sets parent to entt::null, recalculates depth to 0 and descendants accordingly
    - Implement cycle detection: reject if new parent is a descendant of the node being moved or is the node itself
    - Implement `findRootEntities()` returning all entities with parent == entt::null
    - _Requirements: 8.1, 8.2, 8.3, 8.5, 8.6_

  - [ ] 6.2 Write property test for node tree acyclicity
    - **Property 12: Node Tree Acyclicity**
    - Generate random hierarchy operations (reparent, create, delete), verify parent chain from any entity reaches entt::null within entity count steps
    - **Validates: Requirements 8.1, 8.2, 8.3**

- [ ] 7. Implement .mov file serialization
  - [ ] 7.1 Implement MovFileSerializer::save()
    - Validate file path (reject ".." path traversal and unexpected absolute paths)
    - Open file in binary mode, return false if directory doesn't exist or open fails
    - Write MovFileHeader with magic, version, nodeCount, flags=0, timestamp
    - Traverse node tree depth-first pre-order, writing MovNodeData for each entity
    - Write animation data for all entities with AnimatorComponent (timelines, tracks, keyframes)
    - On write error, remove partially written file and return false
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.7, 11.5_

  - [ ] 7.2 Implement MovFileSerializer::load()
    - Validate file path (reject ".." path traversal and unexpected absolute paths)
    - Open file, validate magic number (0x4D4F5646), validate version <= VERSION
    - Validate nodeCount <= 10000, validate string lengths <= 128
    - Validate timelineCount <= 64, trackCount <= 32, keyframeCount <= 10000
    - Validate AnimProperty enum values are in valid range
    - Validate parentEntityId references resolve to previously read entities or equal 0
    - Validate keyframe times are non-negative and ascending within each track
    - On any validation failure or truncated file: abort, return false, leave Registry unmodified
    - On success: clear Registry, reconstruct all entities with components, create Cocos2d-x nodes with correct parent-child relationships
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 11.1, 11.2, 11.3, 11.4, 11.5, 11.6, 11.7_

  - [ ] 7.3 Write property test for serialization round-trip consistency
    - **Property 1: Serialization Round-Trip Consistency**
    - Generate random valid Registry data, save to file, load into new Registry, verify equivalent data (names, transforms, hierarchy, timelines, tracks, keyframes)
    - **Validates: Requirements 1.6, 2.1**

  - [ ] 7.4 Write property test for serialization idempotence
    - **Property 2: Serialization Idempotence**
    - Generate random valid Registry, save to A, load from A, save to B, verify A and B have identical content (excluding timestamp)
    - **Validates: Requirements 1.1, 1.6**

  - [ ] 7.5 Write property test for depth-first serialization ordering
    - **Property 3: Depth-First Serialization Ordering**
    - Generate random node trees, serialize, verify every parent appears before its children in the output
    - **Validates: Requirements 1.3, 8.4**

  - [ ] 7.6 Write property test for failed load preserves state
    - **Property 4: Failed Load Preserves State**
    - Generate invalid .mov byte sequences (wrong magic, bad version, malformed data), attempt load, verify Registry is unchanged
    - **Validates: Requirements 2.5, 11.4**

  - [ ] 7.7 Write property test for file validation rejects invalid input
    - **Property 13: File Validation Rejects Invalid Input**
    - Generate byte sequences without valid magic, or with nodeCount > 10000, nameLength > 128, negative/unsorted keyframe times; verify load returns false
    - **Validates: Requirements 2.2, 2.3, 11.1, 11.2, 11.3**

  - [ ] 7.8 Write property test for path traversal rejection
    - **Property 14: Path Traversal Rejection**
    - Generate file paths containing ".." or unexpected absolute prefixes, verify save/load reject them
    - **Validates: Requirements 11.5**

- [ ] 8. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 9. Implement ECS-Cocos2d-x synchronization
  - [ ] 9.1 Implement sync system in EditorScene update loop
    - Implement `syncTransformToCocos()` that applies TransformComponent values to NodeRefComponent::node each frame
    - Skip sync if NodeRefComponent::node is null
    - During Playing state: apply SampledValues directly to Cocos2d-x Node without modifying TransformComponent
    - On stop: restore Cocos2d-x Node properties from TransformComponent (preserved during playback)
    - Store pre-playback TransformComponent values when play begins to ensure correct restore
    - _Requirements: 9.1, 9.2, 9.3, 9.4, 9.5_

- [ ] 10. Implement Timeline Panel UI with context menu
  - [ ] 10.1 Implement right-click context menu for adding property tracks
    - Display "Add Property Track" submenu on right-click in Timeline Panel track area
    - List all AnimProperty enum values as menu items
    - Disable menu items for properties that already have a track in the active timeline
    - Do not show submenu if no timeline is active on selected entity
    - On selection: add new Track with zero keyframes for chosen property to active Timeline
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

  - [ ] 10.2 Implement Hierarchy Panel inline rename
    - Enter inline edit mode (ImGui InputText) on double-click of node name
    - Pre-fill input with current displayName
    - On Enter: validate name via NodeNamingRules, apply if valid, auto-suffix if duplicate, show error if invalid
    - On Escape or focus loss: discard pending input, retain previous name
    - Do not enter edit mode if NameComponent::isRenamable is false
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.7, 7.8_

  - [ ] 10.3 Implement Hierarchy Panel drag-and-drop reparenting
    - Support drag-and-drop to reparent nodes in the hierarchy
    - Reject operations that would create cycles or self-parenting, display error message
    - Update children lists, parent field, and recalculate depth on successful reparent
    - Support dragging to top level (reparent to root)
    - _Requirements: 8.3, 8.5, 8.6_

- [ ] 11. Configure CMake build system
  - [ ] 11.1 Create/update root CMakeLists.txt
    - Set cmake_minimum_required(VERSION 3.16)
    - Set project name and C++17 standard
    - Collect all source files under Classes/Editor/ and Classes/AppDelegate.cpp
    - Add subdirectories or include paths for External libraries (EnTT, ImGui, Behaviac)
    - Link against Cocos2d-x libraries
    - Support Debug and Release configurations with appropriate compiler flags
    - Target Windows platform with MSVC
    - Add fatal error if any required dependency path doesn't exist under Classes/External/
    - Produce a Windows executable as output
    - _Requirements: 10.1, 10.2, 10.3, 10.4, 10.5, 10.6_

- [ ] 12. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document using RapidCheck or custom fuzzing
- Unit tests validate specific examples and edge cases
- The implementation uses C++17 with EnTT ECS, ImGui for UI, and Cocos2d-x for rendering
- All file I/O uses binary mode with strict validation to prevent security issues

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2"] },
    { "id": 1, "tasks": ["2.1", "2.4", "5.1", "11.1"] },
    { "id": 2, "tasks": ["2.2", "2.3", "3.1", "5.2", "5.3", "6.1"] },
    { "id": 3, "tasks": ["3.2", "6.2"] },
    { "id": 4, "tasks": ["3.3", "3.4", "3.5", "7.1"] },
    { "id": 5, "tasks": ["7.2"] },
    { "id": 6, "tasks": ["7.3", "7.4", "7.5", "7.6", "7.7", "7.8"] },
    { "id": 7, "tasks": ["9.1"] },
    { "id": 8, "tasks": ["10.1", "10.2", "10.3"] }
  ]
}
```
