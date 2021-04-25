#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "intel.h"

extern "C" DLL_EXPORT int generator_seed()
{
#ifdef _MSC_VER
  int r = _MSC_VER;
#ifndef CXX_GENERATOR
  r += 10000000;
#else // CXX_GENERATOR
  r += 20000000;
#endif // CXX_GENERATOR
#ifdef _DEBUG
  r +=  1000000;
#endif // _DEBUG
#ifdef WIN32
  r +=   100000;
#endif // WIN32
#endif // _MSC_VER
#ifdef __GNUC__
  int r = (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__);
#ifndef CXX_GENERATOR
  r += 30000000;
#else // CXX_GENERATOR
  r += 40000000;
#endif // CXX_GENERATOR
#endif // __GNUC__
  return r;
}

namespace intel {
  std::string m_generator;
  int option_handler(const char*);
} // end of namespace intel

extern "C" DLL_EXPORT
void generator_option(int argc, const char** argv, int* error)
{
  using namespace std;
  using namespace intel;
  m_generator = *argv;
  ++argv;
  --argc;
  int* q = &error[0];
  for (const char** p = &argv[0]; p != &argv[argc]; ++p)
    *q++ = option_handler(*p);
}

bool intel::debug_flag;

bool intel::x64 = true;

namespace intel {
  namespace option_impl {
    inline void add_()
    {
#ifdef CXX_GENERATOR
      except::ms::pre1a = "_" + except::ms::pre1a;
      except::ms::pre1b = "_" + except::ms::pre1b;
      except::ms::pre2a = "_" + except::ms::pre2a;
      except::ms::pre2b = "_" + except::ms::pre2b;
      except::ms::pre3 = "_" + except::ms::pre3;
      except::ms::vpsig = "PAX";
#endif // CXX_GENERATOR
    }
  }
} // end of namespace intel

int intel::option_handler(const char* option)
{
  using namespace std;
  if (string("--version") == option) {
    cerr << m_generator << ": version " << "1.4" << '\n';
    return 0;
  }
  if (string("--debug") == option) {
    debug_flag = true;
    return 0;
  }
  if (string("--x86") == option) {
    x64 = false;
    intel::literal::floating::long_double::size = (mode == MS) ? 8 : 12;
    if (mode == MS) {
      external_header = "_";
      option_impl::add_();
    }
    intel::first_param_offset = (mode == MS) ? 12 : 8;
    return 0;
  }
  if (string("--ms") == option) {
    mode = MS;
    pseudo_global = "PUBLIC";
    comment_start = ";";
    intel::literal::floating::long_double::size = 8;
    if (!x64) {
      intel::first_param_offset = 12;
      external_header = "_";
      option_impl::add_();
    }
    return 0;
  }
  cerr << "unknown option " << option << '\n';
  return 1;
}

std::string intel::external_header;

std::ostream intel::out(std::cout.rdbuf());

namespace intel {
  std::ofstream* ptr_out;
} // end of namespace intel

extern "C" DLL_EXPORT int generator_open_file(const char* fn)
{
  using namespace std;
  using namespace intel;
  ptr_out = new ofstream(fn);
  intel::out.rdbuf(ptr_out->rdbuf());
  if (mode == MS) {
    intel::out << "include listing.inc" << '\n';
    if (!x64)
      intel::out << '\t' << ".model" << '\t' << "flat" << '\n';
  }
  return 0;
}

namespace intel {
  void (*output3ac)(std::ostream&, COMPILER::tac*);
} // end of namespace intel

extern "C" DLL_EXPORT void generator_spell(void* arg)
{
  using namespace std;
  using namespace COMPILER;
  using namespace intel;
  void* magic[] = {
    ((char **)arg)[0],
  };
  int index = 0;
  memcpy(&output3ac, &magic[index++], sizeof magic[0]);
}

extern "C" DLL_EXPORT
void generator_generate(const COMPILER::generator::interface_t* ptr)
{
  using namespace std;
  using namespace intel;
  genobj(ptr->m_root);
  if (ptr->m_func)
    genfunc(ptr->m_func, *ptr->m_code);
  uint64_float_t::obj.output();
  uint64_double_t::obj.output();
  uint64_ld_t::obj.output();
  ld_uint64_t::obj.output();
  uminus_float_t::obj.output();
  uminus_double_t::obj.output();
  real_uint64_t::obj.output();
  float_uint64_t::obj.output();
  double_uint64_t::obj.output();
}

void intel::reference_constant::output()
{
  if (!m_label.empty()) {
    if (!m_out) {
      sec_hlp sentry(ROMDATA);
      output_value();
      m_out = true;
    }
  }
}

extern "C" DLL_EXPORT int generator_sizeof(int n)
{
  using namespace COMPILER;
  using namespace intel;

  switch ((type::id_t)n) {
  case type::SHORT:
    return 2;
  case type::INT:
    return 4;
  case type::LONG:
    return mode == MS ? 4 : (x64 ? 8 : 4);
  case type::LONGLONG:
    return 8;
  case type::FLOAT:
    return 4;
  case type::DOUBLE:
    return 8;
  case type::LONG_DOUBLE:
    return literal::floating::long_double::size;
#ifdef CXX_GENERATOR
  case type::FLOAT128:
    return 64;
#endif // CXX_GENERATOR
  default:
    assert((type::id_t)n == type::POINTER);
    return x64 ? 8 : 4;
  }
}

#ifdef linux
extern "C" DLL_EXPORT
int generator_ptrdiff_type()
{
  using namespace COMPILER;
  return type::INT;
}
#endif // linux

#if defined(__x86_64__) || !defined(WIN32)
extern "C" DLL_EXPORT
int generator_ptrdiff_type()
{
  using namespace COMPILER;
  return intel::mode == intel::GNU ? type::LONG : type::LONGLONG;
}
#endif // defined(__x86_64__) || !defined(WIN32)

extern "C" DLL_EXPORT
int generator_wchar_type()
{
  using namespace COMPILER;
#ifdef linux
#ifdef DEBIAN 
  return (int)type::INT;
#else // DEBIAN
  return (int)type::LONG;
#endif /// DEBIAN
#endif // linux
#ifdef __APPLE__
  return (int)type::INT;
#endif // __APPLE__
  return (int)type::USHORT;
}

#ifdef _MSC_VER
extern "C" DLL_EXPORT void* generator_long_double()
{
  if (intel::mode == intel::mode_t::MS)
    return 0;
  static COMPILER::generator::long_double_t ld = {
    &intel::literal::floating::long_double::bit,
    &intel::literal::floating::long_double::add,
    &intel::literal::floating::long_double::sub,
    &intel::literal::floating::long_double::mul,
    &intel::literal::floating::long_double::div,
    &intel::literal::floating::long_double::neg,
    &intel::literal::floating::long_double::zero,
    &intel::literal::floating::long_double::to_double,
    &intel::literal::floating::long_double::from_double,
    &intel::literal::floating::long_double::cmp,
  };
  return &ld;
}
#endif // _MSC_VER

