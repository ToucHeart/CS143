

#include "semant.h"
#include "utilities.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern int semant_debug;
extern char *curr_filename;
const int WHITE = 0;
const int GRAY = 1;
const int BLACK = 2;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol arg, arg2, Bool, concat, cool_abort, copy, Int, in_int, in_string,
    IO, length, Main, main_meth, No_class, No_type, Object, out_int, out_string,
    prim_slot, self, SELF_TYPE, Str, str_field, substr, type_name, val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void) {
  arg = idtable.add_string("arg");
  arg2 = idtable.add_string("arg2");
  Bool = idtable.add_string("Bool");
  concat = idtable.add_string("concat");
  cool_abort = idtable.add_string("abort");
  copy = idtable.add_string("copy");
  Int = idtable.add_string("Int");
  in_int = idtable.add_string("in_int");
  in_string = idtable.add_string("in_string");
  IO = idtable.add_string("IO");
  length = idtable.add_string("length");
  Main = idtable.add_string("Main");
  main_meth = idtable.add_string("main");
  //   _no_class is a symbol that can't be the name of any
  //   user-defined class.
  No_class = idtable.add_string("_no_class");
  No_type = idtable.add_string("_no_type");
  Object = idtable.add_string("Object");
  out_int = idtable.add_string("out_int");
  out_string = idtable.add_string("out_string");
  prim_slot = idtable.add_string("_prim_slot");
  self = idtable.add_string("self");
  SELF_TYPE = idtable.add_string("SELF_TYPE");
  Str = idtable.add_string("String");
  str_field = idtable.add_string("_str_field");
  substr = idtable.add_string("substr");
  type_name = idtable.add_string("type_name");
  val = idtable.add_string("_val");
}

void class__class::type_check(Env env) {
  for (int i = this->features->first(); features->more(i);
       i = features->next(i)) {
    features->nth(i)->type_check(env);
  }
}

void class__class::add2env(Env env) {
  for (int i = this->features->first(); features->more(i);
       i = features->next(i)) {
    features->nth(i)->add2env(env);
  }
  env.vars->addid(self, &SELF_TYPE); // add the default value self
}

// get a method in this class or its parent class accordin to name
method_p class__class::get_method(Symbol name, Env env) {
  for (int i = features->first(); features->more(i); i = features->next(i)) {
    method_p m = features->nth(i)->get_method(name);
    if (m) {
      return m;
    }
  }
  Class_ p = env.ct->get_class(this->parent);
  if (p) {
    return p->get_method(name, env);
  }
  return nullptr;
}

void method_class::add2env(Env env) { /*do nothing*/ }

method_p method_class::get_method(Symbol name) {
  if (name == this->name)
    return this;
  return nullptr;
}

method_p attr_class::get_method(Symbol name) { return NULL; }

void class__class::check_inherit_method(Env env, method_p m) {
  for (int i = features->first(); features->more(i); i = features->next(i)) {
    features->nth(i)->check_inherit_method(env, m);
  }
  Class_ p = env.ct->get_class(this->get_parent());
  if (p)
    p->check_inherit_method(env, m);
}

void method_class::check_inherit_method(Env env, method_p m) {
  bool flag = true;
  if (m->name == this->name) {
    if (this->return_type != m->return_type ||
        this->formals->len() != m->formals->len()) {
      flag = false;
    } else {
      for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        if (this->formals->nth(i)->get_type() != m->formals->nth(i)->get_type())
          flag = false;
      }
    }
  }
  if (!flag) {
    env.ct->semant_error(env.cur_class, m)
        << " inherited method doesn't have same signature" << endl;
  }
}

void attr_class::check_inherit_method(Env env, method_p m) { return; }

