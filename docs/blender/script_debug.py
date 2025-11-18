# fix_rest_pose_from_idle_slot_horse_biped.py
# Blender 4.5 — specialized for:
#   armature object name: 'Horse Biped'
#   reference action: bpy.data.actions['idle']
#
# WARNING: BACKUP your .blend before running. This edits edit bones and creates new actions.
# DEBUG VERSION with extensive logging

import bpy
from mathutils import Matrix
from collections import defaultdict

# ---------- CONFIG ----------
ARMATURE_NAME = "Horse Biped"
REFERENCE_ACTION_NAME = "idle_a"
REST_FRAME = 10           # None -> use first keyframe of reference action
FIXED_SUFFIX = "_fixed"
# ----------------------------

def get_armature(name):
    obj = bpy.data.objects.get(name)
    if not obj:
        raise RuntimeError(f"Armature object named '{name}' not found.")
    if obj.type != 'ARMATURE':
        raise RuntimeError(f"Object '{name}' is not an armature.")
    return obj

def find_slot_for_armature_on_action(action, arm_obj):
    """
    Find slot key + slot object on the action that corresponds to arm_obj.
    Returns (slot_key, slot_obj) or (None, None) if not found.
    Tries multiple heuristics: slot.id, slot.name_display, keys.
    """
    if not hasattr(action, "slots"):
        return (None, None)
    slots = action.slots
    # iterate items if possible
    try:
        items = list(slots.items())
    except Exception:
        # fallback: iterate dir
        items = []
        for k in getattr(slots, "keys", lambda: [])():
            try:
                items.append((k, slots[k]))
            except Exception:
                pass

    for key, slot in items:
        try:
            # many slot types have .id (linked datablock) or .name_display
            if getattr(slot, "id", None) is arm_obj:
                return (key, slot)
            if getattr(slot, "name_display", None) == arm_obj.name:
                return (key, slot)
            # sometimes slot may reference object by name somewhere:
            if getattr(slot, "name", None) == arm_obj.name:
                return (key, slot)
        except Exception:
            continue
    # not found
    return (None, None)

def frames_in_action(action):
    frames = set()
    for fc in action.fcurves:
        for kp in fc.keyframe_points:
            frames.add(int(round(kp.co.x)))
    return sorted(frames)

def collect_actions_targeting_arm(arm_obj):
    """
    Return list of actions that appear to target the given armature via slots mapping.
    Also include actions that are referenced in NLA strips or are direct active action on object.
    """
    candidates = set()

    # 1) actions that have slots pointing to the arm
    for a in bpy.data.actions:
        try:
            slot_key, slot_obj = find_slot_for_armature_on_action(a, arm_obj)
            if slot_key is not None:
                candidates.add(a)
        except Exception:
            continue

    # 2) actions used by the object's active animation_data.action or NLA strips
    for obj in bpy.data.objects:
        ad = getattr(obj, "animation_data", None)
        if not ad:
            continue
        if ad.action and obj is arm_obj:
            candidates.add(ad.action)
        for track in ad.nla_tracks:
            for strip in track.strips:
                if strip.action:
                    # heuristics: if strip.name_display or strip.name has arm name, include
                    candidates.add(strip.action)

    # 3) fallback: include any action that has pose bone fcurves (likely armature-targeted)
    if not candidates:
        for a in bpy.data.actions:
            for fc in a.fcurves:
                if fc.data_path.startswith('pose.bones["'):
                    candidates.add(a)
                    break

    return list(candidates)

