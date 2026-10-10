#include <peco\peco.hpp>

#include <cmath>

namespace {

bool approximatelyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) < 0.0001f;
}

} // namespace

int main()
{
    const peco::Vec2 vector{3.0f, 4.0f};
    if (!approximatelyEqual(vector.length(), 5.0f)) {
        return 1;
    }

    const peco::Vec3 xAxis{1.0f, 0.0f, 0.0f};
    const peco::Vec3 yAxis{0.0f, 1.0f, 0.0f};
    const peco::Vec3 cross = xAxis.cross(yAxis);
    if (!approximatelyEqual(cross.x, 0.0f)
        || !approximatelyEqual(cross.y, 0.0f)
        || !approximatelyEqual(cross.z, 1.0f)) {
        return 2;
    }

    const auto transformed = peco::Mat4::translation({2.0f, 3.0f, 4.0f})
        .transform(1.0f, 1.0f, 1.0f, 1.0f);
    if (!approximatelyEqual(transformed[0], 3.0f)
        || !approximatelyEqual(transformed[1], 4.0f)
        || !approximatelyEqual(transformed[2], 5.0f)
        || !approximatelyEqual(transformed[3], 1.0f)) {
        return 3;
    }

    const peco::Mat4 projection = peco::Mat4::perspective(1.5707963f, 1.0f, 1.0f, 10.0f);
    const auto nearPoint = projection.transform(0.0f, 0.0f, -1.0f, 1.0f);
    if (!approximatelyEqual(nearPoint[2] / nearPoint[3], -1.0f)) {
        return 4;
    }

    const peco::Mesh cube = peco::Mesh::cube();
    if (cube.vertices().size() != 8 || cube.triangles().size() != 12) {
        return 5;
    }
    for (const peco::Triangle& triangle : cube.triangles()) {
        if (triangle.a >= cube.vertices().size() || triangle.b >= cube.vertices().size()
            || triangle.c >= cube.vertices().size()) {
            return 6;
        }
    }

    peco::Mat4 inverse;
    const peco::Mat4 transform = peco::Mat4::translation({4.0f, -2.0f, 3.0f})
        * peco::Mat4::scale({2.0f, 3.0f, 4.0f});
    if (!transform.inverse(inverse)) {
        return 7;
    }
    const peco::Vec3 original{1.0f, 2.0f, 3.0f};
    const peco::Vec3 restored = inverse.transformPoint(transform.transformPoint(original));
    if (!approximatelyEqual(restored.x, original.x)
        || !approximatelyEqual(restored.y, original.y)
        || !approximatelyEqual(restored.z, original.z)) {
        return 8;
    }

    const peco::Quaternion turn = peco::Quaternion::axisAngle(
        peco::Vec3::unitZ(), peco::math::pi * 0.5f);
    const peco::Vec3 turned = turn.rotate(peco::Vec3::unitX());
    if (!approximatelyEqual(turned.x, 0.0f)
        || !approximatelyEqual(turned.y, 1.0f)) {
        return 9;
    }

    const peco::Bounds2 bounds{{0.0f, 0.0f}, {10.0f, 10.0f}};
    if (!bounds.contains({10.0f, 5.0f})
        || !bounds.intersects(peco::Bounds2{{9.0f, 9.0f}, {12.0f, 12.0f}})) {
        return 10;
    }
    if (!bounds.intersection(peco::Bounds2{{11.0f, 11.0f}, {12.0f, 12.0f}}).isEmpty()) {
        return 26;
    }

    const peco::Ray2 horizontalRay{{0.0f, 2.0f}, {1.0f, 0.0f}};
    float rectangleHit = 0.0f;
    if (!peco::intersect(horizontalRay, {3.0f, 1.0f, 4.0f, 2.0f}, rectangleHit)
        || !approximatelyEqual(rectangleHit, 3.0f)
        || peco::intersect({{0.0f, 0.0f}, {0.0f, 1.0f}},
                           {2.0f, 2.0f, 1.0f, 1.0f}, rectangleHit)) {
        return 27;
    }

    float sphereHit = 0.0f;
    const peco::Ray3 ray{{0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
    if (!peco::intersect(ray, peco::Vec3::zero(), 1.0f, sphereHit)
        || !approximatelyEqual(sphereHit, 4.0f)) {
        return 11;
    }

    peco::Path2D path;
    path.moveTo({0.0f, 0.0f}).quadraticTo({5.0f, 10.0f}, {10.0f, 0.0f});
    const auto contours = path.flatten(0.25f);
    if (contours.size() != 1 || contours.front().points.size() < 3
        || contours.front().points.front() != peco::Vec2{0.0f, 0.0f}) {
        return 12;
    }

    peco::Gradient gradient{peco::Color::black(), peco::Color::white()};
    const peco::Color midpoint = gradient.sample(0.5f);
    if (midpoint.r < 127 || midpoint.r > 128) {
        return 13;
    }

    peco::Image image{8, 8, peco::Color::black()};
    image.fillRect({2.0f, 2.0f, 3.0f, 3.0f}, peco::Color::red());
    if (image.at(3, 3) != peco::Color::red()
        || image.at(0, 0) != peco::Color::black()) {
        return 14;
    }
    const peco::Image crop = image.cropped({2.0f, 2.0f, 3.0f, 3.0f});
    if (crop.width() != 3 || crop.height() != 3
        || crop.at(1, 1) != peco::Color::red()) {
        return 15;
    }
    image.blit(image, 1, 1);
    if (image.at(2, 2) != peco::Color::black()
        || image.at(3, 3) != peco::Color::red()) {
        return 28;
    }

    const peco::Mesh sphere = peco::MeshBuilder::uvSphere(1.0f, 12, 6);
    if (sphere.vertices().size() != 91 || sphere.triangles().size() != 144) {
        return 16;
    }

    auto root = std::make_shared<peco::SceneNode>("root");
    auto child = std::make_shared<peco::SceneNode>("child");
    root->localTransform().setPosition({2.0f, 0.0f, 0.0f});
    child->localTransform().setPosition({0.0f, 3.0f, 0.0f});
    if (!root->addChild(child) || child->addChild(root)
        || !approximatelyEqual(child->worldPosition().x, 2.0f)
        || !approximatelyEqual(child->worldPosition().y, 3.0f)) {
        return 17;
    }
    auto hiddenParent = std::make_shared<peco::SceneNode>("hidden");
    auto hiddenChild = std::make_shared<peco::SceneNode>("also-hidden");
    hiddenParent->setEnabled(false);
    if (!root->addChild(hiddenParent) || !hiddenParent->addChild(hiddenChild)) {
        return 29;
    }
    peco::Scene scene{root};
    std::size_t visibleCount = 0;
    scene.forEachVisible([&visibleCount](peco::SceneNode&) { ++visibleCount; });
    if (visibleCount != 2 || scene.nodeCount() != 4) {
        return 30;
    }

    peco::ui::FlexLayout layout;
    std::vector<peco::ui::LayoutItem> items(3);
    for (auto& item : items) {
        item.preferred = {40.0f, 20.0f};
        item.minimum = {10.0f, 10.0f};
        item.grow = 1.0f;
    }
    const auto arranged = layout.arrange({0.0f, 0.0f, 300.0f, 40.0f}, items);
    if (arranged.size() != 3
        || !approximatelyEqual(arranged[0].width, 100.0f)
        || !approximatelyEqual(arranged[2].x, 200.0f)) {
        return 18;
    }

    peco::Event<int> event;
    int observed = 0;
    auto connection = event.connect([&observed](int value) { observed += value; });
    event.emit(7);
    if (observed != 7 || !connection.connected()) {
        return 19;
    }
    connection.disconnect();
    event.emit(3);
    if (observed != 7 || !event.empty()) {
        return 20;
    }

    const auto result = peco::Result<int, std::string>::success(4)
        .map([](int value) { return value * 3; });
    if (!result || result.value() != 12) {
        return 21;
    }

    peco::Input input;
    peco::ActionMap actions;
    actions.bindKey("jump", 'J');
    input.beginFrame();
    input.processMessage(WM_KEYDOWN, 'J', 0);
    if (!input.keyDown('J') || !input.keyPressed('J')
        || !actions.down("jump", input) || !actions.pressed("jump", input)) {
        return 22;
    }
    input.beginFrame();
    input.processMessage(WM_KEYUP, 'J', 0);
    if (input.keyDown('J') || !input.keyReleased('J')
        || !actions.released("jump", input)) {
        return 23;
    }
    actions.bindAxis("horizontal", 'A', 'D');
    input.beginFrame();
    input.processMessage(WM_KEYDOWN, 'D', 0);
    if (!approximatelyEqual(actions.axis("horizontal", input), 1.0f)) {
        return 31;
    }

    const peco::Color blended = peco::Color::alphaBlend(
        {255, 0, 0, 128}, {0, 0, 255, 255});
    if (blended.r < 127 || blended.r > 128
        || blended.b < 126 || blended.b > 127 || blended.a != 255) {
        return 24;
    }

    const peco::Ray3 centerRay = peco::Camera{}.rayFromScreen({50.0f, 50.0f},
                                                               {100.0f, 100.0f});
    if (centerRay.direction.z >= 0.0f) {
        return 25;
    }

    peco::Random firstRandom(1234);
    peco::Random secondRandom(1234);
    for (int index = 0; index < 32; ++index) {
        if (firstRandom.nextUInt() != secondRandom.nextUInt()) {
            return 32;
        }
    }

    return 0;
}
