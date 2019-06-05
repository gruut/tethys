#ifndef TETHYSSCE_XML_TOOL_HPP
#define TETHYSSCE_XML_TOOL_HPP

#include <string>
#include <optional>

//#include <pugixml.hpp>
#include <tinyxml2.h>
#include <vector>

class XmlTool {
public:

  static std::optional<tinyxml2::XMLElement*> parseChildFrom(tinyxml2::XMLElement *from_node, std::string_view element_name) {
    if (from_node == nullptr) {
      return std::nullopt;
    } else {
      std::string name(element_name);
      return from_node->FirstChildElement(name.c_str());
    }
  }

  static std::vector<std::pair<tinyxml2::XMLElement*,std::string>> parseChildrenFrom(tinyxml2::XMLElement *from_node, std::string_view element_name) {

    std::vector<std::pair<tinyxml2::XMLElement*,std::string>> parsed_children;

    std::string name(element_name);

    if (from_node != nullptr) {
      tinyxml2::XMLElement *element_ptr;
      element_ptr = from_node->FirstChildElement(name.c_str());
      while(element_ptr) {
        std::string if_id = mt::c2s(element_ptr->Attribute("if"));
        parsed_children.push_back({element_ptr,if_id});

        element_ptr = element_ptr->NextSiblingElement(name.c_str());
      }
    }

    return parsed_children;
  }

  static std::vector<tinyxml2::XMLElement*> parseChildrenFromNoIf(tinyxml2::XMLElement *from_node,
                                                                  std::string_view element_name) {

    std::vector<tinyxml2::XMLElement*> parsed_children;

    std::string name(element_name);

    if (from_node != nullptr) {
      tinyxml2::XMLElement *element_ptr;
      element_ptr = from_node->FirstChildElement(name.c_str());
      while(element_ptr) {
        parsed_children.push_back(element_ptr);

        element_ptr = element_ptr->NextSiblingElement(name.c_str());
      }
    }

    return parsed_children;
  }





#if 0
  static std::optional<pugi::xml_node> parseChildFrom(pugi::xml_node &from_node, std::string_view element_name) {

    pugi::xml_node parsed_child;
    bool found = false;

    for(auto &each_child : from_node){
      if(each_child.name() == element_name) {
        parsed_child.append_copy(each_child);
        found = true;
        break;
      }
    }

    if(found) {
      return parsed_child;
    }
    else {
      return std::nullopt;
    }
  }

  static std::vector<std::pair<pugi::xml_node,std::string>> parseChildrenFrom(pugi::xml_node &from_node,
                                                                              std::string_view element_name) {

    std::vector<std::pair<pugi::xml_node,std::string>> parsed_children;

    for(auto &each_child : from_node){
      if(each_child.name() == element_name) {
        pugi::xml_node parsed_child;
        parsed_child.append_copy(each_child);
        parsed_children.push_back({parsed_child,each_child.attribute("if").value()});
      }
    }

    return parsed_children;
  }

  static std::vector<pugi::xml_node> parseChildrenFromNoIf(pugi::xml_node &from_node, std::string_view element_name) {

    std::vector<pugi::xml_node> parsed_children;

    for(auto &each_child : from_node){
      if(each_child.name() == element_name) {
        pugi::xml_node parsed_child;
        parsed_child.append_copy(each_child);
        parsed_children.push_back(parsed_child);
      }
    }

    return parsed_children;
  }
#endif
};

#endif //TETHYSSCE_XML_TOOL_HPP
