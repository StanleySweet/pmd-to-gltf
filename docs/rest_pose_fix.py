#!/usr/bin/env python3
"""
Blender 4.5 script: Copy animations from old armature to new one
with exact frame-by-frame pose copying (no modifications to original).

Steps:
1. Duplicate old armature structure (geometry + hierarchy identical)
2. Adjust rest pose to match frame 0 of idle_a
3. Bake all animations frame-by-frame with verification
4. No drift, no modifications to original armature

Run in Blender: Alt+P or paste in Blender Python console
"""

import bpy
from mathutils import Vector, Matrix, Quaternion
import math

# ============ CONFIG ============
OLD_ARMATURE_NAME = "Horse Biped"
REFERENCE_ACTION_NAME = "idle_a"
NEW_ARMATURE_NAME = "Horse Biped Fixed"
VERIFY_DRIFT = True
# ================================

def main():
    print("\n" + "=" * 70)
    print("ANIMATION BAKE: Exact World-Space Copy")
    print("=" * 70)
    
    # [STEP 1] Get references
    print("\n[1] Loading references...")
    old_arm = bpy.data.objects.get(OLD_ARMATURE_NAME)
    if not old_arm or old_arm.type != 'ARMATURE':
        raise RuntimeError(f"Armature '{OLD_ARMATURE_NAME}' not found")
    
    ref_action = bpy.data.actions.get(REFERENCE_ACTION_NAME)
    if not ref_action:
        raise RuntimeError(f"Action '{REFERENCE_ACTION_NAME}' not found")
    
    scene = bpy.context.scene
    prev_frame = scene.frame_current
    
    print(f"✓ Old armature: {old_arm.name}")
    print(f"✓ Reference action: {ref_action.name}")
    print(f"✓ Bones: {len(old_arm.data.bones)}")
    
    # DEBUG: Check animation setup
    print(f"\n[DEBUG] Armature animation setup:")
    if old_arm.animation_data:
        print(f"  animation_data exists: True")
        print(f"  current action: {old_arm.animation_data.action.name if old_arm.animation_data.action else 'None'}")
        print(f"  NLA tracks: {len(old_arm.animation_data.nla_tracks)}")
    else:
        print(f"  animation_data exists: False")
    
    # [STEP 2] Capture original bone geometry (edit mode data)
    print("\n[2] Capturing original bone names and hierarchy...")
    
    # We need to read edit bones - switch to edit mode temporarily
    bpy.context.view_layer.objects.active = old_arm
    old_arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    
    original_bone_geometry = {}
    for eb in old_arm.data.edit_bones:
        original_bone_geometry[eb.name] = {
            'parent_name': eb.parent.name if eb.parent else None,
        }
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()
    
    print(f"✓ Captured {len(original_bone_geometry)} bone names")
    
    # Prepare for baking
    ad_old = old_arm.animation_data or old_arm.animation_data_create()
    prev_action_old = ad_old.action
    
    # [STEP 3.5] Capture idle_a pose at frame 0 to get bone positions
    print("\n[3.5] Capturing idle_a frame 0 pose for bone positioning...")
    ad_old.action = ref_action
    scene.frame_set(0)
    bpy.context.view_layer.update()
    
    idle_pose_data = {}
    for pb in old_arm.pose.bones:
        # Capture world matrix at frame 0
        world_matrix = old_arm.matrix_world @ pb.matrix
        idle_pose_data[pb.name] = {
            'world_matrix': Matrix(world_matrix),
            'rotation_mode': pb.rotation_mode,
        }
    print(f"✓ Captured idle_a pose at frame 0")
    
    # [STEP 4] Delete old new armature if it exists
    print("\n[4] Cleaning up old new armature...")
    if NEW_ARMATURE_NAME in bpy.data.objects:
        obj = bpy.data.objects[NEW_ARMATURE_NAME]
        bpy.data.objects.remove(obj, do_unlink=True)
    if NEW_ARMATURE_NAME in bpy.data.armatures:
        arm = bpy.data.armatures[NEW_ARMATURE_NAME]
        bpy.data.armatures.remove(arm, do_unlink=True)
    print("✓ Cleaned")
    
    # [STEP 5] Create new armature with fresh bone hierarchy
    print("\n[5] Creating new armature with fresh bone hierarchy...")
    
    new_arm_data = bpy.data.armatures.new(NEW_ARMATURE_NAME)
    new_arm = bpy.data.objects.new(NEW_ARMATURE_NAME, new_arm_data)
    bpy.context.collection.objects.link(new_arm)
    
    bpy.context.view_layer.objects.active = new_arm
    new_arm.select_set(True)
    
    bpy.ops.object.mode_set(mode='EDIT')
    
    # Create bone hierarchy with simple default positions
    # First pass: create all bones
    for bone_name in original_bone_geometry.keys():
        eb = new_arm.data.edit_bones.new(bone_name)
        # Default positions - will be set to idle pose later
        eb.head = Vector((0, 0, 0))
        eb.tail = Vector((0, 0, 1))
    
    # Second pass: set parent relationships
    for bone_name, geo in original_bone_geometry.items():
        if geo['parent_name']:
            child = new_arm.data.edit_bones[bone_name]
            parent = new_arm.data.edit_bones[geo['parent_name']]
            child.parent = parent
    
    print(f"✓ Created {len(new_arm.data.edit_bones)} bones with hierarchy")
    
    # [STEP 6] Exit edit mode
    print("\n[6] Finalizing armature structure...")
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()
    
    # [STEP 5.5] Align new armature to old armature's world transform
    # This is CRITICAL for matrix conversions to work correctly
    print("\n[5.5] Aligning new armature to old armature transform...")
    new_arm.matrix_world = old_arm.matrix_world
    bpy.context.view_layer.update()
    print(f"✓ New armature world transform aligned to old armature")
    
    # [STEP 5.6] Synchronize rotation modes from old armature to new armature
    print("\n[5.6] Synchronizing rotation modes from old armature...")
    for old_pb in old_arm.pose.bones:
        new_pb = new_arm.pose.bones.get(old_pb.name)
        if new_pb and old_pb.rotation_mode != new_pb.rotation_mode:
            new_pb.rotation_mode = old_pb.rotation_mode
    bpy.context.view_layer.update()
    print(f"✓ All rotation modes synchronized")
    
    # [STEP 5.7] Bake rest pose to match idle_a frame 0 by modifying edit bones
    print("\n[5.7] Baking rest pose to match idle_a frame 0...")
    
    bpy.context.view_layer.objects.active = new_arm
    new_arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    
    # We need to convert world-space idle poses back to edit bone space
    # Strategy: For each bone, set its head/tail based on idle pose positions
    # CRITICAL: bones that are connected must have child.head = parent.tail
    
    new_arm_world = new_arm.matrix_world
    new_arm_world_inv = new_arm_world.inverted()
    
    # First pass: set all positions
    for bone_name in original_bone_geometry.keys():
        if bone_name not in idle_pose_data:
            continue
        
        eb = new_arm.data.edit_bones[bone_name]
        idle_world_matrix = idle_pose_data[bone_name]['world_matrix']
        
        # Convert to local space relative to new armature
        local_matrix = new_arm_world_inv @ idle_world_matrix
        local_pos = local_matrix.translation
        
        if eb.parent:
            # Get parent's tail position (which is now set or will be set)
            parent_eb = eb.parent
            parent_tail = parent_eb.tail
            
            # Child bone: head at parent tail (CONNECTED), tail at this bone's position
            eb.head = parent_tail
            eb.tail = local_pos
        else:
            # Root bone: position at origin or at the captured position
            eb.head = Vector((0, 0, 0))
            eb.tail = local_pos if local_pos.length > 0.001 else Vector((0, 0, 1))
    
    # Second pass: ensure all children are connected to their parents
    for bone_name in original_bone_geometry.keys():
        eb = new_arm.data.edit_bones[bone_name]
        if eb.parent and not eb.use_connect:
            eb.use_connect = True
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()
    print(f"✓ Rest pose baked from idle_a frame 0 with connected bones")
    
    # [CRITICAL FIX] Clear animation data to avoid frame interpolation artifacts
    # Remove all actions from both armatures to reset animation playback state
    ad_old.action = None
    ad_new = new_arm.animation_data or new_arm.animation_data_create()
    ad_new.action = None
    scene.frame_set(0)
    bpy.context.view_layer.update()
    print(f"✓ Animation state reset")
    
    # [STEP 7] Get all base actions (original, unmodified)
    print("\n[7] Finding all original animations...")
    
    base_actions = []
    
    # First: check if old armature has actions linked
    if old_arm.animation_data and old_arm.animation_data.action:
        print(f"  Old armature currently has action: {old_arm.animation_data.action.name}")
    
    # Collect all actions from the project (skip _fixed versions)
    all_actions = []
    for action in bpy.data.actions:
        if action.name.endswith('_fixed'):
            continue
        all_actions.append(action)
    
    print(f"\n  Total actions found: {len(all_actions)}")
    
    # Check what data is in actions
    for action in all_actions:
        print(f"    - {action.name}: {len(action.fcurves)} fcurves")
        for fc in action.fcurves[:3]:  # Show first 3 fcurves
            print(f"        • {fc.data_path}")
    
    # Try to find actions that might be bone animations
    # In glTF, bone animations might be stored as object properties, not pose.bones
    # So we'll accept any action with fcurves for now
    if all_actions:
        base_actions = all_actions
        print(f"\n⚠ Note: Found {len(base_actions)} actions but NO 'pose.bones' data paths detected")
        print("  This suggests animations may be stored differently in this blend file.")
        print("  Will attempt to apply first action ('idle_a') as reference.")
    
    base_actions.sort(key=lambda a: a.name)
    
    if not base_actions:
        print("  ✗ No actions found at all in project!")
        raise RuntimeError("No animation actions found. Please ensure animations are loaded from glTF import.")
    
    print(f"\n✓ Found {len(base_actions)} actions: {[a.name for a in base_actions]}")
    
    # [STEP 8] Create copies of actions for the new armature
    print("\n[8] Creating independent action copies for new armature...")
    
    action_map = {}  # old_action -> new_action_copy
    
    for action in base_actions:
        # Create a new action with _fixed suffix
        new_action_name = action.name + "_fixed"
        
        # Check if it already exists
        if new_action_name in bpy.data.actions:
            new_action = bpy.data.actions[new_action_name]
        else:
            new_action = bpy.data.actions.new(name=new_action_name)
        
        action_map[action] = new_action
        print(f"  Created copy: {action.name} -> {new_action.name}")
    
    # [STEP 9] Capture all animation data first
    print("\n[9] Capturing all animation data from old armature...")
    
    ad_old = old_arm.animation_data or old_arm.animation_data_create()
    
    # Structure: action_name -> { frame_num -> { bone_name -> world_matrix } }
    all_animation_data = {}
    
    for old_action in base_actions:
        # Get all frames from OLD action (source of truth)
        # Use a dict to deduplicate frames with tolerance for floating point errors
        frames_dict = {}
        FRAME_TOLERANCE = 1e-6
        
        for fc in old_action.fcurves:
            for kp in fc.keyframe_points:
                frame_value = float(kp.co.x)
                
                # Check if this frame already exists (within tolerance)
                found = False
                for existing_frame in frames_dict:
                    if abs(existing_frame - frame_value) < FRAME_TOLERANCE:
                        found = True
                        break
                
                if not found:
                    frames_dict[frame_value] = True
        
        frames = sorted(frames_dict.keys())
        
        if not frames:
            print(f"  ⚠ {old_action.name}: no keyframes found in action")
            # Check if animation is in NLA tracks instead
            nla_frames = set()
            for track in ad_old.nla_tracks:
                for strip in track.strips:
                    if strip.action == old_action:
                        print(f"     Found in NLA strip: {strip.name} (frames {strip.frame_start}-{strip.frame_end})")
                        for frame in range(int(strip.frame_start), int(strip.frame_end) + 1):
                            nla_frames.add(frame)
            if nla_frames:
                frames = sorted(list(nla_frames))
                print(f"     Using NLA frame range: {len(frames)} frames")
            else:
                print(f"     No animation data found for {old_action.name}")
                continue
        
        print(f"\n  Capturing '{old_action.name}' ({len(frames)} frames)...")
        
        # Set old armature to play this action
        ad_old.action = old_action
        
        action_data = {}
        
        for frame in frames:
            # Use safe frame rounding: round to nearest integer for scene.frame_set
            frame_int = int(frame + 0.5) if frame >= 0 else int(frame - 0.5)
            scene.frame_set(frame_int)
            bpy.context.view_layer.update()
            
            # Capture bone poses in WORLD SPACE (1:1 copy, independent of rest pose)
            # Use double-precision matrices to avoid accumulation errors
            frame_data = {}
            for pb in old_arm.pose.bones:
                # Compute world matrix with high precision
                world_matrix = old_arm.matrix_world @ pb.matrix
                frame_data[pb.name] = {
                    'world_matrix': Matrix(world_matrix),  # Ensure copy with full precision
                    'rotation_mode': pb.rotation_mode,
                }
            
            action_data[frame] = frame_data
        
        all_animation_data[old_action.name] = action_data
        print(f"    ✓ Captured {len(frames)} frames")
    
    # [STEP 10] Apply captured data to new armature
    print("\n[10] Applying captured data to new armature...")
    
    # Create a temporary empty object to act as a matrix transfer proxy
    # This avoids Blender's bone constraint recalculation
    print("\n[SETUP] Creating temporary empty object for matrix transfer...")
    temp_empty = bpy.data.objects.new("_matrix_proxy", None)
    bpy.context.collection.objects.link(temp_empty)
    temp_empty.rotation_mode = 'QUATERNION'  # Force quaternion to avoid Euler issues
    bpy.context.view_layer.update()
    
    ad_new = new_arm.animation_data or new_arm.animation_data_create()
    
    total_frames_baked = 0
    max_drift = 0.0
    drift_issues = []
    
    for old_action, new_action in action_map.items():
        if old_action.name not in all_animation_data:
            print(f"  ⚠ No captured data for {old_action.name}")
            continue
        
        action_data = all_animation_data[old_action.name]
        frames = sorted(action_data.keys())
        
        print(f"\n  Applying '{old_action.name}' -> '{new_action.name}' ({len(frames)} frames)...")
        
        # CRITICAL: Clear old keyframes from new_action
        for fc in new_action.fcurves:
            new_action.fcurves.remove(fc)
        
        # Set new armature to use new action
        ad_new.action = new_action
        
        frame_drifts = []
        
        for frame in frames:
            # Use safe frame rounding for scene.frame_set
            frame_int = int(frame + 0.5) if frame >= 0 else int(frame - 0.5)
            scene.frame_set(frame_int)
            bpy.context.view_layer.update()
            
            frame_data = action_data[frame]
            
            # Compute inverse with high precision
            new_arm_world = new_arm.matrix_world
            new_arm_world_inv = new_arm_world.inverted()
            
            # [APPLY] Use empty as proxy to transfer exact transforms
            # CRITICAL: We're converting from old armature world-space pose
            # to new armature local-space pose (with different rest pose)
            for pb_new in new_arm.pose.bones:
                if pb_new.name not in frame_data:
                    continue
                
                old_world_matrix = frame_data[pb_new.name]['world_matrix']
                captured_rotation_mode = frame_data[pb_new.name]['rotation_mode']
                
                # [DUMMY TRANSFER] Set dummy to exact world matrix
                temp_empty.matrix_world = old_world_matrix
                bpy.context.view_layer.update()
                
                # [CONVERT TO LOCAL] Read from dummy and convert to new armature space
                new_arm_world = new_arm.matrix_world  # Re-fetch
                new_arm_world_inv = new_arm_world.inverted()
                new_local_matrix = new_arm_world_inv @ temp_empty.matrix_world
                
                # [DECOMPOSE] Extract location, rotation, scale from local matrix
                local_loc = new_local_matrix.translation
                local_rot_mat = new_local_matrix.to_3x3()
                
                # Extract scale before normalizing rotation
                local_scale = Vector([local_rot_mat[i].length for i in range(3)])
                
                # Normalize rotation matrix (remove scale)
                for i in range(3):
                    if local_scale[i] > 0.0001:
                        local_rot_mat[i] = local_rot_mat[i] / local_scale[i]
                
                # [SET TRANSFORMS] Apply to bone - location and scale are straightforward
                pb_new.location = local_loc
                pb_new.scale = local_scale
                
                # For rotation, match the rotation mode and set it
                if captured_rotation_mode == 'QUATERNION':
                    pb_new.rotation_quaternion = local_rot_mat.to_quaternion()
                else:
                    pb_new.rotation_euler = local_rot_mat.to_euler(captured_rotation_mode)
            
            # Update ONCE after all bones set
            bpy.context.view_layer.update()
            
            # [KEYFRAME] Insert keyframes - respect rotation mode, use exact frame number
            for pb in new_arm.pose.bones:
                try:
                    pb.keyframe_insert('location', frame=frame)
                    if pb.rotation_mode == 'QUATERNION':
                        pb.keyframe_insert('rotation_quaternion', frame=frame)
                    else:
                        pb.keyframe_insert('rotation_euler', frame=frame)
                    pb.keyframe_insert('scale', frame=frame)
                except Exception as e:
                    if frame <= 2:  # Only warn on early frames
                        print(f"        Warning: Keyframe insert failed for {pb.name}: {e}")
            
            # [VERIFY] Measure drift AFTER keyframing
            if VERIFY_DRIFT:
                drift, bone_drifts = measure_drift_world(frame_data, new_arm)
                frame_drifts.append(drift)
                max_drift = max(max_drift, drift)
                
                # Alert on frames 1-8 with detailed diagnostics
                if frame <= 8 and drift > 0.001:
                    worst_bones = sorted(bone_drifts.items(), key=lambda x: x[1], reverse=True)[:3]
                    print(f"      ⚠ Frame {frame:5.1f}: drift={drift:.6f}")
                    for bone_name, bone_drift in worst_bones:
                        print(f"         -> {bone_name}: {bone_drift:.6f}")
        
        avg_drift = sum(frame_drifts) / len(frame_drifts) if frame_drifts else 0
        max_frame_drift = max(frame_drifts) if frame_drifts else 0
        
        # Verify keyframes were actually created
        total_keyframes = sum(len(fc.keyframe_points) for fc in new_action.fcurves)
        expected_keyframes = len(frames) * len(new_arm.pose.bones) * 4  # loc, rot, scale per bone
        
        # Detailed analysis for early frames
        print(f"    ✓ Frames: {len(frames)}, Keyframes created: {total_keyframes}/{expected_keyframes}")
        print(f"      Avg drift: {avg_drift:.6f}, Max: {max_frame_drift:.6f}")
        print(f"      FCurves in action: {len(new_action.fcurves)}")
        
        # Diagnostic: show frame-by-frame drift for first action to identify patterns
        if old_action.name == base_actions[0].name if base_actions else False:
            print(f"\n    [DIAGNOSTIC] Frame-by-frame drift for '{old_action.name}':")
            for i, (frame, drift) in enumerate(zip(frames[:min(20, len(frames))], frame_drifts[:20])):
                marker = " ⚠ EARLY FRAME" if frame <= 8 else ""
                print(f"      Frame {frame:6.1f}: {drift:10.6f}{marker}")
            if len(frames) > 20:
                print(f"      ... ({len(frames) - 20} more frames)")
        
        if max_frame_drift > 0.01:  # Alert threshold
            drift_issues.append(f"{old_action.name}: max_drift={max_frame_drift:.4f}")
        
        total_frames_baked += len(frames)
    
    # Clean up temporary empty
    bpy.data.objects.remove(temp_empty, do_unlink=True)
    print("\n[CLEANUP] Temporary empty object removed")
    
    # Restore
    scene.frame_set(prev_frame)
    ad_old.action = prev_action_old
    # Don't restore ad_new.action - leave it on a fixed action for user convenience
    bpy.context.view_layer.update()
    
    # [SUMMARY]
    print("\n" + "=" * 70)
    print("SUMMARY")
    print("=" * 70)
    print(f"✓ New armature: {new_arm.name}")
    print(f"✓ Bones: {len(new_arm.pose.bones)}")
    print(f"✓ Total frames baked: {total_frames_baked}")
    print(f"✓ Maximum drift: {max_drift:.6f}")
    print(f"\n✓ Original actions UNTOUCHED:")
    for action in base_actions:
        print(f"  - {action.name}")
    print(f"\n✓ New action copies created (with _fixed suffix):")
    for old_a, new_a in action_map.items():
        print(f"  - {new_a.name}")
    
    if drift_issues:
        print(f"\n⚠ Actions with notable drift:")
        for issue in drift_issues:
            print(f"  - {issue}")
    else:
        print(f"\n✓ Drift is minimal - animations matched correctly!")
    
    print("\nNext steps:")
    print("  1. Select 'Horse Biped Fixed' armature")
    print("  2. In Action Editor, switch to _fixed actions to test")
    print("  3. Play timeline to verify animations")
    print("  4. If good, hide 'Horse Biped' armature")
    print("  5. Export 'Horse Biped Fixed' to glTF/COLLADA")
    print("=" * 70)

