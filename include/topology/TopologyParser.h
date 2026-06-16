#ifndef RAILWAY_TOPOLOGY_PARSER_H
#define RAILWAY_TOPOLOGY_PARSER_H

#include "topology/Graph.h"
#include "topology/XmlParser.h"
#include <memory>
#include <string>

namespace railway {

class TopologyParser {
public:
    TopologyParser();

    std::shared_ptr<DirectedPropertyGraph> parseFile(const std::string& filePath);
    std::shared_ptr<DirectedPropertyGraph> parseString(const std::string& xmlContent);

    std::string getLastError() const;

private:
    bool parseTrackCircuits(xml::XmlElement& root, DirectedPropertyGraph& graph);
    bool parseSwitches(xml::XmlElement& root, DirectedPropertyGraph& graph);
    bool parseSignals(xml::XmlElement& root, DirectedPropertyGraph& graph);
    bool parseConnections(xml::XmlElement& root, DirectedPropertyGraph& graph);

    std::shared_ptr<DirectedPropertyGraph> buildGraph(xml::XmlElement& root);

    SignalType stringToSignalType(const std::string& s) const;
    SwitchPosition stringToSwitchPosition(const std::string& s) const;

    std::string lastError_;
};

} // namespace railway

#endif // RAILWAY_TOPOLOGY_PARSER_H
