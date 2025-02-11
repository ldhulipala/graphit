//
// Created by Yunming Zhang on 2/9/17.
//

#include <graphit/frontend/fir_printer.h>


namespace graphit {
    namespace fir {

        void FIRPrinter::visit(Program::Ptr program) {
            for (auto elem : program->elems) {
                elem->accept(this);
                oss << std::endl;
            }
        }

        void FIRPrinter::visit(StmtBlock::Ptr stmtBlock) {
            for (auto stmt : stmtBlock->stmts) {
                stmt->accept(this);
                oss << std::endl;
            }
        }

        void FIRPrinter::visit(RangeIndexSet::Ptr set) {
            oss << "0:" << set->range;
        }

        void FIRPrinter::visit(SetIndexSet::Ptr set) {
            oss << set->setName;
        }

        void FIRPrinter::visit(DynamicIndexSet::Ptr set) {
            oss << "*";
        }

        void FIRPrinter::visit(ElementType::Ptr type) {
            oss << type->ident;
        }

        void FIRPrinter::visit(Endpoint::Ptr end) {
            end->set->accept(this);
        }

        void FIRPrinter::visit(HomogeneousEdgeSetType::Ptr type) {
            oss << "set{";
            type->element->accept(this);
            oss << "}(";
            type->endpoint->accept(this);
            oss << " * ";
            type->arity->accept(this);
            oss << ")";
        }

        void FIRPrinter::visit(HeterogeneousEdgeSetType::Ptr type) {
            oss << "set{";
            type->element->accept(this);
            oss << "}";

            if (!type->endpoints.empty()) {
                oss << "(";

                bool printDelimiter = false;
                for (auto endpoint : type->endpoints) {
                    if (printDelimiter) {
                        oss << ",";
                    }

                    endpoint->accept(this);
                    printDelimiter = true;
                }

                oss << ")";
            }
        }

        void FIRPrinter::visit(GridSetType::Ptr type) {
            oss << "grid{";
            type->element->accept(this);
            oss << "}[" << type->dimensions << "](";
            type->underlyingPointSet->accept(this);
            oss << ")";
        }

        void FIRPrinter::visit(TupleElement::Ptr elem) {
            if (elem->name) {
                elem->name->accept(this);
                oss << " : ";
            }
            elem->element->accept(this);
        }

        void FIRPrinter::visit(NamedTupleType::Ptr type) {
            oss << "(";

            bool printDelimiter = false;
            for (auto elem : type->elems) {
                if (printDelimiter) {
                    oss << ", ";
                }

                elem->accept(this);
                printDelimiter = true;
            }

            oss << ")";
        }

        void FIRPrinter::visit(TupleLength::Ptr length) {
            oss << length->val;
        }

        void FIRPrinter::visit(UnnamedTupleType::Ptr type) {
            oss << "(";
            type->element->accept(this);
            oss << " * ";
            type->length->accept(this);
            oss << ")";
        }

        void FIRPrinter::visit(ScalarType::Ptr type) {
            switch (type->type) {
                case ScalarType::Type::INT:
                    oss << "int";
                    break;
                case ScalarType::Type::UINT:
                    oss << "uint";
                    break;
                case ScalarType::Type::FLOAT:
                    oss << "float";
                    break;
                case ScalarType::Type::DOUBLE:
                    oss << "double";
                    break;
                case ScalarType::Type::BOOL:
                    oss << "bool";
                    break;
                case ScalarType::Type::COMPLEX:
                    oss << "complex";
                    break;
                case ScalarType::Type::STRING:
                    oss << "string";
                    break;
                default:
                    //unreachable;
                    break;
            }
        }

        void FIRPrinter::visit(NDTensorType::Ptr type) {
            oss << "tensor[";

            bool printDelimiter = false;
            for (auto indexSet : type->indexSets) {
                if (printDelimiter) {
                    oss << ",";
                }

                indexSet->accept(this);
                printDelimiter = true;
            }

            oss << "]";

            if (type->element != nullptr){
                oss << "{";
                type->element->accept(this);
                oss << "}";
            }

            oss << "(";
            type->blockType->accept(this);
            oss << ")";

            if (type->transposed) {
                oss << "'";
            }
        }