def bake_action_all_bones(arm_obj, action, frames):
    """
    BAKE all bones in the action by inserting keyframes for every bone at every frame.
    This ensures we capture the complete pose including FK/IK and constraint influences.
    MUST be called BEFORE storing pose matrices.
    """
    ad = arm_obj.animation_data
    if ad is None:
        arm_obj.animation_data_create()
        ad = arm_obj.animation_data

    prev_action = ad.action
    ad.action = action
    scene = bpy.context.scene
    prev_frame = scene.frame_current

    print(f"  [BAKE] Starting bake for action '{action.name}'")
    print(f"  [BAKE] Frames to bake: {frames[:5]}...{frames[-5:] if len(frames) > 10 else ''} (total: {len(frames)})")
    print(f"  [BAKE] Total bones: {len(arm_obj.pose.bones)}")
    print(f"  [BAKE] FCurves before bake: {len(action.fcurves)}")
    
    keyframes_inserted = 0
    errors = []
    
    for f in frames:
        scene.frame_set(f)
        bpy.context.view_layer.update()
        
        # Insert keyframes for ALL bones to fully bake the pose
        for pb in arm_obj.pose.bones:
            try:
                pb.keyframe_insert(data_path="location", frame=f)
                keyframes_inserted += 1
            except Exception as e:
                errors.append(f"location {pb.name} @{f}: {str(e)[:50]}")
            
            if pb.rotation_mode == 'QUATERNION':
                try:
                    pb.keyframe_insert(data_path="rotation_quaternion", frame=f)
                    keyframes_inserted += 1
                except Exception as e:
                    errors.append(f"rot_quat {pb.name} @{f}: {str(e)[:50]}")
            else:
                try:
                    pb.keyframe_insert(data_path="rotation_euler", frame=f)
                    keyframes_inserted += 1
                except Exception as e:
                    errors.append(f"rot_euler {pb.name} @{f}: {str(e)[:50]}")
            
            try:
                pb.keyframe_insert(data_path="scale", frame=f)
                keyframes_inserted += 1
            except Exception as e:
                errors.append(f"scale {pb.name} @{f}: {str(e)[:50]}")

    scene.frame_set(prev_frame)
    ad.action = prev_action
    bpy.context.view_layer.update()
    
    print(f"  [BAKE] Complete. FCurves after bake: {len(action.fcurves)}")
    print(f"  [BAKE] Total keyframes inserted: {keyframes_inserted}")
    if errors:
        print(f"  [BAKE] Errors encountered: {len(errors)} (showing first 5)")
        for err in errors[:5]:
            print(f"    {err}")

def store_action_pose_matrices(arm_obj, action, frames):
    """
    For the given action and frames, store world-space per-pose-bone matrices.
    Returns dict frame -> {bone_name: world_matrix}
    """
    print(f"  [CAPTURE] Storing matrices for action '{action.name}'")
    print(f"  [CAPTURE] Frames: {frames[:5]}...{frames[-5:] if len(frames) > 10 else ''} (total: {len(frames)})")
    
    saved = {}
    ad = arm_obj.animation_data
    if ad is None:
        arm_obj.animation_data_create()
        ad = arm_obj.animation_data

    prev_action = ad.action
    ad.action = action
    scene = bpy.context.scene
    prev_frame = scene.frame_current

    for f in frames:
        scene.frame_set(f)
        bpy.context.view_layer.update()
        per_bone = {}
        for pb in arm_obj.pose.bones:
            # world-space matrix
            per_bone[pb.name] = (arm_obj.matrix_world @ pb.matrix).copy()
        saved[f] = per_bone
        
        if f == frames[0]:  # Log first frame as sample
            sample_bone = list(per_bone.keys())[0]
            sample_mat = per_bone[sample_bone]
            print(f"  [CAPTURE] Frame {f}, sample bone '{sample_bone}':")
            print(f"    Translation: {sample_mat.translation}")
            print(f"    Rotation (quat): {sample_mat.to_quaternion()}")

    scene.frame_set(prev_frame)
    ad.action = prev_action
    bpy.context.view_layer.update()
    print(f"  [CAPTURE] Captured {len(saved)} frames, {len(saved[frames[0]])} bones each")
    return saved

