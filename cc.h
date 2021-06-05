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

    struct ASTPosition
    {
        uint32_t line;
        uint32_t col;

        explicit ASTPosition(const ASTPosition* position) : line(position->line), col(position->col) {}
        // ASTPosition(uint32_t line, uint32_t col) : line(line), col(col) {}
    };


    struct ASTValue : public Value, public ASTPosition
    {
        explicit ASTValue(const ASTPosition* position) : ASTPosition(position) {}
        // ASTValue(uint32_t line, uint32_t start_col) : ASTPosition(line, start_col) {}

        typedef void (*TraverseCB)(ASTValue*, Context* ctx, void*);
        virtual void traverse(TraverseCB cb, Context* ctx, void* data) { cb(this, ctx, data); };
        virtual void resolution_pass(Context* context) {};
    };

    struct ASTException : Exception
    {
        const ASTPosition self;
        ASTException(const ASTPosition* position, const std::string& what) :
            Exception(what), self(*position)
        {
        }
    };

    struct TypeDecl : public ASTValue
    {
        const Type* type;
        std::string name;
        Variable* variable;

        TypeDecl(const ASTPosition* position, const Type* type, const char* name_) :
                ASTValue(position), type(type), variable(nullptr)
        {
            TAKE_STRING(name, name_);
        }

        TypeDecl(Context* ctx,
                 const ASTPosition* position,
                 const char* type_ident, const char* name_);

        void resolution_pass(Context* context) override;
    };

    struct FieldDecl : public ASTValue
    {
        TypeDecl* decl;
        FieldDecl* next;

        FieldDecl(const ASTPosition* position, TypeDecl* decl) :
                  ASTValue(position), decl(decl), next(nullptr) {}

        ~FieldDecl() override
        {
            delete next;
        }
    };

    struct Buildable : public ASTValue
    {
        virtual void add(Context* ctx, IRBuilder &IRB) const = 0;
        explicit Buildable(const ASTValue* values) : ASTValue(values) {}
        explicit Buildable(const ASTPosition* position) : ASTValue(position) {}
    };

    struct Expression : public ASTValue
    {
        explicit Expression(const ASTValue* values) : ASTValue(values) {}
        explicit Expression(const ASTPosition* position) : ASTValue(position) {}

        virtual Constant* get_constant(Context* ctx) const = 0;
        virtual const IR* get(Context* ctx, IRBuilder &IRB) const = 0;
    };

    struct Statement : public Buildable
    {
        explicit Statement(const ASTValue* values) : Buildable(values) {}
        explicit Statement(const ASTPosition* position) : Buildable(position) {}
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
        explicit Continue(const ASTPosition* position) : Statement(position) {}
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Break : public Statement
    {
        explicit Break(const ASTPosition* position) : Statement(position) {}
        void add(Context* ctx, IRBuilder &IRB) const override;
    };

    struct Return : public Statement
    {
        Expression* return_value;

        explicit Return(const ASTPosition* position) :
            Statement(position), return_value(nullptr) {}
        explicit Return(const ASTPosition* position, Expression* return_value) :
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

        Constant* get_constant(Context* ctx) const override;
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
        Constant* get_constant(Context* ctx) const override;
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

    struct ASTConstant : public Expression, public Constant
    {
        explicit ASTConstant(const ASTPosition* position) : Expression(position) {}
        const IR* get(Context* ctx, IRBuilder &IRB) const override;
    };

    struct NumericExpr : public ASTConstant
    {
        enum numeric_type_t
        {
            INTEGER,
            FLOATING
        };

        union {
            int64_t integer;
            double floating;
        } value;
        numeric_type_t type;

        template<typename T>
        explicit NumericExpr(const ASTPosition* position,
                             numeric_type_t type,
                             T value_)
        : ASTConstant(position),
        type(type), value({0})
        {
            if (type == INTEGER) value.integer = value_;
            else if (type == FLOATING) value.floating = value_;
        }

        size_t get_size() const override { return sizeof(value); }
        void write(void* buffer) const override
        {
            *static_cast<int64_t*>(buffer) = value.integer;
        }

        Constant* get_constant(Context* ctx) const override { return new NumericExpr(this, type, value.integer); }
        std::string as_string() const override
        {
            return "Imm(" + ((type == FLOATING) ? std::to_string(value.floating) : std::to_string(value.integer)) + ")";
        }

        ALL_OPERATORS_DECL
    };

    struct LiteralExpr : public ASTConstant
    {
        std::string value;
        explicit LiteralExpr(const ASTPosition* position, const char* value_) : ASTConstant(position)
        {
            TAKE_STRING(value, value_);
        }

        explicit LiteralExpr(const ASTPosition* position, std::string value) :
                ASTConstant(position), value(std::move(value)) {}

        size_t get_size() const override { return value.length() + 1; }
        void write(void* buffer) const override;
        Constant* get_constant(Context* ctx) const override { return new LiteralExpr(this, value); }
        std::string as_string() const override { return variadic_string("%%%d=%s", get_id(), value.c_str()); }

        ALL_OPERATORS_DECL
    };

    struct ConstantExpr : public ASTConstant
    {
        Constant* constant;
        explicit ConstantExpr(const ASTPosition* position, Constant* constant) :
                ASTConstant(position), constant(constant) {}

        std::string get_name() const;

        size_t get_size() const override { return constant->get_size(); }
        void write(void* buffer) const override { constant->write(buffer); }
        Constant* get_constant(Context* ctx) const override { return constant; }
        std::string as_string() const override { return constant->as_string(); }

        ALL_OPERATORS_DECL
    };

    struct VariableExpr : public Expression
    {
        std::string variable;
        Variable* value;

        explicit VariableExpr(const ASTPosition* position, const char* value_) : Expression(position), value(nullptr)
        {
            TAKE_STRING(variable, value_);
        }

        void resolution_pass(Context* context) override;
        Constant* get_constant(Context* ctx) const override;
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

        explicit CallExpr(const ASTPosition* position, const char* name, CallArguments* arguments = nullptr) :
                Expression(position), arguments(arguments)
        {
            TAKE_STRING(function, name);
        }

        ~CallExpr() override
        {
            delete arguments;
        }

        Constant* get_constant(Context* ctx) const override;
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

        Constant* get_constant(Context* ctx) const override;
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
        explicit ASTGlobal(const ASTPosition* position) : Buildable(position), next(nullptr), symbol(nullptr) {}

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

        ASTFunction(const ASTPosition* position, const Type* return_type, const char* name_, Arguments* args)
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
        ASTFunctionDefine(const ASTPosition* position, const Type* return_type, const char* name_,
                          Arguments* args, MultiStatement* body)
                : ASTFunction(position, return_type, name_, args), body(body) {}

        void add(Context* ctx, IRBuilder &IRB) const override;

        ~ASTFunctionDefine() override
        {
            delete body;
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
    };

    struct ASTGlobalVariable : public ASTGlobal
    {
        TypeDecl* decl;
        Constant* initializer;
        Variable* variable;

        explicit ASTGlobalVariable(Decl* decl_stmt) :
        ASTGlobal(decl_stmt->decl), decl(decl_stmt->decl),
        variable(nullptr), initializer(nullptr)
        {
            if (dynamic_cast<DeclInit*>(decl_stmt))
            {
                initializer = dynamic_cast<DeclInit*>(decl_stmt)->initializer->get_constant(nullptr);
            }
        }

        void traverse(TraverseCB cb, Context* ctx, void* data) override;
        void add(Context* ctx, IRBuilder &IRB) const override;

        void resolution_pass(Context* context) override;
    };

    struct StructDecl : public ASTGlobal
    {
        std::string name;
        FieldDecl* fields;
        const Type* type;

        StructDecl(const ASTPosition* position, Context* ctx,
                   std::string name, FieldDecl* fields);

        StructDecl(const ASTPosition* position, Context* ctx, FieldDecl* fields) :
                StructDecl(position, ctx, get_anonymous_name(position), fields) {}

        StructDecl(const ASTPosition* position, Context* ctx,
                   char* name_, FieldDecl* fields) :
                StructDecl(position, ctx, std::string(name_), fields)
        {
            free(name_);
        }

        const Type* get_type() const { return type; }
        void add(Context* ctx, IRBuilder &IRB) const override { };

        static std::string get_anonymous_name(const ASTPosition* position)
        {
            return variadic_string(".anonymous.structure@%d:%d", position->line, position->col);
        }

        ~StructDecl() override
        {
            delete fields;
        }
    };
}

#endif //CC_H