Symbol method_class::type_check(Env env) {
  // first check inherit method is the same or not
  Class_ p = env.ct->get_class(env.cur_class->get_parent());
  if (p) {
    p->check_inherit_method(env, this);
  }
  env.vars->enterscope();
  for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
    formals->nth(i)->add2env(env);
  }

  Symbol expr_type = this->expr->type_check(env);
  Symbol ret_type = nullptr;
  if (env.ct->conform(expr_type, return_type, env.cur_class)) {
    ret_type = expr_type;
  } else {
    env.ct->semant_error(env.cur_class, this)
        << "in function " << this->name->get_string()
        << ", expression type not conform to return_type " << endl;
    ret_type = Object;
  }
  env.vars->exitscope();
  return ret_type;
}

Symbol attr_class::type_check(Env env) {
  Symbol init_type = init->type_check(env);
  if (init_type == No_type) { // no init provided, no_expr return No_type,
    return type_decl;
  }
  if (env.ct->conform(init_type, type_decl,
                      env.cur_class)) { // init_type should <= type_decl
    return init_type;
  } else {
    env.ct->semant_error(env.cur_class, this)
        << "id name " << name->get_string()
        << ",init type doesn't conform to id type" << endl;
    return Object;
  }
}

// check assignment a <- b , type(b) must <= type(a)
Symbol assign_class::type_check(Env env) {
  if (name == self) {
    env.ct->semant_error(env.cur_class, this)
        << "can't assign to self " << endl;
  }
  Symbol *type_decl_ptr =
      env.vars->lookup(this->name); // get type(a),a maybe not defined
  Symbol type_decl = type_decl_ptr ? (*type_decl_ptr) : nullptr;
  if (type_decl == nullptr) { // a is not defined
    env.ct->semant_error(env.cur_class, this)
        << name->get_string() << " not defined" << endl;
    this->type = Object;
  }
  Symbol init_type = this->expr->type_check(env);
  if (env.ct->conform(init_type, type_decl,
                      env.cur_class)) { // init_type should <= type_decl
    this->type = init_type;
  } else {
    env.ct->semant_error(env.cur_class, this)
        << "id name " << name->get_string()
        << ",init type doesn't conform to id type" << endl;
    this->type = Object;
  }
  return this->type;
}

// for case epxression
Symbol typcase_class::type_check(Env env) {}

// loop always return type Object
Symbol loop_class::type_check(Env env) {
  Symbol type1 = this->pred->type_check(env);
  if (type1 != Bool) {
    env.ct->semant_error(env.cur_class, this)
        << "pred should be Bool type of loop" << endl;
  }
  this->body->type_check(env);
  this->type = Object;
  return this->type;
}

Symbol dispatch_class::type_check(Env env) {
  Symbol expr_type = this->expr->type_check(env);
  method_p func;
  if (expr_type == SELF_TYPE) { // call a function in current class,expr is
                                // self,so get it's type name
    func = env.ct->get_class(env.cur_class->get_name())
               ->get_method(this->name, env);
  } else {
    func = env.ct->get_class(expr_type)->get_method(
        this->name, env); // get the method pointer
  }
  if (func == nullptr) { // this function not exists;
    env.ct->semant_error(env.cur_class, this)
        << name->get_string() << " not exists" << endl;
    this->type = Object;
  } else {
    Formals formals = func->get_formals();
    Symbol ret_type = func->get_ret_type();

    if (formals->len() != actual->len()) {
      env.ct->semant_error(env.cur_class, this)
          << " function parameters are not same to arguments." << endl;
      this->type = Object;
    } else {
      for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Symbol formal_type = formals->nth(i)->get_type();
        Symbol arg_type = actual->nth(i)->type_check(env);
        if (!env.ct->conform(arg_type, formal_type, env.cur_class)) {
          env.ct->semant_error(env.cur_class, actual->nth(i))
              << " arg type doesn't conform to formal type " << endl;
          this->type = Object;
        }
      }
      if (this->type == Object) {
        return this->type;
      }
      this->type = ret_type;
    }
  }
  if (this->type == SELF_TYPE) {
    this->type = expr_type;
  }
  return this->type;
}

