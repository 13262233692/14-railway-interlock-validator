#include "route/ConflictDetector.h"
#include <algorithm>
#include <sstream>

namespace railway {

ConflictDetector::ConflictDetector(std::shared_ptr<DirectedPropertyGraph> graph)
    : graph_(graph) {
}

std::vector<RouteConflict> ConflictDetector::findConflicts(const Route& routeA,
                                                            const Route& routeB) const {
    std::vector<RouteConflict> conflicts;

    std::vector<std::string> trackOverlaps;
    if (checkTrackOverlap(routeA, routeB, trackOverlaps)) {
        RouteConflict conflict;
        conflict.type = ConflictType::TRACK_OVERLAP;
        conflict.description = "轨道区段重叠";
        conflict.conflictingNodes = trackOverlaps;
        conflict.routeA = routeA;
        conflict.routeB = routeB;
        conflicts.push_back(conflict);
    }

    std::vector<std::string> switchConflicts;
    if (checkSwitchConflict(routeA, routeB, switchConflicts)) {
        RouteConflict conflict;
        conflict.type = ConflictType::SWITCH_CONFLICT;
        conflict.description = "道岔位置冲突";
        conflict.conflictingNodes = switchConflicts;
        conflict.routeA = routeA;
        conflict.routeB = routeB;
        conflicts.push_back(conflict);
    }

    std::vector<std::string> headOnConflicts;
    if (checkHeadOn(routeA, routeB, headOnConflicts)) {
        RouteConflict conflict;
        conflict.type = ConflictType::HEAD_ON;
        conflict.description = "对向冲突";
        conflict.conflictingNodes = headOnConflicts;
        conflict.routeA = routeA;
        conflict.routeB = routeB;
        conflicts.push_back(conflict);
    }

    std::vector<std::string> sideConflicts;
    if (checkSideConflict(routeA, routeB, sideConflicts)) {
        RouteConflict conflict;
        conflict.type = ConflictType::SIDE_CONFLICT;
        conflict.description = "侧面冲突";
        conflict.conflictingNodes = sideConflicts;
        conflict.routeA = routeA;
        conflict.routeB = routeB;
        conflicts.push_back(conflict);
    }

    return conflicts;
}

std::vector<RouteConflict> ConflictDetector::findAllConflicts(const std::vector<Route>& routes) const {
    std::vector<RouteConflict> allConflicts;

    for (size_t i = 0; i < routes.size(); i++) {
        for (size_t j = i + 1; j < routes.size(); j++) {
            auto conflicts = findConflicts(routes[i], routes[j]);
            allConflicts.insert(allConflicts.end(), conflicts.begin(), conflicts.end());
        }
    }

    return allConflicts;
}

bool ConflictDetector::hasConflict(const Route& routeA, const Route& routeB) const {
    std::vector<std::string> dummy;
    return checkTrackOverlap(routeA, routeB, dummy) ||
           checkSwitchConflict(routeA, routeB, dummy) ||
           checkHeadOn(routeA, routeB, dummy) ||
           checkSideConflict(routeA, routeB, dummy);
}

std::vector<Route> ConflictDetector::findConflictingRoutes(const Route& target,
                                                            const std::vector<Route>& candidates) const {
    std::vector<Route> result;
    for (const auto& candidate : candidates) {
        if (hasConflict(target, candidate)) {
            result.push_back(candidate);
        }
    }
    return result;
}

std::string ConflictDetector::conflictTypeToString(ConflictType type) const {
    switch (type) {
        case ConflictType::NONE: return "无冲突";
        case ConflictType::TRACK_OVERLAP: return "轨道区段重叠";
        case ConflictType::SWITCH_CONFLICT: return "道岔位置冲突";
        case ConflictType::HEAD_ON: return "对向冲突";
        case ConflictType::SIDE_CONFLICT: return "侧面冲突";
        case ConflictType::CROSSING_CONFLICT: return "交叉冲突";
        default: return "未知冲突";
    }
}

bool ConflictDetector::checkTrackOverlap(const Route& routeA,
                                          const Route& routeB,
                                          std::vector<std::string>& outOverlaps) const {
    auto tracksA = getTrackCircuitsFromRoute(routeA);
    auto tracksB = getTrackCircuitsFromRoute(routeB);

    for (const auto& track : tracksA) {
        if (tracksB.count(track) > 0) {
            outOverlaps.push_back(track);
        }
    }

    return !outOverlaps.empty();
}

bool ConflictDetector::checkSwitchConflict(const Route& routeA,
                                            const Route& routeB,
                                            std::vector<std::string>& outConflictingSwitches) const {
    auto switchesA = getSwitchesFromRoute(routeA);
    auto switchesB = getSwitchesFromRoute(routeB);

    for (const auto& sw : switchesA) {
        if (switchesB.count(sw) > 0) {
            auto node = graph_->getNode(sw);
            if (node && node->getType() == NodeType::SWITCH) {
                auto normalTo = node->getProperty("normalTo");
                auto reverseTo = node->getProperty("reverseTo");

                if (normalTo && reverseTo) {
                    bool aUsesNormal = false, aUsesReverse = false;
                    bool bUsesNormal = false, bUsesReverse = false;

                    for (size_t i = 0; i < routeA.nodeIds.size() - 1; i++) {
                        if (routeA.nodeIds[i] == sw) {
                            if (routeA.nodeIds[i + 1] == *normalTo) aUsesNormal = true;
                            if (routeA.nodeIds[i + 1] == *reverseTo) aUsesReverse = true;
                        }
                        if (routeA.nodeIds[i + 1] == sw) {
                            if (routeA.nodeIds[i] == *normalTo) aUsesNormal = true;
                            if (routeA.nodeIds[i] == *reverseTo) aUsesReverse = true;
                        }
                    }

                    for (size_t i = 0; i < routeB.nodeIds.size() - 1; i++) {
                        if (routeB.nodeIds[i] == sw) {
                            if (routeB.nodeIds[i + 1] == *normalTo) bUsesNormal = true;
                            if (routeB.nodeIds[i + 1] == *reverseTo) bUsesReverse = true;
                        }
                        if (routeB.nodeIds[i + 1] == sw) {
                            if (routeB.nodeIds[i] == *normalTo) bUsesNormal = true;
                            if (routeB.nodeIds[i] == *reverseTo) bUsesReverse = true;
                        }
                    }

                    if ((aUsesNormal && bUsesReverse) || (aUsesReverse && bUsesNormal)) {
                        outConflictingSwitches.push_back(sw);
                    }
                }
            }
        }
    }

    return !outConflictingSwitches.empty();
}

bool ConflictDetector::checkHeadOn(const Route& routeA,
                                    const Route& routeB,
                                    std::vector<std::string>& outConflicts) const {
    auto tracksA = getTrackCircuitsFromRoute(routeA);
    auto tracksB = getTrackCircuitsFromRoute(routeB);

    std::vector<std::string> commonTracks;
    for (const auto& track : tracksA) {
        if (tracksB.count(track) > 0) {
            commonTracks.push_back(track);
        }
    }

    if (commonTracks.empty()) {
        return false;
    }

    for (const auto& track : commonTracks) {
        int idxA = -1, idxB = -1;

        for (size_t i = 0; i < routeA.nodeIds.size(); i++) {
            if (routeA.nodeIds[i] == track) {
                idxA = static_cast<int>(i);
                break;
            }
        }

        for (size_t i = 0; i < routeB.nodeIds.size(); i++) {
            if (routeB.nodeIds[i] == track) {
                idxB = static_cast<int>(i);
                break;
            }
        }

        if (idxA < 0 || idxB < 0) continue;

        std::string prevA, nextA;
        std::string prevB, nextB;

        if (idxA > 0) prevA = routeA.nodeIds[idxA - 1];
        if (idxA < static_cast<int>(routeA.nodeIds.size()) - 1) nextA = routeA.nodeIds[idxA + 1];

        if (idxB > 0) prevB = routeB.nodeIds[idxB - 1];
        if (idxB < static_cast<int>(routeB.nodeIds.size()) - 1) nextB = routeB.nodeIds[idxB + 1];

        bool oppositeDirection = false;

        if (!prevA.empty() && !nextB.empty() && prevA == nextB) {
            oppositeDirection = true;
        }
        if (!nextA.empty() && !prevB.empty() && nextA == prevB) {
            oppositeDirection = true;
        }

        if (oppositeDirection) {
            outConflicts.push_back(track);
        }
    }

    return !outConflicts.empty();
}

bool ConflictDetector::checkSideConflict(const Route& routeA,
                                          const Route& routeB,
                                          std::vector<std::string>& outConflicts) const {
    auto switchesA = getSwitchesFromRoute(routeA);
    auto switchesB = getSwitchesFromRoute(routeB);

    for (const auto& sw : switchesA) {
        if (switchesB.count(sw) == 0) {
            continue;
        }

        auto node = graph_->getNode(sw);
        if (!node || node->getType() != NodeType::SWITCH) {
            continue;
        }

        auto fromProp = node->getProperty("from");
        auto normalTo = node->getProperty("normalTo");
        auto reverseTo = node->getProperty("reverseTo");

        if (!fromProp || !normalTo || !reverseTo) {
            continue;
        }

        bool aFromEntry = false, bFromEntry = false;
        bool aToNormal = false, aToReverse = false;
        bool bToNormal = false, bToReverse = false;

        for (size_t i = 0; i < routeA.nodeIds.size() - 1; i++) {
            if (routeA.nodeIds[i] == *fromProp && routeA.nodeIds[i + 1] == sw) {
                aFromEntry = true;
            }
            if (routeA.nodeIds[i] == sw) {
                if (routeA.nodeIds[i + 1] == *normalTo) aToNormal = true;
                if (routeA.nodeIds[i + 1] == *reverseTo) aToReverse = true;
            }
        }

        for (size_t i = 0; i < routeB.nodeIds.size() - 1; i++) {
            if (routeB.nodeIds[i] == *fromProp && routeB.nodeIds[i + 1] == sw) {
                bFromEntry = true;
            }
            if (routeB.nodeIds[i] == sw) {
                if (routeB.nodeIds[i + 1] == *normalTo) bToNormal = true;
                if (routeB.nodeIds[i + 1] == *reverseTo) bToReverse = true;
            }
        }

        if (aFromEntry && bFromEntry) {
            if ((aToNormal && bToReverse) || (aToReverse && bToNormal)) {
                outConflicts.push_back(sw);
            }
        }
    }

    return !outConflicts.empty();
}

std::unordered_set<std::string> ConflictDetector::getTrackCircuitsFromRoute(const Route& route) const {
    std::unordered_set<std::string> result;
    for (const auto& id : route.nodeIds) {
        auto node = graph_->getNode(id);
        if (node && node->getType() == NodeType::TRACK_CIRCUIT) {
            result.insert(id);
        }
    }
    return result;
}

std::unordered_set<std::string> ConflictDetector::getSwitchesFromRoute(const Route& route) const {
    std::unordered_set<std::string> result;
    for (const auto& id : route.nodeIds) {
        auto node = graph_->getNode(id);
        if (node && node->getType() == NodeType::SWITCH) {
            result.insert(id);
        }
    }
    return result;
}

std::unordered_set<std::string> ConflictDetector::getAllNodesFromRoute(const Route& route) const {
    std::unordered_set<std::string> result;
    for (const auto& id : route.nodeIds) {
        result.insert(id);
    }
    return result;
}

} // namespace railway
