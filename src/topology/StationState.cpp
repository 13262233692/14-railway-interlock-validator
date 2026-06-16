#include "topology/StationState.h"
#include <stdexcept>

namespace railway {

StationState::StationState(std::shared_ptr<DirectedPropertyGraph> graph)
    : graph_(graph) {
    if (!graph_) {
        throw std::invalid_argument("Graph cannot be null");
    }
    resetAll();
}

std::shared_ptr<DirectedPropertyGraph> StationState::getGraph() const {
    return graph_;
}

bool StationState::isTrackOccupied(const std::string& trackId) const {
    auto it = trackOccupied_.find(trackId);
    if (it != trackOccupied_.end()) {
        return it->second;
    }
    auto node = graph_->getNode(trackId);
    if (node && node->getType() == NodeType::TRACK_CIRCUIT) {
        auto occupiedProp = node->getProperty("occupied");
        if (occupiedProp) {
            return *occupiedProp == "true" || *occupiedProp == "1" || *occupiedProp == "yes";
        }
    }
    return false;
}

void StationState::setTrackOccupied(const std::string& trackId, bool occupied) {
    trackOccupied_[trackId] = occupied;
}

SwitchPosition StationState::getSwitchPosition(const std::string& switchId) const {
    auto it = switchPositions_.find(switchId);
    if (it != switchPositions_.end()) {
        return it->second;
    }
    auto node = graph_->getNode(switchId);
    if (node && node->getType() == NodeType::SWITCH) {
        return node->getSwitchPosition();
    }
    return SwitchPosition::UNKNOWN;
}

void StationState::setSwitchPosition(const std::string& switchId, SwitchPosition position) {
    switchPositions_[switchId] = position;
}

bool StationState::isSwitchLocked(const std::string& switchId) const {
    auto it = switchLocked_.find(switchId);
    if (it != switchLocked_.end()) {
        return it->second;
    }
    auto node = graph_->getNode(switchId);
    if (node && node->getType() == NodeType::SWITCH) {
        auto lockedProp = node->getProperty("locked");
        if (lockedProp) {
            return *lockedProp == "true" || *lockedProp == "1" || *lockedProp == "yes";
        }
    }
    return false;
}

void StationState::setSwitchLocked(const std::string& switchId, bool locked) {
    switchLocked_[switchId] = locked;
}

SignalAspect StationState::getSignalAspect(const std::string& signalId) const {
    auto it = signalAspects_.find(signalId);
    if (it != signalAspects_.end()) {
        return it->second;
    }
    auto node = graph_->getNode(signalId);
    if (node && node->getType() == NodeType::SIGNAL) {
        auto aspectProp = node->getProperty("aspect");
        if (aspectProp) {
            if (*aspectProp == "stop" || *aspectProp == "red" || *aspectProp == "danger") {
                return SignalAspect::STOP;
            } else if (*aspectProp == "proceed" || *aspectProp == "green" || *aspectProp == "clear") {
                return SignalAspect::PROCEED;
            } else if (*aspectProp == "caution" || *aspectProp == "yellow" || *aspectProp == "warning") {
                return SignalAspect::CAUTION;
            } else if (*aspectProp == "shunt" || *aspectProp == "shunting") {
                return SignalAspect::SHUNT_ALLOWED;
            }
        }
    }
    return SignalAspect::UNKNOWN;
}

void StationState::setSignalAspect(const std::string& signalId, SignalAspect aspect) {
    signalAspects_[signalId] = aspect;
}

bool StationState::getBooleanValue(const std::string& variableName) const {
    auto it = customBooleans_.find(variableName);
    if (it != customBooleans_.end()) {
        return it->second;
    }

    if (variableName.rfind("track.", 0) == 0) {
        std::string trackId = variableName.substr(6);
        return isTrackOccupied(trackId);
    }

    if (variableName.rfind("switch.", 0) == 0) {
        std::string rest = variableName.substr(7);
        size_t dotPos = rest.find('.');
        if (dotPos != std::string::npos) {
            std::string switchId = rest.substr(0, dotPos);
            std::string attr = rest.substr(dotPos + 1);
            if (attr == "normal") {
                return getSwitchPosition(switchId) == SwitchPosition::NORMAL;
            } else if (attr == "reverse") {
                return getSwitchPosition(switchId) == SwitchPosition::REVERSE;
            } else if (attr == "locked") {
                return isSwitchLocked(switchId);
            }
        }
    }

    if (variableName.rfind("signal.", 0) == 0) {
        std::string rest = variableName.substr(7);
        size_t dotPos = rest.find('.');
        if (dotPos != std::string::npos) {
            std::string signalId = rest.substr(0, dotPos);
            std::string attr = rest.substr(dotPos + 1);
            if (attr == "stop") {
                return getSignalAspect(signalId) == SignalAspect::STOP;
            } else if (attr == "proceed" || attr == "clear") {
                return getSignalAspect(signalId) == SignalAspect::PROCEED;
            } else if (attr == "caution") {
                return getSignalAspect(signalId) == SignalAspect::CAUTION;
            }
        }
    }

    return false;
}

void StationState::setBooleanValue(const std::string& variableName, bool value) {
    customBooleans_[variableName] = value;
}

std::vector<std::string> StationState::getTrackCircuitsInRoute(const std::vector<std::string>& nodeIds) const {
    std::vector<std::string> result;
    for (const auto& id : nodeIds) {
        auto node = graph_->getNode(id);
        if (node && node->getType() == NodeType::TRACK_CIRCUIT) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> StationState::getSwitchesInRoute(const std::vector<std::string>& nodeIds) const {
    std::vector<std::string> result;
    for (const auto& id : nodeIds) {
        auto node = graph_->getNode(id);
        if (node && node->getType() == NodeType::SWITCH) {
            result.push_back(id);
        }
    }
    return result;
}

bool StationState::isRouteClear(const std::vector<std::string>& routeNodeIds) const {
    for (const auto& id : routeNodeIds) {
        auto node = graph_->getNode(id);
        if (!node) continue;
        if (node->getType() == NodeType::TRACK_CIRCUIT) {
            if (isTrackOccupied(id)) {
                return false;
            }
        }
    }
    return true;
}

void StationState::resetAll() {
    trackOccupied_.clear();
    switchPositions_.clear();
    switchLocked_.clear();
    signalAspects_.clear();
    customBooleans_.clear();

    for (const auto& node : graph_->getAllNodes()) {
        if (node->getType() == NodeType::TRACK_CIRCUIT) {
            trackOccupied_[node->getId()] = false;
        } else if (node->getType() == NodeType::SWITCH) {
            switchPositions_[node->getId()] = node->getSwitchPosition();
            switchLocked_[node->getId()] = false;
        } else if (node->getType() == NodeType::SIGNAL) {
            signalAspects_[node->getId()] = SignalAspect::STOP;
        }
    }
}

} // namespace railway
