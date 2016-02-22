/*
 * manager.cpp - Implementation of AllocSite and Class Manager.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#include <algorithm>
#include <iomanip>
#include "alloc_site.h"
#include "class_method.h"
#include "options.h"
#include "manager.h"
#include "debug.h"

using std::string;
using std::map;
using std::set;
using std::vector;
using std::endl;

inline bool leak_conf_cmp(const Base *x, const Base *y) {
  return x->leak_confidence_ > y->leak_confidence_;
}
inline bool num_obj_cmp(const Base *x, const Base *y) {
  return x->nborn_ > y->nborn_;
}

inline string formatS(string s) {
  string result;
  for (auto c: s) {
    if (c == '<') {
      result = result + "&lt;";
    }  else if (c == '>') {
       result = result + "&gt;";
    } else {
      result.push_back(c);
    }
  }
  return result;
}
/*
 * Copy map values to a vector.
 * http://stackoverflow.com/questions/771453/copy-map-values-to-vector-in-stl
 */
template <typename Map, typename Vec>
void map_to_vector(const Map& m, Vec& v)
{
  for (typename Map::const_iterator i = m.begin(); i != m.end(); ++i)
    v.push_back(i->second);
}


void Manager::PrintHeader(const char *header) const
{
  if (header) {
    std::cout << "\n\t\t=============================================================================" << std::endl;
    std::cout << "\t\t" << header << std::endl;
    std::cout << "\t\t===============================================================================" << std::endl;
  }
}


const char *Manager::GetStaleObjectDescription(double stale_value) const
{
  if (stale_value > 0.9f)
    return "Almost Never Accessed";
  else if (stale_value > 0.8f)
    return "Highly Staled";
  else if (stale_value > 0.7f)
    return "Very Stale";
  else if (stale_value > 0.5f)
    return "Staled";
  else
    return "Not Sure";
}

Manager::~Manager()
{
  for (BaseIterator it = table_.begin(); it != table_.end(); ++it)
    delete it->second;
}




AllocSite* SiteManager::LookUp(u32 site)
{
  BaseIterator it = table_.find(site);

  if (it != table_.end())
    return reinterpret_cast<AllocSite*>(it->second);

  AllocSite *asp = new AllocSite(site, FindMethodNameBySite(site));
  table_[site] = asp;
  return asp;
}


void SiteManager::CollectionStart()
{
  for (auto &elem : table_)
    elem.second->BeforeGc();
}


void SiteManager::CollectionFinished()
{
  for (auto &elem : table_)
    elem.second->AfterGc();

  // if (_p_user_options->is_print_leak_size() && _p_user_options->classify_objects_by_site()) {
  //   vector<AllocSite*> sites;

  //   map_to_vector(alloc_site_table_, sites);
  //   std::sort(sites.begin(), sites.end(), leak_conf_cmp);

  //   int k = _p_user_options->get_num_topleaks_to_print();

  //   // print filtered result first if necessary
  //   if (_p_user_options->has_filter())
  //     for (size_t i = 0; i != sites.size() && k > 0; ++i)
  //       if (sites[i]->_leak_confidence == 0)
  //         break; // no further allocation site can have _leak_confidence > 0
  //       else if (!_p_user_options->match_filter(sites[i]->get_name())) {
  //         --k;
  //         // leading '-' means filtered
  //         printf(" -SITE%s: %u bytes\n", sites[i]->get_name().c_str(), sites[i]->_stale_mem_size);
  //       }


  //   // then print result without filtering
  //   k = _p_user_options->get_num_topleaks_to_print();
  //   for (u32 i = 0; i != sites.size() && k > 0; ++i, --k)
  //     // leading '+' means not filtered
  //     printf(" +SITE%s: %u bytes\n", sites[i]->get_name().c_str(), sites[i]->_stale_mem_size);

  //   putchar('\n');
  // }
}




