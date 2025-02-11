//
// Created by Yunming Zhang on 1/15/17.
//

#include <graphit/frontend/parser.h>


namespace graphit {

// Simit language grammar is documented here in EBNF. Note that '{}' is used
// here to denote zero or more instances of the enclosing term, while '[]' is
// used to denote zero or one instance of the enclosing term.

    Parser::Parser(std::vector<ParseError> *errors) : errors(errors) {}

    fir::Program::Ptr Parser::parse(const TokenStream &tokens) {
        this->tokens = tokens;

        decls = SymbolTable();

        initIntrinsics();



//        for (const auto kv : intrinsics) {
//            decls.insert(kv->name->ident, IdentType::FUNCTION);
//        }

        return parseProgram();
    }

// program: {program_element}
    fir::Program::Ptr Parser::parseProgram() {
        auto program = std::make_shared<fir::Program>();

        while (peek().type != Token::Type::END) {
            const fir::FIRNode::Ptr element = parseProgramElement();
            if (element) {
                program->elems.push_back(element);
            }
        }

        return program;
    }

// program_element: element_type_decl | extern_decl | const_decl
//                | func_decl | test
    fir::FIRNode::Ptr Parser::parseProgramElement() {
        try {
            switch (peek().type) {
                case Token::Type::TEST:
                    return parseTest();
                case Token::Type::EXPORT:
                case Token::Type::FUNC:
                    return parseFuncDecl();
                case Token::Type::ELEMENT:
                    return parseElementTypeDecl();
                case Token::Type::EXTERN:
                    return parseExternFuncOrDecl();
                case Token::Type::CONST:
                    return parseConstDecl();
                default:
                    reportError(peek(), "a program element type");
                    throw SyntaxError();
                    break;
            }
        } catch (const SyntaxError &) {
            skipTo({Token::Type::TEST, Token::Type::FUNC, Token::Type::EXPORT,
                    Token::Type::ELEMENT, Token::Type::EXTERN, Token::Type::CONST});

            return fir::FIRNode::Ptr();
        }
    }

// element_type_decl: 'element' ident field_decl_list 'end'
    fir::ElementTypeDecl::Ptr Parser::parseElementTypeDecl() {
        auto elemTypeDecl = std::make_shared<fir::ElementTypeDecl>();

        const Token elementToken = consume(Token::Type::ELEMENT);
        elemTypeDecl->setBeginLoc(elementToken);

        elemTypeDecl->name = parseIdent();
        elemTypeDecl->fields = parseFieldDeclList();

        const Token endToken = consume(Token::Type::BLOCKEND);
        elemTypeDecl->setEndLoc(endToken);

        return elemTypeDecl;
    }

// field_decl_list: {field_decl}
    std::vector<fir::FieldDecl::Ptr> Parser::parseFieldDeclList() {
        std::vector<fir::FieldDecl::Ptr> fields;

        while (peek().type == Token::Type::IDENT) {
            const fir::FieldDecl::Ptr field = parseFieldDecl();
            fields.push_back(field);
        }

        return fields;
    }

// field_decl: tensor_decl ';'
    fir::FieldDecl::Ptr Parser::parseFieldDecl() {
        auto fieldDecl = std::make_shared<fir::FieldDecl>();

        const auto tensorDecl = parseTensorDecl();
        fieldDecl->name = tensorDecl->name;
        fieldDecl->type = tensorDecl->type;

        const Token endToken = consume(Token::Type::SEMICOL);
        fieldDecl->setEndLoc(endToken);

        return fieldDecl;
    }

// extern_func_or_decl: extern_decl | extern_func_decl
    fir::FIRNode::Ptr Parser::parseExternFuncOrDecl() {
        auto tokenAfterExtern = peek(1);

        if (tokenAfterExtern.type == Token::Type::FUNC)
            return parseExternFuncDecl();
        else
            return parseExternDecl();
    }

// extern_decl: 'extern' ident_decl ';'
    fir::ExternDecl::Ptr Parser::parseExternDecl() {
        auto externDecl = std::make_shared<fir::ExternDecl>();

        const Token externToken = consume(Token::Type::EXTERN);
        externDecl->setBeginLoc(externToken);

        const auto identDecl = parseIdentDecl();
        externDecl->name = identDecl->name;
        externDecl->type = identDecl->type;

        const Token endToken = consume(Token::Type::SEMICOL);
        externDecl->setEndLoc(endToken);

        return externDecl;
    }


// extern_func_decl: 'extern' 'func' ident generic_params arguments results ';'
    fir::FuncDecl::Ptr Parser::parseExternFuncDecl() {
        const unsigned originalLevel = decls.levels();

        try {
            auto externFuncDecl = std::make_shared<fir::FuncDecl>();
            externFuncDecl->type = fir::FuncDecl::Type::EXTERNAL;

            const Token externToken = consume(Token::Type::EXTERN);
            externFuncDecl->setBeginLoc(externToken);

            consume(Token::Type::FUNC);
            externFuncDecl->name = parseIdent();

            decls.insert(externFuncDecl->name->ident, IdentType::FUNCTION);
            decls.scope();

            externFuncDecl->genericParams = parseGenericParams();
            externFuncDecl->args = parseArguments();
            externFuncDecl->results = parseResults();

            decls.unscope();

            const Token endToken = consume(Token::Type::SEMICOL);
            externFuncDecl->setEndLoc(endToken);

            return externFuncDecl;
        } catch (const SyntaxError &) {
            while (decls.levels() != originalLevel) {
                decls.unscope();
            }

            throw;
        }
    }

// func_decl: ['export'] 'func' ident generic_params
//            arguments results stmt_block 'end'
    fir::FuncDecl::Ptr Parser::parseFuncDecl() {
        const unsigned originalLevel = decls.levels();

        try {
            auto funcDecl = std::make_shared<fir::FuncDecl>();

            const Token funcToken = peek();
            funcDecl->setBeginLoc(funcToken);
            funcDecl->type = (funcToken.type == Token::Type::EXPORT) ?
                             fir::FuncDecl::Type::EXPORTED :
                             fir::FuncDecl::Type::INTERNAL;

            tryConsume(Token::Type::EXPORT);
            consume(Token::Type::FUNC);

            funcDecl->name = parseIdent();

            decls.insert(funcDecl->name->ident, IdentType::FUNCTION);
            decls.scope();

            funcDecl->genericParams = parseGenericParams();
            funcDecl->args = parseArguments();
            funcDecl->results = parseResults();

            decls.scope();
            funcDecl->body = parseStmtBlock();
            decls.unscope();

            decls.unscope();


            const Token endToken = consume(Token::Type::BLOCKEND);
            funcDecl->setEndLoc(endToken);

            return funcDecl;
        } catch (const SyntaxError &) {
            while (decls.levels() != originalLevel) {
                decls.unscope();
            }

            throw;
        }
    }

// generic_params: ['<' generic_param {',' generic_param} '>']
    std::vector<fir::GenericParam::Ptr> Parser::parseGenericParams() {
        std::vector<fir::GenericParam::Ptr> genericParams;

        if (tryConsume(Token::Type::LA)) {
            do {
                const fir::GenericParam::Ptr genericParam = parseGenericParam();
                genericParams.push_back(genericParam);
            } while (tryConsume(Token::Type::COMMA));
            consume(Token::Type::RA);
        }

        return genericParams;
    }

// generic_param: [INT_LITERAL(0) ':'] ident
    fir::GenericParam::Ptr Parser::parseGenericParam() {
        auto genericParam = std::make_shared<fir::GenericParam>();
        genericParam->type = fir::GenericParam::Type::UNKNOWN;

        if (peek().type == Token::Type::INT_LITERAL && peek().num == 0) {
            consume(Token::Type::INT_LITERAL);
            consume(Token::Type::COL);

            genericParam->type = fir::GenericParam::Type::RANGE;
        }

        const Token identToken = consume(Token::Type::IDENT);
        genericParam->setLoc(identToken);
        genericParam->name = identToken.str;

        const auto type = (genericParam->type == fir::GenericParam::Type::RANGE) ?
                          IdentType::RANGE_GENERIC_PARAM : IdentType::GENERIC_PARAM;
        decls.insert(genericParam->name, type);

        return genericParam;
    }

// arguments: '(' [argument_decl {',' argument_decl}] ')'
    std::vector<fir::Argument::Ptr> Parser::parseArguments() {
        std::vector<fir::Argument::Ptr> arguments;

        consume(Token::Type::LP);
        if (peek().type != Token::Type::RP) {
            do {
                const fir::Argument::Ptr argument = parseArgumentDecl();
                arguments.push_back(argument);
            } while (tryConsume(Token::Type::COMMA));
        }
        consume(Token::Type::RP);

        return arguments;
    }

// argument_decl: ['inout'] ident_decl
    fir::Argument::Ptr Parser::parseArgumentDecl() {
        auto argDecl = std::make_shared<fir::Argument>();

        if (peek().type == Token::Type::INOUT) {
            const Token inoutToken = consume(Token::Type::INOUT);

            argDecl = std::make_shared<fir::InOutArgument>();
            argDecl->setBeginLoc(inoutToken);
        }

        const auto identDecl = parseIdentDecl();
        argDecl->name = identDecl->name;
        argDecl->type = identDecl->type;

        return argDecl;
    }

// results: ['->' (ident_decl | ('(' ident_decl {',' ident_decl} ')')]
    std::vector<fir::IdentDecl::Ptr> Parser::parseResults() {
        std::vector<fir::IdentDecl::Ptr> results;

        if (tryConsume(Token::Type::RARROW)) {
            if (tryConsume(Token::Type::LP)) {
                do {
                    const fir::IdentDecl::Ptr result = parseIdentDecl();
                    results.push_back(result);
                } while (tryConsume(Token::Type::COMMA));
                consume(Token::Type::RP);
            } else {
                const fir::IdentDecl::Ptr result = parseIdentDecl();
                results.push_back(result);
            }
        }

        return results;
    }


// stmt_block: {stmt}
    fir::StmtBlock::Ptr Parser::parseStmtBlock() {
        auto stmtBlock = std::make_shared<fir::StmtBlock>();

        while (true) {
            switch (peek().type) {
                case Token::Type::BLOCKEND:
                case Token::Type::ELIF:
                case Token::Type::ELSE:
                case Token::Type::END:
                    return stmtBlock;
                default: {
                    const fir::Stmt::Ptr stmt = parseStmt();
                    if (stmt) {
                        stmtBlock->stmts.push_back(stmt);
                    }
                    break;
                }
            }
        }
    }

// stmt: var_decl | const_decl | if_stmt | while_stmt | do_while_stmt
//     | for_stmt | print_stmt | apply_stmt | expr_or_assign_stmt | break_stmt
//    | # identifier # stmt
    fir::Stmt::Ptr Parser::parseStmt() {
        switch (peek().type) {
            case Token::Type::VAR:
                return parseVarDecl();
            case Token::Type::CONST:
                return parseConstDecl();
            case Token::Type::IF:
                return parseIfStmt();
            case Token::Type::WHILE:
                return parseWhileStmt();
            case Token::Type::DO:
                return parseDoWhileStmt();
            case Token::Type::FOR:
                return parseForStmt();
            case Token::Type::PRINT:
            case Token::Type::PRINTLN:
                return parsePrintStmt();
//            case Token::Type::APPLY:
//                return parseApplyStmt();
            case Token::Type::BREAK:
                return parseBreakStmt();
            case Token::Type::NUMBER_SIGN: {
                consume(Token::Type::NUMBER_SIGN);
                fir::Identifier::Ptr label = parseIdent();
                consume(Token::Type::NUMBER_SIGN);
                fir::Stmt::Ptr stmt = parseStmt();
                stmt->stmt_label = label->ident;
                return stmt;
            }
            case Token::Type::DELETE:
                return parseDeleteStmt();
            default:
                return parseExprOrAssignStmt();
        }
    }

// DEPRECATED SIMIT grammar var_decl: 'var' ident (('=' expr) | (':' tensor_type ['=' expr])) ';'
// var_decl: 'var' ident (('=' expr) | (':' type ['=' expr])) ';'
// Similar to Const Declaration, we extend the grammar to support more than just tensor types

