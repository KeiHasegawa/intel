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
    void out_call_site(const call_site_t& info, string fname)
    {
      string start = info.m_start;
      string end = info.m_end;
      string landing = info.m_landing;
      out << '\t' << ".uleb128 " << start << '-' << fname << ' ';
      out << comment_start << " region start" << '\n';
      out << '\t' << ".uleb128 " << end << '-' << start << ' ';
      out << comment_start << " length" << '\n';
      if (landing.empty()) {
	out << '\t' << ".uleb128 0" << ' ';
	out << comment_start << " landing pad" << '\n';
	out << '\t' << ".uleb128 0" << ' ';
	out << comment_start << " action " << '\n';
      }
      else {
	out << '\t' << ".uleb128 " << landing << '-' << fname << ' ';
	out << comment_start << " landing pad" << '\n';
	out << '\t' << ".uleb128 0x1" << ' ';
	out << comment_start << " action " << '\n';
      }
    }
    void out_call_sites(string fname)
    {
      out << '\t' << ".byte	0x1" << ' ';
      out << comment_start << " Call-site format" << '\n';
      string start = ".LLSDACSB." + fname;
      string end   = ".LLSDACSE." + fname;

      out << '\t' << ".uleb128 " << end << '-' << start << ' ';
      out << comment_start << " Call-site table length " << '\n';
      out << start << ':' << '\n';
      for (const auto& info : call_sites)
	out_call_site(info, fname);
      out << end << ':' << '\n';
    }
    void out_action_record(const call_site_t& info)
    {
      string landing = info.m_landing;
      int n = landing.empty() ? 0 : 1;
      out << '\t' << ".byte " << n << ' ';
      out << comment_start << " Action record table" << '\n';
    }
    void out_action_records()
    {
      for (const auto& info : call_sites)
	out_action_record(info);
    }
    void out_type(const type* T)
    {
      out << '\t' << ".long _ZTI";
      T->encode(out);
      out << ' ' << comment_start << " Type info" << '\n';
    }
    void out_lsda(const fundef* fdef)
    {
      usr* u = fdef->m_usr;
      string fname = cxx_label(u);
      string start = ".LLSDATTD." + fname;
      string end =   ".LLSDATT."  + fname;
      out << '\t' << ".uleb128 " << end << '-' << start << ' ';
      out << comment_start << " LSDA length" << '\n';
      out << start << ':' << '\n';

      out_call_sites(fname);
      out_action_records();

      out << '\t' << ".align 4" << '\n';
      for (auto T : call_site_t::types)
	out_type(T);
      out << end << ':' << '\n';
    }
    void out_table(const fundef* fdef)
    {
      if (call_sites.empty()) {
	assert(call_site_t::types.empty());
	return;
      }
      debug_break();
      output_section(EXCEPT_TABLE);
      out << '\t' << ".align 4" << '\n';
      out << '\t' << ".byte	0xff" << ' ';
      out << comment_start << " LDSA header" << '\n';
      out << '\t' << ".byte	0" << ' ';
      out << comment_start << " Type Format" << '\n';
      out_lsda(fdef);
      call_sites.clear();
      call_site_t::types.clear();
      end_section(EXCEPT_TABLE);
    }
    void out_cf(const call_frame_t& info)
    {
#if 0
      string begin = info.m_begin;
      string end = info.m_end;
      out << '\t' << ".byte	0x4" << ' ';
      out << comment_start << " DW_CFA_advance_loc4" << '\n';
      out << '\t' << ".long	" << end << '-' << begin << '\n';

      if (xxx) {
	out << '\t' << ".byte	0xe" << ' ';
	out << comment_start << " DW_CFA_def_cfa_offset" << '\n';
	out << '\t' << ".uleb128 0x8" << '\n';
	out << comment_start << " offset" << '\n';
	out << '\t' << ".byte	0x85" << ' ';
	out << comment_start << " DW_CFA_offset (0x80) | column (0x05)" << '\n';
	out << '\t' << ".uleb128 0x2" << '\n';
      }
      else {
o	out << '\t' << ".byte	0xd" << ' ';
	out << comment_start << " DW_CFA_def_cfa_register" << '\n';
	out << ".uleb128 0x5" << '\n';
      }
#endif
    }
    void out_fd(const frame_desc_t& info, string frame_start)
    {
      string fname = info.m_fname;
      string start = ".LSFDE." + fname;
      string end =   ".LEFDE." + fname;
      out << start << ':' << '\n';
      string l1 = ".LASFDE" + fname;
      out << '\t' << ".long	" << end << '-' << l1 << ' ';
      out << comment_start << " FDE Length" << '\n';
      out << l1 << ':' << '\n';
      out << '\t' << ".long	" << l1 << '-' << frame_start << ' ';
      out << comment_start << " FDE CIE offset" << '\n';
      out << '\t' << ".long	" << fname << ' ';
      out << comment_start << " FDE initial location" << '\n';
      out << '\t' << ".long	" << info.m_end << '-' << fname << '\n';
      out << comment_start << " FDE address range" << '\n';
      out << '\t' << ".uleb128 0x4" << ' ';
      out << comment_start << " Augmentation size" << '\n';
      out << '\t' << ".long	0" << ' ';
      out << comment_start << " Language Specific Data Area (none)" << '\n';

      for (const auto x : info.m_cfs)
	out_cf(x);
    }
    // output Common Infomation Entry
    void out_cie(string frame_start)
    {
      string start = ".SCIE";
      string end =   ".ECIE";
      out << '\t' << ".long	" << end << '-' << start << ' ';
      out << comment_start << " Length of CIE" << '\n';
      out << start << ':' << '\n';
      out << '\t' << ".long	0" << ' ';
      out << comment_start << " CIE Identifier Tag" << '\n';
      out << '\t' << ".byte	0x3" << ' ';
      out << comment_start << " CIE Version" << '\n';
      out << '\t' << ".string	\"zPL\" ";
      out << comment_start << " CIE Augmentation" << '\n';
      out << '\t' << ".uleb128 0x1" << ' ';
      out << comment_start << " CIE Code Alignment Factor" << '\n';
      out << '\t' << ".sleb128 -4" << ' ';
      out << comment_start << " CIE Data Alignment Factor" << '\n';
      out << '\t' << ".uleb128 0x8" << ' ';
      out << comment_start << " CIE RA Column" << '\n';
      out << '\t' << ".uleb128 0x6" << ' ';
      out << comment_start << " Augmentation size" << '\n';
      out << '\t' << ".byte	0" << ' ';
      out << comment_start << " Personality" << '\n';
      out << '\t' << ".long	__gxx_personality_v0" << '\n';
      out << '\t' << ".byte	0" << ' ';
      out << comment_start << " LSDA Encoding" << '\n';

      // vvvvv output 2 default CFI
      out << '\t' << ".byte	0xc // DW_CFA_def_cfa" << '\n';
      out << '\t' << ".uleb128 0x4 // reg_num" << '\n';
      out << '\t' << ".uleb128 0x4 // offset" << '\n';

      out << '\t' << ".byte	0x88" << ' ';
      out << "// DW_CFA_offset(0x80) | reg_num(0x8)" << '\n';
      out << '\t' << ".uleb128 0x1 // offset" << '\n';
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
  } // end of nmaespace exception
} // end of namespace intel


#endif  // CXX_GENERATOR

