#!/usr/bin/env python3
"""Genera Source/Generated/{dsp_wasm.h,param_table.h} a partir de un .fmx.
Uso: python3 tools/gen.py tools/SR1_Vocal_Chain_v5.fmx
"""
import sys, json, base64, gzip, os

GROUPS = [
 ("Input",      ["InGain","InDrive","PhaseInv","HpFreq","LpFreq"]),
 ("EQ",         ["EqOn","EqMudFreq","EqMudGain","EqMidFreq","EqMidGain","EqPresFreq","EqPresGain","EqAirGain"]),
 ("De-Esser",   ["DsOn","DsFreq","DsThresh","DsAmount","DsSplit"]),
 ("Compressor", ["CompOn","CompThresh","CompRatio","CompAttack","CompRelease","CompMakeup","CompHard"]),
 ("Saturation", ["SatOn","SatMode","SatDrive","SatTone","SatMix"]),
 ("Character",  ["CharOn","CharBite","CharWarmth","CharEdge"]),
 ("Space",      ["SpaceOn","Width","MicroPitch","SlapTime","SlapMix","RevType","RevMix","RevDecay","RevDamp"]),
 ("Echo",       ["EchoOn","DelayTime","DelayFeedback","DelaySync","DelayMix"]),
 ("Output",     ["LimiterOn","LimiterCeil","OutGain","Mix"]),
]
LABELS = {"InGain":"In Gain","InDrive":"In Drive","PhaseInv":"Phase Inv","HpFreq":"HP Freq","LpFreq":"LP Freq",
 "EqOn":"EQ On","EqMudFreq":"Mud Freq","EqMudGain":"Mud Gain","EqMidFreq":"Mid Freq","EqMidGain":"Mid Gain",
 "EqPresFreq":"Presence Freq","EqPresGain":"Presence Gain","EqAirGain":"Air Gain",
 "DsOn":"De-Ess On","DsFreq":"Freq","DsThresh":"Thresh","DsAmount":"Amount","DsSplit":"Split Band",
 "CompOn":"Comp On","CompThresh":"Thresh","CompRatio":"Ratio","CompAttack":"Attack","CompRelease":"Release",
 "CompMakeup":"Makeup","CompHard":"Hard Knee","SatOn":"Sat On","SatMode":"Mode","SatDrive":"Drive","SatTone":"Tone",
 "SatMix":"Mix","CharOn":"Char On","CharBite":"Bite","CharWarmth":"Warmth","CharEdge":"Edge","SpaceOn":"Space On",
 "Width":"Width","MicroPitch":"Micro Pitch","SlapTime":"Slap Time","SlapMix":"Slap Mix","RevType":"Reverb Type",
 "RevMix":"Reverb Mix","RevDecay":"Reverb Decay","RevDamp":"Reverb Damp","EchoOn":"Echo On","DelayTime":"Time",
 "DelayFeedback":"Feedback","DelaySync":"Sync","DelayMix":"Mix","LimiterOn":"Limiter On","LimiterCeil":"Ceiling",
 "OutGain":"Out Gain","Mix":"Dry/Wet"}

def fl(v):
    return repr(float(v)) + "f"

def main(path):
    d = json.load(open(path))
    wasm = gzip.decompress(base64.b64decode(d["wasm"]))
    P = json.loads(d["metadata"])["params"]
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Source", "Generated")
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "dsp_wasm.h"), "w") as f:
        f.write("// Autogenerado por tools/gen.py - no editar\n#pragma once\n#include <cstddef>\n")
        f.write("static const unsigned char kDspWasm[] = {\n")
        for i in range(0, len(wasm), 24):
            f.write("  " + ",".join(str(b) for b in wasm[i:i+24]) + ",\n")
        f.write("};\nstatic const size_t kDspWasmSize = %d;\n" % len(wasm))
    byname = {p["name"]: p for p in P}
    gof = {}
    for gi, (g, names) in enumerate(GROUPS):
        for n in names: gof[n] = gi
    assert set(gof) == set(byname), (set(byname) ^ set(gof))
    kind = {"bool": "Bool", "enum": "Choice"}
    def cstr(s): return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
    with open(os.path.join(out, "param_table.h"), "w") as f:
        f.write("// Autogenerado por tools/gen.py - no editar\n#pragma once\n")
        f.write("struct ParamInfo { int wasmId; const char* id; const char* label; int group; enum Kind{Float,Bool,Choice} kind;\n")
        f.write("  const char* curve; const char* unit; float minV, maxV, defV; const char* choices; /* separados por | */ };\n")
        f.write("static const char* const kGroupNames[] = {" + ",".join(cstr(g) for g, _ in GROUPS) + "};\n")
        f.write("static const int kNumGroups = %d;\n" % len(GROUPS))
        f.write("static const ParamInfo kParams[] = {\n")
        for p in sorted(P, key=lambda p: p["id"]):
            k = kind.get(p["type"], "Float")
            f.write("  {%d,%s,%s,%d,ParamInfo::%s,%s,%s,%s,%s,%s,%s},\n" % (
                p["id"], cstr(p["name"]), cstr(LABELS[p["name"]]), gof[p["name"]], k,
                cstr(p["curve"]), cstr("%" if p["type"] == "percent" else (p["unit"] if p["unit"] != "none" else "")),
                fl(p["min"]), fl(p["max"]), fl(p["default"]), cstr("|".join(p["choices"]))))
        f.write("};\nstatic const int kNumParams = %d;\n" % len(P))
    print("OK:", len(P), "parametros,", len(wasm), "bytes de wasm")
main(sys.argv[1])