    fir::VarDecl::Ptr Parser::parseVarDecl() {
        try {
            auto varDecl = std::make_shared<fir::VarDecl>();

            const Token varToken = consume(Token::Type::VAR);
            varDecl->setBeginLoc(varToken);

            varDecl->name = parseIdent();
            if (tryConsume(Token::Type::COL)) {
                // Extend the grammar to support more than just tensor types
                //varDecl->type = parseTensorType();
                varDecl->type = parseType();
                if (tryConsume(Token::Type::ASSIGN)) {
                    varDecl->initVal = parseExpr();
                }
            } else {
                consume(Token::Type::ASSIGN);
                varDecl->initVal = parseExpr();
            }

            const Token endToken = consume(Token::Type::SEMICOL);
            varDecl->setEndLoc(endToken);

            decls.insert(varDecl->name->ident, IdentType::OTHER);

            return varDecl;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::SEMICOL});
            consume(Token::Type::SEMICOL);

            return fir::VarDecl::Ptr();
        }
    }

// DEPRECATED SIMIT Grammar: const_decl: 'const' ident [':' tensor_type | ] '=' expr ';
// const_decl: 'const' ident [':' type | ] '=' expr ';' GraphIt grammar to support more than tensor type

    fir::ConstDecl::Ptr Parser::parseConstDecl() {
        try {
            auto constDecl = std::make_shared<fir::ConstDecl>();

            const Token constToken = consume(Token::Type::CONST);
            constDecl->setBeginLoc(constToken);

            constDecl->name = parseIdent();
            if (tryConsume(Token::Type::COL)) {
                //constDecl->type = parseTensorType();
                // GraphIt wants to be able to support const SET types, more than just TensorTypes.
                // Needed for dynamic sets and loaders.
                constDecl->type = parseType();
            }

            //if an initial value is specified
            if (tryConsume(Token::Type::ASSIGN)) {
                //consume(Token::Type::ASSIGN);
                constDecl->initVal = parseExpr();
            }

            const Token endToken = consume(Token::Type::SEMICOL);
            constDecl->setEndLoc(endToken);

            decls.insert(constDecl->name->ident, IdentType::OTHER);

            return constDecl;

        } catch (const SyntaxError &) {
            skipTo({Token::Type::SEMICOL});
            consume(Token::Type::SEMICOL);

            return fir::ConstDecl::Ptr();
        }
    }

// ident_decl: ident ':' type
    fir::IdentDecl::Ptr Parser::parseIdentDecl() {
        auto identDecl = std::make_shared<fir::IdentDecl>();

        identDecl->name = parseIdent();
        consume(Token::Type::COL);
        identDecl->type = parseType();

        const auto type = fir::isa<fir::TupleType>(identDecl->type) ?
                          IdentType::TUPLE : IdentType::OTHER;
        decls.insert(identDecl->name->ident, type);

        return identDecl;
    }

// tensor_decl: ident ':' tensor_type
// This rule is needed to prohibit declaration of non-tensor variables and
// fields, which are currently unsupported. Probably want to replace with
// ident_decl rule at some point in the future.
    fir::IdentDecl::Ptr Parser::parseTensorDecl() {
        auto tensorDecl = std::make_shared<fir::IdentDecl>();

        tensorDecl->name = parseIdent();
        consume(Token::Type::COL);
        tensorDecl->type = parseTensorType();

        decls.insert(tensorDecl->name->ident, IdentType::OTHER);

        return tensorDecl;
    }

// while_stmt: 'while' expr stmt_block 'end'
    fir::WhileStmt::Ptr Parser::parseWhileStmt() {
        try {
            auto whileStmt = std::make_shared<fir::WhileStmt>();

            const Token whileToken = consume(Token::Type::WHILE);
            whileStmt->setBeginLoc(whileToken);

            whileStmt->cond = parseExpr();

            decls.scope();
            whileStmt->body = parseStmtBlock();
            decls.unscope();

            const Token endToken = consume(Token::Type::BLOCKEND);
            whileStmt->setEndLoc(endToken);

            return whileStmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::BLOCKEND});
            consume(Token::Type::BLOCKEND);

            return fir::WhileStmt::Ptr();
        }
    }

// do_while_stmt: 'do' stmt_block 'end' 'while' expr
    fir::DoWhileStmt::Ptr Parser::parseDoWhileStmt() {
        try {
            auto doWhileStmt = std::make_shared<fir::DoWhileStmt>();

            const Token doToken = consume(Token::Type::DO);
            doWhileStmt->setBeginLoc(doToken);

            decls.scope();
            doWhileStmt->body = parseStmtBlock();
            decls.unscope();

            consume(Token::Type::BLOCKEND);
            consume(Token::Type::WHILE);
            doWhileStmt->cond = parseExpr();

            return doWhileStmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::BLOCKEND, Token::Type::ELIF, Token::Type::ELSE,
                    Token::Type::END, Token::Type::VAR, Token::Type::CONST,
                    Token::Type::IF, Token::Type::WHILE, Token::Type::DO,
                    Token::Type::FOR, Token::Type::PRINT, Token::Type::PRINTLN});

            return fir::DoWhileStmt::Ptr();
        }
    }

// if_stmt: 'if' expr stmt_block else_clause 'end'
    fir::IfStmt::Ptr Parser::parseIfStmt() {
        try {
            auto ifStmt = std::make_shared<fir::IfStmt>();

            const Token ifToken = consume(Token::Type::IF);
            ifStmt->setBeginLoc(ifToken);

            ifStmt->cond = parseExpr();

            decls.scope();
            ifStmt->ifBody = parseStmtBlock();
            decls.unscope();

            decls.scope();
            ifStmt->elseBody = parseElseClause();
            decls.unscope();

            const Token endToken = consume(Token::Type::BLOCKEND);
            ifStmt->setEndLoc(endToken);

            return ifStmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::ELIF, Token::Type::ELSE, Token::Type::BLOCKEND});

            parseElseClause();
            consume(Token::Type::BLOCKEND);

            return fir::IfStmt::Ptr();
        }
    }

// else_clause: [('else' stmt_block) | ('elif' expr stmt_block else_clause)]
    fir::Stmt::Ptr Parser::parseElseClause() {
        switch (peek().type) {
            case Token::Type::ELSE:
                consume(Token::Type::ELSE);
                return parseStmtBlock();
            case Token::Type::ELIF: {
                try {
                    auto elifClause = std::make_shared<fir::IfStmt>();

                    const Token elifToken = consume(Token::Type::ELIF);
                    elifClause->setBeginLoc(elifToken);

                    elifClause->cond = parseExpr();

                    decls.scope();
                    elifClause->ifBody = parseStmtBlock();
                    decls.unscope();

                    decls.scope();
                    elifClause->elseBody = parseElseClause();
                    decls.unscope();

                    return elifClause;
                } catch (const SyntaxError &) {
                    skipTo({Token::Type::ELIF, Token::Type::ELSE, Token::Type::BLOCKEND});

                    decls.scope();
                    parseElseClause();
                    decls.unscope();

                    return fir::Stmt::Ptr();
                }
            }
            default:
                return fir::Stmt::Ptr();
        }
    }

// for_stmt: 'for' ident 'in' for_domain stmt_block 'end'
    fir::ForStmt::Ptr Parser::parseForStmt() {
        try {
            auto forStmt = std::make_shared<fir::ForStmt>();

            const Token forToken = consume(Token::Type::FOR);
            forStmt->setBeginLoc(forToken);

            forStmt->loopVar = parseIdent();
            consume(Token::Type::IN);
            forStmt->domain = parseForDomain();

            decls.scope();
            decls.insert(forStmt->loopVar->ident, IdentType::OTHER);

            forStmt->body = parseStmtBlock();
            decls.unscope();

            const Token endToken = consume(Token::Type::BLOCKEND);
            forStmt->setEndLoc(endToken);

            return forStmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::BLOCKEND});
            consume(Token::Type::BLOCKEND);

            return fir::ForStmt::Ptr();
        }
    }

// DEPRECATED: for_domain: set_index_set | (expr ':' expr)
    // for_domain: (expr ':' expr)
    fir::ForDomain::Ptr Parser::parseForDomain() {
//        GraphIt currently do not support for loop over set_index_set
//        if (peek().type == Token::Type::IDENT && peek(1).type != Token::Type::COL) {
//            auto indexSetDomain = std::make_shared<fir::IndexSetDomain>();
//            indexSetDomain->set = parseSetIndexSet();
//
//            return indexSetDomain;
//        }

        auto rangeDomain = std::make_shared<fir::RangeDomain>();

        rangeDomain->lower = parseExpr();
        consume(Token::Type::COL);
        rangeDomain->upper = parseExpr();

        return rangeDomain;
    }

// print_stmt: ('print' | 'println') expr {',' expr} ';'
    fir::PrintStmt::Ptr Parser::parsePrintStmt() {
        try {
            auto printStmt = std::make_shared<fir::PrintStmt>();

            if (peek().type == Token::Type::PRINT) {
                const Token printToken = consume(Token::Type::PRINT);
                printStmt->setBeginLoc(printToken);
                printStmt->printNewline = false;
            } else {
                const Token printlnToken = consume(Token::Type::PRINTLN);
                printStmt->setBeginLoc(printlnToken);
                printStmt->printNewline = true;
            }

            do {
                const fir::Expr::Ptr arg = parseExpr();
                printStmt->args.push_back(arg);
            } while (tryConsume(Token::Type::COMMA));

            const Token endToken = consume(Token::Type::SEMICOL);
            printStmt->setEndLoc(endToken);

            return printStmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::SEMICOL});
            consume(Token::Type::SEMICOL);

            return fir::PrintStmt::Ptr();
        }
    }