extern "C" DLL_EXPORT int generator_close_file()
{
  using namespace std;
  using namespace COMPILER;
  using namespace intel;

#ifdef CXX_GENERATOR
  init_term_fun();
#if defined(_MSC_VER) || defined(__CYGWIN__)
  for (auto T : except::types_to_output)
    except::out_type_info(T);
  except::out_labeled_types();
  for (auto T : except::ms::call_sites_types_to_output)
      except::ms::out_call_site_type_info(T);
#else // defined(_MSC_VER) || defined(__CYGWIN__)
  except::out_frame();
  for (auto T : except::types_to_output)
    except::out_type_info(T);
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
#endif // CXX_GENERATOR

  transform(mem::refed.begin(), mem::refed.end(), ostream_iterator<string>(out), mem::refgen);

  if (mode == MS)
    intel::out << "END" << '\n';

  delete ptr_out;
#ifdef _DEBUG
  map<usr*, address*>& table = address_descriptor.first;
  for_each(table.begin(), table.end(), destroy_address<usr*>());
#endif // _DEBUG
  return 0;
}

namespace intel {
  namespace doll_need_impl {
    struct table_t : std::set<std::string> {
      table_t();
    } table;
  }  // end of namespace doll_need_impl
}  // end of namespace intel

bool intel::doll_need(std::string label)
{
  if (mode == GNU)
    return false;
  return doll_need_impl::table.find(label) != doll_need_impl::table.end();
}

intel::doll_need_impl::table_t::table_t()
{
  using namespace std;

  insert("rax"); insert("eax"); insert("ax"); insert("ah"); insert("al");
  insert("RAX"); insert("EAX"); insert("AX"); insert("AH"); insert("AL");
  insert("rbx"); insert("ebx"); insert("bx"); insert("bh"); insert("bl");
  insert("RBX"); insert("EBX"); insert("BX"); insert("BH"); insert("BL");
  insert("rcx"); insert("ecx"); insert("cx"); insert("ch"); insert("cl");
  insert("RCX"); insert("ECX"); insert("CX"); insert("CH"); insert("CL");
  insert("rdx"); insert("edx"); insert("dx"); insert("dh"); insert("dl");
  insert("RDX"); insert("EDX"); insert("DX"); insert("DH"); insert("DL");
  insert("rsp"); insert("esp");
  insert("RSP"); insert("ESP");
  insert("sp"); insert("SP");
  insert("rbp"); insert("ebp");
  insert("RBP"); insert("EBP");
  insert("bp"); insert("BP");
  insert("rip"); insert("eip");
  insert("RIP"); insert("EIP");
  insert("ip"); insert("IP");
  insert("rsi"); insert("esi"); insert("si");
  insert("RSI"); insert("ESI"); insert("SI");
  insert("rdi"); insert("edi"); insert("di");
  insert("RDI"); insert("EDI"); insert("DI");
  
  for (int i = 8; i <= 15; ++i) {
    {
      ostringstream os;
      os << 'r' << dec << i;
      string reg = os.str();
      insert(reg);
      insert(reg + 'd');
      insert(reg + 'w');
      insert(reg + 'b');
    }
    {
      ostringstream os;
      os << 'R' << dec << i;
      string reg = os.str();
      insert(reg);
      insert(reg + 'D');
      insert(reg + 'W');
      insert(reg + 'B');
    }
  }

  insert("ss"); insert("SS");
  insert("end"); insert("END");
  insert("out"); insert("OUT");
  insert("name"); insert("NAME");

  insert("push"); insert("PUSH");
  insert("pop"); insert("POP");
  insert("ptr"); insert("PTR");

  insert("label"); insert("LABEL");
}

namespace intel {
  using namespace std;
  using namespace COMPILER;
  pair<map<usr*, address*>, map<var*, address*> > address_descriptor;
} // end of namespace inel

intel::address* intel::getaddr(COMPILER::var* v)
{
  using namespace std;
  using namespace COMPILER;
  if (is_external_declaration(v)) {
    map<usr*, address*>& m = address_descriptor.first;
    usr* u = static_cast<usr*>(v);
    map<usr*, address*>::const_iterator p = m.find(u);
    assert(p != m.end());
    return p->second;
  }
  else {
    map<var*, address*>& m = address_descriptor.second;
    map<var*, address*>::const_iterator p = m.find(v);
    assert(p != m.end());
    return p->second;
  }
}

namespace intel {
  namespace reg {
    struct table : std::map<std::pair<gpr, int>, std::string> {
      table();
    } m_table;
  }
} // end of namespace reg and intel

std::string intel::reg::name(intel::reg::gpr r, int size)
{
  using namespace std;
  table::const_iterator p = m_table.find(make_pair(r, size));
  assert(p != m_table.end());
  string s = p->second;
  if (mode == MS)
    return s.substr(1);
  return s;
}

intel::reg::table::table()
{
  using namespace std;
  (*this)[make_pair(ax, 1)] = "%al";
  (*this)[make_pair(ax, 2)] = "%ax";
  (*this)[make_pair(ax, 4)] = "%eax";
  (*this)[make_pair(ax, 8)] = "%rax";
  (*this)[make_pair(bx, 1)] = "%bl";
  (*this)[make_pair(bx, 2)] = "%bx";
  (*this)[make_pair(bx, 4)] = "%ebx";
  (*this)[make_pair(bx, 8)] = "%rbx";
  (*this)[make_pair(cx, 1)] = "%cl";
  (*this)[make_pair(cx, 2)] = "%cx";
  (*this)[make_pair(cx, 4)] = "%ecx";
  (*this)[make_pair(cx, 8)] = "%rcx";
  (*this)[make_pair(dx, 1)] = "%dl";
  (*this)[make_pair(dx, 2)] = "%dx";
  (*this)[make_pair(dx, 4)] = "%edx";
  (*this)[make_pair(dx, 8)] = "%rdx";
  (*this)[make_pair(si, 2)] = "%si";
  (*this)[make_pair(si, 4)] = "%esi";
  (*this)[make_pair(si, 8)] = "%rsi";
  (*this)[make_pair(di, 2)] = "%di";
  (*this)[make_pair(di, 4)] = "%edi";
  (*this)[make_pair(di, 8)] = "%rdi";

  (*this)[make_pair(r8, 1)] = "%r8b";
  (*this)[make_pair(r8, 2)] = "%r8w";
  (*this)[make_pair(r8, 4)] = "%r8d";
  (*this)[make_pair(r8, 8)] = "%r8";
  (*this)[make_pair(r9, 1)] = "%r9b";
  (*this)[make_pair(r9, 2)] = "%r9w";
  (*this)[make_pair(r9, 4)] = "%r9d";
  (*this)[make_pair(r9, 8)] = "%r9";
}

