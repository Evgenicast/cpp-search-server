#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "document.h" // если исключить заголовочный файл, ругается на Document status.
                      //Переносить пробовал, но так не работает. По идее, в проект бы какой-нубдуь PrintDoc class/.h/cpp

std::string ReadLine();
int ReadLineWithNumber();
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);