def set_edit_bones_from_world_matrices(arm_obj, pose_world_matrices, used_frame):
    """
    Convert world-space matrices -> armature space and set edit bones accordingly.
    """
    print(f"\n  [REST] Applying new rest pose from frame {used_frame}")
    print(f"  [REST] Setting {len(pose_world_matrices)} bones")
    
    world_to_obj = arm_obj.matrix_world.inverted()
    bpy.ops.object.mode_set(mode='EDIT')
    edit_bones = arm_obj.data.edit_bones
    
    bones_set = 0
    sample_bone_name = None
    sample_before = None
    sample_after = None
    
    for bname, world_m in pose_world_matrices.items():
        eb = edit_bones.get(bname)
        if not eb:
            print(f"  [REST] [WARN] edit bone '{bname}' not found; skipping.")
            continue
        
        if sample_bone_name is None:
            sample_bone_name = bname
            sample_before = (eb.head.copy(), eb.tail.copy(), eb.matrix.copy())
        
        target = world_to_obj @ world_m
        try:
            eb.matrix = target
            bones_set += 1
            
            if bname == sample_bone_name:
                sample_after = (eb.head.copy(), eb.tail.copy(), eb.matrix.copy())
        except Exception as e:
            print(f"  [REST] [WARN] could not set edit_bone.matrix for '{bname}': {e}")
    
    if sample_bone_name:
        print(f"  [REST] Sample bone '{sample_bone_name}':")
        print(f"    Head BEFORE: {sample_before[0]}")
        print(f"    Head AFTER:  {sample_after[0]}")
        print(f"    Change: {(sample_after[0] - sample_before[0]).length:.4f} units")
    
    bpy.ops.object.mode_set(mode='POSE')
    bpy.context.view_layer.update()
    print(f"  [REST] Successfully set {bones_set}/{len(pose_world_matrices)} edit bones\n")

def create_fixed_action_from_saved(arm_obj, old_action, saved_frames_map):
    """
    Create a new action whose keyframes reproduce the saved world-space poses.
    Returns new_action.
    """
    print(f"  [REBUILD] Creating fixed action from '{old_action.name}'")
    print(f"  [REBUILD] Frames to rebuild: {sorted(saved_frames_map.keys())[:5]}... (total: {len(saved_frames_map)})")
    
    scene = bpy.context.scene
    ad = arm_obj.animation_data
    if ad is None:
        arm_obj.animation_data_create()
        ad = arm_obj.animation_data

    base_name = old_action.name + FIXED_SUFFIX
    new_name = base_name
    i = 1
    while new_name in bpy.data.actions:
        new_name = f"{base_name}.{i:03d}"
        i += 1
    new_action = bpy.data.actions.new(name=new_name)
    print(f"  [REBUILD] New action name: '{new_name}'")

    prev_action = ad.action
    ad.action = new_action
    prev_frame = scene.frame_current

    world_to_obj = arm_obj.matrix_world.inverted()
    
    rebuilt_keyframes = 0
    
    for frame in sorted(saved_frames_map.keys()):
        scene.frame_set(frame)
        per_bone = saved_frames_map[frame]
        for bname, world_m in per_bone.items():
            pb = arm_obj.pose.bones.get(bname)
            if not pb:
                continue
            target_arm_m = world_to_obj @ world_m
            try:
                pb.matrix = target_arm_m
            except Exception:
                loc, rot, sc = target_arm_m.decompose()
                if pb.rotation_mode == 'QUATERNION':
                    pb.rotation_quaternion = rot
                else:
                    pb.rotation_euler = rot.to_euler(pb.rotation_mode)
                pb.location = loc
                pb.scale = sc
        bpy.context.view_layer.update()

        # Insert keyframes for each pose bone transform channels
        for pb in arm_obj.pose.bones:
            try:
                pb.keyframe_insert(data_path="location", frame=frame)
                rebuilt_keyframes += 1
            except Exception:
                pass
            if pb.rotation_mode == 'QUATERNION':
                try:
                    pb.keyframe_insert(data_path="rotation_quaternion", frame=frame)
                    rebuilt_keyframes += 1
                except Exception:
                    pass
            else:
                try:
                    pb.keyframe_insert(data_path="rotation_euler", frame=frame)
                    rebuilt_keyframes += 1
                except Exception:
                    pass
            try:
                pb.keyframe_insert(data_path="scale", frame=frame)
                rebuilt_keyframes += 1
            except Exception:
                pass

    # restore
    scene.frame_set(prev_frame)
    ad.action = prev_action
    bpy.context.view_layer.update()
    
    print(f"  [REBUILD] Keyframes in new action: {rebuilt_keyframes}")
    print(f"  [REBUILD] FCurves in new action: {len(new_action.fcurves)}")

    # Copy non-transform fcurves (best-effort)
    def is_transform_fc(fc):
        dp = fc.data_path
        if dp.startswith('pose.bones["'):
            tail = dp.split('"]', 1)[1]
            if tail.startswith('.location') or tail.startswith('.rotation_euler') or tail.startswith('.rotation_quaternion') or tail.startswith('.scale'):
                return True
        return False

    non_transform_count = 0
    for fc in old_action.fcurves:
        if not is_transform_fc(fc):
            try:
                new_fc = new_action.fcurves.new(data_path=fc.data_path, index=fc.array_index)
            except Exception:
                new_fc = None
            if new_fc:
                for kp in fc.keyframe_points:
                    new_fc.keyframe_points.insert(kp.co.x, kp.co.y, options={'FAST'})
                non_transform_count += 1
    
    if non_transform_count:
        print(f"  [REBUILD] Copied {non_transform_count} non-transform fcurves")

    return new_action