intel::imm::imm(COMPILER::usr* u) : address(IMM), m_float(false)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = u->m_type;
  if (T->real()) {
    m_float = true;
    constant<float>* c = static_cast<constant<float>*>(u);
    union {
      float f;
      int i;
    };
    f = c->m_value;
    ostringstream os;
    if (mode == GNU)
      os << '$';
    os << i;
    m_expr = os.str();
  }
  else {
    ostringstream os;
    if (mode == GNU)
      os << '$';
    os << u->value();
    m_expr = os.str();
  }
}

void intel::imm::load() const
{
  using namespace COMPILER;
  if (m_float) {
    if (x64) {
      if (mode == GNU) {
        out << '\t' << "movl" << '\t' << m_expr << ", %eax" << '\n';
        out << '\t' << "movq" << '\t' << "%rax, %xmm0" << '\n';
      }
      else {
        out << '\t' << "mov" << '\t' << "eax, " << m_expr << '\n';
        out << '\t' << "movq" << '\t' << "xmm0, rax" << '\n';
      }
    }
    else {
      if (mode == GNU) {
        out << '\t' << "pushl" << '\t' << m_expr << '\n';
        out << '\t' << "flds" << '\t' << "(%esp)" << '\n';
        out << '\t' << "leal" << '\t' << "4(%esp), %esp" << '\n';
      }
      else {
        out << '\t' << "push" << '\t' << m_expr << '\n';
        out << '\t' << "fld" << '\t' << "DWORD PTR [esp]" << '\n';
        out << '\t' << "lea" << '\t' << "esp, 4[esp]" << '\n';
      }
    }
  }
  else
    load(reg::ax);
}

void intel::imm::load(reg::gpr r) const
{
  if (mode == GNU)
    out << '\t' << "movl" << '\t' << m_expr << ", " << reg::name(r, 4) << '\n';
  else
    out << '\t' << "mov"  << '\t' << reg::name(r, 4) << ", " << m_expr << '\n';
}

bool intel::imm::is(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  if (!u->isconstant())
    return false;
#ifdef CXX_GENERATOR
  usr::flag2_t flag2 = u->m_flag2;
  if (flag2 & usr::CONST_USR)
    return false;
#endif // CXX_GENERATOR
  const type* T = u->m_type;
  return T->size() <= 4;
}

bool intel::literal::floating::big(COMPILER::usr* u)
{
  using namespace COMPILER;
  const type* T = u->m_type;
  if (!T->real() || T->modifiable() || T->size() <= 4)
    return false;
  return u->isconstant();
}

namespace intel {
  namespace literal {
    namespace floating {
      namespace output_impl {
        using namespace COMPILER;
        double get(usr* u)
        {
          const type* T = u->m_type;
          T = T->unqualified();
          if (T->m_id == type::DOUBLE) {
            typedef double X;
            constant<X>* c = static_cast<constant<X>*>(u);
            return c->m_value;
          }
          else {
            assert(T->m_id == type::LONG_DOUBLE);
            assert(mode == MS);
            typedef long double X;
            constant<X>* c = static_cast<constant<X>*>(u);
            return c->m_value;
          }
        }
      } // end of namespace output_impl
    } // end of namespace floating
  } // end of namespace literal
} // end of namespace intel

void intel::literal::floating::output(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = u->m_type;
  int size = T->size();
  if (size == 4) {
    constant<float>* c = static_cast<constant<float>*>(u);
    union {
      float f;
      int i;
    } un = { c->m_value };
    out << '\t' << dot_long() << '\t' << dec << un.i << '\n';
  }
  else if (size == 8) {
    union {
      double d;
      int i[2];
    } un = { output_impl::get(u) };
    int n = 1;
    if (!*(char*)&n)
      swap(un.i[0], un.i[1]);
    out << '\t' << dot_long() << '\t' << dec << un.i[0] << '\n';
    out << '\t' << dot_long() << '\t' << dec << un.i[1] << '\n';
  }
  else {
    assert(size == floating::long_double::size);
    constant<long double>* c = static_cast<constant<long double>*>(u);
    union {
      long double ld;
      int i[4];
      unsigned char c[16];
    } un;
    un.i[3] = 0;
    if (sizeof(long double) == 8)
      memcpy(&un.c[0], &c->b[0], size);
    else
      un.ld = c->m_value;

    int n = 1;
    if (!*(char*)&n)
      swap(un.i[0], un.i[3]), swap(un.i[1],un.i[2]);

    out << '\t' << dot_long() << '\t' << dec << un.i[0] << '\n';
    out << '\t' << dot_long() << '\t' << dec << un.i[1] << '\n';
    out << '\t' << dot_long() << '\t' << dec << un.i[2] << '\n';
    if ( size == 16 )
      out << '\t' << dot_long() << '\t' << dec << un.i[3] << '\n';
  }
}

bool intel::literal::integer::big(COMPILER::usr* u)
{
  using namespace COMPILER;
  const type* T = u->m_type;
  return !T->modifiable() && T->integer() && T->size() > 4;
}

void intel::literal::integer::output(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  union {
    __int64 ll;
    int i[2];
  } tmp;
  constant<__int64>* c = static_cast<constant<__int64>*>(u);
  tmp.ll = c->m_value;
  int i = 1;
  if (!*(char*)&i)
    swap(tmp.i[0], tmp.i[1]);
  out << '\t' << dot_long() << '\t' << dec << tmp.i[0] << '\n';
  out << '\t' << dot_long() << '\t' << dec << tmp.i[1] << '\n';
}

char intel::suffix(int size)
{
  if (mode == MS)
    return ' ';
  if (size == 8)
    return 'q';
  if (size == 4)
    return 'l';
  if (size == 2)
    return 'w';
  assert(size == 1);
  return 'b';
}

char intel::fsuffix(int size)
{
  if (mode == MS)
    return ' ';
  switch (size) {
  case 4: return 's';
  case 8: return 'l';
  default:
    assert(size == literal::floating::long_double::size);
    return 't';
  }
}

