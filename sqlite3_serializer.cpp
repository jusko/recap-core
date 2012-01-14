#include "sqlite3_serializer.h"
#include <sqlite3.h>
#include <string>
using namespace std;

//--------------------------------------------------------------------------------
// Table creation statements
// TODO: Change STRING to TEXT
//--------------------------------------------------------------------------------
#define ITEM_DDL     "CREATE TABLE IF NOT EXISTS Item("\
                        "ItemID INTEGER PRIMARY KEY, Title TEXT, Content TEXT);"

#define TAG_DDL      "CREATE TABLE IF NOT EXISTS Tag("\
                        "TagID INTEGER PRIMARY KEY, Title TEXT UNIQUE);"

#define ITEM_TAG_DDL "CREATE TABLE IF NOT EXISTS ItemTag("\
                        "ID INTEGER PRIMARY KEY, ItemID INTEGER, TagID INTEGER, "\
                        "FOREIGN KEY(ItemID) REFERENCES Item(ItemID), "\
                        "FOREIGN KEY(TagID) REFERENCES Tag(TagID));"

#define CREATE_SCHEMA_DDL ITEM_DDL TAG_DDL ITEM_TAG_DDL "PRAGMA foreign_keys = ON;"

//--------------------------------------------------------------------------------
// Convenience function for executing and checking SQL queries
// @pre m_query is set with a valid SQL query
// TODO: Consider removing in favour of other helpers
//--------------------------------------------------------------------------------
inline void SQLite3_Serializer::exec()
    throw(runtime_error) {

    if (sqlite3_exec(m_db, m_query.str().c_str(), 0, 0, &m_error_msg) != SQLITE_OK) {
        throw runtime_error(m_error_msg);
    }
}

//--------------------------------------------------------------------------------
// Convenience function for preparing SQL statements
// @pre  m_sql is set with a valid SQL query
// @post m_statement is prepared with a binary SQL statement
//--------------------------------------------------------------------------------
inline void SQLite3_Serializer::prepare() 
    throw(runtime_error) {

    if (sqlite3_prepare_v2(
        m_db,
        m_query.str().c_str(),
        m_query.str().size(),
        &m_statement,
        NULL) != SQLITE_OK) {

        throw runtime_error(sqlite3_errmsg(m_db));
    }
}

//--------------------------------------------------------------------------------
// Convenience function for stepping through the rows returned by a prepared
// query.
// @pre  m_statement has been prepared with prepare()
// @post m_statement can be passed to sqlite3 column getters to inspect results.
//--------------------------------------------------------------------------------
inline int SQLite3_Serializer::step() 
    throw(runtime_error) {

    int rc =  sqlite3_step(m_statement);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw runtime_error(sqlite3_errmsg(m_db));
    }
    return rc;
}

//--------------------------------------------------------------------------------
// Ctor: Initialises the database connection and creates the schema if need be.
//--------------------------------------------------------------------------------
SQLite3_Serializer::SQLite3_Serializer(const char* db_spec) 
    throw(runtime_error) : m_db(0),
                           m_statement(0),
                           m_error_msg(0),
                           m_query(CREATE_SCHEMA_DDL) {

    if ((sqlite3_open(db_spec, &m_db) != SQLITE_OK)) {
        throw runtime_error(string(sqlite3_errmsg(m_db)));
    }
    exec();
}

//--------------------------------------------------------------------------------
// Dtor: Closes the database connection.
//--------------------------------------------------------------------------------
SQLite3_Serializer::~SQLite3_Serializer() {
    if (m_db) {
        sqlite3_close(m_db);
    }
    if (m_error_msg) {
        sqlite3_free(m_error_msg);
    }
}

//--------------------------------------------------------------------------------
// Add an Item, Tags and ItemTags to the database.
//--------------------------------------------------------------------------------
void SQLite3_Serializer::write(const Item& record) 
    throw(runtime_error) {

    // First insert the Item and hold onto its ID for the ItemTag insert
    m_query.str("");
    m_query << "INSERT INTO Item(Title, Content) VALUES('" << record.title << "', '" 
                                                           << record.content << "')";
    exec();
    sqlite3_int64 item_id = sqlite3_last_insert_rowid(m_db);

    // Then, for each tag, get its ID if it already exists, else add it and hold 
    // on to the new ID. After that, insert an ItemTag with the corresponding
    // ItemID and TagID.
    for (size_t i = 0; i < record.tags.size(); ++i) {
        const string& current_tag = record.tags[i];

        m_query.str("");
        m_query << "SELECT TagId from Tag where Title = '" << current_tag  << "';";

        prepare();

        step();

        // TODO: downcase all tag entries
        int tag_id = sqlite3_column_int(m_statement, 0);
        if (tag_id == 0) {
            m_query.str("");
            m_query << "INSERT INTO Tag(Title) VALUES('" << current_tag << "');";
            exec();
            tag_id = sqlite3_last_insert_rowid(m_db);
        }
        m_query.str("");
        m_query << "INSERT INTO ItemTag(ItemID, TagID) VALUES(" << item_id << "," 
                                                                << tag_id << ");";
        exec();
    }
}

//--------------------------------------------------------------------------------
// Read all items associated with the given tags into th output parameter.
//--------------------------------------------------------------------------------
void SQLite3_Serializer::read(const vector<string>& tags, 
                              vector<Item*>& out_items)
    throw(runtime_error) {

    // TODO: Add option for matching all or one of the tags
    // Select all items matching the tags
    m_query.str("");
    m_query << "SELECT Title, Content FROM Item "
               "JOIN ItemTag "
                    "ON Item.ItemID = ItemTag.ItemID "
               "WHERE ItemTag.TagID = "
                    "(SELECT TagID FROM Tag WHERE Tag.Title = ";

    for (size_t i = 0; i < tags.size(); ++i) {
        m_query << "'" << tags[i] << "'";
        m_query << (i + 1 == tags.size() ? ");" : "OR Tag.Title = ");
    }

    prepare();
    while (step() == SQLITE_ROW) {
        out_items.push_back(new Item);
        Item& item = *out_items.back();

        item.title = 
            reinterpret_cast<const char*>(sqlite3_column_text(m_statement, 0));

        item.content = 
            reinterpret_cast<const char*>(sqlite3_column_text(m_statement, 1));
    }

    // Select the tags for each item
    // TODO: Evaluate inefficiency of this and redo if needed
    for (size_t i = 0; i < out_items.size(); ++i) {
        m_query.str("");
        m_query << "SELECT Title from Tag "
                   "JOIN ItemTag ON Tag.TagID = ItemTag.TagID "
                   "WHERE ItemTag.ItemID = "
                   "(SELECT ItemID from Item WHERE Item.Title = '"
                << out_items[i]->title << "');";

        prepare();
        while (step() == SQLITE_ROW) {
            out_items[i]->tags.push_back(
                reinterpret_cast<const char*>(sqlite3_column_text(m_statement, 0))
            );
        }
    }
}
