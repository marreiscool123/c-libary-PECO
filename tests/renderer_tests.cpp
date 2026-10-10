#include <peco\peco.hpp>

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

int main()
{
    peco::Image canvasImage{32, 32, peco::Color::black()};
    peco::Renderer2D renderer2d{canvasImage};
    renderer2d.setBlendMode(peco::BlendMode2D::Replace);
    renderer2d.fillRect({4.0f, 4.0f, 12.0f, 10.0f}, peco::Color::red());
    assert(canvasImage.at(5, 5) == peco::Color::red());
    assert(canvasImage.at(20, 20) == peco::Color::black());
    assert(renderer2d.pushClip({8.0f, 8.0f, 4.0f, 4.0f}));
    renderer2d.fillRect({0.0f, 0.0f, 32.0f, 32.0f}, peco::Color::blue());
    assert(canvasImage.at(9, 9) == peco::Color::blue());
    assert(canvasImage.at(6, 6) == peco::Color::red());
    assert(renderer2d.popClip());
    renderer2d.setBlendMode(peco::BlendMode2D::Alpha);
    renderer2d.fillSector({16.0f, 16.0f}, {7.0f, 7.0f},
                          0.0f, 1.5707963f, peco::Color::green());
    renderer2d.gaussianBlur({8.0f, 8.0f, 16.0f, 16.0f}, 1);
    assert(renderer2d.drawImageQuad(canvasImage, {2.0f, 2.0f}, {10.0f, 2.0f},
                                    {10.0f, 10.0f}, {2.0f, 10.0f}));

    peco::Image texturePixels{3, 1, peco::Color::black()};
    texturePixels.at(2, 0) = peco::Color::white();
    peco::ImageL3D texture{texturePixels};
    texture.generateMipmaps();
    assert(texture.mipLevelCount() == 3);
    texture.sampler().filter = peco::TextureFilter::Nearest;
    texture.sampler().addressU = peco::TextureAddressMode::Clamp;
    assert(texture.sample({0.9f, 0.5f}) == peco::Color::white());

    peco::Renderer3DTarget renderer3d{64, 64};
    peco::Camera camera;
    camera.setPosition({3.0f, 2.0f, 5.0f});
    camera.setTarget({0.0f, 0.0f, 0.0f});
    assert(renderer3d.render(peco::Mesh::cube(), camera, texture));
    assert(renderer3d.statistics().inputTriangles == 12);
    assert(renderer3d.statistics().shadedPixels > 0);
    peco::Vec2 projected;
    float projectedDepth = 0.0f;
    assert(renderer3d.projectPoint({}, camera, projected, &projectedDepth));
    peco::Vec3 unprojected;
    assert(renderer3d.unprojectPoint(projected, projectedDepth, camera, unprojected));
    assert(peco::Vec3::distance(unprojected, {}) < 0.001f);
    peco::MeshHit3D hit;
    assert(renderer3d.pick(peco::Mesh::cube(), camera, {32.0f, 32.0f}, hit));
    assert(renderer3d.drawScreenSpaceTriangle(
        {1.0f, 1.0f}, 0.0f, peco::Color::red(),
        {6.0f, 1.0f}, 0.0f, peco::Color::green(),
        {1.0f, 6.0f}, 0.0f, peco::Color::blue()));

    peco::Scene scene;
    auto node = std::make_shared<peco::SceneNode>("cube");
    node->setMesh(std::make_shared<const peco::Mesh>(peco::Mesh::cube()));
    assert(scene.root()->addChild(node));
    assert(renderer3d.renderScene(scene));
    assert(renderer3d.statistics().inputTriangles == 12);
}