// delete_stmt: ('delete') expr ';'
    fir::ExprStmt::Ptr Parser::parseDeleteStmt() {
        try {
            fir::ExprStmt::Ptr stmt;
            stmt = std::make_shared<fir::ExprStmt>();

            const Token deleteToken = consume(Token::Type::DELETE);
            fir::CallExpr::Ptr call_expr = std::make_shared<fir::CallExpr>();

            auto ident = std::make_shared<fir::Identifier>();
            ident->ident = "deleteObject";
            ident->setLoc(deleteToken);
            call_expr->func = ident;

            const fir::Expr::Ptr arg = parseExpr();
            call_expr->args.push_back(arg);

            stmt->expr = call_expr;

            const Token endToken = consume(Token::Type::SEMICOL);
            call_expr->setLoc(endToken);
            stmt->setLoc(endToken);

            return stmt;

        } catch (const SyntaxError &) {
            skipTo({Token::Type::SEMICOL});
            consume(Token::Type::SEMICOL);

            return fir::ExprStmt::Ptr();
        }
    }

// apply_stmt: apply ident ['<' endpoints '>'] ['(' [expr_params] ')']
//             'to' set_index_set ';'
    fir::ApplyStmt::Ptr Parser::parseApplyStmt() {
        try {
            auto applyStmt = std::make_shared<fir::ApplyStmt>();
            applyStmt->map = std::make_shared<fir::UnreducedMapExpr>();

            const Token applyToken = consume(Token::Type::APPLY);
            applyStmt->map->setBeginLoc(applyToken);

            applyStmt->map->func = parseIdent();

            if (tryConsume(Token::Type::LA)) {
                applyStmt->map->genericArgs = parseIndexSets();
                consume(Token::Type::RA);
            }

            if (tryConsume(Token::Type::LP)) {
                if (!tryConsume(Token::Type::RP)) {
                    applyStmt->map->partialActuals = parseExprParams();
                    consume(Token::Type::RP);
                }
            }

            consume(Token::Type::TO);
            applyStmt->map->target = parseSetIndexSet();

            const Token endToken = consume(Token::Type::SEMICOL);
            applyStmt->setEndLoc(endToken);

            return applyStmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::SEMICOL});
            consume(Token::Type::SEMICOL);

            return fir::ApplyStmt::Ptr();
        }
    }

    fir::ReduceStmt::Ptr Parser::parseReduceStmt(Token::Type token_type,
                                                 fir::ReduceStmt::ReductionOp reduce_op,
                                                 fir::Expr::Ptr expr) {
        auto reduce_stmt = std::make_shared<fir::ReduceStmt>();
        reduce_stmt->reduction_op = reduce_op;
        reduce_stmt->lhs.push_back(expr);
        while (tryConsume(Token::Type::COMMA)) {
            const fir::Expr::Ptr expr = parseExpr();
            reduce_stmt->lhs.push_back(expr);
        }

        consume(token_type);
        reduce_stmt->expr = parseExpr();

        for (const auto lhs : reduce_stmt->lhs) {
            if (fir::isa<fir::VarExpr>(lhs)) {
                const std::string varName = fir::to<fir::VarExpr>(lhs)->ident;

                if (!decls.contains(varName)) {
                    decls.insert(varName, IdentType::OTHER);
                }
            }
        }
        return reduce_stmt;
    }

// expr_or_assign_stmt: [[expr {',' expr} '='] expr] ';'
    fir::ExprStmt::Ptr Parser::parseExprOrAssignStmt() {
        try {
            fir::ExprStmt::Ptr stmt;

            auto peek_type = peek().type;
            if (peek().type != Token::Type::SEMICOL) {
                const fir::Expr::Ptr expr = parseExpr();

                switch (peek().type) {
                    case Token::Type::COMMA:
                    case Token::Type::ASSIGN: {
                        auto assignStmt = std::make_shared<fir::AssignStmt>();

                        assignStmt->lhs.push_back(expr);
                        while (tryConsume(Token::Type::COMMA)) {
                            const fir::Expr::Ptr expr = parseExpr();
                            assignStmt->lhs.push_back(expr);
                        }

                        consume(Token::Type::ASSIGN);
                        assignStmt->expr = parseExpr();

                        for (const auto lhs : assignStmt->lhs) {
                            if (fir::isa<fir::VarExpr>(lhs)) {
                                const std::string varName = fir::to<fir::VarExpr>(lhs)->ident;

                                if (!decls.contains(varName)) {
                                    decls.insert(varName, IdentType::OTHER);
                                }
                            }
                        }

                        stmt = assignStmt;
                        break;
                    }
                    case Token::Type::PLUS_REDUCE: {
                        stmt = parseReduceStmt(Token::Type::PLUS_REDUCE, fir::ReduceStmt::ReductionOp::SUM, expr);
                        break;
                    }
                    case Token::Type::MIN_REDUCE: {
                        stmt = parseReduceStmt(Token::Type::MIN_REDUCE, fir::ReduceStmt::ReductionOp::MIN, expr);
                        break;
                    }
                    case Token::Type::MAX_REDUCE: {
                        stmt = parseReduceStmt(Token::Type::MAX_REDUCE, fir::ReduceStmt::ReductionOp::MAX, expr);
                        break;
                    }

                    case Token::Type::ASYNC_MAX_REDUCE: {
                        stmt = parseReduceStmt(Token::Type::ASYNC_MAX_REDUCE, fir::ReduceStmt::ReductionOp::ASYNC_MAX,
                                               expr);
                        break;
                    }

                    case Token::Type::ASYNC_MIN_REDUCE: {
                        stmt = parseReduceStmt(Token::Type::ASYNC_MIN_REDUCE, fir::ReduceStmt::ReductionOp::ASYNC_MIN,
                                               expr);
                        break;
                    }

                    default:
                        stmt = std::make_shared<fir::ExprStmt>();
                        stmt->expr = expr;
                        break;
                }
            }

            const Token endToken = consume(Token::Type::SEMICOL);
            if (stmt) {
                stmt->setEndLoc(endToken);
            }

            return stmt;
        } catch (const SyntaxError &) {
            skipTo({Token::Type::SEMICOL});
            consume(Token::Type::SEMICOL);

            return fir::ExprStmt::Ptr();
        }
    }

// expr: map_expr | new_expr | or_expr
    fir::Expr::Ptr Parser::parseExpr() {
        switch (peek().type) {
            case Token::Type::MAP:
                return parseMapExpr();
            case Token::Type::NEW:
                return parseNewExpr();
            case Token::Type::LOAD:
                return parseLoadExpr();
            case Token::Type::INTERSECTION:
                return parseIntersectionExpr();
            default:
                return parseOrExpr();
        }
    }

// map_expr: 'map' ident ['<' endpoints '>'] ['(' [expr_params] ')']
//           'to' set_index_set ['through' set_index_set] ['reduce' '+']
    fir::MapExpr::Ptr Parser::parseMapExpr() {
        const Token mapToken = consume(Token::Type::MAP);
        const fir::Identifier::Ptr func = parseIdent();

        std::vector<fir::IndexSet::Ptr> genericArgs;
        if (tryConsume(Token::Type::LA)) {
            genericArgs = parseIndexSets();
            consume(Token::Type::RA);
        }

        std::vector<fir::Expr::Ptr> partialActuals;
        if (tryConsume(Token::Type::LP)) {
            if (!tryConsume(Token::Type::RP)) {
                partialActuals = parseExprParams();
                consume(Token::Type::RP);
            }
        }

        consume(Token::Type::TO);
        const fir::SetIndexSet::Ptr target = parseSetIndexSet();

        fir::SetIndexSet::Ptr through;
        if (tryConsume(Token::Type::THROUGH)) {
            through = parseSetIndexSet();
        }

        if (tryConsume(Token::Type::REDUCE)) {
            const auto mapExpr = std::make_shared<fir::ReducedMapExpr>();
            mapExpr->setBeginLoc(mapToken);

            mapExpr->func = func;
            mapExpr->genericArgs = genericArgs;
            mapExpr->partialActuals = partialActuals;
            mapExpr->target = target;
            mapExpr->through = through;
            mapExpr->op = fir::MapExpr::ReductionOp::SUM;

            const Token plusToken = consume(Token::Type::PLUS);
            mapExpr->setEndLoc(plusToken);

            return mapExpr;
        }

        const auto mapExpr = std::make_shared<fir::UnreducedMapExpr>();
        mapExpr->setBeginLoc(mapToken);

        mapExpr->func = func;
        mapExpr->genericArgs = genericArgs;
        mapExpr->partialActuals = partialActuals;
        mapExpr->target = target;
        mapExpr->through = through;

        return mapExpr;
    }

// or_expr: and_expr {'or' and_expr}
    fir::Expr::Ptr Parser::parseOrExpr() {
        fir::Expr::Ptr expr = parseAndExpr();

        while (tryConsume(Token::Type::OR)) {
            auto orExpr = std::make_shared<fir::OrExpr>();

            orExpr->lhs = expr;
            orExpr->rhs = parseAndExpr();

            expr = orExpr;
        }

        return expr;
    }

// and_expr: xor_expr {'and' xor_expr}
    fir::Expr::Ptr Parser::parseAndExpr() {
        fir::Expr::Ptr expr = parseXorExpr();

        while (tryConsume(Token::Type::AND)) {
            auto andExpr = std::make_shared<fir::AndExpr>();

            andExpr->lhs = expr;
            andExpr->rhs = parseXorExpr();

            expr = andExpr;
        }

        return expr;
    }

// xor_expr: eq_expr {'xor' eq_expr}
    fir::Expr::Ptr Parser::parseXorExpr() {
        fir::Expr::Ptr expr = parseEqExpr();

        while (tryConsume(Token::Type::XOR)) {
            auto xorExpr = std::make_shared<fir::XorExpr>();

            xorExpr->lhs = expr;
            xorExpr->rhs = parseEqExpr();

            expr = xorExpr;
        }

        return expr;
    }

// eq_expr: term {('==' | '!=' | '>' | '<' | '>=' | '<=') term}
    fir::Expr::Ptr Parser::parseEqExpr() {
        auto expr = std::make_shared<fir::EqExpr>();

        const fir::Expr::Ptr operand = parseTerm();
        expr->operands.push_back(operand);

        while (true) {
            switch (peek().type) {
                case Token::Type::EQ:
                    consume(Token::Type::EQ);
                    expr->ops.push_back(fir::EqExpr::Op::EQ);
                    break;
                case Token::Type::NE:
                    consume(Token::Type::NE);
                    expr->ops.push_back(fir::EqExpr::Op::NE);
                    break;
                case Token::Type::RA:
                    consume(Token::Type::RA);
                    expr->ops.push_back(fir::EqExpr::Op::GT);
                    break;
                case Token::Type::LA:
                    consume(Token::Type::LA);
                    expr->ops.push_back(fir::EqExpr::Op::LT);
                    break;
                case Token::Type::GE:
                    consume(Token::Type::GE);
                    expr->ops.push_back(fir::EqExpr::Op::GE);
                    break;
                case Token::Type::LE:
                    consume(Token::Type::LE);
                    expr->ops.push_back(fir::EqExpr::Op::LE);
                    break;
                default:
                    return (expr->operands.size() == 1) ? expr->operands[0] : expr;
            }

            const fir::Expr::Ptr operand = parseTerm();
            expr->operands.push_back(operand);
        }
    }

