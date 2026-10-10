

## Build

Use Visual Studio 2019 or newer with the Windows SDK and CMake 3.20 or newer:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

This builds the `Peco` library and, by default, the `peco_basic` example.
Disable the example with `-DPECO_BUILD_EXAMPLES=OFF`.
Run the included tests with `ctest --test-dir build --output-on-failure`.

## Quick start

```cpp
#include <peco/peco.hpp>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    peco::Application app(L"Peco app", 960, 640);
    peco::Renderer3D renderer;
    const auto cube = peco::Mesh::cube();
    peco::Camera camera;
    camera.setPosition({3.0f, 2.0f, 5.0f});
    camera.setTarget({0.0f, 0.0f, 0.0f});

    app.setUpdateHandler([&](float dt) {
        (void)dt;
    });
    app.setRenderHandler([&](HDC dc, int width, int height) {
        renderer.render(dc, width, height, cube, camera);
        peco::Canvas2D canvas(dc);
        canvas.text(16, 16, L"Hello from Peco", {255, 255, 255});
    });
    return app.run();
}
```

## Public API

All public headers live directly under `include/peco`; there are no public
category subdirectories. Include the umbrella header for the complete API:

```cpp
#include <peco/peco.hpp>
```

The public API includes:

- **Math:** vectors, matrices, quaternions, angles, 2D/3D transforms, bounds,
  rays, planes, interpolation, easing curves, and seeded random generation.
- **2D graphics:** `Canvas2D`, vector paths and flattened contours, geometric
  shapes, gradients, and a CPU-side `Image` buffer with drawing, cropping,
  flipping, sampling, and alpha compositing. `Renderer2D` from
  `<peco/2DR.hpp>` rasterizes into an `Image` and provides clipping, gradients,
  textured geometry, image transforms, and pixel effects.
- **3D graphics:** perspective camera navigation and screen rays, procedural
  plane/grid/sphere/cylinder/cone meshes, materials, lights, frustum tests, and
  a transformable scene graph. The Win32 `Renderer3D` draws a mesh to an HDC.
  `<peco/3DR.hpp>` also provides `Renderer3DTarget`, a CPU renderer with a
  color/depth image target, clipping, depth testing, mipmapped textures,
  material/light shading, scene instances, picking, and post-processing.
- **Input and UI:** keyboard/mouse held/pressed/released state, named action
  bindings, flex layout, and wrappers for native Win32 buttons, labels,
  checkboxes, radio buttons, edit controls, combo boxes, and list boxes.
- **Core utilities:** scoped event connections, value/error `Result`, and
  scope guards.

Native controls are child HWNDs. `Window::setCommandHandler` receives their
`WM_COMMAND` notifications:

```cpp
peco::Application app(L"Controls", 640, 400);
peco::Window& window = app.window();
peco::ui::Button save;
if (!save.create(window.handle(), 100, L"Save", RECT{20, 20, 120, 54})) {
    return 1;
}
window.setCommandHandler([](int id, int code, HWND) {
    if (id == 100 && code == BN_CLICKED) {
        // Save the document.
    }
});
return app.run();
```

`ImageLoader`, `ImageL2D`, and `ImageL3D` load images through Windows Imaging
Component (WIC); BMP saving is also available. `ImageL2D` draws through
`Canvas2D`, while `ImageL3D` exposes configurable address modes, filtering, and
mip levels for 3D texture sampling.

`Renderer2D` renders into an `Image`, so it can be used without an HDC and then
presented with `Canvas2D::drawImage`. `Renderer3DTarget` renders a `Mesh` and
`Camera` into an image with a depth buffer; `present` copies that image to a
`Canvas2D`. For example:

```cpp
peco::Image frame{640, 480};
peco::Renderer2D renderer2d{frame};
renderer2d.fillRect({20, 20, 200, 100}, peco::Color::blue());

peco::Renderer3DTarget renderer3d{640, 480};
peco::Mesh cube = peco::Mesh::cube();
peco::Camera camera;
renderer3d.render(cube, camera);
```

Use `Mesh::cube()` or `MeshBuilder` to make geometry. `Mesh::loadObj` and
`Mesh::saveObj` provide Wavefront OBJ import/export, including triangulated
polygons, normals, texture coordinates, and material assignments. `Scene`
organizes nodes, transforms, mesh instances, camera, and lights, while
`ActionMap` maps input to named controls.

## Current scope

Peco is a Windows-native foundation, not a hardware-accelerated engine. Both
software renderers operate on CPU memory; the HDC renderer remains available
for direct Win32 presentation. The library currently targets Windows.
