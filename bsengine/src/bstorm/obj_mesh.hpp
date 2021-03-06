#pragma once

#include <bstorm/obj_render.hpp>

#include <memory>

namespace bstorm
{
class Mesh;
class ObjMesh : public ObjRender
{
public:
    ObjMesh(const std::shared_ptr<Package>& state);
    ~ObjMesh();
    void Update() override;
    void Render(const std::shared_ptr<Renderer>& renderer) override;
    void SetMesh(const std::shared_ptr<Mesh>& mesh);
private:
    std::shared_ptr<Mesh> mesh_;
};
}