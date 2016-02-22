/*
 * class_method.cpp - Implementation of class_method.h
 *
 * History:
 *
 * Created on 2015-11-20 by Hengyang Yu.
 */
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include "manager.h"
#include "class_method.h"
#include "debug.h"
#include "gen_html.h"
#include <cassert>


// static bool MethodComparator(const Method *m1, const Method *m2)
// {
//   if (kDebugBuild) {
//     if (!(m1->code_end_ <= m2->code_begin_ ||
//           m1->code_begin_ >= m2->code_end_)) {
//       std::cerr << "assertion failed:\n";
//       std::cerr << std::hex << "m1: " << m1->code_begin_ << ", " << m1->code_end_ << std::endl;
//       std::cerr << std::hex << "m2: " << m2->code_begin_ << ", " << m2->code_end_ << std::endl;
//       exit(1);
//     }
//   }

//   return m1->code_end_ < m2->code_begin_;
// }

bool ReadClassAndMethod(const char *path, MethodTable& method_table, ClassManager& class_mgr, Gen* gen_html)
{
  uint32_t code_begin, code_end, type;
  size_t object_size;
  std::string name, line;
  std::ifstream input(path, std::ifstream::in);

  if (!input.good())
    return false;

  while (input.good()) {
    std::getline(input, line);
    if (line.size() > 0) {
      const char *s = line.c_str() + 2;

      if (line[0] == 'm') {
        sscanf(s, "%x %x", &code_begin, &code_end);
        name = line.substr(20);   // m aaaaaaaa bbbbbbbb ...
        method_table.push_back(new Method(code_begin, code_end, name));
      } else if (line[0] == 'c') {
        sscanf(s, "%x %zu", &type, &object_size);
        name = line.substr(20);   // c zzzzzzzz yyyyyyyy ...
        class_mgr.InsertClass(type, object_size, name);
      } else if (line[0] == '\n')
        continue;
      else
        std::cout << "invalid line: " << line << std::endl;
    }
  }

  // std::sort(method_table.begin(), method_table.end(), MethodComparator);

  std::cout << "Total " << class_mgr.GetNumClass() << " classes, "
            << method_table.size() << " methods." << std::endl;
  gen_html->pri_2(class_mgr.GetNumClass(),method_table.size());
  return true;
}
