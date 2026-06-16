#ifndef RAILWAY_SIDE_PROTECTION_H
#define RAILWAY_SIDE_PROTECTION_H

#include "topology/Graph.h"
#include "topology/StationState.h"
#include "route/RouteFinder.h"
#include "boolean/BooleanExpr.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace railway {

enum class ProtectionType {
    LOCK_NORMAL,
    LOCK_REVERSE,
    LOCK_EITHER,
    BLOCK_SECTION,
    NO_PROTECTION_NEEDED
};

struct ProtectionSwitch {
    std::string switchId;
    ProtectionType requiredPosition;
    std::string description;
    std::vector<std::string> dangerSourceTracks;
    std::string alternativePaths;
    double riskLevel;
};

struct SideProtectionReport {
    Route mainRoute;
    std::vector<ProtectionSwitch> protectionSwitches;
    std::vector<std::string> overlappingTracks;
    std::vector<std::string> blockingSections;
    std::string booleanExpression;
    std::shared_ptr<boolean::BooleanExpr> protectionAst;
    bool isProtected;
};

class SideProtectionCalculator {
public:
    explicit SideProtectionCalculator(std::shared_ptr<DirectedPropertyGraph> graph);

    void setState(std::shared_ptr<StationState> state);

    SideProtectionReport calculateProtection(const Route& mainRoute);

    std::string generateBooleanExpression(const SideProtectionReport& report) const;

    std::shared_ptr<boolean::BooleanExpr> generateProtectionAst(const SideProtectionReport& report) const;

    std::string getProtectionTypeString(ProtectionType type) const;

    void setSearchDepth(size_t depth) { maxSearchDepth_ = depth; }

private:
    struct SearchState {
        std::string currentNode;
        std::string prevNode;
        size_t depth;
        std::vector<std::string> path;
        std::unordered_set<std::string> visited;
        bool reachesMainRoute;
        std::string entryPoint;
    };

    std::vector<std::string> getMainRouteBoundaryPoints(const Route& route) const;

    void bfsFromBoundary(
        const std::string& startNode,
        const std::unordered_set<std::string>& mainRouteSet,
        const std::unordered_set<std::string>& boundarySet,
        std::unordered_map<std::string, std::vector<std::vector<std::string>>>& outDangerPaths);

    ProtectionType computeRequiredPosition(
        const std::string& switchId,
        const std::string& fromMainRouteNode,
        const std::vector<std::vector<std::string>>& dangerPaths,
        const std::unordered_set<std::string>& mainRouteSet);

    bool canReachMainRoute(
        const std::string& switchId,
        SwitchPosition position,
        const std::unordered_set<std::string>& mainRouteSet);

    std::string switchPositionToString(SwitchPosition pos) const;

    std::shared_ptr<DirectedPropertyGraph> graph_;
    std::shared_ptr<StationState> state_;
    size_t maxSearchDepth_;
};

} // namespace railway

#endif // RAILWAY_SIDE_PROTECTION_H
