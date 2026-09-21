"""
land_start_times.py  -  READ-ONLY helper for the Land-clip "hang" fix.

What it does
------------
For every M_Neutral_Jump_F_Land_* clip in the corrected retarget folder it finds
the authored ground-contact time (the "FoleyEvent: Land" notify; falls back to the
first rise of the contact_l / contact_r curve; falls back to 0.53 s Light / 1.03 s
Heavy) and prints the Start Position to feed the Land sequence player:

    start_position = contact_time - CROSSFADE      (CROSSFADE = 0.12 s)

It then writes the table to  <Project>/Saved/JumpCrouch_LandStartTimes.json.
It never modifies, saves or creates any asset.

How to run (Unreal Editor 5.8, Python Editor Script Plugin enabled)
-------------------------------------------------------------------
  Output Log > Cmd box:   py "C:/path/to/land_start_times.py"
  or  Tools > Execute Python Script...

NOTE: this script has NOT been run inside Unreal (no editor available where it was
written). It tries several API spellings and reports which one worked. If a value
comes from a fallback the "source" column says so.
"""
import json
import os

import unreal

CLIP_DIR = "/Game/Characters/Survival_Retargeted_ShoulderFix"
NAME_PREFIX = "M_Neutral_Jump_F_Land_"
CROSSFADE = 0.12  # Transition duration on every SM_Airborne transition (read from the ABP)
FALLBACK_CONTACT = {"Light": 0.53, "Heavy": 1.03}


# --------------------------------------------------------------------------- helpers
def _lib():
    # UE 5.x exposes UAnimationBlueprintLibrary as unreal.AnimationLibrary
    for name in ("AnimationLibrary", "AnimationBlueprintLibrary"):
        lib = getattr(unreal, name, None)
        if lib is not None:
            return lib
    return None


def _notify_time(lib, ev):
    """Absolute trigger time (seconds) of an AnimNotifyEvent, or None."""
    if lib is not None:
        for fn in ("get_anim_notify_event_trigger_time", "get_notify_event_trigger_time"):
            f = getattr(lib, fn, None)
            if f:
                try:
                    return float(f(ev))
                except Exception:
                    pass
    for fn in ("get_time", "get_trigger_time"):
        f = getattr(ev, fn, None)
        if f:
            try:
                return float(f())
            except Exception:
                pass
    for prop in ("display_time", "trigger_time_offset"):
        try:
            return float(ev.get_editor_property(prop))
        except Exception:
            pass
    return None


def _ev_text(ev):
    parts = []
    for prop in ("notify_name",):
        try:
            parts.append(str(ev.get_editor_property(prop)))
        except Exception:
            pass
    for prop in ("notify", "notify_state_class"):
        try:
            obj = ev.get_editor_property(prop)
            if obj:
                parts.append(obj.get_name())
                parts.append(str(obj))
        except Exception:
            pass
    return " ".join(parts).lower()


def find_land_notify_time(lib, anim):
    try:
        events = list(lib.get_animation_notify_events(anim)) if lib else list(anim.get_editor_property("notifies"))
    except Exception:
        try:
            events = list(anim.get_editor_property("notifies"))
        except Exception:
            return None
    times = []
    for ev in events:
        text = _ev_text(ev)
        if "land" in text:
            t = _notify_time(lib, ev)
            if t is not None:
                times.append(t)
    return min(times) if times else None


def find_contact_curve_time(lib, anim):
    if lib is None:
        return None
    try:
        names = [str(n) for n in lib.get_animation_curve_names(anim, unreal.RawCurveTrackTypes.RCT_FLOAT)]
    except Exception:
        return None
    best = None
    for cname in names:
        if not cname.lower().startswith("contact_"):
            continue
        try:
            res = lib.get_float_keys(anim, cname)
            times, values = res[0], res[1]
        except Exception:
            continue
        if not values or len(values) < 2:
            continue
        lo, hi = min(values), max(values)
        if hi - lo < 1e-3:
            continue
        thresh = lo + 0.5 * (hi - lo)
        for t, v in zip(times, values):
            if v >= thresh:
                best = t if best is None else min(best, t)
                break
    return best


def clip_length(lib, anim):
    for fn in ("get_sequence_length", "get_play_length"):
        f = getattr(lib, fn, None) if lib else None
        if f:
            try:
                return float(f(anim))
            except Exception:
                pass
    for prop in ("sequence_length", "play_length"):
        try:
            return float(anim.get_editor_property(prop))
        except Exception:
            pass
    try:
        return float(anim.get_play_length())
    except Exception:
        return None


# --------------------------------------------------------------------------- main
def main():
    lib = _lib()
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = reg.get_assets_by_path(CLIP_DIR, recursive=False)
    rows = []
    for a in assets:
        name = str(a.asset_name)
        if not name.startswith(NAME_PREFIX):
            continue
        anim = unreal.load_asset(a.get_asset().get_path_name()) if hasattr(a, "get_asset") else None
        if anim is None:
            continue
        kind = "Heavy" if "_Heavy_" in name else "Light"
        contact = find_land_notify_time(lib, anim)
        source = "notify"
        if contact is None:
            contact = find_contact_curve_time(lib, anim)
            source = "contact curve"
        if contact is None:
            contact = FALLBACK_CONTACT[kind]
            source = "FALLBACK (%s default)" % kind
        start = max(0.0, contact - CROSSFADE)
        rows.append({
            "clip": name,
            "kind": kind,
            "length_s": clip_length(lib, anim),
            "contact_s": round(contact, 3),
            "start_position_s": round(start, 3),
            "source": source,
        })

    rows.sort(key=lambda r: r["clip"])
    if not rows:
        unreal.log_warning("No %s* clips found under %s" % (NAME_PREFIX, CLIP_DIR))
        return

    unreal.log("Land clip start positions (contact - %.2f s):" % CROSSFADE)
    for r in rows:
        unreal.log("  %-42s contact %.3f  ->  Start Position %.3f   [%s]"
                   % (r["clip"].replace(NAME_PREFIX, ""), r["contact_s"], r["start_position_s"], r["source"]))

    out = os.path.join(unreal.Paths.project_saved_dir(), "JumpCrouch_LandStartTimes.json")
    with open(out, "w") as f:
        json.dump({"crossfade_s": CROSSFADE, "clips": rows}, f, indent=2)
    unreal.log("Wrote %s" % out)


main()