void SiteManager::DoSummary(std::vector<Base*>& v, const char *header, bool do_filter) const
{
  ofstream& show = gen_html->pri_leak_sites();
  PrintHeader(header);

  // int count = _p_user_options->get_num_possible_leaks_to_print();
  int count = 2;
  puts("num. born   stale mem. (bytes)  *leak conf.*         summary         allocation site");
  puts("==========  ==================  ============  =====================  ===============");

  for (u32 i = 0; i != v.size() && count > 0; ++i) { /* only print the first ntop allocation sites */
    AllocSite *asp = reinterpret_cast<AllocSite*>(v[i]);

    if (asp->leak_confidence_ == 0)
      break; // no further sites can has _leak_confidence > 0

    if (do_filter && user_options_->match_filter(asp->GetName()))
      asp = NULL;

    if (asp) {
      --count;
      printf("%10u  %18u  %12.2lf  %-21s  %s()\n", asp->nborn_, asp->stale_mem_size_,
             asp->leak_confidence_,
             GetStaleObjectDescription(asp->sum_cur_normalized_staleness_/asp->nstale_),
             asp->GetName().c_str());

      show << "<tr>" << endl
	   << "<th scope=\"row\">" << asp->nborn_ << "</th>" << endl
	   << "<td>" << asp->stale_mem_size_ << "</td>" << endl
	   << "<td>" << asp->leak_confidence_ << "</td>" << endl
	   << "<td>" << GetStaleObjectDescription(asp->sum_cur_normalized_staleness_/asp->nstale_) << "</td>" << endl
	   << "<td>" << formatS(asp->GetName()) <<"</td>" << endl 
	   << "</tr>" << endl;

      const char *spaces = "                                                                    ";
      for (auto klass : asp->cross_ref_) {
        Class *k = reinterpret_cast<Class*>(klass);
        printf("%s\t %s (%zu bytes)\n", spaces, k->GetName().c_str(), k->GetInstanceSize());
	show << "<tr>" << endl
	     << "<th scope=\"row\"></th>" << endl
	     << "<td></td>" << endl
	     << "<td></td>" << endl
	     << "<td></td>" << endl
	     << "<td>" << formatS(k->GetName()) <<"(" << k->GetInstanceSize() << "bytes)" <<"</td>" << endl 
	     << "</tr>" << endl;
      }
      putchar('\n');
    }
  }
}


static void PrintTop(vector<Base*> &v, u32 k, Gen* gen_html, ofstream& show,int x)
{
  vector<pair<string,double> > pVec;
  int top = v.size() < k ? v.size() : k;
  puts("  Site   No. obj   Per. %  Name");
  puts("======== ======== ======== =========");
  for (int i = 0; i < top; ++i) {
    double x = double(v[i]->GetNumBorn())/GetNumNewObject()*100;
    printf("%8X %8u %8.2f %50s\n",
           v[i]->GetKey(),
           v[i]->GetNumBorn(),
           x,
           v[i]->GetName().c_str());
    pVec.push_back({formatS(v[i]->GetName()),x});

    show << "<tr>" << endl 
	 << "<th scope=\"row\">"<< uppercase << hex << v[i]->GetKey() << "</th>"<< endl
	 << "<td>" << dec << v[i]->GetNumBorn() << "</td>" << endl 
	 << "<td>" << setiosflags(ios::fixed) << setprecision(2) << x << "</td>" << endl
	 << "<td>" << formatS(v[i]->GetName()) << "</td>" << endl
	 << "<td></td>" << endl << "<td></td>" << endl << "</tr>" << endl;

    vector<Base*> bSet(v[i]->cross_ref_.begin(),v[i]->cross_ref_.end());
    sort(bSet.begin(),bSet.end(),num_obj_cmp);
    
    // print the corresponding Class or AllocSite name
    for (auto bp : bSet) {
      const int kNumSpace = 8+1+8+1+8+1+50+1;
      for (int i = 0; i < kNumSpace; ++i)
        putchar(' ');
      printf("%8d %s\n", bp->GetNumBorn(), bp->GetName().c_str());

      show << "<tr>" << endl 
	   << "<th scope=\"row\"></th>"<< endl 
	   << "<td></td>" << endl << "<td></td>" << endl << "<td></td>" << endl
	   << "<td>" << bp->GetNumBorn() << "</td>" << endl
	   << "<td>" << formatS(bp->GetName()) << "</td>" << endl
	   << "</tr>" << endl;
    }
  }
  if (x == 1) {
    gen_html->sites = pVec;
  } else if ( x == 2) {
    gen_html->classes = pVec;
  }
}

void SiteManager::Summary() const
{
  vector<Base*> v;
  map_to_vector(table_, v);

  ofstream& show = gen_html->pri_top_sites();
  // print top sites that allocate objects
  std::sort(v.begin(), v.end(), num_obj_cmp);
  PrintHeader("Top Sites");
  PrintTop(v, static_cast<unsigned>(user_options_->get_num_top_site()),gen_html,show,1);

  // print top sites with most leak confidence
  std::sort(v.begin(), v.end(), leak_conf_cmp);
  DoSummary(v, "Sorted by leak confidence based on site (not filtered)", false);
  // if (_p_user_options->has_filter())
  //   do_summary(sites, "Sorted by leak confidence based on site (filtered)", true);
}




Class* ClassManager::LookUp(u32 vtable)
{
  BaseIterator it = table_.find(vtable);

  if (it != table_.end())
    return reinterpret_cast<Class*>(it->second);

  return nullptr;
}