void intel::mem::load() const
{
  using namespace std;
  using namespace COMPILER;
  const type* T = m_usr->m_type;
  assert(T->scalar());
  int size = T->size();
  usr::flag_t f = m_usr->m_flag;
  refed.insert(refgen_t(m_label,f,size));
  if (T->real()) {
    if (x64) {
      if (literal::floating::big(m_usr)) {
        char ps = psuffix();
        if (mode == GNU)
          out << '\t' << "mov" << ps << '\t' << m_label << "(%rip)," << '\t' << xmm(0) << '\n';
        else {
          out << '\t' << "movsd" << '\t' << xmm(0) << ", " << "QWORD PTR " << m_label << '\n';
        }
      }
      else {
        indirect_code(reg::ax);
        string rax = reg::name(reg::ax, 8);
        string ptr = (mode == MS) ? ms_pseudo(size) + " PTR " : "";
        switch (size) {
        case 4:
          if (mode == GNU)
            out << '\t' << "movss" << '\t' << '(' << rax << "), " << xmm(0) << '\n';
          else
            out << '\t' << "movss" << '\t' << xmm(0) << ", " << ptr << '[' << rax << ']' << '\n';
          break;
        case 8:
          if (mode == GNU)
            out << '\t' << "movsd" << '\t' << '(' << rax << "), " << xmm(0) << '\n';
          else
            out << '\t' << "movsd" << '\t' << xmm(0) << ", " << ptr <<  '[' << rax << ']' << '\n';
          break;
        case 16:
          {
            char ps = psuffix();
            if (mode == GNU)
              out << '\t' << "fldt" << '\t' << '(' << rax << ')' << '\n';
            else
              out << '\t' << "fld" << '\t' << ptr << '[' << rax << ']' << '\n';
            if (mode == GNU)
              out << '\t' << "sub" << ps << '\t' << "$8" << ", " << sp() << '\n';
            else
              out << '\t' << "sub" << '\t' << sp() << ", " << 8 << '\n';
            if (mode == GNU)
              out << '\t' << "fstpl" << '\t' << '(' << sp() << ')' << '\n';
            else
              out << '\t' << "fstp" << '\t' << ptr << '[' << sp() << ']' << '\n';
            if (mode == GNU)
              out << '\t' << "movsd" << '\t' << '(' << sp() << "), " << xmm(0) << '\n';
            else
              out << '\t' << "movsd" << '\t' << ptr << '[' << sp() << "], " << xmm(0) << '\n';
            if (mode == GNU)
              out << '\t' << "add" << ps << '\t' << "$8" << ", " << sp() << '\n';
            else
              out << '\t' << "add" << '\t' << sp() << ", " << 8 << '\n';
            break;
          }
        }
      }
    }
    else {
      if (mode == GNU)
        out << '\t' << "fld" << fsuffix(size) << '\t' << m_label << '\n';
      else {
        out << '\t' << "lea" << '\t' << "eax, " << m_label << '\n';
        out << '\t' << "fld" << '\t' << ms_pseudo(size) << " PTR [eax]" << '\n';
      }
    }
  }
  else
    load(reg::ax);
}

namespace intel {
  namespace mem_impl {
    void load_label_x64(reg::gpr r, std::string label, usr::flag_t f, int size)
    {
      using namespace std;
      string preg = reg::name(r, psize());
      char ps = psuffix();
      char sf = suffix(size);
      string ptr;
      if (mode == MS)
        ptr = ms_pseudo(size) + " PTR ";
      if (mode == GNU) {
        out << '\t' << "mov" << ps << '\t' << ".refptr." << label << "(%rip)," << '\t' << preg << '\n';
        mem::refed.insert(mem::refgen_t(label,f,size));
      }
      else {
        out << '\t' << "lea" << ps << '\t' << preg << ", "<< '\t' << label << '\n';
      }
    }
  } // end of namespace mem_impl
} // end of namespace intel

void intel::mem::load(reg::gpr r) const
{
  using namespace std;
  using namespace COMPILER;
  const type* T = m_usr->m_type;
  assert(T->scalar());
  int size = T->size();
  usr::flag_t f = m_usr->m_flag;
  refed.insert(refgen_t(m_label,f,size));
  string ptr;
  if (mode == MS)
    ptr = ms_pseudo(size) + " PTR ";
  string dst;
  char sf = '\0';
  if (x64 || size <= 4) {
    dst = reg::name(r, size);
    sf = suffix(size);
  }

  if (literal::integer::big(m_usr) || literal::floating::big(m_usr)) {
    if (x64) {
      if (mode == GNU)
        out << '\t' << "mov" << sf << '\t' << m_label << "(%rip)," << '\t' << dst << '\n';
      else
        out << '\t' << "mov" << sf << '\t' << dst << ", " << ptr << m_label << '\n';
    }
    else
      load_adc(r);
  }
  else {
    if (x64) {
      mem_impl::load_label_x64(r, m_label, f, size);
      string preg = reg::name(r, psize());
      if (mode == GNU) {
        out << '\t' << "mov" << sf << '\t' << '(' << preg << "), ";
	out << dst << '\n';
      }
      else {
        out << '\t' << "mov" << sf << '\t' << dst;
	out << ", " << ptr << '[' << preg << ']' << '\n';
      }
    }
    else {
      if (size <= 4) {
        if (mode == GNU)
          out << '\t' << "mov" << sf << '\t' << m_label << ", " << dst << '\n';
        else {
          reg::gpr tmp = r == reg::ax ? reg::bx : reg::ax;
          string ind = reg::name(tmp, psize());
          out << '\t' << "lea" << '\t' << ind << ", " << m_label << '\n';
          out << '\t' << "mov" << '\t' << dst;
	  out << ", " << ptr << '[' << ind << ']' << '\n';
        }
      }
      else
        load_adc(r);
    }
  }
}

void intel::mem::load_adc(reg::gpr r) const
{
  using namespace std;
  using namespace COMPILER;
  assert(r == reg::ax);
  char sf = suffix(4);
  string eax = reg::name(reg::ax, 4);
  string edx = reg::name(reg::dx, 4);
  string ecx = reg::name(reg::cx, 4);
  const type* T = m_usr->m_type;
  int size = T->size();
  string ptr = "DWORD PTR";
  if (mode == GNU) {
    out << '\t' << "mov" << sf << '\t' << m_label << ", " << eax << '\n';
    out << '\t' << "mov" << sf << '\t' << m_label << "+4" << ", " << edx << '\n';
    if (size == 12)
      out << '\t' << "mov" << sf << '\t' << m_label << "+8" << ", " << ecx << '\n';
  }
  else {
    out << '\t' << "mov" << '\t' << eax << ", " << ptr << ' ' << m_label         << '\n';
    out << '\t' << "mov" << '\t' << edx << ", " << ptr << ' ' << m_label << "+4" << '\n';
    if (size == 12)
      out << '\t' << "mov" << '\t' << ecx << ", " << ptr << ' ' << m_label << "+8" << '\n';
  }
}

std::set<intel::mem::refgen_t> intel::mem::refed;

std::string intel::mem::gnu_refgen(const refgen_t& x)
{
  using namespace std;
  using namespace COMPILER;
  string label = x.m_label;
  usr::flag_t f = x.m_flag;
  ostringstream os;
  if (!x64) {
    assert(refgened.empty());
    return "";
  }
  if (refgened.find(label) != refgened.end())
    return "";
  refgened.insert(label);
  os << '\t' << ".section" << '\t' << ".rdata$.refptr." << label << ", \"dr\"" << '\n';
  if (!(f & usr::STATIC)) {
    os << '\t' << ".globl" << '\t' << ".refptr." << label << '\n';
    os << '\t' << ".linkonce        discard" << '\n';
  }
  os << ".refptr." << label << ":\n";
  os << '\t' << ".quad" << '\t' << label << '\n';
  output_section(NONE);
  return os.str();
}

