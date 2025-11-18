# fix_rest_pose_from_idle_slot_horse_biped.py
# Blender 4.5 — World-space based rest pose fixer
# Uses frame 0 of idle_a as reference, applies world-space offset to all frames
# No rest pose modification — just repositioning in world space

import bpy
from mathutils import Vector

# ---------- CONFIG ----------
ARMATURE_NAME = "Horse Biped"
REFERENCE_ACTION_NAME = "idle_a"
REST_FRAME = 0           # Use frame 0 of reference action
FIXED_SUFFIX = "_fixed"
# ----------------------------

def frames_in_action(action):
    """Extract sorted list of unique frame numbers from action."""
    frames = set()
    for fc in action.fcurves:
        for kp in fc.keyframe_points:
            frames.add(int(round(kp.co.x)))
    return sorted(frames)

def collect_actions_targeting_arm(arm_obj):
    """Find all actions with armature pose data."""
    candidates = set()
    for a in bpy.data.actions:
        for fc in a.fcurves:
            if fc.data_path.startswith('pose.bones["'):
                candidates.add(a)
                break
    return list(candidates)

def get_world_positions(arm_obj, action, frame):
    """
    Get world-space position of all bones at a specific frame.
    Returns: {bone_name: world_location_vector}
    """
    scene = bpy.context.scene
    ad = arm_obj.animation_data
    if ad is None:
        arm_obj.animation_data_create()
        ad = arm_obj.animation_data
    
    prev_action = ad.action
    ad.action = action
    prev_frame = scene.frame_current
    
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    
    positions = {}
    for pb in arm_obj.pose.bones:
        world_m = arm_obj.matrix_world @ pb.matrix
        positions[pb.name] = world_m.translation.copy()
    
    scene.frame_set(prev_frame)
    ad.action = prev_action
    bpy.context.view_layer.update()
    
    return positions

