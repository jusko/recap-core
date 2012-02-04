//-----------------------------------------------------------------------------
#include "gpgme_wrapper.h"
//-----------------------------------------------------------------------------
#include <errno.h>
//-----------------------------------------------------------------------------
using namespace std;

//-----------------------------------------------------------------------------
// Helper function to check GPGME error codes
// @pre   m_error is set with the most recent error code return value
// @param msg
//        Description of the context in which a possible error may occur
// @post  If an error occurs, a descriptive exception is thrown. Otherwise 
//        no state change occurs.
// TODO:  Modify to throw different exceptions when GPG is not installed
//-----------------------------------------------------------------------------
inline void GPGME_Wrapper::throw_if_error(const char* msg) 
    throw(runtime_error) {

    if (m_error != GPG_ERR_NO_ERROR) {
        string error_msg = 
            string(msg).append(": ").append(gpgme_strerror(m_error));

        throw runtime_error(error_msg);
    }
}

//-----------------------------------------------------------------------------
// Transfers the data buffer into a standard string.
// @pre   buffer is initialised with valid data from a GPGME operation.
// @throw if an error occurs in the conversion
//-----------------------------------------------------------------------------
inline string GPGME_Wrapper::gpgme_data2str(const gpgme_data_t buffer)
    throw (runtime_error) { 

    const size_t BUF_SIZE = 512;
    char read_buf[BUF_SIZE + 1];

    off_t pos = gpgme_data_seek(buffer, 0, SEEK_SET);
    if (pos != 0) {
        m_error = gpgme_err_code_from_errno(errno);
        throw_if_error("Failed to seek start of data buffer");
    }

    string rval;
    ssize_t bytes = 0;
    while ((bytes = gpgme_data_read(buffer, read_buf, BUF_SIZE)) > 0) {
        rval.append(read_buf, bytes);
    }
    if (bytes < 0) {
        m_error = gpgme_err_code_from_errno(errno);
        throw_if_error("Failed to convert data buffer");
    }
    return rval;
}

//-----------------------------------------------------------------------------
// Creates a human readible string identifying a GPG key
//-----------------------------------------------------------------------------
inline string key2str(const gpgme_key_t key) {
    return string(key->subkeys->keyid).append("\t")
                                      .append(key->uids->name)
                                      .append(" (")
                                      .append(key->uids->email)
                                      .append(")");
}

//-----------------------------------------------------------------------------
// Ctor
//
// Sets the locale, initialises a context and loads any GPG keys present into
// a cache.
//-----------------------------------------------------------------------------
GPGME_Wrapper::GPGME_Wrapper()
    throw(runtime_error)

    : m_context(0) {

    // TODO: check PGP engine, check GPG Agent 
    // Set locale
    setlocale (LC_ALL, "");
    gpgme_check_version (NULL);
    gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));

#ifdef LC_MESSAGES
    gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
#endif

    // Initialise the context
    m_error = gpgme_new(&m_context);
    throw_if_error("Failed to create new GPGME context");

    // Cache the keys
    m_error = gpgme_op_keylist_start(m_context, NULL, 0);
    throw_if_error("Failed to create list of GPG keys");

    while (m_error == GPG_ERR_NO_ERROR) {
        gpgme_key_t key;
        m_error = gpgme_op_keylist_next(m_context, &key);

        if (m_error != GPG_ERR_NO_ERROR) {
            break;
        }
        if (!key->revoked && !key->expired && key->can_encrypt) {
            m_keys.insert(
                 pair<string, gpgme_key_t>(key2str(key), key)
            );
        }
    }
    // All binary is text encoded for DB persistence
    gpgme_set_armor(m_context, 1);

}

//-----------------------------------------------------------------------------
// Dtor
//-----------------------------------------------------------------------------
GPGME_Wrapper::~GPGME_Wrapper() {
    gpgme_release(m_context);

    map<string, gpgme_key_t>::iterator it = m_keys.begin(), 
                                      end = m_keys.end();
    while (it != end) {
        gpgme_key_release(it->second);
        ++it;
    }
}

//-----------------------------------------------------------------------------
// Returns a set human readable strings identifying the GPG keys. This set
// is in fact the set of keys in the internal map to GPGME key data types.
//-----------------------------------------------------------------------------
vector<string> GPGME_Wrapper::all_keys() const {
    vector<string> rval;

    map<string, gpgme_key_t>::const_iterator it = m_keys.begin(), 
                                            end = m_keys.end();
    while (it != end) {
        rval.push_back(it->first);
        ++it;
    }
    return rval;
}

//-----------------------------------------------------------------------------
// Prepare GPGME data types and encrypt the plain text with the given key
// TODO: Refactor long method
//-----------------------------------------------------------------------------
string GPGME_Wrapper::encrypt(const string& plaintext, const string& key) 
    throw (runtime_error) {

    gpgme_data_t input = 0, output = 0;
    try {
        if (m_keys.find(key) == m_keys.end()) {
            throw runtime_error("Invalid key: " + key);
        }
        gpgme_key_t inkey[] = { m_keys[key], NULL };
        
        m_error = gpgme_data_new_from_mem(
            &input,
            plaintext.c_str(),
            plaintext.size(), 
            1
        );
        throw_if_error("Failed to prepare plain text for encryption");

        m_error = gpgme_data_new(&output);
        throw_if_error("Failed to prepare cipher buffer");

        m_error = gpgme_op_encrypt(
            m_context, 
            inkey, 
            GPGME_ENCRYPT_ALWAYS_TRUST,
            input,
            output
        );
        throw_if_error("Failed to encrypt the plain text");

        gpgme_encrypt_result_t result = gpgme_op_encrypt_result(m_context);
        if (result->invalid_recipients) {
            throw runtime_error("Encryption failed (invalid recipient "
                                "for the given key).");
        }
        string cipher = gpgme_data2str(output);
        gpgme_data_release(input);
        gpgme_data_release(output);
        return cipher;
    }
    catch (const exception&) {
        if (input)  gpgme_data_release(input);
        if (output) gpgme_data_release(output);
        throw;
    }
}

//-----------------------------------------------------------------------------
string GPGME_Wrapper::decrypt(const string& cipher) 
    throw (runtime_error) {

    gpgme_data_t input = 0, output = 0;
    try {
        m_error = gpgme_data_new_from_mem(
            &input, cipher.c_str(), cipher.size(), 1
        );
        throw_if_error("Failed to prepare cipher buffer");

        m_error = gpgme_data_new(&output);
        throw_if_error("Failed to prepare plain text buffer");

        m_error = gpgme_op_decrypt(m_context, input, output);
        throw_if_error("Failed to decrypt cipher text");

        string plain_text = gpgme_data2str(output);
        gpgme_data_release(input);
        gpgme_data_release(output);
        return plain_text;
    }
    catch (const exception&) {
        if (input)  gpgme_data_release(input);
        if (output) gpgme_data_release(output);
        throw;
    }
}
