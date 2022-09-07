#include "document.h"

using namespace std::literals::string_literals;

Document::Document()
    : id(0), relevance(0.0), rating(0)
{
}

Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating)
{
}
std::ostream &operator<<(std::ostream &out, const Document &document)
{
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

void PrintDocument(const Document &document)
{
    std::cout << "{ "s
             //"почему бы здесь не использовать оператор вывода  (меньше дублирования) ?" Не могли бы Вы, пожалуйста,
             // немножко подробнее пояснить? Я не очень понял, что я должен сделать. Хотелось бы все Ваши замечания учесть и выполнить.
              << "document_id = "s << document.id << ", "s
              << "relevance = "s << document.relevance << ", "s
              << "rating = "s << document.rating << " }"s << std::endl;
}
