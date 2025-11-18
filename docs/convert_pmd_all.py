#!/usr/bin/env python3
"""
PMD Multi-format Converter - Python 3
Converts PMD files to multiple formats (COLLADA, basic info, etc.)
"""

import sys
import os
from pmd2collada_py3 import convert_pmd_to_collada, load_pmd

def convert_pmd_all_formats(pmd_file: str):
    """Convert PMD to all available formats"""
    
    if not os.path.exists(pmd_file):
        print(f"Error: {pmd_file} not found")
        return False
    
    base_name = os.path.splitext(pmd_file)[0]
    
    print(f"Converting {pmd_file} to multiple formats...")
    
    # 1. Load and validate PMD
    pmd = load_pmd(pmd_file)
    if not pmd:
        return False
    
    # 2. Convert to COLLADA
    dae_file = base_name + ".dae"
    print(f"\n1. Converting to COLLADA...")
    if convert_pmd_to_collada(pmd_file, dae_file):
        size = os.path.getsize(dae_file)
        print(f"   ✅ Created {dae_file} ({size:,} bytes)")
    else:
        print(f"   ❌ Failed to create COLLADA")
    
    # 3. Create summary report
    report_file = base_name + "_report.txt"
    print(f"\n2. Creating analysis report...")
    try:
        with open(report_file, 'w') as f:
            f.write(f"PMD Analysis Report: {os.path.basename(pmd_file)}\n")
            f.write("=" * 50 + "\n\n")
            
            f.write(f"File: {pmd_file}\n")
            f.write(f"Magic: {pmd.magic}\n")
            f.write(f"Version: {pmd.version}\n")
            f.write(f"Data size: {pmd.datasize:,} bytes\n\n")
            
            f.write(f"Geometry:\n")
            f.write(f"  Vertices: {pmd.numVertices:,}\n")
            f.write(f"  Faces: {pmd.numFaces:,}\n")
            f.write(f"  UV sets: {pmd.numTexCoords}\n\n")
            
            f.write(f"Animation:\n")
            f.write(f"  Bones: {pmd.numBones}\n")
            f.write(f"  Prop points: {pmd.numPropPoints}\n\n")
            
            if pmd.propPoints:
                f.write("Prop Points:\n")
                for i, prop in enumerate(pmd.propPoints):
                    f.write(f"  {i}: '{prop.name}' -> bone {prop.bone}\n")
                f.write("\n")
            
            f.write("Export Results:\n")
            if os.path.exists(dae_file):
                size = os.path.getsize(dae_file)
                f.write(f"  COLLADA: {os.path.basename(dae_file)} ({size:,} bytes)\n")
            
        print(f"   ✅ Created {report_file}")
        
    except Exception as e:
        print(f"   ❌ Failed to create report: {e}")
    
    print(f"\n✅ Conversion complete! Files created:")
    if os.path.exists(dae_file):
        print(f"   - {dae_file}")
    if os.path.exists(report_file):
        print(f"   - {report_file}")
    
    return True

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 convert_pmd_all.py <pmd_file>")
        print("  Converts PMD to COLLADA + creates analysis report")
        sys.exit(1)
    
    success = convert_pmd_all_formats(sys.argv[1])
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
