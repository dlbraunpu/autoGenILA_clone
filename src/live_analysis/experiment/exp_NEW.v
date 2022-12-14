module exp(clk, valid, in, out);
  (* src = "experiment.v:13" *)
  wire [4:0] _00_;
  (* src = "experiment.v:13" *)
  wire [4:0] _01_;
  (* src = "experiment.v:13" *)
  wire [4:0] _02_;
  (* src = "experiment.v:13" *)
  wire [4:0] _03_;
  (* src = "experiment.v:13" *)
  wire [4:0] _04_;
  wire [8:0] _05_;
  wire [5:0] _06_;
  wire [6:0] _07_;
  wire [4:0] _08_;
  (* src = "experiment.v:21" *)
  wire [31:0] _09_;
  (* src = "experiment.v:21" *)
  (* unused_bits = "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31" *)
  wire [31:0] _10_;
  (* src = "experiment.v:2" *)
  input clk;
  (* src = "experiment.v:3" *)
  input [4:0] in;
  (* src = "experiment.v:5" *)
  output [4:0] out;
  (* src = "experiment.v:7" *)
  reg [4:0] reg1;
  (* src = "experiment.v:8" *)
  reg [4:0] reg2;
  (* src = "experiment.v:9" *)
  reg [4:0] reg3;
  (* src = "experiment.v:10" *)
  reg [4:0] reg4;
  (* src = "experiment.v:11" *)
  reg [4:0] reg5;
  (* src = "experiment.v:4" *)
  input valid;
  assign _05_ = { _09_[7:5], _08_ } + (* src = "experiment.v:21" *) reg5;
  assign _06_ = reg1 + (* src = "experiment.v:21" *) reg2;
  assign _07_ = _06_ + (* src = "experiment.v:21" *) reg3;
  assign { _09_[7:5], _08_ } = _07_ + (* src = "experiment.v:21" *) reg4;
  assign { _10_[31:5], out } = { 23'b00000000000000000000000, _05_ } / (* src = "experiment.v:21" *) 32'd5;
  always @(posedge clk)
      reg1 <= _00_;
  always @(posedge clk)
      reg2 <= _01_;
  always @(posedge clk)
      reg3 <= _02_;
  always @(posedge clk)
      reg4 <= _03_;
  always @(posedge clk)
      reg5 <= _04_;
  assign _00_ = valid ? (* src = "experiment.v:14" *) in : reg1;
  assign _01_ = valid ? (* src = "experiment.v:15" *) reg1 : reg2;
  assign _02_ = valid ? (* src = "experiment.v:16" *) reg2 : reg3;
  assign _03_ = valid ? (* src = "experiment.v:17" *) reg3 : reg4;
  assign _04_ = valid ? (* src = "experiment.v:18" *) reg4 : reg5;
endmodule
