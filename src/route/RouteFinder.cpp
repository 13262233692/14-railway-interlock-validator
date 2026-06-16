#include "route/RouteFinder.h"
#include <algorithm>
#include <unordered_set>
#include <climits>

namespace railway {

bool Route::containsNode(const std::string& nodeId) const {
    return std::find(nodeIds.begin(), nodeIds.end(), nodeId) != nodeIds.end();
}

std::vector<std::string> Route::getTrackCircuits() const {
    std::vector<std::string> result;
    return result;
}

std::vector<std::string> Route::getSwitches() const {
    std::vector<std::string> result;
    return result;
}

RouteFinder::RouteFinder(std::shared_ptr<DirectedPropertyGraph> graph)
    : graph_(graph), maxRouteLength_(100) {
}

void RouteFinder::setState(std::shared_ptr<StationState> state) {
    state_ = state;
}

std::vector<Route> RouteFinder::findAllRoutes(const std::string& startSignalId,
                                               const std::string& endSignalId,
                                               size_t maxResults) {
    return findAllRoutes(startSignalId, endSignalId, nullptr, maxResults);
}

std::vector<Route> RouteFinder::findAllRoutes(const std::string& startSignalId,
                                               const std::string& endSignalId,
                                               RouteFilter filter,
                                               size_t maxResults) {
    std::vector<Route> results;

    if (!graph_->hasNode(startSignalId) || !graph_->hasNode(endSignalId)) {
        return results;
    }

    auto startNode = graph_->getNode(startSignalId);
    if (!startNode || startNode->getType() != NodeType::SIGNAL) {
        return results;
    }

    auto endNode = graph_->getNode(endSignalId);
    if (!endNode || endNode->getType() != NodeType::SIGNAL) {
        return results;
    }

    std::vector<std::string> path;
    path.push_back(startSignalId);

    dfsSearch(startSignalId, endSignalId, path, results, filter, maxResults, 0.0);

    return results;
}

Route RouteFinder::findShortestRoute(const std::string& startSignalId,
                                      const std::string& endSignalId) {
    auto routes = findAllRoutes(startSignalId, endSignalId, 100);

    Route shortest;
    double minWeight = -1;

    for (const auto& route : routes) {
        if (minWeight < 0 || route.totalWeight < minWeight) {
            minWeight = route.totalWeight;
            shortest = route;
        }
    }

    return shortest;
}

bool RouteFinder::isRouteValid(const Route& route) const {
    if (route.nodeIds.empty()) {
        return false;
    }

    std::unordered_set<std::string> visited;
    for (const auto& id : route.nodeIds) {
        if (visited.count(id) > 0) {
            auto node = graph_->getNode(id);
            if (node && node->getType() != NodeType::SWITCH) {
                return false;
            }
        }
        visited.insert(id);
    }

    for (size_t i = 0; i < route.nodeIds.size() - 1; i++) {
        const std::string& from = route.nodeIds[i];
        const std::string& to = route.nodeIds[i + 1];

        if (!graph_->hasEdge(from, to)) {
            return false;
        }

        auto fromNode = graph_->getNode(from);
        if (fromNode && fromNode->getType() == NodeType::SWITCH) {
            if (!canTraverseSwitch(from, to, "")) {
                return false;
            }
        }

        auto toNode = graph_->getNode(to);
        if (toNode && toNode->getType() == NodeType::SWITCH) {
            if (!canTraverseSwitch(to, from, "")) {
                return false;
            }
        }
    }

    return true;
}

void RouteFinder::dfsSearch(const std::string& current,
                             const std::string& end,
                             std::vector<std::string>& path,
                             std::vector<Route>& results,
                             RouteFilter filter,
                             size_t maxResults,
                             double currentWeight) {
    if (results.size() >= maxResults) {
        return;
    }

    if (path.size() > maxRouteLength_) {
        return;
    }

    if (current == end && path.size() > 1) {
        Route route;
        route.nodeIds = path;
        route.totalWeight = currentWeight;
        route.startSignal = path.front();
        route.endSignal = path.back();

        if (!filter || filter(route)) {
            results.push_back(route);
        }
        return;
    }

    auto outgoing = graph_->getOutgoingEdges(current);
    for (const auto& edge : outgoing) {
        const std::string& next = edge->getToId();

        bool inPath = false;
        for (const auto& id : path) {
            if (id == next) {
                inPath = true;
                break;
            }
        }

        if (inPath) {
            continue;
        }

        auto currentNode = graph_->getNode(current);
        if (currentNode && currentNode->getType() == NodeType::SWITCH) {
            if (!canTraverseSwitch(current, current, next)) {
                continue;
            }
        }

        path.push_back(next);
        dfsSearch(next, end, path, results, filter, maxResults, currentWeight + edge->getWeight());
        path.pop_back();
    }
}

bool RouteFinder::canTraverseSwitch(const std::string& switchId,
                                     const std::string& fromNode,
                                     const std::string& toNode) const {
    auto node = graph_->getNode(switchId);
    if (!node || node->getType() != NodeType::SWITCH) {
        return true;
    }

    auto normalTo = node->getProperty("normalTo");
    auto reverseTo = node->getProperty("reverseTo");
    auto from = node->getProperty("from");

    SwitchPosition pos = SwitchPosition::UNKNOWN;
    if (state_) {
        pos = state_->getSwitchPosition(switchId);
    } else {
        pos = node->getSwitchPosition();
    }

    if (pos == SwitchPosition::UNKNOWN) {
        return true;
    }

    if (pos == SwitchPosition::NORMAL && normalTo) {
        if (!toNode.empty() && toNode == *normalTo) {
            return true;
        }
        if (!fromNode.empty() && fromNode == *normalTo) {
            return true;
        }
    }

    if (pos == SwitchPosition::REVERSE && reverseTo) {
        if (!toNode.empty() && toNode == *reverseTo) {
            return true;
        }
        if (!fromNode.empty() && fromNode == *reverseTo) {
            return true;
        }
    }

    if (from && (!toNode.empty() && toNode == *from)) {
        return true;
    }
    if (from && (!fromNode.empty() && fromNode == *from)) {
        return true;
    }

    if (pos == SwitchPosition::UNKNOWN) {
        return true;
    }

    return true;
}

} // namespace railway
