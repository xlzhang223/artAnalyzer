/*-*- mode:c++ -*-
 *
 * manager.h - Alloc site and type managers.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#ifndef MANAGER_H_
#define MANAGER_H_
#include <map>
#include <iostream>
#include "ac_defs.h"
#include "gen_html.h"

extern const std::string& FindMethodNameBySite(u32 site);
extern u32 GetNumNewObject();

class Base;
class AllocSite;
class Class;
class Options;


class Manager
{
public:
  Manager(Gen* gen_, const Options *options): gen_html(gen_),user_options_(options) { }
  virtual ~Manager();

  void SetOptions(const Options* options) { user_options_ = options; }
  virtual void CollectionFinished() = 0; // do anything after a GC finished
  virtual void CollectionStart() = 0; // do anything before a GC starts
  virtual void Summary() const = 0;   // summary statistics

protected:
  void PrintHeader(const char *header) const;
  const char *GetStaleObjectDescription(double stale_value) const;

  Gen* gen_html;
  typedef std::map<u32, Base*>::iterator BaseIterator;
  std::map<u32, Base*> table_;
  const Options* user_options_;
};


class SiteManager final: public Manager {
public:
  SiteManager(Gen* gen_, const Options *options): Manager(gen_,options) {}
  ~SiteManager() {}

  AllocSite* LookUp(u32 site);
  void CollectionFinished() override;
  void CollectionStart() override;
  void Summary() const override;
  u32 GetNumSites() const { return table_.size(); }

private:
  void DoSummary(std::vector<Base*>& vsites, const char *header, bool do_filer) const;
  // typedef std::map<u32, AllocSite*>::iterator AllocSiteIterator;
  // std::map<u32, AllocSite*> alloc_site_table_;
};


class ClassManager final: public Manager {
public:
  ClassManager(Gen* gen_, const Options *options): Manager(gen_,options) {}
  ~ClassManager() {}

  Class* InsertClass(u32 vtable, size_t obj_size, const std::string& name);
  Class* LookUp(u32 vtable);
  void CollectionFinished() override;
  void CollectionStart() override;
  u32 GetNumClass() const { return table_.size(); }
  void Summary() const;


private:
  void DoSummary(std::vector<Base*>& vclasses, const char *header, bool do_filer) const;

  // typedef std::map<u32, Class*>::iterator ClassIterator;
  // std::map<u32, Class*> class_table_;
};


#endif  /* MANAGER_H_ */
