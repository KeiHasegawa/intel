#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR
#include "intel.h"

namespace intel {
  std::string func_label;
  using namespace COMPILER;
#ifdef CXX_GENERATOR
  void enter(const fundef* func, const std::vector<tac*>& code, bool);
#else // CXX_GENERATOR
  void enter(const fundef* func, const std::vector<tac*>& code);
#endif // CXX_GENERATOR
  void gencode(tac* tac);
  namespace return_impl {
    tac* last3ac;
    std::string leave_label;
  }
#ifdef CXX_GENERATOR
  void leave(const fundef* func, bool);
#else // CXX_GENERATOR
  void leave(const fundef* func);
#endif // CXX_GENERATOR
  std::set<std::string> defined;
#ifdef CXX_GENERATOR
  std::vector<std::string> init_fun;
  std::vector<std::string> term_fun;
  namespace except {
    namespace ms {
      inline bool handler(const std::vector<tac*>& code)
      {
        using namespace std;
        if (mode != MS)
          return false;
        typedef vector<tac*>::const_iterator IT;
        IT p = find_if(begin(code), end(code),
        	       [](tac* p) {return p->m_id == tac::CATCH_BEGIN; });
        return p != end(code);
      }
      namespace x86_handler {
        const std::string prefix = "__ehhandler$";
        const std::string prefix2 = "__catch$";
        void extra()
        {
          using namespace std;
          sec_hlp sentry(CODE);
          out << prefix << func_label << ':' << '\n';
          if (except::call_sites.empty())
            return; // work around
          out << '\t' << "npad	1" << '\n';
          out << '\t' << "npad	1" << '\n';
          out << '\t' << "mov	edx, DWORD PTR[esp + 8]" << '\n';
          out << '\t' << "lea	eax, DWORD PTR[edx + 12]" << '\n';
          out << '\t' << "mov	ecx, DWORD PTR[edx - 224]" << '\n';
          out << '\t' << "xor ecx, eax" << '\n';
          string label1 = "@__security_check_cookie@4";
          out << '\t' << "call	" << label1 << '\n';
          mem::refed.insert(mem::refgen_t(label1, usr::FUNCTION, 0));
          out << '\t' << "mov	eax, OFFSET ";
          out << except::ms::out_table::x86_gen::pre4 << func_label << '\n';
          string label2 = "___CxxFrameHandler3";
          out << '\t' << "jmp	" << label2 << '\n';
          mem::refed.insert(mem::refgen_t(label2, usr::FUNCTION, 0));
        }
      } // end of namespace x86_handler
      namespace x64_handler {
        void magic_code2();
        namespace catch_code {
          void gen();
        } // end of namespace catch_code
      } // end of namespace x64_handler
    } // end of namespace ms
  } // end of namespace except
#endif // CXX_GENERATOR
} // end of namespace intel

void intel::genfunc(const COMPILER::fundef* func,
                    const std::vector<COMPILER::tac*>& code)
{
  using namespace std;
  using namespace COMPILER;

#ifdef CXX_GENERATOR
  bool ms_handler = except::ms::handler(code);
  if (ms_handler && !x64)
    out << "ASSUME NOTHING" << '\n';
#endif // CXX_GENERATOR
  output_section(CODE);
  usr* u = func->m_usr;
#ifdef CXX_GENERATOR
  string name = cxx_label(u);
#else // CXX_GENERATOR
  string name = u->m_name;
#endif // CXX_GENERATOR
  usr::flag_t f = u->m_flag;
#ifdef CXX_GENERATOR
  if (f & usr::C_SYMBOL)
    func_label = external_header + name;
  else
    func_label = name;
#else // CXX_GENERATOR
  func_label = external_header + name;
#endif // CXX_GENERATOR
  if (doll_need(func_label))
    func_label += '$';
  defined.insert(func_label);

#ifdef CXX_GENERATOR
#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU)
    out << '\t' << ".seh_proc	" << func_label << '\n';
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
#endif // CXX_GENERATOR

  usr::flag_t m = usr::flag_t(usr::STATIC | usr::INLINE);
  if (!(f & m) || (f & usr::EXTERN))
    out << '\t' << pseudo_global << '\t' << func_label << '\n';
  out << func_label;
  if (mode == GNU)
    out << ':';
  else {
    out << '\t' << "PROC";
    if ((f & m) && !(f & usr::EXTERN))
      out << '\t' << "PRIVATE";
  }
  out << '\n';
#ifdef CXX_GENERATOR
  enter(func, code, ms_handler);
#else // CXX_GENERATOR
  enter(func, code);
#endif // CXX_GENERATOR
  if (!code.empty())
    return_impl::last3ac = *code.rbegin();
  return_impl::leave_label.erase();
  for_each(code.begin(),code.end(),gencode);
#ifdef CXX_GENERATOR
  leave(func, ms_handler);
  end_section(CODE);
  usr::flag2_t f2 = u->m_flag2;
  if (f2 & usr::INITIALIZE_FUNCTION)
    init_fun.push_back(func_label);
  if (f2 & usr::TERMINATE_FUNCTION)
    term_fun.push_back(func_label);

  if (ms_handler) {
    if (x64)
      except::ms::x64_handler::catch_code::gen();
    else
      except::ms::x86_handler::extra();
  }
  if (ms_handler && x64)
    except::ms::x64_handler::magic_code2();
  except::out_table(ms_handler);
  for (auto T : except::throw_types) {
    if (T->tmp())
      except::out_type_info(T);
    else
      except::types_to_output.insert(T);
  }
  except::throw_types.clear();
#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU) {
    out << '\t' << ".text" << '\n';
    out << '\t' << ".seh_endproc" << '\n';
  }
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
#else // CXX_GENERATOR
  leave(func);
  end_section(CODE);
#endif // CXX_GENERATOR
  {
    map<var*, address*>& table = address_descriptor.second;
    for_each(table.begin(), table.end(), destroy_address<var*>());
    table.clear();
  }
}

#ifdef CXX_GENERATOR
namespace intel {
  inline void init_call(std::string f)
  {
    out << '\t' << "call" << '\t' << f << '\n';
  }
  inline std::string atexit_label()
  {
    if (mode == GNU)
      return "__cxa_atexit";

    using namespace std;
    using namespace COMPILER;
    string label = external_header + "atexit";
    mem::refgen_t ref(label, usr::FUNCTION, 0);
    mem::refed.insert(ref);
    return label;
  }
  inline void term_call(std::string f)
  {
    using namespace std;
    char ps = psuffix();
    int psz = psize();
    if (x64)
      mem_impl::load_label_x64(reg::cx, f, COMPILER::usr::NONE, 8);
    else {
      out << '\t' << "push" << ps << '\t';
      if (mode == GNU)
        out << '$';
      out << f << '\n';
    }
    string name = atexit_label();
    out << '\t' << "call" << '\t' << name << '\n';
  }
} // end of namespace intel

void intel::init_term_fun()
{
  using namespace std;
  using namespace COMPILER;
  if (init_fun.empty() && term_fun.empty())
    return;

  string name = "init_term";
  {
    sec_hlp helper(CODE);
    out << name << ':' << '\n';
    char ps = psuffix();
    string SP = sp();
    string FP = fp();
    int psz = psize();
    out << '\t' << "push" << ps << '\t' << FP << '\n';
    if (mode == MS && !x64)
      out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
    out << '\t' << "mov" << ps << '\t';
    if (mode == GNU)
      out << SP << ", " << FP << '\n';
    else
      out << FP << ", " << SP << '\n';
    if (mode != MS || x64)
      out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
    int stack_size = 16;
    out << '\t' << "sub" << ps << '\t';
    if (mode == GNU)
      out << '$' << dec << stack_size << ", " << SP << '\n';
    else
      out << SP << ", " << dec << stack_size << '\n';

    for_each(begin(init_fun), end(init_fun), init_call);
    for_each(rbegin(term_fun), rend(term_fun), term_call);

    out << '\t' << "mov" << ps << '\t';
    if (mode == GNU)
      out << FP << ", " << SP << '\n';
    else
      out << SP << ", " << FP << '\n';
    if (mode != MS || x64) {
      int m = x64 ? 8 : 4;
      out << '\t' << "sub" << ps << '\t';
      if (mode == GNU)
        out << '$' << m << " , " << SP << '\n';
      else
        out << SP << " , " << m << '\n';
    }
    out << '\t' << "pop" << ps << '\t' << reg::name(reg::bx, psize()) << '\n';
    out << '\t' << "pop" << ps << '\t' << FP << '\n';
    out << '\t' << "ret" << '\n';
  }

  {
    sec_hlp helper(CTOR);
    int n = x64 ? 8 : 4;
    if (mode == GNU)
      out << '\t' << ".align" << '\t' << n << '\n';
    out << '\t' << pseudo(n) << '\t' << name << '\n';
  }
}
#endif // CXX_GENERATOR

namespace intel {
  using namespace COMPILER;
  void sched_stack(const fundef* func, const std::vector<tac*>& code
#ifdef CXX_GENERATOR
                   , bool ms_handler
#endif // CXX_GENERATOR
  );
#ifdef CXX_GENERATOR
  namespace except {
      namespace ms {
          namespace x86_handler {
              void partof_prolog()
              {
                  assert(mode == MS && !x64);
                  out << '\t' << "push  -1" << '\n';
                  out << '\t' << "push	" << prefix << func_label << '\n';
                  out << '\t' << "mov	eax, DWORD PTR fs:0" << '\n';
                  out << '\t' << "push	eax" << '\n';
                  out << '\t' << "push	ecx" << '\n';
              }
              const int ehrec_off = 16;
              void partof_prolog2()
              {
                  using namespace std;
                  assert(mode == MS && !x64);
                  out << '\t' << "push	ebx" << '\n';
                  out << '\t' << "push	esi" << '\n';
                  out << '\t' << "push	edi" << '\n';
                  string label = "___security_cookie";
                  out << '\t' << "mov	eax, DWORD PTR " << label << '\n';
                  mem::refed.insert(mem::refgen_t(label, usr::NONE, 4));
                  out << '\t' << "xor eax, ebp" << '\n';
                  out << '\t' << "push	eax" << '\n';
                  int n = ms::x86_handler::ehrec_off - 4;
                  out << '\t' << "lea	eax, DWORD PTR [ebp-" << n << ']' << '\n';
                  out << '\t' << "mov	DWORD PTR fs:0, eax" << '\n';
                  int m = ms::x86_handler::ehrec_off;
                  out << '\t' << "mov	DWORD PTR [ebp-" << m << "], esp" << '\n';
              }
          } // end of namespace x86_handler
          namespace x64_handler {
              const int magic = 16 * 14;
              const std::string magic_label = "__97ABDBD4_bbb@cpp";
              void magic_code()
              {
                  using namespace std;
                  out << '\t' << "mov	rdi, rsp" << '\n';
                  int n = stack::delta_sp >> 2;
                  out << '\t' << "mov	ecx, " << n << '\n';
                  out << '\t' << "mov	eax, -858993460" << '\n';
                  out << '\t' << "rep stosd" << '\n';
                  out << '\t' << "lea	rcx, OFFSET " << magic_label << '\n';
                  string label = "__CheckForDebuggerJustMyCode";
                  out << '\t' << "call	" << label << '\n';
                  mem::refed.insert(mem::refgen_t(label, usr::FUNCTION, 0));
                  out << '\t' << "npad	1" << '\n';
              }
              void magic_code2()
              {
                  out << "msvcjmc	SEGMENT" << '\n';
                  out << magic_label << " DB 01H" << '\n';
                  out << "msvcjmc	ENDS" << '\n';
              }
          } // end of namespace x64_handler
      } // end of namespace ms
  } // end of namespace except
#endif // CXX_GENERATOR
}

void intel::enter(const COMPILER::fundef* func,
                  const std::vector<COMPILER::tac*>& code
#ifdef CXX_GENERATOR
                  , bool ms_handler
#endif // CXX_GENERATOR
)
{
  using namespace std;
  using namespace COMPILER;
  if (debug_flag)
    out << '\t' << comment_start << " enter\n";
  char ps = psuffix();
  string SP = sp();
  string FP = fp();
  int psz = psize();

#ifdef CXX_GENERATOR
  usr* u = func->m_usr;
  usr::flag2_t f2 = u->m_flag2;
  usr::flag2_t mask =
    usr::flag2_t(usr::INITIALIZE_FUNCTION | usr::TERMINATE_FUNCTION);
#if defined(_MSC_VER) || defined(__CYGWIN__)
  // nothing to be done
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
  if (!(f2 & mask)) {
    except::frame_desc_t fd;
    fd.m_fname = func_label;
    except::fds.push_back(fd);
  }
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)  
#endif // CXX_GENERATOR

  out << '\t' << "push" << ps << '\t' << FP << '\n';
#ifdef CXX_GENERATOR
#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU)
    out << '\t' << ".seh_pushreg	%rbp" << '\n';
#else // defined(_MSC_VER) || defined(__CYGWIN__)  
  if (!(f2 & mask)) {
    string label = except::LCFI_label();
    out << label << ':' << '\n';
    assert(!except::fds.empty());
    except::frame_desc_t& fd = except::fds.back();
    vector<except::call_frame_t*>& cfs = fd.m_cfs;
    cfs.push_back(new except::save_fp(fd.m_fname, label));
  }
#endif // defined(_MSC_VER) || defined(__CYGWIN__)  
#endif // CXX_GENERATOR
  if (mode == MS && !x64) {
#ifdef CXX_GENERATOR
    if (!ms_handler)
      out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
#else // CXX_GENERATOR
    out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
#endif // CXX_GENERATOR
  }
  if (mode == GNU)
    out << '\t' << "mov" << ps << '\t' << SP << ", " << FP << '\n';
  else {
#ifdef CXX_GENERATOR
    if (ms_handler && x64) {
      out << '\t' << "push" << '\t' << "rdi" << '\n';
      out << '\t' << "mov" << ps << '\t' << FP << ", " << SP << '\n';
    }
    else
      out << '\t' << "mov" << ps << '\t' << FP << ", " << SP << '\n';
#else // CXX_GENERATOR
    out << '\t' << "mov" << ps << '\t' << FP << ", " << SP << '\n';
#endif // CXX_GENERATOR
  }
#ifdef CXX_GENERATOR
#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU)
    out << '\t' << ".seh_setframe	%rbp, 0" << '\n';
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
  if (!(f2 & mask)) {
    string label = except::LCFI_label();
    out << label << ':' << '\n';
    assert(!except::fds.empty());
    except::frame_desc_t& fd = except::fds.back();
    vector<except::call_frame_t*>& cfs = fd.m_cfs;
    assert(!cfs.empty());
    except::call_frame_t* cf = cfs.back();
    string begin = cf->m_end;
    cfs.push_back(new except::save_sp(begin, label));
  }
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
#endif // CXX_GENERATOR

  if (mode != MS)
    out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
  else {
    if (x64) {
#ifdef CXX_GENERATOR
      if (!ms_handler)
        out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
#else // CXX_GENERATOR
      out << '\t' << "push" << ps << '\t' << reg::name(reg::bx, psz) << '\n';
#endif // CXX_GENERATOR
    }
    else {
#ifdef CXX_GENERATOR
      if (ms_handler)
        except::ms::x86_handler::partof_prolog();
#endif // CXX_GENERATOR
    }
  }

#ifdef CXX_GENERATOR
  sched_stack(func, code, ms_handler);
#else // CXX_GENERATOR
  sched_stack(func, code);
#endif // CXX_GENERATOR
  if (x64) {
    stack::delta_sp += 8;
#ifdef CXX_GENERATOR
    if (ms_handler)
      stack::delta_sp += except::ms::x64_handler::magic;
#endif // CXX_GENERATOR
  }
  else {
#ifdef CXX_GENERATOR
    if (ms_handler) {
      int n = 16 * 8 - 4;
      stack::delta_sp += n;
      stack::local_area += n;
    }
#endif // CXX_GENERATOR
  }

  int n = stack::delta_sp;
  string ax = reg::name(reg::ax, psz);  
#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (stack::delta_sp < stack::threshold_alloca) {
    out << '\t' << "sub" << ps << '\t';
    if (mode == GNU)
      out << '$' << dec << n << ", " << SP << '\n';
    else
      out << SP << ", " << dec << n << '\n';
  }
  else {
    if (mode == GNU) {
      out << '\t' << "movl" << '\t' << "$" << dec << n << ", %eax" << '\n';
      out << '\t' << "call" << '\t' << "___chkstk_ms" << '\n';
      out << '\t' << "sub" << ps << '\t' << ax << ", " << SP << '\n';      
    }
    else {
      string label = "__chkstk";
      out << '\t' << "mov" << '\t' << "eax, " << dec << n << '\n';
      out << '\t' << "call" << '\t' << label << '\n';
      out << '\t' << "sub" << '\t' << SP << ", " << ax << '\n';
      mem::refed.insert(mem::refgen_t(label, usr::FUNCTION, 0));
    }
  }
#ifdef CXX_GENERATOR
  if (mode == MS && x64) {
    out << func_label << except::ms::prolog_size << ' ';
    out << "EQU" << ' ' << "$ - " << func_label << '\n';
    if (ms_handler)
      except::ms::x64_handler::magic_code();
  }
#endif // CXX_GENERATOR
#else // defined(_MSC_VER) || defined(__CYGWIN__)
  out << '\t' << "sub" << ps << '\t' << "$" << dec << n << ", " << SP << '\n';
#endif // defined(_MSC_VER) || defined(__CYGWIN__)

#ifdef CXX_GENERATOR
#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU) {
    out << '\t' << ".seh_stackalloc	" << n << '\n';
    out << '\t' << ".seh_endprologue" << '\n';
  }
#else // defined(_MSC_VER) || defined(__CYGWIN__)
  if (!(f2 & mask)) {
    string label = except::LCFI_label();
    out << label << ':' << '\n';
    assert(!except::fds.empty());
    except::frame_desc_t& fd = except::fds.back();
    vector<except::call_frame_t*>& cfs = fd.m_cfs;
    assert(!cfs.empty());
    except::call_frame_t* cf = cfs.back();
    string begin = cf->m_end;
    cfs.push_back(new except::save_bx(begin, label));
  }
#endif // else // defined(_MSC_VER) || defined(__CYGWIN__)
#endif // CXX_GENERATOR

#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU) {
    if (func->m_usr->m_name == "main")
      out << '\t' << "call" << '\t' << "__main" << '\n';
  }
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
#ifdef CXX_GENERATOR
  if (ms_handler && !x64)
    except::ms::x86_handler::partof_prolog2();
#endif // CXX_GENERATOR
}

namespace intel { namespace aggregate_func {
  namespace ret {
    bool cmp(COMPILER::tac* x, COMPILER::tac* y);
    int size(COMPILER::tac* tac);
    stack* area;
  }  // end of namespace ret
  namespace param {
    // copy area for `y' where `y' is used as "param y" and type of y
    // is not scalar, or type of y is long double.
    typedef std::map<COMPILER::tac*, stack*> table_t;
    table_t table;

    // 1st param3ac set for t := call f where type of t is not scalar.
    // (for x64, type of t is long double)
    typedef std::set<COMPILER::tac*> first_t;
    first_t first;

    int calc(int offset, COMPILER::tac* tac);
  }  // end of namespace param
} } // end of namespace aggregate_func and intel

#ifdef CXX_GENERATOR
namespace intel {
    namespace except {
        namespace ms {
            namespace x86_handler {
                void partof_epilog()
                {
                    int n = ms::x86_handler::ehrec_off - 4;
                    out << '\t' << "mov	ecx, DWORD PTR [ebp-" << n << ']' << '\n';
                    out << '\t' << "mov	DWORD PTR fs:0, ecx" << '\n';
                    out << '\t' << "pop	ecx" << '\n';
                    out << '\t' << "pop	edi" << '\n';
                    out << '\t' << "pop	esi" << '\n';
                    out << '\t' << "pop	ebx" << '\n';
                }
            } // end of namespace x86_handler
        } // end of namespace ms
    } // end of namespace except
} // end of namespace intel
#endif // CXX_GENERATOR

void intel::leave(const COMPILER::fundef* func
#ifdef CXX_GENERATOR
                  , bool ms_handler
#endif // CXX_GENERATOR
)
{
  using namespace std;
  using namespace COMPILER;
  if (debug_flag)
    out << '\t' << comment_start << " leave\n";
  if ( !return_impl::leave_label.empty() )
    out << return_impl::leave_label << ":\n";

#ifdef CXX_GENERATOR
  if (ms_handler && !x64)
    except::ms::x86_handler::partof_epilog();
#endif // CXX_GENERATOR

  char ps = psuffix();
  if (mode == GNU) {
    out << '\t' << "mov" << ps << '\t' << fp() << ", " << sp() << '\n';
  }
  else {
#ifdef CXX_GENERATOR
    if (ms_handler && x64)
      out << '\t' << "mov" << ps << '\t' << sp() << ", " << fp() << '\n';
    else
      out << '\t' << "mov" << ps << '\t' << sp() << ", " << fp() << '\n';
#else // CXX_GENERATOR
    out << '\t' << "mov" << ps << '\t' << sp() << ", " << fp() << '\n';
#endif // CXX_GENERATOR
  }

  int m = x64 ? 8 : 4;
  if (mode == GNU) {
    out << '\t' << "sub" << ps << '\t' << '$' << m << " , " << sp() << '\n';
  }
  else {
    if (x64) {
#ifdef CXX_GENERATOR
      if (!ms_handler)
        out << '\t' << "sub" << ps << '\t' << sp() << " , " << m << '\n';
#else // CXX_GENERATOR
      out << '\t' << "sub" << ps << '\t' << sp() << " , " << m << '\n';
#endif // CXX_GENERATOR
    }
  }
#ifdef CXX_GENERATOR
  if (ms_handler) {
    if (x64)
      out << '\t' << "pop	rdi" << '\n';
  }
  else
    out << '\t' << "pop" << ps << '\t' << reg::name(reg::bx, psize()) << '\n';
#else // CXX_GENERATOR
  out << '\t' << "pop" << ps << '\t' << reg::name(reg::bx, psize()) << '\n';
#endif // CXX_GENERATOR
  out << '\t' << "pop" << ps << '\t' << fp() << '\n';
#ifdef CXX_GENERATOR
  usr* u = func->m_usr;
  usr::flag2_t f2 = u->m_flag2;
  usr::flag2_t mask =
    usr::flag2_t(usr::INITIALIZE_FUNCTION | usr::TERMINATE_FUNCTION);
#if defined(_MSC_VER) || defined(__CYGWIN__)
  // nothing to be done
#else // defined(_MSC_VER) || defined(__CYGWIN__)
  if (!(f2 & mask)) {
    string label = except::LCFI_label();
    out << label << ':' << '\n';
    assert(!except::fds.empty());
    except::frame_desc_t& fd = except::fds.back();
    vector<except::call_frame_t*>& cfs = fd.m_cfs;
    assert(!cfs.empty());
    except::call_frame_t* cf = cfs.back();
    string begin = cf->m_end;
    cfs.push_back(new except::recover(begin, label));
  }
#endif // defined(_MSC_VER) || defined(__CYGWIN__) 
#endif // CXX_GENERATOR
  out << '\t' << "ret" << '\n';
#ifdef CXX_GENERATOR
#if defined(_MSC_VER) || defined(__CYGWIN__)
  // nothing to be done
#else  // defined(_MSC_VER) || defined(__CYGWIN__)  
  if (!(f2 & mask)) {
    string end;
    if (mode == GNU)
      end = '.' + func_label + ".end";
    else
      end = func_label + ".end" + '$';
    out << end << ':' << '\n';
    assert(!except::fds.empty());
    except::frame_desc_t& fd = except::fds.back();
    fd.m_end = end;
  }
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)    
#endif // CXX_GENERATOR

  if (mode == MS) {
      out << func_label << "$end" << '\t' << "EQU $" << '\n';
      out << func_label << '\t' << "ENDP" << '\n';
  }
  delete aggregate_func::ret::area;
  aggregate_func::ret::area = 0;
  {
    map<tac*, stack*>& table = aggregate_func::param::table;
    for_each(table.begin(), table.end(), destroy_address<tac*>());
    table.clear();
  }
  aggregate_func::param::first.clear();
}

namespace intel {
  int func_local(const COMPILER::fundef* func
#ifdef CXX_GENERATOR
                 , bool ms_handler
#endif // CXX_GENERATOR
  );
  namespace call_arg {
    int calculate(const std::vector<COMPILER::tac*>& code);
  }  // end of namespace call_arg
  inline int align(int n, int m)
  {
    if (n & (m - 1))
      n = (n + m) & ~(m-1);
    return n;
  }
  inline bool cmp_id(const COMPILER::tac* tac, COMPILER::tac::id_t id)
  {
    return tac->m_id == id;
  }
}  // end of namespace intel