// term: ('not' term) | add_expr
    fir::Expr::Ptr Parser::parseTerm() {
        if (peek().type == Token::Type::NOT) {
            auto notExpr = std::make_shared<fir::NotExpr>();

            const Token notToken = consume(Token::Type::NOT);
            notExpr->setBeginLoc(notToken);
            notExpr->operand = parseTerm();

            return notExpr;
        }

        return parseAddExpr();
    }

// add_expr : mul_expr {('+' | '-') mul_expr}
    fir::Expr::Ptr Parser::parseAddExpr() {
        fir::Expr::Ptr expr = parseMulExpr();

        while (true) {
            fir::BinaryExpr::Ptr addExpr;
            switch (peek().type) {
                case Token::Type::PLUS:
                    consume(Token::Type::PLUS);
                    addExpr = std::make_shared<fir::AddExpr>();
                    break;
                case Token::Type::MINUS:
                    consume(Token::Type::MINUS);
                    addExpr = std::make_shared<fir::SubExpr>();
                    break;
                default:
                    return expr;
            }

            addExpr->lhs = expr;
            addExpr->rhs = parseMulExpr();

            expr = addExpr;
        }
    }

// mul_expr: neg_expr {('*' | '/' | '\' | '.*' | './') neg_expr}
    fir::Expr::Ptr Parser::parseMulExpr() {
        fir::Expr::Ptr expr = parseNegExpr();

        while (true) {
            fir::BinaryExpr::Ptr mulExpr;
            switch (peek().type) {
                case Token::Type::STAR:
                    consume(Token::Type::STAR);
                    mulExpr = std::make_shared<fir::MulExpr>();
                    break;
                case Token::Type::SLASH:
                    consume(Token::Type::SLASH);
                    mulExpr = std::make_shared<fir::DivExpr>();
                    break;
                case Token::Type::BACKSLASH:
                    consume(Token::Type::BACKSLASH);
                    mulExpr = std::make_shared<fir::LeftDivExpr>();
                    break;
                case Token::Type::DOTSTAR:
                    consume(Token::Type::DOTSTAR);
                    mulExpr = std::make_shared<fir::ElwiseMulExpr>();
                    break;
                case Token::Type::DOTSLASH:
                    consume(Token::Type::DOTSLASH);
                    mulExpr = std::make_shared<fir::ElwiseDivExpr>();
                    break;
                default:
                    return expr;
            }

            mulExpr->lhs = expr;
            mulExpr->rhs = parseNegExpr();

            expr = mulExpr;
        }
    }

// neg_expr: (('+' | '-') neg_expr) | exp_expr
    fir::Expr::Ptr Parser::parseNegExpr() {
        auto negExpr = std::make_shared<fir::NegExpr>();

        switch (peek().type) {
            case Token::Type::MINUS: {
                const Token minusToken = consume(Token::Type::MINUS);
                negExpr->setBeginLoc(minusToken);
                negExpr->negate = true;
                break;
            }
            case Token::Type::PLUS: {
                const Token plusToken = consume(Token::Type::PLUS);
                negExpr->setBeginLoc(plusToken);
                negExpr->negate = false;
                break;
            }
            default:
                return parseExpExpr();
        }
        negExpr->operand = parseNegExpr();

        return negExpr;
    }

// exp_expr: transpose_expr ['^' exp_expr]
    fir::Expr::Ptr Parser::parseExpExpr() {
        fir::Expr::Ptr expr = parseTransposeExpr();

        if (tryConsume(Token::Type::EXP)) {
            auto expExpr = std::make_shared<fir::ExpExpr>();

            expExpr->lhs = expr;
            expExpr->rhs = parseExpExpr();

            expr = expExpr;
        }

        return expr;
    }


// transpose_expr: tensor_read_expr {'''}
    fir::Expr::Ptr Parser::parseTransposeExpr() {
        fir::Expr::Ptr expr = parseTensorReadExpr();

        while (peek().type == Token::Type::TRANSPOSE) {
            auto transposeExpr = std::make_shared<fir::TransposeExpr>();

            const Token transposeToken = consume(Token::Type::TRANSPOSE);
            transposeExpr->setEndLoc(transposeToken);
            transposeExpr->operand = expr;

            expr = transposeExpr;
        }

        return expr;
    }

// DEPRECATED: tensor_read_expr: field_read_expr {'(' [read_params] ')'}
// changed to square brackets
// tensor_read_expr: field_read_expr {'[' [read_params] ']'}


    fir::Expr::Ptr Parser::parseTensorReadExpr() {
        fir::Expr::Ptr expr = parseFieldReadExpr();

        while (tryConsume(Token::Type::LB)) {
            auto tensorRead = std::make_shared<fir::TensorReadExpr>();
            tensorRead->tensor = expr;

            if (peek().type != Token::Type::RB) {
                tensorRead->indices = parseReadParams();
            }

            const Token rightParenToken = consume(Token::Type::RB);
            tensorRead->setEndLoc(rightParenToken);

            expr = tensorRead;
        }

        return expr;
    }

// DEPRECATED SIMIT GRAMMR: field_read_expr: set_read_expr ['.' ident]
// field_read_expr: set_read_expr {'.' (ident( [ expr_params ] )) | apply '(' ident ')' | where '(' ident ')'
// | from '(' expr ')' '.' to '(' expr ')''.' apply '(' ident ')' ['.' modified '(' ident ')'],}
    fir::Expr::Ptr Parser::parseFieldReadExpr() {
        // We don't need to supprot set read expressions, so we just work with factors directly
        //fir::Expr::Ptr expr = parseSetReadExpr();
        fir::Expr::Ptr expr = parseFactor();

        fir::FromExpr::Ptr from_expr;
        fir::ToExpr::Ptr to_expr;

        while (tryConsume(Token::Type::PERIOD)) {

            // For now, we are assuming that FROM and TO filters cannot appear by themselves. They will always be fused into the apply family of nodes
            // right now this is a bit of a hack, srcFilter and from act the same in the compiler
            if (tryConsume(Token::Type::FROM) || tryConsume(Token::Type::SRC_FILTER)) {
                consume(Token::Type::LP);
                from_expr = std::make_shared<fir::FromExpr>();
                from_expr->input_func = parseIdent();
                consume(Token::Type::RP);
                // right now this is a bit of a hack, dstFilter and to act the same in the compiler
            } else if (tryConsume(Token::Type::TO) || tryConsume(Token::Type::DST_FILTER)) {
                consume(Token::Type::LP);
                to_expr = std::make_shared<fir::ToExpr>();
                to_expr->input_func = parseIdent();
                consume(Token::Type::RP);
            } else if (tryConsume(Token::Type::APPLY)) {
                consume(Token::Type::LP);
                auto apply_expr = std::make_shared<fir::ApplyExpr>();
                apply_expr->target = expr;
                apply_expr->input_function = parseIdent();
                consume(Token::Type::RP);
                expr = apply_expr;
                apply_expr->type = fir::ApplyExpr::Type::REGULAR_APPLY;
                apply_expr->from_expr = from_expr;
                apply_expr->to_expr = to_expr;
            } else if (tryConsume(Token::Type::APPLYMODIFIED)) {
                consume(Token::Type::LP);
                auto apply_expr = std::make_shared<fir::ApplyExpr>();
                apply_expr->target = expr;
                apply_expr->input_function = parseIdent();
                consume(Token::Type::COMMA);
                auto change_tracking_field = parseIdent();
                apply_expr->change_tracking_field = change_tracking_field;
                //check for the optional boolean variable
                if (tryConsume(Token::Type::COMMA)) {
                    if (tryConsume(Token::Type::FALSE)) {
                        //set the deduplication field
                        apply_expr->disable_deduplication = false;
                    } else if (tryConsume(Token::Type::TRUE)) {
                        apply_expr->disable_deduplication = true;
                    } else {
                        reportError(peek(), "applyModified with unrecognized argument for deduplication");
                        throw SyntaxError();
                    }
                }
                consume(Token::Type::RP);
                apply_expr->from_expr = from_expr;
                apply_expr->to_expr = to_expr;
                apply_expr->type = fir::ApplyExpr::Type::REGULAR_APPLY;
                expr = apply_expr;

            } else if (tryConsume(Token::Type::APPLY_UPDATE_PRIORITY)) {
                consume(Token::Type::LP);
                auto apply_expr = std::make_shared<fir::ApplyExpr>();
                apply_expr->target = expr;
                apply_expr->input_function = parseIdent();
                consume(Token::Type::RP);
                expr = apply_expr;
                apply_expr->type = fir::ApplyExpr::Type::UPDATE_PRIORITY_APPLY;
                apply_expr->from_expr = from_expr;
                apply_expr->to_expr = to_expr;
                expr = apply_expr;
            } else if (tryConsume(Token::Type::APPLY_UPDATE_PRIORITY_EXTERN)) {
                consume(Token::Type::LP);
                auto apply_expr = std::make_shared<fir::ApplyExpr>();
                apply_expr->target = expr;
                apply_expr->input_function = parseIdent();
                consume(Token::Type::RP);
                expr = apply_expr;
                apply_expr->type = fir::ApplyExpr::Type::UPDATE_PRIORITY_EXTERN_APPLY;
                apply_expr->from_expr = from_expr;
                apply_expr->to_expr = to_expr;
                expr = apply_expr;
            } else if (tryConsume(Token::Type::WHERE) || tryConsume(Token::Type::FILTER)) {
                consume(Token::Type::LP);
                auto where_expr = std::make_shared<fir::WhereExpr>();
                where_expr->input_func = parseIdent();
                where_expr->target = expr;
                consume(Token::Type::RP);
                expr = where_expr;
            } else if (peek().type == Token::Type::IDENT) {
                // transforming into builtin intrinsics (runtime libraries)
                auto ident = parseIdent();
                if (tryConsume(Token::Type::LP)) {
                    //make a  method call expression
                    auto method_call_expr = std::make_shared<fir::MethodCallExpr>();
                    method_call_expr->target = expr;
                    if (isIntrinsic(ident->ident)) {
                        ident->ident = "builtin_" + ident->ident;
                    }
                    method_call_expr->method_name = ident;
                    if (peek().type != Token::Type::RP) {
                        method_call_expr->args = parseExprParams();
                    }
                    consume(Token::Type::RP);
                    expr = method_call_expr;
                } else {
                    auto field_read = std::make_shared<fir::FieldReadExpr>();

                    field_read->setOrElem = expr;
                    field_read->field = ident;
                    expr = field_read;
                }
            } else {
                reportError(peek(), " error parsing FieldReadExpr");
                throw SyntaxError();
            }

        }
        return expr;
    }


