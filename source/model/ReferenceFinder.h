/******************************************************************************
File:         ReferenceFinder.h
Project:      ReferenceFinder 4.x
Purpose:      Header for ReferenceFinder generic model
Author:       Robert J. Lang
Modified by:  
Created:      2006-04-22
Copyright:    ©1999-2007 Robert J. Lang. All Rights Reserved.
******************************************************************************/
 
#ifndef _REFERENCEFINDER_H_
#define _REFERENCEFINDER_H_

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <fstream>
#include <limits>

// If this symbol is defined, adds the mark key as part of the label, which can
// be helpful in debugging.
//#define RF_PUT_KEY_IN_TEXT

/******************************************************************************
Section 1: lightweight classes that represent points and lines.
******************************************************************************/

/**********
Globals
**********/
const double EPS = 1.0e-8; // used for equality of XYPts, parallelness of XYLines

/*****
Utilities
*****/
template <class T>
inline T min_val(T t1, T t2) {return (t1 < t2) ? t1 : t2;};
template <class T>
inline T max_val(T t1, T t2) {return (t1 > t2) ? t1 : t2;};


/**********
class XYPt - a 2-vector that represents a point or a direction.
**********/
class XYPt {
public:
  double x; // x coordinate
  double y; // y coordinate
  
  // Constructor
  XYPt(double xx = 0, double yy = 0) : x(xx), y(yy) {}
  
  // Arithmetic with XYPts and scalars
  const XYPt operator+(const XYPt& p) const {return XYPt(x + p.x, y + p.y);}
  const XYPt operator-(const XYPt& p) const {return XYPt(x - p.x, y - p.y);}
  const XYPt operator*(const XYPt& p) const {return XYPt(x * p.x, y * p.y);}
  const XYPt operator/(const XYPt& p) const {return XYPt(x / p.x, y / p.y);}
  
  const XYPt operator+(double z) const {return XYPt(x + z, y + z);}
  const XYPt operator-(double z) const {return XYPt(x - z, y - z);}
  const XYPt operator*(double z) const {return XYPt(x * z, y * z);}
  const XYPt operator/(double z) const {return XYPt(x / z, y / z);}
  
  friend const XYPt operator+(double d, const XYPt& pp) {
    return XYPt(d + pp.x, d + pp.y);}
  friend const XYPt operator-(double d, const XYPt& pp) {
    return XYPt(d - pp.x, d - pp.y);}
  friend const XYPt operator*(double d, const XYPt& pp) {
    return XYPt(d * pp.x, d * pp.y);}
  friend const XYPt operator/(double d, const XYPt& pp) {
    return XYPt(d / pp.x, d / pp.y);}
  
  XYPt& operator+=(const XYPt& p) {x += p.x; y += p.y; return (*this);}
  XYPt& operator-=(const XYPt& p) {x -= p.x; y -= p.y; return (*this);}
  XYPt& operator*=(const XYPt& p) {x *= p.x; y *= p.y; return (*this);}
  XYPt& operator/=(const XYPt& p) {x /= p.x; y /= p.y; return (*this);}

  XYPt& operator+=(double z) {x += z; y += z; return (*this);}
  XYPt& operator-=(double z) {x -= z; y -= z; return (*this);}
  XYPt& operator*=(double z) {x *= z; y *= z; return (*this);}
  XYPt& operator/=(double z) {x /= z; y /= z; return (*this);}

  // Counterclockwise rotation
  const XYPt Rotate90() const {return XYPt(-y, x);}
  const XYPt RotateCCW(double a) const {  // a is in radians
    double sa = std::sin(a);
    double ca = std::cos(a);
    return XYPt(ca * x - sa * y, sa * x + ca * y);}
  
  // Scalar products and norms
  double Dot(const XYPt& p) const {
    return x * p.x + y * p.y;}
  double Mag2() const {
    return x * x + y * y;}
  double Mag() const {
    return std::sqrt(x * x + y * y);}
  const XYPt Normalize() const {
    double m = Mag(); return XYPt(x / m, y / m);}
  XYPt& NormalizeSelf() {
    double m = Mag(); x /= m; y /= m; return *this;}
  
  // Other utilities
  friend const XYPt MidPoint(const XYPt& p1, const XYPt& p2) {
    return XYPt(0.5 * (p1.x + p2.x), 0.5 * (p1.y + p2.y));}
  
  // Chop() makes numbers close to zero equal to zero.
  const XYPt Chop() const {
    return XYPt(std::abs(x) < EPS ? 0 : x, std::abs(y) < EPS ? 0 : y);}
  XYPt& ChopSelf() {
    if (std::abs(x) < EPS) x = 0; 
    if (std::abs(y) < EPS) y = 0; return *this;}
  
