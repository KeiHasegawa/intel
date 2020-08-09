#ifndef _INTEL_H_
#define _INTEL_H_

#define FIX_2020_08_05

#ifdef _MSC_VER
#define DLL_EXPORT __declspec(dllexport)
#else // _MSC_VER
#define DLL_EXPORT
#endif // _MSC_VER

#ifdef CXX_GENERATOR
#define COMPILER cxx_compiler
#else  // CXX_GENERATOR
#define COMPILER c_compiler
#endif  // CXX_GENERATOR

namespace intel {
  extern bool debug_flag;
  extern bool x64;
  extern int first_param_offset;  // offset from frame pointer ( %rbp or %ebp )
                                  // especially for x64, at called function,
                                  // parameters are saved at this point.
  enum mode_t { GNU, MS };
  extern mode_t mode;
  extern bool doll_need(std::string);
  inline std::string sp()
  {
    if (x64)
      return mode == GNU ? "%rsp" : "rsp";
    else
      return mode == GNU ? "%esp" : "esp";
  }
  inline std::string fp()
  {
    if (x64)
      return mode == GNU ? "%rbp" : "rbp";
    else
      return mode == GNU ? "%ebp" : "ebp";
  }
  inline std::string xmm(int n)
  {
    using namespace std;
    return (mode == GNU) ? string("%xmm") + char('0' + n) : string("xmm") + char('0' + n);
  }
  inline char psuffix() { if (mode == MS) return ' '; else return x64 ? 'q' : 'l';  }
  inline int psize() { return x64 ? 8 : 4; }
  inline std::string fistp_suffix() { return x64 ? "q" : "ll";  }
  extern std::set<std::string> defined;
  inline std::string gnu_pseudo(int size)
  {
    switch (size) {
    case 1: return ".byte";
    case 2: return ".word";
    case 4: return ".long";
    default: assert(size == 8); return ".quad";
    }
  }
  inline std::string ms_pseudo(int size)
  {
    switch (size) {
    case 1: return "BYTE";
    case 2: return "WORD";
    case 4: return "DWORD";
    default: assert(size == 8); return "QWORD";
    }
  }
  inline std::string pseudo(int size) { return mode == GNU ? gnu_pseudo(size) : ms_pseudo(size); }
  inline std::string dot_long() { return mode == GNU ? ".long" : "DD"; }
  extern std::ostream out;
  extern void genobj(const COMPILER::scope*);
  extern std::string func_label;
  extern void genfunc(const COMPILER::fundef*, const std::vector<COMPILER::tac*>&);
  namespace reg {
    enum gpr { ax, bx, cx, dx, si, di, r8, r9 };
    std::string name(gpr r, int size);
  }
  struct address {
    enum id_t { MEM, STACK, ALLOCATED, IMM };
    id_t m_id;
    address(id_t id) : m_id(id) {}
    virtual void load() const = 0;
    virtual void load(reg::gpr r) const = 0;
    virtual void store() const = 0;
    virtual void store(reg::gpr r) const = 0;
    virtual void get(reg::gpr r) const = 0;
    virtual std::string expr(int delta = 0, bool special = false) const = 0;
    virtual ~address(){}
  };
  extern std::pair<std::map<COMPILER::usr*, address*>, std::map<COMPILER::var*, address*> > address_descriptor;
  template<class T> struct destroy_address {
    void operator()(std::pair<T, address*> x) {
      delete x.second;
    }
  };
  extern address* getaddr(COMPILER::var*);
  extern std::string external_header;
  struct imm : address {
    bool m_float;
    std::string m_expr;
    void load() const;
    void load(reg::gpr r) const;
    void store() const { assert(0); }
    void store(reg::gpr r) const { assert(0); }
    void get(reg::gpr r) const { assert(0); }
    std::string expr(int delta = 0, bool special = false) const
    {
      assert(!delta);
      return m_expr;
    }
    imm(COMPILER::usr* u);
    imm* imm_cast(){ return this; }
    static bool is(COMPILER::usr* u);
  };
  struct mem : address {
    COMPILER::usr* m_usr;
    std::string m_label;
    mem(COMPILER::usr* u);
    static std::vector<mem*> ungen;
    bool genobj();
    static bool is(COMPILER::usr* u);
    static int pseudo(int offset, const std::pair<int,COMPILER::var*>& p);
    static std::string pseudo_helper1(COMPILER::usr* u);
    static std::string pseudo_helper2(COMPILER::usr* u);
    struct refgen_t {
      std::string m_label;
      COMPILER::usr::flag_t m_flag;
      int m_size;
      bool operator<(const refgen_t& that) const
      {
        if (m_label < that.m_label)
          return true;
        if (m_label > that.m_label)
          return false;
        if (m_flag < that.m_flag)
          return true;
        if (m_flag > that.m_flag)
          return false;
        return m_size < that.m_size;
      }
    refgen_t(std::string label, COMPILER::usr::flag_t f, int size) : m_label(label), m_flag(f), m_size(size) {}
      refgen_t(){}
    };
    static std::set<refgen_t> refed;
    static std::string refgen(const refgen_t& x)
    { return mode == GNU ? gnu_refgen(x) : ms_refgen(x); }
    static std::string gnu_refgen(const refgen_t&);
    static std::string ms_refgen(const refgen_t&);
    static std::set<std::string> refgened;
    void load() const;
    void load(reg::gpr r) const;
    void load_adc(reg::gpr r) const;
    void store() const;
    void store(reg::gpr r) const;
    void indirect_code(reg::gpr r) const;
    void get(reg::gpr) const;
    std::string expr(int delta = 0, bool special = false) const;
  };
  namespace mem_impl {
    void load_label_x64(reg::gpr r, std::string label,
			COMPILER::usr::flag_t f, int size);
  } // end of namespace mem_impl
  struct stack : address {
    COMPILER::var* m_var;
    int m_offset;
    void load() const;
    void load(reg::gpr r) const;
    void store() const;
    void store(reg::gpr r) const;
    void get(reg::gpr r) const;
    std::string expr(int delta = 0, bool special = false) const;
    stack(COMPILER::var* v, int offset)
      : address(STACK), m_var(v), m_offset(offset) {}
    static int local_area;  // the amount of local variables and medium variables in the function.
    static int delta_sp;  // shifted value of stack pointer.
    static const int threshold_alloca = 30000;
  };
  struct allocated : address {
    int m_offset;
    void load() const { assert(0); }
    void load(reg::gpr r) const { assert(0); }
    void store() const { assert(0); }
    void store(reg::gpr r) const { assert(0); }
    void get(reg::gpr r) const;
    std::string expr(int delta = 0, bool specail = false) const;
    allocated(int offset) : address(ALLOCATED), m_offset(offset) {}
    static int base;
  };
  namespace literal {
    namespace string {
      bool is(COMPILER::usr* u);
    }  // end of namespace string
    namespace floating {
      bool big(COMPILER::usr* u);
      void output(COMPILER::usr* u);
      namespace long_double {
        void bit(unsigned char*, const char*);
        void add(unsigned char*, const unsigned char*);
        void sub(unsigned char*, const unsigned char*);
        void mul(unsigned char*, const unsigned char*);
        void div(unsigned char*, const unsigned char*);
        void neg(unsigned char*, const unsigned char*);
        bool zero(const unsigned char*);
        double to_double(const unsigned char*);
        void from_double(unsigned char*, double);
        bool cmp(COMPILER::goto3ac::op, const unsigned char*, const unsigned char*);
        extern int size;
      }  // end of namespace long_double
    }  // end of namespace floating
    namespace integer {
      bool big(COMPILER::usr* u);
      void output(COMPILER::usr* u);
    }  // end of namespace integer
  }  // end of namespace literal
  extern char suffix(int);
  extern char fsuffix(int);

