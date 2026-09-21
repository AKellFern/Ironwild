"""
land_start_times.py (v2)  -  READ-ONLY helper for the Land-clip "hang" fix.

For every M_Neutral_Jump_F_Land_* clip in the corrected retarget folder it finds the
authored ground-contact time and prints the Start Position to feed the Land sequence
player:   start_position = contact_time - CROSSFADE   (CROSSFADE = 0.12 s)

Contact time is taken, in order, from:
  1. the "FoleyEvent: Land" notify (matched on the notify's own name/properties or on
     the notify track called "Land" - NOT on the clip's asset path),
  2. the first rise of the contact_l / contact_r curve,
  3. a default (Light 0.53 s, Heavy 1.03 s)   <- flagged "FALLBACK" in the output.
Anything earlier than 0.15 s is treated as spurious and skipped.

It never modifies, saves or creates an asset. It writes
<Project>/Saved/JumpCrouch_LandStartTimes.json and, for any clip that had to use a
fallback, dumps every notify on that clip to the Output Log so the matching can be fixed.

Run:  Output Log > Cmd:  py "C:/path/to/land_start_times.py"
v1 bug: it matched "land" against the notify object's full path, which contains the
clip name (..._Land_...), so every notify matched and the earliest one (time 0.0) won.
"""
import json
import os

import unreal

CLIP_DIR = "/Game/Characters/Survival_Retargeted_ShoulderFix"
NAME_PREFIX = "M_Neutral_Jump_F_Land_"
CROSSFADE = 0.12
MIN_VALID_CONTACT = 0.15
FALLBACK_CONTACT = {"Light": 0.53, "Heavy": 1.03}


def _lib():
    for name in ("AnimationLibrary", "AnimationBlueprintLibrary"):
        lib = getattr(unreal, name, None)
        if lib is not None:
            return lib
    return None


def _prop(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def _notify_time(lib, ev):
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
    for p in ("display_time", "trigger_time_offset"):
        v = _prop(ev, p)
        if v is not None:
            try:
                return float(v)
            except Exception:
                pass
    return None


def _primitive_props_text(obj):
    """Text of the notify object's own (non-object) properties, e.g. Event = Land."""
    parts = []
    for name in dir(obj):
        if name.startswith("_"):
            continue
        try:
            v = getattr(obj, name)
        except Exception:
            continue
        if callable(v) or isinstance(v, unreal.Object):
            continue
        try:
            parts.append("%s=%s" % (name, v))
        except Exception:
            pass
    return " ".join(parts)


def _events(lib, anim):
    try:
        return list(lib.get_animation_notify_events(anim))
    except Exception:
        return list(_prop(anim, "notifies", []) or [])


def _track_names(lib, anim):
    try:
        return [str(n) for n in lib.get_animation_notify_track_names(anim)]
    except Exception:
        return []


def describe(lib, ev, tracks):
    notify = _prop(ev, "notify")
    state = _prop(ev, "notify_state_class")
    tidx = _prop(ev, "track_index")
    track = tracks[tidx] if isinstance(tidx, int) and 0 <= tidx < len(tracks) else ""
    cls = ""
    for o in (notify, state):
        if o:
            try:
                cls = o.get_class().get_name()
            except Exception:
                cls = str(type(o).__name__)
            break
    props = _primitive_props_text(notify) if notify else ""
    return {
        "name": str(_prop(ev, "notify_name", "")),
        "cls": cls,
        "track": track,
        "props": props,
        "time": _notify_time(lib, ev),
    }


def find_land_notify_time(lib, anim):
    tracks = _track_names(lib, anim)
    descs = [describe(lib, ev, tracks) for ev in _events(lib, anim)]

    def valid(d):
        return d["time"] is not None and d["time"] >= MIN_VALID_CONTACT

    # pass 1: notify's own name / properties mention "land" (path deliberately excluded)
    hits = [d for d in descs if valid(d) and "land" in (d["name"] + " " + d["props"]).lower()]
    # pass 2: the notify sits on a track called "Land"
    if not hits:
        hits = [d for d in descs if valid(d) and d["track"].strip().lower() == "land"]
    if hits:
        foley = [d for d in hits if "foley" in (d["name"] + d["cls"] + d["props"]).lower()]
        pick = min((foley or hits), key=lambda d: d["time"])
        return pick["time"], descs
    return None, descs


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
            if t >= MIN_VALID_CONTACT and v >= thresh:
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
    for p in ("sequence_length", "play_length"):
        v = _prop(anim, p)
        if v is not None:
            try:
                return float(v)
            except Exception:
                pass
    return None


def main():
    lib = _lib()
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    rows = []
    for a in reg.get_assets_by_path(CLIP_DIR, recursive=False):
        name = str(a.asset_name)
        if not name.startswith(NAME_PREFIX):
            continue
        anim = a.get_asset()
        if anim is None:
            continue
        kind = "Heavy" if "_Heavy_" in name else "Light"
        contact, descs = find_land_notify_time(lib, anim)
        source = "notify"
        if contact is None:
            contact = find_contact_curve_time(lib, anim)
            source = "contact curve"
        if contact is None:
            contact = FALLBACK_CONTACT[kind]
            source = "FALLBACK (%s default)" % kind
        if source != "notify":
            unreal.log_warning("%s: no usable Land notify; events on this clip:" % name)
            for d in descs:
                unreal.log_warning("    t=%s track=%r cls=%s name=%r props=%s"
                                   % (d["time"], d["track"], d["cls"], d["name"], d["props"][:160]))
        rows.append({
            "clip": name,
            "kind": kind,
            "length_s": clip_length(lib, anim),
            "contact_s": round(contact, 3),
            "start_position_s": round(max(0.0, contact - CROSSFADE), 3),
            "source": source,
        })

    rows.sort(key=lambda r: r["clip"])
    if not rows:
        unreal.log_warning("No %s* clips found under %s" % (NAME_PREFIX, CLIP_DIR))
        return

    unreal.log("Land clip start positions (contact - %.2f s):" % CROSSFADE)
    for r in rows:
        unreal.log("  %-40s contact %.3f  ->  Start Position %.3f   [%s]"
                   % (r["clip"].replace(NAME_PREFIX, ""), r["contact_s"], r["start_position_s"], r["source"]))

    out = os.path.join(unreal.Paths.project_saved_dir(), "JumpCrouch_LandStartTimes.json")
    with open(out, "w") as f:
        json.dump({"crossfade_s": CROSSFADE, "clips": rows}, f, indent=2)
    unreal.log("Wrote %s" % out)


main()