Symbol static_dispatch_class::type_check(Env env) {
  Symbol expr_type = this->expr->type_check(env);
  if (!env.ct->conform(expr_type, type_name, env.cur_class)) {
    env.ct->semant_error(env.cur_class, this)
        << " expr type doesn't conform to type name " << type_name->get_string()
        << endl;
  }
  method_p func = env.ct->get_class(this->type_name)
                      ->get_method(this->name, env); // get the method pointer
  if (func == nullptr) { // this function not exists;
    env.ct->semant_error(env.cur_class, this)
        << name->get_string() << " not exists" << endl;
    this->type = Object;
  } else {
    Formals formals = func->get_formals();
    Symbol ret_type = func->get_ret_type();

    if (formals->len() != actual->len()) {
      env.ct->semant_error(env.cur_class, this)
          << " function parameters are not same to arguments." << endl;
      this->type = Object;
    } else {
      for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Symbol formal_type = formals->nth(i)->get_type();
        Symbol arg_type = actual->nth(i)->type_check(env);
        if (!env.ct->conform(arg_type, formal_type, env.cur_class)) {
          env.ct->semant_error(env.cur_class, actual->nth(i))
              << " arg type doesn't conform to formal type " << endl;
          this->type = Object;
        }
      }
      if (this->type == Object) {
        return this->type;
      }
      this->type = ret_type;
    }
  }
  if (this->type == SELF_TYPE) {
    this->type = expr_type;
  }
  return this->type;
}

Symbol no_expr_class::type_check(Env env) {
  this->type = No_type;
  return this->type;
}

Symbol isvoid_class::type_check(Env env) {
  this->e1->type_check(env);
  this->type = Bool;
  return this->type;
}

Symbol new__class::type_check(Env env) {
  if (this->type_name == SELF_TYPE) {
    this->type = type_name;
    return env.cur_class->get_name();
  } else {
    if (env.ct->exist_class(type_name)) {
      this->type = type_name;
    } else {
      env.ct->semant_error(env.cur_class, this)
          << ",type not defined " << type_name->get_string() << endl;
      this->type = Object;
    }
  }
  return this->type;
}

Symbol string_const_class::type_check(Env env) {
  this->type = Str;
  return Str;
}

Symbol bool_const_class::type_check(Env env) {
  this->type = Bool;
  return this->type;
}

Symbol int_const_class::type_check(Env env) {
  this->type = Int;
  return this->type;
}

Symbol comp_class::type_check(Env env) {
  this->e1->type_check(env);
  this->type = Bool;
  return this->type;
}

Symbol cond_class::type_check(Env env) {
  Symbol pred_type = pred->type_check(env);
  if (pred_type != Bool) {
    env.ct->semant_error(env.cur_class, this) << "pred illeagle" << endl;
  }
  Symbol a = then_exp->type_check(env);
  Symbol b = else_exp->type_check(env);
  this->type = env.ct->join_class(a, b);
  return this->type;
}

Symbol eq_class::type_check(Env env) {
  Symbol s1 = e1->type_check(env);
  Symbol s2 = e2->type_check(env);
  if ((s1 == Int && s2 != Int) || (s1 != Int && s2 == Int) ||
      (s1 == Str && s2 != Str) || (s1 != Str && s2 == Str) ||
      (s1 == Bool && s2 != Bool) || (s1 != Bool && s2 == Bool)) {
    env.ct->semant_error(env.cur_class, this)
        << "Can't compare " << s1 << " = " << s2 << endl;
    this->type = Object;
  } else {
    this->type = Bool;
  }
  return this->type;
}

Symbol lt_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  Symbol type2 = e2->type_check(env);
  if (type1 != Int || type2 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Bool;
  }
  return this->type;
}

Symbol leq_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  Symbol type2 = e2->type_check(env);
  if (type1 != Int || type2 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Bool;
  }
  return this->type;
}

Symbol neg_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  if (type1 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Int;
  }
  return this->type;
}

