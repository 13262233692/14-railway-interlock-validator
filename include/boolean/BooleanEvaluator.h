#ifndef RAILWAY_BOOLEAN_EVALUATOR_H
#define RAILWAY_BOOLEAN_EVALUATOR_H

#include "boolean/BooleanExpr.h"
#include "topology/StationState.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace railway {
namespace boolean {

using VariableResolver = std::function<bool(const std::string&)>;

class BooleanEvaluator {
public:
    BooleanEvaluator();

    void setVariable(const std::string& name, bool value);
    bool getVariable(const std::string& name) const;
    void clearVariables();

    void setVariableResolver(VariableResolver resolver);

    bool evaluate(const std::shared_ptr<BooleanExpr>& expr) const;

    bool evaluate(const std::string& expression);

    void bindStationState(std::shared_ptr<StationState> state);

private:
    bool evaluateNode(const BooleanExpr* expr) const;

    std::unordered_map<std::string, bool> variables_;
    VariableResolver resolver_;
    std::shared_ptr<StationState> stationState_;
};

} // namespace boolean
} // namespace railway

#endif // RAILWAY_BOOLEAN_EVALUATOR_H
