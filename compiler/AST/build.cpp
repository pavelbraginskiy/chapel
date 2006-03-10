#include "build.h"
#include "baseAST.h"
#include "expr.h"
#include "stmt.h"
#include "stringutil.h"
#include "symbol.h"
#include "type.h"

FnSymbol* build_if_expr(Expr* e, Expr* e1, Expr* e2) {
  static int uid = 1;
  FnSymbol* fn = new FnSymbol(stringcat("_if_fn", intstring(uid++)));
  fn->retRef = true;
  fn->formals = new AList<DefExpr>();
  fn->addPragma("inline");
  if (e2)
    fn->insertAtTail(new CondStmt(e, new ReturnStmt(e1), new ReturnStmt(e2)));
  else
    USR_FATAL("if-then expressions currently require an else-clause");
  return fn;
}


FnSymbol* build_let_expr(AList<Stmt>* decls, Expr* expr) {
  static int uid = 1;
  FnSymbol* fn = new FnSymbol(stringcat("_let_fn", intstring(uid++)));
  fn->formals = new AList<DefExpr>();
  fn->addPragma("inline");
  fn->insertAtTail(decls);
  fn->insertAtTail(new ReturnStmt(expr));
  return fn;
}


static void build_loop_labels(BlockStmt* body) {
  static int uid = 1;
  body->pre_loop = new LabelSymbol(stringcat("_pre_loop", intstring(uid)));
  body->post_loop = new LabelSymbol(stringcat("_post_loop", intstring(uid)));
  uid++;
}


AList<Stmt>* build_while_do_block(Expr* cond, BlockStmt* body) {
  if (body->blockType != BLOCK_NORMAL)
    INT_FATAL(body, "Abnormal block passed to build_while_do_block");
  body->blockType = BLOCK_WHILE_DO;
  build_loop_labels(body);
  AList<Stmt>* stmts = new AList<Stmt>();
  stmts->insertAtTail(new LabelStmt(new DefExpr(body->pre_loop)));
  stmts->insertAtTail(new CondStmt(cond, body));
  body->insertAtTail(new GotoStmt(goto_normal, body->pre_loop));
  stmts->insertAtTail(new LabelStmt(new DefExpr(body->post_loop)));
  return stmts;
}


AList<Stmt>* build_do_while_block(Expr* cond, BlockStmt* body) {
  if (body->blockType != BLOCK_NORMAL)
    INT_FATAL(body, "Abnormal block passed to build_do_while_block");
  body->blockType = BLOCK_DO_WHILE;
  build_loop_labels(body);
  AList<Stmt>* stmts = new AList<Stmt>();
  stmts->insertAtTail(new LabelStmt(new DefExpr(body->pre_loop)));
  stmts->insertAtTail(body);
  stmts->insertAtTail(new CondStmt(cond, new GotoStmt(goto_normal, body->pre_loop)));
  stmts->insertAtTail(new LabelStmt(new DefExpr(body->post_loop)));
  return stmts;
}


AList<Stmt>* build_for_block(ForLoopStmt* stmt) {
  AList<Stmt>* stmts = new AList<Stmt>();
  BlockStmt* body = new BlockStmt(stmt);
  switch (stmt->forLoopStmtTag) {
  case FORLOOPSTMT_FOR:
    body->blockType = BLOCK_FOR;
    break;
  case FORLOOPSTMT_FORALL:
    body->blockType = BLOCK_FORALL;
    break;
  case FORLOOPSTMT_ORDEREDFORALL:
    body->blockType = BLOCK_ORDERED_FORALL;
    break;
  }
  build_loop_labels(body);
  stmts->insertAtTail(new LabelStmt(new DefExpr(body->pre_loop)));
  stmts->insertAtTail(body);
  stmts->insertAtTail(new LabelStmt(new DefExpr(body->post_loop)));
  return stmts;
}


AList<Stmt>* build_param_for(char* index, Expr* low, Expr* high, AList<Stmt>* stmts) {
  static int uid = 1;
  FnSymbol* fn = new FnSymbol(stringcat("_param_for_fn", intstring(uid++)));
  fn->formals =
    new AList<DefExpr>(new DefExpr(new ArgSymbol(INTENT_PARAM, index, dtInteger)));
  fn->addPragma("inline");
  stmts->insertAtTail(new CallExpr(fn->name, new CallExpr("+", new SymExpr(index), new_IntLiteral(1))));
  fn->insertAtTail(
    new CondStmt(new CallExpr("<=", new SymExpr(index), high), stmts));
  Stmt* def_stmt = new ExprStmt(new DefExpr(fn));
  Stmt* call_stmt = new ExprStmt(new CallExpr(fn->name, low));
  return new AList<Stmt>(def_stmt, call_stmt);
}


AList<Stmt>* build_type_select(AList<Expr>* exprs, AList<WhenStmt>* whenstmts) {
  static int uid = 1;
  FnSymbol* fn;
  AList<Stmt>* stmts = new AList<Stmt>();
  bool has_otherwise = false;

  for_alist(WhenStmt, whenstmt, whenstmts) {
    if (whenstmt->caseExprs->length() == 0) {
      if (has_otherwise)
        USR_FATAL(exprs, "Type select statement has multiple otherwise clauses");
      has_otherwise = true;
      fn = new FnSymbol(stringcat("_typeselect", intstring(uid)));
      fn->formals = new AList<DefExpr>();
      int lid = 1;
      for_alist(Expr, expr, exprs) {
        fn->formals->insertAtTail(
          new DefExpr(
            new ArgSymbol(INTENT_BLANK,
                          stringcat("_t", intstring(lid++)),
                          dtAny)));
      }
      fn->addPragma("inline");
      fn->insertAtTail(whenstmt->doStmt->body->copy());
      stmts->insertAtTail(new DefExpr(fn));
    } else {
      if (whenstmt->caseExprs->length() != exprs->length())
        USR_FATAL(whenstmt, "Type select statement requires number of selectors to be equal to number of when conditions");
      fn = new FnSymbol(stringcat("_typeselect", intstring(uid)));
      fn->formals = new AList<DefExpr>();
      int lid = 1;
      for_alist(Expr, expr, whenstmt->caseExprs) {
        fn->formals->insertAtTail(
          new DefExpr(
            new ArgSymbol(INTENT_BLANK,
                          stringcat("_t", intstring(lid++)),
                          dtUnknown), NULL, expr->copy()));
      }
      fn->addPragma("inline");
      fn->insertAtTail(whenstmt->doStmt->body->copy());
      stmts->insertAtTail(new DefExpr(fn));
    }
  }
  stmts->insertAtTail(new CallExpr(fn->name, exprs));
  return stmts;
}
