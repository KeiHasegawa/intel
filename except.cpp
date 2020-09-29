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
    std::string ms::pre1 = "_TI2C";
    std::string ms::pre2 = "_CTA2";
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
      void ms_none_tag(const type* T)
      {
        string La = ms::label(ms::pre1, T);
        string Lb = ms::label(ms::pre2, T);
        string pre = x64 ? "imagerel\t" : "";

        out << "xdata$x" << '\t' << "SEGMENT" << '\n';
        out << La;
        out << '\t' << "DD" << '\t' << "01H" << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << '\t' << "DD" << '\t' << pre << Lb << '\n';
        out << "xdata$x" << '\t' << "ENDS" << '\n';

        out << "xdata$x" << '\t' << "SEGMENT" << '\n';
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
        out << Lc;
        out << '\t' << "DD" << '\t' << "01H" << '\n';
        out << '\t' << "DD" << '\t' << pre << Le << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
        out << '\t' << "ORG $ + 4" << '\n';
        int psz = psize();
        out << '\t' << "DD" << '\t' << '0' << psz << 'H' << '\n';
        out << '\t' << "DD" << '\t' << "00H" << '\n';
        out << "xdata$x" << '\t' << "ENDS" << '\n';

        string Lf;
        if (!Ld.empty()) {
          Lf = ms::pre4 + ms::vpsig;
          out << "xdata$x" << '\t' << "SEGMENT" << '\n';
          out << Ld;
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << '\t' << "DD" << '\t' << pre << Lf << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          out << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
          out << '\t' << "ORG $ + 4" << '\n';
          out << '\t' << "DD" << '\t' << '0' << psz << 'H' << '\n';
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
        mode == GNU ? gnu_none_tag(T) : ms_none_tag(T);
      }
    } // end of namespace out_type_impl
    void out_type_info(const type* T)
    {
      if (!T)
        return;
      tag* ptr = T->get_tag();
      if (!ptr)
        return out_type_impl::none_tag(T);
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
    namespace ms {
      namespace out_call_site_type_impl {
        void none_tag(const type* T)
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
        if (!T)
          return;
        tag* ptr = T->get_tag();
        if (!ptr)
          return out_call_site_type_impl::none_tag(T);
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
        const string pre1 = "__catchsym$";
        const string pre2 = "__unwindtable$";
        const string pre3 = "__tryblocktable$";
        void gen()
        {
          if (call_sites.empty()) {
            assert(call_site_t::types.empty());
            return;
          }
          out << "xdata$x" << '\t' << "SEGMENT" << '\n';
          string label = out_table::pre1 + func_label;
          out << label << '\t' << "DD" << '\t' << "01H" << '\n';
          if (call_site_t::types.size() != 1)
            return;  // not implemented
          const type* T = call_site_t::types.back();
          string Le = ms::label(ms::pre4, T);
          out << '\t' << "DD" << '\t' << Le << '\n';
          out << '\t' << "DD" << '\t' << "0ffffffe8H" << '\n';
          if (call_sites.size() != 1)
            return;  // not implemented
          const call_site_t& info = call_sites.back();
          string landing = info.m_landing;
          assert(!landing.empty());
          out << '\t' << "DD" << '\t' << landing << '\n';
          string label2 = out_table::pre2 + func_label;
          out << label2 << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          out << '\t' << "DD" << '\t' << "0ffffffffH" << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          string label3 = out_table::pre3 + func_label;
          out << label3 << '\t' << "DD" << '\t' << "00H" << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << '\t' << "DD" << '\t' << label << '\n';
          string label4 = out_table::pre4 + func_label;
          out << label4 << '\t' << "DD" << '\t' << "019930522H" << '\n';
          out << '\t' << "DD" << '\t' << "02H" << '\n';
          out << '\t' << "DD" << '\t' << label2 << '\n';
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << '\t' << "DD" << '\t' << label3 << '\n';
          out << '\t' << "DD" << '\t' << "2 DUP(00H)" << '\n';
          out << '\t' << "DD" << '\t' << "00H" << '\n';
          out << '\t' << "DD" << '\t' << "01H" << '\n';
          out << "xdata$x" << '\t' << "ENDS" << '\n';
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
        }
      } // end of namespace out_table
    } // end of namespace ms
    void out_table()
    {
      mode == GNU ? gnu_out_table() : ms::out_table::gen();
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