std::string intel::mem::ms_refgen(const refgen_t& x)
{
  using namespace std;
  using namespace COMPILER;
  string label = x.m_label;
  usr::flag_t f = x.m_flag;
  usr::flag_t m = usr::flag_t(usr::STATIC | usr::INLINE);
  if (f & m)
    return "";

  if (label.substr(0, 3) == "LC$")
    return "";
  string tmp = label;
  if (defined.find(tmp) != defined.end())
    return "";
  ostringstream os;
  os << "EXTERN " << label;
  if (f & usr::FUNCTION) {
    os << ":PROC" << '\n';
    return os.str();
  }

  if (int size = x.m_size) {
    if (size == 1 || size == 2 || size == 4 || size == 8)
      os << ':' << ms_pseudo(x.m_size);
    else
      os << ':' << "BYTE";
    os << '\n';
    return os.str();
  }

  os << ':' << "BYTE" << '\n';
  return os.str();
}

std::set<std::string> intel::mem::refgened;

void intel::mem::indirect_code(reg::gpr r) const
{
  using namespace std;
  using namespace COMPILER;
  assert(x64);
  char sf = psuffix();
  string rs = reg::name(r, 8);
  if (mode == GNU)
    out << '\t' << "mov" << sf << '\t' << ".refptr." << m_label << "(%rip), " << rs << '\n';
  else
    out << '\t' << "lea" << sf << '\t' << rs << ", " << m_label << '\n';
  if (!m_usr->isconstant()) {
    usr::flag_t f = m_usr->m_flag;
    int size = m_usr->m_type->size();
    refed.insert(refgen_t(m_label, f, size));
  }
}

void intel::mem::store() const
{
  using namespace std;
  using namespace COMPILER;
  const type* T = m_usr->m_type;
  int size = T->size();
  assert(T->scalar());
  if (T->real()) {
    if (x64) {
      indirect_code(reg::ax);
      switch (size) {
      case 4:
        if (mode == GNU)
          out << '\t' << "movss" << '\t' << "%xmm0, (" << reg::name(reg::ax, 8) << ')' << '\n';
        else
          out << '\t' << "movss" << '\t' << "DWORD PTR " << '[' << reg::name(reg::ax, 8) << ']' << ", xmm0" << '\n';
        break;
      case 8:
        if (mode == GNU)
          out << '\t' << "movsd" << '\t' << "%xmm0, (" << reg::name(reg::ax, 8) << ')' << '\n';
        else
          out << '\t' << "movsd" << '\t' << "QWORD PTR " << '[' << reg::name(reg::ax, 8) << ']' << ", xmm0"<< '\n';
        break;
      default:
        assert(size == literal::floating::long_double::size);
        if (mode == GNU) {
          out << '\t' << "movsd" << '\t' << "%xmm0, (" << reg::name(reg::ax, 8) << ')' << '\n';
          out << '\t' << "fldl" << '\t' << '(' << reg::name(reg::ax, 8) << ')' << '\n';
          out << '\t' << "fstpt" << '\t' << '(' << reg::name(reg::ax, 8) << ')' << '\n';
        }
        else {
          out << '\t' << "movsd" << '\t' << "QWORD PTR " << '[' << reg::name(reg::ax, 8) << ']' << ", xmm0" << '\n';
          out << '\t' << "fldl" << '\t' << '[' << reg::name(reg::ax, 8) << ']' << '\n';
          out << '\t' << "fstpt" << '\t' << '[' << reg::name(reg::ax, 8) << ']' << '\n';
        }
        break;
      }
    }
    else {
      fstp(m_usr);
    }
  }
  else {
    if ( x64 ) {
      int psz = psize();
      string ax = reg::name(reg::ax, psz);
      string bx = reg::name(reg::bx, psz);
      if (mode == GNU)
        out << '\t' << "mov" << psuffix() << '\t' << ax << ", " << bx << '\n';
      else
        out << '\t' << "mov" << psuffix() << '\t' << bx << ", " << ax << '\n';
      store(reg::bx);
    }
    else
      store(reg::ax);
  }
}

void intel::mem::store(reg::gpr r) const
{
  using namespace std;
  using namespace COMPILER;
  const type* T = m_usr->m_type;
  assert(T->scalar());
  int size = T->size();
  char sf = suffix(size);
  if (x64) {
    reg::gpr r2 = (r == reg::ax) ? reg::bx : reg::ax;
    indirect_code(r2);
    string rs = reg::name(r, size);
    string r2s = reg::name(r2, 8);
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << rs << ", (" << r2s << ')' << '\n';
    else {
      out << '\t' << "mov" << '\t';
      out << ms_pseudo(size) << " PTR [" << r2s << "], " << rs << '\n';
    }
  }
  else {
    if ( size <= 4 ) {
      if (mode == GNU)
        out << '\t' << "mov" << sf << '\t' << reg::name(r, size) << ", " << m_label << '\n';
      else {
        reg::gpr tmp = r == reg::ax ? reg::bx : reg::ax;
        string ind = reg::name(tmp, psize());
        out << '\t' << "lea" << '\t' << ind << ", " << m_label << '\n';
        out << '\t' << "mov" << '\t' << ms_pseudo(size);
	out << " PTR [" << ind << "], " << reg::name(r, size) << '\n';
      }
    }
    else {
      assert(size == 8 || size == 12);
      if (mode == GNU) {
        out << '\t' << "movl" << '\t' << "%eax, " << m_label << '\n';
        out << '\t' << "movl" << '\t' << "%edx, " << m_label << "+4" << '\n';
        if ( size == 12 )
          out << '\t' << "movl" << '\t' << "%ecx, " << m_label << "+8" << '\n';
      }
      else {
        string ptr = "DWORD PTR ";
        out << '\t' << "mov" << '\t' << ptr << m_label << ", eax" << '\n';
        out << '\t' << "mov" << '\t' << ptr << m_label << "+4" << ", edx" << '\n';
        if ( size == 12 )
          out << '\t' << "mov" << '\t' << ptr << m_label << "+8" << ", ecx" << '\n';
      }
    }
  }
  usr::flag_t f = m_usr->m_flag;
  refed.insert(refgen_t(m_label, f, size));
}

void intel::mem::get(reg::gpr r) const
{
  using namespace std;
  using namespace COMPILER;
  string rs = reg::name(r, psize());
  char sf = psuffix();
  if (mode == GNU) {
    out << '\t' << "lea" << sf << '\t' << m_label;
    if (x64)
      out << "(%rip)";
    out << ", " << rs << '\n';
  }
  else {
    out << '\t' << "lea" << sf << '\t' << rs << ", " << m_label << '\n';
    usr::flag_t f = m_usr->m_flag;
    int size = m_usr->m_type->size();
    refed.insert(refgen_t(m_label, f, size));
  }
}