Symbol divide_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  Symbol type2 = e2->type_check(env);
  if (type1 != Int || type2 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Int;
  }
  return this->type;
}

Symbol mul_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  Symbol type2 = e2->type_check(env);
  if (type1 != Int || type2 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Int;
  }
  return this->type;
}

Symbol sub_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  Symbol type2 = e2->type_check(env);
  if (type1 != Int || type2 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Int;
  }
  return this->type;
}

Symbol plus_class::type_check(Env env) {
  Symbol type1 = e1->type_check(env);
  Symbol type2 = e2->type_check(env);
  if (type1 != Int || type2 != Int) {
    env.ct->semant_error(env.cur_class, this)
        << "operands should be int" << endl;
    this->type = Object;
  } else {
    this->type = Int;
  }
  return this->type;
}

Symbol let_class::type_check(Env env) {
  Symbol init_type = this->init->type_check(env);
  if (init_type != No_type &&
      !env.ct->conform(init_type, this->type_decl, env.cur_class)) {
    env.ct->semant_error(env.cur_class, this)
        << " init type doesn't conform to var type " << endl;
  }
  env.vars->enterscope();
  if (identifier != self) {
    env.vars->addid(identifier, &this->type_decl);
  } else {
    env.ct->semant_error(env.cur_class, this)
        << "self can't be var name in let " << endl;
  }
  this->type = this->body->type_check(env);
  env.vars->exitscope();
  return this->type;
}

Symbol block_class::type_check(Env env) {
  Symbol ret_type;
  for (int i = body->first(); body->more(i); i = body->next(i)) {
    ret_type = body->nth(i)->type_check(env);
  }
  this->type = ret_type;
  return ret_type;
}

Symbol object_class::type_check(Env env) {
  Symbol *typeptr = env.vars->lookup(this->name);
  Symbol ret_type = typeptr ? (*typeptr) : nullptr;
  if (ret_type == nullptr) {
    env.ct->semant_error(env.cur_class, this)
        << name->get_string() << " not defined" << endl;
  }
  this->type = ret_type ? ret_type : Object;
  return this->type;
}

void attr_class::add2env(Env env) {
  if (env.vars->probe(name)) {
    env.ct->semant_error(env.cur_class, this)
        << "redefinition of attribute " << name->get_string() << endl;
  } else if (name == self) {
    env.ct->semant_error(env.cur_class, this)
        << "attribute can't be self" << endl;
  } else {
    env.vars->addid(name, &type_decl);
  }
}

void formal_class::add2env(Env env) {
  if (env.vars->probe(name)) {
    env.ct->semant_error(env.cur_class, this)
        << "redefinition of formal " << name->get_string() << endl;
  } else if (name == self) {
    env.ct->semant_error(env.cur_class, this) << "formal can't be self" << endl;
  } else {
    if (type_decl == SELF_TYPE) {
      env.ct->semant_error(env.cur_class, this)
          << " SELF_TYPE can't be parameter type " << endl;
    } else {
      env.vars->addid(name, &type_decl);
    }
  }
}

Symbol formal_class::get_type() { return this->type_decl; }

// init an env,add attr to this env including its parent class's attr
void class__class::init_env(Env env) {
  Symbol p = this->get_parent();
  while (p != No_class) {
    Class_ parent = env.ct->get_class(p);
    parent->add2env(env);
    p = parent->get_parent();
  }
  add2env(env);
}

ClassTable::ClassTable(Classes classes) : semant_errors(0), error_stream(cerr) {
  /* Fill this in */
  install_basic_classes();
  for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
    add2classmap(classes->nth(i));
  }
}

// check whether child <= parent
bool ClassTable::conform(Symbol child, Symbol parent, Class_ c) {
  if (child == SELF_TYPE) {
    child = c->get_name();
  }
  if (parent == SELF_TYPE) {
    parent = c->get_name();
  }
  if (child == parent) {
    return true;
  }
  Symbol p = child;
  while (class_parent.find(p) != class_parent.end()) {
    if (parent == class_parent[p]) {
      return true;
    }
    p = class_parent[p];
  }
  return false;
}

