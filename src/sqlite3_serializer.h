#ifndef SQLITE3_SERIALIZER_H
#define SQLITE3_SERIALIZER_H

#include "recap.h"
#include <sstream>
#include <cstdarg>
struct sqlite3;
struct sqlite3_stmt;

//------------------------------------------------------------------------------
// SQLite3 implementation of the serialization interface.
//------------------------------------------------------------------------------
class SQLite3_Serializer : public Serializer {

    public:

        //----------------------------------------------------------------------
        // @param db_spec The filespec of the database
        // @post  A connection to the database is established and the table
        //        schemas are created (if necessary).
        //----------------------------------------------------------------------
        SQLite3_Serializer(const char* db_spec)
            throw(std::runtime_error);

        ~SQLite3_Serializer();

        //----------------------------------------------------------------------
        // @param i The Item to be written.
        // @pre   The Item has blank or empty fields.
        // @post  The Item is serialized and stored.
        // @throw If cannot write through the DB connection.
        //----------------------------------------------------------------------
        virtual void write(const Item&) 
            throw(std::runtime_error);

        //----------------------------------------------------------------------
        // @param tags    An in vector of tag strings.
        // @param items   An out vector to store the Items.
        // @post  All Items associated with the tags are returned in the out 
        //        parameter.
        // @throw If cannot read via the DB connection
        //----------------------------------------------------------------------
        virtual void read(const std::vector<std::string>& tags, 
                          std::vector<Item*>& items) 

            throw(std::runtime_error);
        
        //---------------------------------------------------------------------
        // @param tags Out vector of tag strings
        // @post  All tags stored in the DB are loaded into the out vector.
        // @throw If errors occur querying the DB.
        //---------------------------------------------------------------------
        virtual void tags(std::vector<std::string>& tags) 
            throw(std::runtime_error);

    private:
        void prepare(int, ...) throw(std::runtime_error);
        int  step()            throw(std::runtime_error);

        sqlite3*      m_db;
        sqlite3_stmt* m_statement;
        char*         m_error_msg;

        std::stringstream  m_query;
};

#endif 