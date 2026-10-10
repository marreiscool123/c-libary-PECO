#include <peco\peco.hpp>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    peco::Application app(L"Peco - 2D and 3D example", 960, 640);
    peco::Renderer3D renderer;
    renderer.setClearColor({18, 23, 33});
    peco::Mesh cube = peco::Mesh::cube(1.8f, {90, 174, 255});
    peco::Camera camera;
    camera.setPosition({3.0f, 2.2f, 5.0f});
    camera.setTarget({0.0f, 0.0f, 0.0f});
    app.setUpdateHandler([&](float deltaSeconds) {
        (void)deltaSeconds;
        if (app.window().input().keyDown(VK_ESCAPE)) {
            PostMessageW(app.window().handle(), WM_CLOSE, 0, 0);
        }
    });
    app.setRenderHandler([&](HDC dc, int width, int height) {
        renderer.render(dc, width, height, cube, camera);
        peco::Canvas2D canvas(dc);
        canvas.text(20, 18, L"Peco: native Win32 + 2D / software 3D", {240, 244, 255}, 22);
        canvas.text(20, 48, L"Hold the left mouse button and move it to draw.", {175, 190, 215}, 17);
        canvas.rectangle(18, 78, 220, 112, {100, 190, 255}, 2);
        canvas.fillCircle(72, 134, 22, {255, 170, 80});
        canvas.line(112, 110, 212, 158, {170, 245, 190}, 3);

        if (app.window().input().mouseButtonDown(VK_LBUTTON)) {
            const POINT mouse = app.window().input().mousePosition();
            canvas.fillCircle(mouse.x, mouse.y, 8, {255, 230, 100});
        }
    });

    return app.run();
}
