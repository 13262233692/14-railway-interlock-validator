#include "topology/XmlParser.h"
#include <fstream>
#include <sstream>
#include <cctype>

namespace railway {
namespace xml {

const std::string& XmlElement::getName() const {
    return name_;
}

void XmlElement::setName(const std::string& name) {
    name_ = name;
}

const std::string& XmlElement::getText() const {
    return text_;
}

void XmlElement::setText(const std::string& text) {
    text_ = text;
}

void XmlElement::appendText(const std::string& text) {
    text_ += text;
}

const std::vector<XmlAttribute>& XmlElement::getAttributes() const {
    return attributes_;
}

void XmlElement::addAttribute(const std::string& name, const std::string& value) {
    attributes_.push_back({name, value});
}

std::string XmlElement::getAttributeValue(const std::string& name) const {
    for (const auto& attr : attributes_) {
        if (attr.name == name) {
            return attr.value;
        }
    }
    return "";
}

bool XmlElement::hasAttribute(const std::string& name) const {
    for (const auto& attr : attributes_) {
        if (attr.name == name) {
            return true;
        }
    }
    return false;
}

const std::vector<std::shared_ptr<XmlElement>>& XmlElement::getChildren() const {
    return children_;
}

void XmlElement::addChild(std::shared_ptr<XmlElement> child) {
    children_.push_back(child);
}

std::vector<std::shared_ptr<XmlElement>> XmlElement::getChildrenByName(const std::string& name) const {
    std::vector<std::shared_ptr<XmlElement>> result;
    for (const auto& child : children_) {
        if (child->getName() == name) {
            result.push_back(child);
        }
    }
    return result;
}

std::shared_ptr<XmlElement> XmlElement::getFirstChildByName(const std::string& name) const {
    for (const auto& child : children_) {
        if (child->getName() == name) {
            return child;
        }
    }
    return nullptr;
}

XmlParser::XmlParser() : pos_(0) {
}

std::shared_ptr<XmlElement> XmlParser::parse(const std::string& xmlContent) {
    xml_ = xmlContent;
    pos_ = 0;
    lastError_.clear();

    skipWhitespace();

    if (pos_ < xml_.size() && xml_[pos_] == '<') {
        if (xml_.substr(pos_, 5) == "<?xml") {
            if (!parseDeclaration()) {
                return nullptr;
            }
            skipWhitespace();
        }
    }

    auto root = parseElement();
    if (!root) {
        if (lastError_.empty()) {
            lastError_ = "Failed to parse root element";
        }
        return nullptr;
    }

    return root;
}

std::shared_ptr<XmlElement> XmlParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filePath;
        return nullptr;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    return parse(content);
}

std::string XmlParser::getLastError() const {
    return lastError_;
}

void XmlParser::skipWhitespace() {
    while (pos_ < xml_.size() && std::isspace(static_cast<unsigned char>(xml_[pos_]))) {
        pos_++;
    }
}

bool XmlParser::parseDeclaration() {
    if (!matchString("<?xml")) {
        lastError_ = "Expected '<?xml'";
        return false;
    }

    while (pos_ < xml_.size()) {
        if (xml_.substr(pos_, 2) == "?>") {
            pos_ += 2;
            return true;
        }
        pos_++;
    }

    lastError_ = "Unterminated XML declaration";
    return false;
}

std::shared_ptr<XmlElement> XmlParser::parseElement() {
    if (pos_ >= xml_.size() || xml_[pos_] != '<') {
        lastError_ = "Expected '<' at position " + std::to_string(pos_);
        return nullptr;
    }

    pos_++;

    if (xml_.substr(pos_, 3) == "!--") {
        skipComment();
        skipWhitespace();
        return parseElement();
    }

    std::string name;
    if (!parseName(name)) {
        lastError_ = "Expected element name at position " + std::to_string(pos_);
        return nullptr;
    }

    auto element = std::make_shared<XmlElement>();
    element->setName(name);

    if (!parseAttributes(*element)) {
        return nullptr;
    }

    skipWhitespace();

    if (pos_ < xml_.size() && xml_[pos_] == '/') {
        pos_++;
        if (pos_ >= xml_.size() || xml_[pos_] != '>') {
            lastError_ = "Expected '>' after '/' in self-closing tag";
            return nullptr;
        }
        pos_++;
        return element;
    }

    if (pos_ >= xml_.size() || xml_[pos_] != '>') {
        lastError_ = "Expected '>' at position " + std::to_string(pos_);
        return nullptr;
    }
    pos_++;

    parseContent(*element);

    skipWhitespace();
    if (pos_ + 1 >= xml_.size() || xml_[pos_] != '<' || xml_[pos_ + 1] != '/') {
        lastError_ = "Expected closing tag </" + name + ">";
        return nullptr;
    }
    pos_ += 2;

    std::string endName;
    if (!parseName(endName) || endName != name) {
        lastError_ = "Closing tag name mismatch: expected " + name;
        return nullptr;
    }

    skipWhitespace();
    if (pos_ >= xml_.size() || xml_[pos_] != '>') {
        lastError_ = "Expected '>' at end of closing tag";
        return nullptr;
    }
    pos_++;

    return element;
}

bool XmlParser::parseName(std::string& outName) {
    if (pos_ >= xml_.size()) {
        return false;
    }

    char first = xml_[pos_];
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_' && first != ':') {
        return false;
    }

