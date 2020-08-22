#ifdef CXX_GENERATOR
#include "stdafx.h"
#include "cxx_core.h"
#include "intel.h"

void debug_break()
{
}

namespace intel {
  namespace except {
    using namespace std;
    using namespace COMPILER;
    vector<call_site_t> call_sites;
    vector<const type*> types;
    vector<const type*> call_site_t::types;
    vector<const type*> throw_types;
    set<const type*> types_to_output;
    vector<frame_desc_t> fds;
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
    string label(const type* T, char c)
    {
      ostringstream os;
      os << "_ZT" << c;
      T->encode(os);
      return os.str();
    }
    void out_type(const type* T)
    {
      out << '\t' << ".long	";
      if (T)
	out << label(T, 'I');
      else
	out << 0;
      out << '\n';
    }
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
      for_each(rbegin(v), rend(v), out_type);
      out << end << ':' << '\n';
    }
    bool table_outputed;
    void out_type_info(const type* T)
    {
      debug_break();
      if (!T)
	return;
      tag* ptr = T->get_tag();
      if (!ptr)
	return;
      string L1 = label(T, 'I');
      out << '\t' << ".weak	" << L1 << '\n';
      out << '\t' << ".section	.rodata." << L1 << ",\"aG\",@progbits,";
      out << L1 << ",comdat" << '\n';
      out << '\t' << ".align 4" << '\n';
      out << '\t' << ".type	" << L1 << ", @object" << '\n';
      out << '\t' << ".size	" << L1 << ", 8" << '\n';
      out << L1 << ':' << '\n';
      out << '\t' << ".long	";
      vector<base*>* bases = ptr->m_bases;
      if (!bases)
	out << "_ZTVN10__cxxabiv117__class_type_infoE+8";
      else
	out << "_ZTVN10__cxxabiv120__si_class_type_infoE+8";
      out << '\n';
      string L2 = label(T, 'S');
      out << '\t' << ".long	" << L2 << '\n';
      if (bases) {
	for (auto b : *bases) {
	  tag* bptr = b->m_tag;
	  const type* bT = bptr->m_types.second;
	  assert(bT);
	  string bL = label(bT, 'I');
	  out << '\t' << ".long	" << bL << '\n';
	}
      }

      out << '\t' << ".weak	" << L2 << '\n';
      out << '\t' << ".section	.rodata." << L2 << ",\"aG\",@progbits,";
      out << L2 << ",comdat" << '\n';
      out << '\t' << ".align 4" << '\n';
      out << '\t' << ".type	" << L2 << ", @object" << '\n';
      ostringstream os;
      T->encode(os);
      string s = os.str();
      out << '\t' << ".size	" << L2 << ", " << s.length()+1 << '\n';
      out << L2 << ':' << '\n';
      out << '\t' << ".string	" << '"' << s << '"' << '\n';
    }
    void out_table()
    {
      debug_break();
      if (call_sites.empty()) {
	assert(call_site_t::types.empty());
	return;
      }
      output_section(EXCEPT_TABLE);
      out << '\t' << ".align 4" << '\n';
      ostringstream os;
      if (mode == GNU)
	os << '.';
      os << "LLSDA" << '.' << func_label;
      if (mode == MS)
	os << '$';
      string label = os.str();
      out << label << ':' << '\n';
      assert(!except::fds.empty());
      except::frame_desc_t& fd = except::fds.back();
      fd.m_lsda = label;
      typedef vector<call_site_t>::const_iterator IT;
      IT p = find_if(begin(call_sites), end(call_sites),
		     [](const call_site_t& info){ return info.m_for_dest; });
      bool for_dest = (p != end(call_sites));
      out << '\t' << ".byte	0xff" << '\n'; // LDSA header
      out << '\t' << ".byte	";
      if (!for_dest)
	out << 0;
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
      table_outputed = true;
    }
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
      if (table_outputed)
	out << "zPL";
      out << '"' << '\n';

      // CIE Code Alignment Factor
      out << '\t' << ".uleb128 0x1" << '\n';

      // CIE Data Alignment Factor
      out << '\t' << ".sleb128 -4" << '\n';

      // CIE RA Column
      out << '\t' << ".uleb128 0x8" << '\n';

      if (table_outputed) {
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
  } // end of nmaespace except
} // end of namespace intel


#endif  // CXX_GENERATOR

