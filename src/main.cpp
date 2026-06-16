#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>

#include "topology/Graph.h"
#include "topology/TopologyParser.h"
#include "topology/StationState.h"
#include "boolean/BooleanParser.h"
#include "boolean/BooleanEvaluator.h"
#include "route/RouteFinder.h"
#include "route/ConflictDetector.h"
#include "route/SideProtection.h"

using namespace railway;

void printUsage() {
    std::cout << "轨道交通联锁逻辑校验工具 (ilvalidator)" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: ilvalidator [命令] [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "命令:" << std::endl;
    std::cout << "  info <拓扑文件>              显示车站拓扑信息" << std::endl;
    std::cout << "  eval <拓扑文件> <表达式>     计算布尔联锁表达式" << std::endl;
    std::cout << "  route <拓扑文件> <起点> <终点>  搜索有效进路" << std::endl;
    std::cout << "  conflict <拓扑文件> <起点> <终点>  检测进路冲突" << std::endl;
    std::cout << "  validate <拓扑文件> <表达式>  验证联锁条件安全性" << std::endl;
    std::cout << "  protect <拓扑文件> <起点> <终点>  生成侧面防护道岔表" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -h, --help                显示帮助信息" << std::endl;
    std::cout << "  -v, --verbose             详细输出模式" << std::endl;
    std::cout << "  --max-routes <N>          最大进路搜索数量 (默认: 50)" << std::endl;
    std::cout << std::endl;
    std::cout << "布尔表达式语法:" << std::endl;
    std::cout << "  常量: true, false" << std::endl;
    std::cout << "  变量: track.<区段ID>, switch.<道岔ID>.normal/reverse/locked" << std::endl;
    std::cout << "        signal.<信号ID>.stop/proceed/caution" << std::endl;
    std::cout << "  运算符: ! (NOT), & (AND), | (OR), ^ (XOR), -> (蕴含), <-> (等价)" << std::endl;
    std::cout << "  括号: ( )" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  ilvalidator info station.xml" << std::endl;
    std::cout << "  ilvalidator eval station.xml \"switch.S1.normal & !track.T1.occupied\"" << std::endl;
    std::cout << "  ilvalidator route station.xml S1 X3" << std::endl;
    std::cout << "  ilvalidator conflict station.xml S1 X3" << std::endl;
}

void printSectionHeader(const std::string& title) {
    std::cout << std::endl;
    std::cout << "═══ " << title << " ";
    size_t len = title.length() + 4;
    for (size_t i = 0; i < 60 - len; i++) {
        std::cout << "═";
    }
    std::cout << std::endl;
    std::cout << std::endl;
}

int cmdInfo(const std::string& topologyFile, bool verbose) {
    TopologyParser parser;
    auto graph = parser.parseFile(topologyFile);

    if (!graph) {
        std::cerr << "错误: 无法解析拓扑文件 - " << parser.getLastError() << std::endl;
        return 1;
    }

    std::cout << "车站拓扑信息" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    auto trackCircuits = graph->getNodesByType(NodeType::TRACK_CIRCUIT);
    auto switches = graph->getNodesByType(NodeType::SWITCH);
    auto signals = graph->getNodesByType(NodeType::SIGNAL);

    std::cout << "  轨道区段数量: " << trackCircuits.size() << std::endl;
    std::cout << "  道岔数量:     " << switches.size() << std::endl;
    std::cout << "  信号机数量:   " << signals.size() << std::endl;
    std::cout << "  连接边数量:   " << graph->getEdgeCount() << std::endl;

    if (verbose) {
        printSectionHeader("轨道区段列表");
        for (const auto& node : trackCircuits) {
            std::cout << "  " << std::setw(12) << std::left << node->getId();
            auto name = node->getProperty("name");
            if (name) {
                std::cout << "  [" << *name << "]";
            }
            auto length = node->getProperty("length");
            if (length) {
                std::cout << "  长度: " << *length << "m";
            }
            std::cout << std::endl;
        }

        printSectionHeader("道岔列表");
        for (const auto& node : switches) {
            std::cout << "  " << std::setw(12) << std::left << node->getId();
            auto name = node->getProperty("name");
            if (name) {
                std::cout << "  [" << *name << "]";
            }
            SwitchPosition pos = node->getSwitchPosition();
            std::string posStr = "未知";
            if (pos == SwitchPosition::NORMAL) posStr = "定位";
            else if (pos == SwitchPosition::REVERSE) posStr = "反位";
            std::cout << "  位置: " << posStr;
            std::cout << std::endl;
        }

        printSectionHeader("信号机列表");
        for (const auto& node : signals) {
            std::cout << "  " << std::setw(12) << std::left << node->getId();
            auto name = node->getProperty("name");
            if (name) {
                std::cout << "  [" << *name << "]";
            }
            auto sigType = node->getSignalType();
            std::string typeStr = "未知";
            if (sigType) {
                switch (*sigType) {
                    case SignalType::ENTRY: typeStr = "进站信号机"; break;
                    case SignalType::EXIT: typeStr = "出站信号机"; break;
                    case SignalType::BLOCK: typeStr = "通过信号机"; break;
                    case SignalType::SHUNTING: typeStr = "调车信号机"; break;
                }
            }
            std::cout << "  类型: " << typeStr;
            std::cout << std::endl;
        }

        printSectionHeader("连接关系");
        for (const auto& node : graph->getAllNodes()) {
            auto outgoing = graph->getOutgoingEdges(node->getId());
            if (!outgoing.empty()) {
                for (const auto& edge : outgoing) {
                    std::cout << "  " << edge->getFromId() << " → " << edge->getToId();
                    std::cout << "  (权重: " << edge->getWeight() << ")";
                    std::cout << std::endl;
                }
            }
        }
    }

    std::cout << std::endl;
    return 0;
}

int cmdEval(const std::string& topologyFile, const std::string& expression, bool verbose) {
    TopologyParser parser;
    auto graph = parser.parseFile(topologyFile);

    if (!graph) {
        std::cerr << "错误: 无法解析拓扑文件 - " << parser.getLastError() << std::endl;
        return 1;
    }

    auto state = std::make_shared<StationState>(graph);

    boolean::BooleanParser boolParser;
    auto expr = boolParser.parse(expression);

    if (!expr) {
        std::cerr << "错误: 表达式解析失败 - " << boolParser.getLastError() << std::endl;
        return 1;
    }

    boolean::BooleanEvaluator evaluator;
    evaluator.bindStationState(state);

    bool result = evaluator.evaluate(expr);

    std::cout << "布尔表达式求值" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "  表达式: " << expression << std::endl;
    std::cout << "  AST:     " << expr->toString() << std::endl;
    std::cout << std::endl;
    std::cout << "  结果:    " << (result ? "TRUE (真)" : "FALSE (假)") << std::endl;

    if (verbose) {
        std::vector<std::string> variables;
        expr->collectVariables(variables);
        printSectionHeader("变量取值");
        for (const auto& var : variables) {
            bool value = state->getBooleanValue(var);
            std::cout << "  " << std::setw(30) << std::left << var;
            std::cout << " = " << (value ? "true" : "false") << std::endl;
        }
    }

    std::cout << std::endl;
    return 0;
}

int cmdRoute(const std::string& topologyFile, const std::string& startSignal,
             const std::string& endSignal, size_t maxRoutes, bool verbose) {
    TopologyParser parser;
    auto graph = parser.parseFile(topologyFile);

    if (!graph) {
        std::cerr << "错误: 无法解析拓扑文件 - " << parser.getLastError() << std::endl;
        return 1;
    }

    if (!graph->hasNode(startSignal)) {
        std::cerr << "错误: 起点信号机不存在 - " << startSignal << std::endl;
        return 1;
    }
    if (!graph->hasNode(endSignal)) {
        std::cerr << "错误: 终点信号机不存在 - " << endSignal << std::endl;
        return 1;
    }

    auto state = std::make_shared<StationState>(graph);

    RouteFinder finder(graph);
    finder.setState(state);
    finder.setMaxRouteLength(200);

    auto routes = finder.findAllRoutes(startSignal, endSignal, maxRoutes);

    std::cout << "进路搜索结果" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "  起点: " << startSignal << "  →  终点: " << endSignal << std::endl;
    std::cout << "  找到 " << routes.size() << " 条有效进路" << std::endl;
    std::cout << std::endl;

    if (routes.empty()) {
        std::cout << "  未找到有效进路。" << std::endl;
        return 0;
    }

    for (size_t i = 0; i < routes.size(); i++) {
        const auto& route = routes[i];
        std::cout << "── 进路 #" << (i + 1) << " (总权重: " << route.totalWeight << ") ──" << std::endl;
        std::cout << "  ";
        for (size_t j = 0; j < route.nodeIds.size(); j++) {
            if (j > 0) std::cout << " → ";
            std::cout << route.nodeIds[j];
        }
        std::cout << std::endl;

        if (verbose) {
            auto tracks = state->getTrackCircuitsInRoute(route.nodeIds);
            auto switches = state->getSwitchesInRoute(route.nodeIds);
            std::cout << "  包含区段: " << tracks.size() << "个";
            if (!tracks.empty()) {
                std::cout << " [";
                for (size_t j = 0; j < tracks.size(); j++) {
                    if (j > 0) std::cout << ", ";
                    std::cout << tracks[j];
                }
                std::cout << "]";
            }
            std::cout << std::endl;
            std::cout << "  包含道岔: " << switches.size() << "个";
            if (!switches.empty()) {
                std::cout << " [";
                for (size_t j = 0; j < switches.size(); j++) {
                    if (j > 0) std::cout << ", ";
                    std::cout << switches[j];
                }
                std::cout << "]";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    return 0;
}

int cmdConflict(const std::string& topologyFile, const std::string& startSignal,
                const std::string& endSignal, size_t maxRoutes, bool verbose) {
    TopologyParser parser;
    auto graph = parser.parseFile(topologyFile);

    if (!graph) {
        std::cerr << "错误: 无法解析拓扑文件 - " << parser.getLastError() << std::endl;
        return 1;
    }

    auto state = std::make_shared<StationState>(graph);

    RouteFinder finder(graph);
    finder.setState(state);
    finder.setMaxRouteLength(200);

    auto targetRoutes = finder.findAllRoutes(startSignal, endSignal, maxRoutes);

    if (targetRoutes.empty()) {
        std::cout << "未找到从 " << startSignal << " 到 " << endSignal << " 的进路。" << std::endl;
        return 0;
    }

    auto allSignals = graph->getNodesByType(NodeType::SIGNAL);
    std::vector<std::pair<std::string, std::string>> allSignalPairs;

    for (const auto& sig1 : allSignals) {
        for (const auto& sig2 : allSignals) {
            if (sig1->getId() != sig2->getId()) {
                allSignalPairs.push_back({sig1->getId(), sig2->getId()});
            }
        }
    }

    ConflictDetector detector(graph);

    std::cout << "进路冲突检测" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "  目标进路: " << startSignal << " → " << endSignal << std::endl;
    std::cout << "  目标进路数量: " << targetRoutes.size() << " 条" << std::endl;
    std::cout << std::endl;

    int totalConflicts = 0;

    for (size_t i = 0; i < targetRoutes.size(); i++) {
        const auto& target = targetRoutes[i];

        printSectionHeader("目标进路 #" + std::to_string(i + 1));
        std::cout << "  路径: ";
        for (size_t j = 0; j < target.nodeIds.size(); j++) {
            if (j > 0) std::cout << " → ";
            std::cout << target.nodeIds[j];
        }
        std::cout << std::endl;
        std::cout << std::endl;

        int routeConflicts = 0;

        for (const auto& pair : allSignalPairs) {
            if (pair.first == startSignal && pair.second == endSignal) {
                continue;
            }

            auto otherRoutes = finder.findAllRoutes(pair.first, pair.second, 10);

            for (const auto& other : otherRoutes) {
                auto conflicts = detector.findConflicts(target, other);

                if (!conflicts.empty()) {
                    routeConflicts++;
                    totalConflicts++;

                    std::cout << "  ▶ 冲突进路: " << pair.first << " → " << pair.second << std::endl;
                    std::cout << "    路径: ";
                    for (size_t j = 0; j < other.nodeIds.size(); j++) {
                        if (j > 0) std::cout << " → ";
                        std::cout << other.nodeIds[j];
                    }
                    std::cout << std::endl;

                    for (const auto& conflict : conflicts) {
                        std::cout << "      冲突类型: " << detector.conflictTypeToString(conflict.type);
                        if (!conflict.conflictingNodes.empty()) {
                            std::cout << "  涉及节点: [";
                            for (size_t j = 0; j < conflict.conflictingNodes.size(); j++) {
                                if (j > 0) std::cout << ", ";
                                std::cout << conflict.conflictingNodes[j];
                            }
                            std::cout << "]";
                        }
                        std::cout << std::endl;
                    }
                    std::cout << std::endl;
                }
            }
        }

        if (routeConflicts == 0) {
            std::cout << "  ✓ 未发现冲突进路" << std::endl;
        } else {
            std::cout << "  ✗ 共发现 " << routeConflicts << " 条冲突进路" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "  总计: " << totalConflicts << " 条冲突进路" << std::endl;

    return 0;
}

int cmdValidate(const std::string& topologyFile, const std::string& condition, bool verbose) {
    TopologyParser parser;
    auto graph = parser.parseFile(topologyFile);

    if (!graph) {
        std::cerr << "错误: 无法解析拓扑文件 - " << parser.getLastError() << std::endl;
        return 1;
    }

    auto state = std::make_shared<StationState>(graph);

    boolean::BooleanParser boolParser;
    auto expr = boolParser.parse(condition);

    if (!expr) {
        std::cerr << "错误: 表达式解析失败 - " << boolParser.getLastError() << std::endl;
        return 1;
    }

    boolean::BooleanEvaluator evaluator;
    evaluator.bindStationState(state);

    std::cout << "联锁条件安全校验" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "  联锁条件: " << condition << std::endl;
    std::cout << "  表达式:   " << expr->toString() << std::endl;
    std::cout << std::endl;

    bool initialResult = evaluator.evaluate(expr);
    std::cout << "  当前状态下结果: " << (initialResult ? "满足" : "不满足") << std::endl;

    if (verbose) {
        std::vector<std::string> variables;
        expr->collectVariables(variables);

        printSectionHeader("联锁变量分析");
        for (const auto& var : variables) {
            bool value = state->getBooleanValue(var);
            std::cout << "  " << std::setw(32) << std::left << var;
            std::cout << " = " << (value ? "true" : "false") << std::endl;
        }

        printSectionHeader("安全属性说明");
        std::cout << "  该联锁条件在当前车站状态下";
        if (initialResult) {
            std::cout << "成立。" << std::endl;
            std::cout << "  注意: 这只是静态校验，实际联锁系统需要实时动态验证。" << std::endl;
        } else {
            std::cout << "不成立。" << std::endl;
            std::cout << "  可能表示当前状态下存在安全隐患，或条件设置有误。" << std::endl;
        }
    }

    std::cout << std::endl;
    return initialResult ? 0 : 2;
}

int cmdProtect(const std::string& topologyFile, const std::string& startSignal,
               const std::string& endSignal, size_t maxRoutes, bool verbose) {
    TopologyParser parser;
    auto graph = parser.parseFile(topologyFile);
    if (!graph) {
        std::cerr << "错误: 无法解析拓扑文件 - " << parser.getLastError() << std::endl;
        return 1;
    }

    auto state = std::make_shared<StationState>(graph);
    RouteFinder finder(graph);
    finder.setState(state);

    auto routes = finder.findAllRoutes(startSignal, endSignal, maxRoutes);

    if (routes.empty()) {
        std::cerr << "错误: 未找到有效进路" << std::endl;
        return 1;
    }

    SideProtectionCalculator calculator(graph);
    calculator.setState(state);

    std::cout << "进路侧面防护分析报告" << std::endl;
    std::cout << "════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "  起点信号机: " << startSignal << std::endl;
    std::cout << "  终点信号机: " << endSignal << std::endl;
    std::cout << "  可用进路数量: " << routes.size() << " 条" << std::endl;
    std::cout << std::endl;

    for (size_t idx = 0; idx < routes.size(); idx++) {
        const auto& route = routes[idx];

        printSectionHeader("进路 #" + std::to_string(idx + 1) + " 防护分析");

        std::cout << "  进路路径: ";
        for (size_t i = 0; i < route.nodeIds.size(); i++) {
            if (i > 0) std::cout << " → ";
            std::cout << route.nodeIds[i];
        }
        std::cout << std::endl;
        std::cout << "  总权重: " << route.totalWeight << std::endl;
        std::cout << std::endl;

        auto report = calculator.calculateProtection(route);

        if (report.protectionSwitches.empty()) {
            std::cout << "  ✅ 无需额外侧面防护" << std::endl;
            std::cout << "  该进路所有相邻侧线均无法直接导向主进路。" << std::endl;
        } else {
            std::cout << "  ⚠️  需防护道岔数量: " << report.protectionSwitches.size() << " 个" << std::endl;
            std::cout << std::endl;

            for (size_t i = 0; i < report.protectionSwitches.size(); i++) {
                const auto& ps = report.protectionSwitches[i];
                std::cout << "  [" << (i + 1) << "] " << ps.description << std::endl;
                std::cout << "      风险等级: " << std::fixed << std::setprecision(1) << ps.riskLevel << "%" << std::endl;
                if (!ps.dangerSourceTracks.empty()) {
                    std::cout << "      危险源区段: ";
                    for (size_t j = 0; j < ps.dangerSourceTracks.size(); j++) {
                        if (j > 0) std::cout << ", ";
                        std::cout << ps.dangerSourceTracks[j];
                    }
                    std::cout << std::endl;
                }
                if (!ps.alternativePaths.empty()) {
                    std::cout << "      危险路径示例: " << ps.alternativePaths << std::endl;
                }
                std::cout << std::endl;
            }

            printSectionHeader("防护联锁表达式");
            std::cout << "  " << report.booleanExpression << std::endl;
            std::cout << std::endl;

            std::cout << "  解释: 主进路开通前，上述所有防护道岔必须被强制锁闭" << std::endl;
            std::cout << "        在指定位置，以偏转侧线来车方向，防止侧面冲突。" << std::endl;

            if (verbose) {
                boolean::BooleanParser booleanParser;
                auto fullExpr = booleanParser.parse(report.booleanExpression);
                boolean::BooleanEvaluator evaluator;
                evaluator.bindStationState(state);
                bool protectedNow = evaluator.evaluate(fullExpr);

                printSectionHeader("当前状态验证");
                std::cout << "  当前防护条件是否满足: " << (protectedNow ? "✅ 满足" : "❌ 不满足") << std::endl;

                if (!protectedNow) {
                    std::cout << std::endl;
                    std::cout << "  不满足的防护条件:" << std::endl;
                    for (const auto& ps : report.protectionSwitches) {
                        std::string var;
                        if (ps.requiredPosition == ProtectionType::LOCK_NORMAL) {
                            var = "switch." + ps.switchId + ".normal";
                        } else if (ps.requiredPosition == ProtectionType::LOCK_REVERSE) {
                            var = "switch." + ps.switchId + ".reverse";
                        } else {
                            continue;
                        }
                        bool val = state->getBooleanValue(var);
                        if (!val) {
                            std::cout << "    - " << var << " 当前为 false" << std::endl;
                        }
                    }
                }
            }
        }

        std::cout << std::endl;
        if (idx < routes.size() - 1) {
            std::cout << "─────────────────────────────────────────────────────" << std::endl;
        }
    }

    std::cout << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    if (args.empty() || args[0] == "-h" || args[0] == "--help") {
        printUsage();
        return 0;
    }

    bool verbose = false;
    size_t maxRoutes = 50;

    std::vector<std::string> filteredArgs;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "-v" || args[i] == "--verbose") {
            verbose = true;
        } else if (args[i] == "--max-routes" && i + 1 < args.size()) {
            try {
                maxRoutes = std::stoul(args[i + 1]);
                i++;
            } catch (...) {
                std::cerr << "警告: 无效的 --max-routes 值" << std::endl;
            }
        } else {
            filteredArgs.push_back(args[i]);
        }
    }

    if (filteredArgs.empty()) {
        printUsage();
        return 1;
    }

    const std::string& command = filteredArgs[0];

    if (command == "info") {
        if (filteredArgs.size() < 2) {
            std::cerr << "错误: info 命令需要拓扑文件路径" << std::endl;
            return 1;
        }
        return cmdInfo(filteredArgs[1], verbose);
    }

    if (command == "eval") {
        if (filteredArgs.size() < 3) {
            std::cerr << "错误: eval 命令需要拓扑文件和表达式" << std::endl;
            return 1;
        }
        return cmdEval(filteredArgs[1], filteredArgs[2], verbose);
    }

    if (command == "route") {
        if (filteredArgs.size() < 4) {
            std::cerr << "错误: route 命令需要拓扑文件、起点信号机、终点信号机" << std::endl;
            return 1;
        }
        return cmdRoute(filteredArgs[1], filteredArgs[2], filteredArgs[3], maxRoutes, verbose);
    }

    if (command == "conflict") {
        if (filteredArgs.size() < 4) {
            std::cerr << "错误: conflict 命令需要拓扑文件、起点信号机、终点信号机" << std::endl;
            return 1;
        }
        return cmdConflict(filteredArgs[1], filteredArgs[2], filteredArgs[3], maxRoutes, verbose);
    }

    if (command == "validate") {
        if (filteredArgs.size() < 3) {
            std::cerr << "错误: validate 命令需要拓扑文件和联锁条件表达式" << std::endl;
            return 1;
        }
        return cmdValidate(filteredArgs[1], filteredArgs[2], verbose);
    }

    if (command == "protect") {
        if (filteredArgs.size() < 4) {
            std::cerr << "错误: protect 命令需要拓扑文件、起点信号机、终点信号机" << std::endl;
            return 1;
        }
        return cmdProtect(filteredArgs[1], filteredArgs[2], filteredArgs[3], maxRoutes, verbose);
    }

    std::cerr << "错误: 未知命令 - " << command << std::endl;
    std::cerr << "使用 --help 查看帮助信息" << std::endl;
    return 1;
}
