/*****
 * texfile.cc
 * John Bowman 2003/03/14
 *
 * Encapsulates the writing of commands to a TeX file.
 *****/

#include <ctime>
#include <cfloat>

#include "texfile.h"
#include "errormsg.h"

using std::ofstream;
using settings::getSetting;

  
namespace camp {

texfile::texfile(const string& texname, const bbox& box) : box(box)
{
  texengine=getSetting<string>("tex");
  inlinetex=getSetting<bool>("inlinetex");
  Hoffset=inlinetex ? box.right : box.left;
  out=new ofstream(texname.c_str());
  if(!out || !*out) {
    cerr << "Cannot write to " << texname << endl;
    throw handled_error();
  }
  out->setf(std::ios::fixed);
  out->precision(6);
  texdocumentclass(*out);
  resetpen();
}

texfile::~texfile()
{
  if(out) {
    delete out;  
    out=NULL;
  }
}
  
void texfile::miniprologue()
{
  texuserpreamble(*out);
  *out << "\\pagestyle{empty}" << newl;
  *out << "\\begin{document}" << newl;
  texfontencoding(*out);
}

void texfile::prologue()
{
  if(inlinetex) {
    string prename=auxname(getSetting<string>("outname"),"pre");
    std::ifstream exists(prename.c_str());
    std::ofstream *outpreamble=
      new std::ofstream(prename.c_str(),std::ios::app);
    bool ASYdefines=!exists;
    texpreamble(*outpreamble,processData().TeXpreamble,ASYdefines,ASYdefines);
    outpreamble->close();
  }
  
  texdefines(*out,processData().TeXpreamble,false);
  double width=box.right-box.left;
  double height=box.top-box.bottom;
  if(settings::pdf(texengine) && !inlinetex) {
    double voffset=0.0;
    if(settings::latex(texengine)) {
      if(height < 12.0) voffset=height-12.0;
    } else if(height < 10.0) voffset=height-10.0;

    if(width > 0) 
      *out << "\\pdfpagewidth=" << width << "bp" << newl;
    if(height > 0)
      *out << "\\pdfpageheight=" << height << "bp" << newl;
  }
  if(settings::latex(texengine)) {
    *out << "\\setlength{\\unitlength}{1pt}" << newl;
    if(!inlinetex) {
      *out << "\\pagestyle{empty}" << newl
	   << "\\textheight=" << height+18.0 << "bp" << newl
	   << "\\textwidth=" << width+18.0 << "bp" << newl;
      if(settings::pdf(texengine))
	*out << "\\oddsidemargin=-89.9pt" << newl
	     << "\\evensidemargin=\\oddsidemargin" << newl
	     << "\\topmargin=-109.27pt" << newl;
      *out << "\\begin{document}" << newl;
    }
  } else {
    if(settings::pdf(texengine)) {
      *out << "\\hoffset=-92.27pt" << newl
	   << "\\voffset=-72.27pt" << newl;
    } else {
      *out << "\\hoffset=36.6pt" << newl
	   << "\\voffset=54.0pt" << newl;
    }
  }
}
    
void texfile::beginlayer(const string& psname)
{
  if(box.right > box.left && box.top > box.bottom) {
    *out << "\\includegraphics";
    if(!settings::pdf(texengine))
      *out << "[bb=" << box.left << " " << box.bottom << " "
	   << box.right << " " << box.top << "]";
    *out << "{" << psname << "}%" << newl;
    if(!inlinetex)
      *out << "\\kern-" << (box.right-box.left)*ps2tex << "pt%" << newl;
  }
}

void texfile::endlayer()
{
  if(inlinetex && (box.right > box.left && box.top > box.bottom))
    *out << "\\kern-" << (box.right-box.left)*ps2tex << "pt%" << newl;
}

void texfile::writeshifted(path p, bool newPath)
{
  write(p.transformed(shift(pair(-Hoffset,-box.bottom))),newPath);
}

void texfile::setlatexcolor(pen p)
{
  if(p.cmyk() && (!lastpen.cmyk() || 
		  (p.cyan() != lastpen.cyan() || 
		   p.magenta() != lastpen.magenta() || 
		   p.yellow() != lastpen.yellow() ||
		   p.black() != lastpen.black()))) {
    *out << "\\definecolor{ASYcolor}{cmyk}{" 
	 << p.cyan() << "," << p.magenta() << "," << p.yellow() << "," 
	 << p.black() << "}\\color{ASYcolor}" << newl;
  } else if(p.rgb() && (!lastpen.rgb() ||
			(p.red() != lastpen.red() ||
			 p.green() != lastpen.green() || 
			 p.blue() != lastpen.blue()))) {
    *out << "\\definecolor{ASYcolor}{rgb}{" 
	 << p.red() << "," << p.green() << "," << p.blue()
	 << "}\\color{ASYcolor}" << newl;
  } else if(p.grayscale() && (!lastpen.grayscale() || 
			      p.gray() != lastpen.gray())) {
    *out << "\\definecolor{ASYcolor}{gray}{" 
	 << p.gray()
	 << "}\\color{ASYcolor}" << newl;
  }
}
  
void texfile::setfont(pen p)
{
  if((p.size() != lastpen.size() || p.Lineskip() != lastpen.Lineskip()) &&
     settings::latex(texengine)) {
    *out << "\\fontsize{" << p.size() << "}{" << p.Lineskip()
	 << "}\\selectfont" << newl;
  }

  if(p.Font() != lastpen.Font()) {
    *out << p.Font() << "%" << newl;
  }
  
  lastpen=p;
}
  
void texfile::setpen(pen p)
{
  bool latex=settings::latex(texengine);
  
  p.convert();
  if(p == lastpen) return;

  if(latex) setlatexcolor(p);
  else setcolor(p,settings::beginspecial(texengine),settings::endspecial());
  
  setfont(p);
}
   
void texfile::gsave()
{
  *out << settings::beginspecial(texengine);
  psfile::gsave(true);
  *out << settings::endspecial() << newl;
}

void texfile::grestore()
{
  *out << settings::beginspecial(texengine);
  psfile::grestore(true);
  *out << settings::endspecial() << newl;
}

void texfile::beginspecial() 
{
  *out << settings::beginspecial(texengine);
}
  
void texfile::endspecial() 
{
  *out << settings::endspecial() << newl;
}
  
void texfile::beginraw() 
{
  *out << "\\ASYraw{" << newl;
}
  
void texfile::endraw() 
{
  *out << "}%" << newl;
}
  
void texfile::put(const string& label, const transform& T, const pair& z,
		  const pair& align)
{
  double sign=settings::pdf(texengine) ? 1.0 : -1.0;

  if(label.empty()) return;
  
  *out << "\\ASYalign"
       << "(" << (z.getx()-Hoffset)*ps2tex
       << "," << (z.gety()-box.bottom)*ps2tex
       << ")(" << align.getx()
       << "," << align.gety() 
       << "){";
  *out << T.getxx() << " " << sign*T.getyx()
       << " " << sign*T.getxy() << " " << T.getyy()
       << "}{" << label << "}" << newl;
}

void texfile::epilogue()
{
  if(settings::latex(texengine)) {
    if(!inlinetex)
      *out << "\\end{document}" << newl;
  } else {
      *out << "\\bye" << newl;
  }
  out->flush();
}

} //namespace camp
