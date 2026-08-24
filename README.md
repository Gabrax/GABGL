> [!IMPORTANT]
> All rights to the assets belong to their respective authors.

<div align="center">
  
## Game preview

</div>

<div align="center">

https://github.com/user-attachments/assets/dfeda195-f8d6-4073-aff7-d7bfe55fce39

</div>

<div align="center">
  
## Dependencies

</div>

<div align="center">

<p>
<a href="https://github.com/glfw/glfw">glfw</a> •
<a href="https://github.com/Dav1dde/glad">glad</a> •
<a href="https://github.com/g-truc/glm">glm</a> •
<a href="https://github.com/ocornut/imgui">imgui</a> •
<a href="https://github.com/CedricGuillemet/ImGuizmo">ImGuizmo</a> •
<a href="https://github.com/libsndfile/libsndfile">libsndfile</a> •
<a href="https://github.com/kcat/openal-soft">OpenAL-Soft</a> •
<a href="https://github.com/nothings/stb/blob/master/stb_image.h">stb_image</a> •
<a href="https://github.com/freetype/freetype">freetype</a> •
<a href="https://github.com/assimp/assimp">assimp</a> •
<a href="https://github.com/zeux/meshoptimizer">meshoptimizer</a> •
<a href="https://github.com/shader-slang/slang">Slang</a> •
<a href="https://github.com/nlohmann/json">nlohmann_json</a> •
<a href="https://github.com/syoyo/tinyexr">tinyEXR</a> •
<a href="https://github.com/NVIDIA-Omniverse/PhysX">PhysX</a>
</p>

</div>

## Graphics backends

Shaders for both graphics backends are authored as `.slang` files and compiled
at runtime with Slang, which keeps shader hot reload available. Install a Slang
SDK (the Vulkan SDK includes one) and expose it through `VULKAN_SDK`,
`SLANG_ROOT`, or `CMAKE_PREFIX_PATH` before configuring the project.

The shared shaders live directly in `res/shaders`. Files used by both renderers
contain `#api OPENGL` and `#api DX12` sections; the runtime selects the matching
section before asking Slang to compile it. This keeps a single shader path for
scene rendering, shadows, skyboxes, particles, debug drawing, UI, and
post-processing while still allowing the two render pipelines to use different
resource layouts and entry points.

OpenGL remains the default game renderer. On Windows, the optional DirectX 12
path can be enabled at configure time and selected without changing the saved
configuration:

```powershell
cmake -S . -B build -DGABGL_ENABLE_DX12=ON
cmake --build build --config Release
build\gl_engine.exe --renderer=dx12
```

Alternatively, set `graphics.api` in `gab.ini` to `"dx12"`. Use
`--renderer=opengl` to override that setting for a single run.

The DX12 path provides the native device, high-performance adapter selection,
command queue, double-buffered flip-model swap chain, resize, VSync/tearing
support, GPU fences, and scene rendering. It uploads the existing model and
texture assets, renders animated instances with a depth buffer and directional
lighting, and draws the menu/loading UI through a dedicated DX12 pipeline. The
DX12 scene path also includes cubemap skyboxes, diffuse/normal/specular
materials, directional/point/spot lighting, directional and point-light shadow passes, HDR
tone mapping and bloom, particles/impact marks, interaction labels, and the
PS1-style 240-line vertex snapping, pixelation, 5-bit color quantization, and
ordered dithering used by the OpenGL path. The complete ImGui editing workspace
uses the native DX12 backend and can be toggled with `Tab`.

Both renderers are selected through the same backend contract. Models, particles,
screen UI, ImGui, debug layers, culling statistics and visual-effect settings are
submitted without backend-specific branches in scene code, leaving future APIs a
single interface to implement.
