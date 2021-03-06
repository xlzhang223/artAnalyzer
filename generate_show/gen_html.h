/*-*- mode:c++ -*-
 *
 * gen_html.h - gen html files.
 *
 * History:
 *
 * Created on 2016-1-18 by WangFei.
 */
#ifndef GEN_H_
#define GEN_H_
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;


class GenSnap {
public:
  GenSnap (int height_): nums(0),width(1024),height(height_) {
    r = vector<unsigned char>(width*height,169);
    g = vector<unsigned char>(width*height,169);
    b = vector<unsigned char>(width*height,169);
    bitmap();
  }
  
  ~GenSnap() {
    bitmap();
  }
  
  void new_obj(int addr,int size);
  void dead_obj(int addr,int size);
  void bitmap();
  void clear();
private:
  int nums;
  int width;
  int height;
  vector<unsigned char> r;
  vector<unsigned char> g;
  vector<unsigned char> b;
};

class Gen {
public:
  vector<double> gc_live;
  vector<pair<string,double> > sites;
  vector<pair<string,double> > classes;
  
  Gen () {
    show.open("./tem/index.html");
    temp.open("./tem/template");
    mshow.open("./tem/mem.html");
    temp2.open("./tem/mtemplate");
  }

  ~Gen() {
    pri_gc_live_chart();
    pri_site_chart();
    pri_class_chart();

    pri_ori(temp,show);
    pri_ori(temp2,mshow); 
    show.close();
    temp.close();
    mshow.close();
    temp2.close();
  }
  void pri_2(int x,int y);
  void pri_gc_live(vector<double>& gc_data) {
    gc_live = gc_data;
    pri_gc_table();
  }
  ofstream& pri_top_sites();
  ofstream& pri_top_classes();
  ofstream& pri_leak_sites();
  void pri_mem(int total_gc);

private:
  ofstream show;
  ifstream temp;
  ofstream mshow;
  ifstream temp2;
  
  void pri_1(int x);
  void pri_ori(ifstream& ifs,ofstream& ofs);
  void pri_gc_table();
  void pri_gc_live_chart();
  void pri_site_chart();
  void pri_class_chart();
};


#endif  /* GEN_H_ */