  // Comparison
  bool operator==(const XYPt& p) const {
    return (*this - p).Mag() < EPS;}
  
  // Stream I/O
  friend std::ostream& operator<<(std::ostream& os, const XYPt& p);
};


/**********
class XYLine - a class for representing a line by a scalar and the normal to
the line.
**********/
class XYLine {
public:
  double d; // d*u is the point on the line closest to the origin
  XYPt u;   // a unit normal vector to the line
  
  // Constructors
  XYLine(double dd = 0, const XYPt& uu = XYPt(1, 0)) : d(dd), u(uu) {}
  
  XYLine(const XYPt& p1, const XYPt& p2) {
    // line through two points
    u = (p2 - p1).Normalize().Rotate90();
    d = p1.Dot(u);
  }
  
  XYPt Fold(const XYPt& p1) const {
    // Fold a point about the line
    return p1 + 2 * (d - (p1.Dot(u))) * u;
  }
  
  bool IsParallelTo(const XYLine& ll) const {
    // true if lines are parallel
    return std::abs(u.Dot(ll.u.Rotate90())) < EPS;
  }
  
  bool operator==(const XYLine& ll) const {
    // true if lines are same
    return (std::abs(d - ll.d * u.Dot(ll.u)) < EPS) && 
      (std::abs(u.Dot(ll.u.Rotate90())) < EPS);
    }
    
  bool Intersects(const XYPt& pp) const {
    // true if pt on line
    return (std::abs(d - pp.Dot(u)) < EPS);
  }
    
  bool Intersects(const XYLine& ll, XYPt& pp) const {
    // true if lines intersect, intersection goes in pp
    double denom = u.x * ll.u.y - u.y * ll.u.x;
    if (std::abs(denom) < EPS) return false;
    pp.x = (d * ll.u.y - ll.d * u.y) / denom;
    pp.y = (ll.d * u.x - d * ll.u.x) / denom;
    return true;
  }
    
  // Intersection() just returns the intersection point, no error checking
  // for parallel-ness. Use Intersects() when in doubt.
  friend const XYPt Intersection(const XYLine& l1, const XYLine& l2) {
    double denom = l1.u.x * l2.u.y - l1.u.y * l2.u.x;
    return XYPt((l1.d * l2.u.y - l2.d * l1.u.y) / denom, 
      (l2.d * l1.u.x - l1.d * l2.u.x) / denom);
  }

  // Stream I/O
  friend std::ostream& operator<<(std::ostream& os, const XYLine& l);
};    
    

/**********
class XYRect - a class for representing rectangles by two points, the bottom
left and top right corners.
**********/
class XYRect {
public:
  XYPt bl;  // bottom left corner
  XYPt tr;  // top right corner
  
  // Constructors
  XYRect(const XYPt& ap) : bl(ap), tr(ap) {}
  XYRect(const XYPt& abl, const XYPt& atr) : bl(abl), tr(atr) {}
  XYRect(double ablx, double ably, double atrx, double atry) : 
    bl(ablx, ably), tr(atrx, atry) {}
    
  // Dimensional queries
  double GetWidth() const {return tr.x - bl.x;}
  double GetHeight() const {return tr.y - bl.y;}
  double GetAspectRatio() const;
  
  // Boolean queries
  bool IsValid() const {
    // Return true if bl is below and to the left of tr
    return (bl.x <= tr.x) && (bl.y <= tr.y);}
  bool IsEmpty() const {
    // Return true if the rectangle is a line or point
    return (std::abs(bl.x - tr.x) < EPS) || (std::abs(bl.y - tr.y) < EPS);}
  bool Encloses(const XYPt& ap) const {
    // Return true if pt falls within this rectangle, padded by EPS
    return (ap.x >= bl.x - EPS) && (ap.x <= tr.x + EPS) &&
    (ap.y >= bl.y - EPS) && (ap.y <= tr.y + EPS);}
  bool Encloses(const XYPt& ap1, XYPt& ap2) const {
    // Return true if both pts fall within the rectangle.
      return Encloses(ap1) && Encloses(ap2);}
    
  // Include(p) stretches the coordinates so that this rect encloses the point.
  // Returns a reference so multiple calls can be chained.
  XYRect& Include(const XYPt& p) {
    if (bl.x > p.x) bl.x = p.x;
    if (bl.y > p.y) bl.y = p.y;
    if (tr.x < p.x) tr.x = p.x;
    if (tr.y < p.y) tr.y = p.y;
    return *this;
  }
  
  // Stream I/O
  friend std::ostream& operator<<(std::ostream& os, const XYRect& r);
};


