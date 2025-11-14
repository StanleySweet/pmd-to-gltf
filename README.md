# PMD/PSA to glTF Converter

Converts 3D models and animations from PMD (model) and PSA (animation) formats to glTF 2.0.

## Build

### Using Make (Recommended)

```bash
make              # Build with default compiler
make static       # Build with static linking
make mingw-x86_64 # Cross-compile for Windows 64-bit
make mingw-i686   # Cross-compile for Windows 32-bit
make test         # Build and run tests
```

### Manual Build

```bash
gcc -o converter main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c -lm -static
```

### MinGW Cross-Compilation

To build Windows executables from Linux:

```bash
# Install MinGW
sudo apt-get install mingw-w64

# Build for Windows 64-bit
x86_64-w64-mingw32-gcc -o converter.exe main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c -lm -static

# Build for Windows 32-bit
i686-w64-mingw32-gcc -o converter.exe main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c -lm -static
```

## Usage

```bash
./converter model.pmd animation.psa [output.gltf] [skeleton.xml] [skeleton_id]
```

The converter auto-detects `model.xml` skeleton file if not specified.

## CI/CD

This project uses GitHub Actions for continuous integration:

- **Build workflow** (`.github/workflows/build.yml`):
  - Linux builds with GCC and Clang
  - macOS builds
  - MinGW cross-compilation (x86_64 and i686)
  - MinGW native builds on Windows via MSYS2

- **Lint workflow** (`.github/workflows/lint.yml`):
  - Cppcheck static analysis
  - Clang-format code formatting checks
  - Clang-tidy analysis
  - Strict compiler warnings

All workflows run on push and pull requests to main branches.

## Development

### Code Quality Tools

```bash
make lint    # Run static analysis with cppcheck
make format  # Format code with clang-format
```

### Linting

Install linting tools:

```bash
sudo apt-get install cppcheck clang-format clang-tidy
```

Run checks:

```bash
cppcheck --enable=all --suppress=missingIncludeSystem --std=c99 *.c *.h
clang-format -i *.c *.h  # Format in-place
```

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
