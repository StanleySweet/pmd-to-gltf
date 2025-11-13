# PMD/PSA to glTF Converter - Implementation Notes

## File Format Details

### PMD (Model) Format
- Stores vertices in **world space** matching the rest pose
- Bone transforms (restStates) are in **absolute world space**
- Vertex skinning uses up to 4 bones per vertex with weights
- Invalid bone indices marked as 0xFF (no bone)
- Prop points reference bones by index for attachment

### PSA (Animation) Format
- Stores bone transforms per frame
- No bone hierarchy - just flat array indexed by bone number
- Bone states are in world space
- Animation name and frame length fields are unused (handled by game's actor XML)

### Skeleton XML Format
- Defines bone hierarchy (parent-child relationships)
- Provides bone names (PMD/PSA use indices only)
- Standard skeleton defines canonical bone structure
- Multiple skeleton variants can map to the same standard (e.g., "3ds Max Horse biped" → "Horse")

## glTF Conversion Challenges

### Coordinate System
- **PMD/PSA**: Uses absolute world space transforms for bones
- **glTF**: Expects hierarchical bones with transforms relative to parent
- **Solution**: Compute local transforms from world space using quaternion math

### Vertex Skinning
- **PMD**: Vertices already transformed to world space (in rest pose)
- **glTF**: Expects vertices in object/armature space
- **Solution**: Use inverse bind matrices to transform from world space to armature space
  - Each IBM is the inverse of the bone's world transform
  - Formula: `vertex_armature_space = inverseBindMatrix * vertex_world_space`

### Bone Hierarchy
Without hierarchy (flat bones):
- All bones are children of root
- Use world space transforms directly
- Simple but loses bone relationships

With hierarchy (from XML):
- Bones arranged in parent-child tree
- Must convert world transforms to local (relative to parent)
- Local transform computed as: `local = inverse(parent_world) * world`

### Prop Points
- **PropPoint array**: Metadata defining attachment points for other models (weapons, shields, etc.)
- **Not additional bones**: PropPoints reference existing bones via `parent bone` index
- **Bone count**: PMD may have more bones than skeleton XML defines
  - Example: horse.pmd has 33 bones, but horse.xml only defines 27
  - Extra bones (27-32) are unnamed in XML, get generic names like "bone_27"
  - These may be LOD bones, special-purpose bones, or bones for specific animations
- **PropPoint metadata**: Stored separately with local offset from parent bone
  - Contains: name, local translation/rotation, parent bone index
  - Used by game engine to position attached objects
  - Not exported as bones to glTF (bones already exist in restStates)

## Node Structure in glTF

```
Node 0: Armature Root ("Horse Biped")
├─ Node 1: Mesh (with skin reference)
├─ Node 2+: Bones in hierarchy
   └─ Skeleton bones (from XML)
   └─ Prop point bones (remaining)
```

## Key Algorithms

### World to Local Transform
```c
// For child bone with parent
Quaternion parent_inv = quat_inverse(parent.rotation);
local.rotation = quat_mul(parent_inv, world.rotation);

Vector3D diff = world.translation - parent.translation;
local.translation = quat_rotate(parent_inv, diff);
```

### Inverse Bind Matrix Computation
```c
// From bone's world transform (translation + rotation quaternion)
1. Convert quaternion to 3x3 rotation matrix
2. Build 4x4 transform: [R | T] where T is translation column
3. Invert: inverse rotation is transpose of R, conjugate of quaternion
4. Inverse translation = -inverse_rotation * original_translation
5. Build inverse matrix in column-major order (glTF format)
```

## Common Issues & Solutions

### Mesh scrambled in edit mode
- **Cause**: Identity inverse bind matrices with world space vertices
- **Fix**: Compute proper IBMs from rest pose transforms

### Bones all at origin with hierarchy
- **Cause**: Using world transforms with hierarchical parent-child relationships
- **Fix**: Convert to local transforms relative to parent

### Missing prop points
- **Cause**: Bone count mismatch, improper parent assignment
- **Fix**: Match prop point names to skeleton bones, use name-based attachment

### Bone count mismatch
- **PMD**: 33 bones (27 skeleton + 6 prop points)
- **PSA**: 34 bones (animation has one extra)
- **Skeleton XML**: 27 bones (canonical hierarchy)
- **Solution**: Handle gracefully, use what's available

## File Naming Conventions

- Model: `horse.pmd`
- Animation: `horse_idle.psa` (suffix after underscore is animation name)
- Skeleton: `horse.xml` (auto-detected, same name as model)
- Output: `horse.gltf`

## Dependencies

- No external libraries (pure C with stdlib/math)
- Little-endian assumed for binary parsing
- Base64 encoding for embedded glTF buffers
- Simple recursive descent XML parser for skeleton files
