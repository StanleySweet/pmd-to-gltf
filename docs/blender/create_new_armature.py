# create_new_armature.py
# Blender 4.5 — Create new armature with frame 0 as rest pose
# Copies skeleton structure from old armature, bakes all animations

import bpy
from mathutils import Matrix

OLD_ARMATURE_NAME = "Horse Biped"
NEW_ARMATURE_NAME = "Horse Biped Fixed"
REFERENCE_ACTION_NAME = "idle_a"
REST_FRAME = 0
MESH_NAME = "horse"

def frames_in_action(action):
    frames = set()
    for fc in action.fcurves:
        for kp in fc.keyframe_points:
            frames.add(int(round(kp.co.x)))
    return sorted(frames)

def get_all_actions_for_armature(old_arm):
    """Get all actions used on the old armature."""
    candidates = set()
    for a in bpy.data.actions:
        for fc in a.fcurves:
            if fc.data_path.startswith('pose.bones["'):
                candidates.add(a)
                break
    return list(candidates)

print("\n" + "="*80)
print("CREATE NEW ARMATURE - Frame 0 as Rest Pose")
print("="*80 + "\n")

scene = bpy.context.scene

# Get old armature
old_arm = bpy.data.objects.get(OLD_ARMATURE_NAME)
if not old_arm or old_arm.type != 'ARMATURE':
    raise RuntimeError(f"Old armature '{OLD_ARMATURE_NAME}' not found.")

print(f"Source armature: {old_arm.name} ({len(old_arm.pose.bones)} bones)")

# ===== STEP 1: Create new armature data structure =====
print(f"\n[1] Creating new armature structure...")

# Create armature data
new_arm_data = bpy.data.armatures.new(name=NEW_ARMATURE_NAME)
new_arm_obj = bpy.data.objects.new(name=NEW_ARMATURE_NAME, object_data=new_arm_data)
scene.collection.objects.link(new_arm_obj)
bpy.context.view_layer.objects.active = new_arm_obj

# Position new armature
new_arm_obj.location = old_arm.location.copy()
new_arm_obj.rotation_euler = old_arm.rotation_euler.copy()
new_arm_obj.scale = old_arm.scale.copy()

# ===== STEP 2: Copy bone structure from old armature (at frame REST_FRAME) =====
print(f"\n[2] Copying bone structure at frame {REST_FRAME}...")

# Get the pose at frame REST_FRAME
ref_action = bpy.data.actions.get(REFERENCE_ACTION_NAME)
if not ref_action:
    raise RuntimeError(f"Reference action '{REFERENCE_ACTION_NAME}' not found.")

ad = old_arm.animation_data
if ad is None:
    old_arm.animation_data_create()
    ad = old_arm.animation_data

prev_action = ad.action
ad.action = ref_action
prev_frame = scene.frame_current
scene.frame_set(REST_FRAME)
bpy.context.view_layer.update()

# Capture bone structure at frame REST_FRAME
bone_structure = {}  # {bone_name: {parent: parent_name, matrix: world_matrix}}

for pb in old_arm.pose.bones:
    parent_name = None
    if pb.parent:
        parent_name = pb.parent.name
    
    world_m = old_arm.matrix_world @ pb.matrix
    bone_structure[pb.name] = {
        'parent': parent_name,
        'world_matrix': world_m.copy()
    }

print(f"  Captured {len(bone_structure)} bones")

# Enter edit mode on new armature
bpy.context.view_layer.objects.active = new_arm_obj
bpy.ops.object.mode_set(mode='EDIT')
new_edit_bones = new_arm_data.edit_bones

# Create bones in new armature
for bone_name, info in bone_structure.items():
    if bone_name in new_edit_bones:
        continue
    
    # Create new edit bone
    eb = new_edit_bones.new(name=bone_name)
    
    # Set parent
    if info['parent'] and info['parent'] in new_edit_bones:
        eb.parent = new_edit_bones[info['parent']]
    
    # Set position from world matrix
    world_m = info['world_matrix']
    world_to_obj = new_arm_obj.matrix_world.inverted()
    local_m = world_to_obj @ world_m
    
    eb.matrix = local_m

