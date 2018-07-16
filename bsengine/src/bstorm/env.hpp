﻿#pragma once

#include <bstorm/nullable_shared_ptr.hpp>

#include <unordered_map>
#include <memory>

namespace bstorm
{
struct NodeDef;
class Env
{
public:
    Env();
    Env(const std::shared_ptr<Env>& parent);
    const std::shared_ptr<NodeDef>& AddDef(const std::string& name, const std::shared_ptr<NodeDef>& def);
    const std::shared_ptr<NodeDef>& AddDef(std::string&& name, std::shared_ptr<NodeDef>&& def);
    NullableSharedPtr<NodeDef> FindDef(const std::string& name) const;
    bool IsRoot() const;

    using NameTable = std::unordered_map<std::string, std::shared_ptr<NodeDef>>;
    NameTable& GetCurrentBlockNameTable() { return table_; }
    const std::shared_ptr<Env>& GetParent() const { return parent_; }
private:
    std::shared_ptr<Env> parent_;
    NameTable table_;
    const int depth_;
};
}