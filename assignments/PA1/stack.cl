class Stack {
   isEmpty() : Bool { true };
   head() : String { { abort(); ""; } };
   tail() : Stack { { abort(); self; } };
   push(i : String) : Stack {
      (new Cons).init(i, self)
   };
};

class Cons inherits Stack {
   car : String;	
   cdr : Stack;	
   isEmpty() : Bool { false };
   head() : String { car };
   tail() : Stack { cdr };
   init(i : String, rest : Stack) : Stack {
      {
         car <- i;
         cdr <- rest;
         self;
      }
   };
};

class DCommand {
   io : IO <- new IO;
   display(s : Stack) : Object {
      if s.isEmpty()
      then
         io.out_string("")
      else
      {
         io.out_string(s.head());
         io.out_string("\n");
         display(s.tail());
      }
      fi
   };
};

class ECommand {
   a : String;
   b : String;
   conversion : A2I <- new A2I;

   operate(s : Stack) : Stack {
      if s.isEmpty()  -- first check isEmpty?
      then
      {
         s;
      }
      else if s.head() = "+"
      then
      {
         s <- s.tail(); -- pop +
         a <- s.head(); -- get a
         s <- s.tail(); -- pop a
         b <- s.head(); -- get b
         s <- s.tail(); -- pop b 
         s <- s.push(conversion.i2a(conversion.a2i(a) + conversion.a2i(b))); -- push a+b
      }
      else if s.head() = "s"
      then
      {
         s <- s.tail(); -- pop s
         a <- s.head(); -- get a
         s <- s.tail(); -- pop a
         b <- s.head(); -- get b
         s <- s.tail(); -- pop b 
         s <- s.push(a);
         s <- s.push(b);
      }
      else  -- is a interger
      {
         s;
      }
      fi
      fi
      fi
   };
};

class Main inherits IO {
   flag : Bool <- true;
   input : String;
   stack : Stack <- new Stack;
   ecom : ECommand <- new ECommand;
   dcom : DCommand <- new DCommand;

   main() : Object {
      {
         while flag
         loop
            {
               out_string(">");
               input <-in_string();
               if input = "x" 
               then
                  flag <- false
               else
               {
                  if input = "d"
                  then
                     dcom.display(stack)
                  else if input = "e"
                  then 
                     stack <- ecom.operate(stack)
                  else 
                    stack <- stack.push(input)
                  fi
                  fi;
               }
               fi;
            }
         pool;
      }
   };
};
