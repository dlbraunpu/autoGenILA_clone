; SMT-LIBv2 description generated by Yosys 0.8 (git sha1 UNKNOWN, clang 9.0.0 -fPIC -Os)
; yosys-smt2-stdt
; yosys-smt2-module mult
(declare-datatype |mult_s| ((|mult_mk|
  (|mult_is| Bool)
  (|mult#0| Bool) ; \clk
  (|mult#1| (_ BitVec 2)) ; \func
  (|mult#2| (_ BitVec 9)) ; \multiplicand
  (|mult#3| (_ BitVec 9)) ; \multiplier
  (|mult#5| (_ BitVec 9)) ; \remain
  (|mult#11| (_ BitVec 9)) ; \num
  (|mult#17| (_ BitVec 9)) ; \result
  (|mult#19| Bool) ; \rstn
)))
; yosys-smt2-input clk 1
; yosys-smt2-wire clk 1
; yosys-smt2-clock clk posedge
(define-fun |mult_n clk| ((state |mult_s|)) Bool (|mult#0| state))
; yosys-smt2-input func 2
; yosys-smt2-wire func 2
(define-fun |mult_n func| ((state |mult_s|)) (_ BitVec 2) (|mult#1| state))
; yosys-smt2-register multiplicand 9
; yosys-smt2-wire multiplicand 9
(define-fun |mult_n multiplicand| ((state |mult_s|)) (_ BitVec 9) (|mult#2| state))
; yosys-smt2-wire multiplicand_next 9
(define-fun |mult#4| ((state |mult_s|)) (_ BitVec 9) (bvadd (|mult#2| state) (|mult#3| state))) ; $add$mult.v:16$3_Y
(define-fun |mult#6| ((state |mult_s|)) Bool (bvugt (|mult#5| state) #b000000000)) ; $gt$mult.v:16$2_Y
(define-fun |mult#7| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#6| state) (|mult#4| state) (|mult#2| state))) ; $ternary$mult.v:16$4_Y
(define-fun |mult#8| ((state |mult_s|)) Bool (= (|mult#1| state) #b01)) ; $eq$mult.v:15$1_Y
(define-fun |mult#9| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#8| state) (|mult#2| state) (|mult#7| state))) ; \multiplicand_next
(define-fun |mult_n multiplicand_next| ((state |mult_s|)) (_ BitVec 9) (|mult#9| state))
; yosys-smt2-register multiplier 9
; yosys-smt2-wire multiplier 9
(define-fun |mult_n multiplier| ((state |mult_s|)) (_ BitVec 9) (|mult#3| state))
; yosys-smt2-wire multiplier_next 9
(define-fun |mult#10| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#8| state) #b000000000 (|mult#2| state))) ; \multiplier_next
(define-fun |mult_n multiplier_next| ((state |mult_s|)) (_ BitVec 9) (|mult#10| state))
; yosys-smt2-input num 9
; yosys-smt2-wire num 9
(define-fun |mult_n num| ((state |mult_s|)) (_ BitVec 9) (|mult#11| state))
; yosys-smt2-register remain 9
; yosys-smt2-wire remain 9
(define-fun |mult_n remain| ((state |mult_s|)) (_ BitVec 9) (|mult#5| state))
; yosys-smt2-wire remain_next 9
(define-fun |mult#12| ((state |mult_s|)) (_ BitVec 9) (bvsub (|mult#5| state) #b000000001)) ; $auto$wreduce.cc:347:run$36 [8:0]
(define-fun |mult#13| ((state |mult_s|)) Bool (distinct (|mult#5| state) #b000000000)) ; $reduce_bool$mult.v:19$8_Y
(define-fun |mult#14| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#13| state) (|mult#12| state) (|mult#5| state))) ; $ternary$mult.v:19$10_Y [8:0]
(define-fun |mult#15| ((state |mult_s|)) Bool (= (|mult#1| state) #b10)) ; $eq$mult.v:18$6_Y
(define-fun |mult#16| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#15| state) (|mult#11| state) (|mult#14| state))) ; \remain_next
(define-fun |mult_n remain_next| ((state |mult_s|)) (_ BitVec 9) (|mult#16| state))
; yosys-smt2-output result 9
; yosys-smt2-register result 9
; yosys-smt2-wire result 9
(define-fun |mult_n result| ((state |mult_s|)) (_ BitVec 9) (|mult#17| state))
; yosys-smt2-wire result_next 9
(define-fun |mult#18| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#8| state) (|mult#2| state) (|mult#17| state))) ; \result_next
(define-fun |mult_n result_next| ((state |mult_s|)) (_ BitVec 9) (|mult#18| state))
; yosys-smt2-input rstn 1
; yosys-smt2-wire rstn 1
(define-fun |mult_n rstn| ((state |mult_s|)) Bool (|mult#19| state))
(define-fun |mult#20| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#19| state) (|mult#18| state) #b000000000)) ; $0\result[8:0]
(define-fun |mult#21| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#19| state) (|mult#16| state) #b000000000)) ; $0\remain[8:0]
(define-fun |mult#22| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#19| state) (|mult#10| state) #b000000000)) ; $0\multiplier[8:0]
(define-fun |mult#23| ((state |mult_s|)) (_ BitVec 9) (ite (|mult#19| state) (|mult#9| state) #b000000001)) ; $0\multiplicand[8:0]
(define-fun |mult_a| ((state |mult_s|)) Bool true)
(define-fun |mult_u| ((state |mult_s|)) Bool true)
(define-fun |mult_i| ((state |mult_s|)) Bool true)
(define-fun |mult_h| ((state |mult_s|)) Bool true)
(define-fun |mult_t| ((state |mult_s|) (next_state |mult_s|)) Bool (and
  (= (|mult#20| state) (|mult#17| next_state)) ; $procdff$32 \result
  (= (|mult#21| state) (|mult#5| next_state)) ; $procdff$33 \remain
  (= (|mult#22| state) (|mult#3| next_state)) ; $procdff$34 \multiplier
  (= (|mult#23| state) (|mult#2| next_state)) ; $procdff$35 \multiplicand
)) ; end of module mult
; yosys-smt2-topmod mult
; end of yosys output