std::string intel::mem::expr(int delta, bool special) const
{
  using namespace std;
  using namespace COMPILER;
  ostringstream os;
  const type* T = m_usr->m_type;
  int size = T->size();
  if (mode == MS) {
    if (T->scalar())
      os << ms_pseudo(special ? 4 : size) << " PTR ";
  }

  if (delta)
    os << '(';
  os << m_label;
  if (delta)
    os << '+' << delta << ')';
  if (x64) {
    if (mode == GNU)
      os << "(%rip)";
    else {
      usr::flag_t f = m_usr->m_flag;
      refed.insert(refgen_t(m_label, f, size));
    }
  }
  return os.str();
}

bool intel::mem::is(COMPILER::usr* u)
{
  using namespace COMPILER;
  usr::flag_t flag = u->m_flag;
#ifdef CXX_GENERATOR
  if (flag & usr::NAMESPACE)
    return false;
  usr::flag2_t flag2 = u->m_flag2;
  usr::flag2_t mask2 =
    usr::flag2_t(usr::TEMPLATE | usr::PARTIAL_ORDERING);
  if (flag2 & mask2)
    return false;
  if (!(flag & usr::WITH_INI)) {
    if (flag2 & usr::INSTANTIATE) {
      typedef instantiated_usr IU;
      IU* iu = static_cast<IU*>(u);
      const IU::SEED& seed = iu->m_seed;
      typedef IU::SEED::const_iterator IT;
      IT p = find_if(begin(seed), end(seed), incomplete);
      if (p != end(seed))
        return false;
    }
  }
#endif // CXX_GENERATOR
  if (!flag) {
    if (!u->m_scope->m_parent)
      return true;
#ifdef CXX_GENERATOR
    if (u->m_scope->m_id == scope::NAMESPACE)
      return true;
#endif // CXX_GENERATOR
    return false;
  }
  usr::flag_t mask = usr::flag_t(usr::EXTERN | usr::STATIC | usr::INLINE | usr::FUNCTION | usr::WITH_INI | usr::SUB_CONST_LONG);

#ifdef CXX_GENERATOR
  mask = usr::flag_t(mask | usr::STATIC_DEF | usr::C_SYMBOL);
#endif // CXX_GENERATOR
  if (x64)
    mask = usr::flag_t(mask | usr::CONST_PTR);
  return flag & mask;
}

intel::mem::mem(COMPILER::usr* u) : address(MEM), m_usr(u)
{
  using namespace std;
  using namespace COMPILER;
  string name = u->m_name;
  usr::flag_t f = u->m_flag;
#ifdef CXX_GENERATOR
  if ((f & usr::C_SYMBOL) || !(f & usr::FUNCTION))
    m_label = external_header + cxx_label(u);
  else
    m_label = cxx_label(u);
#else // CXX_GENERATOR
  if (mode == MS && name[0] == '.')
    name = name.substr(1);
  m_label = external_header + name;
#endif // CXX_GENERATOR
  if (f & usr::STATIC) {
    if (is_string(name))
      m_label = new_label((mode == GNU) ? ".LC" : "LC$");
    else if (!is_external_declaration(u))
      m_label = new_label(m_label + ((mode == GNU) ? "." : "$"));
  }
  else if (doll_need(m_label))
      m_label += '$';
}

namespace intel {
  std::string escape_sequence(std::string);
  inline bool use_ascii(bool str, string name)
  {
    if (!str)
      return false;
    if (name[0] == 'L')
      return false;
    if (mode != GNU)
      return false;
    auto p = find_if(begin(name), end(name), not1(ptr_fun(isascii)));
    if (p != end(name))
      return false;
    p = find(begin(name), end(name), 0x1b);
    return p == end(name);
  }
} // end of namespace intel

bool intel::mem::genobj()
{
  using namespace std;
  using namespace COMPILER;
  usr::flag_t flag = m_usr->m_flag;
  usr::flag_t mask = usr::flag_t(usr::FUNCTION | usr::EXTERN);
  if (flag & mask)
    return false;
#ifdef CXX_GENERATOR
  if (m_usr->m_scope->m_id == scope::TAG) {
    if (!(flag & usr::STATIC_DEF))
      return false;
  }
#endif // CXX_GENERATOR
  if (literal::floating::big(m_usr)) {
    sec_hlp sentry(ROMDATA);
    m_label = new_label((mode == GNU) ? ".LC" : "LC$");
    out << m_label;
    if (mode == GNU)
      out << ":\n";
    else
      out << ' ';
    literal::floating::output(m_usr);
    return true;
  }
  if (literal::integer::big(m_usr) || (x64 && (flag & usr::CONST_PTR))) {
    sec_hlp sentry(ROMDATA);
    m_label = new_label((mode == GNU) ? ".LC" : "LC$");
    out << m_label << ":\n";
    literal::integer::output(m_usr);
    return true;
  }
  const type* T = m_usr->m_type;
  string name = m_usr->m_name;
  bool str = is_string(name);
  if (flag & usr::WITH_INI) {
    sec_hlp sentry(T->modifiable(true) && !str ? RAM : ROMDATA);
    with_initial* p = static_cast<with_initial*>(m_usr);
    if (!(flag & ~usr::WITH_INI))
      out << '\t' << pseudo_global << '\t' << m_label << '\n';
    out << m_label << ":\n";
    if (use_ascii(str, name)) {
      out << '\t' << ".ascii" << '\t';
      name = escape_sequence(name);
      int len = name.length();
      copy(&name[0], &name[len - 1], ostream_iterator<char>(out));
      out << "\\0" << '"' << '\n';
    }
    else {
      if (mode == MS && str)
        out << '\t' << comment_start << name << '\n';
      const map<int, var*>& value = p->m_value;
      if (int n = T->size() - accumulate(value.begin(), value.end(), 0, pseudo)) {
        assert(n > 0);
        if (mode == GNU)
          out << '\t' << ".space" << '\t' << n << '\n';
        else {
          while (n--)
            out << '\t' << "BYTE" << '\t' << 0 << '\n';
        }
      }
    }
    if (!str)
      defined.insert(m_label);
    return true;
  }
  int size = T->size();
  int n = size < 16 ? 16 : size + 16;
  if (mode == GNU) {
    sec_hlp sentry(T->modifiable() && !str ? BSS : ROMDATA);
    if (flag & usr::STATIC)
      out << '\t' << ".lcomm" << '\t' << m_label << ", " << n << '\n';
    else
      out << '\t' << ".comm" << '\t' << m_label << ", " << n << " # " << size << '\n';
  }
  else {
    sec_hlp sentry(T->modifiable() && !str ? BSS : ROMDATA);
    if (size == 1 || size == 2 || size == 4 || size == 8)
      out << "COMM" << '\t' << m_label << ':' << ms_pseudo(size) << '\n';
    else
      out << m_label << " DB " << dec << size << " DUP (?)" << '\n';
  }
  defined.insert(m_label);
  return true;
}

std::string intel::escape_sequence(std::string s)
{
  using namespace std;
  string::size_type p = s.find("\\a");
  while (p != string::npos) {
    s[p + 1] = '7';
    p = s.find("\\a", p);
  }
  return s;
}