def replace_usages(old_action, new_action, arm_obj):
    """
    Replace typical usages:
     - object.animation_data.action
     - NLA strips (for any object)
    Also attempt to copy slot metadata from old_action to new_action.
    """
    # object active action and NLA strips
    replaced_count = 0
    for obj in bpy.data.objects:
        ad = getattr(obj, "animation_data", None)
        if not ad:
            continue
        if ad.action == old_action:
            ad.action = new_action
            replaced_count += 1
        for track in ad.nla_tracks:
            for strip in track.strips:
                if strip.action == old_action:
                    strip.action = new_action
                    replaced_count += 1
    
    print(f"  [REPLACE] Replaced {replaced_count} usages")

    # Attempt to copy slot metadata (best-effort)
    try:
        old_slots = list(old_action.slots.items())
    except Exception:
        old_slots = []
    for key, slot in old_slots:
        try:
            # ensure slot entry exists on new_action
            if key not in new_action.slots:
                # try to create a slot key by simple assignment if supported
                try:
                    new_action.slots[key] = None
                except Exception:
                    pass
            new_slot = new_action.slots.get(key, None)
            if new_slot is not None:
                # copy common properties if they exist
                if hasattr(slot, "name_display"):
                    try:
                        new_slot.name_display = slot.name_display
                    except Exception:
                        pass
                # try to copy id reference if present
                if hasattr(slot, "id"):
                    try:
                        new_slot.id = slot.id
                    except Exception:
                        pass
            else:
                print(f"  [REPLACE] [INFO] Could not create/copy slot '{key}' into action '{new_action.name}'. You may need to re-link slot manually.")
        except Exception as e:
            print(f"  [REPLACE] [INFO] Exception while copying slot '{key}': {e}")