bool ClassTable::exist_class(Symbol s) {
  return class_defs.find(s) != class_defs.end();
}

Symbol ClassTable::get_class_name(Class_ c) {
  if (class_names.find(c) != class_names.end())
    return class_names[c];
  return nullptr;
}

// get LCA of two classes in the class graph
Symbol ClassTable::join_class(Symbol type_a, Symbol type_b) {
  int depth_a = 0, depth_b = 0;
  Symbol a = type_a, b = type_b;
  while (a != Object) {
    a = class_parent[a];
    depth_a++;
  }
  while (b != Object) {
    b = class_parent[b];
    depth_b++;
  }

  int offset = abs(depth_a - depth_b);
  Symbol longer = type_a, shorter = type_b;
  if (depth_a > depth_b) {
    longer = type_a, shorter = type_b;
  } else if (depth_a < depth_b) {
    longer = type_b, shorter = type_a;
  }
  while (offset--) {
    longer = class_parent[longer];
  }
  while (class_parent[longer] != class_parent[shorter]) {
    longer = class_parent[longer];
    shorter = class_parent[shorter];
  }
  return class_parent[longer];
}

void ClassTable::add2classmap(Class_ c) {
  Symbol name = c->get_name();
  Symbol parent = c->get_parent();
  Features f = c->get_features();
  Symbol filename = c->get_filename();
  if (parent == Bool || parent == Int || parent == Str || parent == SELF_TYPE) {
    semant_error(c, c) << "can't inherit from Int/Bool/Str/Self_type." << endl;
  } else if (name == SELF_TYPE) {
    semant_error(c, c) << "Redefinition of basic class SELF_TYPE." << endl;
  } else if (class_defs.find(name) != class_defs.end()) {
    semant_error(c, c) << "Redefinition of class" << endl;
  } else {
    class_defs[name] = c;
    class_parent[name] = parent;
    class_names[c] = name;
  }
}

void ClassTable::visit(Symbol s, std::unordered_map<Symbol, int> &colors) {
  colors[s] = GRAY;
  if (class_parent.find(s) != class_parent.end()) {
    Symbol p = class_parent[s];
    if (colors.find(p) != colors.end()) {
      if (colors[p] == GRAY) {
        semant_error(class_defs[s], class_defs[s])
            << "acycle class definition" << endl;
      }
    } else {
      visit(p, colors);
    }
  }
  colors[s] = BLACK;
}

// check 3 things:
// 1. has Main class
// 2. inherited class must be defined
// 3. no cycle in the class graph
void ClassTable::check_valid() {
  if (class_defs.find(Main) == class_defs.end()) {
    semant_error() << "Class Main is not defined." << endl;
  }
  std::unordered_map<Symbol, int> colors;
  for (auto it = class_parent.begin(); it != class_parent.end(); ++it) {
    Symbol first = it->first, second = it->second;
    if (first != Object && class_defs.find(second) == class_defs.end()) {
      semant_error(class_defs[first], class_defs[first])
          << "inherit class not defined" << endl;
      ;
    } else {
      if (colors.find(first) != colors.end() && colors[first] == BLACK)
        continue;
      visit(first, colors);
    }
  }
}

// get class according to its name
Class_ ClassTable::get_class(Symbol s) {
  if (class_defs.find(s) != class_defs.end()) {
    return class_defs[s];
  }
  return nullptr;
}

