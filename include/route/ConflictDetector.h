#ifndef RAILWAY_CONFLICT_DETECTOR_H
#define RAILWAY_CONFLICT_DETECTOR_H

#include "route/RouteFinder.h"
#include "topology/Graph.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace railway {

enum class ConflictType {
    NONE,
    TRACK_OVERLAP,
    SWITCH_CONFLICT,
    HEAD_ON,
    SIDE_CONFLICT,
    CROSSING_CONFLICT
};

struct RouteConflict {
    ConflictType type;
    std::string description;
    std::vector<std::string> conflictingNodes;
    Route routeA;
    Route routeB;
};

class ConflictDetector {
public:
    explicit ConflictDetector(std::shared_ptr<DirectedPropertyGraph> graph);

    std::vector<RouteConflict> findConflicts(const Route& routeA, const Route& routeB) const;

    std::vector<RouteConflict> findAllConflicts(const std::vector<Route>& routes) const;

    bool hasConflict(const Route& routeA, const Route& routeB) const;

    std::vector<Route> findConflictingRoutes(const Route& target,
                                              const std::vector<Route>& candidates) const;

    std::string conflictTypeToString(ConflictType type) const;

private:
    bool checkTrackOverlap(const Route& routeA, const Route& routeB,
                           std::vector<std::string>& outOverlaps) const;

    bool checkSwitchConflict(const Route& routeA, const Route& routeB,
                             std::vector<std::string>& outConflictingSwitches) const;

    bool checkHeadOn(const Route& routeA, const Route& routeB,
                     std::vector<std::string>& outConflicts) const;

    bool checkSideConflict(const Route& routeA, const Route& routeB,
                           std::vector<std::string>& outConflicts) const;

    std::unordered_set<std::string> getTrackCircuitsFromRoute(const Route& route) const;
    std::unordered_set<std::string> getSwitchesFromRoute(const Route& route) const;
    std::unordered_set<std::string> getAllNodesFromRoute(const Route& route) const;

    std::shared_ptr<DirectedPropertyGraph> graph_;
};

} // namespace railway

#endif // RAILWAY_CONFLICT_DETECTOR_H
