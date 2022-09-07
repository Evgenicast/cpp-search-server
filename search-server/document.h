#pragma once
#include <iostream>

enum class DocumentStatus {
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

void PrintDocument(const Document& document);
std::ostream& operator<<(std::ostream& out, const Document& document);
