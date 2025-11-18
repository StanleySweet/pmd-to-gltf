# PMD/PSA to glTF Converter

Converts 3D models and animations from PMD (model) and PSA (animation) formats to glTF 2.0.

## Build

### Using CMake

```bash
# Quick start
cmake --preset default && cmake --build --preset default

# Or step by step
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
cd build && ctest

# Available presets
cmake --list-presets

# Build configurations
cmake --preset default    # Release build
cmake --preset debug      # Debug build
cmake --build --preset default
cmake --build --preset debug

# Clean
rm -rf build

# Install (optional)
cmake --install build
```

### Legacy Make Build

```bash
make              # Build with default compiler
make static       # Build with static linking
make test         # Build and run tests
```


x86_64-w64-mingw32-gcc -o converter.exe main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c -lm -static

# Build for Windows 32-bit
i686-w64-mingw32-gcc -o converter.exe main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c -lm -static
```

## Usage

```bash
./converter <base_name> [--print-bones]
```

- Loads: `<base_name>.pmd`, `<base_name>.xml`, `<base_name>_*.psa`
- Outputs: `output/<filename>.gltf` (where filename is extracted from base_name)
- The converter automatically detects skeleton ID from the XML file
- Example: `./converter input/horse` (auto-detects skeleton ID from input/horse.xml, loads input/horse.pmd, input/horse_*.psa → outputs output/horse.gltf)
- Use `--print-bones` to display bone hierarchy information

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
