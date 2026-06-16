#ifndef RAILWAY_ROUTE_FINDER_H
#define RAILWAY_ROUTE_FINDER_H

#include "topology/Graph.h"
#include "topology/StationState.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace railway {

struct Route {
    std::vector<std::string> nodeIds;
    double totalWeight;
    std::string startSignal;
    std::string endSignal;

    size_t length() const { return nodeIds.size(); }
    bool containsNode(const std::string& nodeId) const;
    std::vector<std::string> getTrackCircuits() const;
    std::vector<std::string> getSwitches() const;
};

using RouteFilter = std::function<bool(const Route&)>;

class RouteFinder {
public:
    explicit RouteFinder(std::shared_ptr<DirectedPropertyGraph> graph);

    void setState(std::shared_ptr<StationState> state);

    std::vector<Route> findAllRoutes(const std::string& startSignalId,
                                     const std::string& endSignalId,
                                     size_t maxResults = 100);

    std::vector<Route> findAllRoutes(const std::string& startSignalId,
                                     const std::string& endSignalId,
                                     RouteFilter filter,
                                     size_t maxResults = 100);

    Route findShortestRoute(const std::string& startSignalId,
                            const std::string& endSignalId);

    bool isRouteValid(const Route& route) const;

    void setMaxRouteLength(size_t maxLen) { maxRouteLength_ = maxLen; }
    size_t getMaxRouteLength() const { return maxRouteLength_; }

private:
    void dfsSearch(const std::string& current,
                   const std::string& end,
                   std::vector<std::string>& path,
                   std::vector<Route>& results,
                   RouteFilter filter,
                   size_t maxResults,
                   double currentWeight);

    bool canTraverseSwitch(const std::string& switchId,
                           const std::string& fromNode,
                           const std::string& toNode) const;

    std::shared_ptr<DirectedPropertyGraph> graph_;
    std::shared_ptr<StationState> state_;
    size_t maxRouteLength_;
};

} // namespace railway

#endif // RAILWAY_ROUTE_FINDER_H