  enum section { NONE, CODE, ROMDATA, RAM, BSS, CTOR, DTOR,
		 EXCEPT_TABLE, EXCEPT_FRAME };
  extern void output_section(section);
  extern void end_section(section);
  struct sec_hlp {
    enum section m_section;
    sec_hlp(enum section s) : m_section(s) { output_section(m_section); }
    ~sec_hlp() { end_section(m_section); }
  };
  extern std::string new_label(std::string head);
  extern std::pair<std::map<std::pair<int,COMPILER::goto3ac*>, std::string>, std::map<int, std::vector<std::string> > > label_table;

  extern void (*output3ac)(std::ostream& os, COMPILER::tac* tac);
  struct reference_constant {
    std::string m_label;
    bool m_out;
    void output();
    virtual void output_value() const = 0;
  };
  void fld(COMPILER::var*);
  void fstp(COMPILER::var*);
  class uint64_float_t : public reference_constant {
    uint64_float_t(){}
  public:
    static uint64_float_t obj;
    void output_value() const;
  };
  class uint64_double_t : public reference_constant {
    uint64_double_t(){}
  public:
    static uint64_double_t obj;
    void output_value() const;
  };
  class uint64_ld_t : public reference_constant {
    uint64_ld_t(){}
  public:
    static uint64_ld_t obj;
    void output_value() const;
  };
  class ld_uint64_t : public reference_constant {
    ld_uint64_t() {}
  public:
    static ld_uint64_t obj;
    void output_value() const;
  };
  class uminus_float_t : public reference_constant {
    uminus_float_t() {}
  public:
    static uminus_float_t obj;
    void output_value() const;
  };
  class uminus_double_t : public reference_constant {
    uminus_double_t() {}
  public:
    static uminus_double_t obj;
    void output_value() const;
  };
  class real_uint64_t : public reference_constant {
    real_uint64_t() {}
  public:
    static real_uint64_t obj;
    void output_value() const;
  };
  class float_uint64_t : public reference_constant {
    float_uint64_t() {}
  public:
    static float_uint64_t obj;
    void output_value() const;
  };
  class double_uint64_t : public reference_constant {
    double_uint64_t() {}
  public:
    static double_uint64_t obj;
    void output_value() const;
  };

