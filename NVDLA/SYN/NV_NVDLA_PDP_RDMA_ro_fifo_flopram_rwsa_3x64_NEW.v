module NV_NVDLA_PDP_RDMA_ro_fifo_flopram_rwsa_3x64(clk, pwrbus_ram_pd, di, we, wa, ra, dout);
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3986" *)
  wire [63:0] _00_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3986" *)
  wire [63:0] _01_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3986" *)
  wire [63:0] _02_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3987" *)
  wire _03_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3990" *)
  wire _04_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3993" *)
  wire _05_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3987" *)
  wire _06_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3990" *)
  wire _07_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3993" *)
  wire _08_;
  wire _09_;
  wire _10_;
  wire _11_;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3922" *)
  input clk;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3924" *)
  input [63:0] di;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3928" *)
  output [63:0] dout;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3923" *)
  input [31:0] pwrbus_ram_pd;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3927" *)
  input [1:0] ra;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3983" *)
  reg [63:0] ram_ff0;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3984" *)
  reg [63:0] ram_ff1;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3985" *)
  reg [63:0] ram_ff2;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3926" *)
  input [1:0] wa;
  (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3925" *)
  input we;
  assign _03_ = ! (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3987" *) wa;
  assign _04_ = wa == (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3990" *) 1'b1;
  assign _05_ = wa == (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3993" *) 2'b10;
  assign _06_ = we && (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3987" *) _03_;
  assign _07_ = we && (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3990" *) _04_;
  assign _08_ = we && (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3993" *) _05_;
  always @(posedge clk)
      ram_ff0 <= _00_;
  always @(posedge clk)
      ram_ff1 <= _01_;
  always @(posedge clk)
      ram_ff2 <= _02_;
  function [63:0] _21_;
    input [63:0] a;
    input [191:0] b;
    input [2:0] s;
    (* full_case = 32'd1 *)
    (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:4003|./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3999" *)
    (* parallel_case *)
    casez (s)
      3'b??1:
        _21_ = b[63:0];
      3'b?1?:
        _21_ = b[127:64];
      3'b1??:
        _21_ = b[191:128];
      default:
        _21_ = a;
    endcase
  endfunction
  assign dout = _21_(ram_ff0, { ram_ff1, ram_ff2, di }, { _11_, _10_, _09_ });
  assign _09_ = ra == (* full_case = 32'd1 *) (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:4003|./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3999" *) 2'b11;
  assign _10_ = ra == (* full_case = 32'd1 *) (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:4002|./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3999" *) 2'b10;
  assign _11_ = ra == (* full_case = 32'd1 *) (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:4001|./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3999" *) 1'b1;
  assign _02_ = _08_ ? (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3993" *) di : ram_ff2;
  assign _01_ = _07_ ? (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3990" *) di : ram_ff1;
  assign _00_ = _06_ ? (* src = "./vmod/nvdla/pdp/NV_NVDLA_PDP_RDMA_eg.v:3987" *) di : ram_ff0;
endmodule