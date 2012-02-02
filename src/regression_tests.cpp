#include "gpgme_wrapper.h"
#include <stdexcept>
#include <iostream>
using namespace std;

string list_keys(const GPGME_Wrapper&);

string encrypt_and_display(const string&, 
                           const string&, 
                           GPGME_Wrapper&);

void decrypt_and_display(const string& cipher,
                         GPGME_Wrapper& gw);

int main() {
    try {
        GPGME_Wrapper gw;
        string key = list_keys(gw);
        string cipher = encrypt_and_display(
            "L'enfer, c'est les autres", key, gw
        );
        decrypt_and_display(cipher, gw);
    }
    catch(const exception& e) {
        cout << e.what() << endl;
    }
}

string list_keys(const GPGME_Wrapper& gw) {
    vector<string> keys = gw.all_keys();

    for (size_t i = 0; i < keys.size(); ++i) {
        cout << i + 1 << ": " << keys[i] << endl;
    }
    if (keys.empty()) {
        throw runtime_error("No GPG keys. Aborting tests.");
    }
    return keys[0];
}

string encrypt_and_display(const string& text,
                           const string& key, 
                           GPGME_Wrapper& gw) {

    string cipher = gw.encrypt(text, key);
    cout << "Encrypted cipher:\n" << endl << cipher << endl;
    return cipher;
}

void decrypt_and_display(const string& cipher,
                         GPGME_Wrapper& gw) {

    string text = gw.decrypt(cipher);
    cout << "Decrypted text:\t" << text << endl;
}