        void FIRPrinter::visit(OpaqueType::Ptr ident) {
            oss << "opaque";
        }

        void FIRPrinter::visit(Identifier::Ptr ident) {
            oss << ident->ident;
        }

        void FIRPrinter::visit(IdentDecl::Ptr decl) {
            printIdentDecl(decl);
        }

        void FIRPrinter::visit(FieldDecl::Ptr decl) {
            printIdentDecl(decl);
            oss << ";";
        }

        void FIRPrinter::visit(ElementTypeDecl::Ptr decl) {
            oss << "element ";
            decl->name->accept(this);
            oss << std::endl;

            indent();
            for (auto field : decl->fields) {
                printIndent();
                field->accept(this);
                oss << std::endl;
            }
            dedent();

            oss << "end";
        }

        void FIRPrinter::visit(Argument::Ptr arg) {
            if (arg->isInOut()) {
                oss << "inout ";
            }
            printIdentDecl(arg);
        }

        void FIRPrinter::visit(ExternDecl::Ptr decl) {
            oss << "extern ";
            printIdentDecl(decl);
            oss << ";";
        }

        void FIRPrinter::visit(GenericParam::Ptr param) {
            if (param->type == GenericParam::Type::RANGE) {
                oss << "0:";
            }
            oss << param->name;
        }

        void FIRPrinter::visit(FuncDecl::Ptr decl) {
            switch (decl->type) {
                case FuncDecl::Type::EXPORTED:
                    oss << "export ";
                    break;
                case FuncDecl::Type::EXTERNAL:
                    oss << "extern ";
                    break;
                default:
                    break;
            }

            oss << "func ";
            decl->name->accept(this);

            if (!decl->genericParams.empty()) {
                oss << "<";

                bool printDelimiter = false;
                for (auto genericParam : decl->genericParams) {
                    if (printDelimiter) {
                        oss << ", ";
                    }

                    genericParam->accept(this);
                    printDelimiter = true;
                }

                oss << ">";
            }

            oss << "(";

            bool printDelimiter = false;
            for (auto arg : decl->args) {
                if (printDelimiter) {
                    oss << ", ";
                }

                arg->accept(this);
                printDelimiter = true;
            }

            oss << ") ";

            if (!decl->results.empty()) {
                oss << "-> (";

                bool printDelimiter = false;
                for (auto result : decl->results) {
                    if (printDelimiter) {
                        oss << ", ";
                    }

                    result->accept(this);
                    printDelimiter = true;
                }

                oss << ")";
            }

            if (decl->body) {
                oss << std::endl;

                indent();
                decl->body->accept(this);
                dedent();

                oss << "end";
            } else {
                oss << ";";
            }
        }

        void FIRPrinter::visit(VarDecl::Ptr decl) {
            printVarOrConstDecl(decl);
        }

        void FIRPrinter::visit(ConstDecl::Ptr decl) {
            printVarOrConstDecl(decl, true);
        }

        void FIRPrinter::visit(WhileStmt::Ptr stmt) {
            printIndent();
            if (stmt->stmt_label != "")
                oss << " # " << stmt->stmt_label << " # ";
            oss << "while ";
            stmt->cond->accept(this);
            oss << std::endl;

            indent();
            stmt->body->accept(this);
            dedent();

            printIndent();
            oss << "end";
        }

        void FIRPrinter::visit(DoWhileStmt::Ptr stmt) {
            printIndent();
            oss << "do " << std::endl;

            indent();
            stmt->body->accept(this);
            dedent();

            printIndent();
            oss << "end while ";
            stmt->cond->accept(this);
        }

        void FIRPrinter::visit(IfStmt::Ptr stmt) {
            printIndent();
            oss << "if ";
            stmt->cond->accept(this);
            oss << std::endl;

            indent();
            stmt->ifBody->accept(this);
            dedent();

            if (stmt->elseBody) {
                printIndent();
                oss << "else" << std::endl;

                indent();
                stmt->elseBody->accept(this);
                dedent();

                oss << std::endl;
            }

            printIndent();
            oss << "end";
        }

        void FIRPrinter::visit(IndexSetDomain::Ptr domain) {
            domain->set->accept(this);
        }

