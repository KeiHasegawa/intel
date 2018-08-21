#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR
#include "intel.h"
#include <cmath>

#ifndef _MSC_VER
#define __int64 long long int
#endif // _MSC_VER

#ifdef _MSC_VER
#define I0x8000000000000000LL 0x8000000000000000
#define I0x7fffffff00000000LL 0x7fffffff00000000
#define I0x1fffffffffffffLL 0x1fffffffffffff
#define I0x100000000LL 0x100000000
#else // _MSC_VER
#define I0x8000000000000000LL 0x8000000000000000LL
#define I0x7fffffff00000000LL 0x7fffffff00000000LL
#define I0x1fffffffffffffLL 0x1fffffffffffffLL
#define I0x100000000LL 0x100000000LL
#endif // _MSC_VER

namespace intel80 {
  const int nbytes = 10;
  const int q = 0x4000-1;
  const int p = 63;
  extern const unsigned char table[17][nbytes];
  using namespace std;
  extern pair<int,pair<__int64,__int64> > unpack(const unsigned char*);
  extern void pack(unsigned char*, int, pair<__int64,__int64>);
  extern void normalize(unsigned char*, int, pair<__int64,__int64>);
  extern void add(unsigned char*, const unsigned char*);
  extern void mul(unsigned char*, const unsigned char*);
  extern void div(unsigned char*, const unsigned char*);
  extern void dec(unsigned char*, const char*);
  extern void hex(unsigned char*, const char*);
  extern void bit(unsigned char*, const char*);
  unsigned char inf[nbytes] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x7f
  };
} // end of namespace intel80

const unsigned char intel80::table[17][nbytes] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x3f}, // 1
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x40}, // 2
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x40}, // 3
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x40}, // 4
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x40}, // 5
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x40}, // 6
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x40}, // 7
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x40}, // 8
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x40}, // 9
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x40}, // 10
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0x40}, // 11
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x40}, // 12
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x40}, // 13
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0x40}, // 14
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x40}, // 15
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x40}, // 16
};

std::pair<int,std::pair<__int64,__int64> >
intel80::unpack(const unsigned char* b)
{
  using namespace std;
  int e = ((b[9] & 0x7f) << 8) | b[8];
  unsigned int x = ((b[7] & 0x7f) << 24)
                 |  (b[6] << 16)
                 |  (b[5] << 8)
                 |   b[4];
  __int64 fh = x;
  unsigned int y = (b[3] << 24)
                 | (b[2] << 16)
                 | (b[1] << 8)
                 |  b[0];
  __int64 fl = y;
  if ( e || fh || fl )
    fh |= ((__int64)1 << (p-32));
  if ( b[9] & 0x80 )
    fh |= I0x8000000000000000LL;
  return make_pair(e,make_pair(fh,fl));
}

void intel80::pack(unsigned char* res, int e, std::pair<__int64,__int64> f)
{
  using namespace std;
  fill(&res[0], &res[nbytes], 0);
  __int64 fh = f.first;
  if ( fh & I0x8000000000000000LL ){
    res[9] = 0x80;
    fh &= ~I0x8000000000000000LL;
  }
  res[9] |= e >> 8;
  res[8] = e;
  res[7] = fh >> 24;
  res[6] = fh >> 16;
  res[5] = fh >> 8;
  res[4] = fh;
  __int64 fl = f.second;
  res[3] = fl >> 24;
  res[2] = fl >> 16;
  res[1] = fl >> 8;
  res[0] = fl;
}

