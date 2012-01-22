#include "sqlite3_serializer.h"
#include <sqlite3.h>
#include <string>
#include <cstring>
using namespace std;

//--------------------------------------------------------------------------------
// Table creation statements
//--------------------------------------------------------------------------------
#define ITEM_DDL     "CREATE TABLE IF NOT EXISTS Item("\
                        "ItemID INTEGER PRIMARY KEY, Title TEXT, Content TEXT);"

#define TAG_DDL      "CREATE TABLE IF NOT EXISTS Tag("\
                        "TagID INTEGER PRIMARY KEY, Title TEXT UNIQUE);"

#define ITEM_TAG_DDL "CREATE TABLE IF NOT EXISTS ItemTag("\
                        "ID INTEGER PRIMARY KEY, ItemID INTEGER, TagID INTEGER, "\
                        "FOREIGN KEY(ItemID) REFERENCES Item(ItemID), "\
                        "FOREIGN KEY(TagID) REFERENCES Tag(TagID));"

#define FKEYS_ON     "PRAGMA foreign_keys = ON;"

//--------------------------------------------------------------------------------
// Convenience function for preparing SQL statements. Binds the values given
// as varargs to the SQL statement as SQL parameters. 
// @pre  m_sql is set with a valid SQL query and all varargs are of type const char*
// @post m_statement is prepared with a binary SQL statement
// @param count The number of varargs received
// TODO: Update to use variadic template args (to unpack vector params in read()).
//--------------------------------------------------------------------------------
inline void SQLite3_Serializer::prepare(int count, ...) 
    throw(runtime_error) {

    if (sqlite3_prepare_v2(
        m_db,
        m_query.str().c_str(),
        m_query.str().size(),
        &m_statement,
        NULL) != SQLITE_OK) {

        throw runtime_error(sqlite3_errmsg(m_db));
    }
    va_list vargs;
    va_start(vargs, count);
    for (int i = 0; i < count; ++i) {
        const char* sql_param = va_arg(vargs, const char*);
        sqlite3_bind_text(m_statement, 
            i + 1, 
            sql_param,
            strlen(sql_param),
            SQLITE_STATIC
        );
    }
    va_end(vargs);
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
                           m_query("") {

    if ((sqlite3_open(db_spec, &m_db) != SQLITE_OK)) {
        throw runtime_error(string(sqlite3_errmsg(m_db)));
    }
    m_query.str(ITEM_DDL);
    prepare(0);
    step();

    m_query.str(TAG_DDL);
    prepare(0);
    step();

    m_query.str(ITEM_TAG_DDL);
    prepare(0);
    step();

    m_query.str(FKEYS_ON);
    prepare(0);
    step();
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
    m_query.str("INSERT INTO Item(Title, Content) VALUES(?, ?);");
    prepare(2, record.title.c_str(), record.content.c_str());
    step();
    sqlite3_int64 item_id = sqlite3_last_insert_rowid(m_db);

    // Then, for each tag, get its ID if it already exists, else add it and hold 
    // on to the new ID. After that, insert an ItemTag with the corresponding
    // ItemID and TagID.
    m_query.str("SELECT TagId from Tag where Title = ?;");
    for (size_t i = 0; i < record.tags.size(); ++i) {
        const string& current_tag = record.tags[i];
        prepare(1, current_tag.c_str());
        step();

        // TODO: downcase all tag entries
        int tag_id = sqlite3_column_int(m_statement, 0);
        m_query.str( "INSERT INTO Tag(Title) VALUES(?);");
        if (tag_id == 0) {
            prepare(1, current_tag.c_str());
            step();
            tag_id = sqlite3_last_insert_rowid(m_db);
        }
        m_query.str("");
        m_query << "INSERT INTO ItemTag(ItemID, TagID) VALUES(" << item_id << "," 
                                                                << tag_id << ");";
        prepare(0);
        step();
    }
}

//--------------------------------------------------------------------------------
// Read all items associated with the given tags into the output parameter.
//--------------------------------------------------------------------------------
void SQLite3_Serializer::read(const vector<string>& tags, 
                              vector<Item*>& out_items)
    throw(runtime_error) {

    if (tags.empty()) {
        return;
    }
    // TODO: Add option for matching all or one of the tags
    // Select all items matching the tags
    m_query.str("");
    m_query << "SELECT DISTINCT Title, Content FROM Item "
               "JOIN ItemTag "
                    "ON Item.ItemID = ItemTag.ItemID "
               "WHERE ItemTag.TagID IN "
                    "(SELECT TagID FROM Tag WHERE Tag.Title = ";

    for (size_t i = 0; i < tags.size(); ++i) {
        m_query << "?";
        m_query << (i + 1 == tags.size() ? ");" : " OR Tag.Title = ");
    }
    //------
    // HACK: This isn't necessary with C++11x variadic template args
    //       so refine it during the sweep.
    prepare(0);
    for (size_t i = 0; i < tags.size(); ++i) {
        sqlite3_bind_text(
            m_statement, 
            i + 1, 
            tags[i].c_str(),
            tags[i].size(), 
            SQLITE_STATIC
        );
    }
    //------

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
    m_query.str("");
    m_query << "SELECT Title from Tag "
               "JOIN ItemTag ON Tag.TagID = ItemTag.TagID "
               "WHERE ItemTag.ItemID = "
               "(SELECT ItemID from Item WHERE Item.Title = ?);";

    for (size_t i = 0; i < out_items.size(); ++i) {
        prepare(1, out_items[i]->title.c_str());
        while (step() == SQLITE_ROW) {
            out_items[i]->tags.push_back(
                reinterpret_cast<const char*>(sqlite3_column_text(m_statement, 0))
            );
        }
    }
}

//--------------------------------------------------------------------------------
// Read all tags in the Tag table into the output parameter.
//--------------------------------------------------------------------------------
void SQLite3_Serializer::tags(vector<string>& out_tags) 
    throw(runtime_error) {

    m_query.str("SELECT Title from Tag;");
    prepare(0);
    while (step() == SQLITE_ROW) {
        out_tags.push_back(
            reinterpret_cast<const char*>(sqlite3_column_text(m_statement, 0))
        );

    }
}