def main():
    print("\n" + "="*80)
    print("REST POSE FIXER - DEBUG VERSION")
    print("="*80 + "\n")
    
    arm = get_armature(ARMATURE_NAME)
    print(f"Working on armature: {arm.name}")
    print(f"Total bones in armature: {len(arm.pose.bones)}")
    print(f"Armature world matrix: {arm.matrix_world}")

    if REFERENCE_ACTION_NAME not in bpy.data.actions:
        raise RuntimeError(f"Reference action '{REFERENCE_ACTION_NAME}' not found in bpy.data.actions.")
    ref_action = bpy.data.actions[REFERENCE_ACTION_NAME]

    # find which slot on the reference action points to this armature
    slot_key, slot_obj = find_slot_for_armature_on_action(ref_action, arm)
    if slot_key is None:
        print("[WARN] Could not find slot on reference action that references the armature by direct id/name. Proceeding anyway.")
    else:
        print(f"Reference action slot key for this armature: {slot_key} (slot.name_display='{getattr(slot_obj,'name_display',None)}')")

    # choose rest frame
    frames_ref = frames_in_action(ref_action)
    if not frames_ref:
        raise RuntimeError(f"Reference action '{ref_action.name}' has no keyframes.")
    rest_frame = REST_FRAME if REST_FRAME is not None else frames_ref[0]
    print(f"Using frame {rest_frame} from '{ref_action.name}' as rest pose source.")

    # collect actions to fix
    actions_to_fix = collect_actions_targeting_arm(arm)
    if not actions_to_fix:
        raise RuntimeError("No candidate actions found to fix for this armature.")
    print(f"\nFound {len(actions_to_fix)} actions to process:")
    for a in actions_to_fix:
        print(f"  - {a.name} ({len(frames_in_action(a))} frames)")

    # STEP 1: BAKE all actions first
    print("\n" + "="*80)
    print("STEP 1: BAKING ALL ACTIONS")
    print("="*80)
    for a in actions_to_fix:
        frames = frames_in_action(a)
        if not frames:
            print(f"Skipping action '{a.name}' (no keyframes).")
            continue
        bake_action_all_bones(arm, a, frames)

    # STEP 2: Capture pose matrices for each BAKED action
    print("\n" + "="*80)
    print("STEP 2: CAPTURING BAKED POSES")
    print("="*80)
    per_action_saved = {}
    for a in actions_to_fix:
        frames = frames_in_action(a)
        if not frames:
            continue
        per_action_saved[a.name] = store_action_pose_matrices(arm, a, frames)

    # STEP 3: Store the reference pose matrices for new rest pose
    print("\n" + "="*80)
    print("STEP 3: CAPTURING REFERENCE POSE")
    print("="*80)
    ref_saved = store_action_pose_matrices(arm, ref_action, [rest_frame])
    ref_pose_world = ref_saved[rest_frame]

    # STEP 4: Apply the new rest pose to edit bones
    print("="*80)
    print("STEP 4: APPLYING NEW REST POSE")
    print("="*80)
    set_edit_bones_from_world_matrices(arm, ref_pose_world, rest_frame)

    # STEP 5: Rebuild actions with the new rest pose
    print("="*80)
    print("STEP 5: REBUILDING ACTIONS")
    print("="*80)
    new_map = {}
    for action_name, saved_map in per_action_saved.items():
        old_action = bpy.data.actions.get(action_name)
        if not old_action:
            print(f"Skipping '{action_name}' (not found).")
            continue
        new_action = create_fixed_action_from_saved(arm, old_action, saved_map)
        new_map[old_action] = new_action
        print(f"  Created new action: '{new_action.name}'\n")

    # replace usages and attempt to copy slots
    print("="*80)
    print("STEP 6: REPLACING USAGES")
    print("="*80)
    for old_a, new_a in new_map.items():
        print(f"Replacing '{old_a.name}' with '{new_a.name}'")
        replace_usages(old_a, new_a, arm)

    print("\n" + "="*80)
    print("FINISHED")
    print("="*80)
    print(f"New actions have suffix: {FIXED_SUFFIX}")
    print("Original actions are preserved. Verify the animations; delete originals if satisfied.")
    print("\nDEBUG: Check the console output above for detailed information about the process.")

if __name__ == "__main__":
    main()
