#include "topology/TopologyParser.h"
#include <algorithm>
#include <cctype>

namespace railway {

TopologyParser::TopologyParser() {
}

std::shared_ptr<DirectedPropertyGraph> TopologyParser::parseFile(const std::string& filePath) {
    xml::XmlParser xmlParser;
    auto root = xmlParser.parseFile(filePath);
    if (!root) {
        lastError_ = "XML parse error: " + xmlParser.getLastError();
        return nullptr;
    }
    return buildGraph(*root);
}

std::shared_ptr<DirectedPropertyGraph> TopologyParser::parseString(const std::string& xmlContent) {
    xml::XmlParser xmlParser;
    auto root = xmlParser.parse(xmlContent);
    if (!root) {
        lastError_ = "XML parse error: " + xmlParser.getLastError();
        return nullptr;
    }
    return buildGraph(*root);
}

std::string TopologyParser::getLastError() const {
    return lastError_;
}

bool TopologyParser::parseTrackCircuits(xml::XmlElement& root, DirectedPropertyGraph& graph) {
    auto circuitsElement = root.getFirstChildByName("trackCircuits");
    if (!circuitsElement) {
        lastError_ = "Missing <trackCircuits> element";
        return false;
    }

    auto circuits = circuitsElement->getChildrenByName("circuit");
    for (const auto& circuit : circuits) {
        std::string id = circuit->getAttributeValue("id");
        if (id.empty()) {
            lastError_ = "Track circuit missing 'id' attribute";
            return false;
        }

        auto node = std::make_shared<GraphNode>(id, NodeType::TRACK_CIRCUIT);

        std::string name = circuit->getAttributeValue("name");
        if (!name.empty()) {
            node->setProperty("name", name);
        }

        std::string length = circuit->getAttributeValue("length");
        if (!length.empty()) {
            node->setProperty("length", length);
        }

        std::string occupied = circuit->getAttributeValue("occupied");
        if (!occupied.empty()) {
            node->setProperty("occupied", occupied);
        }

        for (const auto& attr : circuit->getAttributes()) {
            if (attr.name != "id" && attr.name != "name" &&
                attr.name != "length" && attr.name != "occupied") {
                node->setProperty(attr.name, attr.value);
            }
        }

        if (!graph.addNode(node)) {
            lastError_ = "Duplicate track circuit id: " + id;
            return false;
        }
    }

    return true;
}

bool TopologyParser::parseSwitches(xml::XmlElement& root, DirectedPropertyGraph& graph) {
    auto switchesElement = root.getFirstChildByName("switches");
    if (!switchesElement) {
        return true;
    }

    auto switches = switchesElement->getChildrenByName("switch");
    for (const auto& sw : switches) {
        std::string id = sw->getAttributeValue("id");
        if (id.empty()) {
            lastError_ = "Switch missing 'id' attribute";
            return false;
        }

        auto node = std::make_shared<GraphNode>(id, NodeType::SWITCH);

        std::string name = sw->getAttributeValue("name");
        if (!name.empty()) {
            node->setProperty("name", name);
        }

        std::string position = sw->getAttributeValue("position");
        if (!position.empty()) {
            node->setSwitchPosition(stringToSwitchPosition(position));
            node->setProperty("position", position);
        }

        std::string locked = sw->getAttributeValue("locked");
        if (!locked.empty()) {
            node->setProperty("locked", locked);
        }

        std::string normalTo = sw->getAttributeValue("normalTo");
        if (!normalTo.empty()) {
            node->setProperty("normalTo", normalTo);
        }

        std::string reverseTo = sw->getAttributeValue("reverseTo");
        if (!reverseTo.empty()) {
            node->setProperty("reverseTo", reverseTo);
        }

        std::string from = sw->getAttributeValue("from");
        if (!from.empty()) {
            node->setProperty("from", from);
        }

        if (!graph.addNode(node)) {
            lastError_ = "Duplicate switch id: " + id;
            return false;
        }
    }

    return true;
}

bool TopologyParser::parseSignals(xml::XmlElement& root, DirectedPropertyGraph& graph) {
    auto signalsElement = root.getFirstChildByName("signals");
    if (!signalsElement) {
        return true;
    }

    auto signals = signalsElement->getChildrenByName("signal");
    for (const auto& sig : signals) {
        std::string id = sig->getAttributeValue("id");
        if (id.empty()) {
            lastError_ = "Signal missing 'id' attribute";
            return false;
        }

        auto node = std::make_shared<GraphNode>(id, NodeType::SIGNAL);

        std::string name = sig->getAttributeValue("name");
        if (!name.empty()) {
            node->setProperty("name", name);
        }

        std::string type = sig->getAttributeValue("type");
        if (!type.empty()) {
            node->setSignalType(stringToSignalType(type));
            node->setProperty("signalType", type);
        }

        std::string aspect = sig->getAttributeValue("aspect");
        if (!aspect.empty()) {
            node->setProperty("aspect", aspect);
        }

        std::string protects = sig->getAttributeValue("protects");
        if (!protects.empty()) {
            node->setProperty("protects", protects);
        }

        if (!graph.addNode(node)) {
            lastError_ = "Duplicate signal id: " + id;
            return false;
        }
    }

    return true;
}

bool TopologyParser::parseConnections(xml::XmlElement& root, DirectedPropertyGraph& graph) {
    auto connectionsElement = root.getFirstChildByName("connections");
    if (!connectionsElement) {
        lastError_ = "Missing <connections> element";
        return false;
    }

    auto connections = connectionsElement->getChildrenByName("connection");
    for (const auto& conn : connections) {
        std::string from = conn->getAttributeValue("from");
        std::string to = conn->getAttributeValue("to");

        if (from.empty() || to.empty()) {
            lastError_ = "Connection missing 'from' or 'to' attribute";
            return false;
        }

        if (!graph.hasNode(from)) {
            lastError_ = "Connection references unknown node: " + from;
            return false;
        }
        if (!graph.hasNode(to)) {
            lastError_ = "Connection references unknown node: " + to;
            return false;
        }

        double weight = 1.0;
        std::string weightStr = conn->getAttributeValue("weight");
        if (!weightStr.empty()) {
            try {
                weight = std::stod(weightStr);
            } catch (...) {
                lastError_ = "Invalid weight value: " + weightStr;
                return false;
            }
        }

        auto edge = std::make_shared<GraphEdge>(from, to, weight);

        for (const auto& attr : conn->getAttributes()) {
            if (attr.name != "from" && attr.name != "to" && attr.name != "weight") {
                edge->setProperty(attr.name, attr.value);
            }
        }

        std::string direction = conn->getAttributeValue("direction");
        if (direction == "both" || direction == "bidirectional") {
            graph.addEdge(edge);
            auto reverseEdge = std::make_shared<GraphEdge>(to, from, weight);
            for (const auto& attr : conn->getAttributes()) {
                if (attr.name != "from" && attr.name != "to" && attr.name != "weight" && attr.name != "direction") {
                    reverseEdge->setProperty(attr.name, attr.value);
                }
            }
            graph.addEdge(reverseEdge);
        } else {
            graph.addEdge(edge);
        }
    }

    return true;
}

SignalType TopologyParser::stringToSignalType(const std::string& s) const {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (lower == "entry" || lower == "entrance") {
        return SignalType::ENTRY;
    } else if (lower == "exit") {
        return SignalType::EXIT;
    } else if (lower == "block") {
        return SignalType::BLOCK;
    } else if (lower == "shunting" || lower == "shunt") {
        return SignalType::SHUNTING;
    }
    return SignalType::BLOCK;
}

SwitchPosition TopologyParser::stringToSwitchPosition(const std::string& s) const {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (lower == "normal" || lower == "norm" || lower == "straight") {
        return SwitchPosition::NORMAL;
    } else if (lower == "reverse" || lower == "rev" || lower == "turned") {
        return SwitchPosition::REVERSE;
    }
    return SwitchPosition::UNKNOWN;
}

std::shared_ptr<DirectedPropertyGraph> TopologyParser::buildGraph(xml::XmlElement& root) {
    auto graph = std::make_shared<DirectedPropertyGraph>();

    if (root.getName() != "station" && root.getName() != "topology") {
        auto station = root.getFirstChildByName("station");
        if (!station) {
            station = root.getFirstChildByName("topology");
        }
        if (!station) {
            lastError_ = "Root element must be <station> or <topology>";
            return nullptr;
        }
        root = *station;
    }

    if (!parseTrackCircuits(root, *graph)) {
        return nullptr;
    }

    if (!parseSwitches(root, *graph)) {
        return nullptr;
    }

    if (!parseSignals(root, *graph)) {
        return nullptr;
    }

    if (!parseConnections(root, *graph)) {
        return nullptr;
    }

    return graph;
}

} // namespace railway
