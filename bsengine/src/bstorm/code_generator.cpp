#include <bstorm/code_generator.hpp>

#include <bstorm/script_name_prefix.hpp>
#include <bstorm/string_util.hpp>
#include <bstorm/file_util.hpp>

namespace bstorm
{
static std::string runtime(const std::string& name)
{
    return bstorm::DNH_RUNTIME_PREFIX + name;
}

static std::string builtin(const std::shared_ptr<NodeDef>& def)
{
    return bstorm::DNH_BUILTIN_FUNC_PREFIX + def->convertedName;
}

static std::string varname(const std::shared_ptr<NodeDef>& def)
{
    return bstorm::DNH_VAR_PREFIX + def->convertedName;
}

static std::string varname(const std::string& name, const std::shared_ptr<Env>& env)
{
    auto def = env->FindDef(name);
    return bstorm::DNH_VAR_PREFIX + def->convertedName;
}

static bool isDeclarationNeeded(const std::shared_ptr<NodeDef>& def)
{
    if (std::dynamic_pointer_cast<NodeVarDecl>(def)) return true;
    if (std::dynamic_pointer_cast<NodeFuncDef>(def)) return true;
    if (std::dynamic_pointer_cast<NodeSubDef>(def)) return true;
    if (std::dynamic_pointer_cast<NodeBuiltInSubDef>(def)) return true;
    if (std::dynamic_pointer_cast<NodeTaskDef>(def)) return true;
    return false;
}

CodeGenerator::CodeGenerator() :
    indentLevel_(0),
    outputLine_(1),
    isLineHead_(true)
{
}

void CodeGenerator::Generate(bool embedLocalVarName, Node & n)
{
    code_.clear();
    embedLocalVarName_ = embedLocalVarName;
    n.Traverse(*this);
}

void CodeGenerator::Traverse(NodeNum& exp)
{
    AddCode(exp.number);
}

void CodeGenerator::Traverse(NodeChar& exp)
{
    AddCode("[=[" + ToUTF8(std::wstring{ exp.c }) + "]=]");
    if (exp.c == L'\r' || exp.c == L'\n') outputLine_++;
}

void CodeGenerator::Traverse(NodeStr& exp)
{
    AddCode("{");
    for (int i = 0; i < exp.str.size(); i++)
    {
        if (i != 0) AddCode(",");
        NodeChar(exp.str[i]).Traverse(*this);
    }
    AddCode("}");
}
void CodeGenerator::Traverse(NodeArray& exp)
{
    AddCode("{");
    for (int i = 0; i < exp.elems.size(); i++)
    {
        if (i != 0) AddCode(",");
        exp.elems[i]->Traverse(*this);
    }
    AddCode("}");
}
void CodeGenerator::Traverse(NodeNeg& exp) { GenMonoOp("neg", exp); }
void CodeGenerator::Traverse(NodeNot& exp) { GenMonoOp("not", exp); }
void CodeGenerator::Traverse(NodeAbs& exp) { GenMonoOp("abs", exp); }

void CodeGenerator::Traverse(NodeAdd& exp) { GenBinOp("add", exp); }
void CodeGenerator::Traverse(NodeSub& exp) { GenBinOp("sub", exp); }
void CodeGenerator::Traverse(NodeMul& exp) { GenBinOp("mul", exp); }
void CodeGenerator::Traverse(NodeDiv& exp) { GenBinOp("div", exp); }
void CodeGenerator::Traverse(NodeRem& exp) { GenBinOp("rem", exp); }
void CodeGenerator::Traverse(NodePow& exp) { GenBinOp("pow", exp); }

void CodeGenerator::Traverse(NodeLt& exp) { GenBinOp("lt", exp); }
void CodeGenerator::Traverse(NodeGt& exp) { GenBinOp("gt", exp); }
void CodeGenerator::Traverse(NodeLe& exp) { GenBinOp("le", exp); }
void CodeGenerator::Traverse(NodeGe& exp) { GenBinOp("ge", exp); }
void CodeGenerator::Traverse(NodeEq& exp) { GenBinOp("eq", exp); }
void CodeGenerator::Traverse(NodeNe& exp) { GenBinOp("ne", exp); }

void CodeGenerator::Traverse(NodeAnd& exp) { GenLogBinOp("and", exp); }
void CodeGenerator::Traverse(NodeOr& exp) { GenLogBinOp("or", exp); }
void CodeGenerator::Traverse(NodeCat& exp)
{
    if (IsCopyNeeded(exp.lhs))
    {
        GenBinOp("cat", exp);
    } else
    {
        GenBinOp("mcat", exp);
    }
}
void CodeGenerator::Traverse(NodeNoParenCallExp& call)
{
    auto def = env_->FindDef(call.name);
    if (std::dynamic_pointer_cast<NodeVarDecl>(def) ||
        std::dynamic_pointer_cast<NodeProcParam>(def) ||
        std::dynamic_pointer_cast<NodeLoopParam>(def))
    {
        GenCheckNil(call.name);
    } else if (std::dynamic_pointer_cast<NodeBuiltInFunc>(def))
    {
        if (call.name == "GetCurrentScriptDirectory")
        {
            NodeStr(GetParentPath(*call.srcPos->filename) + L"/").Traverse(*this);
        } else
        {
            AddCode(builtin(def) + "()");
        }
    } else if (auto c = std::dynamic_pointer_cast<NodeConst>(def))
    {
        AddCode(c->value);
#ifdef _DEBUG
        AddCode(" --[[ " + call.name + " ]]");
#endif
} else
    {
        AddCode(varname(def) + "()");
    }
}

void CodeGenerator::Traverse(NodeCallExp& call)
{
    auto def = env_->FindDef(call.name);
    if (call.name == "GetCurrentScriptDirectory" && std::dynamic_pointer_cast<NodeBuiltInFunc>(def))
    {
        NodeStr(GetParentPath(*call.srcPos->filename) + L"/").Traverse(*this);
    } else if (auto c = std::dynamic_pointer_cast<NodeConst>(def))
    {
        AddCode(c->value);
#ifdef _DEBUG
        AddCode(" --[[ " + call.name + " ]]");
#endif
} else
    {
        bool isUserFunc = !std::dynamic_pointer_cast<NodeBuiltInFunc>(def);
        if (isUserFunc)
        {
            AddCode(varname(def) + "(");
        } else
        {
            AddCode(builtin(def) + "(");
        }
        for (int i = 0; i < call.args.size(); i++)
        {
            if (i != 0) AddCode(",");
            if (isUserFunc)
            {
                GenCopy(call.args[i]);
            } else
            {
                call.args[i]->Traverse(*this);
            }
        }
        AddCode(")");
    }
}

void CodeGenerator::Traverse(NodeHeader &) {}

void CodeGenerator::AddCode(const std::wstring & s)
{
    AddCode(ToUTF8(s));
}
void CodeGenerator::AddCode(const std::string& s)
{
#ifdef _DEBUG
    constexpr char* tab = "  ";
    if (isLineHead_ && !s.empty())
    {
        for (int i = 0; i < indentLevel_; i++) { code_ += tab; }
        isLineHead_ = false;
    }
#endif
    code_ += s;
}
void CodeGenerator::NewLine()
{
    outputLine_++;
    AddCode("\n");
    isLineHead_ = true;
}
void CodeGenerator::NewLine(const std::shared_ptr<SourcePos>& srcPos)
{
    srcMap_.LogSourcePos(outputLine_, srcPos->filename, srcPos->line);
    NewLine();
}
void CodeGenerator::Indent()
{
    indentLevel_++;
}
void CodeGenerator::Unindent()
{
    indentLevel_--;
}
void CodeGenerator::GenMonoOp(const std::string& fname, NodeMonoOp& exp)
{
    AddCode(runtime(fname) + "(");
    exp.rhs->Traverse(*this);
    AddCode(")");
}
void CodeGenerator::GenBinOp(const std::string& fname, NodeBinOp& exp)
{
    AddCode(runtime(fname) + "(");
    exp.lhs->Traverse(*this);
    AddCode(",");
    exp.rhs->Traverse(*this);
    AddCode(")");
}
void CodeGenerator::GenLogBinOp(const std::string & fname, NodeBinOp & exp)
{
    AddCode(runtime(fname) + "(");
    exp.lhs->Traverse(*this);
    AddCode(", function() return ");
    exp.rhs->Traverse(*this);
    AddCode(" end");
    AddCode(")");
}
void CodeGenerator::GenCheckNil(const std::string & name)
{
    if (embedLocalVarName_)
    {
        AddCode(runtime("checknil") + "(" + varname(name, env_) + ", \"" + name + "\")");
    } else
    {
        AddCode(runtime("checknil") + "(" + varname(name, env_) + ")");
    }
}
void CodeGenerator::GenProc(std::shared_ptr<NodeDef> def, const std::vector<std::string>& params_, NodeBlock & blk)
{
    AddCode(varname(def) + " = function(");
    for (int i = 0; i < params_.size(); i++)
    {
        if (i != 0) AddCode(",");
        AddCode(varname(params_[i], blk.env));
    }
    AddCode(")"); NewLine();
    blk.Traverse(*this);
    if (std::dynamic_pointer_cast<NodeFuncDef>(def))
    {
        auto result = blk.env->GetCurrentBlockNameTable().at("result");
        if (!std::dynamic_pointer_cast<NodeProcParam>(result))
        {
            Indent();
            AddCode("do return " + varname(result) + " end");
            NewLine(result->srcPos);
            Unindent();
        }
    }
    AddCode("end"); NewLine();
}
void CodeGenerator::GenOpAssign(const std::string & fname, const std::shared_ptr<NodeLeftVal>& left, std::shared_ptr<NodeExp> right)
{
    switch (left->indices.size())
    {
        case 0:
            // a += e;
            // out : a = add(r_checknil(a), e);
            AddCode(varname(left->name, env_) + " = ");
            AddCode(runtime(fname) + "(");
            GenCheckNil(left->name);
            if (right)
            {
                AddCode(",");
                right->Traverse(*this);
            }
            AddCode(");"); NewLine(left->srcPos);
            break;
        case 1:
            // a[_i] += e;
            // out :  r_checknil(a);
            //        local i = _i;
            //        r_write1(a, i, r_add(r_read(a, i), e));
            AddCode("do"); NewLine();
            GenCheckNil(left->name); NewLine(left->srcPos);
            AddCode("local i = "); left->indices[0]->Traverse(*this); AddCode(";"); NewLine(left->srcPos);
            AddCode(runtime("write1") + "(" + varname(left->name, env_) + ", i, ");
            AddCode(runtime(fname) + "(");
            AddCode(runtime("read") + "(" + varname(left->name, env_) + ", i)");
            if (right)
            {
                AddCode(",");
                right->Traverse(*this);
            }
            AddCode("));"); NewLine(left->srcPos);
            AddCode("end"); NewLine();
            break;
        default:
            // a[i][j]..[z] += e;
            // out : check_nil(a);
            //       local is = {i, j, .. , z};
            //       r_write(check_nil(a), is, r_add(r_read(..r_read(a, is[1]), is[2]), .. is[n]), e);
            AddCode("do"); NewLine();
            GenCheckNil(left->name); NewLine(left->srcPos);
            AddCode("local is = {");
            for (int i = 0; i < left->indices.size(); i++)
            {
                if (i != 0) AddCode(",");
                left->indices[i]->Traverse(*this);
            }
            AddCode("};"); NewLine(left->srcPos);
            AddCode(runtime("write") + "(" + varname(left->name, env_) + ", is");
            AddCode("," + runtime(fname) + "(");
            for (int i = 0; i < left->indices.size(); i++)
            {
                AddCode(runtime("read") + "(");
            }
            AddCode(varname(left->name, env_) + ",");
            for (int i = 0; i < left->indices.size(); i++)
            {
                if (i != 0) AddCode(",");
                AddCode("is[" + std::to_string(i + 1) + "])");
            }
            if (right)
            {
                AddCode(",");
                right->Traverse(*this);
            }
            AddCode("));"); NewLine(left->srcPos);
            AddCode("end"); NewLine();
            break;
    }
}
void CodeGenerator::GenCopy(std::shared_ptr<NodeExp> exp)
{
    if (IsCopyNeeded(exp))
    {
        AddCode(runtime("cp") + "("); exp->Traverse(*this); AddCode(")");
    } else
    {
        exp->Traverse(*this);
    }
}
bool CodeGenerator::IsCopyNeeded(const std::shared_ptr<NodeExp>& exp)
{
    if (std::dynamic_pointer_cast<NodeNum>(exp) ||
        std::dynamic_pointer_cast<NodeChar>(exp) ||
        std::dynamic_pointer_cast<NodeStr>(exp) ||
        std::dynamic_pointer_cast<NodeMonoOp>(exp) ||
        std::dynamic_pointer_cast<NodeArrayRef>(exp) ||
        std::dynamic_pointer_cast<NodeArraySlice>(exp))
    {
        return false;
    } else if (auto arr = std::dynamic_pointer_cast<NodeArray>(exp))
    {
        for (auto& elem : arr->elems)
        {
            if (IsCopyNeeded(elem)) return true;
        }
        return false;
    } else if (auto op = std::dynamic_pointer_cast<NodeAnd>(exp))
    {
        return IsCopyNeeded(op->lhs) || IsCopyNeeded(op->rhs);
    } else if (auto op = std::dynamic_pointer_cast<NodeOr>(exp))
    {
        return IsCopyNeeded(op->lhs) || IsCopyNeeded(op->rhs);
    } else if (std::dynamic_pointer_cast<NodeBinOp>(exp))
    {
        return false;
    } else if (auto call = std::dynamic_pointer_cast<NodeNoParenCallExp>(exp))
    {
        auto def = env_->FindDef(call->name);
        if (std::dynamic_pointer_cast<NodeBuiltInFunc>(def) ||
            std::dynamic_pointer_cast<NodeConst>(def))
        {
            return false;
        }
        return true;
    } else if (auto call = std::dynamic_pointer_cast<NodeCallExp>(exp))
    {
        auto def = env_->FindDef(call->name);
        if (std::dynamic_pointer_cast<NodeBuiltInFunc>(def) ||
            std::dynamic_pointer_cast<NodeConst>(def))
        {
            return false;
        }
        return true;
    }
    return true;
}
void CodeGenerator::Traverse(NodeArrayRef& exp)
{
    AddCode(runtime("read") + "(");
    exp.array->Traverse(*this);
    AddCode(",");
    exp.idx->Traverse(*this);
    AddCode(")");
}
void CodeGenerator::Traverse(NodeRange &) {}
void CodeGenerator::Traverse(NodeArraySlice& exp)
{
    AddCode(runtime("slice") + "(");
    exp.array->Traverse(*this);
    AddCode(",");
    exp.range->start->Traverse(*this);
    AddCode(",");
    exp.range->end->Traverse(*this);
    AddCode(")");
}
void CodeGenerator::Traverse(NodeNop &) {}
void CodeGenerator::Traverse(NodeLeftVal &) {}
void CodeGenerator::Traverse(NodeAssign& stmt)
{
    auto def = env_->FindDef(stmt.lhs->name);
    switch (stmt.lhs->indices.size())
    {
        case 0:
            // a = e;
            // out : a = r_cp(e);
            AddCode(varname(def) + " = "); GenCopy(stmt.rhs); AddCode(";");
            break;
        case 1:
            // a[i] = e;
            // out : r_write1(r_checknil(a), i, e);
            AddCode(runtime("write1") + "(");
            GenCheckNil(def->name);
            AddCode(",");
            stmt.lhs->indices[0]->Traverse(*this);
            AddCode(",");
            stmt.rhs->Traverse(*this);
            AddCode(");");
            break;
        default:
            // a[i1][i2] .. [in] = e;
            // out : r_write(r_checknil(a), {i1, i2, .. in}, e);
            AddCode(runtime("write") + "(");
            GenCheckNil(def->name);
            AddCode(", {");
            for (int i = 0; i < stmt.lhs->indices.size(); i++)
            {
                if (i != 0) AddCode(",");
                stmt.lhs->indices[i]->Traverse(*this);
            }
            AddCode("}, ");
            stmt.rhs->Traverse(*this);
            AddCode(");");
            break;
    }
    NewLine(stmt.srcPos);
}
void CodeGenerator::Traverse(NodeAddAssign& stmt) { GenOpAssign("add", stmt.lhs, stmt.rhs); }
void CodeGenerator::Traverse(NodeSubAssign& stmt) { GenOpAssign("sub", stmt.lhs, stmt.rhs); }
void CodeGenerator::Traverse(NodeMulAssign& stmt) { GenOpAssign("mul", stmt.lhs, stmt.rhs); }
void CodeGenerator::Traverse(NodeDivAssign& stmt) { GenOpAssign("div", stmt.lhs, stmt.rhs); }
void CodeGenerator::Traverse(NodeRemAssign& stmt) { GenOpAssign("rem", stmt.lhs, stmt.rhs); }
void CodeGenerator::Traverse(NodePowAssign& stmt) { GenOpAssign("pow", stmt.lhs, stmt.rhs); }
void CodeGenerator::Traverse(NodeCatAssign& stmt) { GenOpAssign("mcat", stmt.lhs, stmt.rhs); }

void CodeGenerator::Traverse(NodeCallStmt& call)
{
    auto def = env_->FindDef(call.name);

    if (std::dynamic_pointer_cast<NodeConst>(def)) return;

    bool isTask = (bool)std::dynamic_pointer_cast<NodeTaskDef>(def);
    bool isUserFunc = !std::dynamic_pointer_cast<NodeBuiltInFunc>(def);
    if (isTask)
    {
        AddCode(runtime("fork") + "(" + varname(def));
        if (call.args.size() != 0)
        {
            AddCode(", {");
        }
    } else
    {
        if (isUserFunc)
        {
            AddCode(varname(def) + "(");
        } else
        {
            AddCode(builtin(def) + "(");
        }
    }
    for (int i = 0; i < call.args.size(); i++)
    {
        if (i != 0) AddCode(",");
        if (isUserFunc)
        {
            GenCopy(call.args[i]);
        } else
        {
            call.args[i]->Traverse(*this);
        }
    }
    if (isTask && call.args.size() != 0)
    {
        AddCode("}");
    }
    AddCode(");"); NewLine(call.srcPos);
}
void CodeGenerator::Traverse(NodeReturn& stmt)
{
    AddCode("do return "); stmt.ret->Traverse(*this); AddCode(" end");
    NewLine(stmt.srcPos);
}
void CodeGenerator::Traverse(NodeReturnVoid& stmt)
{
    if (procStack_.empty())
    {
        // top level return
        AddCode("do return end");
    } else
    {
        if (auto func = std::dynamic_pointer_cast<NodeFuncDef>(procStack_.top()))
        {
            auto result = func->block->env->GetCurrentBlockNameTable().at("result");
            if (auto funcParam = std::dynamic_pointer_cast<NodeProcParam>(result))
            {
                // function has "result" param
                AddCode("do return end");
            } else
            {
                // return result
                AddCode("do return " + varname(result) + " end");
            }
        } else
        {
            // return from sub or task
            AddCode("do return end");
        }
    }
    NewLine(stmt.srcPos);
}
void CodeGenerator::Traverse(NodeYield& stmt)
{
    if (!procStack_.empty())
    {
        AddCode("coroutine.yield();");
        NewLine(stmt.srcPos);
    }
}
void CodeGenerator::Traverse(NodeBreak& stmt)
{
    if (procStack_.empty())
    {
        AddCode("do return end");
    } else
    {
        AddCode("do break end");
    }
    NewLine(stmt.srcPos);
}

void CodeGenerator::Traverse(NodeSucc& stmt) { GenOpAssign("succ", stmt.lhs, std::shared_ptr<NodeExp>()); }
void CodeGenerator::Traverse(NodePred& stmt) { GenOpAssign("pred", stmt.lhs, std::shared_ptr<NodeExp>()); }

void CodeGenerator::Traverse(NodeVarDecl &) {}
void CodeGenerator::Traverse(NodeVarInit& stmt)
{
    AddCode(varname(stmt.name, env_) + " = "); GenCopy(stmt.rhs); AddCode(";");
    NewLine(stmt.rhs->srcPos);
}
void CodeGenerator::Traverse(NodeProcParam &) {}
void CodeGenerator::Traverse(NodeLoopParam& param)
{
    AddCode("local " + varname(param.name, env_) + " = " + runtime("cp") + "(i);");
    NewLine(param.srcPos);
}
void CodeGenerator::Traverse(NodeBuiltInFunc &) {}
void CodeGenerator::Traverse(NodeConst &) {}
void CodeGenerator::Traverse(NodeLocal& stmt)
{
    AddCode("do"); NewLine();
    stmt.block->Traverse(*this);
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeLoop& stmt)
{
    AddCode("while 1 do"); NewLine();
    stmt.block->Traverse(*this);
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeTimes& stmt)
{
    AddCode("do"); NewLine();
    AddCode("local e = " + runtime("ceil") + "(");  stmt.cnt->Traverse(*this); AddCode(");");
    NewLine(stmt.cnt->srcPos);
    AddCode("for i = 0, e - 1 do"); NewLine();
    stmt.block->Traverse(*this);
    AddCode("end"); NewLine();
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeWhile& stmt)
{
    AddCode("while " + runtime("tobool") + "("); stmt.cond->Traverse(*this); AddCode(") do"); NewLine(stmt.cond->srcPos);
    stmt.block->Traverse(*this);
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeAscent& stmt)
{
    AddCode("do"); NewLine();
    AddCode("local e = ");  stmt.range->end->Traverse(*this); AddCode(";"); NewLine(stmt.range->end->srcPos);
    AddCode("for i = "); stmt.range->start->Traverse(*this); AddCode(", e - 1 do"); NewLine(stmt.srcPos);
    stmt.block->Traverse(*this);
    AddCode("end"); NewLine();
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeDescent& stmt)
{
    AddCode("do"); NewLine();
    AddCode("local s = ");  stmt.range->start->Traverse(*this); AddCode(";"); NewLine(stmt.range->start->srcPos);
    AddCode("for i = "); stmt.range->end->Traverse(*this); AddCode(", s + 1, -1 do"); NewLine(stmt.srcPos);
    stmt.block->Traverse(*this);
    AddCode("end"); NewLine();
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeElseIf& elsif)
{
    AddCode("elseif " + runtime("tobool") + "("); elsif.cond->Traverse(*this); AddCode(") then"); NewLine(elsif.cond->srcPos);
    elsif.block->Traverse(*this);
}
void CodeGenerator::Traverse(NodeIf& stmt)
{
    AddCode("if " + runtime("tobool") + "("); stmt.cond->Traverse(*this); AddCode(") then"); NewLine(stmt.cond->srcPos);
    stmt.thenBlock->Traverse(*this);
    for (auto elsif : stmt.elsifs) { elsif->Traverse(*this); }
    if (stmt.elseBlock)
    {
        AddCode("else"); NewLine();
        stmt.elseBlock->Traverse(*this);
    }
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeCase &) {}
void CodeGenerator::Traverse(NodeAlternative& stmt)
{
    AddCode("do"); NewLine();
    AddCode("local c = "); GenCopy(stmt.cond); AddCode(";"); NewLine(stmt.cond->srcPos);
    /* gen if-seq */
    for (int i = 0; i < stmt.cases.size(); i++)
    {
        // case
        if (i != 0) AddCode("else"); // 1つ目以降のcaseはelse if
        AddCode("if false");
        // orの右結合計算列を作る(右結合の方が早く短絡するので)
        for (auto& exp : stmt.cases[i]->exps)
        {
            AddCode(" or (");
            AddCode("(" + runtime("eq") + "(c,"); exp->Traverse(*this); AddCode("))");
        }
        for (auto& exp : stmt.cases[i]->exps)
        {
            AddCode(")");
        }
        AddCode(" then"); NewLine(stmt.cases[i]->srcPos);
        stmt.cases[i]->block->Traverse(*this);
    }
    /* gen else*/
    if (stmt.others)
    {
        AddCode(stmt.cases.empty() ? "do" : "else"); NewLine();
        stmt.others->Traverse(*this);
        AddCode("end"); NewLine();
    } else
    {
        if (!stmt.cases.empty())
        {
            AddCode("end"); NewLine();
        }
    }
    AddCode("end"); NewLine();
}
void CodeGenerator::Traverse(NodeBlock& blk)
{
    auto prevEnv = env_;
    env_ = blk.env;

    if (!env_->IsRoot())
    {
        Indent();
        // local declare
        for (const auto& bind : blk.env->GetCurrentBlockNameTable())
        {
            auto def = bind.second;
            if (isDeclarationNeeded(def))
            {
                AddCode("local "); AddCode(varname(def)); AddCode(";"); NewLine(def->srcPos);
            }
        }
    }

    for (const auto& bind : blk.env->GetCurrentBlockNameTable())
    {
        bind.second->Traverse(*this);
    }

    for (auto& stmt : blk.stmts)
    {
        stmt->Traverse(*this);
    }

    if (!env_->IsRoot())
    {
        Unindent();
    }
    env_ = prevEnv;
}
void CodeGenerator::Traverse(NodeSubDef& sub)
{
    std::shared_ptr<NodeSubDef> def = std::make_shared<NodeSubDef>(sub);
    procStack_.push(def);
    GenProc(def, {}, *sub.block);
    procStack_.pop();
}
void CodeGenerator::Traverse(NodeBuiltInSubDef& sub)
{
    std::shared_ptr<NodeBuiltInSubDef> def = std::make_shared<NodeBuiltInSubDef>(sub);
    procStack_.push(def);
    GenProc(def, {}, *sub.block);
    procStack_.pop();
}
void CodeGenerator::Traverse(NodeFuncDef& func)
{
    std::shared_ptr<NodeFuncDef> def = std::make_shared<NodeFuncDef>(func);
    procStack_.push(def);
    GenProc(def, func.params_, *func.block);
    procStack_.pop();
}
void CodeGenerator::Traverse(NodeTaskDef& task)
{
    std::shared_ptr<NodeTaskDef> def = std::make_shared<NodeTaskDef>(task);
    procStack_.push(def);
    GenProc(def, task.params_, *task.block);
    procStack_.pop();
}
}