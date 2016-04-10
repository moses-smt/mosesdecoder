#pragma once

#include <map>
#include <string>

#include "mblas/matrix.h"
#include "npz_converter.h"

struct Weights {
  
  //////////////////////////////////////////////////////////////////////////////
  
  struct EncEmbeddings {
    EncEmbeddings(const NpzConverter& model)
    : E_(model["Wemb"])
    {}
    
    const mblas::Matrix E_;
  };
  
  struct EncForwardRnn {
    EncForwardRnn(const NpzConverter& model) 
    : Wx_(model["encoder_Wx"]),
      W_(model["encoder_W"]),  
      U_(model["encoder_U"]),
      Ux_(model["encoder_Ux"]),
      B_(model("encoder_b", true)),
      Bx_(model("encoder_bx", true))
    { }
    
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix Wx_;
    const mblas::Matrix Bx_;
    const mblas::Matrix Ux_;
  };
  
  struct EncBackwardRnn {
    EncBackwardRnn(const NpzConverter& model) 
    : W_(model["encoder_r_W"]),  
      B_(model("encoder_r_b", true)),
      U_(model["encoder_r_U"]),
      Wx_(model["encoder_r_Wx"]),
      Bx_(model("encoder_r_bx", true)),
      Ux_(model["encoder_r_Ux"])
    {}
    
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix Wx_;
    const mblas::Matrix Bx_;
    const mblas::Matrix Ux_;
  };
  
  //////////////////////////////////////////////////////////////////////////////
  
  struct DecEmbeddings {
    DecEmbeddings(const NpzConverter& model)
    : E_(model["Wemb_dec"])
    {}
    
    const mblas::Matrix E_;
  };

  struct DecRnn {
    DecRnn(const NpzConverter& model)
    : Wi_(model["ff_state_W"]),
      Bi_(model("ff_state_b", true)),
  
      // s_m
  
      W_(model["decoder_W"]),
      B_(model("decoder_b", true)),
      U_(model["decoder_U"]),      
      Wx_(model["decoder_Wx"]),
      Bx_(model("decoder_bx", true)),
      Ux_(model["decoder_Ux"]),

      // s_i
      
      Wp_(model["decoder_Wc"]),
      Bp_(model("decoder_b_nl", true)),
      Up_(model["decoder_U_nl"]),      
      Wpx_(model["decoder_Wcx"]),
      Bpx_(model("decoder_bx_nl", true)),
      Upx_(model["decoder_Ux_nl"])
    {}
          
    const mblas::Matrix Wi_;
    const mblas::Matrix Bi_;
    
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix Wx_;
    const mblas::Matrix Bx_;
    const mblas::Matrix Ux_;

    const mblas::Matrix Wp_;
    const mblas::Matrix Bp_;
    const mblas::Matrix Up_;
    const mblas::Matrix Wpx_;
    const mblas::Matrix Bpx_;
    const mblas::Matrix Upx_;
  };
  
  struct DecAlignment {
    DecAlignment(const NpzConverter& model)
    : V_(model("decoder_U_att", true)),
      W_(model["decoder_W_comb_att"]),
      B_(model("decoder_b_att", true)),
      U_(model["decoder_Wc_att"]),
      C_(model["decoder_c_tt"]) // scalar?
    {}
          
    const mblas::Matrix V_;
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix C_;
  };
  
  struct DecSoftmax {
    DecSoftmax(const NpzConverter& model)
    : W1_(model["ff_logit_lstm_W"]),
      B1_(model("ff_logit_lstm_b", true)),
      W2_(model["ff_logit_prev_W"]),
      B2_(model("ff_logit_prev_b", true)),
      W3_(model["ff_logit_ctx_W"]),
      B3_(model("ff_logit_ctx_b", true)),
      W4_(model["ff_logit_W"]),
      B4_(model("ff_logit_b", true))
    {}
          
    const mblas::Matrix W1_;
    const mblas::Matrix B1_;
    const mblas::Matrix W2_;
    const mblas::Matrix B2_;
    const mblas::Matrix W3_;
    const mblas::Matrix B3_;
    const mblas::Matrix W4_;
    const mblas::Matrix B4_;
  };
  
  Weights(const std::string& npzFile, size_t device = 0)
  : Weights(NpzConverter(npzFile), device)
  {}
  
  Weights(const NpzConverter& model, size_t device = 0)
  : encEmbeddings_(model),
    encForwardRnn_(model),
    encBackwardRnn_(model),
    decEmbeddings_(model),
    decRnn_(model),
    decAlignment_(model),
    decSoftmax_(model),
    device_(device)
    {}
  
  size_t GetDevice() {
    return device_;
  }
  
  const EncEmbeddings encEmbeddings_;
  const DecEmbeddings decEmbeddings_;
  const EncForwardRnn encForwardRnn_;
  const EncBackwardRnn encBackwardRnn_;
  const DecRnn decRnn_;
  const DecAlignment decAlignment_;
  const DecSoftmax decSoftmax_;
  
  const size_t device_;
};