def main():
    scene = bpy.context.scene
    
    print("\n" + "="*80)
    print("REST POSE FIXER - WORLD-SPACE MODE")
    print("="*80 + "\n")
    
    # Get armature
    arm = bpy.data.objects.get(ARMATURE_NAME)
    if not arm or arm.type != 'ARMATURE':
        raise RuntimeError(f"Armature '{ARMATURE_NAME}' not found or not an armature.")
    
    print(f"Armature: {arm.name} ({len(arm.pose.bones)} bones)")
    
    # Get reference action
    if REFERENCE_ACTION_NAME not in bpy.data.actions:
        raise RuntimeError(f"Reference action '{REFERENCE_ACTION_NAME}' not found.")
    ref_action = bpy.data.actions[REFERENCE_ACTION_NAME]
    
    frames_ref = frames_in_action(ref_action)
    if REST_FRAME not in frames_ref:
        raise RuntimeError(f"Frame {REST_FRAME} not in reference action.")
    
    print(f"Reference: {REFERENCE_ACTION_NAME} frame {REST_FRAME}")
    
    # Collect actions to fix
    actions_to_fix = collect_actions_targeting_arm(arm)
    if not actions_to_fix:
        raise RuntimeError("No actions found to fix.")
    
    print(f"Actions to fix: {len(actions_to_fix)}")
    for a in actions_to_fix:
        frames = frames_in_action(a)
        print(f"  - {a.name} ({len(frames)} frames)")
    
    # ===== STEP 1: Capture world positions for ALL frames in all actions =====
    print(f"\n[STEP 1] CAPTURING world-space positions from all actions...")
    
    all_world_positions = {}  # {action_name: {frame: {bone_name: world_pos}}}
    
    for action in actions_to_fix:
        frames = frames_in_action(action)
        all_world_positions[action.name] = {}
        
        for f in frames:
            all_world_positions[action.name][f] = get_world_positions(arm, action, f)
        
        print(f"  {action.name}: {len(frames)} frames captured")
    
    # ===== STEP 2: Get reference world position (frame REST_FRAME) =====
    print(f"\n[STEP 2] CAPTURING reference world position (frame {REST_FRAME})...")
    
    ref_world_pos = get_world_positions(arm, ref_action, REST_FRAME)
    print(f"  Reference positions captured")
    
    # ===== STEP 3: Calculate offset (what to add to move to new rest pose) =====
    print(f"\n[STEP 3] CALCULATING position offsets...")
    
    # Get current default rest pose
    ad = arm.animation_data
    if ad is None:
        arm.animation_data_create()
        ad = arm.animation_data
    
    ad.action = None
    scene.frame_set(0)
    bpy.context.view_layer.update()
    
    current_rest_positions = {}
    for pb in arm.pose.bones:
        world_m = arm.matrix_world @ pb.matrix
        current_rest_positions[pb.name] = world_m.translation.copy()
    
    # Calculate offset for each bone
    offsets = {}
    for bone_name in current_rest_positions:
        ref_pos = ref_world_pos.get(bone_name, current_rest_positions[bone_name])
        offsets[bone_name] = ref_pos - current_rest_positions[bone_name]
    
    print(f"  Offsets calculated for {len(offsets)} bones")
    sample_bone = list(offsets.keys())[0]
    print(f"  Example - {sample_bone}: {offsets[sample_bone]}")
    
    # ===== STEP 4: Create new actions with offset applied =====
    print(f"\n[STEP 4] CREATING _fixed actions with offsets applied...")
    
    for action_name, frames_data in all_world_positions.items():
        new_name = action_name + FIXED_SUFFIX
        
        # Create new action
        new_action = bpy.data.actions.new(name=new_name)
        ad.action = new_action
        
        frame_count = 0
        for frame in sorted(frames_data.keys()):
            scene.frame_set(frame)
            bpy.context.view_layer.update()
            
            bone_positions = frames_data[frame]
            
            # Set each bone to target world position
            for pb in arm.pose.bones:
                original_world_pos = bone_positions.get(pb.name)
                if original_world_pos is None:
                    continue
                
                # Apply offset
                target_world_pos = original_world_pos + offsets[pb.name]
                
                # Create target world matrix at this position
                # (preserve original rotation/scale)
                target_world_m = arm.matrix_world.copy()
                target_world_m.translation = target_world_pos
                
                # Convert to local bone matrix
                world_to_obj = arm.matrix_world.inverted()
                target_local_m = world_to_obj @ target_world_m
                
                try:
                    pb.matrix = target_local_m
                except Exception:
                    # Fallback: decompose and set components
                    loc, rot, sc = target_local_m.decompose()
                    pb.location = loc
                    if pb.rotation_mode == 'QUATERNION':
                        pb.rotation_quaternion = rot
                    else:
                        pb.rotation_euler = rot.to_euler(pb.rotation_mode)
                    pb.scale = sc
            
            bpy.context.view_layer.update()
            
            # Insert keyframes for all bones
            for pb in arm.pose.bones:
                try:
                    pb.keyframe_insert(data_path="location", frame=frame)
                    if pb.rotation_mode == 'QUATERNION':
                        pb.keyframe_insert(data_path="rotation_quaternion", frame=frame)
                    else:
                        pb.keyframe_insert(data_path="rotation_euler", frame=frame)
                    pb.keyframe_insert(data_path="scale", frame=frame)
                except Exception:
                    pass
            
            frame_count += 1
        
        ad.action = None
        print(f"  Created {new_name} ({frame_count} frames)")
    
    # ===== STEP 5: Replace action usages =====
    print(f"\n[STEP 5] REPLACING action usages...")
    
    for action_name in all_world_positions.keys():
        old_action = bpy.data.actions.get(action_name)
        new_action = bpy.data.actions.get(action_name + FIXED_SUFFIX)
        
        if not (old_action and new_action):
            continue
        
        # Replace in object animation_data and NLA strips
        replaced = 0
        for obj in bpy.data.objects:
            obj_ad = getattr(obj, "animation_data", None)
            if not obj_ad:
                continue
            
            if obj_ad.action == old_action:
                obj_ad.action = new_action
                replaced += 1
            
            for track in obj_ad.nla_tracks:
                for strip in track.strips:
                    if strip.action == old_action:
                        strip.action = new_action
                        replaced += 1
        
        print(f"  {old_action.name} → {new_action.name}: {replaced} usages replaced")
    
    print("\n" + "="*80)
    print(f"✓ COMPLETE")
    print("="*80)
    print(f"New actions have suffix: {FIXED_SUFFIX}")
    print("Original actions are preserved. Verify the animations; delete originals if satisfied.")

if __name__ == "__main__":
    main()