#ifdef CXX_GENERATOR
namespace intel {
  void add_types(COMPILER::base* bp)
  {
    tag* ptr = bp->m_tag;
    const type* T = ptr->m_types.second;
    except::throw_types.push_back(T);
    if (auto bases = ptr->m_bases)
      for_each(begin(*bases), end(*bases), add_types);
  }
} // end of namespace intel
#endif // CXX_GENERATOR

int intel::mem::pseudo(int offset, const std::pair<int, COMPILER::var*>& p)
{
  using namespace std;
  using namespace COMPILER;
  if (int n = p.first - offset) {
    assert(n > 0);
    if (mode == GNU)
      out << '\t' << ".space" << '\t' << n << '\n';
    else {
      while (n--)
        out << '\t' << "BYTE" << '\t' << 0 << '\n';
    }
    offset = p.first;
  }
  var* v = p.second;
  const type* T = v->m_type;
  int size = T->size();
  offset += size;
  addrof* addr = v->addrof_cast();
  if (!addr) {
    usr* u = static_cast<usr*>(v);
    if (literal::floating::big(u)) {
      literal::floating::output(u);
      return offset;
    }
    if (literal::integer::big(u)) {
      literal::integer::output(u);
      return offset;
    }
  }
  out << '\t' << intel::pseudo(size) << '\t';
  if (addr) {
    var* v = addr->m_ref;
#ifdef CXX_GENERATOR
    usr* u = v->usr_cast();
    if (!u) {
      auto ti = static_cast<type_information*>(v);
      const type* T = ti->m_T;
      if (mode == GNU)
          out << except::gnu_label(T, 'I') << '\n';
      else
          out << except::ms::label(except::ms::pre1a, T) << '\n';
      except::throw_types.push_back(T);
      tag* ptr = T->get_tag();
      assert(ptr);
      if (auto bases = ptr->m_bases)
	for_each(begin(*bases), end(*bases), add_types);
      return offset;
    }
#else  // CXX_GENERATOR
    assert(v->usr_cast());
    usr* u = static_cast<usr*>(v);
#endif  // CXX_GENERATOR
    out << (is_external_declaration(u) ? pseudo_helper1(u) : pseudo_helper2(u));
    if (int offset = addr->m_offset)
      out << " + " << offset;
    out << '\n';
  }
  else {
    usr* u = static_cast<usr*>(v);
    imm imm(u);
    string s = imm.expr();
    if (mode == GNU)
      out << s.substr(1) << '\n';
    else
      out << s << '\n';
  }
  return offset;
}

std::vector<intel::mem*> intel::mem::ungen;

std::string intel::mem::pseudo_helper1(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  map<usr*, address*>& table = address_descriptor.first;
  map<usr*, address*>::const_iterator p = table.find(u);
  if (p != table.end()) {
    address* addr = p->second;
    mem* m = static_cast<mem*>(addr);
    return m->m_label;
  }
  else {
    mem* m = new mem(u);
    table[u] = m;
    mem::ungen.push_back(m);
    return m->m_label;
  }
}

std::string intel::mem::pseudo_helper2(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  map<var*, address*>& table = address_descriptor.second;
  map<var*, address*>::const_iterator p = table.find(u);
  if (p != table.end()) {
    address* addr = p->second;
    mem* m = static_cast<mem*>(addr);
    return m->m_label;
  }
  else {
    mem* m = new mem(u);
    table[u] = m;
    mem::ungen.push_back(m);
    return m->m_label;
  }
}

void intel::stack::load() const
{
  using namespace std;
  using namespace COMPILER;
  const type* T = m_var->m_type;
  assert(T->scalar());
  int size = T->size();
  if (T->real()) {
    if (x64) {
      char ps = psuffix();
      switch (size) {
      case 4:
        if (mode == GNU)
          out << '\t' << "movss" << '\t' << expr() << ", " << xmm(0) << '\n';
        else
          out << '\t' << "movss" << '\t' << xmm(0) << ", " << expr() << '\n';
        break;
      case 8:
        if (mode == GNU)
          out << '\t' << "movsd" << '\t' << expr() << ", " << xmm(0) << '\n';
        else
          out << '\t' << "movsd" << '\t' << xmm(0) << ", " << expr() << '\n';
        break;
      case 16:
        fld(m_var);
        if (mode == GNU)
          out << '\t' << "sub" << ps << '\t' << '$' << 8 << ", " << sp() << '\n';
        else
          out << '\t' << "sub" << '\t' << sp() << ", " << 8 << '\n';
        string qptr = "QWORD PTR";
        char sf = fsuffix(8);
        if (mode == GNU)
          out << '\t' << "fstp" << sf << '\t' << '(' << sp() << ')' << '\n';
        else
          out << '\t' << "fstp" << '\t' << ' ' << qptr << ' ' << '[' << sp() << ']' << '\n';
        if (mode == GNU)
          out << '\t' << "movsd" << '\t' << '(' << sp() << "), " << xmm(0) << '\n';
        else
          out << '\t' << "movsd" << '\t' << xmm(0) << ", " << qptr << ' ' << '[' << sp() << ']' << '\n';
        if (mode == GNU)
          out << '\t' << "add" << ps << '\t' << '$' << 8 << ", " << sp() << '\n';
        else
          out << '\t' << "add" << '\t' << sp() << ", " << 8 << '\n';
        break;
      }
    }
    else
      fld(m_var);
  }
  else
    load(reg::ax);
}

void intel::stack::load(reg::gpr r) const
{
  using namespace COMPILER;
  const type* T = m_var->m_type;
  assert(T->scalar());
  int size = T->size();
  if ( x64 )
    assert(size <= 8);

  if (x64 || size <= 4) {
    out << '\t' << "mov" << suffix(size) << '\t';
    if (mode == GNU)
      out << expr() << ", " << reg::name(r, size) << '\n';
    else
      out << reg::name(r, size) << ", " << expr() << '\n';
  }
  else {
    assert(r == reg::ax);
    if (mode == GNU) {
      out << '\t' << "movl" << '\t' << expr(0, true) << ", %eax" << '\n';
      out << '\t' << "movl" << '\t' << expr(4, true) << ", %edx" << '\n';
      if (size == 12)
        out << '\t' << "movl" << '\t' << expr(8, true) << ", %ecx" << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << "eax, " << expr(0, true) << '\n';
      out << '\t' << "mov" << '\t' << "edx, " << expr(4, true) << '\n';
      if (size == 12)
        out << '\t' << "mov" << '\t' << "ecx, " << expr(8, true) << '\n';
    }
  }
}

