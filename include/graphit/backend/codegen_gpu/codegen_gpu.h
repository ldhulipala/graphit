
#ifndef GRAPHIT_CODEGEN_GPU_H
#define GRAPHIT_CODEGEN_GPU_H

#include <graphit/midend/mir.h>
#include <graphit/midend/mir_visitor.h>
#include <graphit/midend/mir_context.h>
#include <iostream>
#include <sstream>
#include <graphit/backend/gen_edge_apply_func_decl.h>
#include <unordered_set>

namespace graphit {
class CodeGenGPUKernelEmitter: public mir::MIRVisitor {
public:
	CodeGenGPUKernelEmitter(std::ostream &input_oss, MIRContext *mir_context):
		oss(input_oss), mir_context_(mir_context), indentLevel(0) {
		}
	void indent() { ++indentLevel; }
	void dedent() { --indentLevel; }
	void printIndent() { oss << std::string(indentLevel, '\t'); }

	std::ostream &oss;
	unsigned      indentLevel;

	MIRContext * mir_context_;

	void visit(mir::PushEdgeSetApplyExpr::Ptr);
	void visit(mir::PullEdgeSetApplyExpr::Ptr);

	void genEdgeSetGlobalKernel(mir::EdgeSetApplyExpr::Ptr);
	void genFuncDecl(mir::FuncDecl::Ptr);

};

class CodeGenGPU : public mir::MIRVisitor{
public:
	CodeGenGPU(std::ostream &input_oss, MIRContext *mir_context, std::string module_name_, std::string module_path):
		oss(input_oss), mir_context_(mir_context), module_name(module_name_) {
			indentLevel = 0;
			edgeset_apply_func_gen_ = new EdgesetApplyFunctionDeclGenerator(mir_context_, oss);
		}

	int genGPU();

protected:

	void indent() { ++indentLevel; }
	void dedent() { --indentLevel; }
	void printIndent() { oss << std::string(indentLevel, '\t'); }

	std::ostream &oss;
	std::string module_name;
	unsigned      indentLevel;
	MIRContext * mir_context_;

private:
	void genIncludeStmts(void);
	void genEdgeSets(void);


	void genPropertyArrayImplementationWithInitialization(mir::VarDecl::Ptr shared_ptr);


	void genPropertyArrayDecl(mir::VarDecl::Ptr);
	void genPropertyArrayAlloca(mir::VarDecl::Ptr);
	
	void genFusedWhileLoop(mir::WhileStmt::Ptr);
	void genEdgeSetApplyExpr(mir::EdgeSetApplyExpr::Ptr, mir::Expr::Ptr);

	EdgesetApplyFunctionDeclGenerator* edgeset_apply_func_gen_;

	virtual std::string getBackendFunctionLabel(void) {
		return "__device__";
	}
protected:
	std::vector<mir::Var> kernel_hoisted_vars;
	std::string current_kernel_name;
	bool is_hoisted_var (mir::Var var) {
		for (auto h_var: kernel_hoisted_vars)
			if (h_var.getName() == var.getName())
				return true;
		return false;
	}
	

	void generateBinaryExpr(mir::BinaryExpr::Ptr, std::string);
protected:
	virtual void visit(mir::EdgeSetType::Ptr) override;
	virtual void visit(mir::VertexSetType::Ptr) override;
	virtual void visit(mir::ScalarType::Ptr) override;
	virtual void visit(mir::FuncDecl::Ptr) override;
	virtual void visit(mir::ElementType::Ptr) override;
	virtual void visit(mir::ExprStmt::Ptr) override;
	virtual void visit(mir::VarExpr::Ptr) override;
	virtual void visit(mir::AssignStmt::Ptr) override;

	virtual void visit(mir::AddExpr::Ptr) override;
	virtual void visit(mir::MulExpr::Ptr) override;
	virtual void visit(mir::DivExpr::Ptr) override;
	virtual void visit(mir::SubExpr::Ptr) override;
	virtual void visit(mir::EqExpr::Ptr) override;
	virtual void visit(mir::NegExpr::Ptr) override;

	virtual void visit(mir::TensorArrayReadExpr::Ptr) override;
	virtual void visit(mir::IntLiteral::Ptr) override;
	virtual void visit(mir::BoolLiteral::Ptr) override;
	virtual void visit(mir::StringLiteral::Ptr) override;



	virtual void visit(mir::ReduceStmt::Ptr) override;
	virtual void visit(mir::CompareAndSwapStmt::Ptr) override;
	virtual void visit(mir::VarDecl::Ptr) override;

	virtual void visit(mir::ForStmt::Ptr) override;
	virtual void visit(mir::WhileStmt::Ptr) override;
	virtual void visit(mir::IfStmt::Ptr) override;
	virtual void visit(mir::PrintStmt::Ptr) override;
	virtual void visit(mir::Call::Ptr) override;	

	virtual void visit(mir::BreakStmt::Ptr) override;

	virtual void visit(mir::VertexSetApplyExpr::Ptr) override;
	virtual void visit(mir::VertexSetAllocExpr::Ptr) override;

	virtual void visit(mir::VertexSetDedupExpr::Ptr) override;
	virtual void visit(mir::HybridGPUStmt::Ptr) override;


};
class CodeGenGPUHost: public CodeGenGPU {
public:
	using CodeGenGPU::CodeGenGPU;
	using CodeGenGPU::visit;
private:
	virtual std::string getBackendFunctionLabel(void) {
		return "__host__";
	}
	virtual void visit(mir::TensorArrayReadExpr::Ptr) override;
	virtual void visit(mir::StmtBlock::Ptr) override;

	virtual void visit(mir::Call::Ptr) override;	
	virtual void visit(mir::PrintStmt::Ptr) override;



	void generateDeviceToHostCopy(mir::TensorArrayReadExpr::Ptr tare);
	void generateHostToDeviceCopy(mir::TensorArrayReadExpr::Ptr tare);
};


class CodeGenGPUFusedKernel: public CodeGenGPU {
public:
	using CodeGenGPU::CodeGenGPU;
	using CodeGenGPU::visit;
	void genEdgeSetApplyExpr(mir::EdgeSetApplyExpr::Ptr, mir::Expr::Ptr);
	virtual void visit(mir::StmtBlock::Ptr) override;
	virtual void visit(mir::AssignStmt::Ptr) override;
	virtual void visit(mir::VarDecl::Ptr) override;
	virtual void visit(mir::PrintStmt::Ptr) override;
	
	std::string var_name (std::string var) {
		//return current_kernel_name + "_" + var;
		return "__local_" + var;
	}
};

class KernelVariableExtractor: public mir::MIRVisitor {
public:
	using mir::MIRVisitor::visit;
	std::vector<mir::Var> hoisted_vars; 
	std::vector<mir::VarDecl::Ptr> hoisted_decls;

	void insertVar(mir::Var var_to_insert) {
		for (auto var: hoisted_vars)
			if (var.getName() == var_to_insert.getName())
				return;
		hoisted_vars.push_back(var_to_insert);
	}
	void insertDecl(mir::VarDecl::Ptr decl_to_insert) {
		hoisted_decls.push_back(decl_to_insert);
	}

	virtual void visit(mir::VarExpr::Ptr);
	virtual void visit(mir::VarDecl::Ptr);
};

}
#endif
