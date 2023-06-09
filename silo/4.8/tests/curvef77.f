************************************************************************
* Copyright (c) 1994 - 2010, Lawrence Livermore National Security, LLC.
* LLNL-CODE-425250.
* All rights reserved.
* 
* This file is part of Silo. For details, see silo.llnl.gov.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the disclaimer below.
*    * Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the disclaimer (as noted
*      below) in the documentation and/or other materials provided with
*      the distribution.
*    * Neither the name of the LLNS/LLNL nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
* 
* THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
* "AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
* LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
* LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
* CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
* PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
* NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* This work was produced at Lawrence Livermore National Laboratory under
* Contract No.  DE-AC52-07NA27344 with the DOE.
* 
* Neither the  United States Government nor  Lawrence Livermore National
* Security, LLC nor any of  their employees, makes any warranty, express
* or  implied,  or  assumes  any  liability or  responsibility  for  the
* accuracy, completeness,  or usefulness of  any information, apparatus,
* product, or  process disclosed, or  represents that its use  would not
* infringe privately-owned rights.
* 
* Any reference herein to  any specific commercial products, process, or
* services by trade name,  trademark, manufacturer or otherwise does not
* necessarily  constitute or imply  its endorsement,  recommendation, or
* favoring  by  the  United  States  Government  or  Lawrence  Livermore
* National Security,  LLC. The views  and opinions of  authors expressed
* herein do not necessarily state  or reflect those of the United States
* Government or Lawrence Livermore National Security, LLC, and shall not
* be used for advertising or product endorsement purposes.
************************************************************************

c-------------------------------------------------------------------------
c Purpose
c
c   Demonstrate use of SILO for creating curve objects.
c
c-------------------------------------------------------------------------

      program main
      implicit none
      include "silo.inc"
      integer dbid, curveid, err, i, curve_id, dtype, npts
      real x(20), y1(20), y2(20), xin(20), yin1(20), yin2(20)

c     Create the curve x and y values...

      do i=1, 20
         x(i) = (i-1) * 3.1415927 / 20.0
         y1(i) = sin (x(i))
         y2(i) = cos (x(i))
      enddo

c     Write curve data to a PDB file...

      err = dbcreate ("curvef77.pdb", 12, 0, DB_LOCAL,
     $     "Curve Data", 10, DB_PDB, dbid)

      err = dbputcurve (dbid, "sincurve", 8, x, y1, DB_FLOAT,
     $     20, DB_F77NULL, curve_id)

      err = dbputcurve (dbid, "coscurve", 8, x, y2, DB_FLOAT,
     $     20, DB_F77NULL, curve_id)

      err = dbclose (dbid)

c     Read the pdb file...

      err = dbopen ("curvef77.pdb", 12, DB_PDB, DB_READ, dbid)
      err = dbgetcurve (dbid, "sincurve", 8, 20, xin, yin1,
     $     dtype, npts)
      err = dbgetcurve (dbid, "coscurve", 8, 20, xin, yin2,
     $     dtype, npts)
      err = dbclose (dbid)

c     Check for errors...

      do i=1,20
         if (x(i) .ne. xin(i)) then
            write (*,*) "x mismatch error at index ", i
            stop
         endif
         if (yin1(i) .ne. y1(i)) then
            write (*,*) "y1 mismatch error at index ", i
            stop
         endif
         if (yin2(i) .ne. y2(i)) then
            write (*,*) "y2 mismatch error at index ", i
            stop
         endif
      enddo

      write (*,*) "Test successful."

      stop
      end