void intel::sched_stack(const COMPILER::fundef* func,
                        const std::vector<COMPILER::tac*>& code
#ifdef CXX_GENERATOR
                        , bool ms_handler
#endif // CXX_GENERATOR
)
{
  using namespace std;
  using namespace COMPILER;
  int psz = psize();
  stack::local_area = psz;  // for return address save area
#ifdef CXX_GENERATOR
  if (ms_handler && !x64) {
    stack::local_area += except::ms::x86_handler::ehrec_off;
    typedef vector<tac*>::const_iterator IT;
    IT p = find_if(begin(code), end(code),
        	   [](const tac* ptr) { return ptr->m_id == tac::CATCH_BEGIN; });
    if (p != end(code)) {
      tac* ptr = *p;
      if (var* x = ptr->x) {
        stack::local_area = align(stack::local_area, 8);
        address_descriptor.second[x] =
          new stack(x, -stack::local_area); // reserved here
        const type* T = x->m_type;
        int size = T->size();
        stack::local_area += size;
      }
    }
  }
#endif // CXX_GENERATOR
  vector<tac*>::const_iterator p =
    max_element(code.begin(),code.end(), aggregate_func::ret::cmp);
  if ( p != code.end() ){
    if (int n = aggregate_func::ret::size(*p)) {
      stack::local_area += n;
      aggregate_func::ret::area = new stack(0, -stack::local_area);
    }
  }

  p = code.begin();
  while (p != code.end()) {
    p = find_if(p, code.end(), aggregate_func::ret::size);
    if (p == code.end())
      break;
    typedef vector<tac*>::const_reverse_iterator IT;
    IT q(p);
    IT r = find_if(q, code.rend(), not1(bind2nd(ptr_fun(cmp_id), tac::PARAM)));
    --r;
    if (cmp_id(*r, tac::PARAM))
      aggregate_func::param::first.insert(*r);
    ++p;
  }

  if (x64) {
    stack::local_area =
      accumulate(code.begin(), code.end(), stack::local_area,
                 aggregate_func::param::calc);
  }
  allocated::base = 0;
#ifdef CXX_GENERATOR
  stack::local_area += func_local(func, ms_handler);
#else // CXX_GENERATOR
  stack::local_area += func_local(func);
#endif // CXX_GENERATOR
  if (allocated::base) {
    if (debug_flag)
      out << '\t' << comment_start << " The top of local area of stack is saved\n";
    char sf = psuffix();
    string rax = reg::name(reg::ax, psize());
    int n = -stack::local_area;
    int m = -allocated::base;
    if (mode == GNU) {
      out << '\t' << "lea" << sf << '\t' << n << '(' << fp() << ')' << ", " << rax << '\n';
      out << '\t' << "mov" << sf << '\t' << rax << ", " << m << '(' << fp() << ')' << '\n';
    }
    else {
      out << '\t' << "lea" << '\t' << rax << ", " << '[' << fp() << n << ']' << '\n';
      string ptr = ms_pseudo(psize()) + " PTR ";
      out << '\t' << "mov" << '\t' << ptr << '[' << fp() << m << ']' << ", " << rax << '\n';
    }
    if (debug_flag)
      out << '\n';
  }

  int n = stack::local_area + call_arg::calculate(code);
  n = align(n, 16);
  stack::delta_sp = n;
}

bool intel::aggregate_func::ret::cmp(COMPILER::tac* x, COMPILER::tac* y)
{
  return size(x) < size(y);
}

int intel::aggregate_func::ret::size(COMPILER::tac* tac)
{
  using namespace COMPILER;
  if ( tac->m_id != tac::CALL )
    return 0;
  var* y = tac->y;
  const type* T = y->m_type;
  T = T->unqualified();
  if ( T->m_id == type::POINTER ){
    typedef const pointer_type PT;
    PT* pt = static_cast<PT*>(T);
    T = pt->referenced_type();
  }
  assert(T->m_id == type::FUNC);
  typedef const func_type FUNC;
  FUNC* func = static_cast<FUNC*>(T);
  T = func->return_type();
#ifdef CXX_GENERATOR
  if (!T)
    return 0;
#endif // CXX_GENERATOR
  assert(T);
  T = T->complete_type();
  int size = T->size();
  if (!size)
    return 0;
  if ( T->scalar() ){
    if ( x64 ) {
      if (mode == GNU)
        return (T->real() && size == literal::floating::long_double::size) ? size : 0;
      else
        return 0;
    }
    else
      return 0;
  }
  return size;
}

int intel::aggregate_func::param::calc(int offset, COMPILER::tac* tac)
{
  using namespace COMPILER;
  if (tac->m_id != tac::PARAM)
    return offset;
  var* y = tac->y;
  const type* T = y->m_type;
  T = T->complete_type();
  int size = T->size();
  if (T->scalar() && size <= 8)
    return offset;
  offset += size;
  offset = align(offset, T->align());
  table[tac] = new stack(tac->y, -offset);
  return offset;
}

namespace intel {
  bool big_ret(const COMPILER::type* T);
  int param_decide(int offset, COMPILER::usr* u);
  int param_save(int offset, COMPILER::usr* u);
  std::string param_reg(int nth, const COMPILER::type* T);
  int local_variable(int offset, COMPILER::scope* scope);
}

int intel::first_param_offset = 0x10;

namespace intel {
  mode_t mode = GNU;
}

int intel::func_local(const COMPILER::fundef* func
#ifdef CXX_GENERATOR
                      , bool ms_handler
#endif // CXX_GENERATOR
)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = func->m_usr->m_type;
  T = T->unqualified();
  typedef const func_type FUNC;
  FUNC* X = static_cast<FUNC*>(T);
  T = X->return_type();
  bool big = big_ret(T);
  int offset = first_param_offset;
#ifdef CXX_GENERATOR
  if (ms_handler && !x64)
    offset -= 4;
#endif // CXX_GENERATOR
  int psz = psize();
  if (big)
    offset += psz;
  const vector<usr*>& order = func->m_param->m_order;
  offset = accumulate(order.begin(),order.end(),offset,param_decide);
  if (debug_flag && !order.empty() && x64)
    out << '\t' << comment_start << " parameter registers are saved\n";
  if (x64) {
    char ps = psuffix();
    if (big) {
      out << '\t' << "mov" << ps << '\t';
      string rcx = reg::name(reg::cx, psz);
      if (mode == GNU)
        out << rcx << ", " << dec << first_param_offset << '(' << fp() << ')' << '\n';
      else
        out << '[' << fp() << '+' << dec << first_param_offset << ']' << ", " << rcx << '\n';
    }
    vector<usr*>::const_iterator end = order.end();
    if (order.size() > (big ? 3 : 4))
      end = order.begin() + (big ? 3 : 4);
    (void)accumulate(order.begin(), end, big ? 1 : 0, param_save);
    const vector<const type*>& param = X->param();
    if (param.size() > order.size()) {
      T = param[order.size()];
      if (T->m_id == type::ELLIPSIS) {
        for (int nth = order.size(); nth != 4; ++nth, offset += 8) {
          string reg = param_reg(nth, 0);
          out << '\t' << "mov" << ps << '\t';
          if (mode == GNU)
            out << reg << ", " << dec << offset << '(' << fp() << ')' << '\n';
          else
            out << '[' << fp() << '+' << dec << offset << ']' << ", " << reg << '\n';
        }
      }
    }
  }
  if (debug_flag && !order.empty() && x64)
    out << '\n';

  const vector<scope*>& children = func->m_param->m_children;
#ifdef CXX_GENERATOR
  assert(!children.empty());
#else
  assert(children.size() == 1);
#endif
  scope* ptr = children.back();
  assert(ptr->m_id == scope::BLOCK);
  return local_variable(stack::local_area, ptr);
}

bool intel::big_ret(const COMPILER::type* T)
{
  if (T) {
    if (x64) {
      if (T->aggregate())
        return true;
      if (T->size() == literal::floating::long_double::size)
        return mode == GNU;
    }
    else
      return T->aggregate();
  }
  return false;
}

namespace intel {
  inline bool isEllipsis(const COMPILER::type* T)
  {
    return T->m_id == COMPILER::type::ELLIPSIS;
  }
  namespace call_arg {
    std::set<COMPILER::tac*> ellipsised;
    class isEllipsised {
      int nth;
    public:
      isEllipsised(int n) : nth(n) {}
      bool operator()(COMPILER::tac* tac);
    };
    int param_size(int n, COMPILER::tac*);
  }  // end of namespace call_arg
}  // end of namespace intel

int intel::call_arg::calculate(const std::vector<COMPILER::tac*>& code)
{
  using namespace std;
  using namespace COMPILER;
  int n = 0;
  ellipsised.clear();
  typedef vector<tac*>::const_iterator IT;
  for ( IT p = code.begin(); p != code.end(); ) {
    IT q = find_if(p, code.end(), bind2nd(ptr_fun(cmp_id), tac::CALL));
    if (q == code.end())
      break;
    int m = x64 ? count_if(p, q, bind2nd(ptr_fun(cmp_id), tac::PARAM)) * 8 :
      accumulate(p, q, 0, param_size);
    call3ac* r = static_cast<call3ac*>(*q);
    const type* T = r->y->m_type;
    T = T->unqualified();
    if (T->m_id == type::POINTER) {
      const pointer_type* pt = static_cast<const pointer_type*>(T);
      T = pt->referenced_type();
    }
    T = T->unqualified();
    const func_type* ft = static_cast<const func_type*>(T);
    T = ft->return_type();
#ifdef CXX_GENERATOR
    if (!T) {
      p = q + 1;
      continue;
    }
#endif // CXX_GENERATOR
    T = T->complete_type();
    if ( T->aggregate() )
      m += x64 ? 8 : 4;
    n = max(n, m);
    if ( x64 ){
      const vector<const type*>& param = ft->param();
      vector<const type*>::const_iterator it = find_if(param.begin(), param.end(), isEllipsis);
      if (it != param.end()) {
        int x = distance(param.begin(), it);
        copy_if(p, q, inserter(ellipsised, ellipsised.begin()), isEllipsised(x));
        n = max(n, 4 * 8);  // allocate for %rcx, %rdx, %r8, %r9
      }
    }
    p = q + 1;
  }

  return n;
}

bool intel::call_arg::isEllipsised::operator()(COMPILER::tac* tac)
{
  using namespace COMPILER;
  if (!cmp_id(tac, tac::PARAM))
    return false;
  if ( --nth > 0 )
    return false;
  return true;
}

int intel::call_arg::param_size(int n, COMPILER::tac* ptr)
{
  using namespace COMPILER;
  if ( !cmp_id(ptr, tac::PARAM) )
    return n;
  const type* T = ptr->y->m_type;
  T = T->promotion();
  T = T->complete_type();
  return n + T->size();
}

int intel::param_decide(int offset, COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  map<var*, address*>& table = address_descriptor.second;
  table[u] = new stack(u,offset);
  if (x64)
    offset += 8;
  else {
    const type* T = u->m_type;
    T = T->promotion();
    T = T->complete_type();
    offset += T->size();
  }
  return offset;
}

int intel::param_save(int nth, COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  assert(nth < 4);
  const map<var*, address*>& m = address_descriptor.second;
  map<var*, address*>::const_iterator p = m.find(u);
  assert(p != m.end());
  address* stack = p->second;
  const type* T = u->m_type;
  string reg = param_reg(nth, T);
  int size = T->size();
  char sf = T->integer() ? suffix(size) : psuffix();
  out << '\t' << "mov";
  if (mode == MS && T->real())
    out << 's' << ((size == 4) ? 's' : 'd');
  out << sf << '\t';
  if (mode == GNU)
    out << reg << ", " << stack->expr() << '\n';
  else
    out << stack->expr() << ", " << reg << '\n';
  return nth + 1;
}

std::string intel::param_reg(int nth, const COMPILER::type* T)
{
  using namespace std;
  assert(nth < 4);
  if (T && T->real() && T->size() <= 8) {
    ostringstream os;
    if (mode == GNU)
      os << '%';
    os << "xmm" << nth;
    return os.str();
  }

  int psz = psize();
  int size = psz;
  if (T) {
    size = T->size();
    size = T->scalar() && size <= 8 ? size : psz;
  }
        
  switch (nth) {
  case 0: return reg::name(reg::cx, size);
  case 1: return reg::name(reg::dx, size);
  case 2: return reg::name(reg::r8, size);
  case 3: return reg::name(reg::r9, size);
  }
  assert(0);
  return "";
}


namespace intel {
  using namespace std;
  using namespace COMPILER;
  int usr_local1(int offset, const pair<string, vector<usr*> >& entry);
  int usr_local2(int offset, usr* u);
  int decide_local(int offset, var* v);
} // end of namespace intel

int intel::local_variable(int n, COMPILER::scope* p)
{
  using namespace std;
  using namespace COMPILER;
  if ( p->m_id != scope::BLOCK)
    return n;
  const map<string, vector<usr*> >& usrs = p->m_usrs;
  n = accumulate(usrs.begin(),usrs.end(),n,usr_local1);
  block* b = static_cast<block*>(p);
  const vector<var*>& vars = b->m_vars;
  n = accumulate(vars.begin(),vars.end(),n,decide_local);
  const vector<scope*>& ch = p->m_children;
  return accumulate(ch.begin(), ch.end(),n,local_variable);
}

int intel::usr_local1(int offset, const std::pair<std::string,
                      std::vector<COMPILER::usr*> >& entry)
{
  using namespace std;
  using namespace COMPILER;
  const vector<usr*>& v = entry.second;
  return accumulate(v.begin(),v.end(),offset,usr_local2);
}

int intel::usr_local2(int offset, COMPILER::usr* u)
{
  using namespace COMPILER;
  usr::flag_t flag = u->m_flag;
  usr::flag_t mask = usr::flag_t(usr::EXTERN | usr::STATIC | usr::INLINE | usr::FUNCTION | usr::TYPEDEF);
  return flag & mask ? offset : decide_local(offset,u);
}

int intel::decide_local(int offset, COMPILER::var* v)
{
  using namespace std;
  using namespace COMPILER;
  usr* u = v->usr_cast();
  usr::flag_t f = u ? u->m_flag : usr::NONE;
  assert(!(f & usr::flag_t(usr::FUNCTION | usr::TYPEDEF)));
  map<var*, address*>& tbl = address_descriptor.second;
  if (f & usr::CONST_PTR) {
    assert(tbl.find(v) != tbl.end());
    return offset;
  }
  const type* T = v->m_type;
  int size = (f & usr::VL) ? psize() : T->size();
  offset += size;
  int al = (f & usr::VL) ? psize() : T->align();
  offset = align(offset, al);
  if (f & usr::VL){
    assert(tbl.find(v) == tbl.end());
    tbl[v] = new allocated(-offset);
    if ( !allocated::base ){
      offset += psize();
      allocated::base = offset;
    }
  }
  else {
    if (tbl.find(v) == tbl.end())
      tbl[v] = new stack(v, -offset);
  }
  return offset;
}

namespace intel {
  using namespace std;
  using namespace COMPILER;
  struct gencode_table : map<tac::id_t, void (*)(tac*)> {
    gencode_table();
  } gencode_table;
#ifdef CXX_GENERATOR
  namespace except {
      namespace ms {
          namespace x64_handler {
              const string postfix = "$catch_end";
              namespace catch_code {
                  vector<tac*>* ptr;
                  bool flag;
                  void gen()
                  {
                      if (!ptr)
                          return;
                      auto_ptr<vector<tac*> > sweeper1(ptr);
                      sec_hlp sweeper2(CODE);
                      string label = catch_code::pre + func_label + catch_code::post;
                      out << label;
                      out << ' ' << "PROC	FRAME" << '\n';
                      out << '\t' << "mov	QWORD PTR [rsp+8], rcx" << '\n';
                      out << '\t' << ".savereg rcx, 8" << '\n';
                      out << '\t' << "mov	QWORD PTR [rsp+16], rdx" << '\n';
                      out << '\t' << ".savereg rdx, 16" << '\n';
                      out << '\t' << "push	rbp" << '\n';
                      out << '\t' << ".pushreg rbp" << '\n';
                      out << '\t' << "push	rdi" << '\n';
                      out << '\t' << ".pushreg rdi" << '\n';
                      out << '\t' << "sub	rsp, 40" << '\n';
                      out << '\t' << ".allocstack 40" << '\n';
                      out << '\t' << ".endprolog" << '\n';
                      out << '\t' << "lea	rbp, QWORD PTR [rdx+72]" << '\n';
                      for_each(begin(*ptr), end(*ptr), gencode);
                      out << '\t' << "npad	1" << '\n';
                      out << '\t' << "lea	rax, " << func_label << postfix << '\n';
                      out << '\t' << "add	rsp, 40" << '\n';
                      out << '\t' << "pop	rdi" << '\n';
                      out << '\t' << "pop	rbp" << '\n';
                      out << '\t' << "ret	0" << '\n';
                      out << '\t' << "int	3" << '\n';
                      out << label << ' ' << "ENDP" << '\n';
                  }
              } // end of namespace catch_code;
          } // end of namespace x64_handler
      } // end of namespace ms
  } // end of namespace except
#endif // CXX_GENERATOR
} // end of namespace intel

void intel::gencode(COMPILER::tac* ptr)
{
  using namespace std;
  if (debug_flag) {
    out << '\t' << comment_start << " ";
    output3ac(out,ptr);
    out << '\n';
  }
#ifdef CXX_GENERATOR
  if (except::ms::x64_handler::catch_code::flag) {
      if (ptr->m_id == tac::CATCH_END) {
          except::ms::x64_handler::catch_code::flag = false;
          gencode_table[ptr->m_id](ptr);
      }
      else
          except::ms::x64_handler::catch_code::ptr->push_back(ptr);
  }
  else
    gencode_table[ptr->m_id](ptr);
#else // CXX_GENERATOR
  gencode_table[ptr->m_id](ptr);
#endif // CXX_GENERATOR
}

namespace intel {
  using namespace COMPILER;
  void assign(tac*);
  void add(tac*);
  void sub(tac*);
  void mul(tac*);
  void div(tac*);
  void mod(tac*);
  void lsh(tac*);
  void rsh(tac*);
  void _and(tac*);
  void _xor(tac*);
  void _or(tac*);
  void uminus(tac*);
  void tilde(tac*);
  void cast(tac*);
  void addr(tac*);
  void invladdr(tac*);
  void invraddr(tac*);
  void param(tac*);
  void call(tac*);
  void _return(tac*);
  void _goto(tac*);
  void to(tac*);
  void loff(tac*);
  void roff(tac*);
  void _alloca_(tac*);
  void _asm_(tac*);
  void _va_start(tac*);
  void _va_arg(tac*);
  void _va_end(tac*);
#ifdef CXX_GENERATOR
  void alloce(tac*);
  void throwe(tac*);
  void rethrow(tac*);
  void try_begin(tac*);
  void try_end(tac*);
  void here(tac*);
  void here_reason(tac*);
  void here_info(tac*);
  void there(tac*);
  void unwind_resume(tac*);
  void catch_begin(tac*);
  void catch_end(tac*);
#endif // CXX_GENERATOR
} // end of namespace intel

intel::gencode_table::gencode_table()
{
  using namespace COMPILER;
  (*this)[tac::ASSIGN] = assign;
  (*this)[tac::ADD] = add;
  (*this)[tac::SUB] = sub;
  (*this)[tac::MUL] = mul;
  (*this)[tac::DIV] = div;
  (*this)[tac::MOD] = mod;
  (*this)[tac::LSH] = lsh;
  (*this)[tac::RSH] = rsh;
  (*this)[tac::AND] = _and;
  (*this)[tac::XOR] = _xor;
  (*this)[tac::OR] = _or;
  (*this)[tac::UMINUS] = uminus;
  (*this)[tac::TILDE] = tilde;
  (*this)[tac::CAST] = cast;
  (*this)[tac::ADDR] = addr;
  (*this)[tac::INVLADDR] = invladdr;
  (*this)[tac::INVRADDR] = invraddr;
  (*this)[tac::PARAM] = param;
  (*this)[tac::CALL] = call;
  (*this)[tac::RETURN] = _return;
  (*this)[tac::GOTO] = _goto;
  (*this)[tac::TO] = to;
  (*this)[tac::LOFF] = loff;
  (*this)[tac::ROFF] = roff;
  (*this)[tac::ALLOCA] = _alloca_;
  (*this)[tac::ASM] = _asm_;
  (*this)[tac::VASTART] = _va_start;
  (*this)[tac::VAARG] = _va_arg;
  (*this)[tac::VAEND] = _va_end;
#ifdef CXX_GENERATOR
  (*this)[tac::ALLOCE] = alloce;
  (*this)[tac::THROW] = throwe;
  (*this)[tac::RETHROW] = rethrow;
  (*this)[tac::TRY_BEGIN] = try_begin;
  (*this)[tac::TRY_END] = try_end;
  (*this)[tac::HERE] = here;
  (*this)[tac::HERE_REASON] = here_reason;
  (*this)[tac::HERE_INFO] = here_info;
  (*this)[tac::THERE] = there;
  (*this)[tac::UNWIND_RESUME] = unwind_resume;
  (*this)[tac::CATCH_BEGIN] = catch_begin;
  (*this)[tac::CATCH_END] = catch_end;
#endif // CXX_GENERATOR
}

namespace intel { namespace assign_impl {
  void single(COMPILER::tac*);
  void multi(COMPILER::tac*);
} } // end of namespace assign_impl and intel

void intel::assign(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace assign_impl;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64)
    (T->scalar() && size <= 8) ? single(tac) : multi(tac);
  else
    T->scalar() ? single(tac) : multi(tac);
}

void intel::assign_impl::single(COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load();
  x->store();
}

namespace intel {
  void copy(address* dst, address* src, int size);
} // end of namespace intel

void intel::assign_impl::multi(COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  address* y = getaddr(tac->y);
  address* x = getaddr(tac->x);
  char ps = psuffix();
  if (x64) {
    bool bx = (tac->x->m_scope->m_id == scope::PARAM);
    bool by = (tac->y->m_scope->m_id == scope::PARAM);
    if (bx && by) {
      if (mode == GNU) {
        out << '\t' << "mov" << ps << '\t' << x->expr() << ", %rax" << '\n';
        out << '\t' << "mov" << ps << '\t' << y->expr() << ", %rbx" << '\n';
      }
      else {
        out << '\t' << "mov" << '\t' << "rax, " << x->expr() << '\n';
        out << '\t' << "mov" << '\t' << "rbx, " << y->expr() << '\n';
      }
      copy(0, 0, size);
    }
    else if (bx) {
      if (mode == GNU)
        out << '\t' << "mov" << ps << '\t' << x->expr() << ", %rax" << '\n';
      else
        out << '\t' << "mov" << '\t' << "rax, " << x->expr() << '\n';              
      copy(0, y, size);
    }
    else if (by) {
      if (mode == GNU)
        out << '\t' << "mov" << ps << '\t' << y->expr() << ", %rax" << '\n';
      else
        out << '\t' << "mov" << '\t' << "rax, " << y->expr() << '\n';
      copy(x, 0, size);
    }
    else
      copy(x, y, size);
  }
  else {
    copy(x, y, size);
  }
}

namespace intel {
  void copy(address* dst, address* src, int offset, int size);
} // end of namespace intel

void intel::copy(address* dst, address* src, int size)
{
  int offset = 0;
  while (size) {
    if (x64 && size >= 8) {
      copy(dst, src, offset, 8);
      offset += 8, size -= 8;
    }
    else if (size >= 4) {
      copy(dst, src, offset, 4);
      offset += 4, size -= 4;
    }
    else if (size >= 2) {
      copy(dst, src, offset, 2);
      offset += 2, size -= 2;
    }
    else if (size >= 1) {
      copy(dst, src, offset, 1);
      offset += 1, size -= 1;
    }
  }
}

void intel::copy(address* dst, address* src, int offset, int size)
{
  using namespace std;
  char sf = suffix(size);
  int psz = psize();
  string ptr = (mode == MS) ? (ms_pseudo(size) + " PTR ") : "";
  if (dst && src) {
    string ax = reg::name(reg::ax, size);
    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << src->expr(offset) << ", " << ax << '\n';
      out << '\t' << "mov" << sf << '\t' << ax << ", " << dst->expr(offset) << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << ax << ", " << ptr << src->expr(offset) << '\n';
      out << '\t' << "mov" << '\t' << ptr << dst->expr(offset) << ", " << ax << '\n';
    }
  }
  else if ( dst ) {
    string ax = reg::name(reg::ax, psz);
    string bx = reg::name(reg::bx, size);
    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << offset << '(' << ax << "), " << bx << '\n';
      out << '\t' << "mov" << sf << '\t' << bx << ", " << dst->expr(offset) << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << bx << ", " << ptr << offset << '[' << ax << ']' << '\n';
      out << '\t' << "mov" << '\t' << ptr << dst->expr(offset) << ", " << bx << '\n';
    }
  }
  else if ( src ){
    string ax = reg::name(reg::ax, psz);
    string bx = reg::name(reg::bx, size);
    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << src->expr(offset) << ", " << bx << '\n';
      out << '\t' << "mov" << sf << '\t' << bx << ", " << offset << '(' << ax << ')' << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << bx << ", " << ptr << src->expr(offset) << '\n';
      out << '\t' << "mov" << '\t' << ptr << offset << '[' << ax << ']' << ", " << bx << '\n';
    }
  }
  else {
    string ax = reg::name(reg::ax, psz);
    string bx = reg::name(reg::bx, psz);
    string cx = reg::name(reg::cx, size);
    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << offset << '(' << bx << "), " << cx << '\n';
      out << '\t' << "mov" << sf << '\t' << cx << ", " << offset << '(' << ax << ')' << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << cx << ", " << ptr << offset << '[' << bx << ']' << '\n';
      out << '\t' << "mov" << '\t' << ptr << offset << '[' << ax << ']' << ", " << cx << '\n';
    }
  }
}

namespace intel { namespace binary {
  void integer(COMPILER::tac* tac, std::string inst, std::string op2);
  void real(COMPILER::tac* tac, std::string inst, std::string ld_inst);
} } // end of namespace binary and intel

void intel::add(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  string inst = x64 ? "adds" : "fadd";
  T->real() ? binary::real(tac, inst, "faddp") : binary::integer(tac, "add", "adc");
}

void intel::sub(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  string inst = x64 ? "subs" : "fsub";
  T->real() ? binary::real(tac, inst, "fsubp") : binary::integer(tac, "sub", "sbb");
}

