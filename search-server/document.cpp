#include "document.h"

Document::Document()
    : id(0), relevance(0.0), rating(0)
{
}

Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating)
{
}
