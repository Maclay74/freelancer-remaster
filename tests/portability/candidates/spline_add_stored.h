#pragma once

// Investigation only: Clang Debug quiets both loaded NaNs before FADDP,
// whereas the original FADD memory operand has not been separately quieted.
inline void CandidateAddStored(float* dst, const float* a, const float* b)
{
    dst[2] = a[2] + b[2];
    dst[1] = a[1] + b[1];
    dst[0] = a[0] + b[0];
}
