module RAMDP_256X7_GL_M2_E2(CLK_R, CLK_W, RE, WE, RADR_7, RADR_6, RADR_5, RADR_4, RADR_3, RADR_2, RADR_1, RADR_0, WADR_7, WADR_6, WADR_5, WADR_4, WADR_3, WADR_2, WADR_1, WADR_0, WD_6, WD_5, WD_4, WD_3, WD_2, WD_1, WD_0, RD_6, RD_5, RD_4, RD_3, RD_2, RD_1, RD_0, IDDQ, SVOP_1, SVOP_0, SLEEP_EN_7, SLEEP_EN_6, SLEEP_EN_5, SLEEP_EN_4, SLEEP_EN_3, SLEEP_EN_2, SLEEP_EN_1, SLEEP_EN_0, RET_EN);
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input CLK_R;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input CLK_W;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input IDDQ;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:41" *)
  wire [7:0] RA;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_0;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_1;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_2;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_3;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_4;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_5;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_6;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RADR_7;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:44" *)
  wire [6:0] RD;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_0;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_1;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_2;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_3;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_4;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_5;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:38" *)
  output RD_6;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RE;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input RET_EN;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:47" *)
  wire [7:0] SLEEP_EN;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_0;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_1;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_2;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_3;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_4;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_5;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_6;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SLEEP_EN_7;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:46" *)
  wire [1:0] SVOP;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SVOP_0;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input SVOP_1;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:42" *)
  wire [7:0] WA;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_0;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_1;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_2;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_3;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_4;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_5;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_6;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WADR_7;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:43" *)
  wire [6:0] WD;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_0;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_1;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_2;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_3;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_4;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_5;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WD_6;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:31" *)
  input WE;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:625" *)
  wire clobber_array;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:626" *)
  wire clobber_flops;
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:623" *)
  wire clobber_x;
  (* module_not_derived = 32'd1 *)
  (* src = "./vmod/rams/model/RAMDP_256X7_GL_M2_E2.v:629" *)
  RAM_BANK_RAMDP_256X7_GL_M2_E2 ITOP (
    .CLK_R(CLK_R),
    .CLK_W(CLK_W),
    .IDDQ(IDDQ),
    .RA({ RADR_7, RADR_6, RADR_5, RADR_4, RADR_3, RADR_2, RADR_1, RADR_0 }),
    .RD(RD),
    .RE(RE),
    .RET_EN(RET_EN),
    .SLEEP_EN({ SLEEP_EN_7, SLEEP_EN_6, SLEEP_EN_5, SLEEP_EN_4, SLEEP_EN_3, SLEEP_EN_2, SLEEP_EN_1, SLEEP_EN_0 }),
    .SVOP({ SVOP_1, SVOP_0 }),
    .WA({ WADR_7, WADR_6, WADR_5, WADR_4, WADR_3, WADR_2, WADR_1, WADR_0 }),
    .WD({ WD_6, WD_5, WD_4, WD_3, WD_2, WD_1, WD_0 }),
    .WE(WE),
    .clobber_array(1'b0),
    .clobber_flops(1'b0),
    .clobber_x(1'b0)
  );
  assign RA = { RADR_7, RADR_6, RADR_5, RADR_4, RADR_3, RADR_2, RADR_1, RADR_0 };
  assign RD_0 = RD[0];
  assign RD_1 = RD[1];
  assign RD_2 = RD[2];
  assign RD_3 = RD[3];
  assign RD_4 = RD[4];
  assign RD_5 = RD[5];
  assign RD_6 = RD[6];
  assign SLEEP_EN = { SLEEP_EN_7, SLEEP_EN_6, SLEEP_EN_5, SLEEP_EN_4, SLEEP_EN_3, SLEEP_EN_2, SLEEP_EN_1, SLEEP_EN_0 };
  assign SVOP = { SVOP_1, SVOP_0 };
  assign WA = { WADR_7, WADR_6, WADR_5, WADR_4, WADR_3, WADR_2, WADR_1, WADR_0 };
  assign WD = { WD_6, WD_5, WD_4, WD_3, WD_2, WD_1, WD_0 };
  assign clobber_array = 1'b0;
  assign clobber_flops = 1'b0;
  assign clobber_x = 1'b0;
endmodule