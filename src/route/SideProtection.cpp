#include "route/SideProtection.h"
#include "route/ConflictDetector.h"
#include "boolean/BooleanParser.h"
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace railway {

SideProtectionCalculator::SideProtectionCalculator(std::shared_ptr<DirectedPropertyGraph> graph)
    : graph_(graph), maxSearchDepth_(10) {
}

void SideProtectionCalculator::setState(std::shared_ptr<StationState> state) {
    state_ = state;
}

std::string SideProtectionCalculator::getProtectionTypeString(ProtectionType type) const {
    switch (type) {
        case ProtectionType::LOCK_NORMAL: return "LOCK_NORMAL";
        case ProtectionType::LOCK_REVERSE: return "LOCK_REVERSE";
        case ProtectionType::LOCK_EITHER: return "LOCK_EITHER";
        case ProtectionType::BLOCK_SECTION: return "BLOCK_SECTION";
        case ProtectionType::NO_PROTECTION_NEEDED: return "NO_PROTECTION_NEEDED";
    }
    return "UNKNOWN";
}

std::string SideProtectionCalculator::switchPositionToString(SwitchPosition pos) const {
    switch (pos) {
        case SwitchPosition::NORMAL: return "normal";
        case SwitchPosition::REVERSE: return "reverse";
        case SwitchPosition::UNKNOWN: return "unknown";
    }
    return "unknown";
}

std::vector<std::string> SideProtectionCalculator::getMainRouteBoundaryPoints(const Route& route) const {
    std::vector<std::string> boundaries;
    std::unordered_set<std::string> mainRouteSet(route.nodeIds.begin(), route.nodeIds.end());

    for (size_t i = 0; i < route.nodeIds.size(); i++) {
        const std::string& nodeId = route.nodeIds[i];
        auto node = graph_->getNode(nodeId);
        if (!node) continue;

        auto outgoing = graph_->getOutgoingEdges(nodeId);
        for (const auto& edge : outgoing) {
            if (mainRouteSet.count(edge->getToId()) == 0) {
                boundaries.push_back(nodeId);
                break;
            }
        }

        auto incoming = graph_->getIncomingEdges(nodeId);
        for (const auto& edge : incoming) {
            if (mainRouteSet.count(edge->getFromId()) == 0) {
                auto it = std::find(boundaries.begin(), boundaries.end(), nodeId);
                if (it == boundaries.end()) {
                    boundaries.push_back(nodeId);
                }
                break;
            }
        }
    }

    return boundaries;
}

void SideProtectionCalculator::bfsFromBoundary(
    const std::string& startNode,
    const std::unordered_set<std::string>& mainRouteSet,
    const std::unordered_set<std::string>& boundarySet,
    std::unordered_map<std::string, std::vector<std::vector<std::string>>>& outDangerPaths) {
    std::queue<SearchState> q;

    SearchState initial;
    initial.currentNode = startNode;
    initial.prevNode = "";
    initial.depth = 0;
    initial.path.push_back(startNode);
    initial.visited.insert(startNode);
    initial.reachesMainRoute = false;
    initial.entryPoint = startNode;

    q.push(initial);

    while (!q.empty()) {
        SearchState s = q.front();
        q.pop();

        if (s.depth > maxSearchDepth_) {
            continue;
        }

        if (s.currentNode != startNode && mainRouteSet.count(s.currentNode) > 0) {
            s.reachesMainRoute = true;
            if (s.path.size() >= 2) {
                const std::string& switchId = s.path[0];
                outDangerPaths[switchId].push_back(s.path);
            }
            continue;
        }

        auto currentNodePtr = graph_->getNode(s.currentNode);
        if (currentNodePtr && currentNodePtr->getType() == NodeType::SIGNAL) {
            continue;
        }

        auto outgoing = graph_->getOutgoingEdges(s.currentNode);
        for (const auto& edge : outgoing) {
            const std::string& next = edge->getToId();

            if (next == s.prevNode) continue;
            if (s.visited.count(next) > 0) continue;
            if (s.depth > 0 && boundarySet.count(next) > 0 && next != startNode) {
                continue;
            }

            if (s.depth > 0 && mainRouteSet.count(next) > 0) {
                SearchState ns = s;
                ns.path.push_back(next);
                ns.reachesMainRoute = true;
                if (ns.path.size() >= 2) {
                    const std::string& switchId = ns.path[0];
                    outDangerPaths[switchId].push_back(ns.path);
                }
                continue;
            }

            SearchState ns = s;
            ns.prevNode = s.currentNode;
            ns.currentNode = next;
            ns.depth++;
            ns.path.push_back(next);
            ns.visited.insert(next);

            if (ns.depth <= maxSearchDepth_) {
                q.push(std::move(ns));
            }
        }
    }
}

ProtectionType SideProtectionCalculator::computeRequiredPosition(
    const std::string& switchId,
    const std::string& fromMainRouteNode,
    const std::vector<std::vector<std::string>>& dangerPaths,
    const std::unordered_set<std::string>& mainRouteSet) {
    auto switchNode = graph_->getNode(switchId);
    if (!switchNode || switchNode->getType() != NodeType::SWITCH) {
        return ProtectionType::NO_PROTECTION_NEEDED;
    }

    bool normalLeadsToMain = canReachMainRoute(switchId, SwitchPosition::NORMAL, mainRouteSet);
    bool reverseLeadsToMain = canReachMainRoute(switchId, SwitchPosition::REVERSE, mainRouteSet);

    if (!normalLeadsToMain && !reverseLeadsToMain) {
        return ProtectionType::NO_PROTECTION_NEEDED;
    }

    if (!normalLeadsToMain && reverseLeadsToMain) {
        return ProtectionType::LOCK_NORMAL;
    }

    if (normalLeadsToMain && !reverseLeadsToMain) {
        return ProtectionType::LOCK_REVERSE;
    }

    if (normalLeadsToMain && reverseLeadsToMain) {
        return ProtectionType::BLOCK_SECTION;
    }

    return ProtectionType::NO_PROTECTION_NEEDED;
}

bool SideProtectionCalculator::canReachMainRoute(
    const std::string& switchId,
    SwitchPosition position,
    const std::unordered_set<std::string>& mainRouteSet) {
    auto switchNode = graph_->getNode(switchId);
    if (!switchNode || switchNode->getType() != NodeType::SWITCH) {
        return false;
    }

    auto normalTo = switchNode->getProperty("normalTo");
    auto reverseTo = switchNode->getProperty("reverseTo");
    auto from = switchNode->getProperty("from");

    if (!normalTo || !reverseTo || !from) {
        return false;
    }

    std::string allowedDir1;
    std::string allowedDir2;
    std::string blockedDir;

    if (position == SwitchPosition::NORMAL) {
        allowedDir1 = *from;
        allowedDir2 = *normalTo;
        blockedDir = *reverseTo;
    } else {
        allowedDir1 = *from;
        allowedDir2 = *reverseTo;
        blockedDir = *normalTo;
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    q.push(allowedDir1);
    visited.insert(allowedDir1);
    if (allowedDir2 != allowedDir1) {
        q.push(allowedDir2);
        visited.insert(allowedDir2);
    }
    visited.insert(switchId);

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();

        if (mainRouteSet.count(current) > 0) {
            return true;
        }

        auto node = graph_->getNode(current);
        if (node && node->getType() == NodeType::SIGNAL) {
            continue;
        }

        auto outgoing = graph_->getOutgoingEdges(current);
        for (const auto& edge : outgoing) {
            const std::string& next = edge->getToId();

            if (next == switchId) {
                continue;
            }

            if (visited.count(next) > 0) {
                continue;
            }

            auto nextNode = graph_->getNode(next);
            if (nextNode && nextNode->getType() == NodeType::SWITCH) {
                auto nextFrom = nextNode->getProperty("from");
                auto nextNormal = nextNode->getProperty("normalTo");
                auto nextReverse = nextNode->getProperty("reverseTo");

                if (nextFrom && nextNormal && nextReverse) {
                    SwitchPosition nextPos = nextNode->getSwitchPosition();
                    std::string nextAllowedDir1 = *nextFrom;
                    std::string nextAllowedDir2;
                    std::string nextBlocked;

                    if (nextPos == SwitchPosition::NORMAL) {
                        nextAllowedDir2 = *nextNormal;
                        nextBlocked = *nextReverse;
                    } else if (nextPos == SwitchPosition::REVERSE) {
                        nextAllowedDir2 = *nextReverse;
                        nextBlocked = *nextNormal;
                    } else {
                        nextAllowedDir2 = *nextNormal;
                        nextBlocked = *nextReverse;
                    }

                    if (current == *nextFrom) {
                        if ((nextPos == SwitchPosition::NORMAL && next == *nextReverse) ||
                            (nextPos == SwitchPosition::REVERSE && next == *nextNormal)) {
                            continue;
                        }
                    }

                    if (current == nextBlocked) {
                        continue;
                    }
                }
            }

            visited.insert(next);
            q.push(next);
        }
    }

    return false;
}

SideProtectionReport SideProtectionCalculator::calculateProtection(const Route& mainRoute) {
    SideProtectionReport report;
    report.mainRoute = mainRoute;
    report.isProtected = false;

    if (mainRoute.nodeIds.size() < 2) {
        return report;
    }

    std::unordered_set<std::string> mainRouteSet(
        mainRoute.nodeIds.begin(), mainRoute.nodeIds.end());

    auto boundaries = getMainRouteBoundaryPoints(mainRoute);
    std::unordered_set<std::string> boundarySet(
        boundaries.begin(), boundaries.end());

    std::unordered_map<std::string, std::vector<std::vector<std::string>>> switchDangerPaths;

    for (const auto& boundary : boundaries) {
        auto node = graph_->getNode(boundary);
        if (!node) continue;

        if (node->getType() == NodeType::SWITCH) {
            bfsFromBoundary(boundary, mainRouteSet, boundarySet, switchDangerPaths);
        } else if (node->getType() == NodeType::TRACK_CIRCUIT) {
            auto incoming = graph_->getIncomingEdges(boundary);
            for (const auto& edge : incoming) {
                const std::string& from = edge->getFromId();
                if (mainRouteSet.count(from) == 0) {
                    auto fromNode = graph_->getNode(from);
                    if (fromNode && fromNode->getType() == NodeType::SWITCH) {
                        bfsFromBoundary(from, mainRouteSet, boundarySet, switchDangerPaths);
                    }
                }
            }
            auto outgoing = graph_->getOutgoingEdges(boundary);
            for (const auto& edge : outgoing) {
                const std::string& to = edge->getToId();
                if (mainRouteSet.count(to) == 0) {
                    auto toNode = graph_->getNode(to);
                    if (toNode && toNode->getType() == NodeType::SWITCH) {
                        bfsFromBoundary(to, mainRouteSet, boundarySet, switchDangerPaths);
                    }
                }
            }
        }
    }

    for (const auto& pair : switchDangerPaths) {
        const std::string& switchId = pair.first;
        const auto& paths = pair.second;

        if (mainRouteSet.count(switchId) > 0) {
            continue;
        }

        if (paths.empty()) {
            continue;
        }

        std::string fromMain = "";
        for (const auto& path : paths) {
            if (!path.empty()) {
                fromMain = path.front();
                break;
            }
        }

        ProtectionType pt = computeRequiredPosition(switchId, fromMain, paths, mainRouteSet);

        if (pt == ProtectionType::NO_PROTECTION_NEEDED) {
            continue;
        }

        ProtectionSwitch ps;
        ps.switchId = switchId;
        ps.requiredPosition = pt;

        std::stringstream desc;
        desc << "道岔 " << switchId << " 需锁定于 ";
        if (pt == ProtectionType::LOCK_NORMAL) {
            desc << "定位";
        } else if (pt == ProtectionType::LOCK_REVERSE) {
            desc << "反位";
        } else if (pt == ProtectionType::LOCK_EITHER) {
            desc << "任意方向";
        } else if (pt == ProtectionType::BLOCK_SECTION) {
            desc << "区段封锁";
        }

        ps.description = desc.str();

        for (const auto& path : paths) {
            if (path.size() >= 2) {
                for (size_t i = 0; i < path.size() - 1; i++) {
                    auto n = graph_->getNode(path[i]);
                    if (n && n->getType() == NodeType::TRACK_CIRCUIT) {
                        auto it = std::find(
                            ps.dangerSourceTracks.begin(),
                            ps.dangerSourceTracks.end(),
                            path[i]);
                        if (it == ps.dangerSourceTracks.end()) {
                            ps.dangerSourceTracks.push_back(path[i]);
                        }
                    }
                }
            }
        }

        if (!paths.empty()) {
            std::stringstream alt;
            for (size_t i = 0; i < paths.size() && i < 3; i++) {
                if (i > 0) alt << "; ";
                for (size_t j = 0; j < paths[i].size(); j++) {
                    if (j > 0) alt << "→";
                    alt << paths[i][j];
                }
            }
            ps.alternativePaths = alt.str();
        }

        ps.riskLevel = static_cast<double>(paths.size()) * 10.0;

        report.protectionSwitches.push_back(std::move(ps));
    }

    report.booleanExpression = generateBooleanExpression(report);
    report.protectionAst = generateProtectionAst(report);
    report.isProtected = !report.protectionSwitches.empty();

    return report;
}

std::string SideProtectionCalculator::generateBooleanExpression(const SideProtectionReport& report) const {
    if (report.protectionSwitches.empty()) {
        return "true";
    }

    std::stringstream ss;
    bool first = true;

    for (const auto& ps : report.protectionSwitches) {
        if (!first) {
            ss << " & ";
        }
        first = false;

        if (ps.requiredPosition == ProtectionType::LOCK_NORMAL) {
            ss << "switch." << ps.switchId << ".normal";
        } else if (ps.requiredPosition == ProtectionType::LOCK_REVERSE) {
            ss << "switch." << ps.switchId << ".reverse";
        } else if (ps.requiredPosition == ProtectionType::LOCK_EITHER) {
            ss << "(switch." << ps.switchId << ".normal | switch." << ps.switchId << ".reverse";
        } else if (ps.requiredPosition == ProtectionType::BLOCK_SECTION) {
            ss << "track." << ps.switchId << ".occupied";
        }
    }

    return ss.str();
}

std::shared_ptr<boolean::BooleanExpr> SideProtectionCalculator::generateProtectionAst(
    const SideProtectionReport& report) const {
    boolean::BooleanParser parser;
    std::string expr = generateBooleanExpression(report);
    return parser.parse(expr);
}

} // namespace railway
