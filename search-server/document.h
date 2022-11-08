#pragma once
#include <string>
#include <vector>
#include <iostream>

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class Document
{
public:
    int id;
    double relevance;
    int rating;

    Document();
    Document(int id, double relevance, int rating);
};
std::ostream& operator<<(std::ostream& out, const Document& document);
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);
void PrintDocument(const Document& document);
