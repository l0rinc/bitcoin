// Copyright (c) 2026 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "field-observers.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace clang::ast_matchers;

namespace bitcoin {

FieldObservers::FieldObservers(clang::StringRef check_name, clang::tidy::ClangTidyContext* context)
    : ClangTidyCheck(check_name, context),
      m_qualified_field_name{Options.get("QualifiedFieldName", "")},
      m_replacement_main_file{Options.get("ReplacementMainFile", "")},
      m_accessor_declaration{Options.get("AccessorDeclaration", "")}
{
    if (m_qualified_field_name.empty() && m_accessor_declaration.empty() && m_replacement_main_file.empty()) {
        return;
    }

    const auto [class_name, field_name]{StringRef{m_qualified_field_name}.rsplit("::")};
    if (class_name.empty() || field_name.empty()) {
        configurationDiag("missing or invalid QualifiedFieldName", DiagnosticIDs::Error);
        return;
    }
    m_storage_name = "m_" + field_name.str();

    // Expect `return-type observer_name(...)` so the observer call can be derived.
    const auto [before_paren, after_paren]{StringRef{m_accessor_declaration}.split('(')};
    const auto [return_type, accessor_name]{before_paren.rtrim().rsplit(' ')};
    if (after_paren.empty() || return_type.empty() || accessor_name.empty()) {
        configurationDiag("missing or invalid AccessorDeclaration", DiagnosticIDs::Error);
        return;
    }
    m_accessor_call = accessor_name.str() + "()";
}

void FieldObservers::registerMatchers(MatchFinder* Finder)
{
    if (m_accessor_call.empty()) return;

    const auto field{fieldDecl(hasName(m_qualified_field_name))};
    const auto not_in_generated_function{unless(hasAncestor(functionDecl(anyOf(isImplicit(), isDefaulted()))))};

    Finder->addMatcher(memberExpr(member(field), not_in_generated_function).bind("member"), this);
    Finder->addMatcher(field.bind("field"), this);
    Finder->addMatcher(cxxCtorInitializer(forField(field)).bind("init"), this);
}

void FieldObservers::check(const MatchFinder::MatchResult& Result)
{
    const bool replacement_allowed{m_replacement_main_file.empty() || getCurrentMainFile().ends_with(m_replacement_main_file)};

    if (const auto* field{Result.Nodes.getNodeAs<FieldDecl>("field")}; field && replacement_allowed) {
        auto diagnostic{diag(field->getLocation(), "rename field")};
        diagnostic << FixItHint::CreateReplacement(field->getLocation(), m_storage_name);
        const SourceLocation insert_loc{Lexer::findLocationAfterToken(
            field->getEndLoc(), tok::semi, *Result.SourceManager, getLangOpts(), /*SkipTrailingWhitespaceAndNewLine=*/false)};
        if (insert_loc.isValid()) {
            diagnostic << FixItHint::CreateInsertion(insert_loc, "\n    " + m_accessor_declaration);
        }
    } else if (const auto* init{Result.Nodes.getNodeAs<CXXCtorInitializer>("init")}; init && replacement_allowed) {
        diag(init->getMemberLocation(), "rename field initializer")
            << FixItHint::CreateReplacement(init->getMemberLocation(), m_storage_name);
    } else if (const auto* member{Result.Nodes.getNodeAs<MemberExpr>("member")}; member && (!member->isImplicitAccess() || replacement_allowed)) {
        diag(member->getMemberLoc(), member->isImplicitAccess() ? "rename field access" : "replace direct member access with accessor")
            << FixItHint::CreateReplacement(member->getMemberLoc(), member->isImplicitAccess() ? m_storage_name : m_accessor_call);
    }
}

} // namespace bitcoin