    size_t start = pos_;
    pos_++;
    while (pos_ < xml_.size()) {
        char c = xml_[pos_];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == ':') {
            pos_++;
        } else {
            break;
        }
    }

    outName = xml_.substr(start, pos_ - start);
    return true;
}

bool XmlParser::parseAttributes(XmlElement& element) {
    skipWhitespace();
    while (pos_ < xml_.size() && xml_[pos_] != '>' && xml_[pos_] != '/') {
        XmlAttribute attr;
        if (!parseAttribute(attr)) {
            return false;
        }
        element.addAttribute(attr.name, attr.value);
        skipWhitespace();
    }
    return true;
}

bool XmlParser::parseAttribute(XmlAttribute& outAttr) {
    std::string name;
    if (!parseName(name)) {
        lastError_ = "Expected attribute name";
        return false;
    }

    skipWhitespace();
    if (pos_ >= xml_.size() || xml_[pos_] != '=') {
        lastError_ = "Expected '=' after attribute name";
        return false;
    }
    pos_++;

    skipWhitespace();
    if (pos_ >= xml_.size() || (xml_[pos_] != '"' && xml_[pos_] != '\'')) {
        lastError_ = "Expected quote in attribute value";
        return false;
    }

    char quote = xml_[pos_];
    pos_++;

    std::string value;
    while (pos_ < xml_.size() && xml_[pos_] != quote) {
        if (xml_[pos_] == '&') {
            pos_++;
            if (xml_.substr(pos_, 4) == "quot") {
                value += '"';
                pos_ += 4;
            } else if (xml_.substr(pos_, 3) == "amp") {
                value += '&';
                pos_ += 3;
            } else if (xml_.substr(pos_, 3) == "lt;") {
                value += '<';
                pos_ += 3;
            } else if (xml_.substr(pos_, 3) == "gt;") {
                value += '>';
                pos_ += 3;
            } else if (xml_.substr(pos_, 5) == "apos;") {
                value += '\'';
                pos_ += 5;
            } else {
                value += '&';
            }
        } else {
            value += xml_[pos_];
            pos_++;
        }
    }

    if (pos_ >= xml_.size()) {
        lastError_ = "Unterminated attribute value";
        return false;
    }
    pos_++;

    outAttr.name = name;
    outAttr.value = value;
    return true;
}

bool XmlParser::matchString(const std::string& s) {
    if (xml_.substr(pos_, s.size()) == s) {
        pos_ += s.size();
        return true;
    }
    return false;
}

bool XmlParser::matchChar(char c) {
    if (pos_ < xml_.size() && xml_[pos_] == c) {
        pos_++;
        return true;
    }
    return false;
}

char XmlParser::peekChar() {
    if (pos_ < xml_.size()) {
        return xml_[pos_];
    }
    return '\0';
}

char XmlParser::nextChar() {
    if (pos_ < xml_.size()) {
        return xml_[pos_++];
    }
    return '\0';
}

void XmlParser::skipComment() {
    pos_ += 3;
    while (pos_ + 2 < xml_.size()) {
        if (xml_.substr(pos_, 3) == "-->") {
            pos_ += 3;
            return;
        }
        pos_++;
    }
    pos_ = xml_.size();
}

void XmlParser::parseContent(XmlElement& element) {
    std::string text;

    while (pos_ < xml_.size()) {
        if (xml_[pos_] == '<') {
            if (pos_ + 1 < xml_.size() && xml_[pos_ + 1] == '/') {
                break;
            }
            if (xml_.substr(pos_, 4) == "<!--") {
                if (!text.empty()) {
                    element.appendText(text);
                    text.clear();
                }
                skipComment();
                continue;
            }

            if (!text.empty()) {
                element.appendText(text);
                text.clear();
            }

            auto child = parseElement();
            if (child) {
                element.addChild(child);
            }
        } else {
            text += xml_[pos_];
            pos_++;
        }
    }

    if (!text.empty()) {
        element.appendText(text);
    }
}

} // namespace xml
} // namespace railway
