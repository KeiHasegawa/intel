#ifdef CXX_GENERATOR
#include "stdafx.h"
#include "cxx_core.h"
#include "intel.h"

void debug_break()
{
}

namespace intel {
  namespace exception {
    using namespace std;
    using namespace COMPILER;
    vector<call_site_t> call_sites;
    vector<const type*> call_site_t::types;
    vector<frame_desc_t> fds;
    void out_call_site(const call_site_t& info)
    {
      string start = info.m_start;
      string end = info.m_end;
      string landing = info.m_landing;

      // region start
      out << '\t' << ".uleb128 " << start << '-' << func_label << '\n';

      out << '\t' << ".uleb128 " << end << '-' << func_label << '\n'; // length

      if (landing.empty()) {
	out << '\t' << ".uleb128 0" << '\n'; // landing pad
	out << '\t' << ".uleb128 0" << '\n';  // action
      }
      else {
	// landing pad
	out << '\t' << ".uleb128 " << landing << '-' << func_label << '\n';
	out << '\t' << ".uleb128 0x1" << '\n'; // action
      }
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
    void out_action_record(const call_site_t& info)
    {
      string landing = info.m_landing;
      int n = landing.empty() ? 0 : 1;
      out << '\t' << ".byte " << n << '\n'; // Action record table
    }
    void out_action_records()
    {
      for (const auto& info : call_sites)
	out_action_record(info);
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
      out << '\t' << ".long       " << label(T, 'I') << '\n';
    }
    void out_lsda()
    {
      string start = ".LLSDATTD." + func_label;
      string end =   ".LLSDATT."  + func_label;
      out << '\t' << ".uleb128 " << end << '-' << start << '\n'; // LSDA length
      out << start << ':' << '\n';

      out_call_sites();
      out_action_records();

      out << '\t' << ".align 4" << '\n';
      for (auto T : call_site_t::types)
	out_type(T);
      out << end << ':' << '\n';
    }
    bool table_outputed;
    void out_type_info(const type* T)
    {
      debug_break();
      if (!T->get_tag())
	return;
      string L1 = label(T, 'I');
      out << '\t' << ".weak	" << L1 << '\n';
      out << '\t' << ".section	.rodata." << L1 << ",\"aG\",@progbits,";
      out << L1 << ",comdat" << '\n';
      out << '\t' << ".align 4" << '\n';
      out << '\t' << ".type	" << L1 << ", @object" << '\n';
      out << '\t' << ".size	" << L1 << ", 8" << '\n';
      out << L1 << ':' << '\n';
      out << '\t' << ".long	_ZTVN10__cxxabiv117__class_type_infoE+8" << '\n';
      string L2 = label(T, 'S');
      out << '\t' << ".long	" << L2 << '\n';
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
      assert(!exception::fds.empty());
      exception::frame_desc_t& fd = exception::fds.back();
      fd.m_lsda = label;
      out << '\t' << ".byte	0xff" << '\n'; // LDSA header
      out << '\t' << ".byte	0" << '\n'; // Type Format
      out_lsda();
      call_sites.clear();
      end_section(EXCEPT_TABLE);
      for (auto T : call_site_t::types)
	out_type_info(T);
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
    void recover::out_cf()
    {
      call_frame_t::out_cf();
      out << '\t' << ".byte	0xc5" << '\n';
      out << '\t' << ".byte	0xc3" << '\n';
      out << '\t' << ".byte	0xc" << '\n';
      out << '\t' << ".uleb128 0x4" << '\n';
      out << '\t' << ".uleb128 0x4" << '\n';
    }
    void call::out_cf()
    {
      call_frame_t::out_cf();
      out << '\t' << ".byte	0x83" << '\n';
      out << '\t' << ".uleb128 0x3" << '\n';
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
      string start = ".SCIE";
      string end =   ".ECIE";
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
    string LFCI_label()
    {
      static int counter;
      ostringstream os;
      if (mode == GNU)
	os << '.';
      os << "LFCI" << counter++;
      if (mode == MS)
	os << '$';
      return os.str();
    }
  } // end of nmaespace exception
} // end of namespace intel


#endif  // CXX_GENERATOR

