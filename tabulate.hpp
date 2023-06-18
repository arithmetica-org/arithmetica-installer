#pragma once

#include <string>
#include <vector>

namespace table {
std::string tabulate(std::vector<std::vector<std::string>> table) {
  if (table.empty())
    return "";

  // regulate table to fix empty spaces
  size_t table_max_len = 0;
  for (auto &i : table) {
    if (i.size() > table_max_len) {
      table_max_len = i.size();
    }
  }
  for (auto &i : table) {
    if (i.size() < table_max_len) {
      for (auto j = 0; j < table_max_len - i.size(); j++)
        i.push_back("-");
    }
  }

  // get the max lengths of each row
  std::vector<size_t> max_len;
  for (size_t i = 0; i < table[0].size(); i++) {
    max_len.push_back(0);
    for (size_t j = 0; j < table.size(); j++) {
      if (table[j][i].length() > max_len[i])
        max_len[i] = table[j][i].length();
    }
  }

  // start making table
  std::string bar = "+";
  for (auto &i : max_len)
    bar += std::string(i + 2, '-') + "+";
  std::string answer = bar;
  for (size_t i = 0; i < table.size(); i++) {
    answer += "\n|";
    for (size_t j = 0; j < table[i].size(); j++) {
      answer += " " + table[i][j];
      if (max_len[j] - table[i][j].length() > 0) {
        answer += std::string(max_len[j] - table[i][j].length(), ' ');
      }
      answer += " |";
    }
    if (i == 0)
      answer += "\n" + bar;
  }
  answer += "\n" + bar;

  return answer;
}
} // namespace table