void
intel80::normalize(unsigned char* res, int e, std::pair<__int64,__int64> f)
{
  using namespace std;
  __int64& fh = f.first;
  __int64& fl = f.second;
  // N1: Test f
  if ( fh & I0x7fffffff00000000LL )
    goto N4;
  if ( !(fh & I0x1fffffffffffffLL) && !fl ){
    e = fh ? 1 : 0;
    goto N7;
  }
N2: // Is f normmalized?
  if ( fh & ((__int64)1 << (p-32)) )
    goto N5;
  // N3 : Scale left
  if ( e > 1 ){
    if ( fh & I0x8000000000000000LL ){
      fh &= ~I0x8000000000000000LL;
      fh <<= 1;
      if ( fl & 0x80000000 )
        fh |= 1;
      fl <<= 1;
      fl &= 0xffffffff;
      fh |= I0x8000000000000000LL;
    }
    else {
      fh <<= 1;
      if ( fl & 0x80000000 )
        fh |= 1;
      fl <<= 1;
      fl &= 0xffffffff;
    }
    --e;
    goto N2;
  }
  else {
    e = 0;
    goto N7;
  }
N4: // Scale right
  if ( fh & I0x8000000000000000LL ){
    fh &= ~I0x8000000000000000LL;
    int carry = fh & 1;
    fh >>= 1;
    fl >>= 1;
    if ( carry )
      fl |= 0x80000000;
    fh |= I0x8000000000000000LL;
  }
  else {
    int carry = fh & 1;
    fh >>= 1;
    fl >>= 1;
    if ( carry )
      fl |= 0x80000000;
  }
  ++e;
N5: // Round
  // N6: Check e
N7: // Pack
  pack(res,e,f);
}

void intel80::add(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  pair<int,pair<__int64, __int64> > U = unpack(u);
  int eu = U.first;
  pair<__int64,__int64> fu = U.second;
  pair<int,pair<__int64, __int64> > V = unpack(v);
  int ev = V.first;
  pair<__int64, __int64> fv = V.second;
  if ( eu < ev ){
    swap(eu,ev);
    swap(fu,fv);
  }
  int ew = eu;
  pair<__int64, __int64> fw;
  if ( eu - ev >= p + 1 ){  // original code was `if ( eu - ev >= p + 2){'
    fw = fu;
    normalize(u,ew,fw);
  }
  else {
    bool su = fu.first & I0x8000000000000000LL ? true : false;
    if ( su )
      fu.first &= ~I0x8000000000000000LL;
    bool sv = fv.first & I0x8000000000000000LL ? true : false;
    if ( sv )
      fv.first &= ~I0x8000000000000000LL;
    if ( eu - ev == p + 1 && (int)fu.first == 0xffffffff && (int)fu.second == 0xffffffff ){
      fv.first = 0;
      fv.second = 1;
    }
    else {
      unsigned __int64 tmp = fv.first << 32 | fv.second;
      tmp >>= eu - ev;
      fv.first = tmp >> 32;
      fv.second = tmp & 0xffffffff;
    }
    if ( su == sv ){
      fw.first = fu.first + fv.first;
      fw.second = fu.second + fv.second;
      if ( fw.second & I0x100000000LL ){
        fw.second &= ~I0x100000000LL;
        ++fw.first;
      }
    }
    else {
      fw.first = fu.first - fv.first;
      fw.second = fu.second - fv.second;
      if ( fw.second & I0x8000000000000000LL ){
        fw.second &= ~I0x8000000000000000LL;
        --fw.first;
      }
    }
    if ( su && sv )
      fw.first |= I0x8000000000000000LL;
    else if ( su && !sv && fw.first > 0 )
      fw.first |= I0x8000000000000000LL;
    else if ( !su && sv && fw.first < 0 )
      fw.first |= I0x8000000000000000LL;
    normalize(u,ew,fw);
  }
}

namespace intel80 { namespace muldiv_impl {
  using namespace std;
  int help(pair<__int64,__int64>*, int*);
} } // end of namespace mul_impl and intel80

int intel80::muldiv_impl::help(std::pair<__int64,__int64>* f, int* p)
{
  int n = 0;
  while ( !(f->second & 1) && *p ){
    if ( f->first & 1 ){
      f->first >>= 1;
      f->second >>= 1;
      f->second |= 0x80000000;
    }
    else {
      f->first >>= 1;
      f->second >>= 1;
    }
    ++n;
    if ( !--*p )
      break;
  }
  return n;
}

void intel80::mul(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  pair<int,pair<__int64,__int64> > U = unpack(u);
  int eu = U.first;
  pair<__int64,__int64> fu = U.second;
  pair<int,pair<__int64,__int64> > V = unpack(v);
  int ev = V.first;
  pair<__int64,__int64> fv = V.second;
  int ew = eu + ev - q;
  int pp = p;
  muldiv_impl::help(&fu,&pp);
  muldiv_impl::help(&fv,&pp);
  pair<__int64,__int64> fw;
  fw.first = (fu.first * fv.second + fu.second * fv.first) >> pp;
  fw.second = fu.second * fv.second >> pp;
  fw.first += fw.second >> 32;
  fw.second &= ~(~(__int64)0 << 32);
  normalize(u,ew,fw);
}

