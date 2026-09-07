#include "services/shaders/PhoenixShaderParser.h"

#include <sstream>
#include <vector>

namespace shadereditor {
PhoenixParseResult PhoenixShaderParser::parse(ShaderPairDocument& document) const {
    if (document.source.size() >= 3 &&
        static_cast<unsigned char>(document.source[0]) == 0xEF &&
        static_cast<unsigned char>(document.source[1]) == 0xBB &&
        static_cast<unsigned char>(document.source[2]) == 0xBF) {
        document.source.erase(0, 3);
    }

    enum class Stage { None, Vertex, Fragment };
    Stage stage = Stage::None;
    int vertexMarkers = 0;
    int fragmentMarkers = 0;
    int stageLine = 0;
    std::ostringstream vertex;
    std::ostringstream fragment;
    document.vertexLineMap.clear();
    document.fragmentLineMap.clear();

    std::istringstream input(document.source);
    std::string line;
    int documentLine = 0;
    while (std::getline(input, line)) {
        ++documentLine;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == "#type vertex") {
            stage = Stage::Vertex;
            stageLine = 0;
            ++vertexMarkers;
            continue;
        }
        if (line == "#type fragment") {
            stage = Stage::Fragment;
            stageLine = 0;
            ++fragmentMarkers;
            continue;
        }
        if (stage == Stage::Vertex) {
            ++stageLine;
            vertex << line << '\n';
            document.vertexLineMap.push_back({stageLine, documentLine});
        } else if (stage == Stage::Fragment) {
            ++stageLine;
            fragment << line << '\n';
            document.fragmentLineMap.push_back({stageLine, documentLine});
        }
    }

    if (vertexMarkers != 1 || fragmentMarkers != 1) {
        return {false, "Phoenix shader must contain exactly one '#type vertex' and one '#type fragment' marker."};
    }
    if (document.vertexLineMap.empty() || document.fragmentLineMap.empty()) {
        return {false, "Phoenix shader vertex and fragment sections cannot be empty."};
    }

    document.vertexSource = vertex.str();
    document.fragmentSource = fragment.str();
    return {true, {}};
}
}  // namespace shadereditor
