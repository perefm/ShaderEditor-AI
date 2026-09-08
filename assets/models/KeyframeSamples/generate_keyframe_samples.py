"""Generates the ShaderEditor keyframe-animation sample models.

Both outputs are self-contained glTF 2.0 files (embedded base64 buffers) so they can be
committed as readable text and regenerated deterministically:

* ``KeyframeCube.gltf``   - two cubes animated by node keyframes (translation/rotation/scale),
                            exercising object animation with the free camera.
* ``KeyframeCamera.gltf`` - a static cube plus a *keyframed camera node*, exercising the Render
                            panel camera selector, animated ``view``/``projection`` and the
                            per-frame ``uCameraPos`` update.

Run with: python assets/models/KeyframeSamples/generate_keyframe_samples.py
"""

from __future__ import annotations

import base64
import json
import math
import pathlib
import struct

OUTPUT_DIR = pathlib.Path(__file__).parent

# Unit cube: 24 vertices (4 per face) so each face gets a correct flat normal.
_FACES = [
    ((0, 0, 1), [(-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]),
    ((0, 0, -1), [(1, -1, -1), (-1, -1, -1), (-1, 1, -1), (1, 1, -1)]),
    ((1, 0, 0), [(1, -1, 1), (1, -1, -1), (1, 1, -1), (1, 1, 1)]),
    ((-1, 0, 0), [(-1, -1, -1), (-1, -1, 1), (-1, 1, 1), (-1, 1, -1)]),
    ((0, 1, 0), [(-1, 1, 1), (1, 1, 1), (1, 1, -1), (-1, 1, -1)]),
    ((0, -1, 0), [(-1, -1, -1), (1, -1, -1), (1, -1, 1), (-1, -1, 1)]),
]
_UVS = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]


def cube_geometry(half_extent: float = 0.5):
    positions, normals, uvs, indices = [], [], [], []
    for normal, corners in _FACES:
        base = len(positions)
        for corner, uv in zip(corners, _UVS):
            positions.append(tuple(c * half_extent for c in corner))
            normals.append(tuple(float(c) for c in normal))
            uvs.append(uv)
        indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return positions, normals, uvs, indices


class BufferBuilder:
    """Accumulates padded binary chunks and emits glTF bufferViews/accessors."""

    def __init__(self) -> None:
        self.data = bytearray()
        self.buffer_views: list[dict] = []
        self.accessors: list[dict] = []

    def _add_view(self, payload: bytes, target=None) -> int:
        while len(self.data) % 4:
            self.data.append(0)
        view = {"buffer": 0, "byteOffset": len(self.data), "byteLength": len(payload)}
        if target is not None:
            view["target"] = target
        self.data += payload
        self.buffer_views.append(view)
        return len(self.buffer_views) - 1

    def add_floats(self, values, type_name: str, target=None) -> int:
        tuples = [v if isinstance(v, (tuple, list)) else (v,) for v in values]
        flat = [c for value in tuples for c in value]
        view = self._add_view(struct.pack(f"<{len(flat)}f", *flat), target)
        columns = list(zip(*tuples))
        self.accessors.append(
            {
                "bufferView": view,
                "componentType": 5126,  # FLOAT
                "count": len(values),
                "type": type_name,
                "min": [min(column) for column in columns],
                "max": [max(column) for column in columns],
            }
        )
        return len(self.accessors) - 1

    def add_indices(self, indices) -> int:
        view = self._add_view(struct.pack(f"<{len(indices)}H", *indices), 34963)
        self.accessors.append(
            {
                "bufferView": view,
                "componentType": 5123,  # UNSIGNED_SHORT
                "count": len(indices),
                "type": "SCALAR",
                "min": [min(indices)],
                "max": [max(indices)],
            }
        )
        return len(self.accessors) - 1

    def uri(self) -> str:
        return "data:application/octet-stream;base64," + base64.b64encode(bytes(self.data)).decode("ascii")


def quaternion_y(angle_radians: float):
    return (0.0, math.sin(angle_radians * 0.5), 0.0, math.cos(angle_radians * 0.5))


def write(document: dict, filename: str) -> None:
    path = OUTPUT_DIR / filename
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {path}")


def cube_mesh(position_accessor, normal_accessor, uv_accessor, index_accessor, name="Cube"):
    return {
        "name": name,
        "primitives": [
            {
                "attributes": {
                    "POSITION": position_accessor,
                    "NORMAL": normal_accessor,
                    "TEXCOORD_0": uv_accessor,
                },
                "indices": index_accessor,
                "material": 0,
                "mode": 4,
            }
        ],
    }


