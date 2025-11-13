# PMD/PSA to glTF Converter

Converts 3D models and animations from PMD (model) and PSA (animation) formats to glTF 2.0.

## Build

```bash
gcc -o converter main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c -lm -static
```

## Usage

```bash
./converter model.pmd animation.psa [output.gltf] [skeleton.xml] [skeleton_id]
```

The converter auto-detects `model.xml` skeleton file if not specified.

## Features

- **Skeleton hierarchy** - Loads bone parent-child relationships from XML
- **Bone naming** - Uses skeleton XML for bone names; bones beyond XML definition get generic names (bone_27, etc.)
- **Prop points** - PMD propPoints are metadata for attachment, not additional bones
- **Inverse bind matrices** - Identity matrices since PMD vertices match rest pose
- **Animation names** - Extracts animation name from filename suffix (e.g., `horse_idle.psa` → "idle")
- **Mesh naming** - Extracts mesh name from PMD filename

## File Structure

- **PMD bones**: May contain more bones than skeleton XML defines (e.g., 33 vs 27)
  - Bones 0-26: Defined in skeleton XML with proper names
  - Bones 27-32: Extra bones (tail segments, IK bones, etc.) with generic names
- **PropPoints**: Separate metadata array defining attachment points
  - References existing bones via parent bone index
  - Contains local offset transforms for positioning attached objects
  - Not exported as additional bones (bones already exist in restStates)

## File Formats

- **PMD**: Mesh with vertices, normals, UVs, skinning weights, and skeleton rest pose
- **PSA**: Skeletal animation with bone transforms per frame
- **XML**: Skeleton hierarchy definition with bone names and parent relationships
- **glTF**: Output format supporting mesh, skeleton, and animation data