void intel::stack::store() const
{
  using namespace std;
  using namespace COMPILER;
  const type* T = m_var->m_type;
  assert(T->scalar());
  int size = T->size();
  if (T->real()) {
    if (x64) {
      out << '\t' << "movs";
      switch (size) {
      case 4:
        out << 's';
        break;
      case 8:
      case 16:
        out << 'd';
        break;
      }
      if (mode == GNU)
        out << '\t' << xmm(0) << ", " << expr() << '\n';
      else
        out << '\t' << expr() << ", " << xmm(0) << '\n';
      if (size == literal::floating::long_double::size && mode == GNU) {
        char sf = fsuffix(8);
        out << '\t' << "fld" << sf << '\t' << expr() << '\n';
        fstp(m_var);
      }
    }
    else
      fstp(m_var);
  }
  else
    store(reg::ax);
}

void intel::stack::store(reg::gpr r) const
{
  using namespace COMPILER;
  const type* T = m_var->m_type;
  int size = T->size();
  if ( x64 )
    assert(size <= 8);
  if (x64 || size <= 4) {
    out << '\t' << "mov" << suffix(size) << '\t';
    if (mode == GNU)
      out << reg::name(r, size) << ", " << expr() << '\n';
    else
      out << expr() << ", " << reg::name(r, size) << '\n';
  }
  else {
    assert(!x64 && (size == 8 || size == 12));
    assert(r == reg::ax);
    if (mode == GNU) {
      out << '\t' << "movl" << '\t' << "%eax, " << expr() << '\n';
      out << '\t' << "movl" << '\t' << "%edx, " << expr(4) << '\n';
      if (size == 12)
        out << '\t' << "movl" << '\t' << "%ecx, " << expr(8) << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << expr(0, true) << ", eax" << '\n';
      out << '\t' << "mov" << '\t' << expr(4, true) << ", edx" << '\n';
      if (size == 12)
        out << '\t' << "mov" << '\t' << expr(8, true) << ", ecx" << '\n';
    }
  }
}

void intel::stack::get(reg::gpr r) const
{
  out << '\t' << "lea" << psuffix() << '\t';
  if (mode == GNU)
    out << expr() << ", " << reg::name(r, psize()) << '\n';
  else
    out << reg::name(r, psize()) << ", " << expr() << '\n';
}

std::string intel::stack::expr(int delta, bool special) const
{
  using namespace std;
  using namespace COMPILER;
  string FP = fp();
  ostringstream os;
  if (mode == GNU)
    os << m_offset + delta << '(' << FP << ')';
  else {
    if (m_var) {
      const type* T = m_var->m_type;
      int size = T->size();
      if (T->scalar())
        os << ms_pseudo(special ? 4 : size) << " PTR ";
    }
    int n = m_offset + delta;
    os << '[' << FP;
    if (n >= 0)
      os << '+';
    os << n << ']';
  }
  return os.str();
}

int intel::stack::local_area;

int intel::stack::delta_sp;

void intel::allocated::get(reg::gpr r) const
{
  if (mode == GNU)
    out << '\t' << "mov" << psuffix() << '\t' << expr() << ", " << reg::name(r, psize()) << '\n';
  else
    out << '\t' << "mov" << psuffix() << '\t' << reg::name(r, psize()) << ", " << expr() << '\n';
}

std::string intel::allocated::expr(int delta, bool special) const
{
  using namespace std;
  ostringstream os;
  if (mode == GNU)
    os << m_offset + delta << '(' << fp() << ')';
  else
    os << '[' << fp() << (m_offset + delta) << ']';
  return os.str();
}

int intel::allocated::base = 0;

void intel::output_section(section kind)
{
  if (mode == GNU) {
    static section current;
    if (current != kind) {
      current = kind;
      switch (kind) {
      case CODE: case ROMDATA:
        out << '\t' << ".text" << '\n'; break;
      case RAM: out << '\t' << ".data" << '\n'; break;
      case BSS: break;
      case CTOR:
      case DTOR:
        out << '\t' << ".section" << '\t';
        out << (kind == CTOR ? ".ctors," : ".dtors,");
        out << '"' << 'w' << '"' << '\n';
        break;
      case EXCEPT_TABLE:
#if defined(_MSC_VER) || defined(__CYGWIN__)
        out << '\t' << ".seh_handler	__gxx_personality_seh0,";
        out << "@unwind, @except" << '\n';
        out << '\t' << ".seh_handlerdata" << '\n';
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
        out << '\t' << ".section" << '\t' << ".gcc_except_table,";
        out << '"' << 'a' << '"' << ",@progbits" << '\n';
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
        break;
      case EXCEPT_FRAME:
#if defined(_MSC_VER) || defined(__CYGWIN__)
        // nothing to be done
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
        out << '\t' << ".section" << '\t' << ".eh_frame,";
        out << '"' << 'a' << '"' << ",@progbits" << '\n';
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
        break;
      }
    }
  }
  else {
    switch (kind) {
    case NONE:
      break;
    case CODE:
      out << "_TEXT SEGMENT" << '\n';
      break;
    case ROMDATA:
      out << "CONST SEGMENT" << '\n';
      break;
    case RAM:
      out << "_DATA SEGMENT" << '\n';
      break;
    case BSS:
      out << "_BSS SEGMENT" << '\n';
      break;
    case CTOR:
      {
        out << "_CRT$XCU SEGMENT READONLY";
        int n = x64 ? 8 : 4;
        out << " ALIGN(" << n << ')';
        out << " ALIAS(\".CRT$XCU\")" << '\n';
      }
      break;
    default:
      break;
    }
  }
}

void intel::end_section(section kind)
{
  if (mode == GNU)
    return;
  switch (kind) {
  case CODE:
    out << "_TEXT ENDS" << '\n';
    break;
  case ROMDATA:
    out << "CONST ENDS" << '\n';
    break;
  case RAM:
    out << "_DATA ENDS" << '\n';
    break;
  case BSS:
    out << "_BSS ENDS" << '\n';
    break;
  case CTOR:
    out << "_CRT$XCU ENDS" << '\n';
    break;
  case DTOR:
    out << "text$yd ENDS" << '\n';
    break;
  default:
    break;
  }
}

std::string intel::new_label(std::string head)
{
  using namespace std;
  ostringstream os;
  static int cnt;
  os << head << cnt++;
  return os.str();
}

namespace intel {
  using namespace std;
  using namespace COMPILER;
  pair<map<pair<int, goto3ac*>, string>, map<int, vector<string> > > label_table;
} // end of namespace intel

intel::uint64_float_t intel::uint64_float_t::obj;

intel::uint64_double_t intel::uint64_double_t::obj;

intel::uint64_ld_t intel::uint64_ld_t::obj;

intel::ld_uint64_t intel::ld_uint64_t::obj;

intel::uminus_float_t intel::uminus_float_t::obj;

intel::uminus_double_t intel::uminus_double_t::obj;

intel::real_uint64_t intel::real_uint64_t::obj;

intel::float_uint64_t intel::float_uint64_t::obj;

intel::double_uint64_t intel::double_uint64_t::obj;

std::string intel::pseudo_global = ".globl";

std::string intel::comment_start = "//";

