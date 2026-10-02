# Semantic Staging Tool

Semantic Staging Tool is a desktop 3D set-dressing and level-design prototype written in C++20. It turns a natural-language prompt into a practical arrangement of the 3D assets available in the project, then lets you inspect and refine that result directly in a real-time viewport.

The application sends the prompt, room dimensions, measurements for the loaded props, and (when applicable) the current scene to a Groq OpenAI-compatible chat-completions endpoint. The model returns a strict JSON layout. The application validates the referenced props, places floor and surface objects, keeps floor objects inside the room, resolves simple overlaps, and renders the result.

## Features

- Loads `.obj`, `.gltf`, and `.glb` models recursively from `assets/models`.
- Builds an AI prompt from the actual available assets and their bounding dimensions.
- Creates a new scene from the first prompt, or adds to the existing scene on later prompts.
- Supports floor placement and placing small assets on top of another placed asset.
- Clamps floor objects to the selected room bounds and resolves simple XZ overlaps.
- Offers a fly camera and on-screen gizmos for manual translation and Y-axis rotation.
- Exports the placed instances as JSON.

## Requirements

- CMake 3.24 or newer
- A C++20-capable compiler (GCC, Clang, or MSVC)
- Git and an internet connection for CMake to download dependencies on the first configure
- A desktop graphics environment supported by [raylib](https://www.raylib.com/)
- A Groq API key, for AI generation

CMake downloads and builds raylib, Dear ImGui/rlImGui, ImGuizmo, nlohmann/json, and CPR automatically. No manual installation of these libraries is required.

## Build

From the project root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run the executable from the build directory so it can find the `assets` directory copied by CMake:

```bash
./build/SemanticStagingTool
```

On multi-configuration generators such as Visual Studio, the executable may instead be under `build/Release/`.

## Generate a scene

1. Start the application.
2. Enter a Groq API key in **API Key**. The key stays in memory only for the current run.
3. Set the room **Width (X)** and **Depth (Z)** in metres.
4. Describe the desired setup in **Prompt**. For example: `Arrange a cosy study area with a desk, chair, lamp, books, and a bookshelf.`
5. Select **Send** and wait for the scene to appear.

The first successful request replaces the scene. Once a scene contains objects, later requests are treated as additions: existing objects are preserved and the model receives their positions to help it avoid conflicts. Use **Clear** before sending a prompt when you want to start over.

The generator can only choose props that the application found under `assets/models`. Structural assets—such as walls, floors, ceilings, doors, windows, boards, and posters—are deliberately excluded from generated prop choices.

## Viewport controls

| Action | Control |
| --- | --- |
| Look around | Move the mouse |
| Move forward / backward | `W` / `S` |
| Strafe right / left | `D` / `A` |
| Move up / down | `E` / `Q` |
| Move faster | Hold `Left Shift` |
| Select an object | Left-click it in the viewport |
| Use translation gizmo | Press `T` |
| Use rotation gizmo | Press `R` |

After selecting an object, drag the displayed gizmo to edit it. Rotation is stored around the vertical (Y) axis.

## Exported scene format

Select **Export to JSON** to write `saved_scene.json` to the process working directory. The export contains the final placed instances rather than the original model response:

```json
{
  "entities": [
    {
      "instance_id": "desk_01",
      "prop_id": "M_room_desk",
      "position": { "x": 1.2, "y": 0.0, "z": -2.4 },
      "rotation_y": 90.0,
      "is_static": true
    }
  ]
}
```

`prop_id` is the filename stem of the corresponding loaded model. `position` uses raylib world coordinates, and `rotation_y` is in degrees.

## Adding assets

Add supported model files anywhere below `assets/models`, then rebuild (or ensure the files are present next to the executable in its copied `assets/models` directory). The application discovers models recursively. Avoid duplicate filename stems: the stem is the asset identifier used in generation and exports.

## API notes and troubleshooting

- The endpoint is currently configured for Groq's OpenAI-compatible API and uses the `openai/gpt-oss-120b` model. A valid Groq key is therefore required for generation.
- Requests have a 15-second timeout. Authentication, network, and API errors are displayed in the editor panel.
- The API key is sent as a Bearer token directly to Groq; it is not saved by the application.
- If the application opens without models, make sure the executable is being run alongside the `assets` directory produced by the CMake post-build step.
- AI output is constrained to known props, but layout quality still depends on the model response and the dimensions of the available assets. Use the gizmos to make final artistic adjustments.
