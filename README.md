# Opengl_boilerplate

---

Easy startup for OpenGL crossplatform development with C++

To build and run project, use the build.sh in root directory

- Windows & Linux & MacOS
```bash
sh build.sh
```

## 👨‍💻 Utilities
- GLFW
- GLAD
- GLM
- GL2D
- ImGUI
- RAudio
- stb_image
- stb_truetype
- Assimp
- ENET
- [glText](https://github.com/vallentin/glText)

## 💬 IMPORTANT!
  To ship the game: 
  In Cmakelists.txt, set the PRODUCTION_BUILD flag to ON to build a shippable version of application. This will change the file paths to be relative to exe (RESOURCES_PATH macro), will remove the console, and also will change the asserts to not allow people to debug them. To make sure the changes take effect, is recommended deleting the out folder to make a new clean build
