#ifndef RAILWAY_XML_PARSER_H
#define RAILWAY_XML_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace railway {
namespace xml {

struct XmlAttribute {
    std::string name;
    std::string value;
};

class XmlElement {
public:
    XmlElement() = default;

    const std::string& getName() const;
    void setName(const std::string& name);

    const std::string& getText() const;
    void setText(const std::string& text);
    void appendText(const std::string& text);

    const std::vector<XmlAttribute>& getAttributes() const;
    void addAttribute(const std::string& name, const std::string& value);
    std::string getAttributeValue(const std::string& name) const;
    bool hasAttribute(const std::string& name) const;

    const std::vector<std::shared_ptr<XmlElement>>& getChildren() const;
    void addChild(std::shared_ptr<XmlElement> child);

    std::vector<std::shared_ptr<XmlElement>> getChildrenByName(const std::string& name) const;
    std::shared_ptr<XmlElement> getFirstChildByName(const std::string& name) const;

private:
    std::string name_;
    std::string text_;
    std::vector<XmlAttribute> attributes_;
    std::vector<std::shared_ptr<XmlElement>> children_;
};

class XmlParser {
public:
    XmlParser();

    std::shared_ptr<XmlElement> parse(const std::string& xmlContent);
    std::shared_ptr<XmlElement> parseFile(const std::string& filePath);

    std::string getLastError() const;

private:
    void skipWhitespace();
    bool parseDeclaration();
    std::shared_ptr<XmlElement> parseElement();
    bool parseName(std::string& outName);
    bool parseAttributes(XmlElement& element);
    bool parseAttribute(XmlAttribute& outAttr);
    bool matchString(const std::string& s);
    bool matchChar(char c);
    char peekChar();
    char nextChar();
    void skipComment();
    void parseContent(XmlElement& element);

    std::string xml_;
    size_t pos_;
    std::string lastError_;
};

} // namespace xml
} // namespace railway

#endif // RAILWAY_XML_PARSER_H
