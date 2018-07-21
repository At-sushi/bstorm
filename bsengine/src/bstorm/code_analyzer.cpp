#include <bstorm/code_analyzer.hpp>

#include <bstorm/env.hpp>
#include <bstorm/script_entry_routine_names.hpp>

#include <algorithm>

namespace bstorm
{
// NOTE: 到達可能な部分だけ解析

void CodeAnalyzer::Analyze(Node & n)
{
    env_ = nullptr;
    n.Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeNum & lit)
{
    lit.noSubEffect = true;
    lit.expType = NodeExp::T_REAL;
}
void CodeAnalyzer::Traverse(NodeChar & lit)
{
    lit.noSubEffect = true;
    lit.expType = NodeExp::T_CHAR;
}
void CodeAnalyzer::Traverse(NodeStr & lit)
{
    lit.noSubEffect = true;
    lit.expType = lit.str.empty() ? NodeExp::T_ARRAY(NodeExp::T_EMPTY) : NodeExp::T_STRING;
}
void CodeAnalyzer::Traverse(NodeArray & array)
{
    array.noSubEffect = true;
    for (auto& e : array.elems)
    {
        e->Traverse(*this);
        if (!e->noSubEffect)
        {
            array.noSubEffect = false;
        }
    }
    if (array.elems.empty())
    {
        array.expType = NodeExp::T_ARRAY(NodeExp::T_EMPTY);
    } else
    {
        const auto firstElemExpType = array.elems[0]->expType;
        if (std::all_of(array.elems.begin(), array.elems.end(), [firstElemExpType](auto& e) { return e->expType == firstElemExpType; }))
        {
            array.expType = firstElemExpType;
        } else
        {
            array.expType = NodeExp::T_ARRAY(NodeExp::T_ANY);
        }
    }
}
void CodeAnalyzer::Traverse(NodeNeg& exp) { AnalyzeMonoOp(exp); exp.expType = NodeExp::T_REAL; }
void CodeAnalyzer::Traverse(NodeNot& exp) { AnalyzeMonoOp(exp); exp.expType = NodeExp::T_BOOL; }
void CodeAnalyzer::Traverse(NodeAbs& exp) { AnalyzeMonoOp(exp); exp.expType = NodeExp::T_REAL; }
void CodeAnalyzer::Traverse(NodeAdd& exp) { AnalyzeArithAndArrayBinOp(exp); }
void CodeAnalyzer::Traverse(NodeSub& exp) { AnalyzeArithAndArrayBinOp(exp); }
void CodeAnalyzer::Traverse(NodeMul& exp) { AnalyzeArithBinOp(exp); }
void CodeAnalyzer::Traverse(NodeDiv& exp) { AnalyzeArithBinOp(exp); }
void CodeAnalyzer::Traverse(NodeRem& exp) { AnalyzeArithBinOp(exp); }
void CodeAnalyzer::Traverse(NodePow& exp) { AnalyzeArithBinOp(exp); }
void CodeAnalyzer::Traverse(NodeLt& exp) { AnalyzeCmpBinOp(exp); }
void CodeAnalyzer::Traverse(NodeGt& exp) { AnalyzeCmpBinOp(exp); }
void CodeAnalyzer::Traverse(NodeLe& exp) { AnalyzeCmpBinOp(exp); }
void CodeAnalyzer::Traverse(NodeGe& exp) { AnalyzeCmpBinOp(exp); }
void CodeAnalyzer::Traverse(NodeEq& exp) { AnalyzeCmpBinOp(exp); }
void CodeAnalyzer::Traverse(NodeNe& exp) { AnalyzeCmpBinOp(exp); }
void CodeAnalyzer::Traverse(NodeAnd& exp) { AnalyzeBinOp(exp); }
void CodeAnalyzer::Traverse(NodeOr& exp) { AnalyzeBinOp(exp); }
void CodeAnalyzer::Traverse(NodeCat& exp)
{
    AnalyzeBinOp(exp);
    if (exp.lhs->expType == NodeExp::T_ARRAY(NodeExp::T_EMPTY) && NodeExp::IsArrayType(exp.rhs->expType))
    {
        exp.expType = exp.rhs->expType;
    } else if (exp.rhs->expType == NodeExp::T_ARRAY(NodeExp::T_EMPTY) && NodeExp::IsArrayType(exp.lhs->expType))
    {
        exp.expType = exp.lhs->expType;
    } else if (exp.lhs->expType == exp.rhs->expType && NodeExp::IsArrayType(exp.lhs->expType))
    {
        exp.expType = exp.lhs->expType;
    } else
    {
        exp.expType = NodeExp::T_ARRAY(NodeExp::T_ANY);
    }
}
void CodeAnalyzer::Traverse(NodeNoParenCallExp & call)
{
    AnalyzeDef(call.name);
    auto def = env_->FindDef(call.name);
    call.noSubEffect = def->noSubEffect;
    if (auto varDecl = std::dynamic_pointer_cast<NodeVarDecl>(def))
    {
        varDecl->refCnt++;
    }
}
void CodeAnalyzer::Traverse(NodeCallExp & call)
{
    AnalyzeDef(call.name);
    call.noSubEffect = env_->FindDef(call.name)->noSubEffect;
    for (auto& arg : call.args)
    {
        arg->Traverse(*this);
        if (!arg->noSubEffect)
        {
            call.noSubEffect = false;
        }
    }
}
void CodeAnalyzer::Traverse(NodeArrayRef& exp)
{
    exp.array->Traverse(*this);
    exp.idx->Traverse(*this);
    exp.noSubEffect = exp.array->noSubEffect && exp.idx->noSubEffect;
}
void CodeAnalyzer::Traverse(NodeRange& range)
{
    range.start->Traverse(*this);
    range.end->Traverse(*this);
    range.noSubEffect = range.start->noSubEffect && range.end->noSubEffect;
}
void CodeAnalyzer::Traverse(NodeArraySlice& exp)
{
    exp.array->Traverse(*this);
    exp.range->Traverse(*this);
    exp.noSubEffect = exp.array->noSubEffect && exp.range->noSubEffect;
}
void CodeAnalyzer::Traverse(NodeNop &) {}
void CodeAnalyzer::Traverse(NodeLeftVal & left)
{
    AnalyzeDef(left.name);
    for (auto& idx : left.indices)
    {
        idx->Traverse(*this);
    }
    if (auto varDecl = std::dynamic_pointer_cast<NodeVarDecl>(env_->FindDef(left.name)))
    {
        varDecl->assignCnt++;
    }
}
void CodeAnalyzer::Traverse(NodeAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeAddAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeSubAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeMulAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeDivAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeRemAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodePowAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeCatAssign& stmt) { AnalyzeAssign(stmt); }
void CodeAnalyzer::Traverse(NodeBlock & blk)
{
    env_ = std::make_shared<Env>(blk.nameTable, env_);

    if (env_->IsRoot())
    {
        for (auto&& name : SCRIPT_ENTRY_ROUTINE_NAMES)
        {
            AnalyzeDef(name);
        }
    }

    for (const auto& stmt : blk.stmts)
    {
        stmt->Traverse(*this);
    }

    env_ = env_->GetParent();
}
void CodeAnalyzer::Traverse(NodeSubDef & def)
{
    def.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeBuiltInSubDef & def)
{
    def.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeFuncDef & def)
{
    def.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeTaskDef & def)
{
    def.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeBuiltInFunc &) {}
void CodeAnalyzer::Traverse(NodeConst &) {}
void CodeAnalyzer::Traverse(NodeLocal & stmt)
{
    stmt.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeLoop & stmt)
{
    stmt.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeTimes & stmt)
{
    stmt.cnt->Traverse(*this);
    stmt.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeWhile & stmt)
{
    stmt.cond->Traverse(*this);
    stmt.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeAscent & stmt)
{
    stmt.range->Traverse(*this);
    stmt.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeDescent & stmt)
{
    stmt.range->Traverse(*this);
    stmt.block->Traverse(*this);
}

void CodeAnalyzer::Traverse(NodeElseIf& elsif)
{
    elsif.cond->Traverse(*this);
    elsif.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeIf& stmt)
{
    stmt.cond->Traverse(*this);
    stmt.thenBlock->Traverse(*this);
    for (auto& elsif : stmt.elsifs) elsif->Traverse(*this);
    if (stmt.elseBlock) stmt.elseBlock->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeCase& c)
{
    for (auto& exp : c.exps)
    {
        exp->Traverse(*this);
    }
    c.block->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeAlternative& stmt)
{
    stmt.cond->Traverse(*this);
    for (auto& c : stmt.cases) c->Traverse(*this);
    if (stmt.others) stmt.others->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeCallStmt & call)
{
    AnalyzeDef(call.name);
    for (auto& arg : call.args)
    {
        arg->Traverse(*this);
    }
}
void CodeAnalyzer::Traverse(NodeReturn & stmt)
{
    stmt.ret->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeReturnVoid &) {}
void CodeAnalyzer::Traverse(NodeYield &) {}
void CodeAnalyzer::Traverse(NodeBreak &) {}
void CodeAnalyzer::Traverse(NodeSucc& stmt)
{
    stmt.lhs->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodePred& stmt)
{
    stmt.lhs->Traverse(*this);
}
void CodeAnalyzer::Traverse(NodeVarDecl& def)
{
    def.noSubEffect = true;
}

void CodeAnalyzer::Traverse(NodeVarInit& stmt)
{
    AnalyzeDef(stmt.name);
    stmt.rhs->Traverse(*this);
    stmt.noSubEffect = false;
    if (auto varDecl = std::dynamic_pointer_cast<NodeVarDecl>(env_->FindDef(stmt.name)))
    {
        varDecl->assignCnt++;
    }
}
void CodeAnalyzer::Traverse(NodeProcParam & def)
{
    def.noSubEffect = true;
}
void CodeAnalyzer::Traverse(NodeLoopParam & def)
{
    def.noSubEffect = true;
}

void CodeAnalyzer::Traverse(NodeResult & def)
{
    def.noSubEffect = true;
}

void CodeAnalyzer::Traverse(NodeHeader &) {}

void CodeAnalyzer::AnalyzeDef(const std::string& name)
{
    auto defEnv = env_;

    while (defEnv && defEnv->GetCurrentBlockNameTable()->count(name) == 0)
    {
        defEnv = defEnv->GetParent();
    }

    if (defEnv)
    {
        auto& def = (*(defEnv->GetCurrentBlockNameTable()))[name];
        if (def->unreachable)
        {
            auto prevEnv = env_;
            env_ = defEnv;
            def->unreachable = false;
            def->Traverse(*this);
            env_ = prevEnv;
        }
    }
}
void CodeAnalyzer::AnalyzeMonoOp(NodeMonoOp & exp)
{
    exp.rhs->Traverse(*this);
    exp.noSubEffect = exp.rhs->noSubEffect;
}
void CodeAnalyzer::AnalyzeBinOp(NodeBinOp & exp)
{
    exp.lhs->Traverse(*this);
    exp.rhs->Traverse(*this);
    exp.noSubEffect = exp.lhs->noSubEffect && exp.rhs->noSubEffect;
}
void CodeAnalyzer::AnalyzeArithAndArrayBinOp(NodeBinOp & exp)
{
    AnalyzeBinOp(exp);
    if (exp.lhs->expType == NodeExp::T_REAL && exp.rhs->expType == NodeExp::T_REAL)
    {
        exp.expType = NodeExp::T_REAL;
    }
}
void CodeAnalyzer::AnalyzeArithBinOp(NodeBinOp & exp)
{
    AnalyzeBinOp(exp);
    exp.expType = NodeExp::T_REAL;
}
void CodeAnalyzer::AnalyzeCmpBinOp(NodeBinOp & exp)
{
    AnalyzeBinOp(exp);
    exp.expType = NodeExp::T_BOOL;
}
void CodeAnalyzer::AnalyzeAssign(NodeAssign & stmt)
{
    stmt.lhs->Traverse(*this);
    stmt.rhs->Traverse(*this);
    stmt.noSubEffect = false;
}
}