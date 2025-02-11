//
// Created by Yunming Zhang on 1/24/17.
//

#ifndef GRAPHIT_FIR_VISITOR_H
#define GRAPHIT_FIR_VISITOR_H


#include <memory>
#include <graphit/midend/label_scope.h>

// Visitor pattern abstract class
namespace graphit {
    namespace fir {

        struct Program;
        struct StmtBlock;
        struct RangeIndexSet;
        struct SetIndexSet;
        struct GenericIndexSet;
        struct DynamicIndexSet;
        struct ElementType;
        struct Endpoint;
        struct UnstructuredSetType;
        struct HomogeneousEdgeSetType;
        struct HeterogeneousEdgeSetType;
        struct GridSetType;
        struct TupleElement;
        struct NamedTupleType;
        struct TupleLength;
        struct UnnamedTupleType;
        struct ScalarType;
        struct NDTensorType;
        struct OpaqueType;
        struct Identifier;
        struct IdentDecl;
        struct FieldDecl;
        struct ElementTypeDecl;
        struct Argument;
        struct InOutArgument;
        struct ExternDecl;
        struct GenericParam;
        struct FuncDecl;
        struct VarDecl;
        struct ConstDecl;
        struct WhileStmt;
        struct DoWhileStmt;
        struct IfStmt;
        struct IndexSetDomain;
        struct RangeDomain;
        struct ForStmt;
        struct PrintStmt;
        struct NameNode;

        struct ExprStmt;
        struct AssignStmt;
        struct Slice;
        struct ExprParam;
        struct MapExpr;
        struct ReducedMapExpr;
        struct UnreducedMapExpr;
        struct UnaryExpr;
        struct BinaryExpr;
        struct NaryExpr;
        struct OrExpr;
        struct AndExpr;
        struct XorExpr;
        struct EqExpr;
        struct NotExpr;
        struct AddExpr;
        struct SubExpr;
        struct MulExpr;
        struct DivExpr;
        struct LeftDivExpr;
        struct ElwiseMulExpr;
        struct ElwiseDivExpr;
        struct NegExpr;
        struct ExpExpr;
        struct TransposeExpr;
        struct CallExpr;
        struct TensorReadExpr;
        struct SetReadExpr;
        struct NamedTupleReadExpr;
        struct UnnamedTupleReadExpr;
        struct FieldReadExpr;
        struct ParenExpr;
        struct VarExpr;
        struct RangeConst;
        struct IntLiteral;
        struct FloatLiteral;
        struct BoolLiteral;
        struct StringLiteral;
        struct IntVectorLiteral;
        struct FloatVectorLiteral;
        struct NDTensorLiteral;
        struct ApplyStmt;
        struct Test;

        struct VertexSetType;
        struct EdgeSetType;
        struct ListType;

        struct IntersectionExpr;

        struct EdgeSetLoadExpr;

        // Expression that allocates a new vertexset (new vertexset(node_id));
        struct VertexSetAllocExpr;

        // Experession that allocates the new list
        struct ListAllocExpr;

        // Experession that allocates a new vector
        struct VectorAllocExpr;

        struct MethodCallExpr;
        struct ApplyExpr;
        struct WhereExpr;
        struct FromExpr;
        struct ToExpr;
        struct BreakStmt;

        struct ReduceStmt;

	// OG Additions
        struct PriorityQueueType;
        struct PriorityQueueAllocExpr;


        struct FIRVisitor {
            virtual void visit(std::shared_ptr<Program>);

            virtual void visit(std::shared_ptr<StmtBlock>);

            virtual void visit(std::shared_ptr<RangeIndexSet> op) {}

            virtual void visit(std::shared_ptr<SetIndexSet> op) {}

            virtual void visit(std::shared_ptr<GenericIndexSet>);

            virtual void visit(std::shared_ptr<DynamicIndexSet> op) {}

            virtual void visit(std::shared_ptr<ElementType> op) {}

            virtual void visit(std::shared_ptr<Endpoint>);

            virtual void visit(std::shared_ptr<HomogeneousEdgeSetType>);

            virtual void visit(std::shared_ptr<HeterogeneousEdgeSetType>);

            virtual void visit(std::shared_ptr<GridSetType>);

            virtual void visit(std::shared_ptr<TupleElement>);

            virtual void visit(std::shared_ptr<NamedTupleType>);

            virtual void visit(std::shared_ptr<TupleLength> op) {}

            virtual void visit(std::shared_ptr<UnnamedTupleType>);

            virtual void visit(std::shared_ptr<ScalarType> op) {}

            virtual void visit(std::shared_ptr<NDTensorType>);

            virtual void visit(std::shared_ptr<ListType>);


            virtual void visit(std::shared_ptr<OpaqueType>) {}

            virtual void visit(std::shared_ptr<Identifier> op) {}

            virtual void visit(std::shared_ptr<IdentDecl>);

            virtual void visit(std::shared_ptr<FieldDecl>);

            virtual void visit(std::shared_ptr<ElementTypeDecl>);

            virtual void visit(std::shared_ptr<Argument>);

            virtual void visit(std::shared_ptr<InOutArgument>);

