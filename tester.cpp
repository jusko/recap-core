#include "sqlite3_serializer.h"
#include <string.h>
#include <string>
#include <iostream>
#include <vector>
using namespace std;

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void usage(char** argv);
void cleanup(vector<Item*>& items);
void parse_tags(const char* in_tags, vector<string>& out_list);

//------------------------------------------------------------------------------
// Main function
//------------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 4 || (strcmp(argv[2], "-c") && strcmp(argv[2], "-r"))) {
        usage(argv);
        return 1;
    }

    vector<Item*> items;
    vector<string> tags;
    try {
        Serializer* sr = new SQLite3_Serializer(argv[1]);

        if (strcmp(argv[2], "-c") == 0) {
            if (argc != 6) {
                usage(argv);
                return 1;
            }
            parse_tags(argv[5], tags);

            Item record = {
                argv[3],
                argv[4],
                tags
            };
            sr->write(record);
        }
        else if (strcmp(argv[2], "-r") == 0) {
            if (argc != 4) {
                usage(argv);
                return 1;
            }
            parse_tags(argv[3], tags);
            sr->read(tags, items);

            if (items.empty()) {
                cout << "No results found" << endl;
            }
            else {
                cout << "|Title\t|Content\t|Tags\t|" << endl;
                for (size_t i = 0; i < items.size(); ++i) {
                    cout << "|" << items[i]->title << "\t|" 
                         << items[i]->content << "\t|";

                    for (size_t j = 0; j < items[i]->tags.size(); ++j) {
                        cout << items[i]->tags[j];
                        cout << (j + 1 == items[i]->tags.size() ? "|" : ", ");
                    }
                    cout << endl;
                }
            }
        }
    }
    catch(const exception& e) {
        cout << e.what() << endl;
        cleanup(items);
        return 1;
    }
    cleanup(items);
    return 0;
}

//------------------------------------------------------------------------------
// Cleanup the dynamically allocated tag array
//------------------------------------------------------------------------------
void cleanup(vector<Item*>& items) {
    for (size_t i = 0; i < items.size(); ++i) {
        delete items[i];
    }
}

//------------------------------------------------------------------------------
// Display the usage string
//------------------------------------------------------------------------------
void usage(char** argv) {
    cout << "Usage: " << argv[0] 
         << " DATABASE [-c 'TITLE' 'CONTENT' 'TAG1, TAG2, ...'"
            " | -r 'TAG1, TAG2, ...']"
         << endl;
}

//------------------------------------------------------------------------------
// Parse the comma separated tagstring into a vector of strings
//------------------------------------------------------------------------------
void parse_tags(const char* tagstr, vector<string>& out_tags) {
    if (tagstr == NULL) {
        throw runtime_error("No tags provided");
    }
    string tags(tagstr);

    for (size_t pos = 0; pos != string::npos; pos = tags.find(',', pos)) {

        if (tags[pos] == ',') ++pos;

        string curr_tag(tags.substr(pos, tags.find(',', pos) - pos));
        // Trim
        while (curr_tag[0] == ' ') {
            curr_tag.erase(0, 1);
        }
        while (curr_tag[curr_tag.size() - 1] == ' ') {
            curr_tag.erase(curr_tag.size() - 1 ,1);
        }
        out_tags.push_back(curr_tag);
    }
}