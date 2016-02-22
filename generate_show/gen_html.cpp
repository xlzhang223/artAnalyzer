/*
 * gen_html.cpp - Implementation of Gen.
 *
 * History:
 *
 * Created on 2016-1-18 by WangFei.
 */
#include "gen_html.h"
#include <iomanip>

using namespace std;

void Gen::pri_1(int x)
{
  string line,word;
  while (getline(temp,line)) {
    istringstream tempLine(line);
    if(tempLine >> word) {
      if (word == "(data)") {
        show << x << endl;
	break;
      } else {
	show << word;
	while (tempLine >> word) {
	  show << " " << word;
	}
      }
    }
    show << endl;
  }
}

void Gen::pri_gc_table()
{
  string line,word;
  while (getline(temp,line)) {
    istringstream tempLine(line);
    if(tempLine >> word) {
      if (word == "(gc_live_table)") {
        for (size_t i = 0; i < gc_live.size(); i++) {
	  show << "<tr>" << endl;
	  show << "<th scope=\"row\">" << i
	       << "</th>" << endl;
	  show << "<td>" << setiosflags(ios::fixed) << setprecision(2)<< gc_live[i] << "%</td>" 
	      << endl;
	  show << "</tr>" << endl;
	}
	break;
      } else {
	show << word;
	while (tempLine >> word) {
	  show << " " << word;
	}
      }
    }
    show << endl;
  }
}

void Gen::pri_gc_live_chart()
{
  string line,word;
  while (getline(temp,line)) {
    istringstream tempLine(line);
    if(tempLine >> word) {
      if (word == "(gc_live_chart)") {
        show << setiosflags(ios::fixed) << setprecision(2)<< gc_live[0];
        for (size_t i = 1; i< 20; i++) {
          show << "," << setiosflags(ios::fixed) << setprecision(2)<< gc_live[i];
        }
	break;
      } else {
	show << word;
	while (tempLine >> word) {
	  show << " " << word;
	}
      }
    }
    show << endl;
  }
}

ofstream& Gen::pri_top_sites() {
  string line,word;
  while (getline(temp,line)) {
    if (line != "(topsites)") {
      show << line << endl;
    } else {
      break;
    }
  }
  return show;
}

ofstream& Gen::pri_leak_sites() {
  string line,word;
  while (getline(temp,line)) {
    if (line != "(leaksites)") {
      show << line << endl;
    } else {
      break;
    }
  }
  return show;
}

ofstream& Gen::pri_top_classes() {
  string line,word;
  while (getline(temp,line)) {
    if (line != "(topclasses)") {
      show << line << endl;
    } else {
      break;
    }
  }
  return show;
}

void Gen::pri_site_chart() {
  string line,word;
  while (getline(temp,line)) {
    if (line != "(site_chart)") {
      show << line << endl;
    } else {
      double x = 100;
      for (auto iter: sites) {
	show << "['" << iter.first << "'," << setiosflags(ios::fixed) << setprecision(2)<< iter.second << "]," <<endl;
	x = x - iter.second;
      }
      show << "['others'," << setiosflags(ios::fixed) << setprecision(2)<< x << "]" <<endl;
      break;
    }
  }
}

void Gen::pri_class_chart() {
  string line,word;
  while (getline(temp,line)) {
    if (line != "(class_chart)") {
      show << line << endl;
    } else {
      double x = 100;
      for (auto iter: classes) {
	show << "['" << iter.first << "'," << setiosflags(ios::fixed) << setprecision(2)<< iter.second << "]," <<endl;
	x = x - iter.second;
      }
      show << "['others'," << setiosflags(ios::fixed) << setprecision(2)<< x << "]" <<endl;
      break;
    }
  }
}

void Gen::pri_mem(int total_gc) {
  string line,word;
  while (getline(temp2,line)) {
    if (line != "(len)") {
      mshow << line << endl;
    } else {
      mshow << total_gc << endl;
      break;
    }
  }
}

void Gen::pri_ori(ifstream& ifs,ofstream& ofs)
{
  string line,word;
  while (getline(ifs,line)) {
    ofs << line << endl;
  }
}

void Gen::pri_2(int x, int y)
{
  pri_1(x);
  pri_1(y);
}

void GenSnap::new_obj(int addr,int size) {
  // while (addr+size > width*height) {
  //   vector<unsigned char> app = vector<unsigned char>(width*128,169);
  //   r.insert(r.end(), app.begin(), app.end());
  //   g.insert(g.end(), app.begin(), app.end());
  //   b.insert(b.end(), app.begin(), app.end());
  //   height = height + 128;
  // }
  for (int i =0; i < size; i++) {
    if(addr+i < width*height) {
      r[addr+i] = 0;
      g[addr+i] = 255;
      b[addr+i] = 0;
    } else {
      break;
    }
  }
}

void GenSnap::dead_obj(int addr,int size) {
  for (int i =0; i < size; i++) {
    if(addr+i < width*height) {
      r[addr+i] = 255;
      g[addr+i] = 0;
      b[addr+i] = 0;
    } else {
      break;
    }
  }
}

void GenSnap::clear() {
  for (auto& x: r) {
    x = 169;
  }
  for (auto& x: g) {
    x = 169;
  }
  for (auto& x: b) {
    x = 169;
  }
}

void GenSnap::bitmap()
{
typedef struct                       /**** BMP file header structure ****/
    {
    unsigned int   bfSize;           /* Size of file */
    unsigned short bfReserved1;      /* Reserved */
    unsigned short bfReserved2;      /* ... */
    unsigned int   bfOffBits;        /* Offset to bitmap data */
    } BITMAPFILEHEADER;

typedef struct                       /**** BMP file info structure ****/
    {
    unsigned int   biSize;           /* Size of info header */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes */
    unsigned short biBitCount;       /* Number of bits per pixel */
    unsigned int   biCompression;    /* Type of compression to use */
    unsigned int   biSizeImage;      /* Size of image data */
    int            biXPelsPerMeter;  /* X pixels per meter */
    int            biYPelsPerMeter;  /* Y pixels per meter */
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors */
    } BITMAPINFOHEADER;

BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;

 stringstream ss;
 ss << "./tem/image/" << nums++ << ".bmp";
 ofstream pic_stream(ss.str());

/* Magic number for file. It does not fit in the header structure due to alignment requirements, so put it outside */
unsigned short bfType=0x4d42;           
bfh.bfReserved1 = 0;
bfh.bfReserved2 = 0;
bfh.bfSize = 2+sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+width*height*3;
bfh.bfOffBits = 0x36;

bih.biSize = sizeof(BITMAPINFOHEADER);
bih.biWidth = width;
bih.biHeight = height;
bih.biPlanes = 1;
bih.biBitCount = 24;
bih.biCompression = 0;
bih.biSizeImage = 0;
bih.biXPelsPerMeter = 5000;
bih.biYPelsPerMeter = 5000;
bih.biClrUsed = 0;
bih.biClrImportant = 0;



/*Write headers*/
 pic_stream.write((char*)(&bfType),sizeof(bfType));
 pic_stream.write((char*)(&bfh),sizeof(bfh)); 
 pic_stream.write((char*)(&bih),sizeof(bih));
/*Write bitmap*/
for (int y = bih.biHeight-1; y>=0; y--) /*Scanline loop backwards*/
    {
    for (int x = 0; x < bih.biWidth; x++) /*Column loop forwards*/
        {
        /*compute some pixel values*/
        unsigned char rc = r[y*width+x];
        unsigned char gc = g[y*width+x];
        unsigned char bc = b[y*width+x];
	pic_stream << bc << gc << rc;
        }
    }
 pic_stream.close();
}