/*****
Return an XYRect that encloses all of the points passed as parameters.
*****/
inline const XYRect GetBoundingBox(const XYPt& p1, const XYPt& p2) {
    return XYRect(p1).Include(p2);
}
inline const XYRect GetBoundingBox(const XYPt& p1, const XYPt& p2,
                                const XYPt& p3) {
    return XYRect(p1).Include(p2).Include(p3);
}


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
* Section 2: classes that represent reference marks and reference lines on a 
* rectangular piece of paper.
******************************************************************************/

/**********
class Paper - specialization of XYRect for representing the paper
**********/
class Paper : public XYRect {
public:
  double mWidth;        // width of the paper
  double mHeight;       // height of the paper
  XYPt mBotLeft;       // only used by MakeAllMarksAndLines
  XYPt mBotRight;        // ditto
  XYPt mTopLeft;       // ditto
  XYPt mTopRight;        // ditto
  XYLine mTopEdge;       // used by InteriorOverlaps() and MakeAllMarksAndLines
  XYLine mLeftEdge;      // ditto
  XYLine mRightEdge;     // ditto
  XYLine mBottomEdge;      // ditto
  XYLine mUpwardDiagonal;    // only used by MakeAllMarksAndLines
  XYLine mDownwardDiagonal;  // ditto
  
  std::string mWidthAsText; // paper width, as an expression
  std::string mHeightAsText; // paper height, as an expression

  Paper(double aWidth, double aHeight);
  void SetSize(double aWidth, double aHeight);
  
  bool ClipLine(const XYLine& al, XYPt& ap1, XYPt& ap2) const;
  bool InteriorOverlaps(const XYLine& al) const;
  bool MakesSkinnyFlap(const XYLine& al) const;
  
  void DrawSelf();
};

class RefDgmr;  // forward declaration, see Section 5 below

/**********
class RefBase - base class for a mark or line. 
**********/
class RefBase {
public:
  typedef unsigned short rank_t;
  typedef int key_t;
  
  rank_t mRank;         // rank of this mark or line
  key_t mKey;           // key used for maps within RefContainers

  static std::vector<RefBase*> sSequence; // a sequence of refs that fully define a ref

  struct DgmInfo {        // information that encodes a diagram description
    std::size_t idef;     // first ref that's defined in this diagram
    std::size_t iact;     // ref that terminates this diagram
    DgmInfo(std::size_t adef, std::size_t aact) : idef(adef), iact(aact) {}
  };
  static std::vector<DgmInfo> sDgms;  // a list of diagrams that describe a given ref

protected:
  typedef short index_t;        // type for indices
  index_t mIndex;               // used to label this ref in a folding sequence
  
  static RefDgmr* sDgmr;        // object that draws diagrams
  static bool sClarifyVerbalAmbiguities;// true = clarify ambiguous verbal instructions
  static bool sAxiomsInVerbalDirections;// true = list the axiom number in verbal instructions
  enum {
    // Drawing happens in multiple passes to get the stacking order correct
    PASS_LINES, 
    PASS_HLINES, 
    PASS_POINTS, 
    PASS_ARROWS, 
    PASS_LABELS, 
    NUM_PASSES
  }; // drawing order
  
public:
  RefBase(rank_t arank = 0) : mRank(arank), mKey(0), mIndex(0) {}    
  virtual ~RefBase() {}

  // routines for building a sequence of refs
  virtual void SequencePushSelf();
  void BuildAndNumberSequence();
  
  // routine for creating a text description of how to fold a ref
  virtual const char GetLabel() const = 0;
  virtual bool PutName(std::ostream& os) const = 0;
  virtual bool PutHowto(std::ostream& os) const;
  std::ostream& PutHowtoSequence(std::ostream& os);
  
  // routines for drawing diagrams
  void BuildDiagrams();
  static void DrawPaper();
  static void DrawDiagram(RefDgmr& aDgmr, const DgmInfo& aDgm);
  static void PutDiagramCaption(std::ostream& os, const DgmInfo& aDgm);

protected:
  virtual bool UsesImmediate(RefBase* rb) const;
  virtual bool IsActionLine() const = 0;
  virtual bool IsDerived() const;
  virtual void SetIndex() = 0;
  static void SequencePushUnique(RefBase* rb);
  enum RefStyle {
    REFSTYLE_NORMAL, 
    REFSTYLE_HILITE, 
    REFSTYLE_ACTION
  };
  virtual void DrawSelf(RefStyle rstyle, short ipass) const = 0;
};