        void FIRPrinter::visit(RangeDomain::Ptr domain) {
            domain->lower->accept(this);
            oss << " : ";
            domain->upper->accept(this);
        }

        void FIRPrinter::visit(ForStmt::Ptr stmt) {
            printIndent();

            if (stmt->stmt_label != "")
                oss << " # " << stmt->stmt_label << " # ";

            oss << "for ";
            stmt->loopVar->accept(this);
            oss << " in ";
            stmt->domain->accept(this);
            oss << std::endl;

            indent();
            stmt->body->accept(this);
            dedent();

            printIndent();
            oss << "end";
        }

        void FIRPrinter::visit(PrintStmt::Ptr stmt) {
            printIndent();

            if (stmt->stmt_label != "")
                oss << " # " << stmt->stmt_label << " # ";

            oss << (stmt->printNewline ? "println " : "print ");

            bool printDelimiter = false;
            for (auto arg : stmt->args) {
                if (printDelimiter) {
                    oss << ", ";
                }

                arg->accept(this);
                printDelimiter = true;
            }

            oss << ";";
        }

        void FIRPrinter::visit(BreakStmt::Ptr stmt) {
            printIndent();
            oss << " break;";
        }

        void FIRPrinter::visit(ExprStmt::Ptr stmt) {
            printIndent();
            if (stmt->stmt_label != "")
                oss << " # " << stmt->stmt_label << " # ";
            if (stmt->stmt_label != "") {
                oss << stmt->stmt_label << ": ";
            }
            stmt->expr->accept(this);
            oss << ";";
        }

        void FIRPrinter::visit(AssignStmt::Ptr stmt) {
            printIndent();
            if (stmt->stmt_label != "")
                oss << " # " << stmt->stmt_label << " # ";
            bool printDelimiter = false;
            for (auto lhs : stmt->lhs) {
                if (printDelimiter) {
                    oss << ", ";
                }

                lhs->accept(this);
                printDelimiter = true;
            }

            oss << " = ";
            stmt->expr->accept(this);
            oss << ";";
        }

        void FIRPrinter::visit(ReduceStmt::Ptr stmt) {
            printIndent();
            if (stmt->stmt_label != "")
                oss << " # " << stmt->stmt_label << " # ";
            bool printDelimiter = false;
            for (auto lhs : stmt->lhs) {
                if (printDelimiter) {
                    oss << ", ";
                }

                lhs->accept(this);
                printDelimiter = true;
            }

            switch (stmt->reduction_op) {
                case ReduceStmt::ReductionOp::SUM:
                    oss << " += ";
                    break;
                case ReduceStmt::ReductionOp::MAX:
                    oss << " max= ";
                    break;
                case ReduceStmt::ReductionOp::MIN:
                    oss << " min= ";
                    break;
                case ReduceStmt::ReductionOp::ASYNC_MAX:
                    oss << " asyncMax= ";
                    break;
                case ReduceStmt::ReductionOp::ASYNC_MIN:
                    oss << " asyncMin= ";
                    break;
            }
            stmt->expr->accept(this);
            oss << ";";
        }

        void FIRPrinter::visit(Slice::Ptr slice) {
            oss << ":";
        }

        void FIRPrinter::visit(ExprParam::Ptr param) {
            param->expr->accept(this);
        }

        void FIRPrinter::visit(MapExpr::Ptr expr) {
            printMapOrApply(expr);
        }

        void FIRPrinter::visit(OrExpr::Ptr expr) {
            printBinaryExpr(expr, "or");
        }

        void FIRPrinter::visit(AndExpr::Ptr expr) {
            printBinaryExpr(expr, "and");
        }

        void FIRPrinter::visit(XorExpr::Ptr expr) {
            printBinaryExpr(expr, "xor");
        }

        void FIRPrinter::visit(EqExpr::Ptr expr) {
            oss << "(";
            expr->operands[0]->accept(this);
            oss << ")";

            for (unsigned i = 0; i < expr->ops.size(); ++i) {
                switch (expr->ops[i]) {
                    case EqExpr::Op::LT:
                        oss << " < ";
                        break;
                    case EqExpr::Op::LE:
                        oss << " <= ";
                        break;
                    case EqExpr::Op::GT:
                        oss << " > ";
                        break;
                    case EqExpr::Op::GE:
                        oss << " >= ";
                        break;
                    case EqExpr::Op::EQ:
                        oss << " == ";
                        break;
                    case EqExpr::Op::NE:
                        oss << " != ";
                        break;
                    default:
                        unreachable;
                        break;
                }

                oss << "(";
                expr->operands[i + 1]->accept(this);
                oss << ")";
            }
        }

