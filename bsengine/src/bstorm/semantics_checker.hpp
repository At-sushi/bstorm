﻿#pragma once

#include <bstorm/node.hpp>
#include <bstorm/logger.hpp>

#include <vector>
#include <stack>

namespace bstorm
{
class Env;
class SemanticsChecker : public NodeTraverser
{
public:
    std::vector<Log> Check(Node& n);
    void Traverse(NodeNum&) override;
    void Traverse(NodeChar&) override;
    void Traverse(NodeStr&) override;
    void Traverse(NodeArray&) override;
    void Traverse(NodeNeg&) override;
    void Traverse(NodeNot&) override;
    void Traverse(NodeAbs&) override;
    void Traverse(NodeAdd&) override;
    void Traverse(NodeSub&) override;
    void Traverse(NodeMul&) override;
    void Traverse(NodeDiv&) override;
    void Traverse(NodeRem&) override;
    void Traverse(NodePow&) override;
    void Traverse(NodeLt&) override;
    void Traverse(NodeGt&) override;
    void Traverse(NodeLe&) override;
    void Traverse(NodeGe&) override;
    void Traverse(NodeEq&) override;
    void Traverse(NodeNe&) override;
    void Traverse(NodeAnd&) override;
    void Traverse(NodeOr&) override;
    void Traverse(NodeCat&) override;
    void Traverse(NodeNoParenCallExp&) override;
    void Traverse(NodeCallExp&) override;
    void Traverse(NodeArrayRef&) override;
    void Traverse(NodeRange&) override;
    void Traverse(NodeArraySlice&) override;
    void Traverse(NodeNop&) override;
    void Traverse(NodeLeftVal&) override;
    void Traverse(NodeAssign&) override;
    void Traverse(NodeAddAssign&) override;
    void Traverse(NodeSubAssign&) override;
    void Traverse(NodeMulAssign&) override;
    void Traverse(NodeDivAssign&) override;
    void Traverse(NodeRemAssign&) override;
    void Traverse(NodePowAssign&) override;
    void Traverse(NodeCatAssign&) override;
    void Traverse(NodeCallStmt&) override;
    void Traverse(NodeReturn&) override;
    void Traverse(NodeReturnVoid&) override;
    void Traverse(NodeYield&) override;
    void Traverse(NodeBreak&) override;
    void Traverse(NodeSucc&) override;
    void Traverse(NodePred&) override;
    void Traverse(NodeVarDecl&) override;
    void Traverse(NodeVarInit&) override;
    void Traverse(NodeProcParam&) override;
    void Traverse(NodeLoopParam&) override;
    void Traverse(NodeResult&) override;
    void Traverse(NodeBlock&) override;
    void Traverse(NodeSubDef&) override;
    void Traverse(NodeBuiltInSubDef&) override;
    void Traverse(NodeFuncDef&) override;
    void Traverse(NodeTaskDef&) override;
    void Traverse(NodeBuiltInFunc&) override;
    void Traverse(NodeConst&) override;
    void Traverse(NodeLocal&) override;
    void Traverse(NodeLoop&) override;
    void Traverse(NodeTimes&) override;
    void Traverse(NodeWhile&) override;
    void Traverse(NodeAscent&) override;
    void Traverse(NodeDescent&) override;
    void Traverse(NodeElseIf&) override;
    void Traverse(NodeIf&) override;
    void Traverse(NodeCase&) override;
    void Traverse(NodeAlternative&) override;
    void Traverse(NodeHeader&) override;
private:
    bool IsInFunc() const;
    bool IsInLoop() const;
    std::vector<Log> errors_;
    std::stack<bool> funcCtxStack_;
    std::stack<bool> loopCtxStack_;
    std::shared_ptr<Env> env_;
    void CheckMonoOp(NodeMonoOp& exp);
    void CheckBinOp(NodeBinOp& exp);
    void CheckAssign(NodeAssign& stmt);
};
}