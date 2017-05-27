// Created by Yunming Zhang on 2/9/17.
//

#ifndef GRAPHIT_MIR_EMITTER_H
#define GRAPHIT_MIR_EMITTER_H

#include <cassert>

#include <graphit/frontend/fir.h>
#include <graphit/frontend/fir_visitor.h>
#include <graphit/midend/mir.h>
#include <graphit/midend/mir_visitor.h>
#include <graphit/midend/mir_context.h>
#include <graphit/midend/var.h>
#include <vector>


namespace graphit {


        class MIREmitter : public fir::FIRVisitor {
        public:
            MIREmitter(MIRContext* ctx) : ctx(ctx)  {}
            ~MIREmitter()  {}

            void emitIR(fir::Program::Ptr program) {
                program->accept(this);
            }

            virtual void visit(fir::ElementTypeDecl::Ptr);
            virtual void visit(fir::ConstDecl::Ptr);
            virtual void visit(fir::VarDecl::Ptr);

            virtual void visit(fir::ForStmt::Ptr);
            virtual void visit(fir::RangeDomain::Ptr);
            virtual void visit(fir::WhileStmt::Ptr);


            virtual void visit(fir::ExprStmt::Ptr);
            virtual void visit(fir::AssignStmt::Ptr);
            virtual void visit(fir::PrintStmt::Ptr);

            virtual void visit(fir::StmtBlock::Ptr);
            virtual void visit(fir::IdentDecl::Ptr);

            virtual void visit(fir::FuncDecl::Ptr);

            virtual void visit(fir::VertexSetAllocExpr::Ptr);

            virtual void visit(fir::TensorReadExpr::Ptr);

            virtual void visit(fir::CallExpr::Ptr);
            virtual void visit(fir::MethodCallExpr::Ptr);

            virtual void visit(fir::ApplyExpr::Ptr);
            //virtual void visit(fir::FromExpr::Ptr);
            //virtual void visit(fir::ToExpr::Ptr);

            virtual void visit(fir::WhereExpr::Ptr);

            virtual void visit(fir::EdgeSetLoadExpr::Ptr);
            virtual void visit(fir::VarExpr::Ptr);

            virtual void visit(fir::NegExpr::Ptr);
            virtual void visit(fir::EqExpr::Ptr);


            virtual void visit(fir::MulExpr::Ptr);
            virtual void visit(fir::DivExpr::Ptr);
            virtual void visit(fir::AddExpr::Ptr);
            virtual void visit(fir::SubExpr::Ptr);

            virtual void visit(fir::BoolLiteral::Ptr);
            virtual void visit(fir::StringLiteral::Ptr);
            virtual void visit(fir::FloatLiteral::Ptr);
            virtual void visit(fir::IntLiteral::Ptr);

            virtual void visit(fir::ScalarType::Ptr);
            virtual void visit(fir::ElementType::Ptr);
            virtual void visit(fir::VertexSetType::Ptr);
            virtual void visit(fir::EdgeSetType::Ptr);
            virtual void visit(fir::NDTensorType::Ptr);


            MIRContext *ctx;

            mir::Expr::Ptr retExpr;
            mir::Stmt::Ptr retStmt;
            mir::Type::Ptr retType;
            mir::Var  retVar;
            mir::ForDomain::Ptr retForDomain;

        private:
            //helper methods for the visitor pattern to return MIR nodes
            mir::Expr::Ptr     emitExpr(fir::Expr::Ptr);
            mir::Stmt::Ptr     emitStmt(fir::Stmt::Ptr);
            mir::Type::Ptr     emitType(fir::Type::Ptr);
            mir::Var           emitVar (fir::IdentDecl::Ptr);
            mir::ForDomain::Ptr     emitDomain(fir::ForDomain::Ptr ptr);

            void addVarOrConst(fir::VarDecl::Ptr var_decl, bool is_const);
            void addElementType(mir::ElementType::Ptr);

        };

}

#endif //GRAPHIT_MIR_EMITTER_H

