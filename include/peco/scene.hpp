#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <peco\camera.hpp>
#include <peco\lighting.hpp>
#include <peco\material.hpp>
#include <peco\mesh.hpp>
#include <peco\transform3d.hpp>

namespace peco {

class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
    explicit SceneNode(std::string name = {})
        : name_(std::move(name)) {}

    const std::string& name() const { return name_; }
    void setName(std::string name) { name_ = std::move(name); }
    bool enabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    const Transform3D& localTransform() const { return localTransform_; }
    Transform3D& localTransform() { return localTransform_; }
    void setLocalTransform(const Transform3D& transform) { localTransform_ = transform; }
    std::shared_ptr<SceneNode> parent() const { return parent_.lock(); }
    const std::vector<std::shared_ptr<SceneNode>>& children() const { return children_; }
    std::size_t childCount() const { return children_.size(); }

    bool addChild(const std::shared_ptr<SceneNode>& child)
    {
        if (!child || child.get() == this || child->isAncestorOf(this)) {
            return false;
        }
        const auto self = weak_from_this().lock();
        if (!self) {
            return false;
        }
        const auto currentParent = child->parent_.lock();
        if (currentParent.get() == this) {
            return false;
        }
        if (currentParent) {
            currentParent->removeChild(child);
        }
        child->parent_ = self;
        children_.push_back(child);
        return true;
    }
    bool removeChild(const std::shared_ptr<SceneNode>& child)
    {
        const auto found = std::find(children_.begin(), children_.end(), child);
        if (found == children_.end()) {
            return false;
        }
        (*found)->parent_.reset();
        children_.erase(found);
        return true;
    }
    void detach()
    {
        const auto owner = parent_.lock();
        const auto self = weak_from_this().lock();
        if (owner && self) {
            owner->removeChild(self);
        }
    }
    Transform3D worldTransform() const
    {
        const auto owner = parent_.lock();
        return owner ? localTransform_.combined(owner->worldTransform()) : localTransform_;
    }
    Vec3 worldPosition() const { return worldTransform().position(); }

    void setMesh(std::shared_ptr<const Mesh> mesh) { mesh_ = std::move(mesh); }
    const std::shared_ptr<const Mesh>& mesh() const { return mesh_; }
    bool hasMesh() const { return static_cast<bool>(mesh_); }
    Material& material() { return material_; }
    const Material& material() const { return material_; }

    void traverseMutable(const std::function<void(SceneNode&)>& visitor)
    {
        visitor(*this);
        const auto snapshot = children_;
        for (const auto& child : snapshot) {
            if (child) {
                child->traverseMutable(visitor);
            }
        }
    }
    void traverse(const std::function<void(const SceneNode&)>& visitor) const
    {
        visitor(*this);
        const auto snapshot = children_;
        for (const auto& child : snapshot) {
            if (child) {
                static_cast<const SceneNode&>(*child).traverse(visitor);
            }
        }
    }
    std::shared_ptr<SceneNode> find(const std::string& nodeName)
    {
        if (name_ == nodeName) {
            return weak_from_this().lock();
        }
        for (const auto& child : children_) {
            auto result = child->find(nodeName);
            if (result) {
                return result;
            }
        }
        return {};
    }

private:
    bool isAncestorOf(const SceneNode* node) const
    {
        auto cursor = node->parent_.lock();
        while (cursor) {
            if (cursor.get() == this) {
                return true;
            }
            cursor = cursor->parent_.lock();
        }
        return false;
    }

    std::string name_;
    bool enabled_ = true;
    Transform3D localTransform_;
    std::weak_ptr<SceneNode> parent_;
    std::vector<std::shared_ptr<SceneNode>> children_;
    std::shared_ptr<const Mesh> mesh_;
    Material material_;
};

class Scene {
public:
    Scene() : root_(std::make_shared<SceneNode>("root")) {}
    explicit Scene(std::shared_ptr<SceneNode> root)
        : root_(root ? std::move(root) : std::make_shared<SceneNode>("root")) {}

    const std::shared_ptr<SceneNode>& root() const { return root_; }
    Camera& camera() { return camera_; }
    const Camera& camera() const { return camera_; }
    AmbientLight& ambientLight() { return ambient_; }
    const AmbientLight& ambientLight() const { return ambient_; }
    std::vector<Light>& lights() { return lights_; }
    const std::vector<Light>& lights() const { return lights_; }
    void addLight(const Light& light) { lights_.push_back(light); }
    void clearLights() { lights_.clear(); }
    std::shared_ptr<SceneNode> find(const std::string& name)
    {
        return root_->find(name);
    }
    void forEachVisible(const std::function<void(SceneNode&)>& visitor)
    {
        std::function<void(SceneNode&)> visit = [&visitor, &visit](SceneNode& node) {
            if (!node.enabled()) {
                return;
            }
            visitor(node);
            const auto children = node.children();
            for (const auto& child : children) {
                if (child) {
                    visit(*child);
                }
            }
        };
        visit(*root_);
    }
    std::size_t nodeCount() const
    {
        std::size_t count = 0;
        root_->traverse([&count](const SceneNode&) { ++count; });
        return count;
    }

private:
    std::shared_ptr<SceneNode> root_;
    Camera camera_;
    AmbientLight ambient_;
    std::vector<Light> lights_;
};

} // namespace peco
