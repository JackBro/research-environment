
// F# Visual Studio Sample File
//
// This file contains some sample constructs to guide you through the
// primitives of F#.
//
// Contents:
//   - Simple computations
//   - Functions on integers.  
//   - Tuples 
//   - Strings
//   - Lists
//   - Arrays
//   - Functions

// Simple computations
// ---------------------------------------------------------------
// Here is a simple computation.  Note how code can be documented
// with '///' comments.  You can use the extra --html-* command line
// options to generate HTML documentation directly from these comments.

/// The position of 'c' in a sample string
let posc = String.index "abcdef" 'c'

  // Try re-typing the above line to see intellisense in action.
  // Try ctrl-J on (partial) identifiers.

// Simple arithmetic:
let xA = 1
let xB = 2
let xC = xA + xB
let twist x = 10 - x
do  Printf.printf "res = %d\n" (twist (xC+4))
  // note: let binds values and functions
  // note: do <expr> evaluates expression 


// Functions on integers.  
// ---------------------------------------------------------------

let inc x = x + 1
let rec fact n = if n=0 then 1 else n * fact (n-1)

/// Highest-common-factor 
let rec hcf a b =           // notice: 2 arguments seperated by spaces
  if a=0 then
    b
  else
    if a<b then
      hcf a (b-a)           // notice: 2 arguments seperated by spaces
    else
      hcf (a-b) b
  // note: function arguments are usually space seperated.
  // note: let rec binds recursive functions.

      
// Tuples.  These combine values into packets.
// ---------------------------------------------------------------
let pA = (1,2,3)
let pB = (1,"fred",3.1415)
let swap (a,b) = (b,a)

/// Note: hcf2 takes one argument which is in fact a tuple.
let rec hcf2 (a,b) = 
  if a = 0 then b
  else if a<b then hcf2 (a,b-a)
  else hcf2 (a-b,a)


// Booleans.
// ---------------------------------------------------------------

let bA = true
let bB = false
let bC = not bA && (bB || false)


// Strings.
// ---------------------------------------------------------------

let sA  = "hello"
let sB  = "world"
let sC  = sA + " " + sB
let sC2 = String.concat " " [sA;sB]
do  Printf.printf "sC = %s, sC2 = %s\n" sC sC2
let s3 = Printf.sprintf "sC = %s, sC2 = %d\n" sC sC2.Length


// Functional Lists.
// ---------------------------------------------------------------

let xsA = [ ]           // empty list
let xsB = [ 1;2;3 ]     // list of 3 ints
let xsC = 1 :: [2;3]    // :: is cons operation.

do print_any xsC

let rec sumList xs =
  match xs with
  | []    -> 0
  | y::ys -> y + sumList ys
	
let y   = sumList [1;2;3]  // sum a list
let xsD = xsA @ [1;2;3]    // append
let xsE = 99 :: xsD        // cons on front


// Mutable Arrays, a primitive for efficient computations
// ---------------------------------------------------------------

let arr = Array.create 4 "hello"
do  arr.(1) <- "world"
do  arr.(3) <- "don"
let nA = Array.length arr  // function in Array module/class
let nB = arr.Length        // instance method on array object

let front = Array.sub arr 0 2
  // Trying re-typing the above line to see intellisense in action.
  // Note, ctrl-J on (partial) identifiers re-activates it.

// Other common data structures
// ---------------------------------------------------------------

// See namespaces 
//   System.Collections.Generic
//   Microsoft.FSharp.Collections
//   Hashtbl
//   Set
//   Map


// Functions
// ---------------------------------------------------------------

let inc2 x = x + 2              // as a function definition
let inc3   = fun x -> x + 3     // as a lambda expression

let ysA = List.map inc2 [1;2;3]
let ysB = List.map inc3 [1;2;3]
let ysC = List.map (fun x -> x+4) [1;2;3]

// Pipelines:
let pipe1 = [1;2;3] |> List.map (fun x -> x+4) 
let pipe2 = 
  [1;2;3] 
  |> List.map (fun x -> x+4) 
  |> List.filter (fun x -> x>5) 

// Composition pipelines:
let processor = List.map (fun x -> x+4) >> List.filter (fun x -> x>5) 

// Types - datatypes
// ---------------------------------------------------------------

type expr = 
  | Num of int
  | Add of expr * expr
  | Sub of expr * expr
  | Mul of expr * expr
  | Div of expr * expr
  | Var of string
  
let lookup id (env : (string * int) list) = List.assoc id env
  
let rec eval env exp = match exp with
  | Num n -> n
  | Add (x,y) -> eval env x + eval env y
  | Sub (x,y) -> eval env x - eval env y
  | Mul (x,y) -> eval env x * eval env y
  | Div (x,y) -> eval env x / eval env y
  | Var id    -> lookup id env
  
let envA = [ "a",1 ;
             "b",2 ;
             "c",3 ]
             
let expT1 = Add(Var "a",Mul(Num 2,Var "b"))
let resT1 = eval envA expT1


// Types - records
// ---------------------------------------------------------------

type card = { name  : string;
              phone : string;
              ok    : bool }
              
let cardA = { name = "Alf" ; phone = "+44.1223.000.000" ; ok = false }
let cardB = {cardA with phone = "+44.1223.123.456"; ok = true }
let string_of_card c = 
  c.name + " phone: " + c.phone + (if not c.ok then " (unchecked)" else "")


// Here's a longer construction syntax should you get name conflicts:
let cardC = {  new card 
               with name  = "Alf" 
               and  phone = "+44.1223.000.000" 
               and  ok = false }



// Types - interfaces, which are like records of functions
// ---------------------------------------------------------------

type IPeekPoke = interface
  abstract member Peek: unit -> int
  abstract member Poke: int -> unit
end
              

// Types - classes
// ---------------------------------------------------------------

type widget = class
  val mutable state: int 
  member x.Poke(n) = x.state <- x.state + n
  member x.Peek() = x.state 
  member x.HasBeenPoked = (x.state <> 0)
  new() = { state = 0 }
end
              
// Types - classes with interface implementations
// ---------------------------------------------------------------

type wodget = class
  val mutable state: int 
  interface IPeekPoke with 
    member x.Poke(n) = x.state <- x.state + n
    member x.Peek() = x.state 
  end
  member x.HasBeenPoked = (x.state <> 0)
  new() = { state = 0 }
end
              
