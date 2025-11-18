# fix_rest_pose_from_idle_slot_horse_biped.py
# Blender 4.5 — specialized for:
#   armature object name: 'Horse Biped'
#   reference action: bpy.data.actions['idle']
#
# WARNING: BACKUP your .blend before running. This edits edit bones and creates new actions.

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

def store_action_pose_matrices(arm_obj, action, frames, prepend_rest_pose=None):
    """
    For the given action and frames, set arm_obj.animation_data.action = action and store world-space per-pose-bone matrices.
    If prepend_rest_pose is provided (dict of bone_name->world_matrix), it will be inserted at frame 0.
    Returns dict frame -> {bone_name: world_matrix}
    """
    saved = {}
    ad = arm_obj.animation_data
    if ad is None:
        arm_obj.animation_data_create()
        ad = arm_obj.animation_data

    prev_action = ad.action
    ad.action = action
    scene = bpy.context.scene
    prev_frame = scene.frame_current

    # Add frame 0 with rest pose if provided
    if prepend_rest_pose is not None:
        saved[0] = prepend_rest_pose.copy()

    for f in frames:
        scene.frame_set(f)
        bpy.context.view_layer.update()
        per_bone = {}
        for pb in arm_obj.pose.bones:
            # world-space matrix
            per_bone[pb.name] = (arm_obj.matrix_world @ pb.matrix).copy()
        saved[f] = per_bone

    scene.frame_set(prev_frame)
    ad.action = prev_action
    bpy.context.view_layer.update()
    return saved

def set_edit_bones_from_world_matrices(arm_obj, pose_world_matrices, used_frame):
    """
    Convert world-space matrices -> armature space and set edit bones accordingly.
    """
    world_to_obj = arm_obj.matrix_world.inverted()
    bpy.ops.object.mode_set(mode='EDIT')
    edit_bones = arm_obj.data.edit_bones
    for bname, world_m in pose_world_matrices.items():
        eb = edit_bones.get(bname)
        if not eb:
            print(f"[WARN] edit bone '{bname}' not found; skipping.")
            continue
        target = world_to_obj @ world_m
        try:
            eb.matrix = target
        except Exception as e:
            print(f"[WARN] could not set edit_bone.matrix for '{bname}': {e}")
    bpy.ops.object.mode_set(mode='POSE')
    bpy.context.view_layer.update()
    print(f"Set edit bones from frame {used_frame}.")

def create_fixed_action_from_saved(arm_obj, old_action, saved_frames_map):
    """
    Create a new action whose keyframes reproduce the saved world-space poses.
    Returns new_action.
    """
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

    prev_action = ad.action
    ad.action = new_action
    prev_frame = scene.frame_current

    world_to_obj = arm_obj.matrix_world.inverted()

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
            except Exception:
                pass
            if pb.rotation_mode == 'QUATERNION':
                try:
                    pb.keyframe_insert(data_path="rotation_quaternion", frame=frame)
                except Exception:
                    pass
            else:
                try:
                    pb.keyframe_insert(data_path="rotation_euler", frame=frame)
                except Exception:
                    pass
            try:
                pb.keyframe_insert(data_path="scale", frame=frame)
            except Exception:
                pass

    # restore
    scene.frame_set(prev_frame)
    ad.action = prev_action
    bpy.context.view_layer.update()

    # Copy non-transform fcurves (best-effort)
    def is_transform_fc(fc):
        dp = fc.data_path
        if dp.startswith('pose.bones["'):
            tail = dp.split('"]', 1)[1]
            if tail.startswith('.location') or tail.startswith('.rotation_euler') or tail.startswith('.rotation_quaternion') or tail.startswith('.scale'):
                return True
        return False

    for fc in old_action.fcurves:
        if not is_transform_fc(fc):
            try:
                new_fc = new_action.fcurves.new(data_path=fc.data_path, index=fc.array_index)
            except Exception:
                new_fc = None
            if new_fc:
                for kp in fc.keyframe_points:
                    new_fc.keyframe_points.insert(kp.co.x, kp.co.y, options={'FAST'})

    return new_action

def replace_usages(old_action, new_action, arm_obj):
    """
    Replace typical usages:
     - object.animation_data.action
     - NLA strips (for any object)
    Also attempt to copy slot metadata from old_action to new_action.
    """
    # object active action and NLA strips
    for obj in bpy.data.objects:
        ad = getattr(obj, "animation_data", None)
        if not ad:
            continue
        if ad.action == old_action:
            ad.action = new_action
        for track in ad.nla_tracks:
            for strip in track.strips:
                if strip.action == old_action:
                    strip.action = new_action

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
                print(f"[INFO] Could not create/copy slot '{key}' into action '{new_action.name}'. You may need to re-link slot manually.")
        except Exception as e:
            print(f"[INFO] Exception while copying slot '{key}': {e}")

def main():
    arm = get_armature(ARMATURE_NAME)
    print(f"Working on armature: {arm.name}")

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
    print(f"Found {len(actions_to_fix)} actions to process.")

    # store pose matrices for each action
    per_action_saved = {}
    for a in actions_to_fix:
        frames = frames_in_action(a)
        if not frames:
            print(f"Skipping action '{a.name}' (no keyframes).")
            continue
        print(f"Capturing {len(frames)} frames for action '{a.name}'...")
        per_action_saved[a.name] = store_action_pose_matrices(arm, a, frames)

    # store the reference pose matrices for rest pose
    ref_saved = store_action_pose_matrices(arm, ref_action, [rest_frame])
    ref_pose_world = ref_saved[rest_frame]

    # set edit bones to the reference pose
    print("Applying reference pose to edit bones...")
    set_edit_bones_from_world_matrices(arm, ref_pose_world, rest_frame)

    # rebuild actions
    new_map = {}
    for action_name, saved_map in per_action_saved.items():
        old_action = bpy.data.actions.get(action_name)
        if not old_action:
            print(f"Skipping '{action_name}' (not found).")
            continue
        print(f"Rebuilding action '{action_name}' ({len(saved_map)} frames)...")
        new_action = create_fixed_action_from_saved(arm, old_action, saved_map)
        new_map[old_action] = new_action
        print(f" Created new action: '{new_action.name}'")

    # replace usages and attempt to copy slots
    print("Replacing usages and attempting to copy slot metadata...")
    for old_a, new_a in new_map.items():
        replace_usages(old_a, new_a, arm)

    print("Finished. New actions have suffix:", FIXED_SUFFIX)
    print("Original actions are preserved. Verify the animations; delete originals if satisfied.")

if __name__ == "__main__":
    main()
