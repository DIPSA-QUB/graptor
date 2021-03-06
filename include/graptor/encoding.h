// -*- C++ -*-
#ifndef GRAPTOR_ENCODING_H
#define GRAPTOR_ENCODING_H

#include <math.h>

#include "graptor/mm.h"
#include "graptor/bitfield.h"
#include "graptor/target/vector.h"

alignas(64) extern const uint8_t avx2_1x32_array_encoding_permute_lut_vl4[32];
// alignas(64) extern const uint32_t avx512_4x16_evenodd_intlv_epi32_vl4[16];
alignas(64) extern const uint8_t avx512_1x32_array_encoding_permute_lut_vl8[64];
alignas(64) extern const uint8_t shuffle_encoding_bitfield_1x8_off2[16];
alignas(64) extern const uint8_t shuffle_encoding_bitfield_1x8_off1[16];

alignas(64) extern const uint8_t shuffle_encoding_bitfield_shuffle_2bx32[32];
alignas(64) extern const uint32_t shuffle_encoding_bitfield_sllv_2bx32[8];

template<typename StoredTy, typename Enable = void>
struct array_encoding {
    using stored_type = StoredTy;
    using storage_type = StoredTy;

    template<unsigned short VL_>
    using stored_traits = vector_type_traits_vl<stored_type, VL_>;

    static mmap_ptr<stored_type>
    allocate( size_t num_elems, const numa_allocation && kind ) {
	return mmap_ptr<stored_type>( num_elems, kind );
    }
    static mmap_ptr<stored_type>
    allocate( size_t num_elems, const numa_allocation & kind ) {
	return mmap_ptr<stored_type>( num_elems, kind );
    }
    static mmap_ptr<stored_type>
    allocate( numa_allocation_partitioned kind ) {
	return mmap_ptr<stored_type>( kind );
    }

