#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR
#include "intel.h"

namespace intel {
  void usrs1(const std::pair<std::string, std::vector<COMPILER::usr*> >&);
  bool usrs2(bool done, COMPILER::usr*);
#ifdef CXX_GENERATOR
  bool incomplete(const std::pair<const COMPILER::type*, COMPILER::var*>& x)
  {
    using namespace std;
    using namespace COMPILER;
    const type* T = x.first;
    if (!T) {
      var* v = x.second;
      if (v->addrof_cast())
	return false;
      assert(v->usr_cast());
      usr* u = static_cast<usr*>(v);
      return u->m_flag2 & usr::TEMPL_PARAM;
    }
    T = T->unqualified();
    if (T->m_id == type::TEMPLATE_PARAM)
      return true;
    if (tag* ptr = T->get_tag()) {
      if (ptr->m_flag & tag::TYPENAMED) {
	if (T->m_id == type::INCOMPLETE_TAGGED)
	  return true;
      }
      if (ptr->m_flag & tag::INSTANTIATE) {
	instantiated_tag* it = static_cast<instantiated_tag*>(ptr);
	const instantiated_tag::SEED& seed = it->m_seed;
	typedef instantiated_tag::SEED::const_iterator IT;
	IT p = find_if(begin(seed), end(seed), incomplete);
	return p != end(seed);
      }
    }
    return false;
  }
#endif // CXX_GENERATOR
}

void intel::genobj(const COMPILER::scope* p)
{
  using namespace std;
  using namespace COMPILER;
#ifdef CXX_GENERATOR
  if (p->m_id == scope::TAG) {
    const tag* ptr = static_cast<const tag*>(p);
    if (ptr->m_flag & tag::TEMPLATE)
      return;
    if (ptr->m_flag & tag::INSTANTIATE) {
      const instantiated_tag* it = static_cast<const instantiated_tag*>(ptr);
      const instantiated_tag::SEED& seed = it->m_seed;
      typedef instantiated_tag::SEED::const_iterator IT;
      IT p = find_if(begin(seed), end(seed), incomplete);
      if (p != end(seed))
	return;
    }
  }
#endif // CXX_GENERATOR
  const map<string, vector<usr*> >& usrs = p->m_usrs;
  for_each(usrs.begin(), usrs.end(),usrs1);
  for_each(mem::ungen.begin(), mem::ungen.end(), mem_fun(&mem::genobj));
  mem::ungen.clear();
  const vector<scope*>& c = p->m_children;
  for_each(c.begin(),c.end(),genobj);
}

void intel::usrs1(const std::pair<std::string, std::vector<COMPILER::usr*> >& entry)
{
  using namespace std;
  using namespace COMPILER;
  const vector<usr*>& vec = entry.second;
  accumulate(vec.begin(),vec.end(),false,usrs2);
}

bool intel::usrs2(bool done, COMPILER::usr* u)
{
  using namespace std;
  using namespace COMPILER;
  if (is_external_declaration(u)) {
    map<usr*, address*>::const_iterator p = address_descriptor.first.find(u);
    if (p != address_descriptor.first.end()) {
#ifdef CXX_GENERATOR
      usr::flag_t flag = u->m_flag;
      return flag & usr::WITH_INI;
#else // CXX_GENERATOR
      return false;
#endif // CXX_GENERATOR
    }
  }
  if (imm::is(u)) {
    if (is_external_declaration(u))
      address_descriptor.first[u] = new imm(u);
    else
      address_descriptor.second[u] = new imm(u);
  }
  else if (mem::is(u)) {
    mem* m = new mem(u);
    if (is_external_declaration(u))
      address_descriptor.first[u] = m;
    else
      address_descriptor.second[u] = m;
    if ( !done )
      return m->genobj();
  }
  return done;
}
