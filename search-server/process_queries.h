#ifndef PROCESS_QUERIES_H
#define PROCESS_QUERIES_H
#pragma once
#include <functional>
#include <execution>
#include <vector>
#include <list>

#include "search_server.h"
std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries);
std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);

#endif // PROCESS_QUERIES_H
