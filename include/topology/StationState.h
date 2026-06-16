#ifndef RAILWAY_STATION_STATE_H
#define RAILWAY_STATION_STATE_H

#include "topology/Graph.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace railway {

enum class SignalAspect {
    STOP,
    PROCEED,
    CAUTION,
    SHUNT_ALLOWED,
    UNKNOWN
};

class StationState {
public:
    explicit StationState(std::shared_ptr<DirectedPropertyGraph> graph);

    std::shared_ptr<DirectedPropertyGraph> getGraph() const;

    bool isTrackOccupied(const std::string& trackId) const;
    void setTrackOccupied(const std::string& trackId, bool occupied);

    SwitchPosition getSwitchPosition(const std::string& switchId) const;
    void setSwitchPosition(const std::string& switchId, SwitchPosition position);

    bool isSwitchLocked(const std::string& switchId) const;
    void setSwitchLocked(const std::string& switchId, bool locked);

    SignalAspect getSignalAspect(const std::string& signalId) const;
    void setSignalAspect(const std::string& signalId, SignalAspect aspect);

    bool getBooleanValue(const std::string& variableName) const;
    void setBooleanValue(const std::string& variableName, bool value);

    std::vector<std::string> getTrackCircuitsInRoute(const std::vector<std::string>& nodeIds) const;
    std::vector<std::string> getSwitchesInRoute(const std::vector<std::string>& nodeIds) const;

    bool isRouteClear(const std::vector<std::string>& routeNodeIds) const;

    void resetAll();

private:
    std::shared_ptr<DirectedPropertyGraph> graph_;
    std::unordered_map<std::string, bool> trackOccupied_;
    std::unordered_map<std::string, SwitchPosition> switchPositions_;
    std::unordered_map<std::string, bool> switchLocked_;
    std::unordered_map<std::string, SignalAspect> signalAspects_;
    std::unordered_map<std::string, bool> customBooleans_;
};

} // namespace railway

#endif // RAILWAY_STATION_STATE_H