/**********
class RefMark - base class for a mark on the paper. 
**********/
class RefMark : public RefBase {
public:
  typedef XYPt bare_t;    // type of bare object a RefMark represents
  bare_t p;               // coordinates of the mark
private:
  static index_t sCount;    // class index, used for numbering sequences of marks
  static char sLabels[];    // labels for marks, indexed by sCount

public:
  RefMark(rank_t arank) : RefBase(arank) {}
  RefMark(const XYPt& ap, rank_t arank) : RefBase(arank), p(ap) {}
  
  void FinishConstructor();
  
  double DistanceTo(const XYPt& ap) const;
  bool IsOnEdge() const;    
  bool IsActionLine() const;

  const char GetLabel() const;
  bool PutName(std::ostream& os) const;
  void PutDistanceAndRank(std::ostream& os, const XYPt& ap) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;

protected:
  static rank_t CalcMarkRank(const RefBase* ar1, const RefBase* ar2) {
    return ar1->mRank + ar2->mRank;}
  void SetIndex();

private:
  static void ResetCount();
  friend class RefBase;
};


/**********
class RefMark_Original - Specialization of RefMark that represents a named mark
(like a corner).
**********/
class RefMark_Original : public RefMark {
private:
  std::string mName;  // name of the mark
  
public:
  RefMark_Original(const XYPt& ap, rank_t arank, std::string aName);

  const char GetLabel() const;
  bool PutName(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  
protected:
  virtual bool IsDerived() const;
  void SetIndex();
};


class RefLine;  // forward declaration needed by RefMark_Intersection

/**********
class RefMark_Intersection - Specialization of a RefMark for a mark defined by
the intersection of 2 lines.
**********/
class RefMark_Intersection : public RefMark {
public:
  RefLine* rl1;   // first line
  RefLine* rl2;   // second line
  
  RefMark_Intersection(RefLine* al1, RefLine* al2);

  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();      
  bool PutHowto(std::ostream& os) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine - base class for a reference line. 
**********/
class RefLine : public RefBase {
public: 
  typedef XYLine bare_t;    // type of bare object that a RefLine represents
  bare_t l;         // the line this contains
private:
  static index_t sCount;    // class index, used for numbering sequences of lines
  static char sLabels[];    // labels for lines, indexed by sCount

public:
  RefLine(rank_t arank) : RefBase(arank) {}
  RefLine(const XYLine& al, rank_t arank) : RefBase(arank), l(al) {}

  void FinishConstructor();
  double DistanceTo(const XYLine& al) const;
  bool IsOnEdge() const;
  bool IsActionLine() const;

  const char GetLabel() const;
  bool PutName(std::ostream& os) const;
  void PutDistanceAndRank(std::ostream& os, const XYLine& al) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  
protected:
  static rank_t CalcLineRank(const RefBase* ar1, const RefBase* ar2) {
    return 1 + ar1->mRank + ar2->mRank;}   
  static rank_t CalcLineRank(const RefBase* ar1, const RefBase* ar2, 
    const RefBase* ar3) {return 1 + ar1->mRank + ar2->mRank + ar3->mRank;}   
  static rank_t CalcLineRank(const RefBase* ar1, const RefBase* ar2, 
    const RefBase* ar3, const RefBase* ar4) {
    return 1 + ar1->mRank + ar2->mRank + ar3->mRank + ar4->mRank;}
  void SetIndex();

private:
  static void ResetCount();
  friend class RefBase;
};


/**********
class RefLine_Original - Specialization of RefLine that represents a line that
is the edge of the paper or an initial crease (like the diagonal).
**********/
class RefLine_Original : public RefLine {
private:
  std::string mName;  // name of the line
  
public:
  RefLine_Original(const XYLine& al, rank_t arank, std::string aName);

  bool IsActionLine() const;
  const char GetLabel() const;
  bool PutName(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;

protected:
  virtual bool IsDerived() const;
  void SetIndex();
};


/**********
class RefLine_C2P_C2P - Huzita-Hatori Axiom O1
Make a crease through two points p1 and p2.
**********/
class RefLine_C2P_C2P : public RefLine {
public:
  RefMark* rm1;       // make a crease from one mark...
  RefMark* rm2;       // to another mark
  