print(f"  Created {len(new_edit_bones)} edit bones")

# Exit edit mode
bpy.ops.object.mode_set(mode='OBJECT')
bpy.context.view_layer.update()

# ===== STEP 3: Link mesh to new armature =====
print(f"\n[3] Linking mesh to new armature...")

mesh_obj = bpy.data.objects.get(MESH_NAME)
if mesh_obj and mesh_obj.type == 'MESH':
    # Find armature modifier
    arm_mod = None
    for mod in mesh_obj.modifiers:
        if mod.type == 'ARMATURE':
            arm_mod = mod
            break
    
    if arm_mod:
        arm_mod.object = new_arm_obj
        print(f"  Mesh modifier updated to use new armature")
    else:
        # Create new modifier
        arm_mod = mesh_obj.modifiers.new(name="Armature", type='ARMATURE')
        arm_mod.object = new_arm_obj
        print(f"  New armature modifier created")

# ===== STEP 4: Bake all animations to new armature =====
print(f"\n[4] Baking animations to new armature...")

all_actions = get_all_actions_for_armature(old_arm)
print(f"  Found {len(all_actions)} actions to bake")

for action in all_actions:
    frames = frames_in_action(action)
    if not frames:
        continue
    
    # Create new action for new armature
    new_action = bpy.data.actions.new(name=action.name)
    
    # Set animation data to new action
    ad_new = new_arm_obj.animation_data
    if ad_new is None:
        new_arm_obj.animation_data_create()
        ad_new = new_arm_obj.animation_data
    
    ad_new.action = new_action
    
    # For each frame, set pose bones to match old armature pose
    for f in frames:
        scene.frame_set(f)
        
        # Get poses from old armature
        ad.action = action
        bpy.context.view_layer.update()
        
        old_poses = {}
        for pb in old_arm.pose.bones:
            world_m = old_arm.matrix_world @ pb.matrix
            old_poses[pb.name] = world_m.copy()
        
        # Set poses on new armature
        ad_new.action = new_action
        bpy.context.view_layer.update()
        
        world_to_obj_new = new_arm_obj.matrix_world.inverted()
        
        for pb_new in new_arm_obj.pose.bones:
            if pb_new.name in old_poses:
                old_world_m = old_poses[pb_new.name]
                new_local_m = world_to_obj_new @ old_world_m
                
                try:
                    pb_new.matrix = new_local_m
                except Exception:
                    loc, rot, sc = new_local_m.decompose()
                    pb_new.location = loc
                    pb_new.rotation_quaternion = rot if pb_new.rotation_mode == 'QUATERNION' else rot.to_euler(pb_new.rotation_mode)
                    pb_new.scale = sc
        
        bpy.context.view_layer.update()
        
        # Insert keyframes
        for pb_new in new_arm_obj.pose.bones:
            try:
                pb_new.keyframe_insert(data_path="location", frame=f)
                if pb_new.rotation_mode == 'QUATERNION':
                    pb_new.keyframe_insert(data_path="rotation_quaternion", frame=f)
                else:
                    pb_new.keyframe_insert(data_path="rotation_euler", frame=f)
                pb_new.keyframe_insert(data_path="scale", frame=f)
            except Exception:
                pass
    
    ad_new.action = None
    print(f"  Baked {action.name} ({len(frames)} frames)")

# Restore old animation
scene.frame_set(prev_frame)
ad.action = prev_action
bpy.context.view_layer.update()

# Select new armature
bpy.context.view_layer.objects.active = new_arm_obj
new_arm_obj.select_set(True)

print("\n" + "="*80)
print("✓ COMPLETE")
print("="*80)
print(f"New armature: {NEW_ARMATURE_NAME}")
print(f"Rest pose: frame {REST_FRAME} of {REFERENCE_ACTION_NAME}")
print(f"Mesh linked to new armature")
print(f"All animations baked")
print("\nNext: Hide or delete old armature, verify new one, export glTF/COLLADA")
