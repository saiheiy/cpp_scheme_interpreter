# cpp_scheme_interpreter

minimal Scheme lisp interpreter.  Does not support lambda functions yet, feel free to improve!

To build (on linux):
g++ -std=c++11 ./lispc.cpp

To run:
./a.out
>  (+ 1 1)
2
>  (- (* 5 3) (+ 1 1 1))  
12
>  (define x 3)
nil
>  (* x 6)        
18
