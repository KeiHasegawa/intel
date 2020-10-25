#ifdef CXX_GENERATOR
#include "stdafx.h"
#include "cxx_core.h"
#include "intel.h"

namespace intel {
  namespace except {
    using namespace std;
    using namespace COMPILER;
    vector<call_site_t> call_sites;
    vector<const type*> types;
    vector<const type*> call_site_t::types;
    vector<int> call_site_t::offsets;
    vector<const type*> throw_types;
    set<const type*> types_to_output;
    set<const type*> ms::call_sites_types_to_output;
#if defined(_MSC_VER) || defined(__CYGWIN__)
    // nothing to be defined
#else // defined(_MSC_VER) || defined(__CYGWIN__)
    vector<frame_desc_t> fds;
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
    int conv(int x)
    {
      if (!x)
        return 0;
      assert(x == 1);
      const vector<const type*>& v = call_site_t::types;
      int n = v.size();
      return (n << 1) - 1; 
    }
    void out_call_site(const call_site_t& info)
    {
      string start = info.m_start;
      string end = info.m_end;
      string landing = info.m_landing;

      // region start
      out << '\t' << ".uleb128 " << start << '-' << func_label << '\n';

      // length
      out << '\t' << ".uleb128 " << end << '-' << start << '\n';

       // landing pad
      if (landing.empty())
        out << '\t' << ".uleb128 0" << '\n';
      else
        out << '\t' << ".uleb128 " << landing << '-' << func_label << '\n';

      out << '\t' << ".uleb128 " << conv(info.m_action) << '\n'; // action
    }
    void out_call_sites()
    {
      out << '\t' << ".byte	0x1" << '\n'; // Call-site format
      string start = ".LLSDACSB." + func_label;
      string end   = ".LLSDACSE." + func_label;

      // Call-site table length
      out << '\t' << ".uleb128 " << end << '-' << start << '\n';

      out << start << ':' << '\n';
      for (const auto& info : call_sites)
        out_call_site(info);
      out << end << ':' << '\n';
    }
    void out_action_records()
    {
      const vector<const type*>& v = call_site_t::types;
      int n = v.size();
      if (n <= 1) {
        out << '\t' << ".byte " << 1 << '\n';
        out << '\t' << ".byte " << 0 << '\n';
        return;
      }
      out << '\t' << ".byte " << n << '\n';
      out << '\t' << ".byte " << 0 << '\n';
      while (--n) {
        out << '\t' << ".byte	" << n << '\n';
        out << '\t' << ".byte	0x7d" << '\n';
      }
    }
    string gnu_label(const type* T, char c)
    {
      assert(mode == GNU);
      ostringstream os;
      os << "_ZT" << c;
      T->encode(os);
      return os.str();
    }
    bool ms::label_flag;
    std::string ms::pre1a = "_TI1";
    std::string ms::pre1b = "_TI2C";
    std::string ms::pre2a = "_CTA1";
    std::string ms::pre2b = "_CTA2";
    std::string ms::pre3 = "_CT??_R0";
    std::string ms::pre4 = "??_R0";
    std::string ms::pre5 = ".";
    std::string ms::vpsig = "PEAX";
    string ms::label(string pre, const type* T)
    {
      assert(mode == MS);
      ms::label_flag = true;
      string r = pre + signature(T);
      ms::label_flag = false;
      return r;
    }
    void out_type(const type* T)
    {
      if (x64)
        out << '\t' << ".quad	";
      else
        out << '\t' << ".long	";
      if (T)
        out << gnu_label(T, 'I');
      else
        out << 0;
      out << '\n';
    }
#if defined(_MSC_VER) || defined(__CYGWIN__)
    vector<pair<string, const type*> > labeled_types;
    void out_type_label(const type* T)
    {
      string L = new_label((mode == GNU) ? ".LDFCM" : "LDFCM$");
      out << '\t' << ".long	" << L << "-." << '\n';
      labeled_types.push_back(make_pair(L, T));
    }
    void out_labeled_type(const pair<string, const type*>& p)
    {
      string L = p.first;
      out << L << ':' << '\n';
      const type* T = p.second;
      out_type(T);
    }
    void out_labeled_types()
    {
      vector<pair<string, const type*> >& v = labeled_types;
      if (v.empty())
        return;
      sec_hlp sentry(RAM);
      out << '\t' << ".align 8" << '\n';
      for_each(begin(v), end(v), out_labeled_type);
      v.clear();
    }
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)    
    void out_lsda(bool for_dest)
    {
      if (for_dest) {
        out_call_sites();
        assert(call_site_t::types.empty());
        return;
      }

      string start = ".LLSDATTD." + func_label;
      string end   = ".LLSDATT."  + func_label;

      // LSDA length
      out << '\t' << ".uleb128 " << end << '-' << start << '\n';
      out << start << ':' << '\n';
      out_call_sites();
      out_action_records();
      out << '\t' << ".align 4" << '\n';
      const vector<const type*>& v = call_site_t::types;
#if defined(_MSC_VER) || defined(__CYGWIN__)
      for_each(rbegin(v), rend(v), out_type_label);
#else // defined(_MSC_VER) || defined(__CYGWIN__)
      for_each(rbegin(v), rend(v), out_type);
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
      out << end << ':' << '\n';
    }
    namespace out_type_impl {
      void gnu_none_tag(const type* T)
      {
        if (x64) {
          string L1 = gnu_label(T, 'I');
          mem::refgen_t obj(L1, usr::NONE, 8);
          out << mem::refgen(obj) << '\n';
        }
      }
      void ms_common(const type* T, tag* ptr)
      {
        string s1 = ptr ? ms::pre1a : ms::pre1b;
        string La = ms::label(s1, T);
        string s2 = ptr ? ms::pre2a : ms::pre2b;
        string Lb = ms::label(s2, T);
        string pre = x64 ? "imagerel\t" : "";

        out << "xdata$x" << '\t' << "SEGMENT" << '\n';
        out << '\t' << "ALIGN 4" << '\n';
        out << La;
        out << '\t' << "DD" << '\t';
        out << (ptr ? "00H" : "01H") << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << '\t' << "DD" << '\t' << pre << Lb << '\n';
        out << "xdata$x" << '\t' << "ENDS" << '\n';

        out << "xdata$x" << '\t' << "SEGMENT" << '\n';
        out << '\t' << "ALIGN 4" << '\n';
        out << Lb;
        string Lc = ms::label(ms::pre3, T);
        string Ld;
        if (T->m_id == type::POINTER) {
          Ld = ms::pre3 + ms::vpsig;
          out << '\t' << "DD" << '\t' << "02H" << '\n';
          out << '\t' << "DD" << '\t' << pre << Lc << '\n';
          out << '\t' << "DD" << '\t' << pre << Ld << '\n';
        }
        else {
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << '\t' << "DD" << '\t' << pre << Lc << '\n';
        }
        out << "xdata$x" << '\t' << "ENDS" << '\n';

        string Le = ms::label(ms::pre4, T);
        out << "xdata$x" << '\t' << "SEGMENT" << '\n';
        out << '\t' << "ALIGN 4" << '\n';
        out << Lc;
        out << '\t' << "DD" << '\t';
        out << (ptr ? "00H" : "01H") << '\n';
        out << '\t' << "DD" << '\t' << pre << Le << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
        out << '\t' << "ORG $ + 4" << '\n';
        int sz = T->size();
        out << '\t' << "DD" << '\t' << '0' << sz << 'H' << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << "xdata$x" << '\t' << "ENDS" << '\n';

        string Lf;
        if (!Ld.empty()) {
          Lf = ms::pre4 + ms::vpsig;
          out << "xdata$x" << '\t' << "SEGMENT" << '\n';
          out << '\t' << "ALIGN 4" << '\n';
          out << Ld;
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << '\t' << "DD" << '\t' << pre << Lf << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          out << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
          out << '\t' << "ORG $ + 4" << '\n';
          out << '\t' << "DD" << '\t' << '0' << sz << 'H' << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          out << "xdata$x" << '\t' << "ENDS" << '\n';
        }

        ms::out_call_site_type_info(T);

        if (!Lf.empty()) {
          out << "data$r" << '\t' << "SEGMENT" << '\n';
          out << Lf;
          string d = ms_pseudo(psize());
          out << '\t' << d << '\t' << ms::ti_label << '\n';
          out << '\t' << d << '\t' << "00H" << '\n';
          out << '\t' << "DB" << '\t';
          out << "'" << ms::pre5 << ms::vpsig << "', 00H" << '\n';
          out << "data$r" << '\t' << "ENDS" << '\n';
        }

        ms::call_sites_types_to_output.erase(T);
      }
      void none_tag(const type* T)
      {
        mode == GNU ? gnu_none_tag(T) : ms_common(T, 0);
      }
      void gnu_tagged(const type* T, tag* ptr)
      {
        string L1 = gnu_label(T, 'I');
#if defined(_MSC_VER) || defined(__CYGWIN__)      
        out << '\t' << ".globl	" << L1 << '\n';
        out << '\t' << ".section	.rdata$" << L1 << ",\"dr\"" << '\n';
        out << '\t' << ".linkonce same_size" << '\n';
        out << '\t' << ".align 8" << '\n';
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
        out << '\t' << ".weak	" << L1 << '\n';
        out << '\t' << ".section	.rodata." << L1 << ",\"aG\",@progbits,";
        out << L1 << ",comdat" << '\n';
        out << '\t' << ".align 4" << '\n';
        out << '\t' << ".type	" << L1 << ", @object" << '\n';
        out << '\t' << ".size	" << L1 << ", 8" << '\n';
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
        out << L1 << ':' << '\n';      
        if (x64)
          out << '\t' << ".quad	";
        else
          out << '\t' << ".long	";
        vector<base*>* bases = ptr->m_bases;
        if (!bases)
          out << "_ZTVN10__cxxabiv117__class_type_infoE+";
        else
          out << "_ZTVN10__cxxabiv120__si_class_type_infoE+";
        out << (x64 ? 16 : 8) << '\n';
        string L2 = gnu_label(T, 'S');
        if (x64)
          out << '\t' << ".quad	";
        else
          out << '\t' << ".long	";
        out << L2 << '\n';
        if (bases) {
          for (auto b : *bases) {
            tag* bptr = b->m_tag;
            const type* bT = bptr->m_types.second;
            assert(bT);
            string bL = gnu_label(bT, 'I');
            if (x64)
              out << '\t' << ".quad	";
            else
              out << '\t' << ".long	";
            out << bL << '\n';
          }
        }

        ostringstream os;
        T->encode(os);
        string s = os.str();
#if defined(_MSC_VER) || defined(__CYGWIN__)
        out << '\t' << ".globl	" << L2 << '\n';
        out << '\t' << ".section	.rdata$" << L2 << ",\"dr\"" << '\n';
        out << '\t' << ".linkonce same_size" << '\n';
#else  // defined(_MSC_VER) || defined(__CYGWIN__)
        out << '\t' << ".weak	" << L2 << '\n';
        out << '\t' << ".section	.rodata." << L2 << ",\"aG\",@progbits,";
        out << L2 << ",comdat" << '\n';
        out << '\t' << ".align 4" << '\n';
        out << '\t' << ".type	" << L2 << ", @object" << '\n';
        out << '\t' << ".size	" << L2 << ", " << s.length()+1 << '\n';
#endif  // defined(_MSC_VER) || defined(__CYGWIN__)
        out << L2 << ':' << '\n';
        out << '\t' << ".string	" << '"' << s << '"' << '\n';
      }
      void tagged(const type* T, tag* ptr)
      {
        mode == GNU ? gnu_tagged(T, ptr) : ms_common(T, ptr);
      }
    } // end of namespace out_type_impl
    void out_type_info(const type* T)
    {
      if (!T)
        return;
      tag* ptr = T->get_tag();
      ptr ? out_type_impl::tagged(T, ptr) : out_type_impl::none_tag(T);
    }
    namespace ms {
      namespace out_call_site_type_impl {
        void ms_common(const type* T)
        {
          mem::refed.insert(mem::refgen_t(ms::ti_label, usr::NONE, 8));
          out << "data$r" << '\t' << "SEGMENT" << '\n';
          string Le = ms::label(ms::pre4, T);
          out << Le;
          string d = ms_pseudo(psize());
          out << '\t' << d << '\t' << ms::ti_label << '\n';
          out << '\t' << d << '\t' << "00H" << '\n';
          out << '\t' << "DB" << '\t';
          out << "'" << ms::label(ms::pre5, T) << "', 00H" << '\n';
          out << "data$r" << '\t' << "ENDS" << '\n';
        }
      } // end of namespace out_call_site_type_impl
      void out_call_site_type_info(const type* T)
      {
        assert(mode == MS);
        if (T)
            out_call_site_type_impl::ms_common(T);
      }
    } // end of namespace ms
    bool gnu_table_outputed;
    void gnu_out_table()
    {
      if (call_sites.empty()) {
        assert(call_site_t::types.empty());
        return;
      }
      output_section(EXCEPT_TABLE);
      assert(mode == GNU);
      out << '\t' << ".align 4" << '\n';
      string label = ".LLSDA." + func_label;
      out << label << ':' << '\n';
#if defined(_MSC_VER) || defined(__CYGWIN__)
      // nothing to be done
#else // defined(_MSC_VER) || defined(__CYGWIN__)
      assert(!except::fds.empty());
      except::frame_desc_t& fd = except::fds.back();
      fd.m_lsda = label;
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
      typedef vector<call_site_t>::const_iterator IT;
      IT p = find_if(begin(call_sites), end(call_sites),
                     [](const call_site_t& info){ return info.m_for_dest; });
      bool for_dest = (p != end(call_sites));
      out << '\t' << ".byte	0xff" << '\n'; // LDSA header
      out << '\t' << ".byte	";
      if (!for_dest) {
#if defined(_MSC_VER) || defined(__CYGWIN__)
        out << "0x9b";
#else // defined(_MSC_VER) || defined(__CYGWIN__)
        out << 0;
#endif	// defined(_MSC_VER) || defined(__CYGWIN__)
      }
      else
        out << "0xff";
      out << '\n'; // Type Format
      out_lsda(for_dest);
      call_sites.clear();
      end_section(EXCEPT_TABLE);
      for (auto T : call_site_t::types) {
        if (T) {
          if (T->tmp())
            out_type_info(T);
          else
            types_to_output.insert(T);
        }
      }
      call_site_t::types.clear();
      gnu_table_outputed = true;
    }
    namespace ms {
      namespace out_table {
        using namespace std;
        namespace x86_gen {
          const string pre1 = "__catchsym$";
          const string pre2 = "__unwindtable$";
          const string pre3 = "__tryblocktable$";
          void action()
          {
            out << "xdata$x" << '\t' << "SEGMENT" << '\n';
            out << '\t' << "ALIGN 4" << '\n';
            string label1;
            if (!call_site_t::types.empty()) {
              label1 = out_table::x86_gen::pre1 + func_label;
              out << label1 << '\t' << "DD" << '\t';
              if (call_site_t::types.size() != 1)
                return;  // not implemented
              const type* T = call_site_t::types.back();
              if (T) {
                tag* ptr = T->get_tag();
                out << (ptr ? "08H" : "01H") << '\n';
                string Le = ms::label(ms::pre4, T);
                out << '\t' << "DD" << '\t' << Le << '\n';
              }
              else {
                out << "040H" << '\n';
                out << '\t' << "DD" << '\t' << "00H" << '\n';
              }

              if (call_site_t::offsets.size() != 1)
                return;  // not implemented
              int offset = call_site_t::offsets.back();
              out << '\t' << "DD" << '\t' << offset << '\n';
              if (call_sites.size() != 1)
                return;  // not implemented
              const call_site_t& info = call_sites.back();
              string landing = info.m_landing;
              assert(!landing.empty());
              out << '\t' << "DD" << '\t' << landing << '\n';
            }
            string label2 = out_table::x86_gen::pre2 + func_label;
            out << label2 << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
            string label3;
            if (!call_site_t::types.empty()) {
              out << '\t' << "DD" << '\t' << "00H" << '\n';
              out << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
              out << '\t' << "DD" << '\t' << "00H" << '\n';
              label3 = out_table::x86_gen::pre3 + func_label;
              out << label3 << '\t' << "DD" << '\t' << "00H" << '\n';
              out << '\t' << "DD" << '\t' << "00H" << '\n';
              out << '\t' << "DD" << '\t' << "01H" << '\n';
              out << '\t' << "DD" << '\t' << "01H" << '\n';
              out << '\t' << "DD" << '\t' << label1 << '\n';
            }
            else {
              string label5 = x86_gen::pre5 + func_label;
              out << '\t' << "DD" << '\t' << label5 << '\n';
            }
            string label4 = out_table::x86_gen::pre4 + func_label;
            out << label4 << '\t' << "DD" << '\t' << "019930522H" << '\n';
            if (!call_site_t::types.empty()) {
              out << '\t' << "DD" << '\t' << "02H" << '\n';
              out << '\t' << "DD" << '\t' << label2 << '\n';
              out << '\t' << "DD" << '\t' << "01H" << '\n';
              out << '\t' << "DD" << '\t' << label3 << '\n';
            }
            else {
              out << '\t' << "DD" << '\t' << "01H" << '\n';
              out << '\t' << "DD" << '\t' << label2 << '\n';
              out << '\t' << "DD" << '\t' << "2 DUP(00H)" << '\n';
            }
            out << '\t' << "DD" << '\t' << "2 DUP(00H)" << '\n';
            out << '\t' << "DD" << '\t' << "00H" << '\n';
            out << '\t' << "DD" << '\t' << "01H" << '\n';
            out << "xdata$x" << '\t' << "ENDS" << '\n';
          }
        } // end of namespace x86_gen
        namespace x64_gen {
          const string pre1 = "$ip2state$";
          const string pre2 = "$handlerMap$";
          const string pre3 = "$tryMap$";
          const string pre4 = "$stateUnwindMap$";
          const string pre5 = "$cppxdata$";
          void unwind_data(bool ms_handler)
          {
            out << "xdata	SEGMENT	READONLY ALIGN(4) ALIAS(\".xdata\")" << '\n';
            out << '\t' << "ALIGN 4" << '\n';
            string unwind = "$unwind$" + func_label;

            if (ms_handler) {
              // push	rbp
              // .pushreg rbp
              // push	rdi
              // .pushreg rdi
              // mov rbp, rsp
              // .setframe rbp, 0
              // sub	rsp, xx
              // .allocstack xx
              out << unwind << '\t' << "DB 019H" << '\n'; // version : 1,
              // flag : UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER
              string s = func_label + except::ms::x64_handler::prolog_size;
              out << '\t' << "DB " << s << '\n'; // prolog size
              out << '\t' << "DB 05H" << '\n'; // Count of unwind codes
              out << '\t' << "DB 05H" << '\n';  // frame register : rbp, offset 0
              out << '\t' << "WORD ";
              out << "0100H OR " << s << '\n';  // .allocstack xx (UWOP_ALLOC_LARGE = 1)
              int n = stack::delta_sp >> 3;
              out << '\t' << "WORD " << n << '\n';
              out << '\t' << "WORD 05305H" << '\n'; // .setframe rbp, 0
              out << '\t' << "WORD 07002H" << '\n'; // .pushreg rdi
              out << '\t' << "WORD 05001H" << '\n'; // .pushreg rbp
              out << '\t' << "WORD 0H" << '\n';
              string label = "__CxxFrameHandler4";
              mem::refed.insert(mem::refgen_t(label, usr::FUNCTION, 0));
              out << '\t' << "DD imagerel " << label << '\n';
              out << '\t' << "DD imagerel " << pre5 << func_label << '\n';
            }
            else {
              // push rbp
              // .pushreg rbp
              // mov rbp, rsp
              // .setframe rbp, 0
              // push rbp
              // sub rsp, xx
              // .allocstack xx+8
              // .endprolog
              out << unwind << '\t' << "DB 01H" << '\n'; // version : 1, flag : 0
              string s = func_label + except::ms::x64_handler::prolog_size;
              out << '\t' << "DB " << s << '\n'; // prolog size
              out << '\t' << "DB 04H" << '\n'; // Count of unwind codes
              out << '\t' << "DB 05H" << '\n'; // frame register : rbp, offset 0
              out << '\t' << "WORD" << ' ';
              out << "0100H OR " << s << '\n'; // .allocstack xx+8 (UWOP_ALLOC_LARGE = 1)
              int n = (stack::delta_sp+8) >> 3;
              out << '\t' << "WORD " << n << '\n';
              out << '\t' << "WORD" << ' ' << "05304H" << '\n'; // .setframe rbp, 0
              out << '\t' << "WORD" << ' ' << "05001H" << '\n'; // .pushreg rbp
            }

            out << "xdata	ENDS" << '\n';
            out << "pdata	SEGMENT	READONLY ALIGN(4) ALIAS(\".pdata\")" << '\n';
            out << '\t' << "DD imagerel " << func_label << '\n';
            out << '\t' << "DD imagerel " << func_label << "$end" << '\n';
            out << '\t' << "DD imagerel " << unwind << '\n';
            out << "pdata	ENDS" << '\n';
          }
          void action()
          {
            out << "xdata	SEGMENT" << '\n';
            out << '\t' << "ALIGN 4" << '\n';
            string label1 = x64_gen::pre1 + func_label;
            out << label1 << " DB 06H" << '\n';
            out << '\t' << "DB 00H" << '\n';
            out << '\t' << "DB 00H" << '\n';
            out << '\t' << "DB 'V'" << '\n';
            out << '\t' << "DB 02H" << '\n';
            string s = except::ms::x64_handler::try_size_pre + func_label;
            out << '\t' << "DB " << s << '\n';
            out << '\t' << "DB 00H" << '\n';
            out << "xdata	ENDS" << '\n';

            if (call_sites.empty()) {
              out << "xdata	SEGMENT" << '\n';
              out << '\t' << "ALIGN 4" << '\n';
              string label4 = x64_gen::pre4 + func_label;
              out << label4 << " DB 02H" << '\n';
              out << '\t' << "DB 0eH" << '\n';
              string dlabel = x64_handler::catch_code::pre2 + func_label;
              dlabel += x64_handler::catch_code::post;
              out << '\t' << "DD imagerel " << dlabel << '\n';
              out << "xdata	ENDS" << '\n';

              out << "xdata	SEGMENT" << '\n';
              out << '\t' << "ALIGN 4" << '\n';
              out << except::ms::out_table::x64_gen::pre5;
              out << func_label << ' ';
              out << "DB 028H" << '\n';
              out << '\t' << "DD imagerel " << label4 << '\n';
              out << '\t' << "DD imagerel " << label1 << '\n';
              out << "xdata	ENDS" << '\n';

              out << "CONST	SEGMENT" << '\n';
              string label5 = func_label + "$rtcName$";
              out << label5 << ' ';
              out << "DB 073H" << '\n';
              out << '\t' << "DB 00H" << '\n';
              out << '\t' << "ORG $+14" << '\n';
              string label6 = func_label + "$rtcVarDesc";
              out << label6 << ' ' << "DD 024H" << '\n';
              out << '\t' << "DD 01H" << '\n';
              out << '\t' << "DQ " << label5 << '\n';
              out << '\t' << "ORG $+48" << '\n';
              string label7 = func_label + "$rtcFrameData";
              out << label7 << ' ' << "DD 01H" << '\n';
              out << '\t' << "DD 00H" << '\n';
              out << '\t' << "DQ " << label6 << '\n';
              out << "CONST	ENDS" << '\n';
            }
            else {
              out << "xdata	SEGMENT" << '\n';
              out << '\t' << "ALIGN 4" << '\n';
              string label2 = x64_gen::pre2 + func_label;
              out << label2 << " DB 02H" << '\n';
              if (call_site_t::types.size() != 1)
                return;  // not implemented
              const type* T = call_site_t::types.back();
              if (T) {
                out << '\t' << "DB 07H" << '\n';
                out << '\t' << "DB ";
                tag* ptr = T->get_tag();
                out << (ptr ? "010H" : "02H") << '\n';
                string Le = ms::label(ms::pre4, T);
                out << '\t' << "DD imagerel " << Le << '\n';
              }
              else {
                out << '\t' << "DB 01H" << '\n';
              }
              int n = stack::delta_sp - except::ms::x64_handler::magic;
              assert(n > 0);
              n -= 0x20;
              assert(n >= 0);
              assert(!(n & 7));
              n <<= 1;
              out << '\t' << "DB " << n << '\n';
              out << '\t' << "DD imagerel ";
              out << x64_handler::catch_code::pre;
              out << func_label;
              out << x64_handler::catch_code::post << '\n';
              out << "xdata	ENDS" << '\n';

              out << "xdata	SEGMENT" << '\n';
              out << '\t' << "ALIGN 4" << '\n';
              string label3 = x64_gen::pre3 + func_label;
              out << label3 << " DB 02H" << '\n';
              out << '\t' << "DB 00H" << '\n';
              out << '\t' << "DB 00H" << '\n';
              out << '\t' << "DB 02H" << '\n';
              out << '\t' << "DD imagerel " << label2 << '\n';
              out << "xdata	ENDS" << '\n';

              out << "xdata	SEGMENT" << '\n';
              out << '\t' << "ALIGN 4" << '\n';
              string label4 = x64_gen::pre4 + func_label;
              out << label4 << " DB 04H" << '\n';
              out << '\t' << "DB 08H" << '\n';
              out << '\t' << "DB 010H" << '\n';
              out << "xdata	ENDS" << '\n';

              out << "xdata	SEGMENT" << '\n';
              out << '\t' << "ALIGN 4" << '\n';
              out << except::ms::out_table::x64_gen::pre5;
              out << func_label << ' ';
              out << "DB 038H" << '\n';
              out << '\t' << "DD imagerel " << label4 << '\n';
              out << '\t' << "DD imagerel " << label3 << '\n';
              out << '\t' << "DD imagerel " << label1 << '\n';
              out << "xdata	ENDS" << '\n';


              if (!T)
                return;

              out << "CONST	SEGMENT" << '\n';
              string label5 = func_label + "$rtcName$";
              out << label5 << ' ';
              if (T->get_tag()) {
                out << "DB 06fH" << '\n';
                out << '\t' << "DB 06fH" << '\n';
                out << '\t' << "DB 00H" << '\n';
                out << '\t' << "ORG $+13" << '\n';
              }
              else {
                out << "DB 070H" << '\n';
                out << '\t' << "DB 00H" << '\n';
                out << '\t' << "ORG $+14" << '\n';
              }
              string label6 = func_label + "$rtcVarDesc";
              out << label6 << ' ' << "DD 028H" << '\n';
              out << '\t' << "DD 08H" << '\n';
              out << '\t' << "DQ " << label5 << '\n';
              out << '\t' << "ORG $+48" << '\n';
              string label7 = func_label + "$rtcFrameData";
              out << label7 << ' ' << "DD 01H" << '\n';
              out << '\t' << "DD 00H" << '\n';
              out << '\t' << "DQ " << label6 << '\n';
              out << "CONST	ENDS" << '\n';
            }
          }
        } // end of namespace x64_gen
        void gen(bool ms_handler)
        {
          if (x64)
            x64_gen::unwind_data(ms_handler);
          if (call_sites.empty()) {
            assert(call_site_t::types.empty());
            if (ms_handler)
              x64 ? x64_gen::action() : x86_gen::action();
            return;
          }
          x64 ? x64_gen::action() : x86_gen::action();
          call_sites.clear();
          for (auto T : call_site_t::types) {
            if (T) {
              if (T->tmp())
                out_type_info(T);
              else
                ms::call_sites_types_to_output.insert(T);
            }
          }
          call_site_t::types.clear();
          call_site_t::offsets.clear();
        }
      } // end of namespace out_table
    } // end of namespace ms
    void out_table(bool ms_handler)
    {
      mode == GNU ? gnu_out_table() : ms::out_table::gen(ms_handler);
    }
#if defined(_MSC_VER) || defined(__CYGWIN__)
    // nothing to be defined
#else // defined(_MSC_VER) || defined(__CYGWIN__)
    void call_frame_t::out_cf()
    {
      out << '\t' << ".byte	0x4" << '\n'; // DW_CFA_advance_loc4
      out << '\t' << ".long	" << m_end << '-' << m_begin << '\n';
    }
    void save_fp::out_cf()
    {
      call_frame_t::out_cf();

      out << '\t' << ".byte	0xe" << '\n'; // DW_CFA_def_cfa_offset
      out << '\t' << ".uleb128 0x8" << '\n'; // offset

      // DW_CFA_offset (0x80) | column (0x05)
      out << '\t' << ".byte	0x85" << '\n';
      out << '\t' << ".uleb128 0x2" << '\n';
    }
    void save_sp::out_cf()
    {
      call_frame_t::out_cf();
      out << '\t' << ".byte	0xd" << '\n'; // DW_CFA_def_cfa_register
      out << '\t' << ".uleb128 0x5" << '\n';
    }
    void save_bx::out_cf()
    {
      call_frame_t::out_cf();
      out << '\t' << ".byte	0x83" << '\n';
      out << '\t' << ".uleb128 0x3" << '\n';
    }
    void recover::out_cf()
    {
      call_frame_t::out_cf();
      out << '\t' << ".byte	0xc5" << '\n';
      out << '\t' << ".byte	0xc3" << '\n';
      out << '\t' << ".byte	0xc" << '\n';
      out << '\t' << ".uleb128 0x4" << '\n';
      out << '\t' << ".uleb128 0x4" << '\n';
    }
    void out_fd(const frame_desc_t& info, string frame_start)
    {
      string fname = info.m_fname;
      string start = ".LSFDE." + fname;
      string end =   ".LEFDE." + fname;
      out << start << ':' << '\n';
      string l1 = ".LASFDE" + fname;
      out << '\t' << ".long	" << end << '-' << l1 << '\n'; // FDE Length
      out << l1 << ':' << '\n';

      // FDE CIE offset
      out << '\t' << ".long	" << l1 << '-' << frame_start << '\n';

      out << '\t' << ".long	" << fname << '\n'; // FDE initial location

      // FDE address range
      out << '\t' << ".long	" << info.m_end << '-' << fname << '\n';

      out << '\t' << ".uleb128 0x4" << '\n'; // Augmentation size

      // Language Specific Data Area
      out << '\t' << ".long	";
      string lsda = info.m_lsda;
      if (lsda.empty())
        out << 0;
      else
        out << lsda;
      out << '\n';

      for (auto p : info.m_cfs)
        p->out_cf();

#ifdef _DEBUG
      for (auto p : info.m_cfs)
        delete p;
#endif // _DEBUG

      out << '\t' << ".align 4" << '\n';
      out << end << ':' << '\n';
    }
    // output Common Infomation Entry
    void out_cie(string frame_start)
    {
      string start = ".LSCIE";
      string end =   ".LECIE";
      // Length of CIE
      out << '\t' << ".long	" << end << '-' << start << '\n';

      out << start << ':' << '\n';

      // CIE Identifier Tag
      out << '\t' << ".long	0" << '\n';

      // CIE Version
      out << '\t' << ".byte	0x3" << '\n';

      // CIE Augmentation
      out << '\t' << ".string	";
      out << '"';
      if (gnu_table_outputed)
        out << "zPL";
      out << '"' << '\n';

      // CIE Code Alignment Factor
      out << '\t' << ".uleb128 0x1" << '\n';

      // CIE Data Alignment Factor
      out << '\t' << ".sleb128 -4" << '\n';

      // CIE RA Column
      out << '\t' << ".uleb128 0x8" << '\n';

      if (gnu_table_outputed) {
        // Augmentation size
        out << '\t' << ".uleb128 0x6" << '\n';

        // Personality
        out << '\t' << ".byte	0" << '\n';
        out << '\t' << ".long	__gxx_personality_v0" << '\n';
        // LSDA Encoding
        out << '\t' << ".byte	0" << '\n';
      }

      // vvvvv output 2 default CFI
      // DW_CFA_def_cfa
      out << '\t' << ".byte	0xc" << '\n';
      // reg_num
      out << '\t' << ".uleb128 0x4" << '\n';
      // offset
      out << '\t' << ".uleb128 0x4" << '\n';

      // DW_CFA_offset(0x80) | reg_num(0x8)
      out << '\t' << ".byte	0x88" << '\n';
      // offset
      out << '\t' << ".uleb128 0x1" << '\n';
      // ^^^^^ output 2 default CFI
      out << '\t' << ".align 4" << '\n';
      out << end << ':' << '\n';
      
      for (const auto& info : fds)
        out_fd(info, frame_start);
    }
    void out_frame()
    {
      if (fds.empty())
        return;
      output_section(EXCEPT_FRAME);
      string start = ".Lframe";
      out << start << ':' << '\n';
      out_cie(start);
      end_section(EXCEPT_FRAME);
    }
    string LCFI_label()
    {
      static int counter;
      ostringstream os;
      if (mode == GNU)
        os << '.';
      os << "LCFI" << counter++;
      if (mode == MS)
        os << '$';
      return os.str();
    }
#endif // defined(_MSC_VER) || defined(__CYGWIN__)
  } // end of nmaespace except
} // end of namespace intel

#endif  // CXX_GENERATOR

