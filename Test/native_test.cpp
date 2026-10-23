#include <iiXml/Xml.h>
#include <cstdlib>
#include <iostream>
using namespace ii::xml;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::exit(1); } } while(false)
int main() {
    auto reader=Reader::open("<?xml version=\"1.0\"?><svg xmlns=\"urn:svg\" xmlns:x=\"urn:x\" x:id=\"가&amp;나\"><path/><g>한&lt;글</g></svg>");
    CHECK(reader); int starts=0,ends=0,texts=0;
    while (true) {
        auto next=reader->next(); CHECK(next); if (!*next) break;
        auto &e=**next;
        if(e.kind==Kind::start_element) { ++starts; CHECK(e.namespace_uri=="urn:svg");
            if(e.local_name=="svg") { CHECK(e.attributes.size()==3); CHECK(e.attributes[2].value=="가&나" && e.attributes[2].namespace_uri=="urn:x"); }
            if(e.local_name=="path") CHECK(e.empty);
        }
        if(e.kind==Kind::end_element) ++ends;
        if(e.kind==Kind::text) { ++texts; CHECK(e.value=="한<글"); }
    }
    CHECK(starts==3 && ends==2 && texts==1); CHECK(reader->next() && !*reader->next());
    Writer writer;
    CHECK(writer.doctype("fcpxml")); CHECK(writer.start("fcpxml"));
    std::vector<Attribute> attrs{{"name","A&B\"<가"}};
    CHECK(writer.start("asset",attrs)); CHECK(writer.text("text & < > 한글")); CHECK(writer.end()); CHECK(writer.end());
    auto xml=writer.finish(); CHECK(xml); CHECK(!writer.finish());
    auto written=Reader::open(*xml); CHECK(written);
    int events=0; while(true) { auto e=written->next(); CHECK(e); if(!*e) break; ++events; } CHECK(events==6);
    for(const auto &bad:{"<!DOCTYPE a SYSTEM 'file:///etc/passwd'><a/>","<!DOCTYPE a [<!ENTITY x 'xx'>]><a>&x;</a>"}) CHECK(!Reader::open(bad));
    auto broken=Reader::open("<a><b></a>"); CHECK(broken);
    bool failed=false; while(true) { auto e=broken->next(); if(!e) { failed=true; break; } if(!*e) break; } CHECK(failed);
    Limits small; small.depth=1; auto deep=Reader::open("<a><b/></a>",small); CHECK(deep);
    CHECK(deep->next()); CHECK(!deep->next());
    small={}; small.input_bytes=2; CHECK(!Reader::open("<a/>",small));
    small={}; small.attributes=1; auto many=Reader::open("<a x='1' y='2'/>",small); CHECK(many && !many->next());
    small={}; small.events=1; auto tooMany=Reader::open("<a>x</a>",small); CHECK(tooMany && tooMany->next() && !tooMany->next());
    small={}; small.output_bytes=32; Writer bounded(small); CHECK(!bounded.start(std::string(40,'x')));
    Writer unfinished; CHECK(unfinished.start("a")); CHECK(!unfinished.finish());
    Writer invalid; CHECK(!invalid.start("bad name")); CHECK(!invalid.end());
    Writer badText; CHECK(badText.start("a")); CHECK(badText.text(std::string("\0",1))); CHECK(badText.end()); CHECK(!badText.finish());
    auto literal=Reader::open("<a><!-- example <!DOCTYPE b SYSTEM 'file:///unused'> --><![CDATA[<!DOCTYPE b [literal]>]]></a>"); CHECK(literal);
    while(true) { auto e=literal->next(); CHECK(e); if(!*e) break; }
    std::cout << "native XML contracts passed\n";
}