void intel80::div(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  pair<int,pair<__int64,__int64> > U = unpack(u);
  int eu = U.first;
  pair<__int64,__int64> fu = U.second;
  pair<int,pair<__int64,__int64> > V = unpack(v);
  int ev = V.first;
  pair<__int64,__int64> fv = V.second;
  int ew = eu - ev + q + 1;
  int pp = p - 1;
  muldiv_impl::help(&fv,&pp);
  pair<__int64,__int64> fw;
  if ( fv.second ){
    if ( ew < 1 )
      fu.first &= ~((__int64)1 << (p-32));
    if ( fu.first < fv.second ){
      fu.second += I0x100000000LL * (fv.second - fu.first);
      fu.first = 0;
    }
    fw.first  = fu.first / fv.second  << pp;
    fw.second = fu.second / fv.second << pp;
    normalize(u,ew,fw);
  }
  else {
    memcpy(u,inf,nbytes);
    if ( fu.first & I0x8000000000000000LL )
      u[9] |= 0x80;
  }
}

void intel80::dec(unsigned char* res, const char* s)
{
  using namespace std;
  fill(&res[0], &res[nbytes], 0);
  while ( isdigit(*s) ){
    mul(res,table[10]);
    add(res,table[*s - '0']);
    ++s;
  }
  if ( *s == '.' )
    ++s;
  unsigned char tmp[nbytes];
  memcpy(&tmp[0], table[0], nbytes);
  int n = 0;
  while ( isdigit(*s) ){
    mul(tmp,table[10]);
    add(tmp,table[*s - '0']);
    ++s;
    ++n;
  }
  while ( n-- )
    div(tmp,table[10]);
  add(res,tmp);
  int sign = 1;
  if ( *s == 'e' || *s == 'E' ){
    ++s;
    if ( *s == '+' )
      ++s;
    else if ( *s == '-' )
      sign = -1, ++s;
  }
  int gamma = 0;
  while ( isdigit(*s) ){
    gamma *= 10;
    gamma += *s - '0';
    ++s;
  }
  while ( gamma-- )
    sign > 0 ? mul(res,table[10]) : div(res,table[10]);
}

void intel80::hex(unsigned char* res, const char* s)
{
  using namespace std;
  fill(&res[0], &res[nbytes], 0);
  assert(*s == '0');
  ++s;
  assert(*s == 'x' || *s == 'X');
  ++s;
  while ( *s != '.' && *s != 'p' && *s != 'P' ){
    assert(isxdigit(*s));
    mul(res,table[16]);
    int a;
    if ( isdigit(*s) )
      a = *s - '0';
    else if ( isupper(*s) )
      a = *s - 'A' + 10;
    else
      a = *s - 'a' + 10;
    add(res,table[a]);
    ++s;
  }
  if ( *s == '.' )
    ++s;
  unsigned char tmp[nbytes];
  memcpy(&tmp[0], table[0], nbytes);
  int n = 0;
  while ( *s && *s != 'p' && *s != 'P' ){
    assert(isxdigit(*s));
    mul(tmp,table[16]);
    int a;
    if ( isdigit(*s) )
      a = *s - '0';
    else if ( isupper(*s) )
      a = *s - 'A' + 10;
    else
      a = *s - 'a' + 10;
    add(tmp,table[a]);
    ++s;
    ++n;
  }
  while ( n-- )
    div(tmp,table[16]);
  add(res, tmp);
  int sign = 1;
  if ( *s == 'p' || *s == 'P' ){
    ++s;
    if ( *s == '+' )
      ++s;
    else if ( *s == '-' )
      sign = -1, ++s;
  }
  int gamma = 0;
  while ( isdigit(*s) ){
    gamma *= 10;
    gamma += *s - '0';
    ++s;
  }
  while ( gamma-- )
    sign > 0 ? mul(res,table[2]) : div(res,table[2]);
}

void intel80::bit(unsigned char* res, const char* s)
{
  s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ? hex(res,s) : dec(res,s);
}