void ClassTable::install_basic_classes() {

  // The tree package uses these globals to annotate the classes built below.
  // curr_lineno  = 0;
  Symbol filename = stringtable.add_string("<basic class>");

  // The following demonstrates how to create dummy parse trees to
  // refer to basic Cool classes.  There's no need for method
  // bodies -- these are already built into the runtime system.

  // IMPORTANT: The results of the following expressions are
  // stored in local variables.  You will want to do something
  // with those variables at the end of this method to make this
  // code meaningful.

  //
  // The Object class has no parent class. Its methods are
  //        abort() : Object    aborts the program
  //        type_name() : Str   returns a string representation of class name
  //        copy() : SELF_TYPE  returns a copy of the object
  //
  // There is no need for method bodies in the basic classes---these
  // are already built in to the runtime system.

  Class_ Object_class = class_(
      Object, No_class,
      append_Features(
          append_Features(single_Features(method(cool_abort, nil_Formals(),
                                                 Object, no_expr())),
                          single_Features(method(type_name, nil_Formals(), Str,
                                                 no_expr()))),
          single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
      filename);

  //
  // The IO class inherits from Object. Its methods are
  //        out_string(Str) : SELF_TYPE       writes a string to the output
  //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
  //        in_string() : Str                 reads a string from the input
  //        in_int() : Int                      "   an int     "  "     "
  //
  Class_ IO_class = class_(
      IO, Object,
      append_Features(
          append_Features(
              append_Features(single_Features(method(
                                  out_string, single_Formals(formal(arg, Str)),
                                  SELF_TYPE, no_expr())),
                              single_Features(method(
                                  out_int, single_Formals(formal(arg, Int)),
                                  SELF_TYPE, no_expr()))),
              single_Features(
                  method(in_string, nil_Formals(), Str, no_expr()))),
          single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
      filename);

  //
  // The Int class has no methods and only a single attribute, the
  // "val" for the integer.
  //
  Class_ Int_class = class_(
      Int, Object, single_Features(attr(val, prim_slot, no_expr())), filename);

  //
  // Bool also has only the "val" slot.
  //
  Class_ Bool_class = class_(
      Bool, Object, single_Features(attr(val, prim_slot, no_expr())), filename);

  //
  // The class Str has a number of slots and operations:
  //       val                                  the length of the string
  //       str_field                            the string itself
  //       length() : Int                       returns length of the string
  //       concat(arg: Str) : Str               performs string concatenation
  //       substr(arg: Int, arg2: Int): Str     substring selection
  //
  Class_ Str_class = class_(
      Str, Object,
      append_Features(
          append_Features(
              append_Features(
                  append_Features(
                      single_Features(attr(val, Int, no_expr())),
                      single_Features(attr(str_field, prim_slot, no_expr()))),
                  single_Features(
                      method(length, nil_Formals(), Int, no_expr()))),
              single_Features(method(concat, single_Formals(formal(arg, Str)),
                                     Str, no_expr()))),
          single_Features(
              method(substr,
                     append_Formals(single_Formals(formal(arg, Int)),
                                    single_Formals(formal(arg2, Int))),
                     Str, no_expr()))),
      filename);
  add2classmap(Object_class);
  add2classmap(Int_class);
  add2classmap(Str_class);
  add2classmap(Bool_class);
  add2classmap(IO_class);
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream &ClassTable::semant_error(Class_ c, tree_node *t) {
  return semant_error(c->get_filename(), t);
}

ostream &ClassTable::semant_error(Symbol filename, tree_node *t) {
  error_stream << filename << ":" << t->get_line_number() << ": ";
  return semant_error();
}

ostream &ClassTable::semant_error() {
  semant_errors++;
  return error_stream;
}

/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant() {
  initialize_constants();

  /* ClassTable constructor may do some semantic analysis */
  ClassTable *classtable = new ClassTable(classes);

  /* some semantic analysis code may go here */
  if (!classtable->errors()) {
    classtable->check_valid();
  }
  if (!classtable->errors()) {
    Env env(classtable);
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
      env.vars->enterscope();
      env.cur_class = classes->nth(i);
      classes->nth(i)->init_env(env);
      classes->nth(i)->type_check(env);
      env.vars->exitscope();
    }
  }
  if (classtable->errors()) {
    cerr << "Compilation halted due to static semantic errors." << endl;
    exit(1);
  }
}