    template<typename index_type>
    static stored_type
    get( const storage_type * base, index_type idx ) {
	return base[idx];
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::load( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::loadu( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    ntload( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::ntload( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static void
    store( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( raw );
	stored_traits<Tr::VL>::store( &base[idx], value );
    }

    template<typename Tr, typename index_type>
    static void
    storeu( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( raw );
	stored_traits<Tr::VL>::storeu( &base[idx], value );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::gather( base, idx );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx, mask_type mask ) {
	auto raw = stored_traits<Tr::VL>::gather( base, idx, mask );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val ) {
	static_assert( sizeof(StoredTy) >= 4 || Tr::VL == 1,
		       "vector scatter i32/i64 only" );
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( val );
	stored_traits<Tr::VL>::scatter( base, idx, value );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val,
	     mask_type mask ) {
	// static_assert( sizeof(StoredTy) >= 4 || Tr::VL == 1,
	// "vector scatter i32/i64 only" );
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( val );
	stored_traits<Tr::VL>::scatter( base, idx, value, mask );
    }

    [[deprecated("replaced by template version for correctness")]]
    static bool cas( volatile storage_type * addr,
		     stored_type old, stored_type val ) {
	return stored_traits<1>::cas( addr, old, val );
    }

    template<typename Tr>
    static bool cas( volatile storage_type * addr,
		     typename Tr::member_type old,
		     typename Tr::member_type val ) {
	static_assert( Tr::VL == 1, "CAS applies to scalar values only" );
	auto s_old = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<1>::member_type,1>::convert( old );
	auto s_val = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<1>::member_type,1>::convert( val );
	return stored_traits<1>::cas( addr, s_old, s_val );
    }
};

template<unsigned short E, unsigned short M>
struct array_encoding<customfp<E,M>,std::enable_if_t<(E+M)==21>> {
    using stored_type = customfp<E,M>;
    using storage_type = uint64_t;

    template<unsigned short VL_>
    using stored_traits = vector_type_traits_vl<stored_type, VL_>;

    template<unsigned short VL_>
    using enc_traits = vector_type_traits_vl<storage_type, VL_>;

    static mmap_ptr<storage_type>
    allocate( size_t num_elems, const numa_allocation & kind ) {
	// TODO: if mapped in numa-aware fashion, then boundaries between
	//       numa nodes are determined by partitioner and
	//       need to be adjusted by factor of 3.
	return mmap_ptr<storage_type>( (num_elems+2)/3, kind );
    }
    static mmap_ptr<storage_type>
    allocate( size_t num_elems, const numa_allocation && kind ) {
	// TODO: if mapped in numa-aware fashion, then boundaries between
	//       numa nodes are determined by partitioner and
	//       need to be adjusted by factor of 3.
	return mmap_ptr<storage_type>( (num_elems+2)/3, kind );
    }
    static mmap_ptr<storage_type>
    allocate( numa_allocation_partitioned kind ) {
	// TODO: scale to match NUMA boundaries
	partitioner part = kind.get_partitioner().contract_widen( 3 );
	return mmap_ptr<storage_type>( kind );
    }

    template<typename index_type>
    static stored_type
    get( const storage_type * base, index_type idx ) {
	auto raw = stored_traits<3>::load(
	    reinterpret_cast<stored_type *>( &base[idx/3] ) );
	auto elm = stored_traits<3>::lane( raw, idx % 3 );
	return stored_type( elm );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
	if constexpr ( Tr::VL == 1 ) {
	    auto raw = stored_traits<3>::load(
		reinterpret_cast<stored_type *>( &base[idx/3] ) );
	    auto elm = stored_traits<3>::lane( raw, idx % 3 );
	    stored_type cfp( elm );
	    return (typename Tr::type)cfp;
	} else {
	    assert( idx % Tr::VL == 0 && Tr::VL % 3 == 0 && "aligned load" );
	    auto raw = stored_traits<Tr::VL>::load( &base[idx/3] );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	}
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( storage_type * base, index_type idx ) {
	if constexpr ( Tr::VL == 1 ) {
	    auto raw = stored_traits<3>::load(
		reinterpret_cast<stored_type *>( &base[idx/3] ) );
	    auto elm = stored_traits<3>::lane( raw, idx % 3 );
	    stored_type cfp( elm );
	    return (typename Tr::type)cfp;
	} else if( idx % 3 == 0 ) {
	    auto raw = stored_traits<Tr::VL>::load( &base[idx/3] );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	} else {
	    assert( 0 && "NYI" ); // only VL==3 ?
/*
	    auto raw0 = stored_traits<Tr::VL>::load( &base[idx/3] );
	    auto raw1 = stored_traits<Tr::VL>::load( &base[(idx/3)+1] );
	    index_type s = 21 * ( idx % 3 );
	    index_type t = 21 * ( 3 - ( idx % 3 ) );
	    auto raw = ( ( raw0 >> s ) | ( raw1 << t ) ) & ~( uint64_t(1)<<63 );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
*/
	}
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    ntload( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::ntload( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static void
    store( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	if constexpr ( Tr::VL == 1 ) {
	    stored_type value( raw );
	    auto m = stored_traits<3>::load( 
		reinterpret_cast<stored_type *>( &base[idx/3] ) );
	    m = stored_traits<3>::setlane(  m, value, idx % 3 );
	    stored_traits<3>::store( 
		reinterpret_cast<stored_type *>( &base[idx/3] ), m );
	} else {
	    assert( idx % Tr::VL == 0 && Tr::VL % 3 == 0 && "aligned store" );
	    auto value = conversion_traits<
		typename Tr::member_type,
		typename stored_traits<Tr::VL>::member_type,
		Tr::VL>::convert( raw );
	    stored_traits<Tr::VL>::store( &base[idx/3], value );
	}
    }

    template<typename Tr, typename index_type>
    static void
    storeu( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	if constexpr ( Tr::VL == 1 )
	    return load<Tr>( base, idx );
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx, mask_type mask ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	using idx_traits = vector_type_traits<int_type_of_size_t<
	    sizeof(index_type)/Tr::VL>,sizeof(index_type)>;
	using int_traits = vector_type_traits_vl<uint32_t,Tr::VL>;
	auto dm = idx_traits::divmod3( idx );
	auto div = dm.a;
	auto off = idx_traits::mul( dm.b, idx_traits::set1( E+M ) );
	auto raw = enc_traits<Tr::VL>::gather( base, div, mask );
	auto coff = conversion_traits<
	    typename idx_traits::member_type,
	    typename int_traits::member_type,
	    Tr::VL>::convert( off );
	auto shf = enc_traits<Tr::VL>::srl( raw, coff );
	auto wid = conversion_traits<
	    typename enc_traits<Tr::VL>::member_type,
	    typename int_traits::member_type,
	    Tr::VL>::convert( shf );

	// From now on, should be working on 32-bit elements
	// TODO: simplify sequence cvt_ methods ...?
	static_assert( int_traits::W == 4, "expect 32-bit elements" );
	const auto one = int_traits::setone();
	const auto msk = int_traits::srl( one, 32-(E+M) );
	const auto exp = int_traits::sll(
	    int_traits::srl( one, 32-((8-E)-1) ), E+M );
	auto num = int_traits::logical_or(
	    int_traits::logical_and( wid, msk ), exp );
	auto fin = int_traits::sll( num, 23-M );
	auto fp32 = int_traits::castfp( fin );

	return fp32;
    }

    template<typename Tr, typename index_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val,
	     mask_type mask ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	assert( 0 && "NYI" );
    }

    [[deprecated("replaced by template version for correctness")]]
    static bool cas( volatile storage_type * addr,
		     stored_type old, stored_type val ) {
	assert( 0 && "NYI" );
	return false;
    }

    template<typename Tr>
    static bool cas( volatile storage_type * addr,
		     typename Tr::member_type old,
		     typename Tr::member_type val ) {
	static_assert( Tr::VL == 1, "CAS applies to scalar values only" );
	assert( 0 && "NYI" );
    }
};

template<>
struct array_encoding<void> {
    using stored_type = void;
    using storage_type = void;
};

template<typename Encoding, typename Tr, typename Idx>
struct encoded_element_ref {
    using encoding = Encoding;
    using storage_type = typename encoding::storage_type;
    using stored_type = typename encoding::stored_type;
    using ifc_type = typename Tr::type;
    using index_type = Idx;

    encoded_element_ref( storage_type * ptr_, index_type idx_ )
	: ptr( ptr_ ), idx( idx_ ) { }

    ifc_type operator = ( ifc_type t ) const {
	encoding::template store<simd::detail::vdata_traits<ifc_type,1>>( ptr, idx, t );
	return t;
    }
    operator ifc_type () const {
	return encoding::template load<simd::detail::vdata_traits<ifc_type,1>>( ptr, idx );
    }
    stored_type get() const {
	return encoding::get( ptr, idx );
    }

private:
    storage_type * ptr;
    index_type idx;
};

template<unsigned short Bits>
struct array_encoding_bit {
    static constexpr unsigned short bits = Bits;
    static constexpr unsigned short factor = 8/bits;
    static_assert( bits == 1 || bits == 2 || bits == 4,
		   "assuming a whole number of bitfields per byte" );
    
    using stored_type = bitfield<bits>;
    using storage_type = uint8_t;

    template<unsigned short VL_>
    using stored_traits = vector_type_traits_vl<stored_type, VL_>;

    template<unsigned short VL_>
    using enc_traits = vector_type_traits_vl<storage_type, VL_>;

    static mmap_ptr<storage_type>
    allocate( size_t num_elems, const numa_allocation & kind ) {
	// TODO: if mapped in numa-aware fashion, then boundaries between
	//       numa nodes are determined by partitioner and
	//       need to be adjusted by factor of 8/bits.
	return mmap_ptr<storage_type>( (num_elems+factor-1)/factor, kind );
    }
    static mmap_ptr<storage_type>
    allocate( size_t num_elems, const numa_allocation && kind ) {
	// TODO: if mapped in numa-aware fashion, then boundaries between
	//       numa nodes are determined by partitioner and
	//       need to be adjusted by factor of 3.
	return mmap_ptr<storage_type>( (num_elems+factor-1)/factor, kind );
    }
    static mmap_ptr<storage_type>
    allocate( numa_allocation_partitioned kind ) {
	// TODO: scale to match NUMA boundaries
	partitioner part = kind.get_partitioner().contract_widen( factor );
	return mmap_ptr<storage_type>( kind );
    }

    template<typename index_type>
    static stored_type
    get( const storage_type * base, index_type idx ) {
	auto raw = enc_traits<1>::load(
	    reinterpret_cast<const storage_type *>( &base[idx/factor] ) );
	auto elm = stored_traits<factor>::lane( raw, idx % factor );
	return stored_type( elm );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
	using tr = stored_traits<Tr::VL>;
	if constexpr ( Tr::VL == 1 ) {
	    auto raw = tr::load(
		reinterpret_cast<typename tr::pointer_type *>( base ), idx );
	    return conversion_traits<
		typename tr::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	} else {
	    static_assert( Tr::VL % factor == 0, "full bytes" );
	    // assert( idx % Tr::VL == 0 && "aligned load" );
	    auto raw = tr::load(
		reinterpret_cast<typename tr::type *>( &base[idx/factor] ) );
	    return conversion_traits<
		typename tr::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	}
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( const storage_type * base, index_type idx ) {
	if constexpr ( Tr::VL == 1 ) {
	    return (typename Tr::type)get( base, idx );
	} else if( idx % factor == 0 ) {
	    using tr = stored_traits<Tr::VL>;
	    auto raw = tr::load(
		reinterpret_cast<const typename tr::type *>( &base[idx/factor] ) );
	    return conversion_traits<
		typename tr::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	} else {
	    assert( 0 && "NYI" ); // only VL==factor ?
	}
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    ntload( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::ntload( &base[idx/factor] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static void
    store( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	storeu<Tr>( base, idx, raw );
    }

    template<typename Tr, typename index_type>
    static void
    storeu( storage_type * base, index_type idx,
	    typename Tr::type raw ) {
	if constexpr ( Tr::VL == 1 ) {
	    stored_type value( raw );
	    bool success;
	    do {
		auto m = enc_traits<1>::load( 
		    reinterpret_cast<storage_type *>( &base[idx/factor] ) );
		auto u = stored_traits<factor>::setlane( m, value, idx % factor );
		// A CAS is necessary as two threads may modify different bits
		// of the same byte concurrently
		success = cas<Tr>( &base[idx/factor], m, u );
	    } while( !success );
	} else {
	    // A CAS is necessary as two threads may modify different bits
	    // of the same byte concurrently in an unaligned store. This is
	    // however not possible for aligned stores (which assumes that full
	    // bytes are modified).
	    assert( idx % Tr::VL == 0 && Tr::VL % factor == 0
		    && "aligned store" );
	    auto value = conversion_traits<
		typename Tr::member_type,
		typename stored_traits<Tr::VL>::member_type,
		Tr::VL>::convert( raw );
	    using tr = stored_traits<Tr::VL>;
	    tr::store(
		reinterpret_cast<typename tr::type *>( &base[idx/factor] ),
		value );
	}
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	if constexpr ( Tr::VL == 1 )
	    return load<Tr>( base, idx );
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( const storage_type * base, index_type idx, mask_type mask ) {
	// this gather loads only aligned bytes / words
	// gather bytes requires to first gather words, compact to bytes,
	// then compact to bits again. Can those compaction steps be fused?
	// Presumably the bitshuffle extensions may help
	using idx_traits = vector_type_traits<int_type_of_size_t<
	    sizeof(index_type)/Tr::VL>,sizeof(index_type)>;
	using tgt_traits = typename Tr::traits;
	using tr = stored_traits<Tr::VL>;

	// More efficient version of code in case we want to extract a byte
	// We know that srlv on bytes is not possible and so are gathers.
	// Hence we perform gather and srlv on 32-bit elements prior to
	// converting to bytes
	if constexpr ( Tr::B >= 8
#if __AVX512BW__
		       && Tr::B <= 64
#elif __AVX2__
		       && Tr::B <= 32
#endif
	    ) {
	    // Address arithmetic assumes byte operations for bitfield
	    using u32t = vector_type_traits_vl<uint32_t,Tr::VL>;
	    constexpr unsigned short B = ilog2(bits);
	    constexpr unsigned short F = ilog2(32/bits);
	    static_assert( F == 5-B, "yes" );
	    static_assert( bits == 2, "code not validated for 4 bits" );
	    // Worse version: no srli but gather_w<1>
	    auto div = idx_traits::srli( idx, F );
	    auto raw = u32t::gather(
		reinterpret_cast<const typename u32t::member_type *>( base ),
		div, mask );

	    // Now we have loaded uint32_t's where the low byte in each lane
	    // is the byte of interest
	    const auto omask = idx_traits::slli( idx_traits::setone(), F );
	    auto off = idx_traits::bitwise_andnot( omask, idx );
	    auto coff = conversion_traits<
		typename idx_traits::member_type,
		typename u32t::member_type,
		Tr::VL>::convert( off );

	    // shr contains 32-bit lanes where the lower 2 bits are of interest
	    auto coff2 = u32t::slli( coff, B );
	    auto shr = u32t::srlv( raw, coff2 );

	    // Mask out unneeded bits - lacking slli also for bytes
	    const auto bmask = u32t::slli( u32t::setone(), bits );
	    auto bsel = u32t::bitwise_andnot( bmask, shr );

	    // Convert to short width (typically bytes)
	    using str =
		vector_type_traits_vl<int_type_of_size_t<
		    sizeof(typename Tr::element_type)>,Tr::VL>;
	    auto cvt = conversion_traits<
		typename u32t::member_type,
		typename str::member_type,
		Tr::VL>::convert( bsel );
	    
	    // Done
	    return cvt;
	}

	
#if __AVX512F__
	// If just one bit is needed, and mask returned,
	// use comparison operation
	if constexpr ( bits == 1 ) {
	    using u32t = vector_type_traits_vl<uint32_t,Tr::VL>;
	    constexpr unsigned short B = ilog2(bits);
	    constexpr unsigned short F = ilog2(32/bits);
	    static_assert( F == 5-B, "yes" );

	    auto div = idx_traits::srli( idx, F );
	    auto raw = u32t::gather(
		reinterpret_cast<const typename u32t::member_type *>( base ),
		div, mask );

	    const auto omask =
		idx_traits::srli( idx_traits::setone(),
				  (unsigned short)(8*idx_traits::W-F) );
	    typename idx_traits::type woff;
	    if constexpr ( idx_traits::has_ternary ) {
		// ? omask idx
		// ? 0 0 | 0
		// ? 0 1 | 0
		// ? 1 0 | 1
		// ? 1 1 | 0
		auto undef = omask; // any value
		woff = idx_traits::template ternary<0b01000100>(
		    undef, omask, idx );
	    } else {
		auto off = idx_traits::bitwise_and( idx, omask );
		woff = idx_traits::bitwise_xor( omask, off );
	    }
	    auto s = u32t::sllv( raw, woff );

	    const auto hmask = u32t::srli( u32t::setone(), 1 );
	    auto r = u32t::cmpgt( s, hmask, target::mt_mask() );
	    return r;
	}
#endif

	using g_traits =
	    vector_type_traits_vl<int_type_of_size_t<
		sizeof(typename Tr::element_type)>,Tr::VL>;

	constexpr unsigned short F = ilog2(g_traits::W*8/bits);
	constexpr unsigned short B = ilog2(bits);
	auto div = idx_traits::srli( idx, F );
	// TODO - gather aligned on u32
	auto raw = g_traits::gather(
	    reinterpret_cast<const typename g_traits::member_type *>( base ),
	    div, mask );

	// We distinguish cases with small vectors (e.g., 1x8 bytes)
	// as these do not support srlv. Instead we use pdep.
	if constexpr ( g_traits::size == g_traits::vlen ) {
	    const auto omask =
		idx_traits::srli( idx_traits::setone(),
				  (unsigned short)(8*idx_traits::W-F) );
	    static_assert( F <= 4, "fields in off are at most a nibble wide" );
	    static_assert( bits == 1 || bits == 2,
			   "code not validated for 4 bits" );
	    auto off = idx_traits::bitwise_and( idx, omask );
	    auto coff = conversion_traits<
		typename idx_traits::member_type,
		typename g_traits::member_type,
		Tr::VL>::convert( off );

	    if constexpr ( g_traits::vlen == 8 ) {
		// Use a lookup table to translate 4-bit or 2-bit offsets
		// to parts of a bitmask for pext. sllv on byte not supported
#if GRAPTOR_USE_MMX
		auto off2 = _mm_cvtsi64_si128( _mm_cvtm64_si64( coff ) );
#else
		auto off2 = coff;
#endif
		const __m128i * lut_p;
		if constexpr ( bits == 2 )
		    lut_p = reinterpret_cast<const __m128i *>(
			shuffle_encoding_bitfield_1x8_off2 );
		else if constexpr ( bits == 1 )
		    lut_p = reinterpret_cast<const __m128i *>(
			shuffle_encoding_bitfield_1x8_off1 );
		__m128i lut = _mm_load_si128( lut_p );
		auto off3 = _mm_shuffle_epi8( lut, off2 );
		auto pmask = _mm_extract_epi64( off3, 0 );
		auto c = _pext_u64( g_traits::asint( raw ), pmask );
	    
		if constexpr ( is_bitfield_v<typename Tr::member_type> )
		    return c;
		else {
		    auto d = conversion_traits<
			bitfield<bits>,
			typename tgt_traits::member_type,
			Tr::VL>::convert( c );
		    return d;
		}
	    }
#if __AVX512F__ && __AVX512VL__ && __AVX512BW__
	    // sllv_epi16 is available
	    if constexpr ( g_traits::vlen == 32 ) {
		// Split in even/odd lanes
// TODO: extend to 64 VL
// TODO: rework such that bytes are shuffled across epi32 lanes, then do other add (shuf epi32 + add epi32?) -- would work at VL64
		using w_traits = vector_type_traits_vl<uint16_t,Tr::VL/2>;
		auto mbyte = w_traits::srli( w_traits::setone(), 8 );
		auto soff = w_traits::slli( coff, 1 );
		auto idx_hi1 = w_traits::bitwise_andnot( mbyte, soff );
		auto idx_hi = w_traits::srli( idx_hi1, 8 );
		auto raw_hi = w_traits::bitwise_andnot( mbyte, raw );
		auto idx_lo = w_traits::bitwise_and( mbyte, soff );
		auto raw_lo = w_traits::bitwise_and( mbyte, raw );
		auto sel_hi = w_traits::srlv( raw_hi, idx_hi );
		auto sel_lo = w_traits::srlv( raw_lo, idx_lo );
		auto mbits = w_traits::srli( w_traits::setone(),
					     8*w_traits::W-bits );
		auto bits_lo = w_traits::bitwise_and( mbits, sel_lo );
		mbits = w_traits::slli( mbits, 8 );
		auto bits_hi = w_traits::bitwise_and( mbits, sel_hi );
		auto vbyte = w_traits::bitwise_or( bits_hi, bits_lo );

		// shuffle to create groups of equal lanes mod 2**bits
		// do this independently for each 128-bit group
		auto shuf_p = reinterpret_cast<const uint16_t *>(
		    shuffle_encoding_bitfield_shuffle_2bx32 );
		auto shuf = w_traits::load( shuf_p );
		auto gbyte = _mm256_shuffle_epi8( vbyte, shuf );

		// Now shift each group
		auto sllv_p = reinterpret_cast<const uint16_t *>(
		    shuffle_encoding_bitfield_sllv_2bx32 );
		auto sllv = w_traits::load( sllv_p );
		auto sbyte = _mm256_sllv_epi32( gbyte, sllv );

		// We have 8 groups of 4 bytes. Each 4 groups need to be
		// reduced between them
		// sbyte: g7 g6 g5 g4 | g3 g2 g1 g0
		// sswap: g3 g2 g1 g0 | g7 g6 g5 g4
		// auto sswap = _mm256_permutex_epi64( sbyte, 0b01001110 );
		auto sswap = _mm256_castsi128_si256(
		    _mm256_extracti128_si256( sbyte, 1 ) );
		// reduce:  ... | g7+g6 g5+g4 g3+g2 g1+g0
		auto reduce = _mm256_hadd_epi32( sbyte, sswap );
		// reduce2: ... | u64 u64 g7+g6+g5+g4 g3+g2+g1+g0
		auto reduce2 = _mm256_hadd_epi32( reduce, reduce );

		auto c = _mm256_extract_epi64( reduce2, 0 );

		if constexpr ( is_bitfield_v<typename Tr::member_type> )
		    return c;
		else {
		    auto d = conversion_traits<
			bitfield<bits>,
			typename tgt_traits::member_type,
			Tr::VL>::convert( c );
		    return d;
		}
	    }
#endif
	    if constexpr ( g_traits::vlen == 16 ) {
		// Use a lookup table to translate 4-bit or 2-bit offsets
		// to parts of a bitmask for pext. sllv on byte not supported
		auto off2 = coff;
		auto lut_p = reinterpret_cast<const __m128i *>(
		    shuffle_encoding_bitfield_1x8_off2 );
		__m128i lut = _mm_load_si128( lut_p );
		auto off3 = _mm_shuffle_epi8( lut, off2 );
		auto pmask0 = _mm_extract_epi64( off3, 0 );
		auto c0 = _pext_u64( _mm_extract_epi64( raw, 0 ), pmask0 );
		auto pmask1 = _mm_extract_epi64( off3, 1 );
		auto c1 = _pext_u64( _mm_extract_epi64( raw, 1 ), pmask1 );
		auto c = ( c1 << 16 ) | c0;
	    
		if constexpr ( is_bitfield_v<typename Tr::member_type> )
		    return c;
		else {
		    auto d = conversion_traits<
			bitfield<bits>,
			typename tgt_traits::member_type,
			Tr::VL>::convert( c );
		    return d;
		}
	    }
	    if constexpr ( Tr::VL > 16 ) {
		// recursive case - spanning multiple vectors
		using STr = typename Tr::template rebindVL_t<Tr::VL/2>;
		using SIt = idx_traits;

		if constexpr ( sizeof(mask_type) == sizeof(index_type) ) {
		    auto lo = array_encoding_bit<bits>::template gather<STr>(
			base, SIt::lower_half( idx ), SIt::lower_half( mask ) );
		    auto hi = array_encoding_bit<bits>::template gather<STr>(
			base, SIt::upper_half( idx ), SIt::upper_half( mask ) );
		    auto r = Tr::traits::set_pair( hi, lo );
		    return r;
		} else if constexpr ( 8*sizeof(mask_type) == Tr::VL ) {
		    using SMt = typename SIt::mtraits;
		    auto lo = array_encoding_bit<bits>::template gather<STr>(
			base, SIt::lower_half( idx ), SMt::lower_half( mask ) );
		    auto hi = array_encoding_bit<bits>::template gather<STr>(
			base, SIt::upper_half( idx ), SMt::upper_half( mask ) );
		    auto r = Tr::traits::set_pair( hi, lo );
		    return r;
		} else if constexpr ( 8*sizeof(mask_type) == bits*Tr::VL ) {
		    using SMt = stored_traits<Tr::VL>;
		    auto lo = array_encoding_bit<bits>::template gather<STr>(
			base, SIt::lower_half( idx ), SMt::lower_half( mask ) );
		    auto hi = array_encoding_bit<bits>::template gather<STr>(
			base, SIt::upper_half( idx ), SMt::upper_half( mask ) );
		    auto r = Tr::traits::set_pair( hi, lo );
		    return r;
		} else {
		    assert( 0 && "NYI" );
		}
	    }
	    assert( 0 && "NYI" );
	} else {
	    // This case does not include bitfields, as they would have
	    // a width of 1 byte
	    const auto omask =
		idx_traits::srli( idx_traits::setone(),
				  (unsigned short)(8*idx_traits::W-F) );
	    auto off = idx_traits::slli(
		idx_traits::bitwise_and( idx, omask ), B );
	    auto coff = conversion_traits<
		typename idx_traits::member_type,
		typename tgt_traits::member_type,
		Tr::VL>::convert( off );
	    auto pos = tgt_traits::srlv( raw, coff );

	    auto fmask = tgt_traits::srli( tgt_traits::setone(),
					   (unsigned short)(8*Tr::W-bits) );
	    auto val = tgt_traits::bitwise_and( fmask, pos );
	    return val;
	}
    }

    template<typename Tr, typename index_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val,
	     mask_type mask ) {
	// would be better to fuse gather and conversion operations to avoid
	// packing+unpacking and shifting after gathering words?
	assert( 0 && "NYI" );
    }

    [[deprecated("replaced by template version for correctness")]]
    static bool cas( volatile storage_type * addr,
		     storage_type old, storage_type val ) {
	return __sync_bool_compare_and_swap( addr, old, val );
    }

    template<typename Tr>
    static bool cas( volatile storage_type * addr,
		     typename Tr::member_type old,
		     typename Tr::member_type val ) {
	static_assert( Tr::VL == 1, "CAS applies to scalar values only" );
	assert( 0 && "NYI" );
    }
};

template<unsigned short Bits>
struct array_encoding<bitfield<Bits>> : public array_encoding_bit<Bits> {
};

template<typename StoredTy, typename Enable = void>
struct array_encoding_wide {
    using stored_type = StoredTy;
    using storage_type = StoredTy;

    template<unsigned short VL_>
    using stored_traits = vector_type_traits_vl<stored_type, VL_>;

    static mmap_ptr<stored_type>
    allocate( size_t num_elems, const numa_allocation && kind ) {
	return mmap_ptr<stored_type>( num_elems, kind );
    }
    static mmap_ptr<stored_type>
    allocate( size_t num_elems, const numa_allocation & kind ) {
	return mmap_ptr<stored_type>( num_elems, kind );
    }
    static mmap_ptr<stored_type>
    allocate( numa_allocation_partitioned kind ) {
	// TODO: scale to match NUMA boundaries
	return mmap_ptr<stored_type>( kind );
    }

    template<typename index_type>
    static stored_type
    get( const storage_type * base, index_type idx ) {
	return base[idx];
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::load( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::loadu( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    ntload( storage_type * base, index_type idx ) {
	auto raw = stored_traits<Tr::VL>::ntload( &base[idx] );
	return conversion_traits<
	    typename stored_traits<Tr::VL>::member_type,
	    typename Tr::member_type,
	    Tr::VL>::convert( raw );
    }

    template<typename Tr, typename index_type>
    static void
    store( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( raw );
	stored_traits<Tr::VL>::store( &base[idx], value );
    }

    template<typename Tr, typename index_type>
    static void
    storeu( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( raw );
	stored_traits<Tr::VL>::storeu( &base[idx], value );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code
	    return array_encoding<stored_type>::template load<Tr>(
		base, idx );
	}
	using st = stored_traits<Tr::VL>;

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    return array_encoding<stored_type>::template load<Tr>( base, idx );
	} else

	if constexpr ( is_extended_floating_point_v<stored_type>
		       && is_extended_floating_point_v<typename Tr::member_type>
		       && Tr::W != st::W ) {
	    using it = typename Tr::traits::int_traits;
	    auto raw = gather_helper<it>( base, idx );
	    auto b = cvt_float_from_spaced_int<
		stored_type,typename Tr::member_type,Tr::VL>( raw );
	    return b;
	} else

	if constexpr ( std::is_integral_v<stored_type>
		       && std::is_integral_v<typename Tr::member_type>
		       && sizeof(stored_type) < sizeof(typename Tr::member_type)
		       ) {
	    using it = typename Tr::traits::int_traits;
	    auto a = gather_helper<it>( base, idx );
	    auto b = cvt_uint_from_spaced_int<
		stored_type,typename Tr::member_type,Tr::VL>( a );
	    return b;
	} else
	
	// Default (fall-through)
	{
	    auto raw = stored_traits<Tr::VL>::gather( base, idx );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	}
    }

    // Helper function: load a spaced vector of elements of shorter width.
    // Raw result of gather is returned with desired data in lsb of lane.
    // Spacing is not zeroed out.
    // it: vector_type_traits of integer type, width of final type
    template<typename it, typename idx_ty, typename mask_ty>
    static auto gather_helper( storage_type * base, idx_ty idx, mask_ty mask ) {
	using st = stored_traits<it::vlen>;

#if WIDEN_APPROACH == 0
	// Simplest version: perform unaligned gather, apply scale factor
	// in line with stored data.
	auto a = it::template gather_w<st::W>(
	    reinterpret_cast<const typename it::member_type *>( base ),
	    idx, mask );
#elif WIDEN_APPROACH == 1
	// Perform aligned gather. Use native scale factor.
	using xt = vector_type_traits_vl<
	    int_type_of_size_t<sizeof(idx_ty)/it::vlen>,it::vlen>;
	constexpr unsigned short lg = ilog2(it::W) - ilog2(st::W);
	const auto m = xt::slli( xt::setone(), lg );
	auto algn = xt::srli( idx, lg );
	auto raw = it::gather(
	    reinterpret_cast<const typename it::member_type *>( base ),
	    algn, mask );
	auto sh = xt::slli( xt::bitwise_andnot( m, idx ), 3+lg );
	auto a = it::srlv( raw, sh );
#endif
	
	return a;
    }

    template<typename it, typename idx_ty>
    static auto gather_helper( storage_type * base, idx_ty idx ) {
	using st = stored_traits<it::vlen>;

#if WIDEN_APPROACH == 0
	// Simplest version: perform unaligned gather, apply scale factor
	// in line with stored data.
	auto a = it::template gather_w<st::W>(
	    reinterpret_cast<const typename it::member_type *>( base ), idx );
#elif WIDEN_APPROACH == 1
	// Perform aligned gather. Use native scale factor.
	using xt = vector_type_traits_vl<
	    int_type_of_size_t<sizeof(idx_ty)/it::vlen>,it::vlen>;
	constexpr unsigned short lg = ilog2(it::W) - ilog2(st::W);
	const auto m = xt::slli( xt::setone(), lg );
	auto algn = xt::srli( idx, lg );
	auto raw = it::gather(
	    reinterpret_cast<const typename it::member_type *>( base ),
	    algn );
	auto sh = xt::slli( xt::bitwise_andnot( m, idx ), 3+lg );
	auto a = it::srlv( raw, sh );
#endif
	
	return a;
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx, mask_type mask ) {
	using st = stored_traits<Tr::VL>;

	// static_assert( Tr::W > st::W,
	// "encoding wide gather must widen" );

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code. Avoid performing a random-access read if mask
	    // says so. Load operations have indices that are always valid,
	    // however, gather operations may have invalid indices.
	    if( st::mask_traits::lane0( mask ) )
		return array_encoding<stored_type>::template load<Tr>(
		    base, idx );
	    else
		// Could also return uninitialised
		return st::setzero();
	} else

	if constexpr ( is_extended_floating_point_v<stored_type>
		       && is_extended_floating_point_v<typename Tr::member_type>
		       && Tr::W != st::W ) {
	    using it = typename Tr::traits::int_traits;
	    auto raw = gather_helper<it>( base, idx, mask );
	    auto b = cvt_float_from_spaced_int<
		stored_type,typename Tr::member_type,Tr::VL>( raw );
	    return b;
	} else

	if constexpr ( std::is_integral_v<stored_type>
		       && std::is_integral_v<typename Tr::member_type>
		       && sizeof(stored_type) < sizeof(typename Tr::member_type)
		       ) {
	    using it = typename Tr::traits::int_traits;
	    auto a = gather_helper<it>( base, idx, mask );
	    auto b = cvt_uint_from_spaced_int<
		stored_type,typename Tr::member_type,Tr::VL>( a );
	    return b;
	} else
	
	// Default (fall-through)
	{
	    auto raw = stored_traits<Tr::VL>::gather( base, idx, mask );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	}
    }

    template<typename Tr, typename index_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val ) {
	static_assert( sizeof(StoredTy) >= 4 || Tr::VL == 1,
		       "vector scatter i32/i64 only" );
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( val );
	stored_traits<Tr::VL>::scatter( base, idx, value );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val,
	     mask_type mask ) {
	static_assert( sizeof(StoredTy) >= 4 || Tr::VL == 1,
		       "vector scatter i32/i64 only" );
	auto value = conversion_traits<
	    typename Tr::member_type,
	    typename stored_traits<Tr::VL>::member_type,
	    Tr::VL>::convert( val );
	stored_traits<Tr::VL>::scatter( base, idx, value, mask );
    }

    [[deprecated("replaced by template version for correctness")]]
    static bool cas( volatile storage_type * addr,
		     stored_type old, stored_type val ) {
	return stored_traits<1>::cas( addr, old, val );
    }

    template<typename Tr>
    static bool cas( volatile storage_type * addr,
		     typename Tr::member_type old,
		     typename Tr::member_type val ) {
	static_assert( Tr::VL == 1, "CAS applies to scalar values only" );
	// Re-use baseline CAS definition
	return array_encoding<stored_type>::template cas<Tr>( addr, old, val );
    }

public:
    // Convert to wider width. Expects meaningful
    // bits in lower half of the word with upper half undefined.
    template<typename Tr>
    static typename Tr::type convert_wide( typename Tr::traits::itype a ) {
	// Unexpected
	if constexpr ( std::is_same_v<stored_type,typename Tr::member_type> ) {
	    if constexpr ( std::is_floating_point_v<stored_type> )
		return Tr::traits::int_traits::castfp( a );
	    else
		return a;
	}

	if constexpr ( is_extended_floating_point_v<stored_type>
		       && is_extended_floating_point_v<typename Tr::member_type>
	    ) {
	    using it = typename Tr::traits::int_traits;
	    using st = stored_traits<Tr::VL>;
	    auto b = cvt_float_from_spaced_int<
		stored_type,typename Tr::member_type,Tr::VL>( a );
	    return b;
	}
	
#if 0
	if constexpr ( std::is_same_v<stored_type,float>
		       && std::is_same_v<typename Tr::member_type,double> ) {
	    using it = vector_type_traits_vl<uint64_t,Tr::VL>;

#if 0
	    // We have integer data: 4 meaningful bytes in every 8 bytes,
	    // in the top parts. Convert this to double.
	    // This conversion does not consider exception values for the
	    // exponent.
	    // How to make faster:
	    // - Prior knowledge of sign bit: always positive (2 insn)
	    // - Get subtract out of inner loop
	    auto m1 = it::slli( it::setone(), 63 );
	    // auto mlo = it::srli( it::setone(), 32 );
	    // auto aa = it::bitwise_andnot( mlo, a ); // clear lower half
	    auto aa = it::slli( a, 32 ); // clear lower half
	    auto b = it::bitwise_andnot( m1, aa ); // remove sign bit
	    auto c = it::srli( b, 11-8 );
	    // auto eoff = it::sub( it::srli( m1, 1 ), it::srli( m1, 1+11-8 ) );
	    auto eoff = it::srli( it::slli( setone(), 64-(11-8) ), 2 );
	    auto d = it::add( c, eoff );
	    // auto e = it::blend( m1, d, a ); // m1 ? a : d
	    auto s = it::bitwise_and( m1, aa );
	    auto e = it::bitwise_or( s, d );
	    auto f = it::castfp( e );
	    return f;
#else
#if CVT_FP_POSITIVE
#if 0
	    // Assumes sign bit == 0
	    const auto mask = it::slli( it::setone(), 64-(11-8)-1 );
	    const auto eoff =
		it::srli( it::slli( it::setone(), 64-(11-8) ), 2 );
	    auto b = it::slli( a, 32-(11-8) );
	    auto d = it::bitwise_andnot( mask, b );
	    auto e = it::add( d, eoff );
	    auto f = it::castfp( e );
	    return f;
#else
	    // Assumes sign bit == 0 and exponent < 0
	    const auto mask = it::slli( it::setone(), 64-(11-8)-2 );
	    const auto eoff =
		it::srli( it::slli( it::setone(), 64-(11-8) ), 2 );
	    auto b = it::slli( a, 32-(11-8) );
	    if constexpr( it::has_ternary ) {
		auto c = it::template ternary<0b11001010>( mask, eoff, b );
		auto d = it::castfp( c );
		return d;
	    } else {
		auto c = it::bitwise_andnot( mask, b );
		auto d = it::add( c, eoff );
		auto e = it::castfp( d );
		return e;
	    }
#endif
#else
	    // const auto r = it::srli( it::setone(), 62 ); // value 3
	    const auto mask = it::srli( it::slli( it::setone(), 61 ), 1 );
	    const auto eoff =
		it::srli( it::slli( it::setone(), 64-(11-8) ), 2 );
	    auto b = it::slli( a, 32 );
	    auto c = it::srai( b, 3 ); // srav (not) faster than srai in AVX512
	    auto d = it::bitwise_andnot( mask, c );
	    auto e = it::add( d, eoff ); // need addition if exp > 0
	    auto f = it::castfp( e );
	    return f;
#endif
#endif
	}
#endif

	if constexpr ( std::is_integral_v<stored_type>
		       && std::is_integral_v<typename Tr::member_type>
		       && Tr::W == 2 * sizeof(stored_type) ) {
	    // This zeroes the upper bits
	    using it = typename Tr::traits;
	    auto mask = it::slli( it::setone(), 8*Tr::W/2 );
	    return it::bitwise_andnot( mask, a );
	}

	if constexpr ( std::is_integral_v<stored_type>
		       && std::is_integral_v<typename Tr::member_type>
		       && Tr::W == 4 * sizeof(stored_type) ) {
	    // This zeroes the upper bits
	    using it = typename Tr::traits;
	    auto mask = it::slli( it::setone(), 8*Tr::W/2 );
	    return it::bitwise_andnot( mask, a );
	}

	assert( 0 && "Unhandled case" );
    }

    // Convert back to narrower width. This time leave meaningful
    // bits in lower/upper half of the word with other half undefined.
    template<typename Tr, bool upper>
    static typename stored_traits<Tr::VL*2>::type
    convert_narrow( typename Tr::traits::itype a ) {
	if constexpr ( std::is_same_v<stored_type,float>
		       && std::is_same_v<typename Tr::member_type,double> ) {
	    using it = vector_type_traits_vl<uint64_t,Tr::VL>;
	    using tr = stored_traits<Tr::VL*2>;

	    // This code assumes the floating-point number is in range
	    auto m1 = it::bitwise_invert( it::srli( it::setone(), 1 ) );
	    auto eoff = it::sub( it::srli( m1, 1 ), it::srli( m1, 1+11-8 ) );

	    auto b = it::bitwise_andnot( m1, a );  // remove sign bit
	    auto c = it::sub( b, eoff ); // adjust exponent
	    auto d = it::slli( c, 11-8 );
	    auto s = it::bitwise_and( m1, a );
	    auto e = it::bitwise_or( s, d );
	    if constexpr ( upper ) {
		auto f = tr::int_traits::castfp( e );
		return f;
	    } else {
		auto f = it::srli( e, 32 );
		auto g = tr::int_traits::castfp( f );
		return g;
	    }
	}

	assert( 0 && "Unhandled case" );
    }
};

// Allows to load a mini-vector of short fields per wide vector lane,
// e.g. load 8x byte per lane x VL
template<typename StoredTy, typename Enable = void>
struct array_encoding_multi {
    using stored_type = StoredTy;
    using storage_type = StoredTy;

    template<unsigned short VL_>
    using stored_traits = vector_type_traits_vl<stored_type, VL_>;

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
	using st = stored_traits<Tr::VL>;
	using it = typename Tr::traits;
	static_assert( Tr::W >= st::W,
		       "encoding multi must load multiple fields" );

	return it::loadu(
	    reinterpret_cast< typename Tr::member_type *>( &base[idx] ) );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( storage_type * base, index_type idx ) {
	using st = stored_traits<Tr::VL>;
	using it = typename Tr::traits;
	static_assert( Tr::W >= st::W,
		       "encoding multi must load multiple fields" );

	return it::loadu(
	    reinterpret_cast< typename Tr::member_type *>( &base[idx] ) );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	using st = stored_traits<Tr::VL>;

	static_assert( Tr::W >= st::W,
		       "encoding multi gather must load multiple fields" );

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    return array_encoding<typename Tr::member_type>
		::template loadu<Tr>( &base[idx], 0 );
	} else

	if constexpr ( is_integral_or_logical_v<stored_type>
		       && is_integral_or_logical_v<typename Tr::member_type> ) {
	    using it = typename Tr::traits::int_traits;
	    auto a = it::template gather_w<st::W>(
		reinterpret_cast<const typename it::member_type *>( base ),
		idx );
	    return a;
	} else
	
	// Default (fall-through)
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx, mask_type mask ) {
	using st = stored_traits<Tr::VL>;

	static_assert( Tr::W >= st::W,
		       "encoding multi gather must load multiple fields" );

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code. Avoid performing a random-access read if mask
	    // says so. Load operations have indices that are always valid,
	    // however, gather operations may have invalid indices.
	    if( st::mask_traits::lane0( mask ) )
		return array_encoding<typename Tr::member_type>
		    ::template loadu<Tr>( &base[idx], 0 );
	    else
		// Could also return uninitialised
		return st::setzero();
	} else

	if constexpr ( is_integral_or_logical_v<stored_type>
		       && is_integral_or_logical_v<typename Tr::member_type> ) {
	    using it = typename Tr::traits::int_traits;
	    auto a = it::template gather_w<st::W>(
		reinterpret_cast<const typename it::member_type *>( base ),
		idx, mask );
	    return a;
	} else
	
	// Default (fall-through)
	assert( 0 && "NYI" );
    }

};

// An encoding where by default (base pointer is null/no backing memory)
// zero is read; except when the value is cached (base pointer is non-null),
// then the actual values are read/written.
template<typename StoredTy>
struct array_encoding_zero {
    using base_encoding = array_encoding<StoredTy>;
    using stored_type = typename base_encoding::stored_type;
    using storage_type = typename base_encoding::storage_type;

    template<typename index_type>
    static stored_type
    get( const storage_type * base, index_type idx ) {
    	assert( base && "requirement" );
	return base_encoding::get( base, idx );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
    	if( base )
	    return base_encoding::template load<Tr>( base, idx );
	else
	    return vector_type_traits_vl<typename Tr::member_type,Tr::VL>
		::setzero();
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( storage_type * base, index_type idx ) {
	if( base )
	    return base_encoding::template loadu<Tr>( base, idx );
	else
	    return vector_type_traits_vl<typename Tr::member_type,Tr::VL>
		::setzero();
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    ntload( storage_type * base, index_type idx ) {
	if( base )
	    return base_encoding::template ntload<Tr>( base, idx );
	else
	    return vector_type_traits_vl<typename Tr::member_type,Tr::VL>
		::setzero();
    }

    template<typename Tr, typename index_type>
    static void
    store( storage_type * base, index_type idx, typename Tr::type raw ) {
	if( base )
	    return base_encoding::template store<Tr>( base, idx, raw );
    }

    template<typename Tr, typename index_type>
    static void
    storeu( storage_type * base, index_type idx, typename Tr::type raw ) {
	if( base )
	    return base_encoding::template storeu<Tr>( base, idx, raw );
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	if( base )
	    return base_encoding::template gather<Tr>( base, idx );
	else
	    return vector_type_traits_vl<typename Tr::member_type,Tr::VL>
		::setzero();
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx, mask_type mask ) {
	if( base )
	    return base_encoding::template gather<Tr>( base, idx, mask );
	else
	    return vector_type_traits_vl<typename Tr::member_type,Tr::VL>
		::setzero();
    }

    template<typename Tr, typename index_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val ) {
	if( base )
	    return base_encoding::template scatter<Tr>( base, idx, val );
    }

    template<typename Tr, typename index_type, typename mask_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val,
	     mask_type mask ) {
	if( base )
	    return base_encoding::template scatter<Tr>( base, idx, val, mask );
    }

    [[deprecated("replaced by template version for correctness")]]
    static bool cas( volatile storage_type * addr,
		     stored_type old, stored_type val ) {
    	assert( addr && "requirement" );
	return base_encoding::cas( addr, old, val );
    }

    template<typename Tr>
    static bool cas( volatile storage_type * addr,
		     typename Tr::member_type old,
		     typename Tr::member_type val ) {
	static_assert( Tr::VL == 1, "CAS applies to scalar values only" );
	assert( 0 && "NYI" );
    }
};

template<typename T>
struct is_array_encoding_zero : public std::false_type { };

template<typename StoredTy>
struct is_array_encoding_zero<array_encoding_zero<StoredTy>>
    : public std::true_type { };

template<typename T>
constexpr bool is_array_encoding_zero_v = is_array_encoding_zero<T>::value;

// Permuted encoding: array elements are re-ordered such that for every
// two aligned vectors of length VL, the array elements are interleaved
// in even/odd fashion: first the VL elements at even positions in 2*VL
// elements are stored, then the VL elements at odd positions.
template<typename StoredTy, unsigned short VL, typename Enable = void>
struct array_encoding_permute {
    static_assert( ( VL & ( VL - 1 ) ) == 0, "VL must be power of 2" );
    
    using stored_type = StoredTy;
    using storage_type = StoredTy;

    template<unsigned short VL_>
    using stored_traits = vector_type_traits_vl<stored_type, VL_>;

    static mmap_ptr<stored_type>
    allocate( size_t num_elems, const numa_allocation && kind ) {
	return mmap_ptr<stored_type>( num_elems, kind );
    }
    static mmap_ptr<stored_type>
    allocate( size_t num_elems, const numa_allocation & kind ) {
	return mmap_ptr<stored_type>( num_elems, kind );
    }
    static mmap_ptr<stored_type>
    allocate( numa_allocation_partitioned kind ) {
	// TODO: scale to match NUMA boundaries
	return mmap_ptr<stored_type>( kind );
    }

    template<typename index_type>
    static stored_type
    get( const storage_type * base, index_type idx ) {
	return base[permute(idx)];
    }

    template<typename Tr, typename index_type>
    static typename Tr::type
    load( storage_type * base, index_type idx ) {
	using traits = typename Tr::traits;
	using itraits = typename traits::int_traits;

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code
	    return array_encoding<stored_type>::template load<Tr>(
		base, permute_scalar( idx ) );
	}

	assert( (idx & (index_type(VL)-1)) == 0 && "alignment" );

	if constexpr ( Tr::W == stored_traits<Tr::VL>::W ) {
	    if constexpr ( Tr::VL > VL ) {
		// Shuffle within vector with even/odd interleaving, e.g.,
		// ( a, b, c, d ) -> ( a, c, b, d )
		using wt = stored_traits<Tr::VL>;
		auto raw = wt::load( &base[idx] );
		auto perm = wt::template permute_inv_evenodd<VL>( raw );
		return perm;
	    }

	    if constexpr ( Tr::W == 4 && Tr::VL == VL ) {
		using st = stored_traits<Tr::VL>;
		using wt = stored_traits<Tr::VL*2>;
		using tt = vector_type_traits_vl<int_type_of_size_t<Tr::W*2>,
						 Tr::VL>;
		index_type ridx = idx;
		if( idx & index_type(VL) ) {
		    ridx -= index_type(VL);
		}
		auto raw = wt::load( &base[ridx] );
		auto w = wt::castint( raw );
		if( idx & index_type(VL) )
		    w = tt::srli( w, 32 );
		auto n = conversion_traits<
		    uint64_t, uint32_t, Tr::VL>::convert( w );
		auto ns = st::int_traits::castfp( n );
		return ns;
	    }
	}

	if constexpr ( Tr::W == 2 * stored_traits<Tr::VL>::W ) {
	    // Careful with subtracting 1 as index_type may be unsigned and 0U-1
	    // is a very large number
	    if( idx & index_type(VL) ) { // upper half
		auto raw = itraits::loadu(
		    reinterpret_cast<typename itraits::member_type *>(
			&base[idx-index_type(VL)+1] ) );
		return array_encoding_wide<stored_type>::
		    template convert_wide<Tr>( raw );
	    } else {
		auto raw = itraits::loadu(
		    reinterpret_cast<typename itraits::member_type *>(
			&base[idx] ) );
		return array_encoding_wide<stored_type>::
		    template convert_wide<Tr>( raw );
	    }
	}

	assert( 0 && "NYI" );
	return traits::setzero();
    }

    // Complicated: vectors may be folded over
    template<typename Tr, typename index_type>
    static typename Tr::type
    loadu( storage_type * base, index_type idx ) {
	// Scalar loads are always aligned to vector length
	using traits = typename Tr::traits;
	if constexpr ( Tr::VL == 1 )
	    return load<Tr>( base, idx );
	
	assert( 0 && "NYI" );
	return traits::setzero();
    }

    // Complicated / unused
    template<typename Tr, typename index_type>
    static typename Tr::type
    ntload( storage_type * base, index_type idx );

    template<typename Tr, typename index_type>
    static void
    store( storage_type * base, index_type idx,
	   typename Tr::type raw ) {
	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code
	    array_encoding<stored_type>::template store<Tr>(
		base, permute_scalar( idx ), raw );
	    return;
	}

	// Required for al non-scalar code
	assert( (idx & (index_type(VL)-1)) == 0 && "alignment" );

	if constexpr ( Tr::W == stored_traits<Tr::VL>::W ) {
	    if constexpr ( Tr::VL > VL ) {
		// Shuffle within vector with even/odd interleaving, e.g.,
		// ( a, b, c, d ) -> ( a, c, b, d )
		// assert( (idx & (((Tr::VL/VL)-1)*index_type(VL)) ) == 0 );
		using wt = stored_traits<Tr::VL>;
		auto perm = wt::template permute_evenodd<VL>( raw );
		wt::store( &base[idx], perm );
		return;
	    }

/*
	    if constexpr ( Tr::W == 4 && Tr::VL == 4 * VL ) {
		assert( (idx & index_type(VL*3)) == 0 );
		using wt = stored_traits<Tr::VL>;
#if 0
		const uint32_t * shuf = avx512_4x16_evenodd_intlv_epi32_vl4;
		const auto mask = wt::load( shuf );
		const auto perm = _mm512_permutexvar_epi32( mask, raw );
#endif
		auto perm = wt::template permute_evenodd<VL>( raw );
		wt::store( &base[idx], perm );
		return;
	    }
*/
	    
	    if constexpr ( Tr::W == 4 && Tr::VL == VL ) {
		using wt = stored_traits<Tr::VL*2>;
		using tt = vector_type_traits_vl<int_type_of_size_t<Tr::W*2>,
						 Tr::VL>;
		using mt = typename wt::mtraits;
		if constexpr ( std::is_integral_v<stored_type> ) {
		    auto w = conversion_traits<
			uint32_t, uint64_t, Tr::VL>::convert( raw );
		    index_type ridx = idx;
		    auto mask = mt::setalternating();
		    if( idx & index_type(VL) ) {
			ridx -= index_type(VL);
			mask = mt::logical_invert( mask );
			w = tt::slli( w, 32 );
		    }
		    auto s = wt::load( &base[ridx] );
		    auto m = wt::blendm( mask, s, w );
		    wt::store( &base[ridx], m );
		} else {
		    using st = stored_traits<Tr::VL>;
		    auto w = conversion_traits<
			uint32_t, uint64_t, Tr::VL>::convert( st::castint( raw ) );
		    index_type ridx = idx;
		    auto mask = mt::setalternating();
		    if( idx & index_type(VL) ) {
			ridx -= index_type(VL);
			mask = mt::logical_invert( mask );
			w = tt::slli( w, 32 );
		    }
		    auto ws = wt::int_traits::castfp( w );
		    auto s = wt::load( &base[ridx] );
		    auto m = wt::blendm( mask, s, ws );
		    wt::store( &base[ridx], m );
		}
		return;
	    }
	} else if constexpr ( Tr::VL == VL ) {
	    assert( (idx & (index_type(VL)-1)) == 0 && "alignment" );
	    using ntraits = stored_traits<Tr::VL*2>;
	    using traits = typename Tr::traits;
	    using itraits = typename traits::int_traits;
	    // Rounding
	    // const auto off = traits::set1( double(1)/(double)(long(1)<<24) );
	    // raw = traits::add( off, raw );
	    if( idx & index_type(VL) ) { // upper half
		auto nrw = array_encoding_wide<stored_type>::
		    template convert_narrow<Tr,true>( traits::castint( raw ) );
		auto other = ntraits::load( &base[idx-index_type(VL)] );
		auto mask = ntraits::mtraits::setalternating();
		auto set = ntraits::blend( mask, nrw, other );
		ntraits::store( &base[idx-index_type(VL)], set );
	    } else {
		auto nrw = array_encoding_wide<stored_type>::
		    template convert_narrow<Tr,false>( traits::castint( raw ) );
		auto other = ntraits::load( &base[idx] );
		auto mask = ntraits::mtraits::setalternating();
		auto set = ntraits::blend( mask, other, nrw );
		ntraits::store( &base[idx], set );
	    }
	    return;
	}
	assert( 0 && "NYI" );
    }

    template<typename Tr, typename index_type>
    static void
    storeu( storage_type * base, index_type idx,
	    typename Tr::type raw );

    template<typename Tr, typename index_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx ) {
	using traits = vector_type_traits_vl<typename Tr::member_type, Tr::VL>;
	using itraits = typename traits::int_traits;
	auto cidx = permute_vector<Tr>( idx );

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code
	    return array_encoding<stored_type>::template load<Tr>(
		base, permute_scalar( idx ) );
	}

	if constexpr ( Tr::W != sizeof(stored_type) ) {
#if ALT_WIDEN
	    using st = stored_traits<Tr::VL>;
	    using it = typename Tr::traits::int_traits;
	    using xt = vector_type_traits_vl<
		int_type_of_size_t<sizeof(index_type)/Tr::VL>,Tr::VL>;
	    constexpr unsigned short lg = ilog2(Tr::W) - ilog2(st::W);
	    const auto m = xt::srli( xt::setone(), xt::W*8-lg );
	    auto algn = xt::srli( idx, lg );
	    auto a = it::gather(
		reinterpret_cast<const typename it::member_type *>( base ),
		algn );
	    auto sh = xt::slli( xt::bitwise_and( idx, m ), 4 );
	    auto adj = it::srlv( a, sh );
	    auto raw = adj;
#else
	    // Ensure meaningful data is in bottom half of lanes
	    auto raw =
		itraits::template gather_w<stored_traits<Tr::VL>::W>(
		    reinterpret_cast<typename itraits::member_type *>( base ),
		    cidx );
#endif

	    return array_encoding_wide<stored_type>::
		template convert_wide<Tr>( raw );
	} else {
	    // Default - no specific handling implemented
	    auto raw = stored_traits<Tr::VL>::gather( base, cidx );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	}
    }

    template<typename Tr, typename index_type, typename mask_type>
    static typename Tr::type
    gather( storage_type * base, index_type idx, mask_type mask ) {
	using traits = vector_type_traits_vl<typename Tr::member_type, Tr::VL>;
	using itraits = typename traits::int_traits;
	auto cidx = permute_vector<Tr>( idx );

	if constexpr ( Tr::VL == 1 ) { // scalar operations
	    // Default code
	    return array_encoding<stored_type>::template load<Tr>(
		base, permute_scalar( idx ) );
	}

	if constexpr ( Tr::W > sizeof(stored_type) ) {
#if ALT_WIDEN
	    using st = stored_traits<Tr::VL>;
	    using it = typename Tr::traits::int_traits;
	    using xt = vector_type_traits_vl<
		int_type_of_size_t<sizeof(index_type)/Tr::VL>,Tr::VL>;
	    constexpr unsigned short lg = ilog2(Tr::W) - ilog2(st::W);
	    const auto m = xt::srli( xt::setone(), xt::W*8-lg );
	    auto algn = xt::srli( idx, lg );
	    auto a = it::gather(
		reinterpret_cast<const typename it::member_type *>( base ),
		algn );
	    auto sh = xt::slli( xt::bitwise_and( idx, m ), 4 );
	    auto adj = it::srlv( a, sh );
	    auto raw = adj;
#else
	    // Ensure meaningful data is in bottom half of lanes
	    auto raw =
		itraits::template gather_w<sizeof(stored_type)>(
		    reinterpret_cast<typename itraits::member_type *>( base ),
		    cidx, mask );
#endif
	    auto t = array_encoding_wide<stored_type>::
		template convert_wide<Tr>( raw );
	    return t;
	} else {
	    // Default - no specific handling implemented
	    auto raw = stored_traits<Tr::VL>::gather( base, cidx, mask );
	    return conversion_traits<
		typename stored_traits<Tr::VL>::member_type,
		typename Tr::member_type,
		Tr::VL>::convert( raw );
	}
    }

    template<typename Tr, typename index_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val );

    template<typename Tr, typename index_type, typename mask_type>
    static void
    scatter( storage_type * base, index_type idx, typename Tr::type val,
	     mask_type mask );

    [[deprecated("replaced by template version for correctness")]]
    static bool cas( volatile storage_type * addr,
		     stored_type old, stored_type val ) {
	return stored_traits<1>::cas( addr, old, val );
    }

    template<typename Tr>
    static bool cas( volatile storage_type * addr,
		     typename Tr::member_type old,
		     typename Tr::member_type val ) {
	static_assert( Tr::VL == 1, "CAS applies to scalar values only" );
	assert( 0 && "NYI" );
    }

//private:
    template<typename index_type>
    static index_type permute_scalar( index_type idx ) {
	constexpr unsigned short logVL = ilog2(VL);

	constexpr index_type mask = index_type(VL<<1) - 1;
	constexpr index_type maskhi = mask >> 1;
	constexpr index_type mask1 = mask - maskhi;
	index_type bit = ( idx & mask1 ) >> logVL;
	index_type hi = ( idx & maskhi ) << 1;
	index_type fin = ( idx & ~mask ) | hi | bit;
/*
	constexpr index_type mask = index_type(VL) - 1;
	constexpr index_type maskhi = mask - 1;
	constexpr index_type mask1 = 1;
	index_type bit = ( idx & mask1 ) << logVL;
	index_type hi = ( idx & maskhi ) >> 1;
	index_type fin = ( idx & ~mask ) | hi | bit;
*/
	return fin;
    }
    template<typename Tr, typename index_type>
    static index_type
    permute_vector( index_type idx ) {
	using it = vector_type_traits_vl<
	    int_type_of_size_t<sizeof(index_type)/Tr::VL>,Tr::VL>;

	// Specialisation for specific vector lengths using LUT
	if constexpr ( it::W == 4 && ( it::vlen == 8 || it::vlen == 4 )
		       && ( VL == 4 || VL == 8 ) ) {
#if __AVX2__
	    using at = vector_type_traits_vl<typename it::member_type,8>;
	    typename at::type widx;
	    if constexpr ( it::vlen == 4 ) // adjust to work with wider idx
		widx = _mm256_castsi128_si256( idx );
	    else
		widx = idx;
	    const auto nibble_mask = at::srli( at::setone(), 8*at::W-4 );
	    // important to unset MSB in each byte before translation, otherwise
	    // the byte is zeroed out.
	    auto nibble = at::bitwise_and( widx, nibble_mask );
	    typename at::type lut;
	    if constexpr ( VL == 4 )
		lut = at::load(
		    reinterpret_cast<const uint32_t*>(
			avx2_1x32_array_encoding_permute_lut_vl4 ) );
	    else
		lut = at::load(
		    reinterpret_cast<const uint32_t*>(
			avx512_1x32_array_encoding_permute_lut_vl8 ) );
	    auto xlat = _mm256_shuffle_epi8( lut, nibble );
#if __AVX512F__ && __AVX512VL__
	    auto fin = at::template ternary<0b11001010>( nibble_mask, xlat, widx );
#else
	    // As already masked out nibbles before translation, no need
	    // to do it again
	    auto lo = xlat; // at::bitwise_and( nibble_mask, xlat );
	    auto hi = at::bitwise_andnot( nibble_mask, widx );
	    auto fin = at::bitwise_or( lo, hi );
#endif
	    if constexpr ( it::vlen == 4 )
		return _mm256_castsi256_si128( fin );
	    else
		return fin;
#endif
	}

	if constexpr ( it::W == 4 && it::vlen == 16 && VL == 8 ) {
#if __AVX512F__
	    using at = vector_type_traits_vl<typename it::member_type,16>;
	    const auto nibble_mask = at::srli( at::setone(), 8*at::W-4 );
	    auto nibble = at::bitwise_and( idx, nibble_mask );
	    const auto lut = at::load(
		reinterpret_cast<const uint32_t*>(
		    avx512_1x32_array_encoding_permute_lut_vl8 ) );
	    auto xlat = _mm512_shuffle_epi8( lut, nibble );
#if __AVX512F__ && __AVX512VL__
	    auto fin = at::template ternary<0b11001010>( nibble_mask, xlat, idx );
#else
	    auto lo = xlat; // at::bitwise_and( nibble_mask, xlat );
	    auto hi = at::bitwise_andnot( nibble_mask, idx );
	    auto fin = at::bitwise_or( lo, hi );
#endif
	    return fin;
#endif
	}

	constexpr unsigned short logVL = ilog2(VL);
	auto mask = it::srli( it::setone(), 8*it::W-logVL-1 );
	auto maskhi = it::srli( it::setone(), 8*it::W-logVL );
	auto mask1 = it::bitwise_andnot( maskhi, mask );
	auto bit = it::srli( it::bitwise_and( idx, mask1 ), logVL );
	auto hi = it::slli( it::bitwise_and( idx, maskhi ), 1 );
	auto fin = it::bitwise_or(
	    it::bitwise_or(
		it::bitwise_andnot( mask, idx ),
		hi ),
	    bit );
	return fin;
    }
};


#endif // GRAPTOR_ENCODING_H
