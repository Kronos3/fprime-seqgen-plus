#ifndef CC_H
#define CC_H

#include <string>
#include <unordered_map>
#include <common/common.h>

#define TAKE_STRING(out, v) do { \
    if (!(v)) throw Exception("Invalid NULL identifier!"); \
    (out) = std::string(v);        \
    free((void*)(v)); \
} while (0)

namespace cc
{
    class Context;
    class Variable;
    class IRBuilder;
    class Type;
    class StructType;
    class Global;
    class Function;
    class GlobalVariable;

    struct ASTValue : public Value
    {
        struct TokenPosition
        {
            uint32_t line;
            uint32_t start_col;
        };

        uint32_t line;
        uint32_t col;

        explicit ASTValue(const ASTValue* v): line(v->line), col(v->col) {}
        explicit ASTValue(const TokenPosition* position) : line(position->line), col(position->start_col) {}
        ASTValue(uint32_t line, uint32_t start_col) : line(line), col(start_col) {}

        typedef void (*TraverseCB)(ASTValue*, Context* ctx, void*);
        virtual void traverse(TraverseCB cb, Context* ctx, void* data) { cb(this, ctx, data); };
        virtual void resolution_pass(Context* context) {};
    };

    struct ASTException : Exception
    {
        ASTValue self;
        ASTException(const ASTValue* self, const std::string& what) :
            Exception(what), self(*self) {}

        ASTException(const ASTValue::TokenPosition* position, const std::string& what) :
            Exception(what), self(position)
        {
        }
    };

    struct TypeDecl : public ASTValue
    {
        const Type* type;
        std::string name;
        Variable* variable;

        TypeDecl(const TokenPosition* position, const Type* type, const char* name_) :
                ASTValue(position), type(type), variable(nullptr)
        {
            TAKE_STRING(name, name_);
        }

        void resolution_pass(Context* context) override;
    };

    struct FieldDecl : public ASTValue
    {
        const Type* type;
        std::string name;
        FieldDecl* next;

        FieldDecl(const TokenPosition* position, const Type* type, const char* name_) :
                  ASTValue(position), type(type), next(nullptr)
        {
            TAKE_STRING(name, name_);
        }

        ~FieldDecl() override
        {
            delete next;
        }
    };

    struct Buildable : public ASTValue
    {
        virtual void add(Context* ctx, IRBuilder &IRB) const = 0;
        explicit Buildable(const ASTValue* values) : ASTValue(values) {}
        explicit Buildable(const TokenPosition* position) : ASTValue(position) {}
    };

    struct Expression : public ASTValue
    {
        explicit Expression(const ASTValue* values) : ASTValue(values) {}
        explicit Expression(const ASTValue::TokenPosition* position) : ASTValue(position) {}
        virtual const IR* get(Context* ctx, IRBuilder &IRB) const = 0;
    };
    
    struct Statement : public Buildable
    {
        explicit Statement(const ASTValue* values) : Buildable(values) {}
        explicit Statement(const ASTValue::TokenPosition* position) : Buildable(position) {}
    };
    
    struct Loop : public Statement
    {
        Expression* conditional;
        Statement* body;
        explicit Loop(Expression* conditional)
        : Statement(conditional), conditional(conditional), body(nullptr) {}

        ~Loop() override
        {
            delete conditional;
            delete body;
        }
    };

    struct ForLoop : public Loop
    {
        Statement* initial;
        Expression* increment;
        ForLoop(Statement* initial, Expression* conditional, Expression* increment)
        : Loop(conditional), initial(initial), increment(increment) {}

