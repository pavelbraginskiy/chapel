/* -*-Mode: c++;-*-
*/
#ifndef _pn_H_
#define _pn_H_

class Code;
class Prim;
class Dom;
class LoopNode;
class Fun;
class MPosition;
class Region;
class UNode;
class Sym;

/* #define CONC_IMPLEMENTED 1 */

// Program node

class Callees : public gc {
 public:
  Vec<Fun *> funs; 
  Vec<MPosition *> arg_positions; 
};

class PNode : public gc {
 public:
  Code *code;
  int id;
  uint  is_dead : 1;
        //variables this node assigns
  Vec<Var *> lvals;
  //variables this node reads
  Vec<Var *> rvals;
  Vec<Var *> tvals;

  // Control-Flow Graph (CFG): cfg.cpp, ssu.cpp 
  int mark; // ssu.c
  Vec<PNode *> cfg_succ;
  Vec<PNode *> cfg_pred;
#ifdef CONC_IMPLEMENTED
  Vec<PNode *> conc_succ;
  Vec<PNode *> conc_pred;
#endif

  // Single-Entry Single-Exit Region (SESE): ssu.cpp
  Region *region;

  // Single-Static Value Functions
  //MOVE nodes that logically follow this node
  Vec<PNode *> phi;
  //MOVE nodes that logically precede this node
  Vec<PNode *> phy;

  // Primitive
  Prim *prim;

  // Callees
  Callees *callees;
  Callees *next_callees;

  // Temporary Space
  union {
    UNode *unode; // ssu.cpp
    int used; // ssu.cpp
    BlockHash<Var *, PointerHashFns> *live_vars; // ssu.cpp
    LoopNode *loop_node; // loop.cpp
  };
  Vec<Var *> lvals_set; // ssu.cpp
  Map<PNode *, int> cfg_pred_index;  // ssu.cpp, cg.cpp

  // Dominators
  Dom *dom; // dom.cpp
  
  // Cloning
  Sym *creates;

  // inline.cpp
  float execution_frequency;
  float false_branch_frequency;

  // Constructors
  PNode(Code *c);
  PNode();
};
#define forv_PNode(_p, _v) forv_Vec(PNode, _p, _v)

void collect_Vars_PNode(PNode *n, Vec<Var *> &vars);
int compar_pnodes(const void *ai, const void *aj);

typedef Vec<PNode *> VecPNode;
typedef Map<PNode *, VecPNode> MapPNVecPN;

#endif

