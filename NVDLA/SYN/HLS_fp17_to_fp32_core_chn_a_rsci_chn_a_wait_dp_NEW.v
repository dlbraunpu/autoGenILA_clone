module HLS_fp17_to_fp32_core_chn_a_rsci_chn_a_wait_dp(nvdla_core_clk, nvdla_core_rstn, chn_a_rsci_oswt, chn_a_rsci_bawt, chn_a_rsci_wen_comp, chn_a_rsci_d_mxwt, chn_a_rsci_biwt, chn_a_rsci_bdwt, chn_a_rsci_d);
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:245" *)
  wire _00_;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:243" *)
  wire _01_;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:251" *)
  wire _02_;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:251" *)
  wire _03_;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:232" *)
  output chn_a_rsci_bawt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:239" *)
  reg chn_a_rsci_bcwt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:236" *)
  input chn_a_rsci_bdwt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:235" *)
  input chn_a_rsci_biwt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:237" *)
  input [16:0] chn_a_rsci_d;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:240" *)
  reg [16:0] chn_a_rsci_d_bfwt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:234" *)
  output [16:0] chn_a_rsci_d_mxwt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:231" *)
  input chn_a_rsci_oswt;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:233" *)
  output chn_a_rsci_wen_comp;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:229" *)
  input nvdla_core_clk;
  (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:230" *)
  input nvdla_core_rstn;
  assign _01_ = ~ (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:243" *) chn_a_rsci_oswt;
  assign _02_ = ~ (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:251" *) chn_a_rsci_bawt;
  assign _00_ = ~ (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:251" *) _03_;
  assign chn_a_rsci_bawt = chn_a_rsci_biwt | (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:242" *) chn_a_rsci_bcwt;
  assign chn_a_rsci_wen_comp = _01_ | (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:243" *) chn_a_rsci_bawt;
  assign _03_ = _02_ | (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:251" *) chn_a_rsci_bdwt;
  always @(posedge nvdla_core_clk or negedge nvdla_core_rstn)
    if (!nvdla_core_rstn)
      chn_a_rsci_bcwt <= 1'b0;
    else
      chn_a_rsci_bcwt <= _00_;
  always @(posedge nvdla_core_clk or negedge nvdla_core_rstn)
    if (!nvdla_core_rstn)
      chn_a_rsci_d_bfwt <= 17'b00000000000000000;
    else
      chn_a_rsci_d_bfwt <= chn_a_rsci_d_mxwt;
  assign chn_a_rsci_d_mxwt = chn_a_rsci_bcwt ? (* full_case = 32'd1 *) (* src = "./vmod/vlibs/HLS_fp17_to_fp32.v:262|./vmod/vlibs/HLS_fp17_to_fp32.v:261" *) chn_a_rsci_d_bfwt : chn_a_rsci_d;
endmodule