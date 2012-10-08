/* IF YOUR BUILD SYSTEM PASSES -DKENLM_MAX_ORDER, THEN CHANGE THE BUILD SYSTEM.
 * If not, this is the default maximum order.  
 * Having this limit means that State can be
 * (kMaxOrder - 1) * sizeof(float) bytes instead of
 * sizeof(float*) + (kMaxOrder - 1) * sizeof(float) + malloc overhead
 */
#ifndef KENLM_MAX_ORDER
#define KENLM_MAX_ORDER 6
#endif
#ifndef KENLM_ORDER_MESSAGE
#define KENLM_ORDER_MESSAGE "Recompile with e.g. `bjam --kenlm-max-order=6 -a' to change the maximum order."
#endif
