#include "topology/Graph.h"
#include <algorithm>

namespace railway {

GraphNode::GraphNode(const std::string& id, NodeType type)
    : id_(id), type_(type), switchPosition_(SwitchPosition::UNKNOWN) {
}

const std::string& GraphNode::getId() const {
    return id_;
}

NodeType GraphNode::getType() const {
    return type_;
}

void GraphNode::setProperty(const std::string& key, const std::string& value) {
    properties_[key] = value;
}

std::optional<std::string> GraphNode::getProperty(const std::string& key) const {
    auto it = properties_.find(key);
    if (it != properties_.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::unordered_map<std::string, std::string>& GraphNode::getProperties() const {
    return properties_;
}

void GraphNode::setSignalType(SignalType type) {
    signalType_ = type;
}

std::optional<SignalType> GraphNode::getSignalType() const {
    return signalType_;
}

void GraphNode::setSwitchPosition(SwitchPosition pos) {
    switchPosition_ = pos;
}

SwitchPosition GraphNode::getSwitchPosition() const {
    return switchPosition_;
}

GraphEdge::GraphEdge(const std::string& fromId, const std::string& toId, double weight)
    : fromId_(fromId), toId_(toId), weight_(weight) {
}

const std::string& GraphEdge::getFromId() const {
    return fromId_;
}

const std::string& GraphEdge::getToId() const {
    return toId_;
}

double GraphEdge::getWeight() const {
    return weight_;
}

void GraphEdge::setWeight(double weight) {
    weight_ = weight;
}

void GraphEdge::setProperty(const std::string& key, const std::string& value) {
    properties_[key] = value;
}

std::optional<std::string> GraphEdge::getProperty(const std::string& key) const {
    auto it = properties_.find(key);
    if (it != properties_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool DirectedPropertyGraph::addNode(std::shared_ptr<GraphNode> node) {
    if (!node || nodes_.count(node->getId()) > 0) {
        return false;
    }
    nodes_[node->getId()] = node;
    outgoingEdges_[node->getId()];
    incomingEdges_[node->getId()];
    return true;
}

bool DirectedPropertyGraph::removeNode(const std::string& nodeId) {
    if (nodes_.count(nodeId) == 0) {
        return false;
    }
    nodes_.erase(nodeId);
    outgoingEdges_.erase(nodeId);
    incomingEdges_.erase(nodeId);

    for (auto& pair : outgoingEdges_) {
        auto& edges = pair.second;
        edges.erase(std::remove_if(edges.begin(), edges.end(),
            [&nodeId](const std::shared_ptr<GraphEdge>& e) {
                return e->getToId() == nodeId;
            }), edges.end());
    }

    for (auto& pair : incomingEdges_) {
        auto& edges = pair.second;
        edges.erase(std::remove_if(edges.begin(), edges.end(),
            [&nodeId](const std::shared_ptr<GraphEdge>& e) {
                return e->getFromId() == nodeId;
            }), edges.end());
    }

    return true;
}

std::shared_ptr<GraphNode> DirectedPropertyGraph::getNode(const std::string& nodeId) const {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return nullptr;
}

bool DirectedPropertyGraph::hasNode(const std::string& nodeId) const {
    return nodes_.count(nodeId) > 0;
}

size_t DirectedPropertyGraph::getNodeCount() const {
    return nodes_.size();
}

bool DirectedPropertyGraph::addEdge(const std::string& fromId, const std::string& toId, double weight) {
    if (!hasNode(fromId) || !hasNode(toId)) {
        return false;
    }
    auto edge = std::make_shared<GraphEdge>(fromId, toId, weight);
    outgoingEdges_[fromId].push_back(edge);
    incomingEdges_[toId].push_back(edge);
    return true;
}

bool DirectedPropertyGraph::addEdge(std::shared_ptr<GraphEdge> edge) {
    if (!edge || !hasNode(edge->getFromId()) || !hasNode(edge->getToId())) {
        return false;
    }
    outgoingEdges_[edge->getFromId()].push_back(edge);
    incomingEdges_[edge->getToId()].push_back(edge);
    return true;
}

bool DirectedPropertyGraph::removeEdge(const std::string& fromId, const std::string& toId) {
    bool removed = false;

    auto outIt = outgoingEdges_.find(fromId);
    if (outIt != outgoingEdges_.end()) {
        auto& edges = outIt->second;
        auto before = edges.size();
        edges.erase(std::remove_if(edges.begin(), edges.end(),
            [&toId](const std::shared_ptr<GraphEdge>& e) {
                return e->getToId() == toId;
            }), edges.end());
        removed = edges.size() < before;
    }

    auto inIt = incomingEdges_.find(toId);
    if (inIt != incomingEdges_.end()) {
        auto& edges = inIt->second;
        edges.erase(std::remove_if(edges.begin(), edges.end(),
            [&fromId](const std::shared_ptr<GraphEdge>& e) {
                return e->getFromId() == fromId;
            }), edges.end());
    }

    return removed;
}

std::shared_ptr<GraphEdge> DirectedPropertyGraph::getEdge(const std::string& fromId, const std::string& toId) const {
    auto it = outgoingEdges_.find(fromId);
    if (it != outgoingEdges_.end()) {
        for (const auto& edge : it->second) {
            if (edge->getToId() == toId) {
                return edge;
            }
        }
    }
    return nullptr;
}

bool DirectedPropertyGraph::hasEdge(const std::string& fromId, const std::string& toId) const {
    return getEdge(fromId, toId) != nullptr;
}

size_t DirectedPropertyGraph::getEdgeCount() const {
    size_t count = 0;
    for (const auto& pair : outgoingEdges_) {
        count += pair.second.size();
    }
    return count;
}

std::vector<std::shared_ptr<GraphNode>> DirectedPropertyGraph::getOutgoingNodes(const std::string& nodeId) const {
    std::vector<std::shared_ptr<GraphNode>> result;
    auto it = outgoingEdges_.find(nodeId);
    if (it != outgoingEdges_.end()) {
        for (const auto& edge : it->second) {
            auto node = getNode(edge->getToId());
            if (node) {
                result.push_back(node);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<GraphNode>> DirectedPropertyGraph::getIncomingNodes(const std::string& nodeId) const {
    std::vector<std::shared_ptr<GraphNode>> result;
    auto it = incomingEdges_.find(nodeId);
    if (it != incomingEdges_.end()) {
        for (const auto& edge : it->second) {
            auto node = getNode(edge->getFromId());
            if (node) {
                result.push_back(node);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<GraphEdge>> DirectedPropertyGraph::getOutgoingEdges(const std::string& nodeId) const {
    auto it = outgoingEdges_.find(nodeId);
    if (it != outgoingEdges_.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::shared_ptr<GraphEdge>> DirectedPropertyGraph::getIncomingEdges(const std::string& nodeId) const {
    auto it = incomingEdges_.find(nodeId);
    if (it != incomingEdges_.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::shared_ptr<GraphNode>> DirectedPropertyGraph::getNodesByType(NodeType type) const {
    std::vector<std::shared_ptr<GraphNode>> result;
    for (const auto& pair : nodes_) {
        if (pair.second->getType() == type) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<std::shared_ptr<GraphNode>> DirectedPropertyGraph::getAllNodes() const {
    std::vector<std::shared_ptr<GraphNode>> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

void DirectedPropertyGraph::clear() {
    nodes_.clear();
    outgoingEdges_.clear();
    incomingEdges_.clear();
}

} // namespace railway
