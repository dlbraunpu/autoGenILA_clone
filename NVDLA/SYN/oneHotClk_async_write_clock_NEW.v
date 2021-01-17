module oneHotClk_async_write_clock(enable_w);
  (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:25" *)
  wire _0_;
  (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:13" *)
  output enable_w;
  (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:20" *)
  wire one_hot_enable;
  (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:21" *)
  wire tp;
  assign _0_ = ! (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:25" *) one_hot_enable;
  assign enable_w = _0_ || (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:25" *) tp;
  (* module_not_derived = 32'd1 *)
  (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:23" *)
  NV_BLKBOX_SRC0 UJ_dft_xclamp_ctrl_asyncfifo_onehotclk_write (
    .Y(one_hot_enable)
  );
  (* module_not_derived = 32'd1 *)
  (* src = "./vmod/vlibs/oneHotClk_async_write_clock.v:24" *)
  NV_BLKBOX_SRC0 UJ_dft_xclamp_scan_asyncfifo_onehotclk_write (
    .Y(tp)
  );
endmodule