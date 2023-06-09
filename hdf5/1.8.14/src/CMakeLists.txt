cmake_minimum_required (VERSION 2.8.11)
PROJECT (HDF5_SRC C CXX)

#-----------------------------------------------------------------------------
# Apply Definitions to compiler in this directory and below
#-----------------------------------------------------------------------------
add_definitions (${HDF_EXTRA_C_FLAGS})

#-----------------------------------------------------------------------------
# List Source Files
#-----------------------------------------------------------------------------
set (H5_SRCS
    ${HDF5_SRC_DIR}/H5.c
    ${HDF5_SRC_DIR}/H5checksum.c
    ${HDF5_SRC_DIR}/H5dbg.c
    ${HDF5_SRC_DIR}/H5system.c
    ${HDF5_SRC_DIR}/H5timer.c
    ${HDF5_SRC_DIR}/H5trace.c
)

set (H5_HDRS
    ${HDF5_SRC_DIR}/hdf5.h
    ${HDF5_SRC_DIR}/H5api_adpt.h
    ${HDF5_SRC_DIR}/H5public.h
    ${HDF5_SRC_DIR}/H5version.h
    ${HDF5_SRC_DIR}/H5overflow.h
)
IDE_GENERATED_PROPERTIES ("H5" "${H5_HDRS}" "${H5_SRCS}" )

set (H5A_SRCS
    ${HDF5_SRC_DIR}/H5A.c
    ${HDF5_SRC_DIR}/H5Abtree2.c
    ${HDF5_SRC_DIR}/H5Adense.c
    ${HDF5_SRC_DIR}/H5Adeprec.c
    ${HDF5_SRC_DIR}/H5Aint.c
    ${HDF5_SRC_DIR}/H5Atest.c
)

set (H5A_HDRS
    ${HDF5_SRC_DIR}/H5Apkg.h
    ${HDF5_SRC_DIR}/H5Apublic.h
)
IDE_GENERATED_PROPERTIES ("H5A" "${H5A_HDRS}" "${H5A_SRCS}" )

set (H5AC_SRCS
    ${HDF5_SRC_DIR}/H5AC.c
)

set (H5AC_HDRS
    ${HDF5_SRC_DIR}/H5ACpkg.h
    ${HDF5_SRC_DIR}/H5ACpublic.h
)
IDE_GENERATED_PROPERTIES ("H5AC" "${H5AC_HDRS}" "${H5AC_SRCS}" )

set (H5B_SRCS
    ${HDF5_SRC_DIR}/H5B.c
    ${HDF5_SRC_DIR}/H5B2.c
    ${HDF5_SRC_DIR}/H5B2cache.c
    ${HDF5_SRC_DIR}/H5B2dbg.c
    ${HDF5_SRC_DIR}/H5B2hdr.c
    ${HDF5_SRC_DIR}/H5B2int.c
    ${HDF5_SRC_DIR}/H5B2stat.c
    ${HDF5_SRC_DIR}/H5B2test.c
    ${HDF5_SRC_DIR}/H5Bcache.c
    ${HDF5_SRC_DIR}/H5Bdbg.c
)

set (H5B_HDRS
    ${HDF5_SRC_DIR}/H5B2pkg.h
    ${HDF5_SRC_DIR}/H5B2public.h
    ${HDF5_SRC_DIR}/H5Bpkg.h
    ${HDF5_SRC_DIR}/H5Bpublic.h
)
IDE_GENERATED_PROPERTIES ("H5B" "${H5B_HDRS}" "${H5B_SRCS}" )

set (H5D_SRCS
    ${HDF5_SRC_DIR}/H5D.c
    ${HDF5_SRC_DIR}/H5Dcompact.c
    ${HDF5_SRC_DIR}/H5Dbtree.c
    ${HDF5_SRC_DIR}/H5Dchunk.c
    ${HDF5_SRC_DIR}/H5Dcontig.c
    ${HDF5_SRC_DIR}/H5Ddbg.c
    ${HDF5_SRC_DIR}/H5Ddeprec.c
    ${HDF5_SRC_DIR}/H5Defl.c
    ${HDF5_SRC_DIR}/H5Dfill.c
    ${HDF5_SRC_DIR}/H5Dint.c
    ${HDF5_SRC_DIR}/H5Dio.c
    ${HDF5_SRC_DIR}/H5Dlayout.c
    ${HDF5_SRC_DIR}/H5Dmpio.c
    ${HDF5_SRC_DIR}/H5Doh.c
    ${HDF5_SRC_DIR}/H5Dscatgath.c
    ${HDF5_SRC_DIR}/H5Dselect.c
    ${HDF5_SRC_DIR}/H5Dtest.c
)

