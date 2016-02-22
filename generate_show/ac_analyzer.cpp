//
// ac_analyzer.cpp - Analyzer program for ART.
//
// History:
//
//   Created on 2015-11-8 by Hengyang Yu.
//
#include <vector>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "ac_defs.h"
#include "object.h"
#include "address_chain.h"
#include "class_method.h"
#include "alloc_site.h"
#include "manager.h"
#include "inline.h"
#include "options.h"
#include "debug.h"
#include "gen_html.h"
#include <cassert>

using namespace std;

static bool first =false;
static bool first_semi = false;
static u32 min_address;
static u32 max_address;

map<u32,int> data_map;
static GenSnap* gen_snap;

static int g_min_idx, g_max_idx;
u32 g_ngc, g_nnew, g_nmove, g_ndead, g_naccess, g_nconflict;
static u32 g_cur_data;
static u32 g_nobj_in_table;
static ACHead *g_table[kTableSize];
static u32 g_lifetime[kMaxObjLifeTime];
static bool g_is_compact_gc = false;
static bool g_gc_started = false;
static FILE *g_fp;

static Gen *gen_html;
static ClassManager *g_p_class_manager;
static SiteManager  *g_p_site_manager;
static Options g_options;

static MethodTable g_method_table;

const std::string& FindMethodNameBySite(u32 site) {
  static std::string unknown("N/A");

  for (auto mp : g_method_table)
    if (site >= mp->code_begin_ && site <= mp->code_end_)
      return mp->name_;

  return unknown;
}

u32 GetNumNewObject() { return g_nnew; }

static size_t next_u32(u32 *p);

static inline Chain* alloc_chain_node(u32 addr) {
  return new Chain(addr, g_ngc);
}

static inline void free_chain(Chain *cp) {
  Chain *next;

  while(cp) {
    next = cp->next_;
    delete cp;
    cp = next;
  }
}

static inline void free_achead(ACHead *ap) {
  free_chain(ap->cp_);	// first free chain nodes of ap
  delete ap;
}


static inline void do_remove(ACHead *p) {
  assert(p);

  // Some objects were allocated during CMS, we treat
  // their born time as cur g_ngc
  ++g_lifetime[g_ngc - p->born_time_ - (p->born_time_ != g_ngc)];
  free_achead(p);

  --g_nobj_in_table;
}


static void err_msg(const char *msg, int line)
{
  fprintf(stderr, "%s, reported at line %d\n", msg, line);
  exit(1);
}




static void mark_nonmoved_object_as_dead(bool just_update_staleness)
{
  // printf("%s\n", __FUNCTION__);
  assert(!g_gc_started);
  int i;

  assert(g_min_idx >= 0);
  assert(g_max_idx < kTableSize);
  assert(g_min_idx <= g_max_idx);

  u32 total_object_size = 0;
  for (i = g_min_idx; i <= g_max_idx; ++i) { // for each slot in table
    ACHead **pp, *p;

    pp = &g_table[i];
    while (*pp) {	// for each object in the same slot
      p = *pp;
      assert(HASH(p->cp_->addr_) == (u32)i);

      if (!just_update_staleness && not_large(p->mixword_) && not_moved(p->mixword_)) {
        *pp = p->next_;
        do_remove(p);
        ++g_ndead;
      } else {
        pp = &p->next_;
        // in case objects were born during CMS, we don't think they are staled in current gc
        if (p->born_time_ < g_ngc)
          p->mixword_ += 1<<kStalenessShiftBits; // <=> o.staleness++
        p->mixword_ &= ~kMoveBit;
        p->asp_->AddStaleObj(p);
        p->klass_->AddStaleObj(p);
        total_object_size += p->obj_size_;
      }
    }
  }
  printf("Mem: %u\n", total_object_size);
}

static inline void table_insert(ACHead *ap) {
  assert(ap && ap->cp_);

  int h = HASH(ap->cp_->addr_);
  if (g_table[h]) {	/* conflicts */
    ap->next_ = g_table[h];
    g_nconflict++;
  } else {		/* slot is empty before */
    if (h < g_min_idx)
      g_min_idx = h;

    if (h > g_max_idx)
      g_max_idx = h;
  }

  g_table[h] = ap;
}


static inline void mark_dead_object(u32 addr)
{
  gen_snap->dead_obj((addr-min_address)/4,data_map[addr]);
}