  RefLine_C2P_C2P(RefMark* arm1, RefMark* arm2);

  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine_P2P - Huzita-Hatori Axiom O2
Bring p1 to p2.
**********/
class RefLine_P2P : public RefLine {
public:
  RefMark* rm1;       // bring one mark...
  RefMark* rm2;       // to another mark, and form a crease.
  
private:
  enum WhoMoves {
    WHOMOVES_P1,
    WHOMOVES_P2
  };  
  WhoMoves mWhoMoves;

public:
  RefLine_P2P(RefMark* arm1, RefMark* arm2);
  
  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine_L2L - Huzita-Hatori Axiom O3
Bring line l1 to line l2.
**********/
class RefLine_L2L : public RefLine {
public:
  RefLine* rl1;       // make a crease by bringing one line...
  RefLine* rl2;       // to another line
  
private:
  enum WhoMoves {
    WHOMOVES_L1,
    WHOMOVES_L2
  };  
  WhoMoves mWhoMoves;

public: 
  RefLine_L2L(RefLine* arl1, RefLine* arl2, short iroot);
  
  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine_L2L_C2P - Huzita-Hatori Axiom O4.
Bring line l1 to itself so that the crease passes through point p1.
**********/
class RefLine_L2L_C2P : public RefLine {
public:
  RefLine* rl1;       // bring line l1 to itself
  RefMark* rm1;       // so that the crease runs through another point.
  
  RefLine_L2L_C2P(RefLine* arl1, RefMark* arm1);
  
  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine_P2L_C2P - Huzita-Hatori Axiom O5.
Bring point p1 to line l1 so that the crease passes through point p2.
**********/
class RefLine_P2L_C2P : public RefLine {
public:
  RefMark* rm1;       // bring a point...
  RefLine* rl1;       // to a line...
  RefMark* rm2;       // so that the crease runs through another point.
  
private:
  enum WhoMoves {
    WHOMOVES_P1,
    WHOMOVES_L1
  };  
  WhoMoves mWhoMoves;
  
public:
  RefLine_P2L_C2P(RefMark* arm1, RefLine* arl1, RefMark* arm2, short iroot);
  
  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine_P2L_P2L - Huzita-Hatori Axiom O6 (the cubic!)
Bring point p1 to line l1 and point p2 to line l2
**********/
class RefLine_P2L_P2L : public RefLine {
public:
  RefMark* rm1;       // bring a point...
  RefLine* rl1;       // to a line...
  RefMark* rm2;       // and another point...
  RefLine* rl2;       // to another line.

private:
  static short order;     // the order of the equation
  static short irootMax;    // maximum value of iroot, = ((# of roots) - 1)

  static double q1;     // used for quadratic equation solutions
  static double q2;
  
  static double S;      // used for cubic equation solutions
  static double Sr;
  static double Si;
  static double U;
  
  enum WhoMoves {
    WHOMOVES_P1P2,
    WHOMOVES_L1L2,
    WHOMOVES_P1L2,
    WHOMOVES_P2L1
  };  
  WhoMoves mWhoMoves;
  
public:   
  RefLine_P2L_P2L(RefMark* arm1, RefLine* arl1, RefMark* arm2, RefLine* arl2, short iroot);
  
  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


/**********
class RefLine_L2L_P2L - Huzita-Hatori Axiom O7 (Hatori's Axiom).
Bring line l1 to itself so that the point p1 goes on line l2.
**********/
class RefLine_L2L_P2L : public RefLine {
public:
  RefLine* rl1;       // bring line l1 onto itself
  RefMark* rm1;       // so that point p1
  RefLine* rl2;       // falls on line l2.
private:
  enum WhoMoves {
    WHOMOVES_P1,
    WHOMOVES_L1
  };  
  WhoMoves mWhoMoves;
  
public:
  RefLine_L2L_P2L(RefLine* arl1, RefMark* arm1, RefLine* arl2);
  
  bool UsesImmediate(RefBase* rb) const;
  void SequencePushSelf();
  bool PutHowto(std::ostream& os) const;
  void DrawSelf(RefStyle rstyle, short ipass) const;
  static void MakeAll(rank_t arank);
};


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
Section 3: container for collections of marks and lines and their construction
******************************************************************************/

/**********
class RefContainer - Container for marks and lines.
**********/
template<class R>
class RefContainer : public std::vector<R*> {
public:
  typedef std::map<typename R::key_t, R*> map_t;  // typedef for map holding R*
  std::vector<map_t> maps;      // Holds maps of objects, one for each rank
  std::size_t rcsz;         // current number of elements in the rank maps
  map_t buffer;           // used to accumulate new objects
  std::size_t rcbz;         // current size of buffer

  typedef typename map_t::iterator rank_iterator; // for iterating through individual ranks
  
public:
  std::size_t GetTotalSize() const {
    // Total number of elements, all ranks
    return rcsz + rcbz;
  };

  template <class Rs>
  void AddCopyIfValidAndUnique(const Rs& ars);  // add a copy of ars if valid and unique

private:
  friend class ReferenceFinder;   // only class that gets to use these methods

  RefContainer();           // Constructor

