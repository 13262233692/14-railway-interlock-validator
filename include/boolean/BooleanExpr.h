#ifndef RAILWAY_BOOLEAN_EXPR_H
#define RAILWAY_BOOLEAN_EXPR_H

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace railway {
namespace boolean {

enum class ExprType {
    CONSTANT,
    VARIABLE,
    NOT,
    AND,
    OR,
    IMPLIES,
    EQUIV,
    XOR
};

class BooleanExpr {
public:
    virtual ~BooleanExpr() = default;

    virtual ExprType getType() const = 0;
    virtual std::string toString() const = 0;
    virtual std::shared_ptr<BooleanExpr> clone() const = 0;

    virtual bool isConstant() const { return false; }
    virtual bool isVariable() const { return false; }

    virtual void collectVariables(std::vector<std::string>& outVars) const = 0;
};

class ConstantExpr : public BooleanExpr {
public:
    explicit ConstantExpr(bool value);

    ExprType getType() const override;
    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;

    bool isConstant() const override { return true; }
    bool getValue() const;

    void collectVariables(std::vector<std::string>& outVars) const override;

private:
    bool value_;
};

class VariableExpr : public BooleanExpr {
public:
    explicit VariableExpr(const std::string& name);

    ExprType getType() const override;
    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;

    bool isVariable() const override { return true; }
    const std::string& getName() const;

    void collectVariables(std::vector<std::string>& outVars) const override;

private:
    std::string name_;
};

class UnaryExpr : public BooleanExpr {
public:
    UnaryExpr(ExprType type, std::shared_ptr<BooleanExpr> operand);

    ExprType getType() const override;
    std::shared_ptr<BooleanExpr> getOperand() const;

    void collectVariables(std::vector<std::string>& outVars) const override;

protected:
    ExprType type_;
    std::shared_ptr<BooleanExpr> operand_;
};

class NotExpr : public UnaryExpr {
public:
    explicit NotExpr(std::shared_ptr<BooleanExpr> operand);

    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;
};

class BinaryExpr : public BooleanExpr {
public:
    BinaryExpr(ExprType type, std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right);

    ExprType getType() const override;
    std::shared_ptr<BooleanExpr> getLeft() const;
    std::shared_ptr<BooleanExpr> getRight() const;

    void collectVariables(std::vector<std::string>& outVars) const override;

protected:
    ExprType type_;
    std::shared_ptr<BooleanExpr> left_;
    std::shared_ptr<BooleanExpr> right_;
};

class AndExpr : public BinaryExpr {
public:
    AndExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right);

    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;
};

class OrExpr : public BinaryExpr {
public:
    OrExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right);

    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;
};

class ImpliesExpr : public BinaryExpr {
public:
    ImpliesExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right);

    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;
};

class EquivExpr : public BinaryExpr {
public:
    EquivExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right);

    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;
};

class XorExpr : public BinaryExpr {
public:
    XorExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right);

    std::string toString() const override;
    std::shared_ptr<BooleanExpr> clone() const override;
};

} // namespace boolean
} // namespace railway

#endif // RAILWAY_BOOLEAN_EXPR_H
