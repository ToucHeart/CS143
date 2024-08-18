#ifndef SEMANT_H_
#define SEMANT_H_

#include "cool-tree.h"
#include "list.h"
#include "stringtab.h"
#include "symtab.h"
#include <assert.h>
#include <iostream>
#include <unordered_map>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  ostream &error_stream;
  std::unordered_map<Symbol, Class_> class_defs;
  std::unordered_map<Symbol, Symbol> class_parent;
  std::unordered_map<Class_, Symbol> class_names;
  void install_basic_classes();
  void add2classmap(Class_ c);
  void visit(Symbol s, std::unordered_map<Symbol, int> &colors);

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream &semant_error();
  ostream &semant_error(Class_ c, tree_node *t);
  ostream &semant_error(Symbol filename, tree_node *t);
  void check_valid();
  Class_ get_class(Symbol s);
  bool conform(Symbol a, Symbol b, Class_ c);
  bool exist_class(Symbol s);
  Symbol get_class_name(Class_);
  Symbol join_class(Symbol type_a, Symbol type_b, Class_ c = nullptr);
};

#endif