        void FIRPrinter::visit(NotExpr::Ptr expr) {
            printUnaryExpr(expr, "not");
        }

        void FIRPrinter::visit(AddExpr::Ptr expr) {
            printBinaryExpr(expr, "+");
        }

        void FIRPrinter::visit(SubExpr::Ptr expr) {
            printBinaryExpr(expr, "-");
        }

        void FIRPrinter::visit(MulExpr::Ptr expr) {
            printBinaryExpr(expr, "*");
        }

        void FIRPrinter::visit(DivExpr::Ptr expr) {
            printBinaryExpr(expr, "/");
        }

        void FIRPrinter::visit(LeftDivExpr::Ptr expr) {
            printBinaryExpr(expr, "\\");
        }

        void FIRPrinter::visit(ElwiseMulExpr::Ptr expr) {
            printBinaryExpr(expr, ".*");
        }

        void FIRPrinter::visit(ElwiseDivExpr::Ptr expr) {
            printBinaryExpr(expr, "./");
        }

        void FIRPrinter::visit(NegExpr::Ptr expr) {
            printUnaryExpr(expr, expr->negate ? "-" : "+");
        }

        void FIRPrinter::visit(ExpExpr::Ptr expr) {
            printBinaryExpr(expr, "^");
        }

        void FIRPrinter::visit(TransposeExpr::Ptr expr) {
            printUnaryExpr(expr, "'", true);
        }

        void FIRPrinter::visit(CallExpr::Ptr expr) {
            expr->func->accept(this);

            if (!expr->genericArgs.empty()) {
                oss << "<";

                bool printDelimiter = false;
                for (auto genericArg : expr->genericArgs) {
                    if (printDelimiter) {
                        oss << ", ";
                    }

                    genericArg->accept(this);
                    printDelimiter = true;
                }

                oss << ">";
            }

            oss << "(";

            bool printDelimiter = false;
            for (auto arg : expr->args) {
                if (printDelimiter) {
                    oss << ", ";
                }

                arg->accept(this);
                printDelimiter = true;
            }

            oss << ")";
        }

        void FIRPrinter::visit(TensorReadExpr::Ptr expr) {
            expr->tensor->accept(this);
            oss << "[";

            bool printDelimiter = false;
            for (auto param : expr->indices) {
                if (printDelimiter) {
                    oss << ", ";
                }

                param->accept(this);
                printDelimiter = true;
            }

            oss << "]";
        }

        void FIRPrinter::visit(SetReadExpr::Ptr expr) {
            expr->set->accept(this);
            oss << "[";

            bool printDelimiter = false;
            for (auto param : expr->indices) {
                if (printDelimiter) {
                    oss << ", ";
                }

                param->accept(this);
                printDelimiter = true;
            }

            oss << "]";
        }

        void FIRPrinter::visit(NamedTupleReadExpr::Ptr expr) {
            expr->tuple->accept(this);
            oss << ".";
            expr->elem->accept(this);
        }

        void FIRPrinter::visit(UnnamedTupleReadExpr::Ptr expr) {
            expr->tuple->accept(this);

            oss << "(";
            expr->index->accept(this);
            oss << ")";
        }

        void FIRPrinter::visit(FieldReadExpr::Ptr expr) {
            expr->setOrElem->accept(this);
            oss << ".";
            expr->field->accept(this);
        }

        void FIRPrinter::visit(VarExpr::Ptr lit) {
            oss << lit->ident;
        }

        void FIRPrinter::visit(IntLiteral::Ptr lit) {
            oss << lit->val;
        }

        void FIRPrinter::visit(StringLiteral::Ptr lit) {
            oss << lit->val;
        }

        void FIRPrinter::visit(FloatLiteral::Ptr lit) {
            oss << lit->val;
        }

