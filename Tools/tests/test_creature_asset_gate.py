#!/usr/bin/env python3
"""Unit tests for creature asset acceptance: no Blender/Internet required."""
from __future__ import annotations

import copy
import hashlib
import json
import pathlib
import struct
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from creature_asset_gate import GateError, validate_manifest, parse_glb, mesh_metrics

LICENSE = {
    "url": "https://example.org/original-cc0-asset",
    "license": "CC0-1.0",
    "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
    "author": "Test Creator",
}


def glb(triangles: int, with_skin: bool, *, normals=True, textured=True) -> bytes:
    # Binary backed by actual embedded buffers rather than JSON-only mocks.
    vertex_count = triangles * 3
    positions = b"\x00" * vertex_count * 12
    normal_bytes = b"\x00" * vertex_count * 12
    uv_bytes = b"\x00" * vertex_count * 8
    joint_bytes = b"\x00" * vertex_count * 8
    weight_bytes = b"\x00" * vertex_count * 16
    binary=b""
    accessors=[]
    views=[]
    attrs={}
    def add(name, raw, component_type, typename, count):
        nonlocal binary
        while len(binary)%4:
            binary+=b"\x00"
        offset=len(binary)
        binary+=raw
        vi=len(views)
        views.append({"buffer":0, "byteOffset":offset,"byteLength":len(raw)})
        ai=len(accessors)
        accessors.append({"bufferView":vi, "componentType":component_type,
                          "count":count, "type":typename})
        attrs[name]=ai
    add("POSITION",positions,5126,"VEC3",vertex_count)
    if normals:
        add("NORMAL",normal_bytes,5126,"VEC3",vertex_count)
    if textured:
        add("TEXCOORD_0",uv_bytes,5126,"VEC2",vertex_count)
    if with_skin:
        add("JOINTS_0",joint_bytes,5123,"VEC4",vertex_count)
        add("WEIGHTS_0",weight_bytes,5126,"VEC4",vertex_count)
    primitive={"attributes":attrs,"mode":4}
    doc={
        "asset":{"version":"2.0"},
        "scene":0,"scenes":[{"nodes":[16]}],
        "nodes":[{"name":f"Joint_{i}"} for i in range(16)]+[
            {"name":"Creature","mesh":0,**({"skin":0} if with_skin else {})}],
        "meshes":[{"name":"CreatureGeometry","primitives":[primitive]}],
        "accessors":accessors,"bufferViews":views,
        "buffers":[{"byteLength":len(binary)}],
        "skins":[{"joints":list(range(16))}] if with_skin else [],
    }
    payload=json.dumps(doc,sort_keys=True,separators=(",",":")).encode()
    payload+=b" " * ((-len(payload))%4)
    binary+=b"\x00"*((-len(binary))%4)
    return b"".join([
        struct.pack("<4sII",b"glTF",2,12+8+len(payload)+8+len(binary)),
        struct.pack("<I4s",len(payload),b"JSON"),payload,
        struct.pack("<I4s",len(binary),b"BIN\x00"),binary,
    ])


def png_chunk(kind: bytes, payload: bytes):
    return struct.pack(">I",len(payload))+kind+payload+struct.pack(
        ">I",zlib.crc32(kind+payload)&0xffffffff)


def png(width: int, height: int, seed=1):
    # Real PNG/DEFLATE, with nontrivial test imagery, not a signature-only file.
    raw=bytearray()
    value=seed
    for y in range(height):
        raw.append(0)
        for x in range(width):
            value=(1103515245*value+12345)&0x7fffffff
            raw.extend(((value>>7)&255,(value>>11)&255,(value>>15)&255))
    compressed=zlib.compress(raw,4)
    return (b"\x89PNG\r\n\x1a\n"+
            png_chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,2,0,0,0))+
            png_chunk(b"IDAT",compressed)+png_chunk(b"IEND",b""))