namespace intel { namespace mul_impl {
  void longlong_x86(COMPILER::tac*);
} }  // end of namespace mul_impl, intel

void intel::mul(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  string inst = x64 ? "muls" : "fmul";
  if ( T->real() )
    return binary::real(tac, inst, "fmulp");
  int size = T->size();
  if ( x64 || size <= 4 )
    return binary::integer(tac, "imul", "");
  assert(!x64 && size == 8);
  mul_impl::longlong_x86(tac);
}

namespace intel {
  namespace mul_impl {
    namespace longlong_x86_impl {
      void GNU_subr(COMPILER::tac* tac);
      void MS_subr(COMPILER::tac* tac);
    } // end of namespace longlong_x86_impl
  } // end of namespace mul_impl
} // end of namespace intel

void intel::mul_impl::longlong_x86(COMPILER::tac* tac)
{
  using namespace longlong_x86_impl;
  mode == GNU ? GNU_subr(tac) : MS_subr(tac);
}

void intel::mul_impl::longlong_x86_impl::GNU_subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  out << '\t' << "movl" << '\t' << y->expr(0) << ", %eax" << '\n';
  out << '\t' << "movl" << '\t' << y->expr(4) << ", %ebx" << '\n';
  out << '\t' << "movl" << '\t' << z->expr(0) << ", %edx" << '\n';
  out << '\t' << "movl" << '\t' << z->expr(4) << ", %ecx" << '\n';

  out << '\t' << "imull        %eax, %ecx" << '\n';
  out << '\t' << "imull        %edx, %ebx" << '\n';
  out << '\t' << "mull        %edx" << '\n';
  out << '\t' << "addl        %ebx, %ecx" << '\n';
  out << '\t' << "leal        (%ecx,%edx), %edx" << '\n';

  address* x = getaddr(tac->x);
  x->store();
}

namespace intel {
  namespace longlong_impl {
    void setarg(COMPILER::tac* tac);
  } // end of namespace longlong_impl
} // end of namespace intel

void intel::mul_impl::longlong_x86_impl::MS_subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  longlong_impl::setarg(tac);
  string fun = "__allmul";
  out << '\t' << "call" << '\t' << fun << '\n';

  mem::refed.insert(mem::refgen_t(fun, usr::FUNCTION, 0));
  
  address* x = getaddr(tac->x);
  x->store();
}

void intel::longlong_impl::setarg(COMPILER::tac* tac)
{
  assert(mode == MS);
  
  address* z = getaddr(tac->z);
  out << '\t' << "mov" << '\t' << "eax, " << z->expr(4, true) << '\n';
  out << '\t' << "push" << '\t' << "eax" << '\n';
  out << '\t' << "mov" << '\t' << "ecx, " << z->expr(0, true) << '\n';
  out << '\t' << "push" << '\t' << "ecx" << '\n';

  address* y = getaddr(tac->y);
  out << '\t' << "mov" << '\t' << "edx, " << y->expr(4, true) << '\n';
  out << '\t' << "push" << '\t' << "edx" << '\n';
  out << '\t' << "mov" << '\t' << "eax, " << y->expr(0, true) << '\n';
  out << '\t' << "push" << '\t' << "eax" << '\n';
}

namespace intel { namespace div_impl {
  void integer(COMPILER::tac* tac, bool div /* div or mod */);
} } // end of namespace div_impl and intel

void intel::div(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  string inst = x64 ? "divs" : "fdiv";
  T->real() ? binary::real(tac, inst, "fdivp") : div_impl::integer(tac, true);
}

namespace intel { namespace div_impl {
    void longlong_x86(COMPILER::tac* tac, bool div);
} } // end of namesapce div_impl, intel

void intel::div_impl::integer(COMPILER::tac* tac, bool div)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  std::string op;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64 && size == 8) {
    y->load(reg::ax);    
    if (mode == GNU) {
      char sf = psuffix();
      z->load(reg::dx);
      out << '\t' << "mov" << sf << '\t' << "%rdx, %r8" << '\n';
      if (T->_signed()) {
        out << '\t' << "cqto" << '\n';
        op = "idiv";
        op += sf;
      }
      else {
        out << '\t' << "xorl" << '\t' << "%edx, %edx" << '\n';
        op = "div";
        op += sf;
      }
      out << '\t' << op << '\t' << "%r8" << '\n';
    }
    else {
      if (T->_signed()) {
        out << '\t' << "cqo" << '\n';
        out << '\t' << "idiv" << '\t' << z->expr() << '\n';
      }
      else {
        out << '\t' << "xor" << '\t' << "edx,edx" << '\n';
        out << '\t' << "div" << '\t' << z->expr() << '\n';
      }
    }
  }
  else if ( !x64 && size == 8 )
    return div_impl::longlong_x86(tac, div);
  else {
    y->load(reg::ax);    
    char sf = suffix(4);
    if (T->_signed()) {
      out << '\t' << (mode == GNU ? "cltd" : "cdq") << '\n';
      op = "idiv"; op += sf;
    }
    else {
      if (mode == GNU)
        out << '\t' << "mov" << sf << '\t' << "$0, %edx" << '\n';
      else
        out << '\t' << "mov" << sf << '\t' << "edx, 0" << '\n';
      op = "div"; op += sf;
    }
    if (z->m_id == address::IMM) {
      if (mode == GNU) {
        out << '\t' << "mov" << sf << '\t' << z->expr() << ", %ecx" << '\n';
        out << '\t' << op << '\t' << "%ecx, %eax" << '\n';
      }
      else {
        out << '\t' << "mov" << sf << '\t' << "ecx, " << z->expr() << '\n';
        out << '\t' << op << '\t' << "ecx" << '\n';
      }
    }
    else {
      if (mode == GNU)
        out << '\t' << op << '\t' << z->expr() << ", %eax" << '\n';
      else
        out << '\t' << op << '\t' << z->expr() << '\n';
    }
  }
  x->store(div ? reg::ax : reg::dx);
}

namespace intel {
  namespace div_impl {
    namespace longlong_x86_impl {
      void GNU_subr(COMPILER::tac* tac, bool div);
      void MS_subr(COMPILER::tac* tac, bool div);
    } // end of namespace longlong_x86_impl
  } // end of namespace div_impl
} // end of namespace intel

void intel::div_impl::longlong_x86(COMPILER::tac* tac, bool div)
{
  using namespace longlong_x86_impl;
  mode == GNU ? GNU_subr(tac, div) : MS_subr(tac, div);
}

void intel::div_impl::longlong_x86_impl::GNU_subr(COMPILER::tac* tac, bool div)
{
  using namespace std;
  using namespace COMPILER;
  const type *T = tac->x->m_type;
  string fun;
  if ( T->_signed() )
    fun = div ? "__divdi3" : "__moddi3";
  else
    fun = div ? "__udivdi3" : "__umoddi3";

  out << '\t' << "subl        $24, %esp" << '\n';
  {
    address* y = getaddr(tac->y);
    y->load();
    out << '\t' << "movl" << '\t' << "%eax, 0(%esp)" << '\n';
    out << '\t' << "movl" << '\t' << "%edx, 4(%esp)" << '\n';
    address* z = getaddr(tac->z);
    z->load();
    out << '\t' << "movl" << '\t' << "%eax, 8(%esp)" << '\n';
    out << '\t' << "movl" << '\t' << "%edx,12(%esp)" << '\n';
    out << '\t' << "call" << '\t' << fun << '\n';
  }
  out << '\t' << "addl        $24, %esp" << '\n';
  address* x = getaddr(tac->x);
  x->store();
}

void intel::div_impl::longlong_x86_impl::MS_subr(COMPILER::tac* tac, bool div)
{
  using namespace std;
  using namespace COMPILER;
  const type *T = tac->x->m_type;
  string fun;
  if ( T->_signed() )
    fun = div ? "__alldiv" : "__allrem";
  else
    fun = div ? "__aulldiv" : "__aullrem";

  longlong_impl::setarg(tac);

  out << '\t' << "call" << '\t' << fun << '\n';
  mem::refed.insert(mem::refgen_t(fun, usr::FUNCTION, 0));

  address* x = getaddr(tac->x);
  x->store();
}

void intel::mod(COMPILER::tac* tac)
{
  div_impl::integer(tac, false);
}

namespace intel { namespace shift {
  void notlonglongx86(COMPILER::tac*, std::string);
  void longlongx86(COMPILER::tac*, const std::string [5], int);
} } // end of namespace shift and intel

void intel::lsh(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64)
    shift::notlonglongx86(tac, "sal");
  else {
    if (size > 4) {
      string eax = reg::name(reg::ax, 4);
      string edx = reg::name(reg::dx, 4);
      string op[] = { "shld", eax, edx, "sal", "mov" };
      shift::longlongx86(tac, &op[0], 0);
    }
    else
      shift::notlonglongx86(tac, "sal");
  }
}

void intel::rsh(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64)
    shift::notlonglongx86(tac, T->_signed() ? "sar" : "shr");
  else {
    if (size > 4) {
      string eax = reg::name(reg::ax, 4);
      string edx = reg::name(reg::dx, 4);
      if (!T->_signed()) {
        string op[] = { "shrd", edx, eax, "shr", "mov" };
        shift::longlongx86(tac, &op[0], 0);
      }
      else {
        string op[] = { "shrd", edx, eax, "sar", "sar" };
        shift::longlongx86(tac, &op[0], 31);
      }
    }
    else
      shift::notlonglongx86(tac, T->_signed() ? "sar" : "shr");
  }
}

void intel::shift::notlonglongx86(COMPILER::tac* tac, std::string op)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  const type* Tx = tac->x->m_type;
  int sx = Tx->size();
  string ax = reg::name(reg::ax, sx);
  char sf = suffix(sx);
  if ( z->m_id == address::IMM ) {
    y->load(reg::ax);
    if (mode == GNU)
      out << '\t' << op << sf << '\t' << z->expr() << ", " << ax << '\n';
    else
      out << '\t' << op << '\t' << ax << ", " << z->expr() << '\n';
  }
  else {
    const type* Tz = tac->z->m_type;
    int sz = Tz->size();
    string ecx = reg::name(reg::cx, 4);
    char sf2 = suffix(4);
    if (sz > 4) {
      if (mode == GNU)
        out << '\t' << "mov" << sf2 << '\t' << z->expr() << ", " << ecx << '\n';
      else
        out << '\t' << "mov" << '\t' << ecx << ", " << z->expr(0, true) << '\n';
    }
    else
      z->load(reg::cx);

    y->load(reg::ax);
    string cl = reg::name(reg::cx, 1);
    if (mode == GNU)
      out << '\t' << op << sf << '\t' << cl << ", " << ax << '\n';
    else
      out << '\t' << op << '\t' << ax << ", " << cl << '\n';
  }
  x->store(reg::ax);
}

void intel::shift::longlongx86(COMPILER::tac* tac, const std::string op[5], int n)
{
  using namespace std;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load(reg::ax);
  char sf = suffix(4);
  string ecx = reg::name(reg::cx, 4);
  string cl = reg::name(reg::cx, 1);

  if (mode == GNU)
    out << '\t' << "mov" << sf << '\t' << z->expr() << ", " << ecx << '\n';
  else
    out << '\t' << "mov" << '\t' << ecx << ", " << z->expr(0, true) << '\n';

  if (mode == GNU)
    out << '\t' << op[0] << sf << '\t' << cl << ", " << op[1] << ", " << op[2] << '\n';
  else
    out << '\t' << op[0] << '\t' << op[2] << ", " << op[1] << ", " << cl << '\n';

  if (mode == GNU)
    out << '\t' << op[3] << sf << '\t' << cl << ", " << op[1] << '\n';
  else
    out << '\t' << op[3] << '\t' << op[1] << ", " << cl << '\n';

  if (mode == GNU)
    out << '\t' << "testb"  << '\t' << "$32" << ", " << cl << '\n';
  else
    out << '\t' << "test" << '\t' << cl << ", " << 32 << '\n';

  string label = new_label((mode == GNU) ? ".label" : "label$");
  out << '\t' << "je" << '\t' << label << '\n';
  if (mode == GNU)
    out << '\t' << "mov" << sf << '\t' << op[1] << ", " << op[2] << '\n';
  else
    out << '\t' << "mov" << '\t' << op[2] << ", " << op[1] << '\n';

  if (mode == GNU)
    out << '\t' << op[4] << sf << '\t' << '$' << n << ", " << op[1] << '\n';
  else
    out << '\t' << op[4] << '\t' << op[1] << ", " << n << '\n';

  out << label << ":\n";
  x->store(reg::ax);
}

void intel::_and(COMPILER::tac* tac)
{
  binary::integer(tac,"and", "and");
}

void intel::_xor(COMPILER::tac* tac)
{
  binary::integer(tac, "xor", "xor");
}

void intel::_or(COMPILER::tac* tac)
{
  binary::integer(tac, "or", "or");
}

void intel::binary::integer(COMPILER::tac* tac, std::string op, std::string op2)
{
  using namespace std;
  using namespace COMPILER;
  const type* Ty = tac->y->m_type;
  const type* Tz = tac->z->m_type;
  int sy = Ty->size();
  int sz = Tz->size();
  if ( sy == sz ){
    address* y = getaddr(tac->y);
    address* z = getaddr(tac->z);
    y->load(reg::ax);
    if (x64 || sy <= 4) {
      out << '\t' << op << suffix(sy) << '\t';
      if (mode == GNU)
        out << z->expr() << ", " << reg::name(reg::ax, sy) << '\n';
      else
        out << reg::name(reg::ax, sy) << ", " << z->expr() << '\n';
    }
    else {
      assert(!x64 && sy == 8);
      if (mode == GNU) {
        out << '\t' << op  << 'l' << '\t' << z->expr(0) << ", %eax" << '\n';
        out << '\t' << op2 << 'l' << '\t' << z->expr(4) << ", %edx" << '\n';
      }
      else {
        out << '\t' << op  << '\t' << "eax, " << z->expr(0, true) << '\n';
        out << '\t' << op2 << '\t' << "edx, " << z->expr(4, true) << '\n';
      }
    }
    address* x = getaddr(tac->x);
    return x->store(reg::ax);
  }

  if (x64 && (sy == 8 && sz == 4 || sy == 4 && sz == 8)){
    address* y = getaddr(tac->y);
    address* z = getaddr(tac->z);
    if ( sy == 8 ){
      y->load(reg::ax);
      z->load(reg::bx);
    }
    else {
      y->load(reg::bx);
      z->load(reg::ax);
    }
        string rax = reg::name(reg::ax, 8);
        string rbx = reg::name(reg::bx, 8);
        string ebx = reg::name(reg::bx, 4);
        string sf; sf += suffix(4); sf += suffix(8);
        if (mode == GNU) {
          out << '\t' << "movslq" << '\t' << ebx << ", " << rbx << '\n';
          out << '\t' << op << suffix(8) << '\t' << rbx << ", " << rax << '\n';
        }
        else {
          out << '\t' << "movsxd" << '\t' << rbx << ", " << ebx << '\n';
          out << '\t' << op << suffix(8) << '\t' << rax << ", " << rbx << '\n';
        }
    address* x = getaddr(tac->x);
    return x->store(reg::ax);
  }

  assert(!x64);
  const type* Tx = tac->x->m_type;
  int sx = Tx->size();
  if ( Tz->integer() ){
    assert(sx == 4 && sy == 4 && sz == 8);
    address* y = getaddr(tac->y);
    address* z = getaddr(tac->z);
    y->load(reg::bx);
    z->load(reg::ax);
    if (mode == GNU)
      out << '\t' << op  << 'l' << '\t' << "%eax, %ebx" << '\n';
    else
      out << '\t' << op  << '\t' << "ebx, eax" << '\n';
    address* x = getaddr(tac->x);
    return x->store(reg::bx);
  }

  assert(Ty->integer() && sx == 4 && sy ==8 && sz == 4);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load(reg::ax);
  out << '\t' << op << 'l' << '\t' << z->expr() << ", %eax" << '\n';
  address* x = getaddr(tac->x);
  return x->store(reg::ax);
 }

namespace intel {
  void fld(COMPILER::var* v)
  {
    using namespace std;
    using namespace COMPILER;
    address* a = getaddr(v);
    const type* T = v->m_type;
    int size = T->size();
    char suffix = fsuffix(size);
    string expr = a->expr();
    char ps = psuffix();
    int psz = psize();
    string SP = sp();

    if (v->isconstant() && size == 4) {
      out << '\t' << "push" << ps << '\t' << expr << '\n';
      if (mode == GNU) {
        out << '\t' << "fld" << suffix << '\t' << '(' << SP << ')' << '\n';
        out << '\t' << "add" << ps << '\t' << '$' << psz << ", " << SP << '\n';
      }
      else {
        out << '\t' << "fld" << suffix << '\t' << "DWORD PTR " << '[' << SP << ']' << '\n';
        out << '\t' << "add" << ps << '\t' << SP << ", " << psz << '\n';
      }
    }
    else if (x64 && v->m_scope->m_id == scope::PARAM && size == 16) {
      string rax = reg::name(reg::ax, psz);
      if (mode == GNU) {
        out << '\t' << "mov" << ps << '\t' << expr << ", " << rax << '\n';
        out << '\t' << "fld" << suffix << '\t' << '(' << rax << ')' << '\n';
      }
      else {
        out << '\t' << "mov" << ps << '\t' << rax << ", " << expr << '\n';
        out << '\t' << "fld" << '\t' << ms_pseudo(size) << " PTR " << '[' << rax << ']' << '\n';
      }
    }
    else {
      if (mode == GNU)
        out << '\t' << "fld" << suffix << '\t' << expr << '\n';
      else {
        if (x64)
          out << '\t' << "fld" << '\t' << expr << '\n';
        else {
          if (a->m_id == address::MEM) {
            a->get(reg::ax);
            out << '\t' << "fld" << '\t' << ms_pseudo(size) << " PTR [eax]" << '\n';
          }
          else
            out << '\t' << "fld" << '\t' << expr << '\n';
        }
      }
    }
  }
  void fstp(COMPILER::var* v)
  {
    using namespace std;
    using namespace COMPILER;
    address* a = getaddr(v);
    const type* T = v->m_type;
    int size = T->size();
    char suffix = fsuffix(size);
    string expr = a->expr();
    if (x64 && v->m_scope->m_id == scope::PARAM && size == 16) {
      string rax = reg::name(reg::ax, psize());
      out << '\t' << "mov" << psuffix() << '\t' << expr << ", " << rax << '\n';
      out << '\t' << "fstp" << suffix << '\t' << '(' << rax << ')' << '\n';
    }
    else
      out << '\t' << "fstp" << suffix << '\t' << expr << '\n';
  }
} // end of namespace intel

void intel::binary::real(COMPILER::tac* tac, std::string inst, std::string ld_inst)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64 && (size == 4 || size == 8)) {
    address* z = getaddr(tac->z);
    z->load();
    out << '\t' << "mov";
    if (mode == MS)
      out << "sd";
    out << psuffix() << '\t';
    if (mode == GNU)
      out << xmm(0) << ", " << xmm(1);
    else
      out << xmm(1) << ", " << xmm(0);
    out << '\n';
    address* y = getaddr(tac->y);
    y->load();
    out << '\t' << inst;
    out << (size == 4 ? 's' : 'd');
    if (mode == GNU)
      out << '\t' << xmm(1) << ", " << xmm(0);
    else
      out << '\t' << xmm(0) << ", " << xmm(1);
    out << '\n';
    address* x = getaddr(tac->x);
    return x->store();
  }

  if (x64 || mode == GNU) { 
    fld(tac->z);
    fld(tac->y);
  }
  else {
    fld(tac->y);
    fld(tac->z);
  }
        
  if (x64) {
    if (mode == GNU)
      out << '\t' << ld_inst << '\t' << "%st, %st(1)" << '\n';
    else
      out << '\t' << ld_inst << '\t' << "st(1), st" << '\n';
  }
  else {
    if (mode == GNU)
      out << '\t' << inst << 'p' << '\t' << "%st, %st(1)" << '\n';
    else
      out << '\t' << inst << 'p' << '\t' << "st(1), st" << '\n';
  }
  fstp(tac->x);
}

namespace intel { namespace unary {
  void integer(COMPILER::tac* tac, std::string op);
  void real(COMPILER::tac* tac);
} } // end of namespace unary and intel

void intel::uminus(COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  T->real() ? unary::real(tac) : unary::integer(tac, "neg");
}

void intel::tilde(COMPILER::tac* tac)
{
  unary::integer(tac, "not");
}

void intel::unary::integer(COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load(reg::ax);
  const type* T = tac->x->m_type;
  int size = T->size();
  if ( x64 || size <= 4 )
    out << '\t' << op << '\t' << reg::name(reg::ax, size) << '\n';
  else {
    assert(!x64 && size == 8);
    if (mode == GNU)
      out << '\t' << op << '\t' << "%eax" << '\n';
    else
      out << '\t' << op << '\t' << "eax" << '\n';
    if ( op == "neg" ) {
      if (mode == GNU)
        out << '\t' << "adcl" << '\t' << "$0, %edx" << '\n';
      else
        out << '\t' << "adc" << '\t' << "edx, 0" << '\n';
    }
    if (mode == GNU)
      out << '\t' << op << '\t' << "%edx" << '\n';
    else
      out << '\t' << op << '\t' << "edx" << '\n';
  }
  x->store(reg::ax);
}

void intel::unary::real(COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64 && (size == 4 || size == 8)) {
    if (mode == MS) {
      if (size == 4) {
        if (uminus_float_t::obj.m_label.empty())
          uminus_float_t::obj.m_label = new_label("LC$");
        out << '\t' << "movss" << '\t' << "xmm1, DWORD PTR " << uminus_float_t::obj.m_label << '\n';
      }
      else {
        if (uminus_double_t::obj.m_label.empty())
          uminus_double_t::obj.m_label = new_label("LC$");
        out << '\t' << "movsd" << '\t' << "xmm1, QWORD PTR " << uminus_double_t::obj.m_label << '\n';
      }
    }
    address* y = getaddr(tac->y);
    y->load();
    out << '\t' << "xorp";
    out << (size == 4 ? 's' : 'd');
    out << '\t';
    if (mode == GNU) {
      if (size == 4) {
        if (uminus_float_t::obj.m_label.empty())
          uminus_float_t::obj.m_label = new_label(".LC");
        out << uminus_float_t::obj.m_label;
      }
      else {
        if (uminus_double_t::obj.m_label.empty())
          uminus_double_t::obj.m_label = new_label((mode == GNU) ? ".LC" : "LC$");
        out << uminus_double_t::obj.m_label;
      }
      out << "(%rip), %xmm0" << '\n';
    }
    else
      out << "xmm0, xmm1" << '\n';

    address* x = getaddr(tac->x);
    return x->store();
  }

  fld(tac->y);
  out << '\t' << "fchs" << '\n';
  fstp(tac->x);
}

void intel::uminus_float_t::output_value() const
{
  if (mode == GNU)
    out << '\t' << ".align 16" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << '\t' << "2147483648" << '\n';
  out << '\t' << dot_long() << '\t' << 0 << '\n';
  out << '\t' << dot_long() << '\t' << 0 << '\n';
  out << '\t' << dot_long() << '\t' << 0 << '\n';
}

void intel::uminus_double_t::output_value() const
{
  if (mode == GNU)
    out << '\t' << ".align 16" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << '\t' << 0 << '\n';
  out << '\t' << dot_long() << '\t' << "-2147483648" << '\n';
  out << '\t' << dot_long() << '\t' << 0 << '\n';
  out << '\t' << dot_long() << '\t' << 0 << '\n';
}

namespace intel {
  using namespace std;
  using namespace COMPILER;
  struct cast_table : map<pair<int,int>, void (*)(tac*)> {
    static int id(const COMPILER::type*);
    cast_table();
  } m_cast_table;
} // end of namespace intel

void intel::cast(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  int x = cast_table::id(tac->x->m_type);
  int y = cast_table::id(tac->y->m_type);
  m_cast_table[make_pair(x,y)](tac);
}

int intel::cast_table::id(const COMPILER::type* T)
{
  int r = T->real() ? 1 : 0;
  int u = (T->integer() && !T->_signed()) ? 1 : 0;
  int s = T->size();
  return (s << 2) + (u << 1) + r;
}

namespace intel {
  void byte_real(COMPILER::tac*);
  void half_real(COMPILER::tac*);
  void longlong_notlonglong(COMPILER::tac*);
  void notlonglong_longlong(COMPILER::tac*);
  void notlonglong_notlonglong(COMPILER::tac*);
  void real_byte(COMPILER::tac*);
  void real_longlong(COMPILER::tac*);
  void real_real(COMPILER::tac*);
  void real_sint32(COMPILER::tac*);
  void real_uint32(COMPILER::tac*);
  void real_sint16(COMPILER::tac*);
  void real_uint16(COMPILER::tac*);
  void half_double(COMPILER::tac*);
  void sint32_real(COMPILER::tac*);
  void sint64_real(COMPILER::tac*);
  void uint32_real(COMPILER::tac*);
  void uint64_float(COMPILER::tac*);
  void uint64_double(COMPILER::tac*);
  void uint64_ld(COMPILER::tac*);
} // end of namespace intel

