/*****************************************************************************
Copyright (c) 1994 - 2010, Lawrence Livermore National Security, LLC.
LLNL-CODE-425250.
All rights reserved.

This file is part of Silo. For details, see silo.llnl.gov.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the disclaimer below.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the disclaimer (as noted
     below) in the documentation and/or other materials provided with
     the distribution.
   * Neither the name of the LLNS/LLNL nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
"AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This work was produced at Lawrence Livermore National Laboratory under
Contract No.  DE-AC52-07NA27344 with the DOE.

Neither the  United States Government nor  Lawrence Livermore National
Security, LLC nor any of  their employees, makes any warranty, express
or  implied,  or  assumes  any  liability or  responsibility  for  the
accuracy, completeness,  or usefulness of  any information, apparatus,
product, or  process disclosed, or  represents that its use  would not
infringe privately-owned rights.

Any reference herein to  any specific commercial products, process, or
services by trade name,  trademark, manufacturer or otherwise does not
necessarily  constitute or imply  its endorsement,  recommendation, or
favoring  by  the  United  States  Government  or  Lawrence  Livermore
National Security,  LLC. The views  and opinions of  authors expressed
herein do not necessarily state  or reflect those of the United States
Government or Lawrence Livermore National Security, LLC, and shall not
be used for advertising or product endorsement purposes.
*****************************************************************************/

/*
 
  Modifications:
    Mark C. Miller, Wed Jul 14 15:30:09 PDT 2010
    I changed '$' external arrays to '#' (int valued) extern and added a new
    test for '$' (string valued) external arrays. I also added an example
    for the kinds of nameschemes as might be used for mult-block objects. 
*/

#include <silo.h>
#include <string.h>

int main()
{
    int i;
    int P[100], U[4];
    char *N[3];
    char blockName[1024];

    /* Test a somewhat complex expression */ 
    DBnamescheme *ns2;
    DBnamescheme *ns = DBMakeNamescheme("@foo_%+03d@3-((n % 3)*(4+1)+1/2)+1");
    if (strcmp(DBGetName(ns, 25), "foo_+01") != 0)
        return 1;
    DBFreeNamescheme(ns);

    /* Test ?:: operator */
    ns = DBMakeNamescheme("@foo_%d@(n-5)?14:77:");
    if (strcmp(DBGetName(ns, 6), "foo_14") != 0)
        return 1;
    DBFreeNamescheme(ns);

    /* Test multiple conversion specifiers */
    ns = DBMakeNamescheme("|foo_%03dx%03d|n/5|n%5");
    if (strcmp(DBGetName(ns, 17), "foo_003x002") != 0)
       return 1;
    if (strcmp(DBGetName(ns, 20), "foo_004x000") != 0)
       return 1;
    if (strcmp(DBGetName(ns, 3), "foo_000x003") != 0)
       return 1;
    DBFreeNamescheme(ns);

    /* Test embedded string value results */
    ns = DBMakeNamescheme("@foo_%s@(n-5)?'master':'slave':");
    if (strcmp(DBGetName(ns, 6), "foo_master") != 0)
        return 1;
    DBFreeNamescheme(ns);

    /* Test array-based references to int valued arrays and whose
       array names are more than just one character long. */
    for (i = 0; i < 100; i++)
        P[i] = i*5;
    for (i = 0; i < 4; i++)
        U[i] = i*i;
    ns = DBMakeNamescheme("@foo_%03dx%03d@#Place[n]@#Upper[n%4]", P, U);
    if (strcmp(DBGetName(ns, 17), "foo_085x001") != 0)
        return 1;
    if (strcmp(DBGetName(ns, 18), "foo_090x004") != 0)
        return 1;
    if (strcmp(DBGetName(ns, 19), "foo_095x009") != 0)
        return 1;
    if (strcmp(DBGetName(ns, 20), "foo_100x000") != 0)
        return 1;
    if (strcmp(DBGetName(ns, 21), "foo_105x001") != 0)
        return 1;
    DBFreeNamescheme(ns);

    /* Test array-based references to char* valued array */
    N[0] = "red";
    N[1] = "green";
    N[2] = "blue";
    ns = DBMakeNamescheme("Hfoo_%sH$N[n%3]", N);
    if (strcmp(DBGetName(ns, 17), "foo_blue") != 0)
        return 1;
    if (strcmp(DBGetName(ns, 6), "foo_red") != 0)
        return 1;
    DBFreeNamescheme(ns);

    /* Test namescheme as it might be used for multi-block objects */
    /* In particular, this test creates nameschems suitable for the data
       produced by 'multi_file' test in multidir mode. */
    ns = DBMakeNamescheme("|multi_file.dir/%03d/%s%d.%s|n/36|'ucd3d'|n/36|'pdb'");
    ns2 = DBMakeNamescheme("|/block%d/mesh1|n");
    strcpy(blockName, DBGetName(ns, 123)); /* filename part */
    strcat(blockName, ":");
    strcat(blockName, DBGetName(ns2, 123)); /* blockname part */
    if (strcmp(blockName, "multi_file.dir/003/ucd3d3.pdb:/block123/mesh1") != 0)
        return 0;
    strcpy(blockName, DBGetName(ns, 0)); /* filename part */
    strcat(blockName, ":");
    strcat(blockName, DBGetName(ns2, 0)); /* blockname part */
    if (strcmp(blockName, "multi_file.dir/000/ucd3d0.pdb:/block0/mesh1") != 0)
        return 0;
    strcpy(blockName, DBGetName(ns, 287)); /* filename part */
    strcat(blockName, ":");
    strcat(blockName, DBGetName(ns2, 287)); /* blockname part */
    if (strcmp(blockName, "multi_file.dir/007/ucd3d7.pdb:/block287/mesh1") != 0)
        return 0;
    DBFreeNamescheme(ns);
    DBFreeNamescheme(ns2);

    return 0;
}
