module NV_NVDLA_SDP_MRDMA_EG_pfifo_flopram_rwsa_3x128(clk, pwrbus_ram_pd, di, we, wa, ra, dout);
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2668" *)
  wire [127:0] _00_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2668" *)
  wire [127:0] _01_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2668" *)
  wire [127:0] _02_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2669" *)
  wire _03_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2672" *)
  wire _04_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2675" *)
  wire _05_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2669" *)
  wire _06_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2672" *)
  wire _07_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2675" *)
  wire _08_;
  wire _09_;
  wire _10_;
  wire _11_;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2604" *)
  input clk;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2606" *)
  input [127:0] di;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2610" *)
  output [127:0] dout;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2605" *)
  input [31:0] pwrbus_ram_pd;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2609" *)
  input [1:0] ra;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2665" *)
  reg [127:0] ram_ff0;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2666" *)
  reg [127:0] ram_ff1;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2667" *)
  reg [127:0] ram_ff2;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2608" *)
  input [1:0] wa;
  (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2607" *)
  input we;
  assign _03_ = ! (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2669" *) wa;
  assign _04_ = wa == (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2672" *) 1'b1;
  assign _05_ = wa == (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2675" *) 2'b10;
  assign _06_ = we && (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2669" *) _03_;
  assign _07_ = we && (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2672" *) _04_;
  assign _08_ = we && (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2675" *) _05_;
  always @(posedge clk)
      ram_ff0 <= _00_;
  always @(posedge clk)
      ram_ff1 <= _01_;
  always @(posedge clk)
      ram_ff2 <= _02_;
  function [127:0] _21_;
    input [127:0] a;
    input [383:0] b;
    input [2:0] s;
    (* full_case = 32'd1 *)
    (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2685|./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2681" *)
    (* parallel_case *)
    casez (s)
      3'b??1:
        _21_ = b[127:0];
      3'b?1?:
        _21_ = b[255:128];
      3'b1??:
        _21_ = b[383:256];
      default:
        _21_ = a;
    endcase
  endfunction
  assign dout = _21_(ram_ff0, { ram_ff1, ram_ff2, di }, { _11_, _10_, _09_ });
  assign _09_ = ra == (* full_case = 32'd1 *) (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2685|./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2681" *) 2'b11;
  assign _10_ = ra == (* full_case = 32'd1 *) (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2684|./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2681" *) 2'b10;
  assign _11_ = ra == (* full_case = 32'd1 *) (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2683|./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2681" *) 1'b1;
  assign _02_ = _08_ ? (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2675" *) di : ram_ff2;
  assign _01_ = _07_ ? (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2672" *) di : ram_ff1;
  assign _00_ = _06_ ? (* src = "./vmod/nvdla/sdp/NV_NVDLA_SDP_MRDMA_EG_din.v:2669" *) di : ram_ff0;
endmodule