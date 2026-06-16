#ifndef RAILWAY_GRAPH_H
#define RAILWAY_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace railway {

enum class NodeType {
    TRACK_CIRCUIT,
    SWITCH,
    SIGNAL
};

enum class SignalType {
    ENTRY,
    EXIT,
    BLOCK,
    SHUNTING
};

enum class SwitchPosition {
    NORMAL,
    REVERSE,
    UNKNOWN
};

struct NodeProperty {
    std::string key;
    std::string value;
};

class GraphNode {
public:
    GraphNode(const std::string& id, NodeType type);
    virtual ~GraphNode() = default;

    const std::string& getId() const;
    NodeType getType() const;

    void setProperty(const std::string& key, const std::string& value);
    std::optional<std::string> getProperty(const std::string& key) const;
    const std::unordered_map<std::string, std::string>& getProperties() const;

    void setSignalType(SignalType type);
    std::optional<SignalType> getSignalType() const;

    void setSwitchPosition(SwitchPosition pos);
    SwitchPosition getSwitchPosition() const;

protected:
    std::string id_;
    NodeType type_;
    std::unordered_map<std::string, std::string> properties_;
    SwitchPosition switchPosition_;
    std::optional<SignalType> signalType_;
};

class GraphEdge {
public:
    GraphEdge(const std::string& fromId, const std::string& toId, double weight = 1.0);

    const std::string& getFromId() const;
    const std::string& getToId() const;
    double getWeight() const;

    void setWeight(double weight);
    void setProperty(const std::string& key, const std::string& value);
    std::optional<std::string> getProperty(const std::string& key) const;

private:
    std::string fromId_;
    std::string toId_;
    double weight_;
    std::unordered_map<std::string, std::string> properties_;
};

class DirectedPropertyGraph {
public:
    DirectedPropertyGraph() = default;

    bool addNode(std::shared_ptr<GraphNode> node);
    bool removeNode(const std::string& nodeId);
    std::shared_ptr<GraphNode> getNode(const std::string& nodeId) const;
    bool hasNode(const std::string& nodeId) const;
    size_t getNodeCount() const;

    bool addEdge(const std::string& fromId, const std::string& toId, double weight = 1.0);
    bool addEdge(std::shared_ptr<GraphEdge> edge);
    bool removeEdge(const std::string& fromId, const std::string& toId);
    std::shared_ptr<GraphEdge> getEdge(const std::string& fromId, const std::string& toId) const;
    bool hasEdge(const std::string& fromId, const std::string& toId) const;
    size_t getEdgeCount() const;

    std::vector<std::shared_ptr<GraphNode>> getOutgoingNodes(const std::string& nodeId) const;
    std::vector<std::shared_ptr<GraphNode>> getIncomingNodes(const std::string& nodeId) const;
    std::vector<std::shared_ptr<GraphEdge>> getOutgoingEdges(const std::string& nodeId) const;
    std::vector<std::shared_ptr<GraphEdge>> getIncomingEdges(const std::string& nodeId) const;

    std::vector<std::shared_ptr<GraphNode>> getNodesByType(NodeType type) const;
    std::vector<std::shared_ptr<GraphNode>> getAllNodes() const;

    void clear();

private:
    std::unordered_map<std::string, std::shared_ptr<GraphNode>> nodes_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<GraphEdge>>> outgoingEdges_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<GraphEdge>>> incomingEdges_;
};

} // namespace railway

#endif // RAILWAY_GRAPH_H
