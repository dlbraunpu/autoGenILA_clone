module word_adder(clk, rst, func, inWord, result);
  wire [8:0] _0_;
  wire _1_;
  input clk;
  wire enable;
  input [1:0] func;
  input [8:0] inWord;
  output [8:0] result;
  input rst;
  reg [8:0] word;
  wire [8:0] word_next;
  assign _1_ = func == 1'h1;
  assign enable = func == 2'h2;
  always @(posedge clk)
      word <= _0_;
  assign _0_ = rst ? 9'h000 : word_next;
  assign word_next = _1_ ? inWord : word;
  adder u0 (
    .clk(clk),
    .enable(enable),
    .result(result),
    .rst(rst),
    .word(word)
  );
endmodule