intel::cast_table::cast_table()
{
  using namespace std;

  // x := (char)y, x := (signed char)y
  (*this)[make_pair(4,4)] = assign_impl::single;
  (*this)[make_pair(4,6)] = assign_impl::single;
  (*this)[make_pair(4,8)] = notlonglong_notlonglong;
  (*this)[make_pair(4,10)] = notlonglong_notlonglong;
  (*this)[make_pair(4,16)] = notlonglong_notlonglong;
  (*this)[make_pair(4,18)] = notlonglong_notlonglong;
  (*this)[make_pair(4,32)] = notlonglong_longlong;
  (*this)[make_pair(4,34)] = notlonglong_longlong;
  (*this)[make_pair(4,17)] = byte_real;
  (*this)[make_pair(4,33)] = byte_real;
  (*this)[make_pair(4,49)] = (*this)[make_pair(4,65)] = byte_real;

  // x := (unsigned char)y
  (*this)[make_pair(6,4)] = assign_impl::single;
  (*this)[make_pair(6,8)] = notlonglong_notlonglong;
  (*this)[make_pair(6,10)] = notlonglong_notlonglong;
  (*this)[make_pair(6,16)] = notlonglong_notlonglong;
  (*this)[make_pair(6,18)] = notlonglong_notlonglong;
  (*this)[make_pair(6,32)] = notlonglong_longlong;
  (*this)[make_pair(6,34)] = notlonglong_longlong;
  (*this)[make_pair(6,17)] = byte_real;
  (*this)[make_pair(6,33)] = byte_real;
  (*this)[make_pair(6,49)] = (*this)[make_pair(6,65)] = byte_real;

  // x := (short int)y
  (*this)[make_pair(8,4)] = notlonglong_notlonglong;
  (*this)[make_pair(8,6)] = notlonglong_notlonglong;
  (*this)[make_pair(8,10)] = assign_impl::single;
  (*this)[make_pair(8,16)] = notlonglong_notlonglong;
  (*this)[make_pair(8,18)] = notlonglong_notlonglong;
  (*this)[make_pair(8,32)] = notlonglong_longlong;
  (*this)[make_pair(8,34)] = notlonglong_longlong;
  (*this)[make_pair(8,17)] = half_real;
  (*this)[make_pair(8,33)] = half_real;
  (*this)[make_pair(8,49)] = (*this)[make_pair(8,65)] = half_real;

  // x := (unsigned short int)y
  (*this)[make_pair(10,4)] = notlonglong_notlonglong;
  (*this)[make_pair(10,6)] = notlonglong_notlonglong;
  (*this)[make_pair(10,8)] = assign_impl::single;
  (*this)[make_pair(10,16)] = notlonglong_notlonglong;
  (*this)[make_pair(10,18)] = notlonglong_notlonglong;
  (*this)[make_pair(10,32)] = notlonglong_longlong;
  (*this)[make_pair(10,34)] = notlonglong_longlong;
  (*this)[make_pair(10,17)] = half_real;
  (*this)[make_pair(10,33)] = half_real;
  (*this)[make_pair(10,49)] = (*this)[make_pair(10,65)] = half_real;

  // x := (int)y
  (*this)[make_pair(16,4)] = notlonglong_notlonglong;
  (*this)[make_pair(16,6)] = notlonglong_notlonglong;
  (*this)[make_pair(16,8)] = notlonglong_notlonglong;
  (*this)[make_pair(16,10)] = notlonglong_notlonglong;
  (*this)[make_pair(16,16)] = assign_impl::single;
  (*this)[make_pair(16,18)] = assign_impl::single;
  (*this)[make_pair(16,32)] = notlonglong_longlong;
  (*this)[make_pair(16,34)] = notlonglong_longlong;
  (*this)[make_pair(16,17)] = sint32_real;
  (*this)[make_pair(16,33)] = sint32_real;
  (*this)[make_pair(16,49)] = (*this)[make_pair(16,65)] = sint32_real;

  // x := (unsigned int)y
  (*this)[make_pair(18,4)] = notlonglong_notlonglong;
  (*this)[make_pair(18,6)] = notlonglong_notlonglong;
  (*this)[make_pair(18,8)] = notlonglong_notlonglong;
  (*this)[make_pair(18,10)] = notlonglong_notlonglong;
  (*this)[make_pair(18,16)] = assign_impl::single;
  (*this)[make_pair(18,18)] = assign_impl::single;
  (*this)[make_pair(18,32)] = notlonglong_longlong;
  (*this)[make_pair(18,34)] = notlonglong_longlong;
  (*this)[make_pair(18,17)] = uint32_real;
  (*this)[make_pair(18,33)] = uint32_real;
  (*this)[make_pair(18,49)] = (*this)[make_pair(18,65)] = uint32_real;

  // x := (long long int)y
  (*this)[make_pair(32,4)] = longlong_notlonglong;
  (*this)[make_pair(32,6)] = longlong_notlonglong;
  (*this)[make_pair(32,8)] = longlong_notlonglong;
  (*this)[make_pair(32,10)] = longlong_notlonglong;
  (*this)[make_pair(32,16)] = longlong_notlonglong;
  (*this)[make_pair(32,18)] = longlong_notlonglong;
  (*this)[make_pair(32, 32)] = assign_impl::single;  // x = (T*)y
  (*this)[make_pair(32, 34)] = assign_impl::single;
  (*this)[make_pair(32,17)] = sint64_real;
  (*this)[make_pair(32,33)] = sint64_real;
  (*this)[make_pair(32,49)] = (*this)[make_pair(32,65)] = sint64_real;

  // x := (unsinged long long int)y
  (*this)[make_pair(34,4)] = longlong_notlonglong;
  (*this)[make_pair(34,6)] = longlong_notlonglong;
  (*this)[make_pair(34,8)] = longlong_notlonglong;
  (*this)[make_pair(34,10)] = longlong_notlonglong;
  (*this)[make_pair(34,16)] = longlong_notlonglong;
  (*this)[make_pair(34,18)] = longlong_notlonglong;
  (*this)[make_pair(34,32)] = assign_impl::single;
  (*this)[make_pair(34,17)] = uint64_float;
  (*this)[make_pair(34,33)] = uint64_double;
  (*this)[make_pair(34,34)] = assign_impl::single;
  (*this)[make_pair(34,49)] = (*this)[make_pair(34,65)] = uint64_ld;

  // x := (float)y
  (*this)[make_pair(17,4)] = real_byte;
  (*this)[make_pair(17,6)] = real_byte;
  (*this)[make_pair(17,8)] = real_sint16;
  (*this)[make_pair(17,10)] = real_uint16;
  (*this)[make_pair(17,16)] = real_sint32;
  (*this)[make_pair(17,18)] = real_uint32;
  (*this)[make_pair(17,32)] = real_longlong;
  (*this)[make_pair(17,34)] = real_longlong;
  (*this)[make_pair(17,33)] = real_real;
  (*this)[make_pair(17,49)] = (*this)[make_pair(17,65)] = real_real;

  // x := (double)y
  (*this)[make_pair(33,4)] = real_byte;
  (*this)[make_pair(33,6)] = real_byte;
  (*this)[make_pair(33,8)] = real_sint16;
  (*this)[make_pair(33,10)] = real_uint16;
  (*this)[make_pair(33,16)] = real_sint32;
  (*this)[make_pair(33,18)] = real_uint32;
  (*this)[make_pair(33,32)] = real_longlong;
  (*this)[make_pair(33,34)] = real_longlong;
  (*this)[make_pair(33, 17)] = real_real;
  (*this)[make_pair(33, 33)] = assign;  // double <- long double at MS mode
  (*this)[make_pair(33,49)] = (*this)[make_pair(33,65)] = real_real;

  // x := (long double)y
  (*this)[make_pair(49,4)] = (*this)[make_pair(65,4)] = real_byte;
  (*this)[make_pair(49,6)] = (*this)[make_pair(65,6)] = real_byte;
  (*this)[make_pair(49,8)] = (*this)[make_pair(65,8)] = real_sint16;
  (*this)[make_pair(49,10)] = (*this)[make_pair(65,10)] = real_uint16;
  (*this)[make_pair(49,16)] = (*this)[make_pair(65,16)] = real_sint32;
  (*this)[make_pair(49,18)] = (*this)[make_pair(65,18)] = real_uint32;
  (*this)[make_pair(49,32)] = (*this)[make_pair(65,32)] = real_longlong;
  (*this)[make_pair(49,34)] = (*this)[make_pair(65,34)] = real_longlong;
  (*this)[make_pair(49,17)] = (*this)[make_pair(65,17)] = real_real;
  (*this)[make_pair(49,33)] = (*this)[make_pair(65,33)] = real_real;
}