static inline void new_object(const Object *obj) {
  AllocSite *sitep = g_p_site_manager->LookUp(obj->alloc_site);
  Class  *klass = g_p_class_manager->LookUp(obj->GetClass());
  u32 mixword = make_mix_word(0, obj->IsLarge(), 0, g_ngc); // move bit, large bit, staleness, last access time

  u32 size = obj->size ? obj->size : klass->GetInstanceSize();

  if (size == 0) size = 8;

  if (obj->Kind() != kLargeObject) {
    if (first == false) {
      min_address = obj->addr;
      max_address = obj->addr;
      first = true;
    } else if(obj->addr > max_address) {
      max_address = obj->addr;
    }
    gen_snap->new_obj((obj->addr-min_address)/4,(size+3)/4);
    data_map[obj->addr] = (size+3)/4;
  }

  ACHead *ap = new ACHead(sitep, klass, alloc_chain_node(obj->addr), size, g_ngc, mixword);

  if (klass == nullptr)
    err_msg("klass pointer not found.", __LINE__);

  table_insert(ap);
  // make type and allocation site cross referenced
  klass->AddAllocSite(sitep);
  sitep->AddClass(klass);
  ++sitep->nborn_;
  ++klass->nborn_;
  ++g_nnew;
  ++g_nobj_in_table;
}
static inline void remove_object(u32 addr) {
  assert(g_gc_started);
  ++g_ndead;
  // printf("Remove %X\n", addr);
  int h = HASH(addr);
  if (g_table[h])
    for (ACHead **app = &g_table[h], *ap = NULL; *app; app = &ap->next_) {
      ap = *app;
      if (ap->cp_->addr_ == addr) {
        *app = ap->next_;
        do_remove(ap);
        return ;
      }
    }
  // printf("leave\n");
}
static inline void move_object(u32 prev_addr, u32 cur_addr) {
  assert(g_gc_started);

  g_nmove++;
  // printf("from %X to %X\n", prev_addr, cur_addr);
  /*
   * The complication is when an object is moved to a new address, it
   * should be repositioned according to the new hash value of this
   * new address. So
   *
   * step1: find the corresponding ACHead of prevaddr_;
   * step2: insert a chain node containing curaddr_ into the found ACHead;
   * step3: remove ACHead from table;
   * step4: insert ACHead into table according to curaddr_'s hash value.
   *
   */
  int h = HASH(prev_addr);
  // assert(g_table[h]);

  /* step1: find prevaddr_ */
  for (ACHead **app = &g_table[h], *ap = NULL; *app; app = &ap->next_) {
    ap = *app;
    if (ap->cp_->addr_ == prev_addr) {

      /* step2: insert curaddr_ as head in chain */
      Chain *cp = alloc_chain_node(cur_addr);
      cp->next_ = ap->cp_;
      ap->cp_ = cp;
      ap->mixword_ |= kMoveBit; /* mark as moved */

      /* step3: remove prevaddr_ from current hash slot */
      *app = ap->next_;

      /* step4: insert curaddr_ into table */
      ap->next_ = NULL;
      table_insert(ap);

      // gen_snap new
       if (first_semi == true) {
	 min_address = cur_addr;
	 first_semi = false;
       }
       int size = data_map[prev_addr];
       gen_snap->new_obj((cur_addr-min_address)/4,size);
       data_map[cur_addr] = size;
       data_map.erase(prev_addr);
   

      return ;
    }
  }
}

static void access_object(u32 addr)
{
  int h = HASH(addr);

  ++g_naccess;
  if (g_table[h]) {
    for (ACHead *ap = g_table[h]; ap; ap = ap->next_) {
      assert(ap->cp_);
      if (ap->cp_->addr_ == addr) {
        ap->mixword_ = make_mix_word(move_bit(ap->mixword_), large_bit(ap->mixword_), 0, g_ngc);
        ap->cp_->stamp_ = g_ngc;
        return;
      }
    }
  }
}




static void expect(u32 expected)
{
  if (g_cur_data != expected) {
    printf("expect %x but give %x\n", expected, g_cur_data);
    err_msg("Corrupted data file.", __LINE__);
  } else {
    if (expected != kAppEnd)    // no more input
      next_u32(&g_cur_data);
  }
}

static void handle_new_object()
{
  Object obj;

  obj.addr = OBJ(g_cur_data);
  //printf("%x\n", obj.addr);
  next_u32(&obj.alloc_site);
  next_u32(&obj.klass);

  obj.size = 0;
  if (obj.Kind() != kNormalObject)
    next_u32(&obj.size);
  new_object(&obj);
}

static void mutator_info()
{
  // printf("%s\n", __FUNCTION__);
  while (TAG(g_cur_data) == kNewObject) {
    handle_new_object();
    next_u32(&g_cur_data);
  }
  // printf("%s end, %x, tag = %x\n", __FUNCTION__, g_cur_data, TAG(g_cur_data));
}

static void before_collection()
{
  gen_snap->bitmap();
  g_p_site_manager->CollectionStart();
  g_p_class_manager->CollectionStart();
}

static void after_collection()
{
  gen_snap->bitmap();
  g_p_site_manager->CollectionFinished();
  g_p_class_manager->CollectionFinished();
}

