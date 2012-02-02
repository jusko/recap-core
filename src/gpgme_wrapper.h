#ifndef GPGME_WRAPPER_H
#define GPGME_WRAPPER_H
//-----------------------------------------------------------------------------
#include <stdexcept>
#include <gpgme.h>
#include <vector>
#include <string>
#include <map>
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Provides interface for the basic cryptographic needs required to store
// encrypted strings in the database.
//-----------------------------------------------------------------------------
class GPGME_Wrapper {

    public:

        //---------------------------------------------------------------------
        // @post   If GPG is present, keys on a system are loaded and can be 
        //         retreived by invoking the all_keys() function.
        // @throw  If GPGME could not be initialised or if errors occurred 
        //         loading the keys.
        // TODO: Throw if engine version is incompatible
        //---------------------------------------------------------------------
        GPGME_Wrapper()
            throw (std::runtime_error);

        //---------------------------------------------------------------------
        // @return The set of all GPG key ids present within the current user's 
        //         environment (empty if no keys exist).
        //---------------------------------------------------------------------
        std::vector<std::string> all_keys() const;

        //---------------------------------------------------------------------
        // Performs an OpenPGP encryption operation
        // @param  plaintext
        //         The text to encrypt 
        // @param  key
        //         The id of the key to be used to perform encryption
        // @return The encrypted cipher
        // @throw  If errors occurred in the encryption process.
        //---------------------------------------------------------------------
        std::string encrypt(const std::string& plaintext,
                            const std::string& key)
            throw (std::runtime_error);

        //---------------------------------------------------------------------
        // Performs an OpenPGP decryption operation
        // @param  cipher
        //         The encrypted ciphertext.
        // @param  key
        //         The id of the key to be used to perform the decryption
        // @return The decrypted plaintext.
        // @throw  If errors occurred in the decryption process.
        //---------------------------------------------------------------------
        std::string decrypt(const std::string& cipher)
            throw (std::runtime_error);

        ~GPGME_Wrapper();

    private:
        void throw_if_error(const char*)         throw(std::runtime_error);
        std::string gpgme_data2str(gpgme_data_t) throw(std::runtime_error);

        gpgme_ctx_t                         m_context;
        gpgme_error_t                       m_error;
        std::map<std::string, gpgme_key_t>  m_keys;
};
#endif