def build_animated_objects() -> None:
    positions, normals, uvs, indices = cube_geometry()
    builder = BufferBuilder()
    position_accessor = builder.add_floats(positions, "VEC3", 34962)
    normal_accessor = builder.add_floats(normals, "VEC3", 34962)
    uv_accessor = builder.add_floats(uvs, "VEC2", 34962)
    index_accessor = builder.add_indices(indices)

    # Five keyframes over 4 s; the last repeats the first so looping is seamless.
    times = [0.0, 1.0, 2.0, 3.0, 4.0]
    time_accessor = builder.add_floats(times, "SCALAR")
    orbit_translations = [
        (2.0, 0.0, 0.0),
        (0.0, 0.0, 2.0),
        (-2.0, 0.0, 0.0),
        (0.0, 0.0, -2.0),
        (2.0, 0.0, 0.0),
    ]
    orbit_accessor = builder.add_floats(orbit_translations, "VEC3")
    spin_accessor = builder.add_floats([quaternion_y(math.tau * i / 4.0) for i in range(5)], "VEC4")
    pulse_accessor = builder.add_floats(
        [(1.0, 1.0, 1.0), (1.4, 0.6, 1.4), (1.0, 1.0, 1.0), (0.6, 1.4, 0.6), (1.0, 1.0, 1.0)], "VEC3"
    )

    document = {
        "asset": {"version": "2.0", "generator": "ShaderEditor keyframe sample generator"},
        "scene": 0,
        "scenes": [{"name": "KeyframeCubes", "nodes": [0, 1]}],
        "nodes": [
            {"name": "OrbitingCube", "mesh": 0, "translation": list(orbit_translations[0])},
            {"name": "PulsingCube", "mesh": 0, "translation": [0.0, 0.0, 0.0]},
        ],
        # A single mesh referenced by two nodes. Assimp's aiProcess_FindInstances also collapses
        # byte-identical meshes into this shape, so both cubes only appear if the renderer draws
        # one instance per referencing node rather than one draw per unique mesh.
        "meshes": [
            cube_mesh(position_accessor, normal_accessor, uv_accessor, index_accessor, "CubeMesh"),
        ],
        "materials": [
            {
                "name": "CubeMaterial",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.85, 0.55, 0.25, 1.0],
                    "metallicFactor": 0.1,
                    "roughnessFactor": 0.6,
                },
            }
        ],
        "animations": [
            {
                "name": "OrbitAndSpin",
                "samplers": [
                    {"input": time_accessor, "output": orbit_accessor, "interpolation": "LINEAR"},
                    {"input": time_accessor, "output": spin_accessor, "interpolation": "LINEAR"},
                ],
                "channels": [
                    {"sampler": 0, "target": {"node": 0, "path": "translation"}},
                    {"sampler": 1, "target": {"node": 0, "path": "rotation"}},
                ],
            },
            {
                "name": "Pulse",
                "samplers": [
                    {"input": time_accessor, "output": pulse_accessor, "interpolation": "LINEAR"}
                ],
                "channels": [{"sampler": 0, "target": {"node": 1, "path": "scale"}}],
            },
        ],
        "buffers": [{"byteLength": len(builder.data), "uri": builder.uri()}],
        "bufferViews": builder.buffer_views,
        "accessors": builder.accessors,
    }
    write(document, "KeyframeCube.gltf")


def build_animated_camera() -> None:
    positions, normals, uvs, indices = cube_geometry()
    builder = BufferBuilder()
    position_accessor = builder.add_floats(positions, "VEC3", 34962)
    normal_accessor = builder.add_floats(normals, "VEC3", 34962)
    uv_accessor = builder.add_floats(uvs, "VEC2", 34962)
    index_accessor = builder.add_indices(indices)

    # A full 6 s orbit around the origin at a fixed height. The camera node's own keyframes drive
    # view/projection and uCameraPos once this camera is picked in the Render panel.
    steps = 6
    times = [float(step) for step in range(steps + 1)]
    radius = 4.0
    orbit, rotations = [], []
    for step in range(steps + 1):
        angle = math.tau * step / steps
        orbit.append((radius * math.sin(angle), 1.5, radius * math.cos(angle)))
        # glTF cameras look down -Z, so yawing by the orbit angle keeps the cube centred.
        rotations.append(quaternion_y(angle))
    time_accessor = builder.add_floats(times, "SCALAR")
    orbit_accessor = builder.add_floats(orbit, "VEC3")
    rotation_accessor = builder.add_floats(rotations, "VEC4")

    document = {
        "asset": {"version": "2.0", "generator": "ShaderEditor keyframe sample generator"},
        "scene": 0,
        "scenes": [{"name": "KeyframeCamera", "nodes": [0, 1]}],
        "nodes": [
            {"name": "TargetCube", "mesh": 0},
            {
                "name": "OrbitCamera",
                "camera": 0,
                "translation": list(orbit[0]),
                "rotation": list(rotations[0]),
            },
        ],
        "cameras": [
            {
                "name": "OrbitCamera",
                "type": "perspective",
                "perspective": {"yfov": 0.7854, "znear": 0.1, "zfar": 100.0},
            }
        ],
        "meshes": [cube_mesh(position_accessor, normal_accessor, uv_accessor, index_accessor)],
        "materials": [
            {
                "name": "TargetMaterial",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.3, 0.6, 0.9, 1.0],
                    "metallicFactor": 0.2,
                    "roughnessFactor": 0.4,
                },
            }
        ],
        "animations": [
            {
                "name": "CameraOrbit",
                "samplers": [
                    {"input": time_accessor, "output": orbit_accessor, "interpolation": "LINEAR"},
                    {"input": time_accessor, "output": rotation_accessor, "interpolation": "LINEAR"},
                ],
                "channels": [
                    {"sampler": 0, "target": {"node": 1, "path": "translation"}},
                    {"sampler": 1, "target": {"node": 1, "path": "rotation"}},
                ],
            }
        ],
        "buffers": [{"byteLength": len(builder.data), "uri": builder.uri()}],
        "bufferViews": builder.buffer_views,
        "accessors": builder.accessors,
    }
    write(document, "KeyframeCamera.gltf")


if __name__ == "__main__":
    build_animated_objects()
    build_animated_camera()