        ~ForLoop() override
        {
            delete initial;
            delete increment;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct WhileLoop : public Loop
    {
        explicit WhileLoop(Expression* conditional) : Loop(conditional) {}
        void add(Context* ctx, IRBuilder &IRB) const override;
        void traverse(TraverseCB cb, Context* ctx, void* data) override;
    };


    struct MultiStatement : public Statement
    {
        Statement* self;
        MultiStatement* next;
        explicit MultiStatement(Statement* self) :
        Statement(self), self(self), next(nullptr) {}

        ~MultiStatement() override
        {
            delete self;
            delete next;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Decl : public Statement
    {
        TypeDecl* decl;

        explicit Decl(TypeDecl* decl) :
                Statement(decl), decl(decl) {}
        ~Decl() override
        {
            delete decl;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct DeclInit : public Decl
    {
        // TODO Possibly deprecate
        Expression* initializer;

        DeclInit(TypeDecl* variable, Expression* initializer)
        : Decl(variable), initializer(initializer) {}

        ~DeclInit() override
        {
            delete initializer;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Eval : public Statement
    {
        Expression* expr;
        explicit Eval(Expression* expr) :
                Statement(expr), expr(expr) {}

        ~Eval() override
        {
            delete expr;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct If : public Statement
    {
        Expression* clause;
        Statement* then_stmt;
        Statement* else_stmt;

        explicit If(Expression* clause) :
                Statement(clause), clause(clause), then_stmt(nullptr), else_stmt(nullptr) {}
        ~If() override
        {
            delete clause;
            delete then_stmt;
            delete else_stmt;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Continue : public Statement
    {
        explicit Continue(const ASTValue::TokenPosition* position) : Statement(position) {}
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Break : public Statement
    {
        explicit Break(const ASTValue::TokenPosition* position) : Statement(position) {}
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Return : public Statement
    {
        Expression* return_value;

        explicit Return(const ASTValue::TokenPosition* position) :
            Statement(position), return_value(nullptr) {}
        explicit Return(const ASTValue::TokenPosition* position, Expression* return_value) :
            Statement(position), return_value(return_value) {}

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct BinaryExpr : public Expression
    {
        enum binary_operator_t
        {
            // Arithmetic expressions
            A_ADD,
            A_SUB,
            A_DIV,
            A_MUL,

            // Bitwise expressions
            B_AND,
            B_OR,
            B_XOR,
            // TODO Shift

            // Logical comparison expressions
            L_LT,
            L_GT,
            L_LE,
            L_GE,
            L_EQ,
            L_AND,
            L_OR,
        };

        Expression* a;
        Expression* b;
        binary_operator_t op;

        BinaryExpr(Expression* a, Expression* b, binary_operator_t op)
                : Expression(a), a(a), b(b), op(op) {}

        ~BinaryExpr() override
        {
            delete a;
            delete b;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
    };

    struct UnaryExpr : public Expression
    {
        enum unary_operator_t
        {
            B_NOT,      //!< Bitwise NOT (~)
            L_NOT,      //!< Logical NOT (!)
            INC_PRE,    //!< ++x
            INC_POST,   //!< x++
            DEC_PRE,    //!< --x
            DEC_POST    //!< x--
        };

        Expression* operand;
        unary_operator_t op;
        UnaryExpr(Expression* operand, unary_operator_t op) :
                Expression(operand), operand(operand), op(op) {}
        ~UnaryExpr() override
        {
            delete operand;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
    };

    struct ASTConstant : public Constant, public Expression
    {
        explicit ASTConstant(const ASTValue::TokenPosition* position) : Expression(position) {}
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
        virtual std::string get_name() const = 0;
    };

    struct ImmIntExpr : public ASTConstant
    {
        int value;
        explicit ImmIntExpr(const TokenPosition* position, int value) : ASTConstant(position), value(value) {}

        size_t get_size() const override { return 4; }
        void write(void* buffer) const override
        {
            *static_cast<int*>(buffer) = value;
        }

        std::string get_name() const override;
    };

    struct ImmFloatExpr : public ASTConstant
    {
        double value;
        explicit ImmFloatExpr(const TokenPosition* position, double value) : ASTConstant(position), value(value) {}

        size_t get_size() const override { return 8; }
        void write(void* buffer) const override
        {
            *static_cast<double*>(buffer) = value;
        }

        std::string get_name() const override;
        std::string as_string() const override { return variadic_string("%%%d=%f", get_id(), value); }
    };

    struct LiteralExpr : public ASTConstant
    {
        std::string value;
        explicit LiteralExpr(const TokenPosition* position, const char* value_) : ASTConstant(position)
        {
            TAKE_STRING(value, value_);
        }

        size_t get_size() const override { return value.length() + 1; }
        void write(void* buffer) const override;
        std::string get_name() const override;
        std::string as_string() const override { return variadic_string("%%%d=%s", get_id(), value.c_str()); }
    };

    struct VariableExpr : public Expression
    {
        std::string variable;
        Variable* value;

        explicit VariableExpr(const TokenPosition* position, const char* value_) : Expression(position), value(nullptr)
        {
            TAKE_STRING(variable, value_);
        }

        void resolution_pass(Context* context) override;
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
    };

    struct CallArguments : ASTValue
    {
        Expression* value;
        CallArguments* next;

        explicit CallArguments(Expression* value) :
        ASTValue(value), value(value), next(nullptr) {}
        ~CallArguments() override
        {
            delete value;
            delete next;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
    };

    struct CallExpr : public Expression
    {
        std::string function;
        CallArguments* arguments;

        explicit CallExpr(const TokenPosition* position, const char* name, CallArguments* arguments = nullptr) :
                Expression(position), arguments(arguments)
        {
            TAKE_STRING(function, name);
        }

        ~CallExpr() override
        {
            delete arguments;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
    };

    struct AssignExpr: public Expression
    {
        Expression* sink;
        Expression* value;
        AssignExpr(Expression* sink, Expression* value) :
                Expression(sink), sink(sink), value(value) {}

        ~AssignExpr() override
        {
            delete sink;
            delete value;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Arguments : ASTValue
    {
        TypeDecl* decl;
        Arguments* next;

        explicit Arguments(TypeDecl* decl) :
                ASTValue(decl), decl(decl), next(nullptr) {}
        ~Arguments() override
        {
            delete decl;
            delete next;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
    };

    struct ASTGlobal : public Buildable
    {
        Global* symbol;
        ASTGlobal* next;

        explicit ASTGlobal(const ASTValue* values) : Buildable(values), next(nullptr), symbol(nullptr) {}
        explicit ASTGlobal(const TokenPosition* position) : Buildable(position), next(nullptr), symbol(nullptr) {}

        ~ASTGlobal() override
        {
            delete next;
        }
    };

    struct ASTFunction : public ASTGlobal
    {
        std::string name;
        const Type* return_type;
        Arguments* args;

        ASTFunction(const TokenPosition* position, const Type* return_type, const char* name_, Arguments* args)
                : ASTGlobal(position), return_type(return_type), args(args)
        {
            TAKE_STRING(name, name_);
        }

        void add(Context* ctx, IRBuilder &IRB) const override { /* forward declaration */ };

        ~ASTFunction() override
        {
            delete args;
        }

        void resolution_pass(Context* context) override;
    };

    struct ASTFunctionDefine : public ASTFunction
    {
        MultiStatement* body;
        ASTFunctionDefine(const TokenPosition* position, const Type* return_type, const char* name_,
                          Arguments* args, MultiStatement* body)
                : ASTFunction(position, return_type, name_, args), body(body) {}

        void add(Context* ctx, IRBuilder &IRB) const override;

        ~ASTFunctionDefine() override
        {
            delete body;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
    };

    struct ASTGlobalVariable : ASTGlobal
    {
        TypeDecl* decl;
        Expression* initializer;
        Variable* variable;

        explicit ASTGlobalVariable(TypeDecl* decl, Expression* initializer = nullptr) :
                ASTGlobal(decl), decl(decl), variable(nullptr), initializer(initializer) {}

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;

        void resolution_pass(Context* context) override;
    };
}

#endif //CC_H
