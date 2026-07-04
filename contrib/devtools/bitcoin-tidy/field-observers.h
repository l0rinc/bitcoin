// Copyright (c) 2026 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TIDY_FIELD_OBSERVERS_H
#define BITCOIN_TIDY_FIELD_OBSERVERS_H

#include <clang-tidy/ClangTidyCheck.h>

#include <string>

namespace bitcoin {

class FieldObservers final : public clang::tidy::ClangTidyCheck
{
public:
    FieldObservers(clang::StringRef check_name, clang::tidy::ClangTidyContext* context);

    void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;

private:
    std::string m_qualified_field_name;
    std::string m_storage_name;
    std::string m_replacement_main_file;
    std::string m_accessor_declaration;
    std::string m_accessor_call;
};

} // namespace bitcoin

#endif // BITCOIN_TIDY_FIELD_OBSERVERS_H
