#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "document.h"

std::string ReadLine();
int ReadLineWithNumber();
void PrintDocument(const Document& document);
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);
std::ostream& operator<<(std::ostream& out, const Document& document);
