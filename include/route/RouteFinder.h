#ifndef RAILWAY_ROUTE_FINDER_H
#define RAILWAY_ROUTE_FINDER_H

#include "topology/Graph.h"
#include "topology/StationState.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace railway {

enum class SwitchClassification {
    SIMPLE,
    SLIP_SINGLE,
    SLIP_DOUBLE,
    DIAMOND_CROSSING,
    THREE_WAY
};

struct Route {
    std::vector<std::string> nodeIds;
    double totalWeight;
    std::string startSignal;
    std::string endSignal;
    std::vector<std::pair<std::string, std::string>> edges;

    size_t length() const { return nodeIds.size(); }
    bool containsNode(const std::string& nodeId) const;
    bool containsEdge(const std::string& from, const std::string& to) const;
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

    void setMaxLoopCount(size_t count) { maxLoopCount_ = count; }
    size_t getMaxLoopCount() const { return maxLoopCount_; }

    SwitchClassification classifySwitch(const std::string& switchId) const;

private:
    struct DfsState {
        std::string currentNode;
        std::string prevNode;
        double currentWeight;
        size_t depth;
        size_t loopCount;
        std::vector<std::string> path;
        std::vector<std::pair<std::string, std::string>> pathEdges;
        std::unordered_map<std::string, size_t> nodeVisitCount;
        std::unordered_set<std::string> edgeVisited;
        size_t nextEdgeIndex;
        std::vector<std::shared_ptr<GraphEdge>> edges;
    };

    std::string edgeKey(const std::string& from, const std::string& to) const;

    void iterativeDfs(const std::string& start,
                      const std::string& end,
                      std::vector<Route>& results,
                      RouteFilter filter,
                      size_t maxResults);

    bool validateEdgeTraversal(const std::string& fromNode,
                               const std::string& toNode,
                               const std::unordered_map<std::string, size_t>& visitCount,
                               size_t currentLoopCount,
                               size_t& outNewLoopCount) const;

    bool canTraverseSwitchDirection(const std::string& switchId,
                                    const std::string& fromNode,
                                    const std::string& toNode) const;

    bool isLoopEntryAllowed(const std::string& nodeId,
                            size_t visitCount,
                            size_t loopCount) const;

    std::shared_ptr<DirectedPropertyGraph> graph_;
    std::shared_ptr<StationState> state_;
    size_t maxRouteLength_;
    size_t maxLoopCount_;
};

} // namespace railway

#endif // RAILWAY_ROUTE_FINDER_H