            virtual void visit(std::shared_ptr<ExternDecl>);

            virtual void visit(std::shared_ptr<GenericParam> op) {}

            virtual void visit(std::shared_ptr<FuncDecl>);

            virtual void visit(std::shared_ptr<VarDecl>);

            virtual void visit(std::shared_ptr<ConstDecl>);

            virtual void visit(std::shared_ptr<WhileStmt>);

            virtual void visit(std::shared_ptr<DoWhileStmt>);

            virtual void visit(std::shared_ptr<IfStmt>);

            virtual void visit(std::shared_ptr<IndexSetDomain>);

            virtual void visit(std::shared_ptr<RangeDomain>);

            virtual void visit(std::shared_ptr<ForStmt>);

            virtual void visit(std::shared_ptr<NameNode>);


            virtual void visit(std::shared_ptr<PrintStmt>);

            virtual void visit(std::shared_ptr<BreakStmt>) {}


            virtual void visit(std::shared_ptr<ExprStmt>);

            virtual void visit(std::shared_ptr<AssignStmt>);

            virtual void visit(std::shared_ptr<ReduceStmt>);


            virtual void visit(std::shared_ptr<Slice> op) {}

            virtual void visit(std::shared_ptr<ExprParam>);

            virtual void visit(std::shared_ptr<MapExpr>);

            virtual void visit(std::shared_ptr<ReducedMapExpr>);

            virtual void visit(std::shared_ptr<UnreducedMapExpr>);

            virtual void visit(std::shared_ptr<OrExpr>);

            virtual void visit(std::shared_ptr<AndExpr>);

            virtual void visit(std::shared_ptr<XorExpr>);

            virtual void visit(std::shared_ptr<EqExpr>);

            virtual void visit(std::shared_ptr<NotExpr>);

            virtual void visit(std::shared_ptr<AddExpr>);

            virtual void visit(std::shared_ptr<SubExpr>);

            virtual void visit(std::shared_ptr<MulExpr>);

            virtual void visit(std::shared_ptr<DivExpr>);

            virtual void visit(std::shared_ptr<LeftDivExpr>);

            virtual void visit(std::shared_ptr<ElwiseMulExpr>);

            virtual void visit(std::shared_ptr<ElwiseDivExpr>);

            virtual void visit(std::shared_ptr<NegExpr>);

            virtual void visit(std::shared_ptr<ExpExpr>);

            virtual void visit(std::shared_ptr<TransposeExpr>);

            virtual void visit(std::shared_ptr<CallExpr>);

            virtual void visit(std::shared_ptr<TensorReadExpr>);

            virtual void visit(std::shared_ptr<SetReadExpr>);

            virtual void visit(std::shared_ptr<NamedTupleReadExpr>);

            virtual void visit(std::shared_ptr<UnnamedTupleReadExpr>);

            virtual void visit(std::shared_ptr<FieldReadExpr>);

            virtual void visit(std::shared_ptr<ParenExpr>);

            virtual void visit(std::shared_ptr<VarExpr> op) {}

            virtual void visit(std::shared_ptr<RangeConst>);

            virtual void visit(std::shared_ptr<IntLiteral> op) {}

            virtual void visit(std::shared_ptr<FloatLiteral> op) {}

            virtual void visit(std::shared_ptr<BoolLiteral> op) {}

            virtual void visit(std::shared_ptr<StringLiteral> op) {}

            virtual void visit(std::shared_ptr<IntVectorLiteral> op) {}

            virtual void visit(std::shared_ptr<FloatVectorLiteral> op) {}

            virtual void visit(std::shared_ptr<NDTensorLiteral>);

            virtual void visit(std::shared_ptr<ApplyStmt>);

            virtual void visit(std::shared_ptr<Test>);


            //GraphIt additions

            virtual void visit(std::shared_ptr<VertexSetType>){};
            virtual void visit(std::shared_ptr<EdgeSetType>){};
            virtual void visit(std::shared_ptr<VertexSetAllocExpr>);

            virtual void visit(std::shared_ptr<ListAllocExpr>);
            virtual void visit(std::shared_ptr<VectorAllocExpr>);

            virtual void visit(std::shared_ptr<IntersectionExpr>);
            virtual void visit(std::shared_ptr<EdgeSetLoadExpr>);

            virtual void visit(std::shared_ptr<MethodCallExpr>);
            virtual void visit(std::shared_ptr<ApplyExpr>);
            virtual void visit(std::shared_ptr<WhereExpr>);
            virtual void visit(std::shared_ptr<FromExpr>);
            virtual void visit(std::shared_ptr<ToExpr>);
	
            // OG Additions

	    virtual void visit(std::shared_ptr<PriorityQueueType>){};
            virtual void visit(std::shared_ptr<PriorityQueueAllocExpr>);


        protected:
            LabelScope label_scope_;


        private:
            void visitUnaryExpr(std::shared_ptr<UnaryExpr>);

            void visitBinaryExpr(std::shared_ptr<BinaryExpr>);

            void visitNaryExpr(std::shared_ptr<NaryExpr>);



        };

    }

}
#endif //GRAPHIT_FIR_VISITOR_H
