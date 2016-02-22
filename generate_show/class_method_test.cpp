#include <iostream>
#include "class_method.h"

int main()
{
  const char *datafile = "linpack.classmethod";

  ClassTable class_table;
  MethodTable method_table;

  if (!ReadClassAndMethod(datafile,  method_table, class_table))
    std::cerr << "failed to read file.\n";
  else
    std::cout << "read file successfully.\n";
  return 0;
}