  void Rebuild();        // Re-initialize with new values
  bool Contains(const R* ar) const; // True if an equivalent element already exists
  void Add(R* ar);          // Add an element to the array
  void FlushBuffer();         // Add the contents of the buffer to the container
  void ClearMaps();         // Clear the map arrays when no longer needed
};


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
Section 4: routines for searching for marks and lines and collecting statistics
******************************************************************************/

/**********
class ReferenceFinder - class that builds and maintains collections of marks
and lines and can search throught the collection for marks and lines close to a
target mark or line.
**********/
class ReferenceFinder {
public:
  typedef RefBase::rank_t rank_t;   // we use ranks, too
  typedef RefBase::key_t key_t;     // and keys

  // Publicly accessible settings. Users can set these directly before calling
  // MakeAllMarksAndLines().
  static Paper sPaper;          // dimensions of the paper

  static bool sUseRefLine_C2P_C2P;
  static bool sUseRefLine_P2P;
  static bool sUseRefLine_L2L;
  static bool sUseRefLine_L2L_C2P;
  static bool sUseRefLine_P2L_C2P;
  static bool sUseRefLine_P2L_P2L;
  static bool sUseRefLine_L2L_P2L;
  
  static rank_t sMaxRank;         // maximum rank to create
  static std::size_t sMaxLines;   // maximum number of lines to create
  static std::size_t sMaxMarks;   // maximum number of marks to create
  
  static key_t sNumX;
  static key_t sNumY;
  static key_t sNumA;
  static key_t sNumD;
  
  static double sGoodEnoughError; // tolerable error in a mark or line
  static double sMinAspectRatio;  // minimum aspect ratio for a triangular flap
  static double sMinAngleSine;    // minimum line intersection for defining a mark
  static bool sVisibilityMatters; // restrict to what can be made w/ opaque paper
  static bool sLineWorstCaseError;// true = use worst-case error vs Pythagorean
  static int sDatabaseStatusSkip;       // frequency that sDatabaseFn gets called
  
  static bool sClarifyVerbalAmbiguities;
  static bool sAxiomsInVerbalDirections;

  static int sNumBuckets;       // how many error buckets to use
  static double sBucketSize;    // size of each bucket
  static int sNumTrials;        // number of test cases total
  static std::string sStatistics;  // Results of statistical analysis

  // Getters
  static std::size_t GetNumLines() {
    return sBasisLines.GetTotalSize();
  };
  static std::size_t GetNumMarks() {
    return sBasisMarks.GetTotalSize();
  };
  
  // Check key sizes against type size
  static bool LineKeySizeOK() {
    return sNumA < std::numeric_limits<key_t>::max() / sNumD;
  };
  static bool MarkKeySizeOK() {
  return sNumX < std::numeric_limits<key_t>::max() / sNumY;
  };
  
  // Support for a callback function to show progress during initialization
  enum DatabaseStatus {
    DATABASE_EMPTY,
    DATABASE_INITIALIZING, 
    DATABASE_WORKING, 
    DATABASE_RANK_COMPLETE, 
    DATABASE_READY
  };
  struct DatabaseInfo {
    DatabaseStatus mStatus;
    rank_t mRank;
    std::size_t mNumLines;
    std::size_t mNumMarks;
    
    DatabaseInfo(DatabaseStatus status = DATABASE_EMPTY, rank_t rank = 0, 
      std::size_t numLines = 0, std::size_t numMarks = 0) :
      mStatus(status), mRank(rank), mNumLines(numLines), 
      mNumMarks(numMarks) {
    };
    bool operator==(const DatabaseInfo& info) const {
      return mStatus == info.mStatus && mRank == info.mRank && 
        mNumLines == info.mNumLines && mNumMarks == info.mNumMarks;
    };
    bool operator !=(const DatabaseInfo& info) const {
      return !operator==(info);
    };
  };
  typedef void (*DatabaseFn)(DatabaseInfo info, void* userData, bool& cancel);
  static void SetDatabaseFn(DatabaseFn databaseFn, void* userData = 0) {
    sDatabaseFn = databaseFn;
    sDatabaseUserData = userData;
  };

  // Complete reinitialization of the database
  static void MakeAllMarksAndLines();

  // Functions for searching for the best marks and/or lines
  static void FindBestMarks(const XYPt& ap, std::vector<RefMark*>& vm, 
    short numMarks);
  static void FindBestLines(const XYLine& al, std::vector<RefLine*>& vl, 
    short numLines);

  // Utility routines for validating user input
  static bool ValidateMark(const XYPt& ap, std::string& err);
  static bool ValidateLine(const XYPt& ap1, const XYPt& ap2, std::string& err);
  
  // Support for a callback function to show progress during statistics
  enum StatisticsStatus {
    STATISTICS_BEGIN,
    STATISTICS_WORKING,
    STATISTICS_DONE
  };
  struct StatisticsInfo {
    StatisticsStatus mStatus;
    std::size_t mIndex;
    double mError;
    