void
intel::literal::floating::long_double::bit(unsigned char* res, const char* s)
{
  using namespace std;
    fill(&res[intel80::nbytes], &res[long_double::size], 0);
    intel80::bit(res,s);
}

void
intel::literal::floating::long_double::add(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  fill(&u[intel80::nbytes], &u[long_double::size], 0);
  intel80::add(u,v);
}

void
intel::literal::floating::long_double::sub(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  using namespace intel80;
  unsigned char minus_v[nbytes];
  memcpy(&minus_v[0], v, nbytes);
  minus_v[nbytes-1] ^= 0x80;
  intel80::add(u,minus_v);
}

void
intel::literal::floating::long_double::mul(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  fill(&u[intel80::nbytes], &u[long_double::size], 0);
  intel80::mul(u,v);
}

void
intel::literal::floating::long_double::div(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  fill(&u[intel80::nbytes], &u[long_double::size], 0);
  intel80::div(u,v);
}

void
intel::literal::floating::long_double::neg(unsigned char* u, const unsigned char* v)
{
  using namespace std;
  fill(&u[intel80::nbytes], &u[long_double::size], 0);
  memcpy(&u[0], v, intel80::nbytes);
  u[intel80::nbytes-1] ^= 0x80;
}

bool intel::literal::floating::long_double::zero(const unsigned char* u)
{
  using namespace std;
  using namespace intel80;
  if ( u[nbytes-1] == 0x00 || u[nbytes-1] == 0x80 )
    return find_if(&u[0],&u[nbytes-1],bind2nd(not_equal_to<int>(),0)) == &u[nbytes-1];
  else
    return false;
}

double
intel::literal::floating::long_double::to_double(const unsigned char* u)
{
  using namespace std;
  pair<int,pair<__int64,__int64> > p = intel80::unpack(u);
  int e = p.first;
  __int64 hi = p.second.first;
  bool neg = false;
  if ( hi & I0x8000000000000000LL ){
    hi &= ~I0x8000000000000000LL;
    neg = true;
  }
  double ff = hi;
  if ( neg )
    ff = -ff;
  ff *= (__int64)1 << 32;
  ff += p.second.second;
  ff /= pow(2.0,(double)intel80::p);
  return ff *= (__int64)1 << (e - intel80::q);
}

void
intel::literal::floating::long_double::from_double(unsigned char* res, double d)
{
  using namespace std;
  unsigned char b[8];
  memcpy(b,&d,8);
  int e = ((b[7] & 0x7f) << 4) | ((b[6] & 0xf0) >> 4);
  unsigned __int64 f = (unsigned __int64)(b[6] & 0x0f) << 48;
  f |= (unsigned __int64)b[5] << 40;
  f |= (unsigned __int64)b[4] << 32;
  f |= (unsigned __int64)b[3] << 24;
  f |= b[2] << 16;
  f |= b[1] << 8;
  f |= b[0] << 0;
  __int64 fh =  f >> 21;
  __int64 fl = (f << (e ? 11 : 12)) & 0xffffffff;
  if ( e || fh  )
    fh |= ((__int64)1 << (intel80::p-32));
  e -= 0x400 - 1;
  e += intel80::q;
  if ( b[7] & 0x80 )
    fh |= I0x8000000000000000LL;
  intel80::normalize(res,e,make_pair(fh,fl));
}

bool intel::literal::floating::long_double::cmp(COMPILER::goto3ac::op op,
                                                const unsigned char* u,
                                                const unsigned char* v)
{
  unsigned char tmp[intel80::nbytes];
  memcpy(tmp,u,intel80::nbytes);
  sub(tmp,v);
  if ( zero(tmp) )
    return op == COMPILER::goto3ac::LE || op == COMPILER::goto3ac::GE || op == COMPILER::goto3ac::EQ;
  if ( tmp[9] & 0x80 )
    return op == COMPILER::goto3ac::LT || op == COMPILER::goto3ac::LE || op == COMPILER::goto3ac::NE;
  else
    return op == COMPILER::goto3ac::GT || op == COMPILER::goto3ac::GE || op == COMPILER::goto3ac::NE;
}

int intel::literal::floating::long_double::size = 16;
