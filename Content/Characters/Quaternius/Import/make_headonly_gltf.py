# Derive a head-only variant of the Quaternius Universal Base Characters bodies.
#
# Why: "Modular Character Outfits - Fantasy" covers pelvis -> neck_01 and the
# whole limbs including the fingers (verified from the outfit glTF skin
# weights), so the base body must not be drawn under the clothing. The pack's
# own Readme.txt says: "When using the clothing, only the head of the model is
# required. Using the full body will result in clipping."
#
# The imported skeletal mesh keeps ONE material slot for skin, shared by the
# head and the body, so neither a slot-level hide nor a bone hide can remove
# the body while keeping the head (hiding a bone collapses its children, and
# the head hangs below spine_01). The UV atlas is interleaved, so a UV mask is
# not separable either. Trimming triangles offline is the remaining option.
#
# This script only rewrites the triangle index list: it keeps every triangle
# whose vertices are all dominated by `head` / `neck_01`, and leaves the
# vertex buffers, materials, textures, skin and skeleton untouched. The cut
# runs around the base of the neck, which the outfit collar covers.
#
# Run with plain python3 (no engine), from the repository root:
#   python3 Content/Characters/Quaternius/Import/make_headonly_gltf.py
# Output: LL_<name>_HeadOnly.gltf/.bin next to the pack's own glTF files in the
# staging folder (outside the repository, regenerable, not committed).

import json
import os
import struct
import sys

STAGING = os.environ.get(
    "LL_QUATERNIUS_STAGING",
    "/Users/mac/Desktop/다겸이취미/Lifelens/assets_staging/Quaternius")
UBC_GLTF = os.path.join(
    STAGING, "UniversalBaseCharacters_Standard", "Universal Base Characters[Standard]",
    "Base Characters", "Godot - UE")

BODIES = ["Superhero_Male_FullBody", "Superhero_Female_FullBody"]
KEEP_BONES = {"head", "neck_01"}   # matched case-insensitively (the pack names the bone "Head")
SKIN_MATERIAL_MARK = "Superhero"   # only the skin mesh is trimmed

COMPONENT = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
NCOMP = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}


def read_accessor(gltf, buffers, index):
    a = gltf["accessors"][index]
    bv = gltf["bufferViews"][a["bufferView"]]
    offset = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
    ctype = COMPONENT[a["componentType"]]
    ncomp = NCOMP[a["type"]]
    size = struct.calcsize(ctype) * ncomp
    stride = bv.get("byteStride") or size
    data = buffers[bv["buffer"]]
    return [struct.unpack_from("<" + ctype * ncomp, data, offset + i * stride)
            for i in range(a["count"])]


def generate(name):
    src = os.path.join(UBC_GLTF, name + ".gltf")
    gltf = json.load(open(src))
    buffers = [open(os.path.join(UBC_GLTF, b["uri"]), "rb").read() for b in gltf["buffers"]]
    joints = [gltf["nodes"][j]["name"] for j in gltf["skins"][0]["joints"]]

    index_blob = b""
    trimmed = 0
    for mesh in gltf["meshes"]:
        for prim in mesh["primitives"]:
            material = gltf["materials"][prim["material"]]["name"] if "material" in prim else ""
            if SKIN_MATERIAL_MARK not in material:
                continue   # eyes and eyebrows stay untouched

            joints0 = read_accessor(gltf, buffers, prim["attributes"]["JOINTS_0"])
            weights0 = read_accessor(gltf, buffers, prim["attributes"]["WEIGHTS_0"])
            keep_vertex = []
            for jj, ww in zip(joints0, weights0):
                dominant = jj[max(range(len(ww)), key=lambda k: ww[k])]
                keep_vertex.append(joints[dominant].lower() in KEEP_BONES)

            indices = [i[0] for i in read_accessor(gltf, buffers, prim["indices"])]
            kept = []
            for t in range(0, len(indices) - 2, 3):
                tri = indices[t:t + 3]
                if all(keep_vertex[v] for v in tri):
                    kept.extend(tri)
            if not kept:
                raise RuntimeError("%s: no head triangles kept" % name)

            accessor = gltf["accessors"][prim["indices"]]
            ctype = COMPONENT[accessor["componentType"]]
            offset = len(index_blob)
            index_blob += struct.pack("<" + ctype * len(kept), *kept)

            gltf["bufferViews"].append({
                "buffer": len(gltf["buffers"]),
                "byteOffset": offset,
                "byteLength": len(index_blob) - offset,
                "target": 34963,
            })
            gltf["accessors"].append({
                "bufferView": len(gltf["bufferViews"]) - 1,
                "componentType": accessor["componentType"],
                "count": len(kept),
                "type": "SCALAR",
                "min": [min(kept)],
                "max": [max(kept)],
            })
            prim["indices"] = len(gltf["accessors"]) - 1
            print("  %s / %s: %d -> %d triangles" % (name, material, len(indices) // 3, len(kept) // 3))
            trimmed += 1

    if trimmed != 1:
        raise RuntimeError("%s: expected exactly one skin primitive, found %d" % (name, trimmed))

    out_name = "LL_" + name.replace("_FullBody", "") + "_HeadOnly"
    open(os.path.join(UBC_GLTF, out_name + ".bin"), "wb").write(index_blob)
    gltf["buffers"].append({"uri": out_name + ".bin", "byteLength": len(index_blob)})
    json.dump(gltf, open(os.path.join(UBC_GLTF, out_name + ".gltf"), "w"), separators=(",", ":"))
    print("  wrote %s.gltf (+ .bin, %d bytes)" % (out_name, len(index_blob)))
    return out_name


def main():
    if not os.path.isdir(UBC_GLTF):
        print("missing staging folder: %s" % UBC_GLTF, file=sys.stderr)
        return 1
    for name in BODIES:
        generate(name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