static void collector_info()
{
  // printf("enter %s\n", __FUNCTION__);

  u32 compacting, prefetch;
  expect(kGcStart);
  compacting = g_cur_data;
  next_u32(&prefetch);

  if (prefetch == kGcEnd) // not a real gc
    next_u32(&g_cur_data);
  else {
    g_cur_data = prefetch;

    std::vector<u32> vdead;
    std::vector<std::pair<u32, u32> > vmove;

    ++g_ngc;
    if (kDebugBuild)
      printf("%dth GC, %s compacting gc (%X), objects in table: %u\n",
             g_ngc, compacting ? "a" : "not a", compacting, g_nobj_in_table);
    g_gc_started = true;
    g_is_compact_gc = static_cast<bool>(compacting);
    if (g_is_compact_gc) {
      first_semi = true;
      gen_snap->clear();
    }
    
    while (g_cur_data != kGcEnd && g_cur_data != kAppEnd) {
      switch (TAG(g_cur_data)) {
      case kNewObject:
        handle_new_object();
        break;

      case kReclaimObject:
        // remove_object(obj);
	mark_dead_object(OBJ(g_cur_data));
        vdead.push_back(OBJ(g_cur_data));
        break;

      case kAccessObject:
        access_object(OBJ(g_cur_data));
        break;

      case kMoveObject: {
        u32 dest = kObjTagMask;
        next_u32(&dest);
        assert(0 == (dest & kObjTagMask));
        // vmove.push_back(OBJ(g_cur_data));
        // vmove.push_back(dest);
        vmove.push_back(std::make_pair(OBJ(g_cur_data), dest));
        // move_object(obj, dest);
      }
        break;

      default:
        fprintf(stderr, "invalid data: %X\n", g_cur_data);
        break;
      }

      next_u32(&g_cur_data);
    }
    for (auto obj : vdead)
      remove_object(obj);

    for (auto &obj_pair : vmove)
      move_object(obj_pair.first, obj_pair.second);

    first_semi = false;
    g_gc_started = false;
    mark_nonmoved_object_as_dead(!g_is_compact_gc);
    g_is_compact_gc = false;
    expect(kGcEnd);
  }
  // printf("leave %s\n", __FUNCTION__);
}



static void cleanup()
{
  ACHead *ap, *q;
  int i;

  // fake a gc
  printf("%s\n", __FUNCTION__);
  ++g_ngc;
  for (i = g_min_idx; i <= g_max_idx; i++)
    for (ap = g_table[i]; ap; ap = q) {
      q = ap->next_;
      do_remove(ap);
    }
  --g_ngc;
}
static void print_interval(int start, int end)
{
  for (int i = start; i < end; ++i)
    printf("%6d ", i);
  putchar('\n');
  vector<double> gc_live_data;
  for (int i = start; i < end; ++i) {
    double per = double(g_lifetime[i])/g_nnew*100;
    printf("%5.2lf%% ", per);
    gc_live_data.push_back(per);
  }
  gen_html->pri_gc_live(gc_live_data);
  putchar('\n');
}
static void summary()
{
  cleanup();
  printf("new = %u\ndead = %u\nmove = %u\naccess = %u\nNo. GC = %u\n",
         g_nnew, g_ndead, g_nmove, g_naccess, g_ngc);
  printf("No. allocation site = %u\n", g_p_site_manager->GetNumSites());
  gen_html->pri_2(g_nnew,g_ndead);
  gen_html->pri_2(g_nmove,g_naccess);
  gen_html->pri_2(g_ngc,g_p_site_manager->GetNumSites());
  gen_html->pri_mem(g_ngc);
  print_interval(0, 20);
  g_p_site_manager->Summary();
  g_p_class_manager->Summary();
}




static void analyze()
{
  next_u32(&g_cur_data);
  expect(kAppStart);
  mutator_info();
  while (g_cur_data == kGcStart) {
    before_collection();
    collector_info();
    after_collection();
    mutator_info();
  }
  expect(kAppEnd);
  summary();
}

int main(int argc, const char *argv[])
{
  g_options.parse(argc, argv);

  gen_html = new Gen();
  gen_snap = new GenSnap(1280);
  g_p_class_manager = new ClassManager(gen_html,&g_options);
  g_p_site_manager = new SiteManager(gen_html,&g_options);

  if (!ReadClassAndMethod(g_options.get_methodfile_name().c_str(),
                          g_method_table,
                          *g_p_class_manager,
			  gen_html)) {
    delete g_p_class_manager;
    delete g_p_site_manager;
    delete gen_html;
    delete gen_snap;
    err_msg("Reading method class file failed.", __LINE__);
  } else {
    g_min_idx = kTableSize - 1; /* YES! */
    g_max_idx = 0;
    g_fp = stdin;
    analyze();
    delete g_p_class_manager;
    delete g_p_site_manager;
    delete gen_html;
    delete gen_snap;
  }
  printf("\nMin_adress %x, Max_address%x, Way %d\n", min_address, max_address,max_address-min_address);
  return 0;
}

static size_t next_u32(u32 *p) {
  size_t count =  fread(p, sizeof *p, 1, g_fp);

  if (count == 0) {
    g_cur_data = kAppEnd;
    summary();
    delete g_p_class_manager;
    delete g_p_site_manager;
    delete gen_html;
    delete gen_snap;
    exit(0);
  }
  return count;
}
