# Semantic Staging Tool

![Semantic Staging Tool logo](extra/logo.png)

Semantic Staging Tool is an AI-assisted desktop application for dressing and arranging 3D scenes. Describe the setup you want in natural language, select a Groq-compatible model, and the application creates an editable layout using the 3D assets included with the project. You can watch a demo here: [Demo YT](https://youtu.be/lHrFCO-PiiQ?si=9yOEDWVKB7NFpmC0)

> **Version 1.0.0** — first public release.

## Features

- Generates 3D layouts from natural-language prompts.
- Loads `.obj`, `.gltf`, and `.glb` models from `assets/models`.
- Uses the dimensions of the available assets when generating a scene.
- Supports new scenes and incremental changes to an existing scene.
- Places objects on the floor or on top of other objects.
- Keeps floor objects within the configured room and applies basic overlap handling.
- Provides a fly camera and on-screen gizmos for manual movement and Y-axis rotation.
- Lets you clear the scene, undo the last prompt, and export the final layout as JSON.

## Download and run

Prebuilt packages are available from the GitHub **Releases** page for:

- Linux x86_64 (`.tar.gz`)
- Windows x64 (`.zip`)

Extract the archive and run the executable from the extracted folder. Keep `assets/` and `shaders/` next to the executable: they are required at runtime.

For Windows, install the Microsoft Visual C++ Redistributable if the application reports a missing runtime DLL.

## Getting started

1. Launch the application.
2. Enter your Groq API key in **API Key**. It is kept in memory only for the current session.
3. Choose an LLM model. `openai/gpt-oss-120b` prioritizes quality, `openai/gpt-oss-20b` is faster, and `qwen/qwen3.8-27b` is an alternative.
4. Set the room **Width (X)** and **Depth (Z)** in metres.
5. Write a request in **Prompt**, for example: `Arrange a cosy study area with a desk, chair, lamp, books, and a bookshelf.`
6. Select **Send** and wait for the scene to appear.

The first successful prompt creates a scene. Later prompts can add objects or edit an existing object by referring to its instance ID. Use **Clear** to start over, or **Undo Prompt** to revert the most recent AI-generated change.

## Viewport controls

| Action | Control |
| --- | --- |
| Look around | Move the mouse while holding Right-click |
| Move forward / backward | `W` / `S` |
| Strafe right / left | `D` / `A` |
| Move up / down | `E` / `Q` |
| Move faster | Hold `Left Shift` |
| Select an object | Left-click it in the viewport |
| Translation gizmo | `T` |
| Rotation gizmo | `R` |

After selecting an object, drag the gizmo to adjust it. Rotation is stored around the vertical (Y) axis.

## Exporting a scene

Select **Export to JSON** to create `saved_scene.json` in the application's working directory. The export contains the final, manually adjusted positions rather than the raw AI response:

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

`prop_id` is the filename stem of the loaded model, positions use raylib world coordinates, and `rotation_y` is expressed in degrees.

## Build from source

### Requirements

- CMake 3.24 or newer
- A C++20-capable compiler: GCC, Clang, or MSVC
- Git and an internet connection for the first CMake configure step
- A graphics environment supported by [raylib](https://www.raylib.com/)
- A Groq API key for AI scene generation

CMake downloads the required libraries automatically: raylib, Dear ImGui/rlImGui, ImGuizmo, nlohmann/json, and CPR.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/SemanticStagingTool
```

With multi-configuration generators such as Visual Studio, the executable may be located at `build/Release/`.

## Adding assets

Add `.obj`, `.gltf`, or `.glb` files anywhere below `assets/models`, then rebuild the application. Models are discovered recursively. Avoid duplicate filename stems: the filename stem becomes the asset identifier used in prompts and exports.

The generator only chooses assets it finds in `assets/models`. Structural assets, including walls, floors, ceilings, doors, windows, boards, and posters, are excluded from generated prop choices.

## Limitations

Generated layouts are drafts, not final scene designs. AI models can make poor spatial or semantic decisions, particularly with crowded rooms, vague prompts, similar assets, incremental edits, or unusual model dimensions. Review each result in the viewport and use the gizmos to make the final adjustments.

The application validates the model response and applies basic bounds and overlap handling, but it cannot guarantee physically realistic, collision-free, or aesthetically suitable results.

## Troubleshooting

- A valid Groq API key is required. The application sends it directly to Groq as a Bearer token and does not save it.
- Requests time out after 15 seconds. Authentication, network, and API errors are shown in the editor panel.
- If no models appear, confirm that the executable is running alongside its `assets/` directory.
- If shaders or models fail to load in a downloaded package, extract the archive completely and keep both `assets/` and `shaders/` beside the executable.

## Credits

The included test assets are from Kenney's [Graveyard Kit](https://kenney.itch.io/kenney-game-assets), licensed under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/).

This project uses [raylib](https://www.raylib.com/), created by Ramon Santamaria and contributors and released under the [zlib/libpng license](https://github.com/raysan5/raylib/blob/master/LICENSE).

## License

Unless stated otherwise, the source code is licensed under the [Semantic Staging Tool Use-Only License](LICENSE). It permits personal and commercial use, including internal business use, but prohibits redistribution of the source code, binaries, or derivatives. This is source-available software, not open source.

The test assets in `assets/models/Graveyard` are licensed separately under CC0 1.0 and are not covered by the project license.
