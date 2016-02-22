/* -*- mode:c++ -*-
 *
 * options.h - Command line options.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>
#include <vector>

struct Options {
  std::string _method_file_name; /* method file contains vtable, obj size, method file name, etc. */
  std::string _data_file_name; /* data file contains raw data dumped out by tracker */
  std::vector<std::string> _filters; /* filter out certain class or methods */
  int _ntop_to_print; /* first top types (or sites) to print during analyzing, only effective if _print_leak_size is true */
  int _npossible_leaks_to_print;
  bool _print_leak_size;
  enum {
    TYPE = 1, SITE = 2, TYPE_AND_SITE = 3,
  } _classification;
  int _num_top_site;
  int _num_top_type;


  Options();

  /* returns true if name matches any filter, false otherwise */
  bool match_filter(const std::string& name) const;
  bool has_filter() const { return _filters.size() > 1U; } /* the "N/A" filter always exists and is the first one */

  /* argc and argv should be the original arguments passed to main() */
  void parse(int argc, const char *argv[]);

  const std::string& get_methodfile_name() const { return _method_file_name; }
  int get_num_topleaks_to_print() const { return _ntop_to_print; }
  int get_classification()        const { return _classification; }
  int get_num_possible_leaks_to_print() const { return _npossible_leaks_to_print; }
  int get_num_top_site() const { return _num_top_site; }
  int get_num_top_type() const { return _num_top_type; }
  bool is_print_leak_size()       const { return _print_leak_size; }
  bool classify_objects_by_site() const { return (_classification & SITE) != 0; }
  bool classify_objects_by_type() const { return (_classification & TYPE) != 0; }
};

#endif  /* OPTIONS_H_ */