        void FIRPrinter::visit(BoolLiteral::Ptr lit) {
            printBoolean(lit->val);
        }

        void FIRPrinter::visit(IntVectorLiteral::Ptr lit) {
            oss << "[";

            bool printDelimiter = false;
            for (auto val : lit->vals) {
                if (printDelimiter) {
                    oss << ", ";
                }

                oss << val;
                printDelimiter = true;
            }

            oss << "]";

            if (lit->transposed) {
                oss << "'";
            }
        }

        void FIRPrinter::visit(FloatVectorLiteral::Ptr lit) {
            oss << "[";

            bool printDelimiter = false;
            for (auto val : lit->vals) {
                if (printDelimiter) {
                    oss << ", ";
                }

                oss << val;
                printDelimiter = true;
            }

            oss << "]";

            if (lit->transposed) {
                oss << "'";
            }
        }

        void FIRPrinter::visit(NDTensorLiteral::Ptr lit) {
            oss << "[";

            bool printDelimiter = false;
            for (auto elem : lit->elems) {
                if (printDelimiter) {
                    oss << ", ";
                }

                elem->accept(this);
                printDelimiter = true;
            }

            oss << "]";

            if (lit->transposed) {
                oss << "'";
            }
        }

        void FIRPrinter::visit(ApplyStmt::Ptr stmt) {
            printMapOrApply(stmt->map, true);
            oss << ";";
        }

        void FIRPrinter::visit(Test::Ptr test) {
            oss << "%! ";
            test->func->accept(this);
            oss << "(";

            bool printDelimiter = false;
            for (auto arg : test->args) {
                if (printDelimiter) {
                    oss << ", ";
                }

                arg->accept(this);
                printDelimiter = true;
            }

            oss << ") == ";
            test->expected->accept(this);
            oss << ";";
        }

        void FIRPrinter::printIdentDecl(IdentDecl::Ptr decl) {
            decl->name->accept(this);
            oss << " : ";
            decl->type->accept(this);
        }

        void FIRPrinter::printVarOrConstDecl(VarDecl::Ptr decl, const bool isConst) {
            printIndent();
            oss << (isConst ? "const " : "var ");
            decl->name->accept(this);

            if (decl->type) {
                oss << " : ";
                decl->type->accept(this);
            }

            if (decl->initVal) {
                oss << " = ";
                decl->initVal->accept(this);
            }

            oss << ";";
        }

        void FIRPrinter::printMapOrApply(MapExpr::Ptr expr, const bool isApply) {
            oss << (isApply ? "apply " : "map ");
            expr->func->accept(this);

            if (!expr->partialActuals.empty()) {
                oss << "(";

                bool printDelimiter = false;
                for (auto param : expr->partialActuals) {
                    if (printDelimiter) {
                        oss << ", ";
                    }

                    param->accept(this);
                    printDelimiter = true;
                }

                oss << ")";
            }

            oss << " to ";
            expr->target->accept(this);

            if (expr->through) {
                oss << " through ";
                expr->through->accept(this);
            }

            if (expr->getReductionOp() != MapExpr::ReductionOp::NONE) {
                oss << " reduce ";

                switch (expr->getReductionOp()) {
                    case MapExpr::ReductionOp::SUM:
                        oss << "+";
                        break;
                    default:
                        unreachable;
                        break;
                }
            }
        }

        void FIRPrinter::printUnaryExpr(UnaryExpr::Ptr expr, const std::string op,
                                        const bool opAfterOperand) {
            if (!opAfterOperand) {
                oss << op;
            }

            oss << "(";
            expr->operand->accept(this);
            oss << ")";

            if (opAfterOperand) {
                oss << op;
            }
        }

        void FIRPrinter::printBinaryExpr(BinaryExpr::Ptr expr, const std::string op) {
            oss << "(";
            expr->lhs->accept(this);
            oss << ") " << op << " (";
            expr->rhs->accept(this);
            oss << ")";
        }

        std::ostream &operator<<(std::ostream &oss, FIRNode &node) {
            FIRPrinter printer(oss);
            node.accept(&printer);
            return oss;
        }


        void FIRPrinter::visit(VertexSetType::Ptr type) {
            oss << "vertexset{";
            type->element->accept(this);
            oss << "}";
        }

