"""Experimental source-preserving graph cut of the fused display base.

Only the imported source is edited. This creates review artifacts, not accepted
runtime models. Original UVs remain per face corner; no voxel/remesh is used.
"""
import json
import hashlib
import datetime as dt
from pathlib import Path
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import maximum_flow

FOOT_ANCHORS = [(-.29,-1.30,.67),(.05,-.71,.51),(.17,1.08,.32),(-.25,.21,.45)]


def main():
    bpy.context.preferences.filepaths.save_version=0
    out = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(out / 'source.glb'))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if len(meshes) != 1:
        raise ValueError('This bounded extraction is only for the reviewed TopoForge source')
    obj = meshes[0]
    obj.matrix_world = Matrix.Rotation(np.pi/2, 4, 'Z') @ obj.matrix_world
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    # Weld duplicate geometric positions only. UV data is per face corner and
    # preserved; the glTF exporter can split UV seams again when exporting.
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-5)
    bm.verts.ensure_lookup_table()
    bm.verts.index_update()
    count = len(bm.verts)
    source, sink = count, count + 1
    rows, columns, weights = [], [], []
    def edge(a, b, weight):
        rows.append(a); columns.append(b); weights.append(weight)
    for e in bm.edges:
        a, b = e.verts
        # Narrow curved attachment regions cost less to cut than broad flat
        # anatomy. Terminal seeds retain the original paws and upper animal.
        angle = e.calc_face_angle(0)
        capacity = max(1, int(e.calc_length()*100000 / (1+abs(angle)*4)))
        edge(a.index, b.index, capacity)
        edge(b.index, a.index, capacity)
    foot_anchors = FOOT_ANCHORS
    body_seeds, base_seeds = 0, 0
    for v in bm.verts:
        x, y, z = v.co
        # Seeds lie on the upper paw coat, not in the supporting wood below it.
        # These four centers were calibrated from orthogonal source renders.
        paw = any(((x-a)/.14)**2+((y-b)/.18)**2+((z-c-.07)/.08)**2 < 1
                  for a,b,c in foot_anchors)
        if z > .98 or paw:
            edge(source, v.index, 100000000)
            body_seeds += 1
        elif z < .12 or (z < .65 and abs(x) > .50):
            edge(v.index, sink, 100000000)
            base_seeds += 1
    if body_seeds < 1000 or base_seeds < 100:
        raise ValueError('Source anatomy or base bounds changed; stop extraction')
    graph = coo_matrix((np.array(weights,dtype=np.int64),(rows,columns)),
                       shape=(count+2,count+2)).tocsr()
    result = maximum_flow(graph, source, sink)
    residual = graph-result.flow
    reached = {source}
    pending = [source]
    while pending:
        current = pending.pop()
        start, end = residual.indptr[current:current+2]
        for neighbor, capacity in zip(residual.indices[start:end],residual.data[start:end]):
            neighbor = int(neighbor)
            if capacity > 0 and neighbor not in reached:
                reached.add(neighbor)
                pending.append(neighbor)
    keep = {i for i in reached if i<count}
    deleted_faces = [face for face in bm.faces if any(v.index not in keep for v in face.verts)]
    before_faces = len(bm.faces)
    bmesh.ops.delete(bm,geom=deleted_faces,context='FACES')
    # Only close the small cut boundaries at the original paw soles. Large
    # holes remain explicit failures rather than being filled across a limb.
    cut_edges = [edge for edge in bm.edges if edge.is_boundary]
    closed = bmesh.ops.holes_fill(bm,edges=cut_edges,sides=80).get('faces',[])
    remaining = sum(edge.is_boundary for edge in bm.edges)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    obj.name = 'LifeLensTigerSourceExtractionCandidate'
    obj['runtime_approved'] = False
    obj['extraction_status'] = 'VISUAL_REVIEW_REQUIRED'
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'body-extraction.blend'))
    bpy.ops.export_scene.gltf(filepath=str(out/'body-extraction.glb'),export_format='GLB',use_selection=True)
    report = {'status':'EXPERIMENTAL_NOT_RUNTIME','source_vertices_welded':count,
              'retained_vertices_before_cut':len(keep),'faces_before':before_faces,
              'faces_removed':len(deleted_faces),'sole_cap_faces':len(closed),
              'remaining_boundary_edges':remaining,'body_seeds':body_seeds,'base_seeds':base_seeds,
              'foot_anchors':foot_anchors,'visual_acceptance':'NOT_REVIEWED',
              'web_observer_validation':'NOT_RUN','merge_allowed':False}
    report['created_at'] = dt.datetime.now(dt.timezone.utc).isoformat()
    report['files_sha256'] = {name:hashlib.sha256((out/name).read_bytes()).hexdigest()
                             for name in ['body-extraction.glb','body-extraction.blend']}
    (out/'body-extraction.json').write_text(json.dumps(report,indent=2)+'\n')
    with (out/'CREDITS.md').open('a') as credits:
        credits.write('\nExperimental derivative changes: seam-position welding with corner UV preservation, '
                      'graph cut of the display base, and limited boundary caps. The first extraction '
                      'failed visual review (lost hind-limb surface and retained wood); it is not '
                      'an accepted runtime asset. Original source.glb remains unmodified.\n')
    print('BODY_EXTRACTION',json.dumps(report))


if __name__ == '__main__':
    main()