set (H5D_HDRS
    ${HDF5_SRC_DIR}/H5Dpkg.h
    ${HDF5_SRC_DIR}/H5Dpublic.h
)
IDE_GENERATED_PROPERTIES ("H5D" "${H5D_HDRS}" "${H5D_SRCS}" )

set (H5E_SRCS
    ${HDF5_SRC_DIR}/H5E.c
    ${HDF5_SRC_DIR}/H5Edeprec.c
    ${HDF5_SRC_DIR}/H5Eint.c
)

set (H5E_HDRS
    ${HDF5_SRC_DIR}/H5Edefin.h
    ${HDF5_SRC_DIR}/H5Einit.h
    ${HDF5_SRC_DIR}/H5Epkg.h
    ${HDF5_SRC_DIR}/H5Epubgen.h
    ${HDF5_SRC_DIR}/H5Epublic.h
    ${HDF5_SRC_DIR}/H5Eterm.h
)
IDE_GENERATED_PROPERTIES ("H5E" "${H5E_HDRS}" "${H5E_SRCS}" )

set (H5F_SRCS
    ${HDF5_SRC_DIR}/H5F.c
    ${HDF5_SRC_DIR}/H5Fint.c
    ${HDF5_SRC_DIR}/H5Faccum.c
    ${HDF5_SRC_DIR}/H5Fcwfs.c
    ${HDF5_SRC_DIR}/H5Fdbg.c
    ${HDF5_SRC_DIR}/H5Fefc.c
    ${HDF5_SRC_DIR}/H5Ffake.c
    ${HDF5_SRC_DIR}/H5Fio.c
    ${HDF5_SRC_DIR}/H5Fmount.c
    ${HDF5_SRC_DIR}/H5Fmpi.c
    ${HDF5_SRC_DIR}/H5Fquery.c
    ${HDF5_SRC_DIR}/H5Fsfile.c
    ${HDF5_SRC_DIR}/H5Fsuper.c
    ${HDF5_SRC_DIR}/H5Fsuper_cache.c
    ${HDF5_SRC_DIR}/H5Ftest.c
)

set (H5F_HDRS
    ${HDF5_SRC_DIR}/H5Fpkg.h
    ${HDF5_SRC_DIR}/H5Fpublic.h
)
IDE_GENERATED_PROPERTIES ("H5F" "${H5F_HDRS}" "${H5F_SRCS}" )

set (H5FD_SRCS
    ${HDF5_SRC_DIR}/H5FD.c
    ${HDF5_SRC_DIR}/H5FDcore.c
    ${HDF5_SRC_DIR}/H5FDdirect.c
    ${HDF5_SRC_DIR}/H5FDfamily.c
    ${HDF5_SRC_DIR}/H5FDint.c
    ${HDF5_SRC_DIR}/H5FDlog.c
    ${HDF5_SRC_DIR}/H5FDmpi.c
    ${HDF5_SRC_DIR}/H5FDmpio.c
    ${HDF5_SRC_DIR}/H5FDmulti.c
    ${HDF5_SRC_DIR}/H5FDsec2.c
    ${HDF5_SRC_DIR}/H5FDspace.c
    ${HDF5_SRC_DIR}/H5FDstdio.c
)

set (H5FD_HDRS
    ${HDF5_SRC_DIR}/H5FDcore.h
    ${HDF5_SRC_DIR}/H5FDdirect.h
    ${HDF5_SRC_DIR}/H5FDfamily.h
    ${HDF5_SRC_DIR}/H5FDlog.h
    ${HDF5_SRC_DIR}/H5FDmpi.h
    ${HDF5_SRC_DIR}/H5FDmpio.h
    ${HDF5_SRC_DIR}/H5FDmulti.h
    ${HDF5_SRC_DIR}/H5FDpkg.h
    ${HDF5_SRC_DIR}/H5FDpublic.h
    ${HDF5_SRC_DIR}/H5FDsec2.h
    ${HDF5_SRC_DIR}/H5FDstdio.h
)
IDE_GENERATED_PROPERTIES ("H5FD" "${H5FD_HDRS}" "${H5FD_SRCS}" )

set (H5FS_SRCS
    ${HDF5_SRC_DIR}/H5FS.c
    ${HDF5_SRC_DIR}/H5FScache.c
    ${HDF5_SRC_DIR}/H5FSdbg.c
    ${HDF5_SRC_DIR}/H5FSsection.c
    ${HDF5_SRC_DIR}/H5FSstat.c
    ${HDF5_SRC_DIR}/H5FStest.c
)

set (H5FS_HDRS
    ${HDF5_SRC_DIR}/H5FSpkg.h
    ${HDF5_SRC_DIR}/H5FSpublic.h
)
IDE_GENERATED_PROPERTIES ("H5FS" "${H5FS_HDRS}" "${H5FS_SRCS}" )