// x := (sint8_t)y or x := (uint8_t)y, where type of y is real
void intel::byte_real(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* Ty = tac->y->m_type;
  int sy = Ty->size();
  if (sy == 4 || sy == 8)
    return half_real(tac);

  fld(tac->y);
  {
    string SP = sp();    
    if (mode == GNU) {
      char ps = psuffix();
      out << '\t' << "sub" << ps << '\t' << "$6, " << SP << '\n';
      out << '\t' << "fnstcw" << '\t' << "4(" << SP << ')' << '\n';
      out << '\t' << "movzwl" << '\t' << "4(" << SP << "), %eax" << '\n';
      out << '\t' << "orw" << '\t' << "$3072, %ax" << '\n';
      out << '\t' << "movw" << '\t' << "%ax, 2(" << SP << ')' << '\n';
      out << '\t' << "fldcw" << '\t' << "2(" << SP << ')' << '\n';
      out << '\t' << "fistps" << '\t' << '(' << SP << ')' << '\n';
      out << '\t' << "fldcw" << '\t' << "4(" << SP << ')' << '\n';
      out << '\t' << "movzwl" << '\t' << '(' << SP << "), %eax" << '\n';
      out << '\t' << "add" << ps << '\t' << "$6, " << SP << '\n';
    }
    else {
      out << '\t' << "sub" << '\t' << SP << ", " << 6 << '\n';
      out << '\t' << "fnstcw" << '\t' << 4 << '[' << SP << ']' << '\n';
      out << '\t' << "movzx" << '\t' << "eax" << ", " << "WORD PTR " << 4 << '[' << SP << ']' << '\n';
      out << '\t' << "or" << '\t' << "ax, 3072" << '\n';
      out << '\t' << "mov" << '\t' << "WORD PTR " << 2 << '[' << SP << ']' << ", " << "ax" << '\n';
      out << '\t' << "fldcw" << '\t' << "2[" << SP << ']' << '\n';
      out << '\t' << "fistp" << '\t' << " DWORD PTR " << '[' << SP << ']' << '\n';
      out << '\t' << "fldcw" << '\t' << 4 << '[' << SP << ']' << '\n';
      out << '\t' << "movzx" << '\t' << "eax" << ", " << "WORD PTR " << '[' << SP << ']' << '\n';
      out << '\t' << "add" << '\t' << SP << ", " << 6 << '\n';
    }
  }
  const type* Tx = tac->x->m_type;
  string op = Tx->_signed() ? "movsbl" : "movzbl";
  out << '\t' << op << '\t' << "%al, %eax" << '\n';
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

// x := (sint16_t)y or x := (uint16_t)y, where type of y is real
void intel::half_real(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* Ty = tac->y->m_type;
  int sy = Ty->size();
  if (x64 && (sy == 4 || sy == 8)) {
    address* y = getaddr(tac->y);
    y->load();
    out << '\t' << "cvtts";
    out << (sy == 4 ? 's' : 'd');
    out << "2si" << '\t';
    if (mode == GNU)
      out << "%xmm0, %eax" << '\n';
    else
      out << "eax, xmm0" << '\n';
    address* x = getaddr(tac->x);
    return x->store();
  }
  const type* Tx = tac->x->m_type;
  fld(tac->y);
  {
    string SP = sp();
    if (mode == GNU) {
      char ps = psuffix();
      out << '\t' << "sub" << ps << '\t' << "$24, " << SP << '\n';
      out << '\t' << "fnstcw" << '\t' << "14(" << SP << ')' << '\n';
      out << '\t' << "movzwl" << '\t' << "14(" << SP << "), %eax" << '\n';
      out << '\t' << "orb" << '\t' << "$12, %ah" << '\n';
      out << '\t' << "movw" << '\t' << "%ax, 12(" << SP << ')' << '\n';
      out << '\t' << "fldcw" << '\t' << "12(" << SP << ')' << '\n';
      if (Tx->_signed())
        out << '\t' << "fistps" << '\t' << "10(" << SP << ')' << '\n';
      else
        out << '\t' << "fistpl" << '\t' << "8(" << SP << ')' << '\n';
      out << '\t' << "fldcw" << '\t' << "14(" << SP << ')' << '\n';
      if (Tx->_signed())
        out << '\t' << "movzwl" << '\t' << "10(" << SP << "), %eax" << '\n';
      else
        out << '\t' << "movl" << '\t' << "8(" << SP << "), %eax" << '\n';
      out << '\t' << "add" << ps << '\t' << "$24, " << SP << '\n';
    }
    else {
      out << '\t' << "sub" << '\t' << SP << ", 24" << '\n';
      out << '\t' << "fnstcw" << '\t' << "WORD PTR 14[" << SP << ']' << '\n';
      out << '\t' << "movzx" << '\t' << "eax, WORD PTR 14[" << SP << "]" << '\n';
      out << '\t' << "or" << '\t' << "ah, 12" << '\n';
      out << '\t' << "mov" << '\t' << "WORD PTR 12[" << SP << "], ax" << '\n';
      out << '\t' << "fldcw" << '\t' << "12[" << SP << ']' << '\n';
      if (Tx->_signed())
        out << '\t' << "fistp" << '\t' << "DWORD PTR 10[" << SP << ']' << '\n';
      else
        out << '\t' << "fistp" << '\t' << "QWORD PTR 8[" << SP << ']' << '\n';
      out << '\t' << "fldcw" << '\t' << "14[" << SP << ']' << '\n';
      if (Tx->_signed())
        out << '\t' << "movzx" << '\t' << "eax, WORD PTR 10[" << SP << ']' << '\n';
      else
        out << '\t' << "mov" << '\t' << "eax, DWORD PTR 8[" << SP << ']' << '\n';
      out << '\t' << "add" << '\t' << SP << ", 24" << '\n';
    }
  }
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

namespace intel {
  namespace sint32_real_impl {
    void GNU_subr(COMPILER::tac* tac);
    void MS_x86_subr(COMPILER::tac* tac);
  } // end of namespace sint32_real_impl
} // end of namespace intel

// x := (sint32_t)y, where type of y is real
void intel::sint32_real(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  if (mode == GNU)
    return sint32_real_impl::GNU_subr(tac);

  if (!x64)
    return sint32_real_impl::MS_x86_subr(tac);
  
  address* y = getaddr(tac->y);
  y->load();
  const type* Ty = tac->y->m_type;
  int size = Ty->size();
  out << '\t' << "cvtts" << (size == 4 ? 's' : 'd') << "2si";
  out << '\t' << "eax, xmm0" << '\n';
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

void intel::sint32_real_impl::GNU_subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  {
    fld(tac->y);
    char ps = psuffix();
    string SP = sp();
    int n1 = 24;
    out << '\t' << "sub" << ps << '\t' << '$' << dec << n1 << ", " << SP << '\n';
    ostringstream os;
    int n2 = 14;
    os << dec << n2 << '(' << SP << ')';
    string spexpr = os.str();
    out << '\t' << "fnstcw" << '\t' << dec << spexpr << '\n';
    out << '\t' << "movzwl" << '\t' << spexpr << ", " << "%eax" << '\n';
    int n3 = 12;
    out << '\t' << "orb" << '\t' << '$' << dec << n3 << ", %ah" << '\n';
    ostringstream os2;
    os2 << dec << n3 << '(' << SP << ')';
    string spexpr2 = os2.str();
    out << '\t' << "movw" << '\t' << "%ax, " << spexpr2 << '\n';
    out << '\t' << "fldcw" << '\t' << spexpr2 << '\n';
    ostringstream os3;
    int n4 = 8;
    os3 << dec << n4 << '(' << SP << ')';
    string spexpr3 = os3.str();
    out << '\t' << "fistp" << fsuffix(8) << '\t' << spexpr3 << '\n';
    out << '\t' << "fldcw" << '\t' << spexpr << '\n';
    out << '\t' << "movl" << '\t' << spexpr3 << ", " << "%eax" << '\n';
    out << '\t' << "add" << ps << '\t' << '$' << n1 << ", " << SP << '\n';
  }

  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

void intel::sint32_real_impl::MS_x86_subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  fld(tac->y);
  string SP = sp();
  int n1 = 24;
  out << '\t' << "sub" << '\t' << "esp, " << dec << n1 << '\n';
  ostringstream os;
  int n2 = 14;
  os << "WORD PTR " << dec << n2 << "[esp]";
  string spexpr = os.str();
  out << '\t' << "fnstcw" << '\t' << spexpr << '\n';
  out << '\t' << "movzx" << '\t' << "eax, " << spexpr << '\n';
  int n3 = 12;
  out << '\t' << "or" << '\t' << "ah, " << dec << n3 << '\n';
  ostringstream os2;
  os2 << "WORD PTR " << dec << n3 << "[esp]";
  string spexpr2 = os2.str();
  out << '\t' << "mov" << '\t' << spexpr2 << ", ax" << '\n';
  out << '\t' << "fldcw" << '\t' << spexpr2 << '\n';
  ostringstream os3;
  int n4 = 8;
  os3 << "DWORD PTR " << dec << n4 << "[esp]";
  string spexpr3 = os3.str();
  out << '\t' << "fistp" << '\t' << spexpr3 << '\n';
  out << '\t' << "fldcw" << '\t' << spexpr << '\n';
  out << '\t' << "mov" << '\t' << "eax, " << spexpr3 << '\n';
  out << '\t' << "add" << '\t' << "esp, " << n1 << '\n';

  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

void intel::uint32_real(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  string SP = sp();
  if (mode == GNU) {
    fld(tac->y);
    char ps = psuffix();
    string sf = fistp_suffix();
    out << '\t' << "sub" << ps << '\t' << "$12, " << SP << '\n';
    out << '\t' << "fnstcw" << '\t' << "10(" << SP << ')' << '\n';
    out << '\t' << "movzwl" << '\t' << "10(" << SP << "), %eax" << '\n';
    out << '\t' << "orw" << '\t' << "$3072, %ax" << '\n';
    out << '\t' << "movw" << '\t' << "%ax, 8(" << SP << ')' << '\n';
    out << '\t' << "fldcw" << '\t' << "8(" << SP << ')' << '\n';
    out << '\t' << "fistp" << sf << '\t' << '(' << SP << ')' << '\n';
    out << '\t' << "fldcw" << '\t' << "10(" << SP << ')' << '\n';
    out << '\t' << "movl" << '\t' << '(' << SP << "), %eax" << '\n';
    out << '\t' << "add" << ps << '\t' << "$12, " << SP << '\n';
  }
  else {
    const type* Ty = tac->y->m_type;
    int size = Ty->size();
    out << '\t' << "cvtts" << (size == 4 ? 's' : 'd') << "2si" << '\t';
    address* y = getaddr(tac->y);
    out << (x64 ? "rax" : "eax") << ", " << y->expr() << '\n';
  }
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

void intel::longlong_notlonglong(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  const type* T = tac->y->m_type;
  int size = T->size();
  string eax = reg::name(reg::ax, 4);
  string edx = reg::name(reg::dx, 4);
  string rax = reg::name(reg::ax, 8);

  if (x64) {
    y->load();
    out << '\t';
    switch (size) {
    case 1:
      if (mode == GNU)
        out << (T->_signed() ? "movsbq" : "movzbl");
      else
        out << (T->_signed() ? "movsx" : "movzx");
      break;
    case 2:
      if (mode == GNU)
        out << (T->_signed() ? "movswq" : "movzwl");
      else
        out << (T->_signed() ? "movsx" : "movzx");
      break;
    case 4:
      if (mode == GNU)
        out << (T->_signed() ? "movslq" : "movl");
      else
        out << (T->_signed() ? "movsxd" : "mov");
      break;
    }
    if (mode == GNU) {
      out << '\t' << reg::name(reg::ax, size) << ", ";
      out << (T->_signed() ? rax : eax);
    }
    else {
      out << '\t' << (T->_signed() ? rax : eax) << ", ";
      out << reg::name(reg::ax, size);
    }
    out << '\n';
  }
  else {
    out << '\t';
    switch (size) {
    case 1:
      if (mode == GNU)
        out << (T->_signed() ? "movsbl" : "movzbl");
      else
        out << (T->_signed() ? "movsx" : "movzx");
      break;
    case 2:
      if (mode == GNU)
        out << (T->_signed() ? "movswl" : "movzwl");
      else
        out << (T->_signed() ? "movsx" : "movzx");
      break;
    case 4:
      if (mode == GNU)
        out << "movl";
      else
        out << "mov";
      break;
    }
    if (mode == GNU)
      out << '\t' << y->expr() << ", " << eax << '\n';
    else
      out << '\t' << eax << ", " << y->expr() << '\n';

    if (!T->_signed()) {
      if (mode == GNU) {
        char sf = suffix(4);
        out << '\t' << "mov" << sf << '\t' << "$0, " << edx << '\n';
      }
      else
        out << '\t' << "mov" << '\t' << edx << ", " << 0 << '\n';
    }
    else {
      if (mode == GNU) {
        char sf = suffix(4);
        out << '\t' << "mov" << sf << '\t' << eax << ", " << edx << '\n';
        out << '\t' << "sar" << sf << '\t' << "$31" << ", " << edx << '\n';
      }
      else {
        out << '\t' << "mov" << '\t' << edx << ", " << eax << '\n';
        out << '\t' << "sar" << '\t' << edx << ", " << 31 << '\n';
      }
    }
  }
  x->store();
}

void intel::notlonglong_longlong(COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load(reg::ax);
  int size = tac->x->m_type->size();
  int mask = size == sizeof(int) ? ~0 : ~(~0 << 8 * size);
  if (mode == GNU)
    out << '\t' << "andl" << '\t' << '$' << mask << ", %eax" << '\n';
  else
    out << '\t' << "and" << '\t' << "eax"<< ", " << mask << '\n';
  x->store(reg::ax);
}

void intel::notlonglong_notlonglong(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  int xsize = tac->x->m_type->size();
  int ysize = tac->y->m_type->size();
  y->load(reg::ax);
  string eax = reg::name(reg::ax, 4);
  if ( xsize < ysize ){
    int mask = ~(~0 << 8 * xsize);
    char sf = suffix(4);
    if (mode == GNU)
      out << '\t' << "and" << sf << '\t' << '$' << mask << ", " << eax << '\n';
    else
      out << '\t' << "and" << sf << '\t' << eax << ", " << mask << '\n';
  }
  else if ( xsize > ysize ){
    const type* T = tac->y->m_type;
    string op = T->_signed() ? "movsx" : "movzx";
    string yax = reg::name(reg::ax, ysize);
    if (mode == GNU)
      out << '\t' << op << '\t' << yax << ", " << eax << '\n';
    else
      out << '\t' << op << '\t' << eax << ", " << yax << '\n';
  }
  x->store(reg::ax);
}

// x := (long long int)y, where type of y is real
void intel::sint64_real(COMPILER::tac* tac)
{
  using namespace std;
  fld(tac->y);
  {
    string SP = sp();
    if (mode == GNU) {
      char ps = psuffix();
      string sf = fistp_suffix();
      out << '\t' << "sub" << ps << '\t' << "$24, " << SP << '\n';
      out << '\t' << "fnstcw" << '\t' << "14(" << SP << ')' << '\n';
      out << '\t' << "movzwl" << '\t' << "14(" << SP << "), %eax" << '\n';
      out << '\t' << "orb" << '\t' << "$12, %ah" << '\n';
      out << '\t' << "movw" << '\t' << "%ax, 12(" << SP << ')' << '\n';
      out << '\t' << "fldcw" << '\t' << "12(" << SP << ')' << '\n';
      out << '\t' << "fistp" << sf << '\t' << '(' << SP << ')' << '\n';
      out << '\t' << "fldcw" << '\t' << "14(" << SP << ')' << '\n';
      if (x64)
        out << '\t' << "mov" << psuffix() << '\t' << '(' << SP << "), %rax" << '\n';
      else {
        out << '\t' << "movl" << '\t' << " (" << SP << "), %eax" << '\n';
        out << '\t' << "movl" << '\t' << "4(" << SP << "), %edx" << '\n';
      }
      out << '\t' << "add" << ps << '\t' << "$24, " << SP << '\n';
    }
    else {
      out << '\t' << "sub" << '\t' << SP << ", 24" << '\n';
      out << '\t' << "fnstcw" << '\t' << "14[" << SP << ']' << '\n';
      out << '\t' << "movzx" << '\t' << "eax, WORD PTR 14[" << SP << ']' << '\n';
      out << '\t' << "or" << '\t' << "ah, 12" << '\n';
      out << '\t' << "mov" << '\t' << "WORD PTR 12[" << SP << ']' << ", " << "ax" << '\n';
      out << '\t' << "fldcw" << '\t' << "12[" << SP << ']' << '\n';
      out << '\t' << "fistp" << '\t' << "QWORD PTR " << '[' << SP << ']' << '\n';
      out << '\t' << "fldcw" << '\t' << "14[" << SP << ']' << '\n';
      if (x64)
        out << '\t' << "mov" << '\t' << "rax" << ", " << " QWORD PTR " << '[' << SP << ']' << '\n';
      else {
        out << '\t' << "mov" << '\t' << "eax" << ", " << '[' << SP << ']' << '\n';
        out << '\t' << "mov" << '\t' << "edx" << ", " << " DWORD PTR 4[" << SP << ']' << '\n';
      }
      out << '\t' << "add" << '\t' << SP << ", 24" << '\n';
    }
  }
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

namespace intel {
  void uint64_real_x86(COMPILER::tac* tac);
}  // end of namespace intel

void intel::uint64_float(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  if (x64) {
    address* y = getaddr(tac->y);
    y->load();
    if (uint64_float_t::obj.m_label.empty())
      uint64_float_t::obj.m_label = new_label((mode == GNU) ? ".LC" : "LC$");
    if (mode == GNU)
      out << '\t' << "ucomiss" << '\t' << uint64_float_t::obj.m_label << "(%rip), %xmm0" << '\n';
    else
      out << '\t' << "ucomiss" << '\t' << "xmm0, DWORD PTR " << uint64_float_t::obj.m_label << '\n';
    string label = new_label((mode == GNU) ? ".label" : "label$");
    out << '\t' << "jnb" << '\t' << label << '\n';
    if (mode == GNU)
      out << '\t' << "cvttss2siq" << '\t' << "%xmm0, %rax" << '\n';
    else
      out << '\t' << "cvttss2si" << '\t' << "rax, xmm0" << '\n';
    string end = new_label((mode == GNU) ? ".label" : "label$");
    out << '\t' << "jmp" << '\t' << end << '\n';
    if (mode == GNU)
      out << '\t' << ".p2align 4,,10" << '\n';
    out << label << ":\n";
    if (mode == GNU) {
      out << '\t' << "subss" << '\t' << uint64_float_t::obj.m_label << "(%rip), %xmm0" << '\n';
      out << '\t' << "movabsq" << '\t' << "$-9223372036854775808, %rdx" << '\n';
      out << '\t' << "cvttss2siq" << '\t' << "%xmm0, %rax" << '\n';
      out << '\t' << "xorq" << '\t' << "%rdx, %rax" << '\n';
    }
    else {
      out << '\t' << "subss" << '\t' << "xmm0, DWORD PTR " << uint64_float_t::obj.m_label << '\n';
      out << '\t' << "mov" << '\t' << "rdx, -9223372036854775808" << '\n';
      out << '\t' << "cvttss2si" << '\t' << "rax, xmm0" << '\n';
      out << '\t' << "xor" << '\t' << "rax, rdx" << '\n';
    }
    out << end << ":\n";
    address* x = getaddr(tac->x);
    x->store(reg::ax);
  }
  else
    uint64_real_x86(tac);
}

void intel::uint64_float_t::output_value() const
{
  if (mode == GNU)
    out << ".align 4" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << '\t' << 1593835520 << '\n';
}

namespace intel {
  namespace uint64_double_impl {
    namespace x64_impl {
      void subr(COMPILER::tac* tac);
    } // end of namespace x64_impl
  } // end of namespace uint64_double_impl
} // end of namespace intel

void intel::uint64_double(COMPILER::tac* tac)
{
  x64 ? uint64_double_impl::x64_impl::subr(tac) : uint64_real_x86(tac);
}

void intel::uint64_double_impl::x64_impl::subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load();
  if (mode == GNU) {
    if (uint64_double_t::obj.m_label.empty())
      uint64_double_t::obj.m_label = new_label(".LC");
    out << '\t' << "movsd" << '\t' << uint64_double_t::obj.m_label << "(%rip), %xmm1" << '\n';
    out << '\t' << "ucomisd        %xmm1, %xmm0" << '\n';
    string label = new_label(".label");
    out << '\t' << "jnb" << '\t' << label << '\n';
    out << '\t' << "cvttsd2siq" << '\t' << "%xmm0, %rax" << '\n';
    string end = new_label(".label");
    out << '\t' << "jmp" << '\t' << end << '\n';
    out << '\t' << ".p2align 4,,10" << '\n';
    out << label << ":\n";
    out << "subsd        %xmm1, %xmm0" << '\n';
    out << "movabsq        $-9223372036854775808, %rdx" << '\n';
    out << "cvttsd2siq        %xmm0, %rax" << '\n';
    out << "xorq        %rdx, %rax" << '\n';
    out << end << ":\n";
  }
  else {
    if (uint64_double_t::obj.m_label.empty())
      uint64_double_t::obj.m_label = new_label("LC$");
    out << '\t' << "movsd" << '\t' << "xmm1, QWORD PTR " << uint64_double_t::obj.m_label << '\n';
    out << '\t' << "ucomisd" << '\t' << "xmm0, xmm1" << '\n';
    string label = new_label("label$");
    out << '\t' << "jnb" << '\t' << label << '\n';
    out << '\t' << "cvttsd2si" << '\t' << "rax, xmm0" << '\n';
    string end = new_label("label$");
    out << '\t' << "jmp" << '\t' << end << '\n';
    out << label << ":\n";
    out << '\t' << "subsd        xmm0, xmm1" << '\n';
    out << '\t' << "mov        rdx, -9223372036854775808" << '\n';
    out << '\t' << "cvttsd2si        rax, xmm0" << '\n';
    out << '\t' << "xor        rax, rdx" << '\n';
    out << end << ":\n";
  }
  address*x = getaddr(tac->x);
  x->store();
}

void intel::uint64_double_t::output_value() const
{
  if (mode == GNU)
    out << '\t' << ".align 8" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << "        0" << '\n';
  out << '\t' << dot_long() << "        1138753536" << '\n';
}

void intel::uint64_ld(COMPILER::tac* tac)
{
  using namespace std;
  if (x64) {
    if (uint64_ld_t::obj.m_label.empty())
      uint64_ld_t::obj.m_label = new_label((mode == GNU) ? ".LC" : "LC$");
    string label = new_label((mode == GNU) ? ".label" : "label$");
    string end = new_label((mode == GNU) ? ".label" : "label$");
    fld(tac->y);
    if (mode == GNU) {
      out << '\t' << "subq" << '\t' << "$24, %rsp" << '\n';
      out << '\t' << "flds" << '\t' << uint64_ld_t::obj.m_label << "(%rip)" << '\n';
      out << '\t' << "fxch        %st(1)" << '\n';
      out << '\t' << "fucomi        %st(1), %st" << '\n';
      out << '\t' << "jnb" << '\t' << label << '\n';
      out << '\t' << "fstp        %st(1)" << '\n';
      out << '\t' << "fnstcw        14(%rsp)" << '\n';
      out << '\t' << "movzwl        14(%rsp), %eax" << '\n';
      out << '\t' << "orb        $12, %ah" << '\n';
      out << '\t' << "movw        %ax, 12(%rsp)" << '\n';
      out << '\t' << "fldcw        12(%rsp)" << '\n';
      out << '\t' << "fistpq        (%rsp)" << '\n';
      out << '\t' << "fldcw        14(%rsp)" << '\n';
      out << '\t' << "mov" << psuffix() << "        (%rsp), %rax" << '\n';
      out << '\t' << "jmp" << '\t' << end << '\n';

      out << '\t' << ".p2align 4,,10" << '\n';
      out << label << ":\n";
      out << '\t' << "fnstcw        14(%rsp)" << '\n';
      out << '\t' << "movzwl        14(%rsp), %eax" << '\n';
      out << '\t' << "fsubp        %st, %st(1)" << '\n';
      out << '\t' << "movabsq        $-9223372036854775808, %rdx" << '\n';
      out << '\t' << "orb        $12, %ah" << '\n';
      out << '\t' << "movw        %ax, 12(%rsp)" << '\n';
      out << '\t' << "fldcw        12(%rsp)" << '\n';
      out << '\t' << "fistpq        (%rsp)" << '\n';
      out << '\t' << "fldcw        14(%rsp)" << '\n';
      out << '\t' << "mov" << psuffix() << " (%rsp), %rax" << '\n';
      out << '\t' << "xorq        %rdx, %rax" << '\n';
      out << end << ":\n";
      out << '\t' << "addq" << '\t' << "$24, %rsp" << '\n';
      address* x = getaddr(tac->x);
      x->store();
    }
    else {
      out << '\t' << "subq" << '\t' << "rsp, 24" << '\n';
      out << '\t' << "fld" << '\t' << "DWORD PTR " << uint64_ld_t::obj.m_label << '\n';
      out << '\t' << "fxch        st(1)" << '\n';
      out << '\t' << "fucomi        st(1), st" << '\n';
      out << '\t' << "jnb" << '\t' << label << '\n';
      out << '\t' << "fstp        st(1)" << '\n';
      out << '\t' << "fnstcw        14[rsp]" << '\n';
      out << '\t' << "movzx        14[rsp], eax" << '\n';
      out << '\t' << "orb        ah, 12" << '\n';
      out << '\t' << "movw        12[rsp], ax" << '\n';
      out << '\t' << "fldcw        12[rsp]" << '\n';
      out << '\t' << "fistp        QWORD PTR [rsp]" << '\n';
      out << '\t' << "fldcw        14[rsp]" << '\n';
      out << '\t' << "mov" << psuffix() << " rax, [rsp]" << '\n';
      out << '\t' << "jmp" << '\t' << end << '\n';

      out << '\t' << "fnstcw        14[rsp]" << '\n';
      out << '\t' << "movzwl        eax, 14[rsp]" << '\n';
      out << '\t' << "fsubp        st(1), st" << '\n';
      out << '\t' << "orb        ah, 12" << '\n';
      out << '\t' << "mov        WORD PTR 12[rsp], ax" << '\n';
      out << '\t' << "fldcw        12[rsp]" << '\n';
      out << '\t' << "fistp        QWORD PTR [rsp]" << '\n';
      out << '\t' << "fldcw        14[rsp]" << '\n';
      out << '\t' << "mov" << psuffix() << " rax, [rsp]";
      out << '\t' << "xor        rax, rdx" << '\n';
      out << end << ":\n";
      out << '\t' << "add" << '\t' << "rsp, 24" << '\n';
      address* x = getaddr(tac->x);
      x->store();
    }
  }
  else
    uint64_real_x86(tac);
}

void intel::uint64_ld_t::output_value() const
{
  if (mode == GNU)
    out << '\t' << ".align 4" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << '\t' << "1593835520" << '\n';
}

void intel::uint64_real_x86(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->y->m_type;
  int size = T->size();
  if (mode == GNU)
    out << '\t' << "subl" << '\t' << '$' << size << ", %esp" << '\n';
  address* y = getaddr(tac->y);
  if (mode == GNU) {
    y->load();
    out << '\t' << "fstp" << fsuffix(size) << '\t' << "(%esp)" << '\n';
    out << '\t' << "call" << '\t' << "__fixuns";
    switch (size) {
    case 4: out << 's'; break;
    case 8: out << 'd'; break;
    default:
      assert(size == literal::floating::long_double::size);
      out << 'x'; break;
    }
    out << "fdi" << '\n';
    out << '\t' << "addl" << '\t' << '$' << size << ", %esp" << '\n';
  }
  else {
    string label = (size == 4 ? "__ftoul3" : "__dtoul3");
    out << '\t' << "movs";
    out << (size == 4 ? 's' : 'd');
    out << '\t' << "xmm0, " << y->expr() << '\n';
    out << '\t' << "call" << '\t' << label << '\n';
    mem::refed.insert(mem::refgen_t(label, usr::FUNCTION, 0));
  }

  address* x = getaddr(tac->x);
  x->store();
}

// x := (T)y , where T is real
void intel::real_byte(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load(reg::ax);
  const type* Ty = tac->y->m_type;
  string SP = sp();
  if (mode == GNU) {
    string op = Ty->_signed() ? "movsbw" : "movzbw";
    out << '\t' << op << '\t' << "%al, %ax" << '\n';
    out << '\t' << "pushw"  << '\t' << "%ax" << '\n';
    out << '\t' << "filds"  << '\t' << '(' << SP << ')' << '\n';
    out << '\t' << "lea" << psuffix() << '\t' << '2' << '(' << SP << "), " << sp() << '\n';
    fstp(tac->x);
  }
  else {
    string op = Ty->_signed() ? "movsx" : "movzx";
    out << '\t' << op << '\t' << "eax, al" << '\n';
    out << '\t' << "cvtsi2s";
    int size = tac->x->m_type->size();
    char suffix = (size == 4 ? 's' : 'd');
    out << suffix;
    out << '\t' << "xmm0, eax" << '\n';
    address* x = getaddr(tac->x);
    out << '\t' << "movs" << suffix << '\t' << x->expr() << ", xmm0" << '\n';
  }
}

void intel::real_sint16(COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  if ( y->m_id == address::IMM )
    return real_sint32(tac);

  int size = tac->x->m_type->size();
  out << '\t' << "fild";
  if (mode == GNU)
    out << 's';
  out << '\t' << y->expr() << '\n';
  fstp(tac->x);
}

void intel::real_uint16(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  if ( y->m_id == address::IMM )
    return real_sint32(tac);

  char ps = psuffix();
  int psz = psize();
  string eax = reg::name(reg::ax, 4);
  string rax = reg::name(reg::ax, psz);
  string SP = sp();
  if (mode == GNU)
    out << '\t' << "movzwl" << '\t' << y->expr() << ", " << eax << '\n';
  else
    out << '\t' << "movzx" << '\t' << eax << ", " << y->expr() << '\n';

  out << '\t' << "push" << ps << '\t' << rax << '\n';
  if (mode == GNU) {
    out << '\t' << "fildl" << '\t' << '(' << SP << ')' << '\n';
    out << '\t' << "lea" << ps << '\t' << psz << '(' << SP << "), " << SP << '\n';
  }
  else {
    out << '\t' << "fild" << '\t' << "DWORD PTR " << '[' << SP << ']' << '\n';
    out << '\t' << "lea" << '\t' << SP << ", " << psz << '[' << SP << ']' << '\n';
  }
  fstp(tac->x);
}

namespace intel {
  namespace real_longlong_impl {
    namespace x64_impl {
      void subr(COMPILER::tac*);
    } // end of namespace x64_impl
    namespace x86_impl {
      void subr(COMPILER::tac*);
    } // end of namespace x86_impl
  } // end of namespace real_longlong_impl
} // end of namespace intel

void intel::real_longlong(COMPILER::tac* tac)
{
  using namespace real_longlong_impl;
  x64 ? real_longlong_impl::x64_impl::subr(tac) : x86_impl::subr(tac);
}

namespace intel {
  namespace real_longlong_impl {
    namespace x64_impl {
      void GNU_subr(COMPILER::tac*);
      void MS_subr(COMPILER::tac*);
      void ld_longlong(COMPILER::tac* tac);      
    } // end of nmaespace x64_impl
  } // end of namespace real_longlong_impl
} // end of namespace intel

void intel::real_longlong_impl::x64_impl::subr(COMPILER::tac* tac)
{
  mode == GNU ? GNU_subr(tac): MS_subr(tac);
}

void intel::real_longlong_impl::x64_impl::GNU_subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* Tx = tac->x->m_type;
  int sx = Tx->size();
  if (sx == literal::floating::long_double::size)
    return ld_longlong(tac);

  address* y = getaddr(tac->y);
  y->load();
  string label;
  const type* Ty = tac->y->m_type;  
  if (!Ty->_signed()) {
    out << '\t' << "testq" << '\t' << "%rax, %rax" << '\n';
    label = new_label(".label");
    out << '\t' << "js" << '\t' << label << '\n';
  }
  char cx = (sx == 4 ? 's' : 'd');
  out << '\t' << "pxor" << '\t';
  out << "%xmm0, %xmm0" << '\n';
  out << '\t' << "cvtsi2s";
  out << cx;
  out << 'q' << '\t' << "%rax, %xmm0" << '\n';

  if (!Ty->_signed()) {
    string end = new_label(".label");
    out << '\t' << "jmp" << '\t' << end << '\n';
    out << '\t' << ".p2align 4,,10" << '\n';
    out << label << ":\n";
    out << '\t' << "mov" << psuffix() << '\t' << "%rax, %rbx" << '\n';
    out << '\t' << "pxor" << '\t' << "%xmm0, %xmm0" << '\n';
    out << '\t' << "shrq" << '\t' << "%rbx" << '\n';
    out << '\t' << "andl" << '\t' << "$1, %eax" << '\n';
    out << '\t' << "orq" << '\t' << "%rax, %rbx" << '\n';
    out << '\t' << "cvtsi2s";
    out << cx;
    out << 'q' << '\t' << "%rbx, %xmm0" << '\n';
    out << '\t' << "adds";
    out << cx;
    out << '\t' << "%xmm0, %xmm0" << '\n';
    out << end << ":\n";
  }
  address* x = getaddr(tac->x);
  x->store();
}  

void intel::real_longlong_impl::x64_impl::MS_subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* Tx = tac->x->m_type;
  int sx = Tx->size();
  char cx = (sx == 4 ? 's' : 'd');                       
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  const type* Ty = tac->y->m_type;
  if (Ty->_signed()) {
    out << '\t' << "cvtsi2s" << cx << '\t' << "xmm0, " << y->expr() << '\n';
    return x->store();
  }
  y->load(reg::ax);
  out << '\t' << "cvtsi2s" << cx << '\t' << "xmm0, rax" << '\n';
  out << '\t' << "test        rax, rax" << '\n';
  string label = new_label("label$");
  out << '\t' << "jge" << '\t' << label << '\n';
  out << '\t' << "adds" << cx << '\t' << "xmm0, " << ms_pseudo(sx) << " PTR ";
  if (sx == 4) {
    if (float_uint64_t::obj.m_label.empty())
      float_uint64_t::obj.m_label = new_label("LC$");
    out << float_uint64_t::obj.m_label << '\n';
  }
  else {
    if (double_uint64_t::obj.m_label.empty())
      double_uint64_t::obj.m_label = new_label("LC$");
    out << double_uint64_t::obj.m_label << '\n';
  }
  out << label << ':' << '\n';
  x->store();
}

void intel::real_longlong_impl::x86_impl::subr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  const type* Ty = tac->y->m_type;    
  if (Ty->_signed()) {
    if (mode == GNU)
      out << '\t' << "fildll" << '\t' << y->expr() << '\n';
    else
      out << '\t' << "fild" << '\t' << y->expr() << '\n';
  }
  else {
    y->load();
    if (mode == GNU) {
      out << '\t' << "pushl" << '\t' << "%edx" << '\n';
      out << '\t' << "pushl" << '\t' << "%eax" << '\n';
      out << '\t' << "testl" << '\t' << "%edx, %edx" << '\n';
      out << '\t' << "fildll" << '\t' << "(%esp)" << '\n';
      out << '\t' << "leal" << '\t' << "8(%esp), %esp" << '\n';
      string label = new_label(".label");
      string end = new_label(".label");
      out << '\t' << "js" << '\t' << label << '\n';
      out << '\t' << "jmp" << '\t' << end << '\n';
      out << '\t' << ".p2align 4,,7" << '\n';
      out << '\t' << ".p2align 3" << '\n';
      out << label << ":\n";
      if (real_uint64_t::obj.m_label.empty())
        real_uint64_t::obj.m_label = new_label(".LC");
      out << '\t' << "fadds" << '\t' << real_uint64_t::obj.m_label << '\n';
      out << end << ":\n";
    }
    else {
      out << '\t' << "push" << '\t' << "edx" << '\n';
      out << '\t' << "push" << '\t' << "eax" << '\n';
      out << '\t' << "test" << '\t' << "edx, edx" << '\n';
      out << '\t' << "fild" << '\t' << "QWORD PTR [esp]" << '\n';
      out << '\t' << "lea" << '\t' << "esp, 8[esp]" << '\n';
      string label = new_label("label$");
      string end = new_label("label$");
      out << '\t' << "js" << '\t' << label << '\n';
      out << '\t' << "jmp" << '\t' << end << '\n';
      out << label << ":\n";
      if (real_uint64_t::obj.m_label.empty())
        real_uint64_t::obj.m_label = new_label("LC$");
      out << '\t' << "fadd" << '\t' << "DWORD PTR ";
      out << real_uint64_t::obj.m_label << '\n';
      out << end << ":\n";
    }
  }
  address* x = getaddr(tac->x);
  x->store();
}

void intel::real_uint64_t::output_value() const
{
  if (mode == GNU)
    out << ".align 4" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << '\t' << 1602224128 << '\n';
}

void intel::float_uint64_t::output_value() const
{
  assert(mode == MS);
  out << m_label << ":\n";
  out << '\t' << "DD 05f800000r ; 1.84467e+19" << '\n';
}

void intel::double_uint64_t::output_value() const
{
  assert(mode == MS);
  out << m_label << ":\n";
  out << '\t' << "DQ 043f0000000000000r        ; 1.84467e+19" << '\n';
}

void intel::real_longlong_impl::x64_impl::ld_longlong(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  assert(x64);
  assert(mode == GNU);

  address* y = getaddr(tac->y);
  y->load();
  {
    const type* Ty = tac->y->m_type;
    if (!Ty->_signed())
      out << '\t' << "testq" << '\t' << "%rax, %rax" << '\n';

    out << '\t' << "pushq" << '\t' << "%rax" << '\n';
    out << '\t' << "fildq" << '\t' << "(%rsp)" << '\n';
    out << '\t' << "popq" << '\t' << "%rax" << '\n';

    if (!Ty->_signed()) {
      string label = new_label((mode == GNU) ? ".label" : "label$");
      out << '\t' << "js" << '\t' << label << '\n';
      string end = new_label((mode == GNU) ? ".label" : "label$");
      out << '\t' << "jmp" << '\t' << end << '\n';
      out << '\t' << ".p2align 4,,10" << '\n';
      out << label << ":\n";

      if (ld_uint64_t::obj.m_label.empty())
        ld_uint64_t::obj.m_label = new_label((mode == GNU) ? ".LC" : "LC$");
      out << '\t' << "fadds" << '\t' << ld_uint64_t::obj.m_label << "(%rip)" << '\n';
      out << end << ":\n";
    }
  }
  fstp(tac->x);
}

void intel::ld_uint64_t::output_value() const
{
  if (mode == GNU)
    out << '\t' << ".align 4" << '\n';
  out << m_label << ":\n";
  out << '\t' << dot_long() << '\t' << "1602224128" << '\n';
}

void intel::real_real(COMPILER::tac* tac)
{
  // double <- float : cvtss2sd        %xmm0, %xmm0
  // ld     <- float : fld & fstp

  // float <- double : cvtsd2ss        %xmm0, %xmm0
  // ld    <- double : fld & fstp

  // float  <- ld : fld & fstp
  // double <- ld : fld & fstp

  using namespace COMPILER;

  const type* Tx = tac->x->m_type;
  int sx = Tx->size();

  const type* Ty = tac->y->m_type;
  int sy = Ty->size();

  if (x64) {
    if ((sx == 8 && sy == 4) || (sx == 4 && sy == 8)) {
      address* y = getaddr(tac->y);
      y->load();
      out << '\t';
      out << (sx == 4 ? "cvtsd2ss" : "cvtss2sd");
      out << '\t' << xmm(0) << ", " << xmm(0) << '\n';
      address* x = getaddr(tac->x);
      return x->store();
    }
  }

  fld(tac->y);
  fstp(tac->x);
}

void intel::real_sint32(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load(reg::ax);
  char ps = psuffix();
  int psz = psize();
  string ax = reg::name(reg::ax, psz);
  string SP = sp();
  out << '\t' << "push" << ps << '\t' << ax << '\n';
  if (mode == GNU) {
    out << '\t' << "fildl" << '\t' << '(' << SP << ')' << '\n';
    out << '\t' << "lea" << ps << '\t' << psz << '(' << SP << ')' << ", " << SP << '\n';
  }
  else {
    out << '\t' << "fild" << '\t' << "DWORD PTR " << '[' << SP << ']' << '\n';
    out << '\t' << "lea" << '\t' << SP << ", " << psz << '[' << SP << ']' << '\n';
  }
  fstp(tac->x);
}

void intel::real_uint32(COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load(reg::ax);

  if (x64) {
    if (mode == GNU) {
      out << '\t' << "pushq" << '\t' << "%rax" << '\n';
      out << '\t' << "fildq" << '\t' << "(%rsp)" << '\n';
      out << '\t' << "leaq" << '\t' << "8(%rsp), %rsp" << '\n';
    }
    else {
      out << '\t' << "push" << '\t' << "rax" << '\n';
      out << '\t' << "fild" << '\t' << "QWORD PTR [rsp]" << '\n';
      out << '\t' << "lea" << '\t' << "rsp, 8[rsp]" << '\n';
    }
  }
  else {
    const type* Ty = tac->y->m_type;
    if (mode == GNU) {
      out << '\t' << "xorl" << '\t' << "%edx, %edx" << '\n';
      out << '\t' << "pushl" << '\t' << "%edx" << '\n';
      out << '\t' << "pushl" << '\t' << "%eax" << '\n';
      out << '\t' << "fildll" << '\t' << "(%esp)" << '\n';
      out << '\t' << "leal" << '\t' << "8(%esp), %esp" << '\n';
    }
    else {
      out << '\t' << "xor" << '\t' << "edx, edx" << '\n';
      out << '\t' << "push" << '\t' << "edx" << '\n';
      out << '\t' << "push" << '\t' << "eax" << '\n';
      out << '\t' << "fild" << '\t' << "QWORD PTR [esp]" << '\n';
      out << '\t' << "lea" << '\t' << "esp, 8[esp]" << '\n';
    }
  }
  fstp(tac->x);
}

namespace intel {
  namespace addr_impl {
    void normal(COMPILER::tac* tac);
  } // end of namespace addr_impl
}  // end of namesapce intel

void intel::addr(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  using namespace addr_impl;

  if (x64) {
    const type* T = tac->y->m_type;
    int size = T->size();
    if (T->scalar() && size <= 8)
      return normal(tac);
    if (tac->y->m_scope->m_id != scope::PARAM)
      return normal(tac);

    address* y = getaddr(tac->y);
    char sf = psuffix();
    string bx = reg::name(reg::bx, psize());
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << y->expr() << ", " << bx << '\n';
    else
      out << '\t' << "mov" << sf << '\t' << bx << ", " << y->expr() << '\n';
    address* x = getaddr(tac->x);
    return x->store(reg::bx);
  }
  else
    return normal(tac);
}

void intel::addr_impl::normal(COMPILER::tac* tac)
{
  address* y = getaddr(tac->y);
  y->get(reg::bx);
  address* x = getaddr(tac->x);
  x->store(reg::bx);
}

namespace intel { namespace invladdr_impl {
  void single(COMPILER::tac*);
  void multi(COMPILER::tac*);
} } // end of namespace invladdr_impl and intel

void intel::invladdr(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace invladdr_impl;
  const type* T = tac->z->m_type;
  int size = T->size();
  if ( x64 )
    (T->scalar() && size <= 8) ? single(tac) : multi(tac);
  else
    T->scalar() ? single(tac) : multi(tac);
}

void intel::invladdr_impl::single(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  const type* T = tac->z->m_type;
  int size = T->size();
  if (x64 || size <= 4) {
    y->load(reg::cx);
    z->load(reg::ax);
    if (x64)
      assert(size <= 8);
    char sf = suffix(size);
    string ax = reg::name(reg::ax, size);
    string cx = reg::name(reg::cx, psize());
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << ax << ", " << '(' << cx << ')' << '\n';
    else
      out << '\t' << "mov" << '\t' << '[' << cx << ']' << ", " << ax << '\n';
  }
  else {
    assert(size == 8 || size == 12);
    y->load(reg::bx);
    z->load(reg::ax);
    int sz = 4;
    char sf = suffix(sz);
    string ax = reg::name(reg::ax, sz);
    string bx = reg::name(reg::bx, sz);
    string cx = reg::name(reg::cx, sz);
    string dx = reg::name(reg::dx, sz);

    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << ax << ", " << " (" << bx << ')' << '\n';
      out << '\t' << "mov" << sf << '\t' << dx << ", " << "4(" << bx << ')' << '\n';
      if (size == 12)
        out << '\t' << "mov" << sf << '\t' << cx << ", " << "8(" << bx << ')' << '\n';
    }
    else {
      out << '\t' << "mov" << sf << '\t' << "[" << bx << "  ]" << ", " << ax << '\n';
      out << '\t' << "mov" << sf << '\t' << "[" << bx << "+4]" << ", " << dx << '\n';
      if (size == 12)
        out << '\t' << "mov" << sf << '\t' << "[" << bx << "+8]" << ", " << cx << '\n';
    }
  }
}