    StatisticsInfo(StatisticsStatus status = STATISTICS_DONE,
      std::size_t index = std::size_t(-1), double error = 0.0) :
      mStatus(status), mIndex(index), mError(error) {
    };
    bool operator==(const StatisticsInfo& info) const {
      return mStatus == info.mStatus && mIndex == info.mIndex && 
        mError == info.mError;
    };
    bool operator!=(const StatisticsInfo& info) const {
      return !operator==(info);
    };
  };
  typedef void (*StatisticsFn)(StatisticsInfo info, void* userData, bool& cancel);
  static void SetStatisticsFn(StatisticsFn statisticsFn, void* userData = 0) {
    sStatisticsFn = statisticsFn;
    sStatisticsUserData = userData;
  };

  // Routine for calculating statistics on marks for a random set of trial points
  static void CalcStatistics();

  // An example that tests axiom O6.
  static void MesserCubeRoot(std::ostream& os);

private:
  static RefContainer<RefLine> sBasisLines;  // all lines
  static RefContainer<RefMark> sBasisMarks;  // all marks

  class EXC_HALT {};          // exception for user cancellation
  static rank_t sCurRank;       // the rank that we're currently working on
  static DatabaseFn sDatabaseFn;    // the show-status function callback
  static void* sDatabaseUserData;       // ptr to user data in callback
  static int sStatusCount;      // number of attempts since last callback
  static StatisticsFn sStatisticsFn;
  static void* sStatisticsUserData;
  
  static void CheckDatabaseStatus();    // called by RefContainer<>
  static void MakeAllMarksAndLinesOfRank(rank_t arank);
  
  // You should never create an instance of this class
  ReferenceFinder();
  ReferenceFinder(const ReferenceFinder&);

  friend class RefBase;
  friend class RefMark;
  friend class RefMark_Intersection;
  friend class RefLine;
  friend class RefLine_C2P_C2P;
  friend class RefLine_P2P;
  friend class RefLine_L2L;
  friend class RefLine_L2L_C2P;
  friend class RefLine_P2L_C2P;
  friend class RefLine_P2L_P2L;
  friend class RefLine_L2L_P2L;
  
//  friend class PSStreamDgmr;  // TBD, does this need to be a friend?
  friend class RefContainer<RefLine>;
  friend class RefContainer<RefMark>;
};


/**********
class CompareError - function object for comparing two refs of class R
according to their distance from a target value of class R::bare_t.
**********/
template <class R>
class CompareError {
public:
  typename R::bare_t mTarget;
  CompareError(const typename R::bare_t& target) : mTarget(target) {};
  bool operator()(R* r1, R* r2) const {
    // Compare the distances from the stored target If the distances are
    // equal, then compare the refs by their rank.
    double d1 = r1->DistanceTo(mTarget);
    double d2 = r2->DistanceTo(mTarget);
    if (d1 == d2) return r1->mRank < r2->mRank; 
    else return d1 < d2;
  };
};


/**********
class CompareRankAndError - function object for comparing two refs of class R
according to their distance from a target value of class R::bare_t, but for
close points, letting rank win out
**********/
template <class R>
class CompareRankAndError {
public:
  typename R::bare_t mTarget; // point that we're comparing to
  CompareRankAndError(const typename R::bare_t& target) : mTarget(target) {};
  bool operator()(R* r1, R* r2) const {
    // Compare the distances from the stored target. If both distances are less
    // than or equal to sGoodEnoughError, compare the refs by their rank.
    double d1 = r1->DistanceTo(mTarget);
    double d2 = r2->DistanceTo(mTarget);
    if ((d1 > ReferenceFinder::sGoodEnoughError) || 
      (d2 > ReferenceFinder::sGoodEnoughError)) {
      if (d1 == d2) return r1->mRank < r2->mRank; 
      else return d1 < d2;
    }
    else {
      if (r1->mRank == r2->mRank) return d1 < d2;
      else return r1->mRank < r2->mRank;
    }
  };
};


#ifdef __MWERKS__
#pragma mark -
#endif


/******************************************************************************
* Section 5: Routines for drawing diagrams
******************************************************************************/

/**********
class RefDgmr - object that draws folding diagrams of references. Subclasses
specialize to particular drawing environments (print vs screen, multiple GUIs,
platform-specific drawing models, etc.
**********/
class RefDgmr {
public:
  RefDgmr() {};
  virtual ~RefDgmr() {};

  // Subclasses must override these methods to implement
  enum PointStyle {
    POINTSTYLE_NORMAL, 
    POINTSTYLE_HILITE, 
    POINTSTYLE_ACTION
  };
  virtual void DrawPt(const XYPt& aPt, PointStyle pstyle);

