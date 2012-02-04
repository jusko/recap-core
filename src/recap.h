#ifndef RECAP_H
#define RECAP_H
#include <vector>
#include <stdexcept>

//------------------------------------------------------------------------------
// Core data type. 
//------------------------------------------------------------------------------
struct Item {
    int id;
    bool encrypted;
    std::string title;
    std::string content;
    std::string timestamp;
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
        // @post  The Item is serialized and a new Item is created or an
        //        existing Item is updated.
        // @throw If errors occur writing the Item.
        //---------------------------------------------------------------------
        virtual void write(Item& i) 
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

        //---------------------------------------------------------------------
        // @param i The item to be deleted
        // @pre     The item is stored.
        // @post    The item is considered "trash" and will not be read in
        //          future calls to read().
        //---------------------------------------------------------------------
        virtual void trash(const Item& i) 
            throw(std::runtime_error) = 0;

        //---------------------------------------------------------------------
        // @param tags Out vector of tag strings
        // @post  All existing tags are loaded into the out vector.
        // @throw If errors occur reading the tags.
        //---------------------------------------------------------------------
        virtual void tags(std::vector<std::string>& tags) 
            throw(std::runtime_error) = 0;
};
#endif