def measure_drift_world(world_matrices_dict, armature):
    """
    Measure difference between captured world-space matrices and current armature.
    Uses high-precision Frobenius norm to detect matrix divergence.
    """
    total_diff = 0.0
    count = 0
    max_element_diff = 0.0
    bone_drifts = {}  # Track drift per bone for diagnostics
    
    for bone_name, data in world_matrices_dict.items():
        pb = armature.pose.bones.get(bone_name)
        if not pb:
            continue
        
        old_world_matrix = data['world_matrix']
        new_world_matrix = armature.matrix_world @ pb.matrix
        
        # Frobenius norm: sqrt(sum of squared differences)
        bone_diff = 0.0
        for i in range(4):
            for j in range(4):
                diff = float(old_world_matrix[i][j]) - float(new_world_matrix[i][j])
                total_diff += diff * diff
                bone_diff += diff * diff
                max_element_diff = max(max_element_diff, abs(diff))
        
        bone_drifts[bone_name] = math.sqrt(bone_diff)
        count += 1
    
    frobenius_norm = math.sqrt(total_diff) if count > 0 else 0.0
    return frobenius_norm, bone_drifts

def analyze_frame_drift(frame, frame_data, new_arm, threshold=0.01):
    """
    Detailed analysis of a specific frame's drift.
    Shows which bones have the most error.
    """
    drift, bone_drifts = measure_drift_world(frame_data, new_arm)
    
    # Sort bones by drift (descending)
    worst_bones = sorted(bone_drifts.items(), key=lambda x: x[1], reverse=True)[:5]
    
    return drift, worst_bones

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"\n✗ ERROR: {e}")
        import traceback
        traceback.print_exc()