  extern std::string pseudo_global;
  extern std::string comment_start;

#ifdef CXX_GENERATOR
  extern std::string cxx_label(COMPILER::usr*);
  extern void init_term_fun();
  extern bool
  incomplete(const std::pair<const COMPILER::type*, COMPILER::var*>&);

  namespace exception {
#ifndef FIX_2020_08_09
    struct call_site_t {
      std::string m_start;
      std::string m_end;
      std::string m_landing;
      static std::vector<const COMPILER::type*> types;
    };
#else
    struct call_site_t {
      std::string m_start;
      std::string m_end;
      std::string m_landing;
    };
    extern std::vector<const COMPILER::type*> types;
#endif
    extern std::string label(const COMPILER::type*, char);
    extern std::vector<call_site_t> call_sites;
    extern void out_table();
    struct call_frame_t {
      std::string m_begin;
      std::string m_end;
      call_frame_t(std::string b, std::string e) : m_begin(b), m_end(e) {}
      virtual void out_cf();
      virtual ~call_frame_t(){}
    };
    struct save_fp : call_frame_t {
      save_fp(std::string b, std::string e) : call_frame_t(b, e) {}
      void out_cf();
    };
    struct save_sp : call_frame_t {
      save_sp(std::string b, std::string e) : call_frame_t(b, e) {}
      void out_cf();
    };
    struct recover : call_frame_t {
      recover(std::string b, std::string e) : call_frame_t(b, e) {}
      void out_cf();
    };
    struct call : call_frame_t {
      call(std::string b, std::string e) : call_frame_t(b, e) {}
      void out_cf();
    };
    struct frame_desc_t {
      std::string m_fname;
      std::string m_end;
      std::vector<call_frame_t*> m_cfs;
      std::string m_lsda;
    };
    extern std::vector<frame_desc_t> fds;
    extern void out_frame();
    extern std::string LFCI_label();
  } // end of nmaespace exception

#endif // CXX_GENERATOR

}  // end of namespace intel

#endif // _INTEL_H_
