#pragma once

#include <map>
#include <string>

#include "mblas/matrix.h"
#include "npz_converter.h"

struct Weights {
  
  //////////////////////////////////////////////////////////////////////////////
  
  struct EncEmbeddings {
    EncEmbeddings(const NpzConverter& model)
    : E_(model["W_0_enc_approx_embdr"]),
      EB_(model("b_0_enc_approx_embdr", true))
    {}
    
    const mblas::Matrix E_;
    const mblas::Matrix EB_;
  };
  
  struct EncForwardRnn {
    EncForwardRnn(const NpzConverter& model) 
    : W_(model["W_0_enc_input_embdr_0"]),  
      B_(model("b_0_enc_input_embdr_0", true)),
      U_(model["W_enc_transition_0"]),
      Wz_(model["W_0_enc_update_embdr_0"]),
      Uz_(model["G_enc_transition_0"]),
      Wr_(model["W_0_enc_reset_embdr_0"]),
      Ur_(model["R_enc_transition_0"]) 
    {}
    
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix Wz_;
    const mblas::Matrix Uz_;
    const mblas::Matrix Wr_;
    const mblas::Matrix Ur_;
  };
  
  struct EncBackwardRnn {
    EncBackwardRnn(const NpzConverter& model)     
    : W_(model["W_0_back_enc_input_embdr_0"]),  
      B_(model("b_0_back_enc_input_embdr_0", true)),
      U_(model["W_back_enc_transition_0"]),
      Wz_(model["W_0_back_enc_update_embdr_0"]),
      Uz_(model["G_back_enc_transition_0"]),
      Wr_(model["W_0_back_enc_reset_embdr_0"]),
      Ur_(model["R_back_enc_transition_0"]) 
    {}
          
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix Wz_;
    const mblas::Matrix Uz_;
    const mblas::Matrix Wr_;
    const mblas::Matrix Ur_;
  };
  
  //////////////////////////////////////////////////////////////////////////////
  
  struct DecEmbeddings {
    DecEmbeddings(const NpzConverter& model)
    : E_(model["W_0_dec_approx_embdr"]),
      EB_(model("b_0_dec_approx_embdr", true))
    {}
    
    const mblas::Matrix E_;
    const mblas::Matrix EB_;
  };

  struct DecRnn {
    DecRnn(const NpzConverter& model)
    : Ws_(model["W_0_dec_initializer_0"]),
      WsB_(model("b_0_dec_initializer_0", true)),
  
      W_(model["W_0_dec_input_embdr_0"]),
      B_(model("b_0_dec_input_embdr_0", true)),
      U_(model["W_dec_transition_0"]),
      C_(model["W_0_dec_dec_inputter_0"]),
  
      Wz_(model["W_0_dec_update_embdr_0"]),
      Uz_(model["G_dec_transition_0"]),
      Cz_(model["W_0_dec_dec_updater_0"]),
  
      Wr_(model["W_0_dec_reset_embdr_0"]),
      Ur_(model["R_dec_transition_0"]),
      Cr_(model["W_0_dec_dec_reseter_0"])
    {}
          
    const mblas::Matrix Ws_;
    const mblas::Matrix WsB_;
    const mblas::Matrix W_;
    const mblas::Matrix B_;
    const mblas::Matrix U_;
    const mblas::Matrix C_;
    const mblas::Matrix Wz_;
    const mblas::Matrix Uz_;
    const mblas::Matrix Cz_;
    const mblas::Matrix Wr_;
    const mblas::Matrix Ur_;
    const mblas::Matrix Cr_;
  };
  
  struct DecAlignment {
    DecAlignment(const NpzConverter& model)
    : Va_(model("D_dec_transition_0", true)),
      Wa_(model["B_dec_transition_0"]),
      Ua_(model["A_dec_transition_0"])
    {}
          
    const mblas::Matrix Va_;
    const mblas::Matrix Wa_;
    const mblas::Matrix Ua_;
  };
  
  struct DecSoftmax {
    DecSoftmax(const NpzConverter& model)
    : WoB_(model("b_dec_deep_softmax", true)),
      Uo_(model["W_0_dec_hid_readout_0"]),
      UoB_(model("b_0_dec_hid_readout_0", true)),
      Vo_(model["W_0_dec_prev_readout_0"]),
      Co_(model["W_0_dec_repr_readout"])
    {
      const mblas::Matrix Wo1_(model["W1_dec_deep_softmax"]);
      const mblas::Matrix Wo2_(model["W2_dec_deep_softmax"]);
      mblas::Prod(const_cast<mblas::Matrix&>(Wo_), Wo1_, Wo2_);
    }
          
    const mblas::Matrix Wo_;
    const mblas::Matrix WoB_;
    const mblas::Matrix Uo_;
    const mblas::Matrix UoB_;
    const mblas::Matrix Vo_;
    const mblas::Matrix Co_;
  };
  
  Weights(const std::string& npzFile, size_t device = 0)
  : Weights(NpzConverter(npzFile), device)
  {}
  
  Weights(const NpzConverter& model, size_t device = 0)
  : encEmbeddings_(model),
    decEmbeddings_(model),
    encForwardRnn_(model),
    encBackwardRnn_(model),
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
