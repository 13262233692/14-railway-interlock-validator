#include "boolean/BooleanExpr.h"
#include <algorithm>

namespace railway {
namespace boolean {

ConstantExpr::ConstantExpr(bool value) : value_(value) {
}

ExprType ConstantExpr::getType() const {
    return ExprType::CONSTANT;
}

std::string ConstantExpr::toString() const {
    return value_ ? "true" : "false";
}

std::shared_ptr<BooleanExpr> ConstantExpr::clone() const {
    return std::make_shared<ConstantExpr>(value_);
}

bool ConstantExpr::getValue() const {
    return value_;
}

void ConstantExpr::collectVariables(std::vector<std::string>& outVars) const {
}

VariableExpr::VariableExpr(const std::string& name) : name_(name) {
}

ExprType VariableExpr::getType() const {
    return ExprType::VARIABLE;
}

std::string VariableExpr::toString() const {
    return name_;
}

std::shared_ptr<BooleanExpr> VariableExpr::clone() const {
    return std::make_shared<VariableExpr>(name_);
}

const std::string& VariableExpr::getName() const {
    return name_;
}

void VariableExpr::collectVariables(std::vector<std::string>& outVars) const {
    if (std::find(outVars.begin(), outVars.end(), name_) == outVars.end()) {
        outVars.push_back(name_);
    }
}

UnaryExpr::UnaryExpr(ExprType type, std::shared_ptr<BooleanExpr> operand)
    : type_(type), operand_(operand) {
}

ExprType UnaryExpr::getType() const {
    return type_;
}

std::shared_ptr<BooleanExpr> UnaryExpr::getOperand() const {
    return operand_;
}

void UnaryExpr::collectVariables(std::vector<std::string>& outVars) const {
    if (operand_) {
        operand_->collectVariables(outVars);
    }
}

NotExpr::NotExpr(std::shared_ptr<BooleanExpr> operand)
    : UnaryExpr(ExprType::NOT, operand) {
}

std::string NotExpr::toString() const {
    if (!operand_) return "!()";
    if (operand_->isVariable() || operand_->isConstant()) {
        return "!" + operand_->toString();
    }
    return "!(" + operand_->toString() + ")";
}

std::shared_ptr<BooleanExpr> NotExpr::clone() const {
    return std::make_shared<NotExpr>(operand_ ? operand_->clone() : nullptr);
}

BinaryExpr::BinaryExpr(ExprType type, std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right)
    : type_(type), left_(left), right_(right) {
}

ExprType BinaryExpr::getType() const {
    return type_;
}

std::shared_ptr<BooleanExpr> BinaryExpr::getLeft() const {
    return left_;
}

std::shared_ptr<BooleanExpr> BinaryExpr::getRight() const {
    return right_;
}

void BinaryExpr::collectVariables(std::vector<std::string>& outVars) const {
    if (left_) {
        left_->collectVariables(outVars);
    }
    if (right_) {
        right_->collectVariables(outVars);
    }
}

AndExpr::AndExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right)
    : BinaryExpr(ExprType::AND, left, right) {
}

std::string AndExpr::toString() const {
    std::string leftStr = left_ ? left_->toString() : "";
    std::string rightStr = right_ ? right_->toString() : "";
    if (left_ && (left_->getType() == ExprType::OR || left_->getType() == ExprType::IMPLIES)) {
        leftStr = "(" + leftStr + ")";
    }
    if (right_ && (right_->getType() == ExprType::OR || right_->getType() == ExprType::IMPLIES)) {
        rightStr = "(" + rightStr + ")";
    }
    return leftStr + " & " + rightStr;
}

std::shared_ptr<BooleanExpr> AndExpr::clone() const {
    return std::make_shared<AndExpr>(
        left_ ? left_->clone() : nullptr,
        right_ ? right_->clone() : nullptr
    );
}

OrExpr::OrExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right)
    : BinaryExpr(ExprType::OR, left, right) {
}

std::string OrExpr::toString() const {
    std::string leftStr = left_ ? left_->toString() : "";
    std::string rightStr = right_ ? right_->toString() : "";
    return leftStr + " | " + rightStr;
}

std::shared_ptr<BooleanExpr> OrExpr::clone() const {
    return std::make_shared<OrExpr>(
        left_ ? left_->clone() : nullptr,
        right_ ? right_->clone() : nullptr
    );
}

ImpliesExpr::ImpliesExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right)
    : BinaryExpr(ExprType::IMPLIES, left, right) {
}

std::string ImpliesExpr::toString() const {
    std::string leftStr = left_ ? left_->toString() : "";
    std::string rightStr = right_ ? right_->toString() : "";
    if (left_ && left_->getType() == ExprType::OR) {
        leftStr = "(" + leftStr + ")";
    }
    return leftStr + " -> " + rightStr;
}

std::shared_ptr<BooleanExpr> ImpliesExpr::clone() const {
    return std::make_shared<ImpliesExpr>(
        left_ ? left_->clone() : nullptr,
        right_ ? right_->clone() : nullptr
    );
}

EquivExpr::EquivExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right)
    : BinaryExpr(ExprType::EQUIV, left, right) {
}

std::string EquivExpr::toString() const {
    std::string leftStr = left_ ? left_->toString() : "";
    std::string rightStr = right_ ? right_->toString() : "";
    return leftStr + " <-> " + rightStr;
}

std::shared_ptr<BooleanExpr> EquivExpr::clone() const {
    return std::make_shared<EquivExpr>(
        left_ ? left_->clone() : nullptr,
        right_ ? right_->clone() : nullptr
    );
}

XorExpr::XorExpr(std::shared_ptr<BooleanExpr> left, std::shared_ptr<BooleanExpr> right)
    : BinaryExpr(ExprType::XOR, left, right) {
}

std::string XorExpr::toString() const {
    std::string leftStr = left_ ? left_->toString() : "";
    std::string rightStr = right_ ? right_->toString() : "";
    return leftStr + " ^ " + rightStr;
}

std::shared_ptr<BooleanExpr> XorExpr::clone() const {
    return std::make_shared<XorExpr>(
        left_ ? left_->clone() : nullptr,
        right_ ? right_->clone() : nullptr
    );
}

} // namespace boolean
} // namespace railway
