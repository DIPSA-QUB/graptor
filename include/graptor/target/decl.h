// -*- c++ -*-
#ifndef GRAPTOR_TARGET_DECL_H
#define GRAPTOR_TARGET_DECL_H

#include <x86intrin.h>
#include <immintrin.h>

namespace target {

/***********************************************************************
 * Tag types to overload functions that return a mask of some sort.
 ***********************************************************************/
struct mt_bool { };
struct mt_mask { };
struct mt_vmask { };

/***********************************************************************
 * Determining the appropriate mask type as a function of vector length
 ***********************************************************************/
// TODO: Used only in combine_mask utility below; try to replace with
// typename bitmask_traits<VL>::type. Current version does not support
// recursively composed bitmasks (non-power-of-2 vector lengths).
template<unsigned short VL>
struct select_mask_type {
    using type = longint<VL/8>;
};

template<>
struct select_mask_type<1> {
    using type = unsigned char;
};

template<>
struct select_mask_type<2> {
    using type = unsigned char;
};

template<>
struct select_mask_type<4> {
    using type = unsigned char;
};

template<>
struct select_mask_type<8> {
    using type = __mmask8;
};

template<>
struct select_mask_type<16> {
    using type = __mmask16;
};

template<>
struct select_mask_type<32> {
    using type = __mmask32;
};

template<>
struct select_mask_type<64> {
    using type = __mmask64;
};

template<unsigned short VL>
using mask_type_t = typename select_mask_type<VL>::type;
}

/***********************************************************************
 * Vector traits
 ***********************************************************************/
template<typename T, unsigned short nbytes, typename = void>
struct vector_type_traits;

template<typename T, typename V>
struct vector_type_traits_of;

template<typename VT, unsigned short VL>
struct vector_type_traits_with;

template<typename T, unsigned short VL, typename Enable = void>
struct vector_type_traits_vl;

#include "graptor/longint.h"
#include "graptor/target/bitmask.h"

namespace target {

/***********************************************************************
 * Mask utility
 ***********************************************************************/
template<unsigned short VL1, unsigned short VL2>
inline mask_type_t<VL1+VL2> combine_mask(
    mask_type_t<VL1> lo, mask_type_t<VL2> hi ) {
    if constexpr ( is_longint_v<mask_type_t<VL1+VL2>> ) {
	using traits =
	    vector_type_traits<uint64_t,mask_type_t<VL1+VL2>::W>;
	if constexpr ( VL1 == VL2 && VL1 <= 64 ) {
	    mask_type_t<VL1+VL2> wc( traits::set_pair( hi, lo ) );
	    return wc;
	} else if constexpr ( VL1 == VL2 ) {
	    mask_type_t<VL1+VL2> wc( traits::set_pair( hi.get(), lo.get() ) );
	    return wc;
	} else {
	    mask_type_t<VL1+VL2> wlo( lo );
	    mask_type_t<VL1+VL2> whi( hi );
	    mask_type_t<VL1+VL2> wc(
		traits::bitwise_or(
		    traits::template bslli<VL1/8>( whi.get() ), wlo.get() ) );
	    return wc;
	}
    } else {
	// mask_type_t<VL1+VL2> wlo( lo );
	// mask_type_t<VL1+VL2> whi( hi );
	// return (whi << VL1) | wlo;
	using traits = mask_type_traits<VL1+VL2>;
	return traits::set_pair( hi, lo );
    }
}


/***********************************************************************
 * Entry point to type traits for mask operations
 ***********************************************************************/

/***********************************************************************
 * Entry point to type traits for vector operations
 ***********************************************************************/
#if __AVX2__
template<typename T = uint32_t>
struct avx2_4x8;

template<typename T = uint64_t>
struct avx2_8x4;
#endif // __AVX2__


} // namespace target

#endif // GRAPTOR_TARGET_DECL_H
