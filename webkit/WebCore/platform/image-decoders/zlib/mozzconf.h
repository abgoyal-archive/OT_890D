
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef MOZZCONF_H
#define MOZZCONF_H

#if defined(XP_WIN) && defined(ZLIB_DLL) && !defined(MOZ_ENABLE_LIBXUL)
#undef ZLIB_DLL
#endif

#ifdef HAVE_VISIBILITY_ATTRIBUTE
#define ZEXTERN __attribute__((visibility ("default"))) extern
#endif

/* Exported Symbols */
#define zlibVersion MOZ_Z_zlibVersion
#define deflate MOZ_Z_deflate
#define deflateEnd MOZ_Z_deflateEnd
#define inflate MOZ_Z_inflate
#define inflateEnd MOZ_Z_inflateEnd
#define deflateSetDictionary MOZ_Z_deflateSetDictionary
#define deflateCopy MOZ_Z_deflateCopy
#define deflateReset MOZ_Z_deflateReset
#define deflateParams MOZ_Z_deflateParams
#define deflateBound MOZ_Z_deflateBound
#define deflatePrime MOZ_Z_deflatePrime
#define inflateSetDictionary MOZ_Z_inflateSetDictionary
#define inflateSync MOZ_Z_inflateSync
#define inflateCopy MOZ_Z_inflateCopy
#define inflateReset MOZ_Z_inflateReset
#define inflateBack MOZ_Z_inflateBack
#define inflateBackEnd MOZ_Z_inflateBackEnd
#define zlibCompileFlags MOZ_Z_zlibCompileFlags
#define compress MOZ_Z_compress
#define compress2 MOZ_Z_compress2
#define compressBound MOZ_Z_compressBound
#define uncompress MOZ_Z_uncompress
#define gzopen MOZ_Z_gzopen
#define gzdopen MOZ_Z_gzdopen
#define gzsetparams MOZ_Z_gzsetparams
#define gzread MOZ_Z_gzread
#define gzwrite MOZ_Z_gzwrite
#define gzprintf MOZ_Z_gzprintf
#define gzputs MOZ_Z_gzputs
#define gzgets MOZ_Z_gzgets
#define gzputc MOZ_Z_gzputc
#define gzgetc MOZ_Z_gzgetc
#define gzungetc MOZ_Z_gzungetc
#define gzflush MOZ_Z_gzflush
#define gzseek MOZ_Z_gzseek
#define gzrewind MOZ_Z_gzrewind
#define gztell MOZ_Z_gztell
#define gzeof MOZ_Z_gzeof
#define gzclose MOZ_Z_gzclose
#define gzerror MOZ_Z_gzerror
#define gzclearerr MOZ_Z_gzclearerr
#define adler32 MOZ_Z_adler32
#define crc32 MOZ_Z_crc32
#define deflateInit_ MOZ_Z_deflateInit_
#define deflateInit2_ MOZ_Z_deflateInit2_
#define inflateInit_ MOZ_Z_inflateInit_
#define inflateInit2_ MOZ_Z_inflateInit2_
#define inflateBackInit_ MOZ_Z_inflateBackInit_
#define inflateSyncPoint MOZ_Z_inflateSyncPoint
#define get_crc_table MOZ_Z_get_crc_table
#define zError MOZ_Z_zError

/* Extra global symbols */
#define _dist_code MOZ_Z__dist_code
#define _length_code MOZ_Z__length_code
#define _tr_align MOZ_Z__tr_align
#define _tr_flush_block MOZ_Z__tr_flush_block
#define _tr_init MOZ_Z__tr_init
#define _tr_stored_block MOZ_Z__tr_stored_block
#define _tr_tally MOZ_Z__tr_tally
#define deflate_copyright MOZ_Z_deflate_copyright
#define inflate_copyright MOZ_Z_inflate_copyright
#define inflate_fast MOZ_Z_inflate_fast
#define inflate_table MOZ_Z_inflate_table
#define z_errmsg MOZ_Z_z_errmsg
#define zcalloc MOZ_Z_zcalloc
#define zcfree MOZ_Z_zcfree
#define alloc_func MOZ_Z_alloc_func
#define free_func MOZ_Z_free_func
#define in_func MOZ_Z_in_func
#define out_func MOZ_Z_out_func

/* New as of libpng-1.2.3 */
#define adler32_combine MOZ_Z_adler32_combine
#define crc32_combine MOZ_Z_crc32_combine
#define deflateSetHeader MOZ_Z_deflateSetHeader
#define deflateTune MOZ_Z_deflateTune
#define gzdirect MOZ_Z_gzdirect
#define inflatePrime MOZ_Z_inflatePrime
#define inflateGetHeader MOZ_Z_inflateGetHeader

#endif
