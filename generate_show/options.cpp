/*
 * ac_options.cpp - Commandline options processing.
 *
 * History:
 *
 *   Create on 2015-5-9 by YU Heng-yang.
 */
#include <algorithm>
#include <iostream>
#include <cstring>
#include "ac_defs.h"
#include "options.h"


using std::string;
using std::vector;
using std::cerr;
using std::cout;



bool Options::match_filter(const string& name) const
{
  for (vector<string>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
    if (name.end() != std::search(name.begin(), name.end(), i->begin(), i->end()))
      return true;
  return false;
}



void Options::parse(int argc, const char *argv[])
{
  if (argc > 1) {
    while (--argc > 0 && (*++argv)[0] == '-') {
      const char *p = strchr(*argv, '=');

      if (p++) { /* p points to exactly after '=', i.e., the start of the value part of an option */
        u32 num = (u32)atoi(p);

        if (strstr(*argv, "numTypesToPrint")) {
          _npossible_leaks_to_print = num;
          cout << "the first " << num << " types will be print.\n";

        } else if (strstr(*argv, "methodfile")) {
          _method_file_name = p;
          cout << "method info file is '" << _method_file_name << "'\n";

        } else if (strstr(*argv, "filter")) {
          char *start, *end;

          start = end = (char*)p;
          while (*start != '\0') {
            while (*end != '\0' && *end != ',')
              end++;
            if (*end == ',')
              *end++ = '\0';
            _filters.push_back(start);
            start = end;
          }

          cout << "filters are: ";
          for (size_t i = 0; i != _filters.size(); i++)
            cout << _filters[i] << ", ";
          cout << '\n';

        } else if (strstr(*argv, "printLeakSize")) {
          _print_leak_size = !strcmp(p, "true");

        } else if (strstr(*argv, "numLeaksToPrint")) {
          _ntop_to_print = num;

        } else if (strstr(*argv, "classification")) {
          if (!strcmp(p, "type"))
            _classification = TYPE;
          else if (!strcmp(p, "site"))
            _classification = SITE;
          else
            _classification = TYPE_AND_SITE;

        } else if (strstr(*argv, "numTopSite")) {
          _num_top_site = num;
        } else if (strstr(*argv, "numTopType")) {
          _num_top_type = num;
        } else
          cerr << "**Unrecognizied option: " << *argv << '\n';
      }
    }
  }

  if (_print_leak_size)
    cout << "The first " << _ntop_to_print << " potential leak types and/or sites will be printed during analyzing.\n";
}




Options::Options()
  :_method_file_name(""),
   _data_file_name(""),
   _ntop_to_print(1),
   _npossible_leaks_to_print(100000u), /* print all types or sites */
   _print_leak_size(false),
   _classification(TYPE_AND_SITE),
   _num_top_site(5),
   _num_top_type(5) {
  // this corresponds to allocation sites that we don't have its name, so filter them out when printing result
  _filters.push_back("N/A");
}
