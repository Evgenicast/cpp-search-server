#pragma once

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