set (H5G_SRCS
    ${HDF5_SRC_DIR}/H5G.c
    ${HDF5_SRC_DIR}/H5Gbtree2.c
    ${HDF5_SRC_DIR}/H5Gcache.c
    ${HDF5_SRC_DIR}/H5Gcompact.c
    ${HDF5_SRC_DIR}/H5Gdense.c
    ${HDF5_SRC_DIR}/H5Gdeprec.c
    ${HDF5_SRC_DIR}/H5Gent.c
    ${HDF5_SRC_DIR}/H5Gint.c
    ${HDF5_SRC_DIR}/H5Glink.c
    ${HDF5_SRC_DIR}/H5Gloc.c
    ${HDF5_SRC_DIR}/H5Gname.c
    ${HDF5_SRC_DIR}/H5Gnode.c
    ${HDF5_SRC_DIR}/H5Gobj.c
    ${HDF5_SRC_DIR}/H5Goh.c
    ${HDF5_SRC_DIR}/H5Groot.c
    ${HDF5_SRC_DIR}/H5Gstab.c
    ${HDF5_SRC_DIR}/H5Gtest.c
    ${HDF5_SRC_DIR}/H5Gtraverse.c
)

set (H5G_HDRS
    ${HDF5_SRC_DIR}/H5Gpkg.h
    ${HDF5_SRC_DIR}/H5Gpublic.h
)
IDE_GENERATED_PROPERTIES ("H5G" "${H5G_HDRS}" "${H5G_SRCS}" )

set (H5HF_SRCS
    ${HDF5_SRC_DIR}/H5HF.c
    ${HDF5_SRC_DIR}/H5HFbtree2.c
    ${HDF5_SRC_DIR}/H5HFcache.c
    ${HDF5_SRC_DIR}/H5HFdbg.c
    ${HDF5_SRC_DIR}/H5HFdblock.c
    ${HDF5_SRC_DIR}/H5HFdtable.c
    ${HDF5_SRC_DIR}/H5HFhdr.c
    ${HDF5_SRC_DIR}/H5HFhuge.c
    ${HDF5_SRC_DIR}/H5HFiblock.c
    ${HDF5_SRC_DIR}/H5HFiter.c
    ${HDF5_SRC_DIR}/H5HFman.c
    ${HDF5_SRC_DIR}/H5HFsection.c
    ${HDF5_SRC_DIR}/H5HFspace.c
    ${HDF5_SRC_DIR}/H5HFstat.c
    ${HDF5_SRC_DIR}/H5HFtest.c
    ${HDF5_SRC_DIR}/H5HFtiny.c
)

set (H5HF_HDRS
    ${HDF5_SRC_DIR}/H5HFpkg.h
    ${HDF5_SRC_DIR}/H5HFpublic.h
)
IDE_GENERATED_PROPERTIES ("H5HF" "${H5HF_HDRS}" "${H5HF_SRCS}" )

set (H5HG_SRCS
    ${HDF5_SRC_DIR}/H5HG.c
    ${HDF5_SRC_DIR}/H5HGcache.c
    ${HDF5_SRC_DIR}/H5HGdbg.c
    ${HDF5_SRC_DIR}/H5HGquery.c
)

set (H5HG_HDRS
    ${HDF5_SRC_DIR}/H5HGpkg.h
    ${HDF5_SRC_DIR}/H5HGpublic.h
)
IDE_GENERATED_PROPERTIES ("H5HG" "${H5HG_HDRS}" "${H5HG_SRCS}" )

set (H5HL_SRCS
    ${HDF5_SRC_DIR}/H5HL.c
    ${HDF5_SRC_DIR}/H5HLcache.c
    ${HDF5_SRC_DIR}/H5HLdbg.c
    ${HDF5_SRC_DIR}/H5HLint.c
)

set (H5HL_HDRS
    ${HDF5_SRC_DIR}/H5HLpkg.h
    ${HDF5_SRC_DIR}/H5HLpublic.h
)
IDE_GENERATED_PROPERTIES ("H5HL" "${H5HL_HDRS}" "${H5HL_SRCS}" )

set (H5MF_SRCS
    ${HDF5_SRC_DIR}/H5MF.c
    ${HDF5_SRC_DIR}/H5MFaggr.c
    ${HDF5_SRC_DIR}/H5MFdbg.c
    ${HDF5_SRC_DIR}/H5MFsection.c
)

set (H5MF_HDRS
)
IDE_GENERATED_PROPERTIES ("H5MF" "${H5MF_HDRS}" "${H5MF_SRCS}" )

set (H5MP_SRCS
    ${HDF5_SRC_DIR}/H5MP.c
    ${HDF5_SRC_DIR}/H5MPtest.c
)

