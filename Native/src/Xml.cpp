#include <iiXml/Xml.h>
#include <libxml/xmlreader.h>
#include <libxml/tree.h>
#include <algorithm>
#include <limits>
#include <set>

namespace ii::xml {
namespace {
auto fail(ErrorCode code,std::string message,int line=0) { return std::unexpected(Error{code,line,std::move(message)}); }
std::string string(const xmlChar* p) { return p?reinterpret_cast<const char*>(p):""; }
bool name(std::string_view text) {
    if(text.empty() || text.find('\0')!=text.npos) return false;
    const std::string owned(text);
    return xmlValidateQName(reinterpret_cast<const xmlChar*>(owned.c_str()),0)==0;
}
Result<void> doctype_check(std::string_view input) {
    // UTF-8 only, with no embedded NUL. Reject resource/entity declarations
    // before constructing a parser; no global entity loader is installed.
    if(input.find('\0')!=input.npos) return fail(ErrorCode::syntax,"XML input must be UTF-8 without NUL");
    for(std::size_t pos=0;(pos=input.find('<',pos))!=input.npos;) {
        const auto remaining=input.substr(pos);
        std::string_view delimiter;
        if(remaining.starts_with("<!--")) delimiter="-->";
        else if(remaining.starts_with("<![CDATA[")) delimiter="]]>";
        else if(remaining.starts_with("<?")) delimiter="?>";
        if(!delimiter.empty()) {
            const auto end=input.find(delimiter,pos+2);
            if(end==input.npos) return fail(ErrorCode::syntax,"Unclosed XML comment, CDATA or instruction");
            pos=end+delimiter.size(); continue;
        }
        if(!remaining.starts_with("<!DOCTYPE")) { ++pos; continue; }
        const auto end=input.find('>',pos); if(end==input.npos) return fail(ErrorCode::syntax,"Unclosed DOCTYPE");
        auto root=input.substr(pos+9,end-pos-9);
        while(!root.empty() && (root.front()==' ' || root.front()=='\t' || root.front()=='\n' || root.front()=='\r')) root.remove_prefix(1);
        while(!root.empty() && (root.back()==' ' || root.back()=='\t' || root.back()=='\n' || root.back()=='\r')) root.remove_suffix(1);
        if(!name(root)) return fail(ErrorCode::forbidden_dtd,"External identifiers, entities and DTD subsets are forbidden");
        pos=end+1;
    }
    return {};
}
}
struct Reader::State final {
    std::string input,diagnostic;
    xmlTextReaderPtr reader=nullptr;
    Limits limits;
    std::size_t events=0;
    int line=0;
    std::optional<Error> failure;
    bool ended=false;
    ~State() { if(reader) xmlFreeTextReader(reader); }
    static void error(void* context,const char* message,xmlParserSeverities severity,xmlTextReaderLocatorPtr locator) {
        if(severity!=XML_PARSER_SEVERITY_ERROR && severity!=XML_PARSER_SEVERITY_VALIDITY_ERROR) return;
        auto& state=*static_cast<State*>(context);
        state.diagnostic=message?message:"Malformed XML";
        state.line=locator?xmlTextReaderLocatorLineNumber(locator):0;
    }
};
Reader::Reader(std::unique_ptr<State> state):state_(std::move(state)){}
Reader::~Reader()=default;
Reader::Reader(Reader&&) noexcept=default;
Reader& Reader::operator=(Reader&&) noexcept=default;
Result<Reader> Reader::open(std::string utf8,Limits limits) {
    if(utf8.empty()) return fail(ErrorCode::syntax,"XML input is empty");
    if(utf8.size()>limits.input_bytes || utf8.size()>std::numeric_limits<int>::max()) return fail(ErrorCode::limit_exceeded,"XML input exceeds its byte budget");
    if(auto safe=doctype_check(utf8); !safe) return std::unexpected(safe.error());
    auto state=std::make_unique<State>(); state->input=std::move(utf8); state->limits=limits;
    state->reader=xmlReaderForMemory(state->input.data(),static_cast<int>(state->input.size()),nullptr,"UTF-8",XML_PARSE_NONET|XML_PARSE_COMPACT);
    if(!state->reader) return fail(ErrorCode::syntax,"XML reader initialization failed");
    xmlTextReaderSetErrorHandler(state->reader,State::error,state.get());
    return Reader(std::move(state));
}
Result<std::optional<Event>> Reader::next() {
    if(!state_) return fail(ErrorCode::invalid_state,"XML reader was moved");
    auto& s=*state_;
    if(s.failure) return std::unexpected(*s.failure);
    if(s.ended) return std::optional<Event>{};
    const auto reject=[&](ErrorCode code,std::string message)->Result<std::optional<Event>> {
        s.failure=Error{code,xmlTextReaderGetParserLineNumber(s.reader),std::move(message)};
        return std::unexpected(*s.failure);
    };
    while(true) {
        const auto status=xmlTextReaderRead(s.reader);
        if(status<0 || !s.diagnostic.empty()) return reject(ErrorCode::syntax,s.diagnostic.empty()?"Malformed XML":s.diagnostic);
        if(!status) { s.ended=true; return std::optional<Event>{}; }
        if(++s.events>s.limits.events) return reject(ErrorCode::limit_exceeded,"XML event count exceeds its budget");
        const int rawDepth=xmlTextReaderDepth(s.reader);
        if(rawDepth<0 || static_cast<std::size_t>(rawDepth)>=s.limits.depth) return reject(ErrorCode::limit_exceeded,"XML depth exceeds its budget");
        Kind kind;
        switch(xmlTextReaderNodeType(s.reader)) {
            case XML_READER_TYPE_ELEMENT: kind=Kind::start_element; break;
            case XML_READER_TYPE_END_ELEMENT: kind=Kind::end_element; break;
            case XML_READER_TYPE_TEXT: case XML_READER_TYPE_CDATA:
            case XML_READER_TYPE_WHITESPACE: case XML_READER_TYPE_SIGNIFICANT_WHITESPACE: kind=Kind::text; break;
            case XML_READER_TYPE_COMMENT: kind=Kind::comment; break;
            case XML_READER_TYPE_PROCESSING_INSTRUCTION: kind=Kind::processing_instruction; break;
            case XML_READER_TYPE_DOCUMENT_TYPE: kind=Kind::doctype; break;
            case XML_READER_TYPE_XML_DECLARATION: continue;
            default: return reject(ErrorCode::syntax,"Unsupported XML entity or node");
        }
        Event event{kind}; event.depth=static_cast<std::size_t>(rawDepth);
        event.name=string(xmlTextReaderConstName(s.reader)); event.local_name=string(xmlTextReaderConstLocalName(s.reader));
        event.namespace_uri=string(xmlTextReaderConstNamespaceUri(s.reader)); event.value=string(xmlTextReaderConstValue(s.reader));
        event.empty=xmlTextReaderIsEmptyElement(s.reader)==1;
        if(event.name.size()>s.limits.text_bytes || event.value.size()>s.limits.text_bytes) return reject(ErrorCode::limit_exceeded,"XML text exceeds its byte budget");
        if(kind==Kind::start_element) {
            auto count=xmlTextReaderAttributeCount(s.reader);
            if(count<0 || static_cast<std::size_t>(count)>s.limits.attributes) return reject(ErrorCode::limit_exceeded,"XML attributes exceed their budget");
            if(xmlTextReaderMoveToFirstAttribute(s.reader)==1) {
                do {
                    Attribute attribute{string(xmlTextReaderConstName(s.reader)),string(xmlTextReaderConstValue(s.reader)),
                        string(xmlTextReaderConstLocalName(s.reader)),string(xmlTextReaderConstNamespaceUri(s.reader))};
                    if(attribute.name.size()>s.limits.text_bytes || attribute.value.size()>s.limits.text_bytes) return reject(ErrorCode::limit_exceeded,"XML attribute exceeds its byte budget");
                    event.attributes.push_back(std::move(attribute));
                } while(xmlTextReaderMoveToNextAttribute(s.reader)==1);
                xmlTextReaderMoveToElement(s.reader);
            }
        }
        return std::optional<Event>{std::move(event)};
    }
}
struct Writer::State final {
    Limits limits;
    std::string output="<?xml version=\"1.0\" encoding=\"UTF-8\"?>",dtd;
    std::vector<std::string> stack;
    bool root=false,finished=false;
    explicit State(Limits value):limits(value){}
    Result<void> append(std::string_view bytes) {
        if(finished) return fail(ErrorCode::invalid_state,"XML writer is finalized");
        if(output.size()>limits.output_bytes || bytes.size()>limits.output_bytes-output.size()) return fail(ErrorCode::limit_exceeded,"XML output exceeds its byte budget");
        output.append(bytes); return {};
    }
    Result<std::string> escaped(std::string_view text,bool attribute) const {
        if(text.size()>limits.text_bytes || text.size()>limits.output_bytes) return fail(ErrorCode::limit_exceeded,"XML text exceeds its byte budget");
        std::string output;
        for(char c:text) {
            std::string_view token;
            switch(c) {
                case '&': token="&amp;"; break; case '<': token="&lt;"; break; case '>': token="&gt;"; break;
                case '"': if(attribute) token="&quot;"; break;
                case '\r': token="&#13;"; break;
                case '\n': if(attribute) token="&#10;"; break;
                case '\t': if(attribute) token="&#9;"; break;
            }
            if(output.size()>limits.output_bytes || (token.empty()?1:token.size())>limits.output_bytes-output.size()) return fail(ErrorCode::limit_exceeded,"Escaped XML text exceeds its budget");
            if(token.empty()) output.push_back(c); else output.append(token);
        }
        return output;
    }
};
Writer::Writer(Limits limits):state_(std::make_unique<State>(limits)){}
Writer::~Writer()=default;
Writer::Writer(Writer&&) noexcept=default;
Writer& Writer::operator=(Writer&&) noexcept=default;
Result<void> Writer::doctype(std::string_view root) {
    if(!state_ || state_->root || !state_->dtd.empty() || !name(root)) return fail(ErrorCode::invalid_state,"DOCTYPE requires a valid root name before the root");
    if(root.size()>state_->limits.text_bytes) return fail(ErrorCode::limit_exceeded,"DOCTYPE name exceeds its budget");
    if(auto ok=state_->append("<!DOCTYPE "+std::string(root)+">"); !ok) return ok;
    state_->dtd=root; return {};
}
Result<void> Writer::start(std::string_view tag,std::span<const Attribute> attributes) {
    if(!state_ || state_->finished) return fail(ErrorCode::invalid_state,"XML writer is not active");
    auto& s=*state_;
    if(!name(tag)) return fail(ErrorCode::syntax,"Invalid element name");
    if(tag.size()>s.limits.text_bytes || s.stack.size()>=s.limits.depth || attributes.size()>s.limits.attributes) return fail(ErrorCode::limit_exceeded,"XML element exceeds its budget");
    if(s.stack.empty() && (s.root || (!s.dtd.empty() && s.dtd!=tag))) return fail(ErrorCode::invalid_state,"XML requires one root matching its DOCTYPE");
    std::string buffer="<"+std::string(tag);
    std::set<std::string> names;
    for(const auto& attribute:attributes) {
        if(!name(attribute.name) || !names.insert(attribute.name).second) return fail(ErrorCode::syntax,"Invalid or duplicate attribute name");
        auto value=s.escaped(attribute.value,true); if(!value) return std::unexpected(value.error());
        if(attribute.name.size()>s.limits.text_bytes || buffer.size()>s.limits.output_bytes ||
            attribute.name.size()+value->size()+4>s.limits.output_bytes-buffer.size()) return fail(ErrorCode::limit_exceeded,"XML attributes exceed output budget");
        buffer+=" "+attribute.name+"=\""+*value+"\"";
    }
    buffer+='>'; if(auto ok=s.append(buffer); !ok) return ok;
    s.root=true; s.stack.emplace_back(tag); return {};
}
Result<void> Writer::text(std::string_view text) {
    if(!state_ || state_->stack.empty()) return fail(ErrorCode::invalid_state,"XML text requires an open element");
    auto escaped=state_->escaped(text,false); if(!escaped) return std::unexpected(escaped.error());
    return state_->append(*escaped);
}
Result<void> Writer::end() {
    if(!state_ || state_->stack.empty()) return fail(ErrorCode::invalid_state,"No open XML element");
    auto& s=*state_; if(auto ok=s.append("</"+s.stack.back()+">"); !ok) return ok;
    s.stack.pop_back(); return {};
}
Result<std::string> Writer::finish() {
    if(!state_ || state_->finished || !state_->root || !state_->stack.empty()) return fail(ErrorCode::invalid_state,"XML writer requires one closed root");
    auto limits=state_->limits; limits.input_bytes=limits.output_bytes;
    auto reader=Reader::open(state_->output,limits); if(!reader) return std::unexpected(reader.error());
    while(true) { auto event=reader->next(); if(!event) return std::unexpected(event.error()); if(!*event) break; }
    state_->finished=true; return std::move(state_->output);
}
}