void intel::invladdr_impl::multi(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->z->m_type;
  int size = T->size();
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load(reg::ax);
  if (x64 && tac->z->m_scope->m_id == scope::PARAM) {
    string rbx = reg::name(reg::bx, psize());
    char ps = psuffix();
    if (mode == GNU)
      out << '\t' << "mov" << ps << '\t' << z->expr() << ", " << rbx << '\n';
    else
      out << '\t' << "mov" << '\t' << rbx << ", QWORD PTR " << z->expr() << '\n';
    copy(0, 0, size);
  }
  else
    copy(0, z, size);
}

namespace intel { namespace invraddr_impl {
  void single(COMPILER::tac*);
  void multi(COMPILER::tac*);
} } // end of namespace invraddr_impl and intel

void intel::invraddr(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace invraddr_impl;
  const type* T = tac->x->m_type;
  int size = T->size();
  (T->scalar() && size <= 8) ? single(tac) : multi(tac);
}

void intel::invraddr_impl::single(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  const type* T = tac->x->m_type;
  int size = T->size();
  y->load(reg::ax);
  if (T->real()) {
    string rax = reg::name(reg::ax, psize());
    char sf = fsuffix(size);
    if (mode == GNU)
      out << '\t' << "fld" << sf << '\t' << '(' << rax << ')' << '\n';
    else
      out << '\t' << "fld" << '\t' << ms_pseudo(size) << " PTR [" << rax << ']' << '\n';
    fstp(tac->x);
  }
  else {
    if ( x64 || size <= 4 ){
      string ptr = reg::name(reg::ax, psize());
      string  ax = reg::name(reg::ax, size);
      char sf = suffix(size);
      if (mode == GNU)
        out << '\t' << "mov" << sf << '\t' << '(' << ptr << ')' << ", " << ax << '\n';
      else
        out << '\t' << "mov" << '\t' << ax << ", " << '[' << ptr << ']' << '\n';
    }
    else {
      assert(!x64 && size == 8);
      int sz = 4;
      string ax = reg::name(reg::ax, sz);
      string dx = reg::name(reg::dx, sz);
      char sf = suffix(sz);
      if (mode == GNU) {
        out << '\t' << "mov" << sf << '\t' << "4(" << ax << "), " << dx << '\n';
        out << '\t' << "mov" << sf << '\t' << " (" << ax << "), " << ax << '\n';
      }
      else {
        out << '\t' << "mov" << '\t' << dx << ", [" << ax << "+4]" << '\n';
        out << '\t' << "mov" << '\t' << ax << ", [" << ax <<   "]" << '\n';
      }
    }
    address* x = getaddr(tac->x);
    x->store();
  }
}

void intel::invraddr_impl::multi(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  address* y = getaddr(tac->y);
  y->load(reg::ax);
  address* x = getaddr(tac->x);
  if (x64 && tac->x->m_scope->m_id == scope::PARAM) {
    string rax = reg::name(reg::ax, psize());
    out << '\t' << "mov" << psuffix() << '\t' << x->expr() << ", " << rax << '\n';
    copy(0, 0, size);
  }
  else
    copy(x, 0, size);
}

namespace intel { namespace param_impl {
  void paramx64(COMPILER::tac*);
  void paramx86(COMPILER::tac*);
} } // end of namespace param_impl and intel;

void intel::param(COMPILER::tac* tac)
{
  using namespace param_impl;
  x64 ? paramx64(tac) : paramx86(tac);
}

namespace intel { namespace param_impl {
  int offset;
} } // end of namespace param_impl and intel;

void intel::param_impl::paramx64(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  aggregate_func::param::first_t::const_iterator p =
    aggregate_func::param::first.find(tac);
  if (p != aggregate_func::param::first.end())
    param_impl::offset = psize();

  const type* T = tac->y->m_type;
  T = T->complete_type();
  address* y = getaddr(tac->y);
  int size = T->size();
  if (T->scalar() && size <= 8)
    y->load(reg::ax);
  else {
    map<COMPILER::tac*, stack*>::const_iterator p =
      aggregate_func::param::table.find(tac);
    assert(p != aggregate_func::param::table.end());
    stack* x = p->second;
    if (tac->y->m_scope->m_id == scope::PARAM) {
      char sf = psuffix();
      if (mode == GNU)
        out << '\t' << "mov" << sf << '\t' << y->expr() << ", %rax" << '\n';
      else
        out << '\t' << "mov" << sf << '\t' << "rax, " << y->expr() << '\n';
      copy(x, 0, size);
    }
    else {
      if (mode == GNU)
        copy(x, y, size);
      else {
        if (y->m_id == address::MEM) {
          y->get(reg::ax);
          copy(x, 0, size);
        }
        else
          copy(x, y, size);
      }
    }
    x->get(reg::ax);
  }

  int src_size = T->integer() ? size : psize();
  string src = reg::name(reg::ax, src_size);
  string dst, dst2;
  if (param_impl::offset < 0x20) {
    dst = param_reg(param_impl::offset >> 3, T);
    if (call_arg::ellipsised.find(tac) != call_arg::ellipsised.end()) {
      if (T->real())
        dst2 = param_reg(param_impl::offset >> 3, 0);
    }
  }
  else {
    ostringstream os;
    if (mode == GNU)
      os << param_impl::offset << '(' << sp() << ')';
    else
      os << ms_pseudo(src_size) << " PTR " << '[' << sp() << '+' << param_impl::offset << ']';
    dst = os.str();
  }

  char sf = suffix(src_size);
  char sf2 = psuffix();
  if (mode == GNU) {
    out << '\t' << "mov" << sf  << '\t' << src << ", " << dst  << '\n';
    if (!dst2.empty())
      out << '\t' << "mov" << sf2 << '\t' << src << ", " << dst2 << '\n';
  }
  else {
    if (T->real() && param_impl::offset < 0x20 )
      sf = 'q';
    out << '\t' << "mov" << sf  << '\t' << dst  << ", " << src << '\n';
    if (!dst2.empty())
      out << '\t' << "mov" << sf2 << '\t' << dst2 << ", " << src << '\n';
  }

  param_impl::offset += 8;
}

void intel::param_impl::paramx86(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  aggregate_func::param::first_t::const_iterator p =
    aggregate_func::param::first.find(tac);
  if (p != aggregate_func::param::first.end())
    param_impl::offset = psize();

  const type* T = tac->y->m_type;
  T = T->complete_type();
  int size = T->size();
  address* y = getaddr(tac->y);
  if (T->scalar()) {
    y->load();
    if (T->real()) {
      if (mode == GNU) {
        out << '\t' << "fstp" << fsuffix(size) << '\t';
        out << param_impl::offset << '(' << sp() << ')' << '\n';
      }
      else {
        out << '\t' << "fstp" << '\t' << ms_pseudo(size) << " PTR ";
        out << param_impl::offset << '[' << sp() << ']' << '\n';
      }
    }
    else {
      if (size <= 4) {
        if (mode == GNU) {
          out << '\t' << "mov" << suffix(size) << '\t' << reg::name(reg::ax, size) << ", ";
          out << param_impl::offset << '(' << sp() << ')' << '\n';
        }
        else {
          std::string ptr = ms_pseudo(size) + " PTR ";
          out << '\t' << "mov" << '\t' << ptr << param_impl::offset << '[' << sp() << ']' << ", ";
          out << reg::name(reg::ax, size) << '\n';
        }
      }
      else {
        if (mode == GNU) {
          out << '\t' << "movl" << '\t' << "%eax" << ", ";
          out << param_impl::offset << '(' << sp() << ')' << '\n';
          out << '\t' << "movl" << '\t' << "%edx" << ", ";
          out << param_impl::offset + 4 << '(' << sp() << ')' << '\n';
          if (size == 12) {
            out << '\t' << "movl" << '\t' << "%ecx" << ", ";
            out << param_impl::offset + 8 << '(' << sp() << ')' << '\n';
          }
        }
        else {
          std::string ptr = " DWORD PTR ";
          out << '\t' << "mov" << '\t' << ptr << param_impl::offset + 0 << '[' << sp() << ']' << ", " << "eax" << '\n';
          out << '\t' << "mov" << '\t' << ptr << param_impl::offset + 4 << '[' << sp() << ']' << ", " << "edx" << '\n';
          if (size == 12) {
            out << '\t' << "movl" << '\t' << ptr << param_impl::offset + 8 << '[' << sp() << ']' << ", " << "ecx" << '\n';
          }
        }
      }
    }
  }
  else {
    if (mode == GNU)
      out << '\t' << "leal" << '\t' << param_impl::offset << '(' << sp() << "), %eax" << '\n';
    else
      out << '\t' << "lea" << '\t' << "eax, " << param_impl::offset << '[' << sp() << "]" << '\n';
    copy(0,y,size);          
  }
  T = T->promotion();
  size = T->size();
  param_impl::offset += size;
}

namespace intel { namespace call_impl {
  void single(COMPILER::tac*);
  void multi(COMPILER::tac*);
  void Void(COMPILER::tac*);
} } // end of namespace call_impl and intel;

void intel::call(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace call_impl;
  if ( tac->x ){
    const type* T = tac->x->m_type;
    if (x64) {
      int size = T->size();
      (T->scalar() && size <= 8) ? single(tac) : multi(tac);
    }
    else
      T->scalar() ? single(tac) : multi(tac);
  }
  else if (aggregate_func::ret::size(tac)) {
    // hcc1 can eliminate `x' where x := call f and `x' is not
    // used after this call.
    multi(tac);
  }
  else
    Void(tac);
}

void intel::call_impl::single(COMPILER::tac* tac)
{
  Void(tac);
  address* x = getaddr(tac->x);
  x->store();
}

void intel::call_impl::multi(COMPILER::tac* tac)
{
  address* x = tac->x ? getaddr(tac->x) : aggregate_func::ret::area;
  x->get(reg::cx);
  if (!x64) {
    if (mode == GNU)
      out << '\t' << "movl" << '\t' << "%ecx, (%esp)" << '\n';
    else
      out << '\t' << "mov" << '\t' << "DWORD PTR [esp], ecx" << '\n';
  }
  Void(tac);
}

namespace intel {
  namespace to_impl {
    using namespace std;
    string label(COMPILER::tac*);
  }  // end of namespace to_impl
}  // end of namespace intel

void intel::call_impl::Void(COMPILER::tac* tac)
{
  using namespace COMPILER;
  param_impl::offset = 0;
  address* y = getaddr(tac->y);
  const type* T = tac->y->m_type;
  if (T->scalar())
    y->load(reg::ax);
  out << '\t' << "call" << '\t';
  if ( T->scalar() ) {
    if (mode == GNU )
      out << '*';
    out << reg::name(reg::ax, psize());    
  }
  else {
    assert(y->m_id == address::MEM);
    mem* m = static_cast<mem*>(y);
    string label = m->m_label;
    out << label;
    if (mode == MS) {
      usr* func = tac->y->usr_cast();
      usr::flag_t f = func->m_flag;
      if (!(f & usr::STATIC))
        mem::refed.insert(mem::refgen_t(label,f,0));
    }
  }
  out << '\n';
}

namespace intel {
  namespace return_impl {
    void single(COMPILER::tac*);
    void multi(COMPILER::tac*);
  } // end of namespace return_impl
} // end of namespace intel

void intel::_return(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace return_impl;
  if ( var* y = tac->y ){
    const type* T = y->m_type;
    int size = T->size();
    if ( x64 )
      ( T->scalar() && size <= 8 ) ? single(tac) : multi(tac);
    else
      T->scalar() ? single(tac) : multi(tac);
  }
  if ( return_impl::last3ac != tac ){
    if ( return_impl::leave_label.empty() )
      return_impl::leave_label = new_label((mode == GNU) ? ".label" : "label$");
    out << '\t' << "jmp" << '\t' << return_impl::leave_label << '\n';
  }
}

void intel::return_impl::single(COMPILER::tac* tac)
{
  address* y = getaddr(tac->y);
  y->load();
}

void intel::return_impl::multi(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->y->m_type;
  int size = T->size();
  char ps = psuffix();
  string FP = fp();
  address* y = getaddr(tac->y);
  int psz = psize();
  out << '\t' << "mov" << ps << '\t';
  if (mode == GNU)
    out << first_param_offset << '(' << FP << "), " << reg::name(reg::ax, psize()) << '\n';
  else
    out << reg::name(reg::ax, psz) << ", " << first_param_offset << '[' << FP << ']' << '\n';

  if (x64 && tac->y->m_scope->m_id == scope::PARAM) {
    if (mode == GNU)
      out << '\t' << "movq" << '\t' << y->expr() << ", %rbx" << '\n';
    else
      out << '\t' << "mov" << '\t' << "rbx, QWORD PTR " << y->expr() << '\n';
    copy(0, 0, size);
  }
  else
    copy(0, y, size);
}

namespace intel {
  namespace goto_impl {
    void cond(COMPILER::goto3ac*);
  }  // end of namespace goto_impl
}  // end of namespace intel

void intel::_goto(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  goto3ac* go = static_cast<goto3ac*>(tac);
  if (!go->m_op)
    out << '\t' << "jmp" << '\t' << to_impl::label(go->m_to) << '\n';
  else
    goto_impl::cond(go);
}

namespace intel { namespace goto_impl {
  void integer(COMPILER::goto3ac*);
  void real(COMPILER::goto3ac*);
} }  // end of namespace goto_impl and intel

void intel::goto_impl::cond(COMPILER::goto3ac* go)
{
  using namespace COMPILER;
  const type* T = go->y->m_type;
  T->real() ? real(go) : integer(go);
}

namespace intel { namespace goto_impl {
  using namespace std;
  using namespace COMPILER;
  struct table : map<pair<goto3ac::op,bool>, string> {
    table();
  } m_table;
  void longlongx86(goto3ac* go);
} } // end of namespace goto_impl, notlonglong_impl and intel

void intel::goto_impl::integer(COMPILER::goto3ac* go)
{
  using namespace std;
  using namespace COMPILER;
  const type* Ty = go->y->m_type;
  const type* Tz = go->z->m_type;
  int sy = Ty->size();
  int sz = Tz->size();
  
  if (!x64 && sy == sz && sy == 8)
    return longlongx86(go);

  int size = x64 ? max(sy, sz) : min(sy, sz);
  string op = m_table[make_pair(go->m_op, !Ty->_signed())];
  address* y = getaddr(go->y);
  address* z = getaddr(go->z);
  y->load(reg::ax);
  string ax = reg::name(reg::ax, size);
  char sf = suffix(size);

  if (mode == GNU)
    out << '\t' << "cmp" << sf << '\t' << z->expr() << ", " << ax << '\n';
  else {
    out << '\t' << "cmp" << '\t' << ax << ", ";
    if (x64)
      out << z->expr();
    else
      out << z->expr(0, true);
    out << '\n';
  }

  out << '\t' << op << '\t' << to_impl::label(go->m_to) << '\n';
}

intel::goto_impl::table::table()
{
  using namespace std;
  using namespace COMPILER;
  typedef pair<goto3ac::op,bool> key;
  (*this)[key(goto3ac::EQ,false)] = "je";  (*this)[key(goto3ac::EQ,true)] = "je";
  (*this)[key(goto3ac::NE,false)] = "jne"; (*this)[key(goto3ac::NE,true)] = "jne";
  (*this)[key(goto3ac::LT,false)] = "jl";  (*this)[key(goto3ac::LT,true)] = "jb";
  (*this)[key(goto3ac::GT,false)] = "jg";  (*this)[key(goto3ac::GT,true)] = "ja";
  (*this)[key(goto3ac::LE,false)] = "jle"; (*this)[key(goto3ac::LE,true)] = "jbe";
  (*this)[key(goto3ac::GE,false)] = "jge"; (*this)[key(goto3ac::GE,true)] = "jae";
}

void intel::goto_impl::real(COMPILER::goto3ac* go)
{
  using namespace std;
  using namespace COMPILER;
  fld(go->z);
  fld(go->y);

  out << '\t' << "fucompp" << '\n';
  out << '\t' << "fnstsw" << '\t' << reg::name(reg::ax,2) << '\n';
  out << '\t' << "sahf" << '\n';
  string label;
  if ( go->m_op == goto3ac::EQ ){
    label = new_label((mode == GNU) ? ".label" : "label$");
    out << '\t' << "jp" << '\t' << label << '\n';
  }
  pair<goto3ac::op,bool> key(go->m_op,true);
  out << '\t' << m_table[key] << '\t' << to_impl::label(go->m_to) << '\n';

  if ( go->m_op == goto3ac::EQ )
    out << label << ":\n";
  if (go->m_op == goto3ac::NE)
          out << '\t' << "jp" << '\t' << to_impl::label(go->m_to) << '\n';
}

namespace intel { namespace goto_impl { namespace longlongx86_impl {
  using namespace std;
  using namespace COMPILER;
  struct table : map<pair<goto3ac::op,bool>, vector<string> > {
    table();
  } m_table;
  void GNU_subr(goto3ac* go);
  void MS_subr(goto3ac* go);
} } } // end of namespace longlongx86_impl, goto_impl and intel

void intel::goto_impl::longlongx86(COMPILER::goto3ac* go)
{
  using namespace longlongx86_impl;
  mode == GNU ? GNU_subr(go) : MS_subr(go);
}

intel::goto_impl::longlongx86_impl::table::table()
{
  using namespace std;
  using namespace COMPILER;
  typedef pair<goto3ac::op,bool> key;
  {
    vector<string>& vec = (*this)[key(goto3ac::EQ,false)];
    vec.push_back(""); vec.push_back("jne"); vec.push_back("je");
  }
  {
    (*this)[key(goto3ac::EQ,true)] = (*this)[key(goto3ac::EQ,false)];
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::NE,false)];
    vec.push_back("jne"); vec.push_back(""); vec.push_back("jne");
  }
  {
    (*this)[key(goto3ac::NE,true)] = (*this)[key(goto3ac::NE,false)];
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::LT,false)];
    vec.push_back("jl"); vec.push_back("jg"); vec.push_back("jb");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::LT,true)];
    vec.push_back("jb"); vec.push_back("ja"); vec.push_back("jb");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::GT,false)];
    vec.push_back("jg"); vec.push_back("jl"); vec.push_back("ja");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::GT,true)];
    vec.push_back("ja"); vec.push_back("jb"); vec.push_back("ja");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::LE,false)];
    vec.push_back("jl"); vec.push_back("jg"); vec.push_back("jbe");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::LE,true)];
    vec.push_back("jb"); vec.push_back("ja"); vec.push_back("jbe");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::GE,false)];
    vec.push_back("jg"); vec.push_back("jl"); vec.push_back("jae");
  }
  {
    vector<string>& vec = (*this)[key(goto3ac::GE,true)];
    vec.push_back("ja"); vec.push_back("jb"); vec.push_back("jae");
  }
}

void intel::goto_impl::longlongx86_impl::GNU_subr(COMPILER::goto3ac* go)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(go->y);
  address* z = getaddr(go->z);
  out << '\t' << "movl" << '\t' << y->expr(4, true) << ", %eax" << '\n';
  out << '\t' << "cmpl" << '\t' << z->expr(4, true) << ", %eax" << '\n';
  bool u = !go->y->m_type->_signed();
  const vector<string>& op = longlongx86_impl::m_table[make_pair(go->m_op,u)];
  if ( !op[0].empty() )
    out << '\t' << op[0] << '\t' << to_impl::label(go->m_to) << '\n';
  string label2 = new_label(".label");
  if ( !op[1].empty() )
    out << '\t' << op[1] << '\t' << label2 << '\n';
  out << '\t' << "movl" << '\t' << y->expr(0, true) << ", %eax" << '\n';
  out << '\t' << "cmpl" << '\t' << z->expr(0, true) << ", %eax" << '\n';
  out << '\t' << op[2] << '\t' << to_impl::label(go->m_to) << '\n';
  out << label2 << ":\n";
}

void intel::goto_impl::longlongx86_impl::MS_subr(COMPILER::goto3ac* go)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(go->y);
  address* z = getaddr(go->z);
  out << '\t' << "mov" << '\t' << "eax, " << y->expr(4, true) << '\n';
  out << '\t' << "cmp" << '\t' << "eax, " << z->expr(4, true) << '\n';
  bool u = !go->y->m_type->_signed();
  const vector<string>& op = longlongx86_impl::m_table[make_pair(go->m_op,u)];
  if ( !op[0].empty() )
    out << '\t' << op[0] << '\t' << to_impl::label(go->m_to) << '\n';
  string label2 = new_label("label$");
  if ( !op[1].empty() )
    out << '\t' << op[1] << '\t' << label2 << '\n';
  out << '\t' << "mov" << '\t' << "eax, " << y->expr(0, true) << '\n';
  out << '\t' << "cmp" << '\t' << "eax, " << z->expr(0, true) << '\n';
  out << '\t' << op[2] << '\t' << to_impl::label(go->m_to) << '\n';
  out << label2 << ":\n";
}

namespace intel {
  namespace to_impl {
    using namespace std;
    using namespace COMPILER;
    string label(tac* tac)
    {
      using namespace std;
      ostringstream os;
      if (mode == GNU)
        os << '.' << 'L' << func_label << tac;
      else
        os << 'L' << func_label << tac << '$';
      return os.str();
    }
  }  // end of namespace to_impl
} // end of namespace intel

void intel::to(COMPILER::tac* tac)
{
  out << to_impl::label(tac) << ":\n";
}

namespace intel { namespace loff_impl {
  void single(COMPILER::tac* tac);
  void multi(COMPILER::tac* tac);
} } // end of namespace loff_impl and intel

void intel::loff(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace loff_impl;
  const type* T = tac->z->m_type;
  int size = T->size();
  if ( x64 )
    (T->scalar() && size <= 8) ? single(tac) : multi(tac);
  else
    T->scalar() ? single(tac) : multi(tac);
}

namespace intel { namespace loff_impl {
  void single(COMPILER::tac* tac, int delta);
} } // end of namespace loff_impl and intel

void intel::loff_impl::single(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  {
    var* y = tac->y;
    if ( y->isconstant() )
      return single(tac,y->value());
  }
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  const type* Tz = tac->z->m_type;
  int size = Tz->size();
  if (x64)
    assert(size <= 8);
  if (x64 && tac->x->m_scope->m_id == scope::PARAM) {
    string rbx = reg::name(reg::bx, psize());
    if (mode == GNU) {
      out << '\t' << "mov" << psuffix() << '\t' << x->expr();
      out << ", " << rbx << '\n';
    }
    else {
      string ptr = ms_pseudo(size) + " PTR ";
      out << '\t' << "mov" << psuffix() << '\t' << rbx;
      out << ", " << x->expr() << '\n';
    }
  }
  else
    x->get(reg::bx);
  y->load(reg::cx);
  int psz = psize();
  string bx = reg::name(reg::bx, psz);
  string cx = reg::name(reg::cx, psz);
  const type* Ty = tac->y->m_type;
  int sy = Ty->size();
  if (sy < psz) {
    if (mode == GNU)
      out << '\t' << "movslq" << '\t' << reg::name(reg::cx, sy) << ", " << cx << '\n';
    else
      out << '\t' << "movsxd" << '\t' << cx << ", " << reg::name(reg::cx, sy) << '\n';
  }
  if (mode == GNU)
    out << '\t' << "add" << psuffix() << '\t' << cx << ", " << bx << '\n';
  else
    out << '\t' << "add" << '\t' << bx << ", " << cx << '\n';

  z->load(reg::ax);
  if (x64 || size <= 4) {
    char sf = suffix(size);
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << reg::name(reg::ax, size) << ", (" << bx << ')' << '\n';
    else
      out << '\t' << "mov" << '\t' << '[' << bx << ']' << ", " << reg::name(reg::ax, size) << '\n';
  }
  else {
    if (mode == GNU) {
      out << '\t' << "movl" << '\t' << "%eax" << ", 0(" << "%ebx" << ')' << '\n';
      out << '\t' << "movl" << '\t' << "%edx" << ", 4(" << "%ebx" << ')' << '\n';
      if (size == 12)
        out << '\t' << "movl" << '\t' << "%ecx" << ", 8(" << "%ebx" << ')' << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' <<  '[' << "ebx" << ']' << ", " << "eax" << '\n';
      out << '\t' << "mov" << '\t' << "4[" << "ebx" << ']' << ", " << "edx" << '\n';
      if (size == 12)
        out << '\t' << "mov" << '\t' << "8[" << "ebx" << ']' << ", " << "ecx" << '\n';
    }
  }
}