set (H5MP_HDRS
    ${HDF5_SRC_DIR}/H5MPpkg.h
)
IDE_GENERATED_PROPERTIES ("H5MP" "${H5MP_HDRS}" "${H5MP_SRCS}" )

set (H5O_SRCS
    ${HDF5_SRC_DIR}/H5O.c
    ${HDF5_SRC_DIR}/H5Oainfo.c
    ${HDF5_SRC_DIR}/H5Oalloc.c
    ${HDF5_SRC_DIR}/H5Oattr.c
    ${HDF5_SRC_DIR}/H5Oattribute.c
    ${HDF5_SRC_DIR}/H5Obogus.c
    ${HDF5_SRC_DIR}/H5Obtreek.c
    ${HDF5_SRC_DIR}/H5Ocache.c
    ${HDF5_SRC_DIR}/H5Ochunk.c
    ${HDF5_SRC_DIR}/H5Ocont.c
    ${HDF5_SRC_DIR}/H5Ocopy.c
    ${HDF5_SRC_DIR}/H5Odbg.c
    ${HDF5_SRC_DIR}/H5Odrvinfo.c
    ${HDF5_SRC_DIR}/H5Odtype.c
    ${HDF5_SRC_DIR}/H5Oefl.c
    ${HDF5_SRC_DIR}/H5Ofill.c
    ${HDF5_SRC_DIR}/H5Oginfo.c
    ${HDF5_SRC_DIR}/H5Olayout.c
    ${HDF5_SRC_DIR}/H5Olinfo.c
    ${HDF5_SRC_DIR}/H5Olink.c
    ${HDF5_SRC_DIR}/H5Omessage.c
    ${HDF5_SRC_DIR}/H5Omtime.c
    ${HDF5_SRC_DIR}/H5Oname.c
    ${HDF5_SRC_DIR}/H5Onull.c
    ${HDF5_SRC_DIR}/H5Opline.c
    ${HDF5_SRC_DIR}/H5Orefcount.c
    ${HDF5_SRC_DIR}/H5Osdspace.c
    ${HDF5_SRC_DIR}/H5Oshared.c
    ${HDF5_SRC_DIR}/H5Oshmesg.c
    ${HDF5_SRC_DIR}/H5Ostab.c
    ${HDF5_SRC_DIR}/H5Otest.c
    ${HDF5_SRC_DIR}/H5Ounknown.c
)

set (H5O_HDRS
    ${HDF5_SRC_DIR}/H5Opkg.h
    ${HDF5_SRC_DIR}/H5Opublic.h
    ${HDF5_SRC_DIR}/H5Oshared.h
)
IDE_GENERATED_PROPERTIES ("H5O" "${H5O_HDRS}" "${H5O_SRCS}" )

set (H5P_SRCS
    ${HDF5_SRC_DIR}/H5P.c
    ${HDF5_SRC_DIR}/H5Pacpl.c
    ${HDF5_SRC_DIR}/H5Pdapl.c
    ${HDF5_SRC_DIR}/H5Pdcpl.c
    ${HDF5_SRC_DIR}/H5Pdeprec.c
    ${HDF5_SRC_DIR}/H5Pdxpl.c
    ${HDF5_SRC_DIR}/H5Pfapl.c
    ${HDF5_SRC_DIR}/H5Pfcpl.c
    ${HDF5_SRC_DIR}/H5Pfmpl.c
    ${HDF5_SRC_DIR}/H5Pgcpl.c
    ${HDF5_SRC_DIR}/H5Pint.c
    ${HDF5_SRC_DIR}/H5Plapl.c
    ${HDF5_SRC_DIR}/H5Plcpl.c
    ${HDF5_SRC_DIR}/H5Pocpl.c
    ${HDF5_SRC_DIR}/H5Pocpypl.c
    ${HDF5_SRC_DIR}/H5Pstrcpl.c
    ${HDF5_SRC_DIR}/H5Ptest.c
)

set (H5P_HDRS
    ${HDF5_SRC_DIR}/H5Ppkg.h
    ${HDF5_SRC_DIR}/H5Ppublic.h
)
IDE_GENERATED_PROPERTIES ("H5P" "${H5P_HDRS}" "${H5P_SRCS}" )

set (H5PL_SRCS
    ${HDF5_SRC_DIR}/H5PL.c
)

set (H5PL_HDRS
    ${HDF5_SRC_DIR}/H5PLextern.h
)
IDE_GENERATED_PROPERTIES ("H5PL" "${H5PL_HDRS}" "${H5PL_SRCS}" )


set (H5R_SRCS
    ${HDF5_SRC_DIR}/H5R.c
    ${HDF5_SRC_DIR}/H5Rdeprec.c
)
set (H5R_HDRS
    ${HDF5_SRC_DIR}/H5Rpkg.h
    ${HDF5_SRC_DIR}/H5Rpublic.h
)
IDE_GENERATED_PROPERTIES ("H5R" "${H5R_HDRS}" "${H5R_SRCS}" )