  enum LineStyle {
    LINESTYLE_CREASE, 
    LINESTYLE_EDGE, 
    LINESTYLE_HILITE, 
    LINESTYLE_VALLEY, 
    LINESTYLE_MOUNTAIN, 
    LINESTYLE_ARROW
  };
  virtual void DrawLine(const XYPt& fromPt, const XYPt& toPt, LineStyle lstyle);
  virtual void DrawArc(const XYPt& ctr, double rad, double fromAngle,
    double toAngle, bool ccw, LineStyle lstyle);
  
  enum PolyStyle {
    POLYSTYLE_WHITE, 
    POLYSTYLE_COLORED, 
    POLYSTYLE_ARROW
  };
  virtual void DrawPoly(const std::vector<XYPt>& poly, PolyStyle pstyle);
  
  enum LabelStyle {
    LABELSTYLE_NORMAL, 
    LABELSTYLE_HILITE, 
    LABELSTYLE_ACTION
  };
  virtual void DrawLabel(const XYPt& aPt, const std::string& aString, 
    LabelStyle lstyle);
  
  // Subclasses may use or override these methods
  virtual void CalcArrow(const XYPt& fromPt, const XYPt& toPt,
    XYPt& ctr, double& rad, double& fromAngle, double& toAngle, bool& ccw,
    double& ahSize, XYPt& fromDir, XYPt& toDir);
  virtual void DrawValleyArrowhead(const XYPt& loc, const XYPt& dir, 
    double len);
  virtual void DrawMountainArrowhead(const XYPt& loc, const XYPt& dir, 
    double len);
  virtual void DrawUnfoldArrowhead(const XYPt& loc, const XYPt& dir, 
    double len);
  virtual void DrawValleyArrow(const XYPt& fromPt, const XYPt& toPt);
  virtual void DrawMountainArrow(const XYPt& fromPt, const XYPt& toPt);
  virtual void DrawUnfoldArrow(const XYPt& fromPt, const XYPt& toPt);
  virtual void DrawFoldAndUnfoldArrow(const XYPt& fromPt, const XYPt& toPt);
};


/**********
class VerbalStreamDgmr - a minimal subclass of RefDgmr that puts verbal-only
descriptions to a stream.
**********/
class VerbalStreamDgmr : public RefDgmr {
public:
  VerbalStreamDgmr(std::ostream& aStream);

  void PutMarkList(const XYPt& pp, std::vector<RefMark*>& vm);
  void PutLineList(const XYLine& ll, std::vector<RefLine*>& vl);

private:
  std::ostream* mStream;
  template <class R>
    void PutRefList(const typename R::bare_t& ar, std::vector<R*>& vr);
};


/**********
class PSStreamDgmr - a subclass of RefDgmr that writes a PostScript stream of
diagrams.
**********/
class PSStreamDgmr : public RefDgmr {
public:
  static double sPSUnit;          // size in points of a unit square
  static const XYRect sPSPageSize;// printable area on the page in PS units

  PSStreamDgmr(std::ostream& os);

  // Write to stream
  void PutMarkList(const XYPt& pp, std::vector<RefMark*>& vm);
  void PutLineList(const XYLine& ll, std::vector<RefLine*>& vl);

private:
  std::ostream* mStream;
  XYPt mPSOrigin;          // current loc of the origin in PS units
  int mPSPageCount;          // for page breaks
    
  class PSPt {
    public:
      double px;
      double py;
      PSPt(double x, double y) : px(x), py(y) {};
  };
  friend std::ostream& operator<<(std::ostream& os, const PSPt& pp);
  
  PSPt ToPS(const XYPt& pt);

  // Overridden functions from ancestor class RefDgmr
  void DrawPt(const XYPt& aPt, PointStyle pstyle);
  void DrawLine(const XYPt& fromPt, const XYPt& toPt, LineStyle lstyle);
  void DrawArc(const XYPt& ctr, double rad, double fromAngle,
    double toAngle, bool ccw, LineStyle lstyle);
  void DrawPoly(const std::vector<XYPt>& poly, PolyStyle pstyle);
  void DrawLabel(const XYPt& aPt, const std::string& aString, LabelStyle lstyle);
  
  // PSStreamDgmr - specific stuff
  void SetPointStyle(PointStyle pstyle);
  void SetLineStyle(LineStyle lstyle);
  void SetPolyStyle(PolyStyle pstyle);
  void SetLabelStyle(LabelStyle lstyle);

  void DecrementOrigin(double d);
  template <class R>
  void PutRefList(const typename R::bare_t& ar, std::vector<R*>& vr);
};

#endif // _REFERENCEFINDER_H_
