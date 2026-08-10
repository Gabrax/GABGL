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
<a href="https://github.com/nlohmann/json">nlohmann_json</a> •
<a href="https://github.com/syoyo/tinyexr">tinyEXR</a> •
<a href="https://github.com/NVIDIA-Omniverse/PhysX">PhysX</a>
</p>

</div>

## Graphics backends

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
lighting, and draws the menu/loading UI through a dedicated DX12 pipeline.

OpenGL is still the feature-complete backend. The optional DX12 renderer does
not yet implement the OpenGL path's skybox, shadow maps, bloom, particles,
normal/specular maps, or editor overlays.