// set_read_expr: factor ['[' [expr_params [';' expr_params]] ']']
    fir::Expr::Ptr Parser::parseSetReadExpr() {
        fir::Expr::Ptr expr = parseFactor();

        if (tryConsume(Token::Type::LB)) {
            auto setRead = std::make_shared<fir::SetReadExpr>();
            setRead->set = expr;

            if (peek().type != Token::Type::RB) {
                setRead->indices = parseExprParams();

                if (tryConsume(Token::Type::SEMICOL)) {
                    auto sink = parseExprParams();
                    std::copy(sink.begin(), sink.end(),
                              std::back_inserter(setRead->indices));
                }
            }

            const Token rightBracketToken = consume(Token::Type::RB);
            setRead->setEndLoc(rightBracketToken);

            expr = setRead;
        }

        return expr;
    }


// factor: ('(' expr ')') | call_expr | unnamed_tuple_read_expr
//       | named_tuple_read_expr | range_const | var_expr | tensor_literal
    fir::Expr::Ptr Parser::parseFactor() {
        switch (peek().type) {
            case Token::Type::LP: {
                auto parenExpr = std::make_shared<fir::ParenExpr>();

                const Token leftParenToken = consume(Token::Type::LP);
                parenExpr->setBeginLoc(leftParenToken);

                parenExpr->expr = parseExpr();

                const Token rightParenToken = consume(Token::Type::RP);
                parenExpr->setEndLoc(rightParenToken);

                return parenExpr;
            }
            case Token::Type::IDENT: {
                if (peek(1).type == Token::Type::LA) {
                    switch (peek(3).type) {
                        case Token::Type::RA:
                            if (peek(4).type != Token::Type::LP ||
                                peek(5).type != Token::Type::RP) {
                                break;
                            }
                        case Token::Type::COMMA:
                            return parseCallExpr();
                        default:
                            break;
                    }
                }

                const std::string identStr = peek().str;

                if (decls.contains(identStr)) {
                    switch (decls.get(identStr)) {
                        case IdentType::FUNCTION:
                            // If the function is actually being called, then return a CallExpr, else treat the function name as a variable and return a VarExpr
                            if (peek(1).type == Token::Type::LP)
                                return parseCallExpr();
                            break;
                        case IdentType::RANGE_GENERIC_PARAM:
                            return parseRangeConst();
                        case IdentType::TUPLE:
                            switch (peek(1).type) {
                                case Token::Type::LP:
                                    return parseUnnamedTupleReadExpr();
                                case Token::Type::PERIOD:
                                    return parseNamedTupleReadExpr();
                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                }

                return parseVarExpr();
            }
            case Token::Type::INT_LITERAL:
            case Token::Type::FLOAT_LITERAL:
            case Token::Type::STRING_LITERAL:
            case Token::Type::TRUE:
            case Token::Type::FALSE:
            case Token::Type::LB:
            case Token::Type::LA:
                return parseTensorLiteral();
            default:
                reportError(peek(), "an expression");
                throw SyntaxError();
                break;
        }

        return fir::Expr::Ptr();
    }

// var_expr: ident
    fir::VarExpr::Ptr Parser::parseVarExpr() {
        auto varExpr = std::make_shared<fir::VarExpr>();

        const Token identToken = consume(Token::Type::IDENT);
        varExpr->setLoc(identToken);
        varExpr->ident = identToken.str;

        return varExpr;
    }

// range_const: ident
    fir::RangeConst::Ptr Parser::parseRangeConst() {
        auto rangeConst = std::make_shared<fir::RangeConst>();

        const Token identToken = consume(Token::Type::IDENT);
        rangeConst->setLoc(identToken);
        rangeConst->ident = identToken.str;

        return rangeConst;
    }

// call_expr: ident ['<' endpoints '>'] '(' [expr_params] ')'
    fir::CallExpr::Ptr Parser::parseCallExpr() {
        auto call = std::make_shared<fir::CallExpr>();

        call->func = parseIdent();

        if (tryConsume(Token::Type::LA)) {
            call->genericArgs = parseIndexSets();
            consume(Token::Type::RA);
        }

        consume(Token::Type::LP);

        if (peek().type != Token::Type::RP) {
            call->args = parseExprParams();
        }

        const Token endToken = consume(Token::Type::RP);
        call->setEndLoc(endToken);

        return call;
    }

// unnamed_tuple_read_expr: var_expr '(' expr ')'
    fir::UnnamedTupleReadExpr::Ptr Parser::parseUnnamedTupleReadExpr() {
        auto tupleRead = std::make_shared<fir::UnnamedTupleReadExpr>();

        tupleRead->tuple = parseVarExpr();
        consume(Token::Type::LP);

        tupleRead->index = parseExpr();

        const Token endToken = consume(Token::Type::RP);
        tupleRead->setEndLoc(endToken);

        return tupleRead;
    }

// named_tuple_read_expr: var_expr '.' ident
    fir::NamedTupleReadExpr::Ptr Parser::parseNamedTupleReadExpr() {
        auto tupleRead = std::make_shared<fir::NamedTupleReadExpr>();

        tupleRead->tuple = parseVarExpr();
        consume(Token::Type::PERIOD);
        tupleRead->elem = parseIdent();

        return tupleRead;
    }


// ident: IDENT
    fir::Identifier::Ptr Parser::parseIdent() {
        auto ident = std::make_shared<fir::Identifier>();

        const Token identToken = consume(Token::Type::IDENT);
        ident->setLoc(identToken);
        ident->ident = identToken.str;

        return ident;
    }

// read_params: read_param {',' read_param}
    std::vector<fir::ReadParam::Ptr> Parser::parseReadParams() {
        std::vector<fir::ReadParam::Ptr> readParams;

        do {
            const fir::ReadParam::Ptr param = parseReadParam();
            readParams.push_back(param);
        } while (tryConsume(Token::Type::COMMA));

        return readParams;
    }

// read_param: ':' | expr
    fir::ReadParam::Ptr Parser::parseReadParam() {
        if (peek().type == Token::Type::COL) {
            auto slice = std::make_shared<fir::Slice>();

            const Token colToken = consume(Token::Type::COL);
            slice->setLoc(colToken);

            return slice;
        }

        auto param = std::make_shared<fir::ExprParam>();
        param->expr = parseExpr();

        return param;
    }

// expr_params: expr {',' expr}
    std::vector<fir::Expr::Ptr> Parser::parseExprParams() {
        std::vector<fir::Expr::Ptr> exprParams;

        do {
            const fir::Expr::Ptr param = parseExpr();
            exprParams.push_back(param);
        } while (tryConsume(Token::Type::COMMA));

        return exprParams;
    }


// type: element_type | unstructured_set_type | grid_set_type
//     | tuple_type | tensor_type
    fir::Type::Ptr Parser::parseType() {
        fir::Type::Ptr type;
        switch (peek().type) {
            case Token::Type::IDENT:
                type = parseElementType();
                break;
            case Token::Type::SET:
                type = parseUnstructuredSetType();
                break;
            case Token::Type::EDGE_SET:
                type = parseEdgeSetType();
                break;
            case Token::Type::VERTEX_SET:
                type = parseVertexSetType();
                break;
            case Token::Type::LIST:
                type = parseListType();
                break;
            case Token::Type::GRID:
                type = parseGridSetType();
                break;
            case Token::Type::LP:
                type = (peek(2).type == Token::Type::COL) ?
                       parseNamedTupleType() : parseUnnamedTupleType();
                break;
            case Token::Type::DOUBLE:
            case Token::Type::INT:
            case Token::Type::UINT:
            case Token::Type::UINT_64:
            case Token::Type::FLOAT:
            case Token::Type::BOOL:
            case Token::Type::COMPLEX:
            case Token::Type::STRING:
            case Token::Type::TENSOR:
            case Token::Type::MATRIX:
            case Token::Type::VECTOR:
                type = parseTensorType();
                break;
            case Token::Type::OPAQUE:
                consume(Token::Type::OPAQUE);
                type = std::make_shared<fir::OpaqueType>();
                break;
                // OG Additions
            case Token::Type::PRIORITY_QUEUE:
                type = parsePriorityQueueType();
                break;

            default:
                reportError(peek(), "a type identifier");
                throw SyntaxError();
                break;
        }

        return type;
    }

// element_type: ident
    fir::ElementType::Ptr Parser::parseElementType() {
        auto elementType = std::make_shared<fir::ElementType>();

        const Token typeToken = consume(Token::Type::IDENT);
        elementType->setLoc(typeToken);
        elementType->ident = typeToken.str;

        return elementType;
    }

// unstructured_set_type: 'set' '{' element_type '}'
//                        ['(' (endpoints | (endpoint '*' tuple_length)) ')']
    fir::SetType::Ptr Parser::parseUnstructuredSetType() {
        const Token setToken = consume(Token::Type::SET);

        consume(Token::Type::LC);

        const fir::ElementType::Ptr element = parseElementType();
        const Token rightCurlyToken = consume(Token::Type::RC);

        if (peek(2).type == Token::Type::STAR && tryConsume(Token::Type::LP)) {
            auto setType = std::make_shared<fir::HomogeneousEdgeSetType>();
            setType->setBeginLoc(setToken);

            setType->element = element;
            setType->endpoint = parseEndpoint();

            consume(Token::Type::STAR);
            setType->arity = parseTupleLength();

            const Token rightParenToken = consume(Token::Type::RP);
            setType->setEndLoc(rightParenToken);

            return setType;
        }

        auto setType = std::make_shared<fir::HeterogeneousEdgeSetType>();

        setType->setBeginLoc(setToken);
        setType->element = element;
        setType->setEndLoc(rightCurlyToken);

        if (tryConsume(Token::Type::LP)) {
            setType->endpoints = parseEndpoints();

            const Token rightParenToken = consume(Token::Type::RP);
            setType->setEndLoc(rightParenToken);
        }

        return setType;
    }

// grid_set_type: 'grid' '[' INT_LITERAL ']' '{' element_type '}' '(' IDENT ')'
    fir::SetType::Ptr Parser::parseGridSetType() {
        auto setType = std::make_shared<fir::GridSetType>();

        const Token gridToken = consume(Token::Type::GRID);
        setType->setBeginLoc(gridToken);

        consume(Token::Type::LB);
        const Token dimsToken = consume(Token::Type::INT_LITERAL);
        setType->dimensions = dimsToken.num;
        consume(Token::Type::RB);

        consume(Token::Type::LC);
        setType->element = parseElementType();
        consume(Token::Type::RC);

        consume(Token::Type::LP);
        auto underlyingPointSet = std::make_shared<fir::Endpoint>();
        underlyingPointSet->set = parseSetIndexSet();
        setType->underlyingPointSet = underlyingPointSet;

        const Token endToken = consume(Token::Type::RP);
        setType->setEndLoc(endToken);

        return setType;
    }

// endpoints: endpoint {',' endpoint}
    std::vector<fir::Endpoint::Ptr> Parser::parseEndpoints() {
        std::vector<fir::Endpoint::Ptr> endpoints;

        do {
            const fir::Endpoint::Ptr endpoint = parseEndpoint();
            endpoints.push_back(endpoint);
        } while (tryConsume(Token::Type::COMMA));

        return endpoints;
    }

// endpoint: set_index_set
    fir::Endpoint::Ptr Parser::parseEndpoint() {
        auto endpoint = std::make_shared<fir::Endpoint>();
        endpoint->set = parseSetIndexSet();

        return endpoint;
    }

// tuple_element: ident ':' element_type
    fir::TupleElement::Ptr Parser::parseTupleElement() {
        auto tupleElement = std::make_shared<fir::TupleElement>();

        tupleElement->name = parseIdent();
        consume(Token::Type::COL);
        tupleElement->element = parseElementType();

        return tupleElement;
    }

// named_tuple_type: '(' tuple_elemnt {',' tuple_element} ')'
    fir::TupleType::Ptr Parser::parseNamedTupleType() {
        auto tupleType = std::make_shared<fir::NamedTupleType>();

        const Token leftParenToken = consume(Token::Type::LP);
        tupleType->setBeginLoc(leftParenToken);

        do {
            const fir::TupleElement::Ptr elem = parseTupleElement();
            tupleType->elems.push_back(elem);
        } while (tryConsume(Token::Type::COMMA));

        const Token rightParenToken = consume(Token::Type::RP);
        tupleType->setEndLoc(rightParenToken);

        return tupleType;
    }

// tuple_length: INT_LITERAL
    fir::TupleLength::Ptr Parser::parseTupleLength() {
        auto tupleLength = std::make_shared<fir::TupleLength>();

        const Token intToken = consume(Token::Type::INT_LITERAL);
        tupleLength->setLoc(intToken);
        tupleLength->val = intToken.num;

        return tupleLength;
    }

// unnamed_tuple_type: '(' element_type '*' tuple_length ')'
    fir::TupleType::Ptr Parser::parseUnnamedTupleType() {
        auto tupleType = std::make_shared<fir::UnnamedTupleType>();

        const Token leftParenToken = consume(Token::Type::LP);
        tupleType->setBeginLoc(leftParenToken);

        tupleType->element = parseElementType();
        consume(Token::Type::STAR);
        tupleType->length = parseTupleLength();

        const Token rightParenToken = consume(Token::Type::RP);
        tupleType->setEndLoc(rightParenToken);

        return tupleType;
    }

// tensor_type: scalar_type | matrix_block_type
//            | (vector_block_type | tensor_block_type) [''']
    fir::TensorType::Ptr Parser::parseTensorType() {
        fir::NDTensorType::Ptr tensorType;
        switch (peek().type) {
            case Token::Type::INT:
            case Token::Type::UINT:
            case Token::Type::UINT_64:
            case Token::Type::FLOAT:
            case Token::Type::DOUBLE:
            case Token::Type::BOOL:
            case Token::Type::COMPLEX:
            case Token::Type::STRING:
                return parseScalarType();
            case Token::Type::MATRIX:
                return parseMatrixBlockType();
            case Token::Type::VECTOR:
                tensorType = parseVectorBlockType();
                break;
            default:
                tensorType = parseTensorBlockType();
                break;
        }

        if (peek().type == Token::Type::TRANSPOSE) {
            const Token transposeToken = consume(Token::Type::TRANSPOSE);
            tensorType->setEndLoc(transposeToken);
            tensorType->transposed = true;
        }

        return tensorType;
    }

// vector_block_type: 'vector' ['[' index_set ']' | '{' element_type '}' ]
//                    '(' (vector_block_type | tensor_component_type) ')'
    fir::NDTensorType::Ptr Parser::parseVectorBlockType() {
        auto tensorType = std::make_shared<fir::NDTensorType>();
        tensorType->transposed = false;

        const Token tensorToken = consume(Token::Type::VECTOR);
        tensorType->setBeginLoc(tensorToken);

        if (tryConsume(Token::Type::LB)) {
            const fir::IndexSet::Ptr indexSet = parseIndexSet();
            tensorType->indexSets.push_back(indexSet);
            consume(Token::Type::RB);
        }

        // added to support specifying the Element Type.
        // The vector is essentially a field of the Element
        if (tryConsume(Token::Type::LC)) {
            const fir::ElementType::Ptr element = parseElementType();
            tensorType->element = element;
            consume(Token::Type::RC);
        }

        consume(Token::Type::LP);
        if (peek().type == Token::Type::VECTOR) {
            tensorType->blockType = parseVectorBlockType();
        } else {
            tensorType->blockType = parseScalarType();
        }

        const Token rightParenToken = consume(Token::Type::RP);
        tensorType->setEndLoc(rightParenToken);

        return tensorType;
    }

// matrix_block_type: 'matrix' ['[' index_set ',' index_set ']']
//                    '(' (matrix_block_type | tensor_component_type) ')'
    fir::NDTensorType::Ptr Parser::parseMatrixBlockType() {
        auto tensorType = std::make_shared<fir::NDTensorType>();
        tensorType->transposed = false;

        const Token tensorToken = consume(Token::Type::MATRIX);
        tensorType->setBeginLoc(tensorToken);

        if (tryConsume(Token::Type::LB)) {
            fir::IndexSet::Ptr indexSet = parseIndexSet();
            tensorType->indexSets.push_back(indexSet);
            consume(Token::Type::COMMA);

            indexSet = parseIndexSet();
            tensorType->indexSets.push_back(indexSet);
            consume(Token::Type::RB);
        }

        consume(Token::Type::LP);
        if (peek().type == Token::Type::MATRIX) {
            tensorType->blockType = parseMatrixBlockType();
        } else {
            tensorType->blockType = parseScalarType();
        }

        const Token rightParenToken = consume(Token::Type::RP);
        tensorType->setEndLoc(rightParenToken);

        return tensorType;
    }

// tensor_block_type: 'tensor' ['[' index_sets ']']
//                    '(' (tensor_block_type | tensor_component_type) ')'
    fir::NDTensorType::Ptr Parser::parseTensorBlockType() {
        auto tensorType = std::make_shared<fir::NDTensorType>();
        tensorType->transposed = false;

        const Token tensorToken = consume(Token::Type::TENSOR);
        tensorType->setBeginLoc(tensorToken);

        if (tryConsume(Token::Type::LB)) {
            tensorType->indexSets = parseIndexSets();
            consume(Token::Type::RB);
        }

        consume(Token::Type::LP);
        if (peek().type == Token::Type::TENSOR) {
            tensorType->blockType = parseTensorBlockType();
        } else {
            tensorType->blockType = parseScalarType();
        }

        const Token rightParenToken = consume(Token::Type::RP);
        tensorType->setEndLoc(rightParenToken);

        return tensorType;
    }

// tensor_component_type: 'int' | 'float' | 'bool' | 'complex'
    fir::ScalarType::Ptr Parser::parseTensorComponentType() {
        auto scalarType = std::make_shared<fir::ScalarType>();

        const Token typeToken = peek();
        scalarType->setLoc(typeToken);

        switch (peek().type) {
            case Token::Type::INT:
                consume(Token::Type::INT);
                scalarType->type = fir::ScalarType::Type::INT;
                break;
            case Token::Type::UINT:
                consume(Token::Type::UINT);
                scalarType->type = fir::ScalarType::Type::UINT;
                break;
            case Token::Type::UINT_64:
                consume(Token::Type::UINT_64);
                scalarType->type = fir::ScalarType::Type::UINT_64;
                break;
            case Token::Type::FLOAT:
                consume(Token::Type::FLOAT);
                scalarType->type = fir::ScalarType::Type::FLOAT;
                break;
            case Token::Type::DOUBLE:
                consume(Token::Type::DOUBLE);
                scalarType->type = fir::ScalarType::Type::DOUBLE;
                break;
            case Token::Type::BOOL:
                consume(Token::Type::BOOL);
                scalarType->type = fir::ScalarType::Type::BOOL;
                break;
            case Token::Type::COMPLEX:
                consume(Token::Type::COMPLEX);
                scalarType->type = fir::ScalarType::Type::COMPLEX;
                break;
            case Token::Type::STRING:
                // TODO: Implement.
            default:
                reportError(peek(), "a tensor component type identifier");
                throw SyntaxError();
                break;
        }

        return scalarType;
    }

// scalar_type: 'string' | tensor_component_type
    fir::ScalarType::Ptr Parser::parseScalarType() {
        if (peek().type == Token::Type::STRING) {
            auto stringType = std::make_shared<fir::ScalarType>();

            const Token stringToken = consume(Token::Type::STRING);
            stringType->setLoc(stringToken);
            stringType->type = fir::ScalarType::Type::STRING;

            return stringType;
        }

        return parseTensorComponentType();
    }

// index_sets: index_set {',' index_set}
    std::vector<fir::IndexSet::Ptr> Parser::parseIndexSets() {
        std::vector<fir::IndexSet::Ptr> indexSets;

        do {
            const fir::IndexSet::Ptr indexSet = parseIndexSet();
            indexSets.push_back(indexSet);
        } while (tryConsume(Token::Type::COMMA));

        return indexSets;
    }

// index_set: INT_LITERAL | set_index_set
    fir::IndexSet::Ptr Parser::parseIndexSet() {
        fir::IndexSet::Ptr indexSet;
        switch (peek().type) {
            case Token::Type::INT_LITERAL: {
                auto rangeIndexSet = std::make_shared<fir::RangeIndexSet>();

                const Token intToken = consume(Token::Type::INT_LITERAL);
                rangeIndexSet->setLoc(intToken);
                rangeIndexSet->range = intToken.num;

                indexSet = rangeIndexSet;
                break;
            }
            case Token::Type::IDENT:
                indexSet = parseSetIndexSet();
                break;
            default:
                reportError(peek(), "an index set");
                throw SyntaxError();
                break;
        }

        return indexSet;
    }

// set_index_set: ident
    fir::SetIndexSet::Ptr Parser::parseSetIndexSet() {
        const Token identToken = consume(Token::Type::IDENT);
        const std::string identStr = identToken.str;

        fir::SetIndexSet::Ptr setIndexSet;

        if (decls.contains(identStr)) {
            switch (decls.get(identStr)) {
                case IdentType::GENERIC_PARAM: {
                    auto genericIndexSet = std::make_shared<fir::GenericIndexSet>();
                    genericIndexSet->type = fir::GenericIndexSet::Type::UNKNOWN;

                    setIndexSet = genericIndexSet;
                    break;
                }
                case IdentType::RANGE_GENERIC_PARAM: {
                    auto genericIndexSet = std::make_shared<fir::GenericIndexSet>();
                    genericIndexSet->type = fir::GenericIndexSet::Type::RANGE;

                    setIndexSet = genericIndexSet;
                    break;
                }
                default:
                    break;
            }
        }

        if (!setIndexSet) {
            setIndexSet = std::make_shared<fir::SetIndexSet>();
        }

        setIndexSet->setLoc(identToken);
        setIndexSet->setName = identStr;

        return setIndexSet;
    }

// tensor_literal: INT_LITERAL | FLOAT_LITERAL | 'true' | 'false'
//               | STRING_LITERAL | complex_literal | dense_tensor_literal
    fir::Expr::Ptr Parser::parseTensorLiteral() {
        fir::Expr::Ptr literal;
        switch (peek().type) {
            case Token::Type::INT_LITERAL: {
                auto intLiteral = std::make_shared<fir::IntLiteral>();

                const Token intToken = consume(Token::Type::INT_LITERAL);
                intLiteral->setLoc(intToken);
                intLiteral->val = intToken.num;

                literal = intLiteral;
                break;
            }
            case Token::Type::FLOAT_LITERAL: {
                auto floatLiteral = std::make_shared<fir::FloatLiteral>();

                const Token floatToken = consume(Token::Type::FLOAT_LITERAL);
                floatLiteral->setLoc(floatToken);
                floatLiteral->val = floatToken.fnum;

                literal = floatLiteral;
                break;
            }
            case Token::Type::TRUE: {
                auto trueLiteral = std::make_shared<fir::BoolLiteral>();

                const Token trueToken = consume(Token::Type::TRUE);
                trueLiteral->setLoc(trueToken);
                trueLiteral->val = true;

                literal = trueLiteral;
                break;
            }
            case Token::Type::FALSE: {
                auto falseLiteral = std::make_shared<fir::BoolLiteral>();

                const Token falseToken = consume(Token::Type::FALSE);
                falseLiteral->setLoc(falseToken);
                falseLiteral->val = false;

                literal = falseLiteral;
                break;
            }
            case Token::Type::STRING_LITERAL: {
                auto stringLiteral = std::make_shared<fir::StringLiteral>();

                const Token stringToken = consume(Token::Type::STRING_LITERAL);
                stringLiteral->setLoc(stringToken);
                stringLiteral->val = stringToken.str;

                literal = stringLiteral;
                break;
            }
//            case Token::Type::LA: {
//                const Token laToken = peek();
//
//                auto complexLiteral = std::make_shared<fir::ComplexLiteral>();
//                double_complex complexVal = parseComplexLiteral();
//                complexLiteral->setLoc(laToken);
//                complexLiteral->val = complexVal;
//
//                literal = complexLiteral;
//                break;
//            }
            case Token::Type::LB:
                literal = parseDenseTensorLiteral();
                break;
            default:
                reportError(peek(), "a tensor literal");
                throw SyntaxError();
                break;
        }

        return literal;
    }

// dense_tensor_literal: '[' dense_tensor_literal_inner ']'
    fir::DenseTensorLiteral::Ptr Parser::parseDenseTensorLiteral() {
        const Token leftBracketToken = consume(Token::Type::LB);
        fir::DenseTensorLiteral::Ptr tensor = parseDenseTensorLiteralInner();
        const Token rightBracketToken = consume(Token::Type::RB);

        tensor->setBeginLoc(leftBracketToken);
        tensor->setEndLoc(rightBracketToken);

        return tensor;
    }

// dense_tensor_literal_inner: dense_tensor_literal {[','] dense_tensor_literal}
//                           | dense_matrix_literal
    fir::DenseTensorLiteral::Ptr Parser::parseDenseTensorLiteralInner() {
        if (peek().type == Token::Type::LB) {
            auto tensor = std::make_shared<fir::NDTensorLiteral>();
            tensor->transposed = false;

            fir::DenseTensorLiteral::Ptr elem = parseDenseTensorLiteral();
            tensor->elems.push_back(elem);

            while (true) {
                switch (peek().type) {
                    case Token::Type::COMMA:
                        consume(Token::Type::COMMA);
                    case Token::Type::LB:
                        elem = parseDenseTensorLiteral();
                        tensor->elems.push_back(elem);
                        break;
                    default:
                        return tensor;
                }
            }
        }

        return parseDenseMatrixLiteral();
    }

// dense_matrix_literal: dense_vector_literal {';' dense_vector_literal}
    fir::DenseTensorLiteral::Ptr Parser::parseDenseMatrixLiteral() {
        auto mat = std::make_shared<fir::NDTensorLiteral>();
        mat->transposed = false;

        do {
            const fir::DenseTensorLiteral::Ptr vec = parseDenseVectorLiteral();
            mat->elems.push_back(vec);
        } while (tryConsume(Token::Type::SEMICOL));

        return (mat->elems.size() == 1) ? mat->elems[0] : mat;
    }

// dense_vector_literal: dense_int_vector_literal | dense_float_vector_literal
//                     | dense_complex_vector_literal
    fir::DenseTensorLiteral::Ptr Parser::parseDenseVectorLiteral() {
        fir::DenseTensorLiteral::Ptr vec;
        switch (peek().type) {
            case Token::Type::INT_LITERAL:
                vec = parseDenseIntVectorLiteral();
                break;
            case Token::Type::FLOAT_LITERAL:
                vec = parseDenseFloatVectorLiteral();
                break;
            case Token::Type::PLUS:
            case Token::Type::MINUS:
                switch (peek(1).type) {
                    case Token::Type::INT_LITERAL:
                        vec = parseDenseIntVectorLiteral();
                        break;
                    case Token::Type::FLOAT_LITERAL:
                        vec = parseDenseFloatVectorLiteral();
                        break;
                    default:
                        reportError(peek(), "a vector literal");
                        throw SyntaxError();
                        break;
                }
                break;
//            case Token::Type::LA:
//                vec = parseDenseComplexVectorLiteral();
//                break;
            default:
                reportError(peek(), "a vector literal");
                throw SyntaxError();
                break;
        }

        return vec;
    }

// dense_int_vector_literal: signed_int_literal {[','] signed_int_literal}
    fir::IntVectorLiteral::Ptr Parser::parseDenseIntVectorLiteral() {
        auto vec = std::make_shared<fir::IntVectorLiteral>();
        vec->transposed = false;

        int elem = parseSignedIntLiteral();
        vec->vals.push_back(elem);

        while (true) {
            switch (peek().type) {
                case Token::Type::COMMA:
                    consume(Token::Type::COMMA);
                case Token::Type::PLUS:
                case Token::Type::MINUS:
                case Token::Type::INT_LITERAL:
                    elem = parseSignedIntLiteral();
                    vec->vals.push_back(elem);
                    break;
                default:
                    return vec;
            }
        }
    }

// dense_float_vector_literal: signed_float_literal {[','] signed_float_literal}
    fir::FloatVectorLiteral::Ptr Parser::parseDenseFloatVectorLiteral() {
        auto vec = std::make_shared<fir::FloatVectorLiteral>();
        vec->transposed = false;

        double elem = parseSignedFloatLiteral();
        vec->vals.push_back(elem);

        while (true) {
            switch (peek().type) {
                case Token::Type::COMMA:
                    consume(Token::Type::COMMA);
                case Token::Type::PLUS:
                case Token::Type::MINUS:
                case Token::Type::FLOAT_LITERAL:
                    elem = parseSignedFloatLiteral();
                    vec->vals.push_back(elem);
                    break;
                default:
                    return vec;
            }
        }
    }

//// dense_complex_vector_literal: complex_literal {[','] complex_literal}
//    fir::ComplexVectorLiteral::Ptr Parser::parseDenseComplexVectorLiteral() {
//        auto vec = std::make_shared<fir::ComplexVectorLiteral>();
//        vec->transposed = false;
//
//        double_complex elem = parseComplexLiteral();
//        vec->vals.push_back(elem);
//
//        while (true) {
//            switch (peek().type) {
//                case Token::Type::COMMA:
//                    consume(Token::Type::COMMA);
//                case Token::Type::LA:
//                    elem = parseComplexLiteral();
//                    vec->vals.push_back(elem);
//                    break;
//                default:
//                    return vec;
//            }
//        }
//    }

// signed_int_literal: ['+' | '-'] INT_LITERAL
    int Parser::parseSignedIntLiteral() {
        int coeff = 1;
        switch (peek().type) {
            case Token::Type::PLUS:
                consume(Token::Type::PLUS);
                break;
            case Token::Type::MINUS:
                consume(Token::Type::MINUS);
                coeff = -1;
                break;
            default:
                break;
        }

        return (coeff * consume(Token::Type::INT_LITERAL).num);
    }

// signed_float_literal: ['+' | '-'] FLOAT_LITERAL
    double Parser::parseSignedFloatLiteral() {
        double coeff = 1.0;
        switch (peek().type) {
            case Token::Type::PLUS:
                consume(Token::Type::PLUS);
                break;
            case Token::Type::MINUS:
                consume(Token::Type::MINUS);
                coeff = -1.0;
                break;
            default:
                break;
        }

        return (coeff * consume(Token::Type::FLOAT_LITERAL).fnum);
    }

// complex_literal: '<' signed_float_literal ',' signed_float_literal '>'
//    double_complex Parser::parseComplexLiteral() {
//        consume(Token::Type::LA);
//        double real = parseSignedFloatLiteral();
//
//        consume(Token::Type::COMMA);
//
//        double imag = parseSignedFloatLiteral();
//        consume(Token::Type::RA);
//
//        return double_complex(real, imag);
//    }

// '%!' ident '(' [expr_params] ')' '==' expr ';'
    fir::Test::Ptr Parser::parseTest() {
        auto test = std::make_shared<fir::Test>();

        const Token testToken = consume(Token::Type::TEST);
        test->setBeginLoc(testToken);

        test->func = parseIdent();
        switch (peek().type) {
            case Token::Type::LP: {
                consume(Token::Type::LP);

                if (!tryConsume(Token::Type::RP)) {
                    test->args = parseExprParams();
                    consume(Token::Type::RP);
                }

                consume(Token::Type::EQ);
                test->expected = parseExpr();

                const Token endToken = consume(Token::Type::SEMICOL);
                test->setEndLoc(endToken);
                break;
            }
            case Token::Type::ASSIGN:
                // TODO: Implement.
            default:
                reportError(peek(), "a test");
                throw SyntaxError();
                break;
        }

        return test;
    }

    void Parser::reportError(const Token &token, std::string expected) {
        std::stringstream errMsg;
        errMsg << "expected " << expected << " but got " << token.toString();

        const auto err = ParseError(token.lineBegin, token.colBegin,
                                    token.lineEnd, token.colEnd, errMsg.str());
        errors->push_back(err);
    }

    void Parser::skipTo(std::vector<Token::Type> types) {
        while (peek().type != Token::Type::END) {
            for (const auto type : types) {
                if (peek().type == type) {
                    return;
                }
            }
            tokens.skip();
        }
    }

    Token Parser::consume(Token::Type type) {
        const Token token = peek();

        if (!tokens.consume(type)) {
            reportError(token, Token::tokenTypeString(type));
            throw SyntaxError();
        }

        return token;
    }

    //parse the vertexset type vertexset{ElementType}
    fir::VertexSetType::Ptr Parser::parseVertexSetType() {
        const Token setToken = consume(Token::Type::VERTEX_SET);
        consume(Token::Type::LC);
        const fir::ElementType::Ptr element = parseElementType();
        const Token rightCurlyToken = consume(Token::Type::RC);

        auto vertexSetType = std::make_shared<fir::VertexSetType>();
        vertexSetType->setBeginLoc(setToken);
        vertexSetType->element = element;
        vertexSetType->setEndLoc(rightCurlyToken);

        return vertexSetType;
    }

    // parses the list type list{Type}
    fir::ListType::Ptr Parser::parseListType() {
        const Token listToken = consume(Token::Type::LIST);
        consume(Token::Type::LC);
        const fir::Type::Ptr list_element_type = parseType();
        const Token rightCurlyToken = consume(Token::Type::RC);

        auto listType = std::make_shared<fir::ListType>();
        listType->setBeginLoc(listToken);
        listType->list_element_type = list_element_type;
        listType->setEndLoc(rightCurlyToken);

        return listType;
    }

    fir::Type::Ptr Parser::parseEdgeSetType() {

        auto edgeSetType = std::make_shared<fir::EdgeSetType>();

        const Token setToken = consume(Token::Type::EDGE_SET);
        consume(Token::Type::LC);
        const fir::ElementType::Ptr element = parseElementType();
        const Token rightCurlyToken = consume(Token::Type::RC);

        consume(Token::Type::LP);
        std::vector<fir::ElementType::Ptr> vertex_element_type_list;
        //do {

        // parse src element type
        const fir::ElementType::Ptr src_vertex_element_type = parseElementType();
        vertex_element_type_list.push_back(src_vertex_element_type);
        consume(Token::Type::COMMA);
        // parse dst element type
        const fir::ElementType::Ptr dst_vertex_element_type = parseElementType();
        vertex_element_type_list.push_back(dst_vertex_element_type);

        // if a third type argument is supplied, we assume that it is a weight
        if (tryConsume(Token::Type::COMMA)) {
            const fir::ScalarType::Ptr weight_type = parseScalarType();
            edgeSetType->weight_type = weight_type;
        }


        //} while (tryConsume(Token::Type::COMMA));

        const Token rightP = consume(Token::Type::RP);


        edgeSetType->setBeginLoc(setToken);
        edgeSetType->edge_element_type = element;
        edgeSetType->setEndLoc(rightCurlyToken);
        edgeSetType->vertex_element_type_list = vertex_element_type_list;
        return edgeSetType;
    }

    // added for parsing the allocation expression for GraphIt
    // new_expr: 'new' ('VertexSet'|'list'|'priority_queue')  '{' element_type '}' '(' [expr] ')'
    fir::NewExpr::Ptr Parser::parseNewExpr() {

        const Token newToken = consume(Token::Type::NEW);

        fir::NewExpr::Ptr output_new_expr;

        if (tryConsume(Token::Type::VERTEX_SET)) {
            output_new_expr = std::make_shared<fir::VertexSetAllocExpr>();

            consume(Token::Type::LC);
            const auto element_type = parseElementType();
            output_new_expr->elementType = element_type;
            consume(Token::Type::RC);

            consume(Token::Type::LP);
            if (tryConsume(Token::Type::RP)) {
                //no expression in the "( )"
            } else {
                const auto expr = parseExpr();
                output_new_expr->numElements = expr;
                consume(Token::Type::RP);
            }
        } else if (tryConsume(Token::Type::LIST)) {
            //allocating a new List
            output_new_expr = std::make_shared<fir::ListAllocExpr>();
            consume(Token::Type::LC);
            const auto list_element_type = parseType();
            output_new_expr->general_element_type = list_element_type;
            consume(Token::Type::RC);

            consume(Token::Type::LP);
            if (tryConsume(Token::Type::RP)) {
                //no expression in the "( )"
            } else {
                const auto expr = parseExpr();
                output_new_expr->numElements = expr;
                consume(Token::Type::RP);
            }
        } else if (tryConsume(Token::Type::PRIORITY_QUEUE)) {
            auto priority_queue_expr = std::make_shared<fir::PriorityQueueAllocExpr>();

            output_new_expr = priority_queue_expr;

            consume(Token::Type::LC);
            const auto element_type = parseElementType();
            priority_queue_expr->elementType = element_type;
            consume(Token::Type::RC);

            consume(Token::Type::LP);
            const auto priority_type = parseScalarType();
            priority_queue_expr->priority_type = priority_type;
            consume(Token::Type::RP);

            consume(Token::Type::LP);
            priority_queue_expr->dup_within_bucket = parseExpr();
            consume(Token::Type::COMMA);
            priority_queue_expr->dup_across_bucket = parseExpr();
            consume(Token::Type::COMMA);
            priority_queue_expr->vector_function = parseIdent();
            consume(Token::Type::COMMA);
            priority_queue_expr->bucket_ordering = parseExpr();
            consume(Token::Type::COMMA);
            priority_queue_expr->priority_ordering = parseExpr();
            consume(Token::Type::COMMA);
            priority_queue_expr->init_bucket = parseExpr();
            consume(Token::Type::COMMA);
            priority_queue_expr->starting_node = parseExpr();
            consume(Token::Type::RP);
        } else if (peek().type == Token::Type::VECTOR){
	    fir::NDTensorType::Ptr tensor_type = parseVectorBlockType();

	    auto vector_alloc_expr = std::make_shared<fir::VectorAllocExpr>();
	    auto element_type = tensor_type->element;
            if (element_type != nullptr) {
	        vector_alloc_expr->elementType = element_type;
                
            }else {
                fir::IndexSet::Ptr set = tensor_type->indexSets[0];
		fir::IntLiteral::Ptr numElements = std::make_shared<fir::IntLiteral>();
		numElements->val = fir::to<fir::RangeIndexSet>(set)->range;
		vector_alloc_expr->numElements = numElements;
            }	 
	    if (fir::isa<fir::ScalarType>(tensor_type->blockType)) {
                vector_alloc_expr->vector_scalar_type = fir::to<fir::ScalarType>(tensor_type->blockType);
	    } else if (fir::isa<fir::NDTensorType>(tensor_type->blockType)){
                vector_alloc_expr->general_element_type = fir::to<fir::NDTensorType>(tensor_type->blockType);
                vector_alloc_expr->vector_scalar_type = nullptr;
            } else {
                std::cout << "Unsupported Vector Element Type " << std::endl;
	    }
	    
            output_new_expr = vector_alloc_expr;
            consume(Token::Type::LP);
            consume(Token::Type::RP);
        } else {
            reportError(peek(), "do not support this new expression");
        }

        output_new_expr->setBeginLoc(newToken);
        // TODO: fix the hack for the end line number
        output_new_expr->setEndLoc(newToken);

        return output_new_expr;

        //const Token vertexSetToken = consume(Token::Type::VERTEX_SET);
//        consume(Token::Type::LC);
//        const fir::ElementType::Ptr element_type = parseElementType();
//        consume(Token::Type::RC);
//
//        consume(Token::Type::LP);
//
//        if (tryConsume(Token::Type::RP)){
//            // no expression in parenthesis
//
//        } else {
//            //expression in parenthesis
//
//        }
//
    }

    // load_expr: 'load' '(' expr ')'
    fir::LoadExpr::Ptr Parser::parseLoadExpr() {

        const Token newToken = consume(Token::Type::LOAD);
        const auto load_expr = std::make_shared<fir::EdgeSetLoadExpr>();

        consume(Token::Type::LP);
        const auto file_name = parseExpr();
        consume(Token::Type::RP);
        load_expr->file_name = file_name;
        return load_expr;
    }

    // intersection_expr: ('intersection') '(' 'expr', 'expr', 'expr', 'expr' ')' ';'
    fir::IntersectionExpr::Ptr Parser::parseIntersectionExpr() {

        const auto intersectionExpr = std::make_shared<fir::IntersectionExpr>();

        const Token intersectionToken = consume(Token::Type::INTERSECTION);
        consume(Token::Type::LP);

        const auto vertexA = parseExpr();
        consume(Token::Type::COMMA);
        const auto vertexB = parseExpr();
        consume(Token::Type::COMMA);
        const auto numA = parseExpr();
        consume(Token::Type::COMMA);
        const auto numB = parseExpr();

        intersectionExpr->vertex_a = vertexA;
        intersectionExpr->vertex_b = vertexB;
        intersectionExpr->numA = numA;
        intersectionExpr->numB = numB;

        if (tryConsume(Token::Type::COMMA)) {
            const auto reference = parseExpr();
            intersectionExpr->reference = reference;
        }

        consume(Token::Type::RP);
        return intersectionExpr;
    }

    void Parser::initIntrinsics() {
        // set up method call intrinsics

        //TODO: this one might need to be removed
        intrinsics_.push_back("sum");

        //library functions for edgeset
        intrinsics_.push_back("getVertices");
        intrinsics_.push_back("getOutDegrees");
        intrinsics_.push_back("getOutDegreesUint");
        intrinsics_.push_back("getOutDegree");
        intrinsics_.push_back("getNgh");
        intrinsics_.push_back("relabel");

        // library functions for vertexset
        intrinsics_.push_back("getVertexSetSize");
        intrinsics_.push_back("addVertex");

        //library functions for list
        intrinsics_.push_back("append");
        intrinsics_.push_back("pop");
        intrinsics_.push_back("transpose");

        // set up function call intrinsics
        decls.insert("fabs", IdentType::FUNCTION);
        decls.insert("startTimer", IdentType::FUNCTION);
        decls.insert("stopTimer", IdentType::FUNCTION);
        decls.insert("atoi", IdentType::FUNCTION);
        decls.insert("floor", IdentType::FUNCTION);
        decls.insert("log", IdentType::FUNCTION);
        decls.insert("to_double", IdentType::FUNCTION);
        decls.insert("max", IdentType::FUNCTION);
        decls.insert("writeMin", IdentType::FUNCTION);
	    decls.insert("getRandomOutNgh", IdentType::FUNCTION);
        decls.insert("getRandomInNgh", IdentType::FUNCTION);
        decls.insert("serialMinimumSpanningTree", IdentType::FUNCTION);
    }

    fir::BreakStmt::Ptr Parser::parseBreakStmt() {
        consume(Token::Type::BREAK);
        consume(Token::Type::SEMICOL);
        return std::make_shared<fir::BreakStmt>();
    }


    //OG Additions

    //parse the priority_queue type priority_queue{ElementType}
    fir::PriorityQueueType::Ptr Parser::parsePriorityQueueType() {
        const Token queueToken = consume(Token::Type::PRIORITY_QUEUE);
        consume(Token::Type::LC);
        const fir::ElementType::Ptr element = parseElementType();
        const Token rightCurlyToken = consume(Token::Type::RC);

        consume(Token::Type::LP);
        //parse the type of the priority
        const fir::ScalarType::Ptr priority_type = parseScalarType();
        const Token RP_token = consume(Token::Type::RP);


        auto priorityQueueType = std::make_shared<fir::PriorityQueueType>();
        priorityQueueType->setBeginLoc(queueToken);
        priorityQueueType->element = element;
        priorityQueueType->priority_type = priority_type;
        priorityQueueType->setEndLoc(RP_token);

        return priorityQueueType;
    }

}