void intel::loff_impl::single(COMPILER::tac* tac, int delta)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* z = getaddr(tac->z);
  z->load(reg::ax);
  const type* T = tac->z->m_type;
  int size = T->size();
  if ( x64 )
    assert(size <= 8);
  if (x64 || size <= 4) {
    string ax = reg::name(reg::ax, size);
    char sf = suffix(size);
    if (x64 && tac->x->m_scope->m_id == scope::PARAM) {
      string rbx = reg::name(reg::bx, psize());
      if (mode == GNU) {
        out << '\t' << "mov" << psuffix() << '\t' << x->expr() << ", " << rbx << '\n';
        out << '\t' << "add" << psuffix() << '\t' << '$' << delta << ", " << rbx << '\n';
        out << '\t' << "mov" << sf << '\t' << ax << ", " << '(' << rbx << ')' << '\n';
      }
      else {
        string ptr = ms_pseudo(size) + " PTR ";
        out << '\t' << "mov" << psuffix() << '\t' << rbx << ", " << x->expr() << '\n';
        out << '\t' << "add" << psuffix() << '\t' << rbx << ", " << delta << '\n';
        out << '\t' << "mov" << sf << '\t' << ptr << '[' << rbx << ']' << ", " << ax << '\n';
      }
    }
    else {
      if (mode == GNU)
        out << '\t' << "mov" << sf << '\t' << ax << ", " << x->expr(delta) << '\n';
      else {
        string ptr = ms_pseudo(size) + " PTR ";
        if (!x64 && x->m_id == address::MEM) {
          x->get(reg::bx);
          out << '\t' << "mov" << '\t' << ptr;
          if (delta)
            out << dec << delta;
          out << "[ebx], " << ax << '\n';
        }
        else
          out << '\t' << "mov" << '\t' << ptr << x->expr(delta) << ", " << ax << '\n';
      }
    }
  }
  else {
    string eax = reg::name(reg::ax, 4);
    string edx = reg::name(reg::dx, 4);
    string ecx = reg::name(reg::cx, 4);
    char sf = suffix(4);

    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << eax << ", " << x->expr(delta + 0) << '\n';
      out << '\t' << "mov" << sf << '\t' << edx << ", " << x->expr(delta + 4) << '\n';
      if (size == 12)
        out << '\t' << "mov" << sf << '\t' << ecx << ", " << x->expr(delta + 8) << '\n';
    }
    else {
      string ptr = ms_pseudo(4) + " PTR ";
      out << '\t' << "mov" << sf << '\t' << ptr << x->expr(delta + 0) << ", " << eax << '\n';
      out << '\t' << "mov" << sf << '\t' << ptr << x->expr(delta + 4) << ", " << edx << '\n';
      if (size == 12)
        out << '\t' << "mov" << sf << '\t' << ptr << x->expr(delta + 8) << ", " << ecx << '\n';
    }
  }
}

namespace intel { namespace loff_impl {
  void multi(COMPILER::tac* tac, int delta);
} } // end of namespace loff_impl and intel

void intel::loff_impl::multi(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  {
    var* y = tac->y;
    if ( y->isconstant() )
      return multi(tac,y->value());
  }
  const type* Tz = tac->z->m_type;
  int size = Tz->size();
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  int psz = psize();
  string ax = reg::name(reg::ax, psz);
  if (x64 && tac->x->m_scope->m_id == scope::PARAM)
    out << '\t' << "mov" << psuffix() << '\t' << x->expr() << ", " << ax << '\n';
  else
    x->get(reg::ax);
  y->load(reg::bx);
  string bx = reg::name(reg::bx, psz);
  const type* Ty = tac->y->m_type;
  int sy = Ty->size();
  if (sy < psz) {
    if (mode == GNU)
      out << '\t' << "movslq" << '\t' << reg::name(reg::bx, sy) << ", " << bx << '\n';
    else
      out << '\t' << "movsxd" << '\t' << bx << ", " << reg::name(reg::bx, sy) << '\n';
  }
  if (mode == GNU)
    out << '\t' << "add" << psuffix() << '\t' << bx << ", " << ax << '\n';
  else
    out << '\t' << "add" << '\t' << ax << ", " << bx << '\n';

  if (x64 && tac->z->m_scope->m_id == scope::PARAM) {
    if (mode == GNU)
      out << '\t' << "mov" << psuffix() << '\t' << z->expr() << ", " << bx << '\n';
    else
      out << '\t' << "mov" << psuffix() << '\t' << bx << ", " << z->expr() << '\n';
    copy(0, 0, size);
  }
  else
    copy(0, z, size);
}

void intel::loff_impl::multi(COMPILER::tac* tac, int delta)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->z->m_type;
  int size = T->size();
  address* x = getaddr(tac->x);
  string rax = reg::name(reg::ax, psize());
  char ps = psuffix();
  if (x64 && tac ->x->m_scope->m_id == scope::PARAM) {
    if (mode == GNU) {
      out << '\t' << "mov" << ps << '\t' << x->expr() << ", " << rax << '\n';
      out << '\t' << "add" << ps << '\t' << '$' << delta << ", " << rax << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << rax << ", QWORD PTR " << x->expr() << '\n';
      out << '\t' << "add" << '\t' << rax << ", " << delta << '\n';
    }
  }
  else {
    if (mode == GNU)
      out << '\t' << "lea" << ps << '\t' << x->expr(delta) << ", " << rax << '\n';
    else
      out << '\t' << "lea" << '\t' << rax << ", " << x->expr(delta) << '\n';
  }

  address* z = getaddr(tac->z);
  if (x64 && tac->z->m_scope->m_id == scope::PARAM) {
    if (mode == GNU)
      out << '\t' << "movq" << '\t' << z->expr() << ", %rbx" << '\n';
    else
      out << '\t' << "mov" << '\t' << "rbx, QWORD PTR " << z->expr() << '\n';
    copy(0, 0, size);
  }
  else
    copy(0, z, size);
}

namespace intel { namespace roff_impl {
  void single(COMPILER::tac* tac);
  void multi(COMPILER::tac* tac);
} } // end of namespace loff_impl and intel

void intel::roff(COMPILER::tac* tac)
{
  using namespace COMPILER;
  using namespace roff_impl;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64)
    (T->scalar() && size <= 8) ? single(tac) : multi(tac);
  else
    T->scalar() ? single(tac) : multi(tac);
}

namespace intel { namespace roff_impl {
  void single(COMPILER::tac* tac, int delta);
} } // end of namespace loff_impl and intel

void intel::roff_impl::single(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  {
    var* z = tac->z;
    if ( z->isconstant() )
      return single(tac,z->value());
  }
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  const type* Tx = tac->x->m_type;
  int size = Tx->size();
  if ( x64 )
    assert(size <= 8);
  char ps = psuffix();
  z->load(reg::dx);
  if (x64 && tac->y->m_scope->m_id == scope::PARAM) {
    if (mode == GNU)
      out << '\t' << "mov" << ps << '\t' << y->expr() << ", %rax" << '\n';
    else
      out << '\t' << "mov" << '\t' << "rax, " << y->expr() << '\n';
  }
  else
    y->get(reg::ax);
  int psz = psize();
  string ax = reg::name(reg::ax, psz);
  string dx = reg::name(reg::dx, psz);
  const type* Tz = tac->z->m_type;
  int sz = Tz->size();
  if (sz < psz) {
    if (mode == GNU) {
      out << '\t' << "movslq" << '\t' << reg::name(reg::dx, sz);
      out << ", " << dx << '\n';
    }
    else {
      out << '\t' << "movsxd" << '\t' << dx;
      out << ", " << reg::name(reg::dx, sz) << '\n';
    }
  }
  if (mode == GNU)
    out << '\t' << "add" << ps << '\t' << dx << ", " << ax << '\n';
  else
    out << '\t' << "add" << '\t' << ax << ", " << dx << '\n';

  if (x64 || size <= 4) {
    char sf = suffix(size);
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << '(' << ax << "), " << reg::name(reg::ax, size) << '\n';
    else
      out << '\t' << "mov" << sf << '\t' << reg::name(reg::ax, size) << ", " << '[' << ax << ']' << '\n';
  }
  else {
    if (mode == GNU) {
      if (size == 12)
        out << '\t' << "movl" << '\t' << "8(%eax), %ecx" << '\n';
      out << '\t' << "movl" << '\t' << "4(%eax), %edx" << '\n';
      out << '\t' << "movl" << '\t' << " (%eax), %eax" << '\n';
    }
    else {
      if (size == 12)
        out << '\t' << "mov" << '\t' << "ecx, 8[eax]" << '\n';
      out << '\t' << "mov" << '\t' << "edx, 4[eax]" << '\n';
      out << '\t' << "mov" << '\t' << "eax, [eax]" << '\n';
    }
  }
  x->store(reg::ax);
}

void intel::roff_impl::single(COMPILER::tac* tac, int delta)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  const type* T = tac->x->m_type;
  int size = T->size();
  char ps = psuffix();
  if (x64 && tac->y->m_scope->m_id == scope::PARAM) {
    if (mode == GNU)
      out << '\t' << "mov" << ps << '\t' << y->expr() << ", %rax" << '\n';
    else
      out << '\t' << "mov" << ps << '\t' << "rax , " << y->expr() << '\n';
    if (T->real()) {
      out << '\t' << "fld";
      if (mode == GNU)
        out << fsuffix(size) << '\t';
      else
        out << '\t' << ms_pseudo(size) << " PTR ";
      if (delta)
        out << delta;
      if (mode == GNU)
        out << "(%rax)" << '\n';
      else
        out << "[rax]" << '\n';
      fstp(tac->x);
    }
    else {
      out << '\t' << "mov" << suffix(size) << '\t';
      if (mode == GNU) {
        if (delta)
          out << delta;
        out << "(%rax), " << reg::name(reg::ax, size) << '\n';
      }
      else {
        out << reg::name(reg::ax, size) << ", ";
        if (delta)
          out << delta;
        out << "[rax]" << '\n';
      }
      address* x = getaddr(tac->x);
      x->store();
    }
  }
  else {
    if (T->real()) {
      if (mode == GNU)
        out << '\t' << "fld" << fsuffix(size) << '\t' << y->expr(delta) << '\n';
      else {
        string ptr = ms_pseudo(size) + " PTR ";
        out << '\t' << "fld" << '\t' << ptr << y->expr(delta) << '\n';
      }
      fstp(tac->x);
    }
    else {
      if (x64 || size <= 4) {
        char sf = suffix(size);
        string ax = reg::name(reg::ax, size);
        if (mode == GNU)
          out << '\t' << "mov" << sf << '\t' << y->expr(delta) << ", " << ax << '\n';
        else {
          if (y->m_id == address::MEM) {
            y->get(reg::ax);
            out << '\t' << "mov" << sf << '\t' << ax << ", ";
            if (delta)
              out << dec << delta;
            out << '[' << reg::name(reg::ax, psize()) << ']' << '\n';
          }
          else
            out << '\t' << "mov" << sf << '\t' << ax << ", " << y->expr(delta) << '\n';
        }
      }
      else {
        assert(!x64 && size == 8);
        if (mode == GNU) {
          out << '\t' << "movl" << '\t' << y->expr(delta + 0) << ", %eax" << '\n';
          out << '\t' << "movl" << '\t' << y->expr(delta + 4) << ", %edx" << '\n';
        }
        else {
          string ptr = "DWORD PTR ";
          out << '\t' << "mov" << '\t' << "eax, " << ptr << y->expr(delta + 0) << '\n';
          out << '\t' << "mov" << '\t' << "edx, " << ptr << y->expr(delta + 4) << '\n';
        }
      }
      address* x = getaddr(tac->x);
      x->store();
    }
  }
}

namespace intel { namespace roff_impl {
  void multi(COMPILER::tac* tac, int delta);
} } // end of namespace loff_impl and intel

void intel::roff_impl::multi(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  {
    var* z = tac->z;
    if ( z->isconstant() )
      return multi(tac,z->value());
  }

  const type* Tx = tac->x->m_type;
  int size = Tx->size();
  address* y = getaddr(tac->y);
  if (x64 && tac->y->m_scope->m_id == scope::PARAM) {
    char sf = psuffix();
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << y->expr() << ", %rax" << '\n';
    else
      out << '\t' << "mov" << sf << '\t' << "eax, " << y->expr() << '\n';
  }
  else
    y->get(reg::ax);
  address* z = getaddr(tac->z);
  z->load(reg::dx);
  int psz = psize();
  string ax = reg::name(reg::ax, psz);
  string dx = reg::name(reg::dx, psz);
  const type* Tz = tac->z->m_type;
  int sz = Tz->size();
  if (sz < psz) {
    if (mode == GNU)
      out << '\t' << "movslq" << '\t' << reg::name(reg::dx, sz) << ", " << dx << '\n';
    else
      out << '\t' << "movsxd" << '\t' << dx << ", " << reg::name(reg::dx, sz) << '\n';
  }
  char sf = psuffix();
  if (mode == GNU)
    out << '\t' << "add" << sf << '\t' << dx << ", " << ax << '\n';
  else
    out << '\t' << "add" << sf << '\t' << ax << ", " << dx << '\n';
  address* x = getaddr(tac->x);
  if (x64 && tac->x->m_scope->m_id == scope::PARAM) {
    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << "%rax, %rbx" << '\n';
      out << '\t' << "mov" << sf << '\t' << x->expr() << ", " << "%rax" << '\n';
    }
    else {
      out << '\t' << "mov" << sf << '\t' << "rbx, rax" << '\n';
      out << '\t' << "mov" << sf << '\t' << "rax" << ", " << x->expr() << '\n';
    }
    copy(0, 0, size);
  }
  else
    copy(x, 0, size);
}

void intel::roff_impl::multi(COMPILER::tac* tac, int delta)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  char sf = psuffix();
  if (x64 && tac->y->m_scope->m_id == scope::PARAM) {
    if (mode == GNU)
      out << '\t' << "mov" << sf << '\t' << y->expr() << ", %rax" << '\n';
    else
      out << '\t' << "mov" << sf << '\t' << "rax, " << y->expr() << '\n';
  }
  else
    y->get(reg::ax);
  if (mode == GNU)
    out << '\t' << "add" << sf << '\t' << '$' << delta << ", " << reg::name(reg::ax, psize()) << '\n';
  else
    out << '\t' << "add" << '\t' << reg::name(reg::ax, psize()) << ", " << delta << '\n';

  if (x64 && tac->x->m_scope->m_id == scope::PARAM) {
    if (mode == GNU) {
      out << '\t' << "mov" << sf << '\t' << "%rax, %rbx" << '\n';
      out << '\t' << "mov" << sf << '\t' << x->expr() << ", " << "%rax" << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << "rbx, rax" << '\n';
      out << '\t' << "mov" << '\t' << "rax, " << x->expr() << '\n';
    }
    copy(0, 0, size);
  }
  else
    copy(x, 0, size);
}

void intel::_alloca_(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load(reg::ax);

  char ps = psuffix();
  int psz = psize();
  string ax = reg::name(reg::ax, psz);
  string bx = reg::name(reg::bx, psz);
  string cx = reg::name(reg::cx, psz);
  int n = -allocated::base;
  ostringstream os;
  if (mode == GNU)
    os << dec << n << '(' << fp() << ')';
  else
    os << '[' << fp() << dec << n << ']';
  string fpexpr = os.str();
  string SP = sp();
  int m = stack::threshold_alloca;

#if defined(_MSC_VER) || defined(__CYGWIN__)
  if (mode == GNU) {
    out << '\t' << "add" << ps << '\t' << '$' << 15 << ", " << ax << '\n';
    out << '\t' << "mov" << ps << '\t' << '$' << 15 << ", " << bx << '\n';
    out << '\t' << "not" << ps << '\t' << bx << '\n';
    out << '\t' << "and" << ps << '\t' << bx << ", " << ax << '\n';
    out << '\t' << "mov" << ps << '\t' << '$' << dec << m << ", " << bx << '\n';
    out << '\t' << "cmp" << ps << '\t' << bx << ", " << ax << '\n';
    string label = new_label(".label");
    out << '\t' << "jge" << '\t' << label << '\n';
    out << '\t' << "sub" << ps << '\t' << ax << ", " << SP << '\n';
    string label2 = new_label(".label");
    out << '\t' << "jmp" << '\t' << label2 << '\n';
    out << label << ":\n";
    out << '\t' << "call" << '\t' << "___chkstk_ms" << '\n';
    out << '\t' << "sub" << ps << '\t' << ax << ", " << SP << '\n';
    out << label2 << ":\n";
    out << '\t' << "mov" << ps << '\t' << fpexpr << ", " << bx << '\n';
    out << '\t' << "sub" << ps << '\t' << ax << ", " << bx << '\n';
    out << '\t' << "mov" << ps << '\t' << bx << ", " << fpexpr << '\n';
  }
  else {
    out << '\t' << "add" << '\t' << ax << ", " << dec << 15 << '\n';
    out << '\t' << "mov" << '\t' << bx << ", " << dec << 15 << '\n';
    out << '\t' << "not" << '\t' << bx << '\n';
    out << '\t' << "and" << '\t' << ax << ", " << bx << '\n';
    out << '\t' << "mov" << '\t' << bx << ", " << dec << m << '\n';
    out << '\t' << "cmp" << '\t' << ax << ", " << bx << '\n';
    string label = new_label("label$");
    out << '\t' << "jge" << '\t' << label << '\n';
    out << '\t' << "sub" << '\t' << SP << ", " << ax << '\n';
    string label2 = new_label("label$");
    out << '\t' << "jmp" << '\t' << label2 << '\n';
    out << label << ":\n";
    string func = "__chkstk";
    if (x64) {
      out << '\t' << "call" << '\t' << func << '\n';
      out << '\t' << "sub" << '\t' << SP << ", " << ax << '\n';
    }
    else {
      out << '\t' << "mov" << '\t' << "ecx, eax" << '\n';
      out << '\t' << "call" << '\t' << func << '\n';
      out << '\t' << "mov" << '\t' << "eax, ecx" << '\n';
    }
    mem::refed.insert(mem::refgen_t(func, usr::FUNCTION, 0));
    out << label2 << ":\n";
    out << '\t' << "mov" << '\t' << bx << ", ";
    string ptr = ms_pseudo(psize()) + " PTR ";
    out << ptr << fpexpr << '\n';
    out << '\t' << "sub" << '\t' << bx << ", " << ax << '\n';
    out << '\t' << "mov" << '\t' << ptr << fpexpr << ", " << bx << '\n';
  }
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
  out << '\t' << "add" << ps << '\t' << '$' << 15 << ", " << ax << '\n';
  out << '\t' << "mov" << ps << '\t' << '$' << 15 << ", " << bx << '\n';
  out << '\t' << "not" << ps << '\t' << bx << '\n';
  out << '\t' << "and" << ps << '\t' << bx << ", " << ax << '\n';
  out << '\t' << "sub" << ps << '\t' << ax << ", " << SP << '\n';
  out << '\t' << "mov" << ps << '\t' << fpexpr << ", " << bx << '\n';
  out << '\t' << "sub" << ps << '\t' << ax << ", " << bx << '\n';
  out << '\t' << "mov" << ps << '\t' << bx << ", " << fpexpr << '\n';
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
  map<var*, address*>& table = address_descriptor.second;
  map<var*, address*>::const_iterator p = table.find(tac->x);
  assert(p != table.end());
  address* q = p->second;
  if (mode == GNU)
    out << '\t' << "mov" << ps << '\t' << bx << ", " << q->expr() << '\n';
  else {
    out << '\t' << "mov" << '\t';
    out << ms_pseudo(psize()) << " PTR " << q->expr() << ", " << bx << '\n';
  }
}

void intel::_asm_(COMPILER::tac* tac)
{
  using namespace COMPILER;
  asm3ac* p = static_cast<asm3ac*>(tac);
  out << '\t' << p->m_inst << '\n';
}