Class* ClassManager::InsertClass(u32 vtable, size_t obj_size, const string& name)
{
  BaseIterator it = table_.find(vtable);

  if (it != table_.end())
    return reinterpret_cast<Class*>(it->second);

  Class *klass = new Class(vtable, obj_size, name);
  table_[vtable] = klass;
  return klass;
}


void ClassManager::CollectionStart()
{
  for (auto & elem : table_)
    elem.second->BeforeGc();
}



void ClassManager::CollectionFinished()
{
  for (auto &elem : table_)
    elem.second->AfterGc();

  // /*
  //  * Report leak size of the first k possible leak types, if necessary.
  //  */
  // if (user_options_->is_print_leak_size() && user_options_->classify_objects_by_type()) {
  //   vector<Class*> types;

  //   map_to_vector(table_, types);
  //   std::sort(types.begin(), types.end(), leak_conf_cmp);

  //   int k = user_options_->get_num_topleaks_to_print();

  //   // print filtered result first if necessary
  //   if (user_options_->has_filter())
  //     for (size_t i = 0; i != types.size() && k > 0; ++i)
  //       if (!user_options_->match_filter(types[i]->GetName())) {
  //         --k;
  //         // leading '-' means filtered
  //         printf(" -TYPE%s: %u bytes\n", types[i]->GetName().c_str(), types[i]->_stale_mem_size);
  //       }

  //   // then print result without filtering
  //   k = user_options_->get_num_topleaks_to_print();
  //   for (u32 i = 0; i != types.size() && k > 0; ++i, --k)
  //     // leading '+' means not filtered
  //     printf(" +TYPE%s: %u bytes\n", types[i]->GetName().c_str(), types[i]->_stale_mem_size);

  //   putchar('\n');
  // }
}



void ClassManager::Summary() const
{
  vector<Base*> v;
  map_to_vector(table_, v);
  std::sort(v.begin(), v.end(), num_obj_cmp);
  ofstream& show = gen_html->pri_top_classes();
  PrintHeader("Top Classes");
  PrintTop(v, static_cast<unsigned>(user_options_->get_num_top_type()),gen_html,show,2);

  // vector<Class*> vtypes;
  // map_to_vector(table_, vtypes);
  // std::sort(vtypes.begin(), vtypes.end(), leak_conf_cmp);
  // DoSummary(vtypes, "Sorted by leak confidence based on type (not filtered)", false);
  // if (user_options_->has_filter())
  //   DoSummary(vtypes, "Sorted by leak confidence based on type (filtered)", true);
}


void ClassManager::DoSummary(std::vector<Base*>& v, const char *header, bool do_filter) const
{
  // PrintHeader(header);

  // int count = user_options_->get_num_possible_leaks_to_print();

  // puts("num. born   stale mem. (bytes)  obj. size  *leak conf.*  summary                object type");
  // puts("----------  ------------------  ---------  ------------  ---------------------  -----------");

  // for (u32 i = 0; i != v.size() && count > 0; ++i) { /* only print the first count allocation sites */
  //   Class *klass = v[i];
  //   const string& name = klass->GetName();

  //   if (klass->leak_confidence_ == 0)
  //     break;	// no more types have its leak_confidence_ > 0

  //   if (do_filter && user_options_->match_filter(name))
  //     klass = NULL;

  //   if (klass) {
  //     --count;
  //     printf("%10u  %18u  %9u  %12.2lf  %-21s  %s\n", klass->nborn_, klass->stale_mem_size_,
  //            klass->obj_size_,
  //            klass->leak_confidence_,
  //            GetStaleObjectDescription(klass->sum_cur_normalized_staleness_/klass->nstale_),
  //            name.c_str());

  //     if (klass->nborn_) {
  //       /*
  //        * Make sure each method name printed only once, since a method may have
  //        * several allocation sites that allocate the same type objects.
  //        */
  //       set<string> method_names;
  //       for (set<AllocSite*>::const_iterator k = klass->alloc_sites_.begin(); k != klass->alloc_sites_.end(); ++k)
  //         method_names.insert((*k)->GetName());

  //       u32 limit = method_names.size() > 5 ? 5 : method_names.size(); /* we print only the first 5 method_names */

  //       const char *spaces = "                                                                             ";
  //       for (set<string>::const_iterator k = method_names.begin(); k != method_names.end() && limit; ++k, --limit)
  //         printf("%s\t  in %s()\n", spaces, k->c_str());
  //       if (method_names.size() > 5)
  //         printf("%s\t... %u more.\n", spaces, method_names.size() - 5);
  //       putchar('\n');
  //     }
  //   }
  // }
}