        void FIRPrinter::visit(ListType::Ptr type) {
            oss << "list{";
            type->list_element_type->accept(this);
            oss << "}";
        }
        void FIRPrinter::visit(VertexSetAllocExpr::Ptr expr) {
            oss << "alloc vertexset {";
            expr->elementType->accept(this);
            oss << "}(";
            if (expr->numElements != nullptr) {
                expr->numElements->accept(this);
            }
            oss << ")";
        }

        void FIRPrinter::visit(PriorityQueueType::Ptr type) {
            oss << "PriorityQueue {";
            type->element->accept(this);
            oss << "}(";
            type->priority_type->accept(this);
            oss << ")";
        }
        void FIRPrinter::visit(PriorityQueueAllocExpr::Ptr expr) {
            oss << "alloc PriorityQueue {";
            expr->elementType->accept(this);
            oss << "}(";
            expr->priority_type->accept(this);
            oss << ")";
        }


        void FIRPrinter::visit(ListAllocExpr::Ptr expr) {
            oss << "alloc list {";
            expr->general_element_type->accept(this);
            oss << "}(";
            if (expr->numElements != nullptr) {
                expr->numElements->accept(this);
            }
            oss << ")";
        }

        void FIRPrinter::visit(VectorAllocExpr::Ptr expr) {
            oss << "alloc vector {";
            expr->elementType->accept(this);
            oss << "}(";

            if (expr->vector_scalar_type != nullptr){
                expr->vector_scalar_type->accept(this);
            } else if (expr->general_element_type != nullptr) {
                expr->general_element_type->accept(this);
            } else {
                std::cout << "Error unsupported vector element type" << std::endl;
            }

            oss << ")";
        }

        void FIRPrinter::visit(MethodCallExpr::Ptr expr) {
            expr->target->accept(this);
            oss << ".";
            expr->method_name->accept(this);
            oss << "(";
            bool printDelimiter = false;
            for (auto arg : expr->args) {
                if (printDelimiter) {
                    oss << ", ";
                }
                arg->accept(this);
                printDelimiter = true;
            }
            oss << ")";
        }

        void FIRPrinter::visit(ApplyExpr::Ptr expr) {
            expr->target->accept(this);

            if (expr->type == ApplyExpr::Type::REGULAR_APPLY){
                oss << ".APPLY(";
            } else if (expr->change_tracking_field != nullptr){
                oss << ".APPLY_MODIFY(";
            } else if (expr->type == ApplyExpr::Type::UPDATE_PRIORITY_APPLY){
                oss << ".APPLY_MODIFY_PRIORITY_UPDATE(";
            } else {
                oss << ".APPLY_MODIFY_PRIORITY_EXTERN(";
            }


            expr->input_function->accept(this);
            oss << ")";

            if (expr->from_expr) {
                oss << " FROM: ";
                expr->from_expr->accept(this);
                oss << " ";
            }

            if (expr->to_expr) {
                oss << " TO: ";
                expr->to_expr->accept(this);
                oss << " ";
            }

        }

        void FIRPrinter::visit(WhereExpr::Ptr expr) {
            expr->target->accept(this);
            oss << ".where(";
            expr->input_func->accept(this);
            oss << ")";

        }

        void FIRPrinter::visit(IntersectionExpr::Ptr expr) {
            oss << "intersection(";
            expr->vertex_a->accept(this);
            oss << ", ";
            expr->vertex_b->accept(this);
            oss << ", ";
            expr->numA->accept(this);
            oss << ", ";
            expr->numB->accept(this);
            if (expr->reference != nullptr) {
                oss << ", ";
                expr->reference->accept(this);
            }
            oss << ") ";
        }

        void FIRPrinter::visit(EdgeSetLoadExpr::Ptr expr) {
            oss << "edgeset_load (";
            expr->file_name->accept(this);
            oss << ") ";
        }

        void FIRPrinter::visit(NameNode::Ptr name_node) {
            printIndent();

            if (name_node->stmt_label != "")
                oss << " # " << name_node->stmt_label << " # { " << std::endl;
            indent();
            name_node->body->accept(this);
            dedent();
            printIndent();
            oss << "} " << std::endl;
        }


    }
}