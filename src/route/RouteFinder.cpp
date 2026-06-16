#include "route/RouteFinder.h"
#include <algorithm>
#include <stack>
#include <iostream>

namespace railway {

static std::string nodeTypeToString(NodeType t) {
    switch (t) {
        case NodeType::TRACK_CIRCUIT: return "TRACK";
        case NodeType::SWITCH: return "SWITCH";
        case NodeType::SIGNAL: return "SIGNAL";
    }
    return "UNKNOWN";
}

bool Route::containsNode(const std::string& nodeId) const {
    return std::find(nodeIds.begin(), nodeIds.end(), nodeId) != nodeIds.end();
}

bool Route::containsEdge(const std::string& from, const std::string& to) const {
    for (const auto& e : edges) {
        if (e.first == from && e.second == to) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> Route::getTrackCircuits() const {
    std::vector<std::string> result;
    for (const auto& id : nodeIds) {
        result.push_back(id);
    }
    return result;
}

std::vector<std::string> Route::getSwitches() const {
    std::vector<std::string> result;
    for (const auto& id : nodeIds) {
        result.push_back(id);
    }
    return result;
}

RouteFinder::RouteFinder(std::shared_ptr<DirectedPropertyGraph> graph)
    : graph_(graph), maxRouteLength_(50), maxLoopCount_(2) {
}

void RouteFinder::setState(std::shared_ptr<StationState> state) {
    state_ = state;
}

std::string RouteFinder::edgeKey(const std::string& from, const std::string& to) const {
    return from + "->" + to;
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

    if (startSignalId == endSignalId) {
        return results;
    }

    iterativeDfs(startSignalId, endSignalId, results, filter, maxResults);

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

SwitchClassification RouteFinder::classifySwitch(const std::string& switchId) const {
    auto node = graph_->getNode(switchId);
    if (!node || node->getType() != NodeType::SWITCH) {
        return SwitchClassification::SIMPLE;
    }

    auto outgoing = graph_->getOutgoingEdges(switchId);
    auto incoming = graph_->getIncomingEdges(switchId);

    size_t outDegree = outgoing.size();
    size_t inDegree = incoming.size();

    auto isSlip = node->getProperty("isSlip");
    if (isSlip && (*isSlip == "true" || *isSlip == "double")) {
        return SwitchClassification::SLIP_DOUBLE;
    }
    if (isSlip && *isSlip == "single") {
        return SwitchClassification::SLIP_SINGLE;
    }

    auto isDiamond = node->getProperty("isDiamond");
    if (isDiamond && *isDiamond == "true") {
        return SwitchClassification::DIAMOND_CROSSING;
    }

    if (outDegree >= 4 || inDegree >= 4) {
        return SwitchClassification::SLIP_DOUBLE;
    }
    if (outDegree >= 3 || inDegree >= 3) {
        return SwitchClassification::THREE_WAY;
    }

    return SwitchClassification::SIMPLE;
}

bool RouteFinder::isRouteValid(const Route& route) const {
    if (route.nodeIds.empty() || route.nodeIds.size() < 2) {
        return false;
    }

    if (route.edges.size() != route.nodeIds.size() - 1) {
        return false;
    }

    std::unordered_set<std::string> edgeSeen;
    for (const auto& e : route.edges) {
        std::string key = edgeKey(e.first, e.second);
        if (edgeSeen.count(key) > 0) {
            return false;
        }
        edgeSeen.insert(key);

        if (!graph_->hasEdge(e.first, e.second)) {
            return false;
        }
    }

    for (size_t i = 0; i < route.nodeIds.size() - 1; i++) {
        const std::string& from = route.nodeIds[i];
        const std::string& to = route.nodeIds[i + 1];

        auto fromNode = graph_->getNode(from);
        if (fromNode && fromNode->getType() == NodeType::SWITCH) {
            if (!canTraverseSwitchDirection(from, to, "")) {
                return false;
            }
        }

        auto toNode = graph_->getNode(to);
        if (toNode && toNode->getType() == NodeType::SWITCH) {
            if (!canTraverseSwitchDirection(to, "", from)) {
                return false;
            }
        }
    }

    return true;
}

void RouteFinder::iterativeDfs(const std::string& start,
                                const std::string& end,
                                std::vector<Route>& results,
                                RouteFilter filter,
                                size_t maxResults) {
    std::stack<DfsState> stk;

    DfsState initial;
    initial.currentNode = start;
    initial.prevNode = "";
    initial.currentWeight = 0.0;
    initial.depth = 0;
    initial.loopCount = 0;
    initial.path.push_back(start);
    initial.nodeVisitCount[start] = 1;
    initial.edges = graph_->getOutgoingEdges(start);
    initial.nextEdgeIndex = 0;

    stk.push(std::move(initial));

    const size_t ABSOLUTE_MAX_DEPTH = maxRouteLength_;
    const size_t ABSOLUTE_MAX_ITERATIONS = 1000000;
    size_t iterationCount = 0;

    while (!stk.empty() && results.size() < maxResults && iterationCount < ABSOLUTE_MAX_ITERATIONS) {
        iterationCount++;

        DfsState& s = stk.top();

        if (s.currentNode == end && s.path.size() > 1) {
            Route route;
            route.nodeIds = s.path;
            route.totalWeight = s.currentWeight;
            route.startSignal = s.path.front();
            route.endSignal = s.path.back();
            route.edges = s.pathEdges;

            if (!filter || filter(route)) {
                results.push_back(std::move(route));
            }
        }

        bool advanced = false;

        while (s.nextEdgeIndex < s.edges.size() && !advanced) {
            auto edge = s.edges[s.nextEdgeIndex];
            s.nextEdgeIndex++;

            const std::string& from = edge->getFromId();
            const std::string& to = edge->getToId();

            if (to == start && s.path.size() <= 2) {
                continue;
            }

            std::string eKey = edgeKey(from, to);
            if (s.edgeVisited.count(eKey) > 0) {
                continue;
            }

            size_t newLoopCount = s.loopCount;
            if (!validateEdgeTraversal(from, to, s.nodeVisitCount, s.loopCount, newLoopCount)) {
                continue;
            }

            if (s.depth + 1 >= ABSOLUTE_MAX_DEPTH) {
                continue;
            }

            auto fromNode = graph_->getNode(from);
            if (fromNode && fromNode->getType() == NodeType::SWITCH) {
                if (!canTraverseSwitchDirection(from, "", to)) {
                    continue;
                }
            }

            DfsState next;
            next.currentNode = to;
            next.prevNode = from;
            next.currentWeight = s.currentWeight + edge->getWeight();
            next.depth = s.depth + 1;
            next.loopCount = newLoopCount;
            next.path = s.path;
            next.path.push_back(to);
            next.pathEdges = s.pathEdges;
            next.pathEdges.emplace_back(from, to);
            next.nodeVisitCount = s.nodeVisitCount;
            next.nodeVisitCount[to]++;
            next.edgeVisited = s.edgeVisited;
            next.edgeVisited.insert(eKey);
            next.edges = graph_->getOutgoingEdges(to);
            next.nextEdgeIndex = 0;

            stk.push(std::move(next));
            advanced = true;
        }

        if (!advanced) {
            stk.pop();
        }
    }
}

bool RouteFinder::validateEdgeTraversal(const std::string& fromNode,
                                          const std::string& toNode,
                                          const std::unordered_map<std::string, size_t>& visitCount,
                                          size_t currentLoopCount,
                                          size_t& outNewLoopCount) const {
    outNewLoopCount = currentLoopCount;

    auto toNodePtr = graph_->getNode(toNode);
    if (!toNodePtr) {
        return false;
    }

    auto toVisitIt = visitCount.find(toNode);
    size_t toVisits = (toVisitIt != visitCount.end()) ? toVisitIt->second : 0;

    if (toNodePtr->getType() == NodeType::TRACK_CIRCUIT) {
        if (toVisits >= 1) {
            return false;
        }
    }

    if (toNodePtr->getType() == NodeType::SIGNAL) {
        if (toVisits >= 1) {
            return false;
        }
    }

    if (toNodePtr->getType() == NodeType::SWITCH) {
        SwitchClassification sc = classifySwitch(toNode);
        size_t maxSwitchVisits = 2;
        if (sc == SwitchClassification::SLIP_DOUBLE || sc == SwitchClassification::THREE_WAY) {
            maxSwitchVisits = 2;
        }
        if (sc == SwitchClassification::DIAMOND_CROSSING) {
            maxSwitchVisits = 2;
        }

        if (toVisits >= maxSwitchVisits) {
            return false;
        }

        if (toVisits >= 1) {
            if (currentLoopCount + 1 > maxLoopCount_) {
                return false;
            }
            outNewLoopCount = currentLoopCount + 1;
        }
    }

    return true;
}

bool RouteFinder::canTraverseSwitchDirection(const std::string& switchId,
                                              const std::string& fromNode,
                                              const std::string& toNode) const {
    auto node = graph_->getNode(switchId);
    if (!node || node->getType() != NodeType::SWITCH) {
        return true;
    }

    SwitchClassification sc = classifySwitch(switchId);

    auto normalTo = node->getProperty("normalTo");
    auto reverseTo = node->getProperty("reverseTo");
    auto from = node->getProperty("from");

    SwitchPosition pos = SwitchPosition::UNKNOWN;
    if (state_) {
        pos = state_->getSwitchPosition(switchId);
    } else {
        pos = node->getSwitchPosition();
    }

    if (sc == SwitchClassification::DIAMOND_CROSSING) {
        return true;
    }

    if (sc == SwitchClassification::SLIP_DOUBLE) {
        return true;
    }

    if (pos == SwitchPosition::UNKNOWN) {
        return true;
    }

    if (pos == SwitchPosition::NORMAL) {
        if (normalTo) {
            if (!toNode.empty() && toNode == *normalTo) return true;
            if (!fromNode.empty() && fromNode == *normalTo) return true;
        }
        if (from) {
            if (!toNode.empty() && toNode == *from) return true;
            if (!fromNode.empty() && fromNode == *from) return true;
        }
        if (!normalTo && !from) {
            return true;
        }
        if (!toNode.empty() && reverseTo && toNode == *reverseTo) {
            return false;
        }
        if (!fromNode.empty() && reverseTo && fromNode == *reverseTo) {
            return false;
        }
        return true;
    }

    if (pos == SwitchPosition::REVERSE) {
        if (reverseTo) {
            if (!toNode.empty() && toNode == *reverseTo) return true;
            if (!fromNode.empty() && fromNode == *reverseTo) return true;
        }
        if (from) {
            if (!toNode.empty() && toNode == *from) return true;
            if (!fromNode.empty() && fromNode == *from) return true;
        }
        if (!reverseTo && !from) {
            return true;
        }
        if (!toNode.empty() && normalTo && toNode == *normalTo) {
            return false;
        }
        if (!fromNode.empty() && normalTo && fromNode == *normalTo) {
            return false;
        }
        return true;
    }

    return true;
}

bool RouteFinder::isLoopEntryAllowed(const std::string& nodeId,
                                      size_t visitCount,
                                      size_t loopCount) const {
    if (loopCount > maxLoopCount_) {
        return false;
    }

    auto node = graph_->getNode(nodeId);
    if (!node) return false;

    if (node->getType() == NodeType::SWITCH) {
        return visitCount <= 2;
    }

    return visitCount <= 1;
}

} // namespace railway
