#include "boolean/BooleanEvaluator.h"
#include "boolean/BooleanParser.h"
#include <stdexcept>
#include <iostream>

namespace railway {
namespace boolean {

BooleanEvaluator::BooleanEvaluator() {
}

void BooleanEvaluator::setVariable(const std::string& name, bool value) {
    variables_[name] = value;
}

bool BooleanEvaluator::getVariable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second;
    }
    return false;
}

void BooleanEvaluator::clearVariables() {
    variables_.clear();
}

void BooleanEvaluator::setVariableResolver(VariableResolver resolver) {
    resolver_ = resolver;
}

bool BooleanEvaluator::evaluate(const std::shared_ptr<BooleanExpr>& expr) const {
    if (!expr) {
        throw std::invalid_argument("Expression cannot be null");
    }
    return evaluateNode(expr.get());
}

bool BooleanEvaluator::evaluate(const std::string& expression) {
    BooleanParser parser;
    auto expr = parser.parse(expression);
    if (!expr) {
        throw std::runtime_error("Parse error: " + parser.getLastError());
    }
    return evaluate(expr);
}

void BooleanEvaluator::bindStationState(std::shared_ptr<StationState> state) {
    stationState_ = state;
}

bool BooleanEvaluator::evaluateNode(const BooleanExpr* expr) const {
    if (!expr) return false;

    switch (expr->getType()) {
        case ExprType::CONSTANT: {
            auto constExpr = dynamic_cast<const ConstantExpr*>(expr);
            return constExpr ? constExpr->getValue() : false;
        }

        case ExprType::VARIABLE: {
            auto varExpr = dynamic_cast<const VariableExpr*>(expr);
            if (!varExpr) return false;

            const std::string& name = varExpr->getName();

            auto it = variables_.find(name);
            if (it != variables_.end()) {
                return it->second;
            }

            if (resolver_) {
                return resolver_(name);
            }

            if (stationState_) {
                return stationState_->getBooleanValue(name);
            }

            return false;
        }

        case ExprType::NOT: {
            auto unaryExpr = dynamic_cast<const UnaryExpr*>(expr);
            if (!unaryExpr || !unaryExpr->getOperand()) return true;
            return !evaluateNode(unaryExpr->getOperand().get());
        }

        case ExprType::AND: {
            auto binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
            if (!binaryExpr) return false;
            bool left = evaluateNode(binaryExpr->getLeft().get());
            if (!left) return false;
            bool right = evaluateNode(binaryExpr->getRight().get());
            return left && right;
        }

        case ExprType::OR: {
            auto binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
            if (!binaryExpr) return false;
            bool left = evaluateNode(binaryExpr->getLeft().get());
            if (left) return true;
            bool right = evaluateNode(binaryExpr->getRight().get());
            return left || right;
        }

        case ExprType::XOR: {
            auto binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
            if (!binaryExpr) return false;
            bool left = evaluateNode(binaryExpr->getLeft().get());
            bool right = evaluateNode(binaryExpr->getRight().get());
            return left != right;
        }

        case ExprType::IMPLIES: {
            auto binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
            if (!binaryExpr) return true;
            bool left = evaluateNode(binaryExpr->getLeft().get());
            if (!left) return true;
            bool right = evaluateNode(binaryExpr->getRight().get());
            return !left || right;
        }

        case ExprType::EQUIV: {
            auto binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
            if (!binaryExpr) return false;
            bool left = evaluateNode(binaryExpr->getLeft().get());
            bool right = evaluateNode(binaryExpr->getRight().get());
            return left == right;
        }

        default:
            return false;
    }
}

} // namespace boolean
} // namespace railway