set (H5RC_SRCS
    ${HDF5_SRC_DIR}/H5RC.c
)
set (H5RC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5RC" "${H5RC_HDRS}" "${H5RC_SRCS}" )


set (H5RS_SRCS
    ${HDF5_SRC_DIR}/H5RS.c
)
set (H5RS_HDRS
)
IDE_GENERATED_PROPERTIES ("H5RS" "${H5RS_HDRS}" "${H5RS_SRCS}" )

set (H5S_SRCS
    ${HDF5_SRC_DIR}/H5S.c
    ${HDF5_SRC_DIR}/H5Sall.c
    ${HDF5_SRC_DIR}/H5Sdbg.c
    ${HDF5_SRC_DIR}/H5Shyper.c
    ${HDF5_SRC_DIR}/H5Smpio.c
    ${HDF5_SRC_DIR}/H5Snone.c
    ${HDF5_SRC_DIR}/H5Spoint.c
    ${HDF5_SRC_DIR}/H5Sselect.c
    ${HDF5_SRC_DIR}/H5Stest.c
)

set (H5S_HDRS
    ${HDF5_SRC_DIR}/H5Spkg.h
    ${HDF5_SRC_DIR}/H5Spublic.h
)
IDE_GENERATED_PROPERTIES ("H5S" "${H5S_HDRS}" "${H5S_SRCS}" )

set (H5SM_SRCS
    ${HDF5_SRC_DIR}/H5SM.c
    ${HDF5_SRC_DIR}/H5SMbtree2.c
    ${HDF5_SRC_DIR}/H5SMcache.c
    ${HDF5_SRC_DIR}/H5SMmessage.c
    ${HDF5_SRC_DIR}/H5SMtest.c
)

set (H5SM_HDRS
    ${HDF5_SRC_DIR}/H5SMpkg.h
)
IDE_GENERATED_PROPERTIES ("H5SM" "${H5SM_HDRS}" "${H5SM_SRCS}" )

set (H5T_SRCS
    ${HDF5_SRC_DIR}/H5T.c
    ${HDF5_SRC_DIR}/H5Tarray.c
    ${HDF5_SRC_DIR}/H5Tbit.c
    ${HDF5_SRC_DIR}/H5Tcommit.c
    ${HDF5_SRC_DIR}/H5Tcompound.c
    ${HDF5_SRC_DIR}/H5Tconv.c
    ${HDF5_SRC_DIR}/H5Tcset.c
    ${HDF5_SRC_DIR}/H5Tdbg.c
    ${HDF5_SRC_DIR}/H5Tdeprec.c
    ${HDF5_SRC_DIR}/H5Tenum.c
    ${HDF5_SRC_DIR}/H5Tfields.c
    ${HDF5_SRC_DIR}/H5Tfixed.c
    ${HDF5_SRC_DIR}/H5Tfloat.c
    ${HDF5_SRC_DIR}/H5Tnative.c
    ${HDF5_SRC_DIR}/H5Toffset.c
    ${HDF5_SRC_DIR}/H5Toh.c
    ${HDF5_SRC_DIR}/H5Topaque.c
    ${HDF5_SRC_DIR}/H5Torder.c
    ${HDF5_SRC_DIR}/H5Tpad.c
    ${HDF5_SRC_DIR}/H5Tprecis.c
    ${HDF5_SRC_DIR}/H5Tstrpad.c
    ${HDF5_SRC_DIR}/H5Tvisit.c
    ${HDF5_SRC_DIR}/H5Tvlen.c
)

set (H5T_HDRS
    ${HDF5_SRC_DIR}/H5Tpkg.h
    ${HDF5_SRC_DIR}/H5Tpublic.h
)
IDE_GENERATED_PROPERTIES ("H5T" "${H5T_HDRS}" "${H5T_SRCS}" )

set (H5Z_SRCS
    ${HDF5_SRC_DIR}/H5Z.c
    ${HDF5_SRC_DIR}/H5Zdeflate.c
    ${HDF5_SRC_DIR}/H5Zfletcher32.c
    ${HDF5_SRC_DIR}/H5Znbit.c
    ${HDF5_SRC_DIR}/H5Zscaleoffset.c
    ${HDF5_SRC_DIR}/H5Zshuffle.c
    ${HDF5_SRC_DIR}/H5Zszip.c
    ${HDF5_SRC_DIR}/H5Ztrans.c
)
if (H5_ZLIB_HEADER)
  SET_PROPERTY(SOURCE ${HDF5_SRC_DIR}/H5Zdeflate.c PROPERTY
      COMPILE_DEFINITIONS H5_ZLIB_HEADER="${H5_ZLIB_HEADER}")
endif (H5_ZLIB_HEADER)


set (H5Z_HDRS
    ${HDF5_SRC_DIR}/H5Zpkg.h
    ${HDF5_SRC_DIR}/H5Zpublic.h
)
IDE_GENERATED_PROPERTIES ("H5Z" "${H5Z_HDRS}" "${H5Z_SRCS}" )

set (common_SRCS
    ${H5_SRCS}
    ${H5A_SRCS}
    ${H5AC_SRCS}
    ${H5B_SRCS}
    ${H5D_SRCS}
    ${H5E_SRCS}
    ${H5F_SRCS}
    ${H5FD_SRCS}
    ${H5FS_SRCS}
    ${H5G_SRCS}
    ${H5HF_SRCS}
    ${H5HG_SRCS}
    ${H5HL_SRCS}
    ${H5MF_SRCS}
    ${H5MP_SRCS}
    ${H5O_SRCS}
    ${H5P_SRCS}
    ${H5PL_SRCS}
    ${H5R_SRCS}
    ${H5RC_SRCS}
    ${H5RS_SRCS}
    ${H5S_SRCS}
    ${H5SM_SRCS}
    ${H5T_SRCS}
    ${H5Z_SRCS}
    ${HDF5_SRC_DIR}/H5C.c
    ${HDF5_SRC_DIR}/H5CS.c
    ${HDF5_SRC_DIR}/H5FL.c
    ${HDF5_SRC_DIR}/H5FO.c
    ${HDF5_SRC_DIR}/H5HP.c
    ${HDF5_SRC_DIR}/H5I.c
    ${HDF5_SRC_DIR}/H5Itest.c
    ${HDF5_SRC_DIR}/H5L.c
    ${HDF5_SRC_DIR}/H5Lexternal.c
    ${HDF5_SRC_DIR}/H5MM.c
    ${HDF5_SRC_DIR}/H5R.c
    ${HDF5_SRC_DIR}/H5RC.c
    ${HDF5_SRC_DIR}/H5Rdeprec.c
    ${HDF5_SRC_DIR}/H5RS.c
    ${HDF5_SRC_DIR}/H5SL.c
    ${HDF5_SRC_DIR}/H5ST.c
    ${HDF5_SRC_DIR}/H5TS.c
    ${HDF5_SRC_DIR}/H5VM.c
    ${HDF5_SRC_DIR}/H5WB.c
)

set (H5_PUBLIC_HEADERS
    ${H5_HDRS}
    ${H5A_HDRS}
    ${H5AC_HDRS}
    ${H5B_HDRS}
    ${H5D_HDRS}
    ${H5E_HDRS}
    ${H5F_HDRS}
    ${H5FD_HDRS}
    ${H5FS_HDRS}
    ${H5G_HDRS}
    ${H5HF_HDRS}
    ${H5HG_HDRS}
    ${H5HL_HDRS}
    ${H5MF_HDRS}
    ${H5MP_HDRS}
    ${H5O_HDRS}
    ${H5P_HDRS}
    ${H5PL_HDRS}
    ${H5R_HDRS}
    ${H5S_HDRS}
    ${H5SM_HDRS}
    ${H5T_HDRS}
    ${H5Z_HDRS}
    ${HDF5_SRC_DIR}/H5Cpkg.h
    ${HDF5_SRC_DIR}/H5Cpublic.h
    ${HDF5_SRC_DIR}/H5Ipkg.h
    ${HDF5_SRC_DIR}/H5Ipublic.h
    ${HDF5_SRC_DIR}/H5Lpkg.h
    ${HDF5_SRC_DIR}/H5Lpublic.h
    ${HDF5_SRC_DIR}/H5MMpublic.h
    ${HDF5_SRC_DIR}/H5Rpkg.h
    ${HDF5_SRC_DIR}/H5Rpublic.h
)

# --------------------------------------------------------------------
# If we are compiling on Windows then add the windows specific files
# --------------------------------------------------------------------
if (WIN32)
  set (common_SRCS  ${common_SRCS}  ${HDF5_SRC_DIR}/H5FDwindows.c)
  set (H5_PUBLIC_HEADERS ${H5_PUBLIC_HEADERS}  ${HDF5_SRC_DIR}/H5FDwindows.h)
endif (WIN32)

set (H5_PRIVATE_HEADERS
    ${HDF5_SRC_DIR}/H5private.h
    ${HDF5_SRC_DIR}/H5Aprivate.h
    ${HDF5_SRC_DIR}/H5ACprivate.h
    ${HDF5_SRC_DIR}/H5B2private.h
    ${HDF5_SRC_DIR}/H5Bprivate.h
    ${HDF5_SRC_DIR}/H5CSprivate.h
    ${HDF5_SRC_DIR}/H5Dprivate.h
    ${HDF5_SRC_DIR}/H5Eprivate.h
    ${HDF5_SRC_DIR}/H5FDprivate.h
    ${HDF5_SRC_DIR}/H5Fprivate.h
    ${HDF5_SRC_DIR}/H5FLprivate.h
    ${HDF5_SRC_DIR}/H5FOprivate.h
    ${HDF5_SRC_DIR}/H5MFprivate.h
    ${HDF5_SRC_DIR}/H5MMprivate.h
    ${HDF5_SRC_DIR}/H5Cprivate.h
    ${HDF5_SRC_DIR}/H5FSprivate.h
    ${HDF5_SRC_DIR}/H5Gprivate.h
    ${HDF5_SRC_DIR}/H5HFprivate.h
    ${HDF5_SRC_DIR}/H5HGprivate.h
    ${HDF5_SRC_DIR}/H5HLprivate.h
    ${HDF5_SRC_DIR}/H5HPprivate.h
    ${HDF5_SRC_DIR}/H5Iprivate.h
    ${HDF5_SRC_DIR}/H5Lprivate.h
    ${HDF5_SRC_DIR}/H5MPprivate.h
    ${HDF5_SRC_DIR}/H5Oprivate.h
    ${HDF5_SRC_DIR}/H5Pprivate.h
    ${HDF5_SRC_DIR}/H5PLprivate.h
    ${HDF5_SRC_DIR}/H5RCprivate.h
    ${HDF5_SRC_DIR}/H5Rprivate.h
    ${HDF5_SRC_DIR}/H5RSprivate.h
    ${HDF5_SRC_DIR}/H5SLprivate.h
    ${HDF5_SRC_DIR}/H5SMprivate.h
    ${HDF5_SRC_DIR}/H5Sprivate.h
    ${HDF5_SRC_DIR}/H5STprivate.h
    ${HDF5_SRC_DIR}/H5Tprivate.h
    ${HDF5_SRC_DIR}/H5TSprivate.h
    ${HDF5_SRC_DIR}/H5VMprivate.h
    ${HDF5_SRC_DIR}/H5WBprivate.h
    ${HDF5_SRC_DIR}/H5Zprivate.h
    ${HDF5_SRC_DIR}/H5win32defs.h
)

#-----------------------------------------------------------------------------
# Setup the H5Detect utility which generates H5Tinit with platform
# specific type checks inside
#-----------------------------------------------------------------------------
add_executable (H5detect ${HDF5_SRC_DIR}/H5detect.c)
TARGET_C_PROPERTIES (H5detect " " " ")
if (MSVC OR MINGW)
  target_link_libraries (H5detect "ws2_32.lib")
endif (MSVC OR MINGW)

set (CMD $<TARGET_FILE:H5detect>)
add_custom_command (
    OUTPUT ${HDF5_BINARY_DIR}/H5Tinit.c
    COMMAND ${CMD}
    ARGS > ${HDF5_BINARY_DIR}/H5Tinit.c
    DEPENDS H5detect
)

add_executable (H5make_libsettings ${HDF5_SRC_DIR}/H5make_libsettings.c)
TARGET_C_PROPERTIES (H5make_libsettings " " " ")
if (MSVC OR MINGW)
  target_link_libraries (H5make_libsettings "ws2_32.lib")
endif (MSVC OR MINGW)

set (CMD $<TARGET_FILE:H5make_libsettings>)
add_custom_command (
    OUTPUT ${HDF5_BINARY_DIR}/H5lib_settings.c
    COMMAND ${CMD}
    ARGS > ${HDF5_BINARY_DIR}/H5lib_settings.c
    DEPENDS H5make_libsettings
    WORKING_DIRECTORY ${HDF5_BINARY_DIR}
)

if (GENERATE_ERROR_HEADERS)
  find_pacakge (Perl)
  if (PERL_FOUND)
    add_custom_command (
        OUTPUT ${HDF5_BINARY_DIR}/H5Edefin.h
        PRE_BUILD
        COMMAND ${PERL_EXECUTABLE}
        ARGS ${HDF5_SOURCE_DIR}/bin/make_err ${HDF5_SOURCE_DIR}/src/H5err.txt
        DEPENDS ${HDF5_SOURCE_DIR}/src/H5err.txt
        COMMENT " Creating err header"
    )

    add_custom_command (
        OUTPUT ${HDF5_BINARY_DIR}/H5version.h
        PRE_BUILD
        COMMAND ${PERL_EXECUTABLE}
        ARGS ${HDF5_SOURCE_DIR}/bin/make_vers ${HDF5_SOURCE_DIR}/src/H5vers.txt
        DEPENDS ${HDF5_SOURCE_DIR}/src/H5vers.txt
       COMMENT " Creating API version macro"
    )

    add_custom_command (
        OUTPUT ${HDF5_BINARY_DIR}/H5overflow.h
        PRE_BUILD
        COMMAND ${PERL_EXECUTABLE}
        ARGS ${HDF5_SOURCE_DIR}/bin/make_overflow ${HDF5_SOURCE_DIR}/src/H5overflow.txt
        DEPENDS ${HDF5_SOURCE_DIR}/src/H5overflow.txt
        COMMENT " Creating Assignment overflow macro"
    )

    add_custom_target(run_perl_scripts ALL
        DEPENDS ${HDF5_BINARY_DIR}/H5Edefin.h ${HDF5_BINARY_DIR}/H5version.h ${HDF5_BINARY_DIR}/H5overflow.h
    )
  else (PERL_FOUND)
    message (STATUS "Cannot generate headers - perl not found")
  endif (PERL_FOUND)
endif (GENERATE_ERROR_HEADERS)

#-----------------------------------------------------------------------------
# Add H5Tinit source to build - generated by H5Detect/CMake at configure time
#-----------------------------------------------------------------------------
set (common_SRCS ${common_SRCS} ${HDF5_BINARY_DIR}/H5Tinit.c)
set_source_files_properties (${HDF5_BINARY_DIR}/H5Tinit.c GENERATED)
set (common_SRCS ${common_SRCS} ${HDF5_BINARY_DIR}/H5lib_settings.c)
set_source_files_properties (${HDF5_BINARY_DIR}/H5lib_settings.c GENERATED)
set (common_SRCS ${common_SRCS} ${HDF5_BINARY_DIR}/H5Edefin.h)
set_source_files_properties (${HDF5_BINARY_DIR}/H5Edefin.h GENERATED)
set (common_SRCS ${common_SRCS} ${HDF5_BINARY_DIR}/H5version.h)
set_source_files_properties (${HDF5_BINARY_DIR}/H5version.h GENERATED)
set (common_SRCS ${common_SRCS} ${HDF5_BINARY_DIR}/H5overflow.h)
set_source_files_properties (${HDF5_BINARY_DIR}/H5overflow.h GENERATED)

add_library (${HDF5_LIB_TARGET} ${LIB_TYPE} ${common_SRCS} ${H5_PUBLIC_HEADERS} ${H5_PRIVATE_HEADERS})
TARGET_C_PROPERTIES (${HDF5_LIB_TARGET} " " " ")
target_link_libraries (${HDF5_LIB_TARGET} ${LINK_LIBS})
if (NOT WIN32)
  target_link_libraries (${HDF5_LIB_TARGET} dl)
endif (NOT WIN32)
if (H5_HAVE_PARALLEL AND MPI_C_FOUND)
  target_link_libraries (${HDF5_LIB_TARGET} ${MPI_C_LIBRARIES})
endif (H5_HAVE_PARALLEL AND MPI_C_FOUND)
set_global_variable (HDF5_LIBRARIES_TO_EXPORT ${HDF5_LIB_TARGET})
H5_SET_LIB_OPTIONS (${HDF5_LIB_TARGET} ${HDF5_LIB_NAME} ${LIB_TYPE})
set_target_properties (${HDF5_LIB_TARGET} PROPERTIES
    FOLDER libraries
    INTERFACE_INCLUDE_DIRECTORIES "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

option (HDF5_ENABLE_DEBUG_APIS "Turn on debugging in all packages" OFF)
if (HDF5_ENABLE_DEBUG_APIS)
  set_target_properties (${HDF5_LIB_TARGET} PROPERTIES
    COMPILE_DEFINITIONS
      "-DH5Z_DEBUG -DH5VM_DEBUG -DH5T_DEBUG
      -DH5S_DEBUG -DH5P_DEBUG -DH5O_DEBUG -DH5MM_DEBUG -DH5MF_DEBUG
      -DH5I_DEBUG -DH5HL_DEBUG -DH5HG_DEBUG -DH5G_DEBUG -DH5F_DEBUG
      -DH5E_DEBUG -DH5D_DEBUG -DH5B_DEBUG -DH5AC_DEBUG -UNDEBUG"
  )
endif (HDF5_ENABLE_DEBUG_APIS)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
if (NOT HDF5_INSTALL_NO_DEVELOPMENT)
  install (
      FILES
          ${H5_PUBLIC_HEADERS}
      DESTINATION
          ${HDF5_INSTALL_INCLUDE_DIR}
      COMPONENT
          headers
  )
endif (NOT HDF5_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} libraries)
  endif (BUILD_SHARED_LIBS)
  
  install (
      TARGETS
          ${HDF5_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT libraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT libraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT libraries
  )
endif (HDF5_EXPORTED_TARGETS)
