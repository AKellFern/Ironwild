"""
fill_land_start_map.py  -  fills the Land Start Position lookup into the ABP's class defaults.

Prerequisite (in the editor, once):
  In /Game/Animation/MotionMatching/ABP_Survival_MotionMatching add a variable
      JC_LandStartMap   type: Map  (key: Anim Sequence, value: Float)
  and compile the Animation Blueprint.

Usage (Output Log > Cmd):
  py "C:/path/fill_land_start_map.py"           -> DRY RUN: prints what it would set, changes nothing
  py "C:/path/fill_land_start_map.py" --apply   -> writes the map into the class defaults, compiles, saves

Reads  <Project>/Saved/JumpCrouch_LandStartTimes.json  (written by land_start_times.py).
Make a git checkpoint before using --apply. Only the ABP is saved.

NOTE: not run inside Unreal by the author. If the property name is not found it lists
the ABP's properties that look similar instead of guessing.
"""
import json
import os
import sys

import unreal

ABP_PATH = "/Game/Animation/MotionMatching/ABP_Survival_MotionMatching"
CLIP_DIR = "/Game/Characters/Survival_Retargeted_ShoulderFix"
VAR_NAME = "JC_LandStartMap"
APPLY = "--apply" in sys.argv


def main():
    json_path = os.path.join(unreal.Paths.project_saved_dir(), "JumpCrouch_LandStartTimes.json")
    with open(json_path) as f:
        rows = json.load(f)["clips"]
    bad = [r["clip"] for r in rows if r["source"] != "notify"]
    if bad:
        unreal.log_error("Refusing: these clips did not use a notify time: %s" % bad)
        return

    abp = unreal.load_asset(ABP_PATH)
    if abp is None:
        unreal.log_error("Could not load %s" % ABP_PATH)
        return
    gen = abp.generated_class()
    cdo = unreal.get_default_object(gen)

    prop = None
    for cand in (VAR_NAME, VAR_NAME.lower(), "jc_land_start_map"):
        try:
            cdo.get_editor_property(cand)
            prop = cand
            break
        except Exception:
            continue
    if prop is None:
        names = [n for n in dir(cdo) if "land" in n.lower() or "start" in n.lower()]
        unreal.log_error("Property %s not found on the ABP class. Create the Map variable and "
                         "compile first. Similar names: %s" % (VAR_NAME, names))
        return

    mapping = {}
    for r in rows:
        seq = unreal.load_asset("%s/%s" % (CLIP_DIR, r["clip"]))
        if seq is None:
            unreal.log_error("Missing clip %s" % r["clip"])
            return
        mapping[seq] = float(r["start_position_s"])

    unreal.log("%s %d entries into %s.%s:" % ("Setting" if APPLY else "DRY RUN - would set",
                                             len(mapping), ABP_PATH.split("/")[-1], prop))
    for seq, v in sorted(mapping.items(), key=lambda kv: kv[0].get_name()):
        unreal.log("  %-44s %.3f" % (seq.get_name(), v))

    if not APPLY:
        unreal.log("Dry run only. Re-run with --apply to write.")
        return

    cdo.set_editor_property(prop, mapping)
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(abp)
    except Exception as e:
        unreal.log_warning("compile_blueprint failed (%s) - compile manually in the editor." % e)
    ok = unreal.EditorAssetLibrary.save_loaded_asset(abp)
    back = cdo.get_editor_property(prop)
    unreal.log("Saved=%s, map now has %d entries." % (ok, len(back)))


main()