def fixture(root, models=("resident-female","resident-male","tiger")):
    result={"schema_version":1,"creatures":[]}
    for name in models:
        lods={}
        for tier,tri in [("near",181),("mid",100),("far",50)]:
            content=glb(tri,tier!="far")
            filename=f"{name}/{tier}.glb"
            (root/name).mkdir(exist_ok=True)
            (root/filename).write_bytes(content)
            lods[tier]={"file":filename,"sha256":hashlib.sha256(content).hexdigest()}
        clips={"idle":"pass","walk":"pass","sleep":"pass","work":"pass"} \
            if name.startswith("resident-") else \
            {"idle":"pass","walk":"pass","run":"pass"}
        motion=f"{name}/web-motion.json"
        (root/motion).write_text(json.dumps({
            "creature_id":name,"runtime":"web-threejs",
            "tested":True,"fallback_works":True,"clips":clips,
        }))
        captures={}
        for view in ("front","side","threequarter","motion","mobile"):
            file=f"{name}/{view}.png"
            dims=(400,720) if view=="mobile" else (640,640)
            (root/file).write_bytes(png(*dims))
            captures[view]=file
        result["creatures"].append({
            "id":name,"source":dict(LICENSE),"lods":lods,
            "web_motion_report":motion,"review":captures,
        })
    return result


class CreatureGateTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=pathlib.Path(self.temp.name)
        self.manifest=fixture(self.root)

    def verify_failure(self, manifest, token):
        with self.assertRaises(GateError) as ctx:
            validate_manifest(manifest,self.root)
        self.assertIn(token,str(ctx.exception))

    def test_all_three_models_structurally_pass_but_never_claim_visual_approval(self):
        result=validate_manifest(self.manifest,self.root)
        self.assertEqual(result["status"],"structural-pass-visual-review-required")
        self.assertFalse(result["release_approved"])
        self.assertTrue(result["review_required"])
        self.assertEqual(len(result["creatures"]),3)
        self.assertEqual(result["creatures"][2]["lods"]["near"]["joints"],16)

    def test_no_unattributed_cc_by(self):
        self.manifest["creatures"][0]["source"]["license"]="CC-BY-4.0"
        self.verify_failure(self.manifest,"attribution")
        self.manifest["creatures"][0]["source"]["attribution"]="By Example, CC BY 4.0"
        self.assertEqual(len(validate_manifest(self.manifest,self.root)["creatures"]),3)

    def test_rejects_noncommercial_restricted_model(self):
        self.manifest["creatures"][0]["source"]["license"]="CC-BY-NC-4.0"
        self.verify_failure(self.manifest,"restricted")

    def test_rejects_corrupted_asset_and_changed_hash(self):
        file=self.root/"tiger/near.glb"
        file.write_bytes(file.read_bytes()[:-8])
        self.verify_failure(self.manifest,"changed since approval")

    def test_no_cheap_fake_lod_alias(self):
        self.manifest["creatures"][2]["lods"]["mid"] = copy.deepcopy(
            self.manifest["creatures"][2]["lods"]["near"])
        self.verify_failure(self.manifest,"LOD reduction")

    def test_rejects_missing_motion_and_fallback(self):
        motion=self.root/"tiger/web-motion.json"
        body=json.loads(motion.read_text())
        body["fallback_works"]=False
        motion.write_text(json.dumps(body))
        self.verify_failure(self.manifest,"fallback")
        body["fallback_works"]=True
        body["clips"]["walk"]="fail"
        motion.write_text(json.dumps(body))
        self.verify_failure(self.manifest,"clip")

    def test_rejects_missing_mobile_view(self):
        self.manifest["creatures"][0]["review"].pop("mobile")
        self.verify_failure(self.manifest,"missing visual review")

    def test_rejects_path_traversal(self):
        self.manifest["creatures"][0]["review"]["side"]="../outside.png"
        self.verify_failure(self.manifest,"Path traversal")

    def test_rejects_invalid_skinned_mesh(self):
        file=self.root/"tiger/mid.glb"
        payload=glb(100,False)
        file.write_bytes(payload)
        self.manifest["creatures"][2]["lods"]["mid"]["sha256"] = hashlib.sha256(payload).hexdigest()
        self.verify_failure(self.manifest,"Skinned near/mid")

    def test_rejects_untextured_no_normals(self):
        file=self.root/"tiger/near.glb"
        payload=glb(181,True,normals=False)
        file.write_bytes(payload)
        self.manifest["creatures"][2]["lods"]["near"]["sha256"]=hashlib.sha256(payload).hexdigest()
        self.verify_failure(self.manifest,"normals")

    def test_partial_model_must_be_explicit(self):
        self.manifest["creatures"]=self.manifest["creatures"][:1]
        self.verify_failure(self.manifest,"Release requires all three")
        self.assertEqual(len(validate_manifest(self.manifest,self.root,True)["creatures"]),1)

    def test_missing_provenance(self):
        self.manifest["creatures"][1]["source"]["url"]="http://no-license.test"
        self.verify_failure(self.manifest,"HTTPS")


if __name__=="__main__":
    unittest.main()
