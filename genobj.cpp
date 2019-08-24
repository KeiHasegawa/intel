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
}

void intel::genobj(const COMPILER::scope* p)
{
  using namespace std;
  using namespace COMPILER;
#ifdef CXX_GENERATOR
  if (p->m_id == scope::TAG) {
    const tag* ptr = static_cast<const tag*>(p);
    if (ptr->m_template)
      return;
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
    if (p != address_descriptor.first.end())
      return false;
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
