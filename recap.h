#ifndef RECAP_H
#define RECAP_H
#include <vector>
#include <stdexcept>

//------------------------------------------------------------------------------
// Core data type. 
//------------------------------------------------------------------------------
struct Item {
    std::string title;
    std::string content;
    std::vector<std::string> tags;
};

//------------------------------------------------------------------------------
// Simple Item serialization interface.
//------------------------------------------------------------------------------
class Serializer {

    public:
        virtual ~Serializer(){};

        //---------------------------------------------------------------------
        // @param i The Item to be written.
        // @pre   The Item has no blank or empty fields.
        // @post  The Item is serialized and stored.
        // @throw If errors occur writing the Item.
        //---------------------------------------------------------------------
        virtual void write(const Item&) 
            throw(std::runtime_error) = 0;

        //---------------------------------------------------------------------
        // @param tags    An in vector of tag strings.
        // @param items   An out vector to store the Items.
        // @post  All Items associated with the tags are returned in the out 
        //        parameter.
        // @throw If errors occur reading the Item.
        //---------------------------------------------------------------------
        virtual void read(const std::vector<std::string>& tags, 
                          std::vector<Item*>& items) 

            throw(std::runtime_error) = 0;

};
#endif