void intel::_va_start(COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->get(reg::ax);
  char sf = psuffix();
  if (x64) {
    if (mode == GNU)
      out << '\t' << "add" << sf << '\t' << "$8, %rax" << '\n';
    else
      out << '\t' << "add" << sf << '\t' << "rax, 8" << '\n';
  }
  else {
    const type* T = tac->y->m_type;
    T = T->promotion();
    int size = T->size();
    if (mode == GNU)
      out << '\t' << "add" << sf << '\t' << '$' << size << ", %eax" << '\n';
    else
      out << '\t' << "add" << sf << '\t' << "eax, " << size << '\n';
  }
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

namespace intel { namespace va_arg_impl {
  void common(COMPILER::tac* tac);
  void multi_x64(COMPILER::tac* tac);
} }  // end of namespace va_arg_impl, intel

void intel::_va_arg(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  using namespace va_arg_impl;
  const type* T = tac->x->m_type;
  int size = T->size();
  if (x64)
    (T->scalar() && size <= 8) ? common(tac) : multi_x64(tac);
  else
    common(tac);
}

void intel::va_arg_impl::common(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  invraddr(tac);
  address* y = getaddr(tac->y);
  y->load();
  if (x64) {
    assert(mode == GNU);
    out << '\t' << "addq" << '\t' << "$8, %rax" << '\n';
  }
  else {
    const type* T = tac->x->m_type;
    T = T->promotion();
    int size = T->size();
    if (mode == GNU)
      out << '\t' << "addl" << '\t' << '$' << size << ", %eax" << '\n';
    else
      out << '\t' << "add" << '\t' << "eax, " << size << '\n';
  }
  y->store();
}

void intel::va_arg_impl::multi_x64(COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load();
  char ps = psuffix();
  int psz = psize();
  string ax = reg::name(reg::ax, psz);
  string cx = reg::name(reg::cx, psz);
  if (mode == GNU) {
      out << '\t' << "mov" << ps << '\t' << ax << ", " << cx << '\n';
      out << '\t' << "mov" << ps << '\t' << '(' << ax << "), " << ax << '\n';
  }
  else {
      out << '\t' << "mov" << ps << '\t' << cx << ", " << ax << '\n';
      out << '\t' << "mov" << ps << '\t' << ax << ", " << '[' << ax << ']' << '\n';
  }
  address* x = getaddr(tac->x);
  const type* T = tac->x->m_type;
  int size = T->size();
  copy(x, 0, size);
  if (mode == GNU)
      out << '\t' << "add" << ps << '\t' << '$' << psz << ", " << cx << '\n';
  else
      out << '\t' << "add" << ps << '\t' << cx << ", " << psz << '\n';
  y->store(reg::cx);
}

void intel::_va_end(COMPILER::tac*)
{
  // just ignore
}

#ifdef CXX_GENERATOR
namespace intel {
    using namespace COMPILER;
    void gnu_alloce(tac* tac)
    {
      address* y = getaddr(tac->y);
      assert(y->m_id == address::IMM);
      if (x64) {
        y->load(reg::cx);
        out << '\t' << "call" << '\t' << "__cxa_allocate_exception" << '\n';
      }
      else {
        out << '\t' << "subl" << '\t' << "$12, %esp" << '\n';
        out << '\t' << "pushl" << '\t' << y->expr() << '\n';
        out << '\t' << "call" << '\t' << "__cxa_allocate_exception" << '\n';
        out << '\t' << "addl" << '\t' << "$16, %esp" << '\n';
      }
      address* x = getaddr(tac->x);
      x->store();
    }
    void ms_alloce(tac* tac)
    {
      var* v = tac->y;
      int n = v->value();
      n = align(n, 16);
      out << '\t' << "sub" << '\t' << sp() << ", " << n << '\n';
      int m = stack::local_area + n;
      int psz = psize();
      out << '\t' << "lea" << '\t' << name(reg::ax, psz) << ", ";
      string ptr = ms_pseudo(psz) + " PTR ";
      out << ptr << '[' << fp() << " - " << m << "]" << '\n';
      address* x = getaddr(tac->x);
      x->store();
    }
} // end of namespace intel

void intel::alloce(COMPILER::tac* tac)
{
  return mode == GNU ? gnu_alloce(tac) : ms_alloce(tac);
}

namespace intel {
    using namespace COMPILER;
    void gnu_throwe(tac* tac)
    {
      address* y = getaddr(tac->y);
      throw3ac* p = static_cast<throw3ac*>(tac);
      const type* T = p->m_type;
      if (x64) {
        y->load(reg::cx);
        if (T->get_tag())
          out << '\t' << "leaq" << '\t';
        else
          out << '\t' << "movq" << '\t' << ".refptr.";
        out << except::gnu_label(T, 'I') << "(%rip)" << ", %rdx" << '\n';
        out << '\t' << "movl" << '\t' << "$0, %r8d" << '\n';
        out << '\t' << "call" << '\t' << "__cxa_throw" << '\n';
      }
      else {
        y->load();
        out << '\t' << "subl" << '\t' << "$4, %esp" << '\n';
        out << '\t' << "pushl" << '\t' << "$0" << '\n';
        out << '\t' << "pushl" << '\t' << '$' << except::gnu_label(T, 'I');
        out << '\n';
        out << '\t' << "pushl" << '\t' << "%eax" << '\n';
        out << '\t' << "call" << '\t' << "__cxa_throw" << '\n';
      }
      except::throw_types.push_back(T);
    }
    void ms_throwe(tac* tac)
    {
      throw3ac* p = static_cast<throw3ac*>(tac);
      const type* T = p->m_type;
      if (x64) {
        out << '\t' << "lea" << '\t' << "rdx, ";
        out << except::ms::label(except::ms::pre1, T) << '\n';
        address* y = getaddr(tac->y);
        y->load(reg::cx);
        string label = "_CxxThrowException";
        out << '\t' << "call" << '\t' << label << '\n';
        mem::refed.insert(mem::refgen_t(label, usr::FUNCTION, 0));
      }
      else {
        out << '\t' << "push" << '\t';
        out << "OFFSET " << except::ms::label(except::ms::pre1, T) << '\n';
        address* y = getaddr(tac->y);
        y->load();
        out << '\t' << "push" << '\t' << "eax" << '\n';
        string label = "__CxxThrowException@8";
        out << '\t' << "call" << '\t' << label << '\n';
        mem::refed.insert(mem::refgen_t(label, usr::FUNCTION, 0));
      }
      except::throw_types.push_back(T);
    }
} // end of namespace intel

void intel::throwe(COMPILER::tac* tac)
{
  return mode == GNU ? gnu_throwe(tac) : ms_throwe(tac);
}

void intel::rethrow(COMPILER::tac* tac)
{
  if (mode == MS)
    return;
  string start = new_label((mode == GNU) ? ".label" : "label$");
  out << start << ":\n";
  out << '\t' << "call" << '\t' << "__cxa_rethrow" << '\n';
  string end = new_label((mode == GNU) ? ".label" : "label$");
  out << end << ":\n";

  except::call_site_t info;
  info.m_start = start;
  info.m_end = end;
  except::call_sites.push_back(info);
}

namespace intel {
  using namespace COMPILER;
  void gnu_try_begin(tac* tac)
  {
    string label = to_impl::label(tac);
    out << label << ":\n";
    except::call_site_t info;
    info.m_start = label;
    except::call_sites.push_back(info);
  }
  void ms_try_begin(tac* tac)
  {
    if (x64)
      return; // not implemented
    int n = except::ms::x86_handler::ehrec_off - 12;
    out << '\t' << "mov	DWORD PTR [ebp-" << n << "], 0" << '\n';
  }
} // end of namespace intel

void intel::try_begin(COMPILER::tac* tac)
{
  mode == GNU ? gnu_try_begin(tac) : ms_try_begin(tac);
}

namespace intel {
  using namespace COMPILER;
  void gnu_try_end(tac* tac)
  {
    string label = to_impl::label(tac);
    out << label << ":\n";
    assert(!except::call_sites.empty());
    except::call_site_t& info = except::call_sites.back();
    assert(!info.m_start.empty());
    assert(info.m_end.empty());
    info.m_end = label;
    assert(info.m_landing.empty());
  }
  void ms_try_end(tac* tac)
  {
    if (x64)
      out << '\t' << "npad	1" << '\n';
    else {
      int n = except::ms::x86_handler::ehrec_off - 12;
      out << '\t' << "mov	DWORD PTR [ebp-" << n << "], -1" << '\n';
    }
  }
} // end of namespace intel

void intel::try_end(COMPILER::tac* tac)
{
  mode == GNU ? gnu_try_end(tac) : ms_try_end(tac);
}

void intel::here(COMPILER::tac* tac)
{
    if (mode == MS)
        return;
    string label = to_impl::label(tac);
    out << label << ":\n";
    assert(!except::call_sites.empty());
    except::call_site_t& info = except::call_sites.back();
    assert(!info.m_start.empty());
    assert(!info.m_end.empty());
    assert(info.m_landing.empty());
    info.m_landing = label;
    here3ac* p = static_cast<here3ac*>(tac);
    if (p->m_for_dest) {
        info.m_action = 0;
        info.m_for_dest = true;
    }
    else
        info.m_action = 1;
}

void intel::here_reason(COMPILER::tac* tac)
{
  if (mode == MS)
    return;
  address* x = getaddr(tac->x);
  x->store(reg::dx);
}

void intel::here_info(COMPILER::tac* tac)
{
  if (mode == MS)
    return;
  address* x = getaddr(tac->x);
  x->store(reg::ax);
}

void intel::there(COMPILER::tac* tac)
{
  if (mode == MS)
    return;
  string label = to_impl::label(tac);
  out << label << ":\n";
  assert(!except::call_sites.empty());
  except::call_site_t& info = except::call_sites.back();
  assert(!info.m_start.empty());
  assert(!info.m_end.empty());
  assert(info.m_landing.empty());
  info.m_landing = label;
  info.m_action = 0;
}

void intel::unwind_resume(COMPILER::tac* tac)
{
  if (mode == MS)
    return;
  address* y = getaddr(tac->y);
  if (x64)
    y->load(reg::cx);
  else {
    y->load();
    out << '\t' << "subl" << '\t' << "$12, %esp" << '\n';
    out << '\t' << "pushl" << '\t' << "%eax" << '\n';
  }
  string label = to_impl::label(tac);
  string start = label + ".start";
  out << start << ":\n";
  out << '\t' << "call" << '\t' << "_Unwind_Resume" << '\n';
  string end = label + ".end";
  out << end << ":\n";
  except::call_site_t info;
  info.m_start = start;
  info.m_end = end;
  except::call_sites.push_back(info);
}

namespace intel {
  using namespace COMPILER;
  void catch_common(tac* tac)
  {
    if (!tac->x) {
      except::call_site_t::types.push_back(0);
      return;
    }
    address* x = getaddr(tac->x);
    const type* T = tac->x->m_type;
    if (mode == GNU)
      T->scalar() ? x->store() : copy(x, 0, T->size());
    else {
      assert(x->m_id == address::STACK);
      stack* ps = static_cast<stack*>(x);
      int offset = ps->m_offset;
      except::call_site_t::offsets.push_back(offset);
    }
    T = T->unqualified();
    if (T->m_id == type::REFERENCE) {
      typedef const reference_type RT;
      RT* rt = static_cast<RT*>(T);
      T = rt->referenced_type();
    }
    except::call_site_t::types.push_back(T);
  }
  void gnu_catch_begin(tac* tac)
  {
    address* y = getaddr(tac->y);
    if (x64) {
      y->load(reg::cx);
      out << '\t' << "call" << '\t' << "__cxa_begin_catch" << '\n';
    }
    else {
      y->load();
      out << '\t' << "subl" << '\t' << "$12, %esp" << '\n';
      out << '\t' << "pushl" << '\t' << "%eax" << '\n';
      out << '\t' << "call" << '\t' << "__cxa_begin_catch" << '\n';
      out << '\t' << "addl" << '\t' << "$16, %esp" << '\n';
    }
    catch_common(tac);
  }
  namespace ms {
    void x64_catch_begin(tac* ptr)
    {
      if (except::ms::x64_handler::catch_code::flag)
        return; // not implemented nest case
      except::ms::x64_handler::catch_code::flag = true;
      if (except::ms::x64_handler::catch_code::ptr)
        return; // not implemented multiple case
      except::ms::x64_handler::catch_code::ptr = new vector<tac*>;
      catch_common(ptr);
    }
    void x86_catch_begin(tac* tac)
    {
      string label = except::ms::x86_handler::prefix2 + func_label;
      except::call_site_t info;
      info.m_landing = label;
      except::call_sites.push_back(info);
      out << label << '\t' << "EQU" << '\t' << '$' << '\n';
      catch_common(tac);
    }
  } // end of namespace ms
  void ms_catch_begin(tac* tac)
  {
    using namespace ms;
    x64 ? x64_catch_begin(tac) : x86_catch_begin(tac);
  }
} // end of namespace intel

void intel::catch_begin(COMPILER::tac* tac)
{
  mode == GNU ? gnu_catch_begin(tac) : ms_catch_begin(tac);
}

void intel::catch_end(COMPILER::tac* tac)
{
  if (mode == GNU) {
    out << '\t' << "call" << '\t' << "__cxa_end_catch" << '\n';
  }
  else {
    if (x64) {
      except::call_site_t info;
      string label = func_label + except::ms::x64_handler::postfix;
      info.m_landing = label;
      except::call_sites.push_back(info);
      out << label << '\t' << "EQU $" << '\n';
    }
  }
}
#endif // CXX_GENERATOR

#ifdef CXX_GENERATOR
namespace intel {
  std::string scope_name(COMPILER::scope*);
  std::string func_name(COMPILER::usr*);
} // end of namespace intel

std::string intel::cxx_label(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  usr::flag_t flag = u->m_flag;
  if (flag & usr::C_SYMBOL) {
    string name = u->m_name;
    if (name[0] == '.')
      name = name.substr(1);
    return name;
  }
  string a = scope_name(u->m_scope);
  string ret;
  if ( !a.empty() || (flag & usr::FUNCTION) ) {
    if (mode == GNU)
      ret = "_Z";
    else
      ret = "?";
  }
  if ( !a.empty() ){
    if (mode == GNU) {
      ostringstream os;
      os << "N" << a;
      ret += os.str();
    }
  }
  string b;
  if (flag & usr::FUNCTION)
    b = func_name(u);
  else {
    ostringstream os;
    b = u->m_name;
    if (b[0] == '.')
        b = b.substr(1);
    if ( !a.empty() ) {
      if (mode == GNU)
        os << b.length();
    }
    os << b;
    if (!a.empty()) {
      if (mode == GNU)
        os << 'E';
      else
        os << a << "@@3HA";
    }
    b = os.str();
  }
  ret += b;
  return ret;
}

std::string intel::scope_name(COMPILER::scope* p)
{
  using namespace std;
  using namespace COMPILER;
  if (p->m_id == scope::TAG) {
    tag* ptr = static_cast<tag*>(p);
    const type* T = ptr->m_types.second;
    if (!T)
      T = ptr->m_types.first;
    ostringstream os;
    if (mode == GNU) {
      T->encode(os);
      return scope_name(ptr->m_parent) + os.str();
    }
    else {
      os << '@' << signature(T);
      return os.str() + scope_name(ptr->m_parent);
    }
  }
  if (p->m_id == scope::NAMESPACE) {
    name_space* ptr = static_cast<name_space*>(p);
    scope* parent = ptr->m_parent;
    assert(parent);
    string name = ptr->m_name;
    if (name == "std" && !parent->m_parent)
      return "";
    ostringstream os;
    if (mode == GNU) {
      os << name.length() << name;
      return scope_name(parent) + os.str();
    }
    else {
      os << '@' << name;
      return os.str() + scope_name(parent);
    }
  }
  return "";
}

namespace intel {
  struct op_tbl_t : map<string, string> {
    op_tbl_t()
    {
      (*this)["new"] = "new";
      (*this)["delete"] = "delete";
      (*this)["new []"] = "new_array";
      (*this)["delete []"] = "delete_array";
      (*this)["+"] = "add";
      (*this)["-"] = "sub";
      (*this)["*"] = "mul";
      (*this)["/"] = "div";
      (*this)["%"] = "mod";
      (*this)["^"] = "xor";
      (*this)["&"] = "and";
      (*this)["|"] = "or";
      (*this)["|"] = "or";
      (*this)["~"] = "tilde";
      (*this)["!"] = "not";
      (*this)["="] = "assign";
      (*this)["<"] = "lt";
      (*this)[">"] = "gt";
      (*this)["+="] = "add_assign";
      (*this)["-="] = "sub_assign";
      (*this)["*="] = "mul_assign";
      (*this)["/="] = "div_assign";
      (*this)["%="] = "mod_assign";
      (*this)["^="] = "xor_assign";
      (*this)["&="] = "and_assign";
      (*this)["|="] = "or_assign";
      (*this)["<<"] = "lsh";
      (*this)[">>"] = "rsh";
      (*this)["<<="] = "lsh_assign";
      (*this)[">>="] = "rsh_assign";
      (*this)["=="] = "eq";
      (*this)["!="] = "ne";
      (*this)["<="] = "le";
      (*this)[">="] = "ge";
      (*this)["&&"] = "and_and";
      (*this)["||"] = "or_or";
      (*this)["++"] = "plus_plus";
      (*this)["--"] = "minus_minus";
      (*this)[","] = "comma";
      (*this)["->*"] = "arrow_aster";
      (*this)[".*"] = "dot_aster";
      (*this)["->"] = "arrow";
      (*this)["()"] = "func";
      (*this)["[]"] = "subscript";
    }
  } op_tbl;
} // end of namespace intel

std::string intel::func_name(COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  usr::flag_t flag = u->m_flag;
  if (flag & usr::NEW_SCALAR) {
    if (mode == GNU)
      return x64 ? "nwm" : "nwj";
    else
      return x64 ? "?2@YAPEAX_K@Z" : "?2@YAPAXI@Z";
  }
  if (flag & usr::NEW_ARRAY) {
    if (mode == GNU)
      return x64 ? "nam" : "naj";
    else
      return x64 ? "?_U@YAPEAX_K@Z" : "?_U@YAPAXI@Z";
  }
  if (flag & usr::DELETE_SCALAR) {
    if (mode == GNU)
      return "dlPv";
    else
      return x64 ? "?3@YAXPEAX_K@Z" : "?3@YAXPAXI@Z";
  }
  if (flag & usr::DELETE_ARRAY) {
    if (mode == GNU)
      return "daPv";
    else
      return x64 ? "?_V@YAXPEAX@Z" : "?_V@YAXPAX@Z";
  }

  usr::flag2_t flag2 = u->m_flag2;
  usr::flag2_t mask2 = usr::flag2_t(usr::TOR_BODY | usr::EXCLUDE_TOR);
  string s = u->m_name;
  ostringstream os;
  if ((flag & usr::CTOR) && !(flag2 & mask2)) {
    if (mode == GNU)
      os << "C1";
    else
      os << "?0" << s;
  }
  else if ((flag & usr::DTOR) && !(flag2 & mask2)) {
    if (mode == GNU)
      os << "D1";
    else {
      assert(s[0] == '~');
      os << "?1" << s.substr(1);
    }
  }
  else if (flag2 & usr::CONV_OPE) {
    const type* T = u->m_type;
    assert(T->m_id == type::FUNC);
    typedef const func_type FT;
    FT* ft = static_cast<FT*>(T);
    T = ft->return_type();
    os << "cv";
    T->encode(os);
  }
  else if (flag2 & usr::OPERATOR) {
    op_tbl_t::const_iterator p = op_tbl.find(s);
    assert(p != op_tbl.end());
    if (mode == GNU)
      os << '.' << p->second << '.';
    else
      os << '?' << p->second << '?';
  }
  else {
    scope* p = u->m_scope;
    if (p->m_id == scope::NAMESPACE) {
      name_space* ptr = static_cast<name_space*>(p);
      scope* parent = ptr->m_parent;
      string name = ptr->m_name;
      if (name == "std" && !parent->m_parent)
        os << "St";
    }
    if (mode == GNU)
      os << s.length() << s;
    else {
      if (flag2 & usr::INITIALIZE_FUNCTION) {
        string t = "initialize.";
        assert(s.substr(0, t.length()) == t);
        s = "?__E" + s.substr(t.length());
      }
      if (flag2 & usr::TERMINATE_FUNCTION) {
        string t = "terminate.";
        assert(s.substr(0, t.length()) == t);
        s = "?__D" + s.substr(t.length());
      }
      if ((flag & usr::VDEL) || (flag2 & usr::TOR_BODY) || (flag2 & usr::EXCLUDE_TOR)) {
        string::size_type p = s.find_first_of('.');
        while (p != string::npos) {
          s[p] = '?';
          p = s.find_first_of('.', p);
        }
      }
      os << s << "@@YA";
    }
  }
  if (flag2 & usr::EXPLICIT_INSTANTIATE) {
    typedef instantiated_usr IU;
    IU* iu = static_cast<IU*>(u);
    os << 'I';
    const IU::SEED& seed = iu->m_seed;
    for (auto p : seed) {
      if (const type* T = p.first)
        T->encode(os);
      else {
        var* v = p.second;
        assert(v->usr_cast());
        usr* u = static_cast<usr*>(v);
        os << u->m_name;
      }
    }
    os << "EPT_";
  }
  string tmp = scope_name(u->m_scope);
  if (!tmp.empty()) {
    if (mode == GNU)
      os << 'E';
    else
      os << tmp << "@@";
  }
  os << signature(u->m_type);
  return os.str();
}

namespace intel {
  namespace ms_signature {
    using namespace std;
    using namespace COMPILER;
    string encode_void(const type*) { return "X"; }
    string encode_char(const type*) { return "D"; }
    string encode_schar(const type*) { return "C"; }
    string encode_uchar(const type*) { return "E"; }
    string encode_bool(const type*) { return "_N"; }
    string encode_short(const type*) { return "F"; }
    string encode_ushort(const type*) { return "G"; }
    string encode_wchar(const type*) { return "_W"; }
    string encode_int(const type*) { return "H"; }
    string encode_uint(const type*) { return "AI"; }
    string encode_long(const type*) { return "J"; }
    string encode_ulong(const type*) { return "K"; }
    string encode_longlong(const type*) { return "_J"; }
    string encode_ulonglong(const type*) { return "_K"; }
    string encode_float(const type*) { return "M"; }
    string encode_double(const type*) { return "N"; }
    string encode_longdouble(const type*) { return "OX"; }
    string helper(const scope::tps_t::val2_t& x)
    {
      if (const type* T = x.first) {
        T = T->unqualified();
        return '@' + signature(T);
      }
      var* v = x.second;
      if (addrof* a = v->addrof_cast())
        v = a->m_ref;
      assert(v->usr_cast());
      usr* u = static_cast<usr*>(v);
      return u->m_name;
    }
    tag* get_src(const tag* ptr)
    {
      tag::flag_t flag = ptr->m_flag;
      if (flag & tag::INSTANTIATE) {
        typedef const instantiated_tag IT;
        IT* it = static_cast<IT*>(ptr);
        return reinterpret_cast<tag*>(it->m_src);
      }
      if (flag & tag::SPECIAL_VER) {
        typedef const special_ver_tag SV;
        SV* sv = static_cast<SV*>(ptr);
        return reinterpret_cast<tag*>(sv->m_src);
      }
      return 0;
    }
    string encode_tag(const tag* ptr)
    {
      ostringstream os;
      os << '?' << 'A';
      tag::kind_t kind = ptr->m_kind;
      switch (kind) {
      case tag::ENUM:   os << "W4"; break;
      case tag::STRUCT: os << 'U'; break;
      case tag::UNION: os << 'T'; break;
      default: assert(kind == tag::CLASS);  os << 'V'; break;
      }
      string name = ptr->m_name;
      if (name[0] == '.')
        name = name.substr(1) + '$';
      if (tag* src = get_src(ptr)) {
        name = src->m_name;
        if (src->m_flag & tag::PARTIAL_SPECIAL) {
          string::size_type p = name.find_first_of('<');
          name.erase(p);
        }
        os << "?$" << name;
        const instantiated_tag::SEED* seed = get_seed(ptr);
        typedef instantiated_tag::SEED::const_iterator IT;
        transform(begin(*seed), end(*seed),
        	  ostream_iterator<string>(os), helper);
      }
      else
        os << name;
      os << "@@";
      return os.str();
    }
    string encode_enum(const type* T)
    {
      return encode_tag(T->get_tag());
    }
    string encode_record(const type* T)
    {
      return encode_tag(T->get_tag());
    }
    string encode_incomplete(const type* T)
    {
      return encode_tag(T->get_tag());
    }
    string encode_func(const type* T)
    {
      ostringstream os;
      assert(T->m_id == type::FUNC);
      typedef const func_type FT;
      FT* ft = static_cast<FT*>(T);
      const type* R = ft->return_type();
      if (R)
        os << signature(R->unqualified());
      const vector<const type*>& param = ft->param();
      for (auto T : param)
        os << signature(T->unqualified());
      assert(!param.empty());
      const type* B = param.back();
      type::id_t id = B->m_id;
      switch (id) {
      case type::VOID: break;
      case type::ELLIPSIS: os << 'Z'; break;
      default: os << '@'; break;
      }
      os << 'Z';
      return os.str();
    }
    string encode_ptr_ref(const type* R)
    {
      ostringstream os;
      if (R->m_id == type::FUNC)
        os << '6';
      else {
        if (x64)
          os << 'E';
      }
      int cvr = 0;
      R = R->unqualified(&cvr);
      if (except::ms::label_flag) {
        os << 'A';
      }
      else {
        switch (cvr) {
        case 0: os << 'A'; break;
        case 1: os << 'B'; break;
        case 2: os << 'C'; break;
        default: os << 'D'; break;
        }
      }
      os << signature(R);
      return os.str();
    }
    string encode_pointer(const type* T)
    {
      assert(T->m_id == type::POINTER);
      typedef const pointer_type PT;
      PT* pt = static_cast<PT*>(T);
      const type* R = pt->referenced_type();
      return 'P' + encode_ptr_ref(R);
    }
    string encode_reference(const type* T)
    {
      assert(T->m_id == type::REFERENCE);
      typedef const reference_type RT;
      RT* rt = static_cast<RT*>(T);
      const type* R = rt->referenced_type();
      return 'A' + encode_ptr_ref(R);
    }
    string encode_array(const type* T)
    {
      assert(T->m_id == type::ARRAY);
      typedef const array_type AT;
      AT* at = static_cast<AT*>(T);
      int dim = at->dim();
      ostringstream os;
      os << "Y0";
      if (dim)
        os << dim - 1;
      else
        os << "A@";
      const type* E = at->element_type();
      int cvr = 0;
      E = E->unqualified(&cvr);
      if (except::ms::label_flag) {
        os << 'A';
      }
      else {
        switch (cvr) {
        case 0: os << 'A'; break;
        case 1: os << 'B'; break;
        case 2: os << 'C'; break;
        default: os << 'D'; break;
        }
      }
      os << signature(E);
      return os.str();
    }
    string encode_varray(const type* T)
    {
      assert(T->m_id == type::VARRAY);
      typedef const varray_type VT;
      VT* vt = static_cast<VT*>(T);
      ostringstream os;
      os << "Y0";
      os << "A@";
      const type* E = vt->element_type();
      int cvr = 0;
      E = E->unqualified(&cvr);
      if (except::ms::label_flag) {
        os << 'A';
      }
      else {
        switch (cvr) {
        case 0: os << 'A'; break;
        case 1: os << 'B'; break;
        case 2: os << 'C'; break;
        default: os << 'D'; break;
        }
      }
      os << signature(E);
      return os.str();
    }
    string encode_elllipsis(const type*)
    {
      return "";
    }
    string encode_pm(const type* T)
    {
      assert(T->m_id == type::POINTER_MEMBER);
      typedef const pointer_member_type PM;
      PM* pm = static_cast<PM*>(T);
      const tag* ptr = pm->ctag();
      string name = ptr->m_name;
      ostringstream os;
      os << "PEQ";
      os << name;
      os << "@@";
      const type* R = pm->referenced_type();
      os << signature(R);
      return os.str();
    }
    struct table_t : map<type::id_t, string(*)(const type*)> {
      table_t();
    } table;
    table_t::table_t()
    {
      (*this)[type::VOID] = encode_void;
      (*this)[type::CHAR] = encode_char;
      (*this)[type::SCHAR] = encode_schar;
      (*this)[type::UCHAR] = encode_uchar;
      (*this)[type::BOOL] = encode_bool;
      (*this)[type::SHORT] = encode_short;
      (*this)[type::USHORT] = encode_ushort;
      (*this)[type::WCHAR] = encode_wchar;
      (*this)[type::INT] = encode_int;
      (*this)[type::UINT] = encode_uint;
      (*this)[type::LONG] = encode_long;
      (*this)[type::ULONG] = encode_ulong;
      (*this)[type::LONGLONG] = encode_longlong;
      (*this)[type::ULONGLONG] = encode_ulonglong;
      (*this)[type::FLOAT] = encode_float;
      (*this)[type::DOUBLE] = encode_double;
      (*this)[type::LONG_DOUBLE] = encode_longdouble;
      (*this)[type::ENUM] = encode_enum;
      (*this)[type::RECORD] = encode_record;
      (*this)[type::INCOMPLETE_TAGGED] = encode_incomplete;
      (*this)[type::FUNC] = encode_func;
      (*this)[type::POINTER] = encode_pointer;
      (*this)[type::REFERENCE] = encode_reference;
      (*this)[type::ARRAY] = encode_array;
      (*this)[type::VARRAY] = encode_varray;
      (*this)[type::ELLIPSIS] = encode_elllipsis;
      (*this)[type::POINTER_MEMBER] = encode_pm;
    }
  } // end of namespace ms_signature
} // end of namespace intel

std::string intel::signature(const COMPILER::type* T)
{
  using namespace std;
  using namespace COMPILER;
  if (mode == MS) {
    using namespace ms_signature;
    table_t::const_iterator p = table.find(T->m_id);
    assert(p != table.end());
    return (p->second)(T);
  }
  ostringstream os;
  assert(T->m_id == type::FUNC);
  typedef const func_type FT;
  FT* ft = static_cast<FT*>(T);
  const vector<const type*>& param = ft->param();
  for (auto T : param)
    T->encode(os);
  return os.str();
}
#endif // CXX_GENERATOR
