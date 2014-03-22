/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    etisnoop.c
          Parse ETI NI G.703 file

    Authors:
         Sergio Sagliocco <sergio.sagliocco@csp.it> 
*/



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "lib_crc.h"


#define ETINIPACKETSIZE 6144


void printbuf(char* h,int level, unsigned char* b, unsigned short int l, char* d);
void decodeFIG(unsigned char* figdata, unsigned char figlen,unsigned short int figtype, unsigned short int indent);

int main (int c, char *v[]) {

    int i,j,etifd,ret,offset;
    unsigned char p[ETINIPACKETSIZE],ficdata[32*4],streamdata[684*8];
    char desc[256];
    char prevsync[3]={0x00,0x00,0x00};
    unsigned char ficf,nst,fp,mid,ficl;
    unsigned short int fl,mnsc,crch;
    unsigned short int crc;
    unsigned int sstc;
    unsigned char scid,tpl,l1;
    unsigned short int sad[64],stl[64];

    etifd=open(v[1],O_RDONLY);

    if (etifd == -1) {
        printf("Cannot open %s\n",v[1]);
        return -1;
    }

    while (1) {

        ret=read(etifd,p,ETINIPACKETSIZE);
        if (ret!=ETINIPACKETSIZE) {
            printf("End of ETI\n");
            break;
        }

        // SYNC
        printbuf("SYNC",0,p,4,"");

        // SYNC - ERR
        if (p[0]==0xFF)
            strcpy(desc,"No error");
        else
            strcpy(desc,"Error");
        printbuf("ERR",1,p,1,desc);

        // SYNC - FSYNC

        if (memcmp(prevsync,"\x00\x00\x00",3)==0) {
            if ((memcmp(p+1,"\x07\x3a\xb6",3)==0)||(memcmp(p+1,"\xf8\xc5\x49",3)==0)) {
                strcpy(desc,"OK");
                memcpy(prevsync,p+1,3);
            }
            else {
                strcpy(desc,"Wrong FSYNC");
                memcpy(prevsync,"\x00\x00\x00",3);
            }
        } else if (memcmp(prevsync,"\x07\x3a\xb6",3)==0) {
            if (memcmp(p+1,"\xf8\xc5\x49",3)!=0) {
                strcpy(desc,"Wrong FSYNC");
                memcpy(prevsync,"\x00\x00\x00",3);
            } else {
                strcpy(desc,"OK");
                memcpy(prevsync,p+1,3);
            }
        } else if (memcmp(prevsync,"\xf8\xc5\x49",3)==0) {
            if (memcmp(p+1,"\x07\x3a\xb6",3)!=0) {
                strcpy(desc,"Wrong FSYNC");
                memcpy(prevsync,"\x00\x00\x00",3);
            } else {
                strcpy(desc,"OK");
                memcpy(prevsync,p+1,3);
            }
        }
        printbuf("Sync FSYNC",1,p+1,3,desc);

        // LIDATA
        printbuf("LDATA",0,NULL,0,"");
        // LIDATA - FC
        printbuf("FC - Frame Characterization field",1,p+4,4,"");
        // LIDATA - FC - FCT
        sprintf(desc,"%d",p[4]);
        printbuf("FCT  - Frame Count",2,p+4,1,desc);
        // LIDATA - FC - FICF
        ficf=(p[5] & 0x80) >> 7;
        if (ficf == 1) {
            sprintf(desc,"%d - FIC Information are present",ficf);
        }
        else {
            sprintf(desc,"%d - FIC Information are not present",ficf);
        }

        printbuf("FICF - Fast Information Channel Flag",2,NULL,0,desc);
        // LIDATA - FC - NST
        nst=p[5] & 0x7F;
        sprintf(desc,"%d",nst);
        printbuf("NST  - Number of streams",2,NULL,0,desc);
        // LIDATA - FC - FP
        fp=(p[6] & 0xE0) >> 5;
        sprintf(desc,"%d",fp);
        printbuf("FP   - Frame Phase",2,&fp,1,desc);
        // LIDATA - FC - MID
        mid=(p[6] & 0x18) >> 3;
        if (mid != 0)
            sprintf(desc,"Mode %d",mid);
        else 
            sprintf(desc,"Mode 4");
        printbuf("MID  - Mode Identity",2,&mid,1,desc);
        // LIDATA - FC - FL
        fl = (p[6] & 0x07) * 256 + p[7];
        sprintf(desc,"%d words",fl);
        printbuf("FL   - Frame Length",2,NULL,0,desc);

        if (ficf==0) 
            ficl=0;
        else if (mid==3)
            ficl=32;
        else
            ficl=24;

        // STC
        printbuf("STC - Stream Characterisation",1,NULL,0,"");

        for (i=0;i<nst;i++) {
            sprintf(desc,"Stream number %d",i);
            printbuf("STC  - Stream Characterisation",2,p+8+4*i,4,desc);
            scid = (p[8+4*i] & 0xFC) >> 2;
            sprintf(desc,"%d",scid);
            printbuf("SCID - Sub-channel Identifier",3,NULL,0,desc);
            sad[i] = (p[8+4*i] & 0x03) * 256 + p[9+4*i];
            sprintf(desc,"%d",sad[i]);
            printbuf("SAD  - Sub-channel Start Address",3,NULL,0,desc);
            tpl = (p[10+4*i] & 0xFC) >> 2; 

            if ((tpl & 0x20) >> 5 == 1) {
                unsigned char opt, plevel;
                char plevelstr[32];
                opt = (tpl & 0x16) >> 2;
                plevel = (tpl & 0x03);
                if (opt == 0x00) {
                    if (plevel == 0)
                        strcpy(plevelstr,"1-A, 1/4, 16 CUs");
                    else if (plevel == 1)
                        strcpy(plevelstr,"2-A, 3/8, 8 CUs");
                    else if (plevel == 2)
                        strcpy(plevelstr,"3-A, 1/2, 6 CUs");
                    else if (plevel == 3)
                        strcpy(plevelstr,"4-A, 3/4, 4 CUs");
                } else if (opt == 0x01) {
                    if (plevel == 0)
                        strcpy(plevelstr,"1-B, 4/9, 27 CUs");
                    else if (plevel == 1)
                        strcpy(plevelstr,"2-B, 4/7, 21 CUs");
                    else if (plevel == 2)
                        strcpy(plevelstr,"3-B, 4/6, 18 CUs");
                    else if (plevel == 3)
                        strcpy(plevelstr,"4-B, 4/5, 15 CUs");
                } else
                    sprintf(plevelstr,"Unknown option %d",opt);
                sprintf(desc,"0x%02x - Equal Error Protection. %s",tpl,plevelstr);
            } else {
                unsigned char tsw,uepidx;
                tsw = (tpl & 0x08);
                uepidx = tpl & 0x07;
                sprintf(desc,"0x%02x - Unequal Error Protection. Table switch %d, UEP index %d",tpl,tsw,uepidx);
            }
            printbuf("TPL  - Sub-channel Type and Protection Level",3,NULL,0,desc);
            stl[i] = (p[10+4*i] & 0x03) * 256 + p[11+4*i];
            sprintf(desc,"%d => %d kbit/s",stl[i],stl[i]*8/3);
            printbuf("STL  - Sub-channel Stream Length",3,NULL,0,desc);
        }

        // EOH
        printbuf("EOH - End Of Header",1,p+8+4*nst,4,"");
        mnsc = p[8+4*nst]*256+p[8+4*nst+1];
        printbuf("MNSC - Multiplex Network Signalling Channel",2,p+8+4*nst,2,"");

        crch = p[8+4*nst+2]*256+p[8+4*nst+3];
        crc=0xffff;

        for (i=4;i<8+4*nst+2;i++)
            crc=update_crc_ccitt(crc,p[i]);
        crc=~crc;
        if (crc == crch) 
            sprintf(desc,"CRC OK");
        else
            sprintf(desc,"CRC Mismatch: %02x",crc);

        printbuf("Header CRC",2,p+8+4*nst+2,2,desc);

        // MST - FIC
        if (ficf==1) {
            int endmarker=0,figcount=0;
            unsigned char *fib, *fig;
            unsigned short int figcrc;

            memcpy(ficdata,p+12+4*nst,ficl*4);
            sprintf(desc,"FIC Data (%d bytes)",ficl*4);
            //printbuf(desc,1,ficdata,ficl*4,"");
            printbuf(desc,1,NULL,0,"");
            fib=p+12+4*nst;
            for(i=0;i<ficl*4/32;i++) {
                fig=fib;
                endmarker=0;
                figcount=0;
                while (!endmarker) {
                    unsigned char figtype,figext,figlen;
                    figtype = (fig[0] & 0xE0) >> 5;
                    if (figtype != 7) {
                        figlen = fig[0] & 0x1F;
                        sprintf(desc,"FIG %d [%d bytes]",figtype,figlen);
                        printbuf(desc,3,fig+1,figlen,"");
                        decodeFIG(fig+1,figlen,figtype,4);
                        fig+=(figlen+1);
                        figcount+=(figlen+1);
                        if (figcount >= 29)
                            endmarker=1;
                    } else
                        endmarker=1;
                }
                figcrc=fib[30]*256+fib[31];
                crc=0xffff;
                for (j=0;j<30;j++)
                    crc=update_crc_ccitt(crc,fib[j]);
                crc=~crc;
                if (crc == figcrc)
                    sprintf(desc,"FIB CRC OK");
                else
                    sprintf(desc,"FIB CRC Mismatch: %02x",crc);

                printbuf("FIB CRC",3,fib+30,2,desc);
                fib+=32;
            }
        }

        offset=0;
        for (i=0;i<nst;i++) {
            memcpy(streamdata,p+12+4*nst+ficf*ficl*4+offset,stl[i]*8);
            offset+=stl[i]*8;
            sprintf(desc,"%d",i);
            printbuf("Stream Data",1,streamdata,stl[i]*8,desc);
        }

        // EOF
        crch = p[12+4*nst+ficf*ficl*4+offset]*256+p[12+4*nst+ficf*ficl*4+offset+1];

        crc=0xffff;

        for (i=12+4*nst;i<12+4*nst+ficf*ficl*4+offset;i++)
            crc=update_crc_ccitt(crc,p[i]);
        crc=~crc;
        if (crc == crch)
            sprintf(desc,"CRC OK");
        else
            sprintf(desc,"CRC Mismatch: %02x",crc);

        printbuf("EOF",1,p+12+4*nst+ficf*ficl*4+offset,4,"");
        printbuf("CRC",2,p+12+4*nst+ficf*ficl*4+offset,2,desc);

        //RFU
        printbuf("RFU",2,p+12+4*nst+ficf*ficl*4+offset+2,2,"");     
        //TIST
        l1 = (p[12+4*nst+ficf*ficl*4+offset+5] & 0xfe) >> 1;
        sprintf(desc,"%d ms",l1*8);
        printbuf("TIST - Time Stamp",1,p+12+4*nst+ficf*ficl*4+offset+4,4,desc);





        printf("-------------------------------------------------------------------------------------------------------------\n");    
    }

}

void printbuf(char* h,int level, unsigned char* b, unsigned short int l, char* d) {
    int i;

    for (i=0;i<level;i++) printf("\t");
    printf("%s",h);
    if (l!=0)
        printf(": ");
    for (i=0;i<l;i++) {
        printf("%02x ",b[i]);
    }
    if (strcmp(d,"") != 0)
        printf(" [%s] ",d);
    printf("\n");

}


void decodeFIG(unsigned char* f, unsigned char figlen,unsigned short int figtype, unsigned short int indent) {

    char desc[256];
    int i,j;

    switch (figtype) {

        case 0: {
                    unsigned short int ext,cn,oe,pd;

                    cn = (f[0] & 0x80) >> 7;
                    oe = (f[0] & 0x40) >> 6;
                    pd = (f[0] & 0x20) >> 5;
                    ext = f[0] & 0x1F;
                    sprintf(desc,"FIG %d/%d: C/N=%d OE=%d P/D=%d",figtype,ext,cn,oe,pd);
                    printbuf(desc,indent,f+1,figlen-1,"");
                    switch (ext) {

                        case 0: {
                                    unsigned char cid,al,ch,hic,lowc,occ;
                                    unsigned short int eid,eref;

                                    eid = f[1]*256+f[2]; 
                                    cid = (f[1] & 0xF0) >> 4;
                                    eref = (f[1] & 0x0F)*256 + f[2];
                                    ch  = (f[3] & 0xC0) >> 6;
                                    al  = (f[3] & 0x20) >> 5;
                                    hic = f[3] & 0x1F;
                                    lowc = f[4];
                                    if (ch != 0) { 
                                        occ = f[5];
                                        sprintf(desc,"Ensamble ID=0x%02x (Country id=%d, Ensamble reference=%d), Change flag=%d, Alarm flag=%d, CIF Count=%d/%d, Occurance change=%d",eid,cid,eref,ch,al,hic,lowc,occ);
                                    } else
                                        sprintf(desc,"Ensamble ID=0x%02x (Country id=%d, Ensamble reference=%d), Change flag=%d, Alarm flag=%d, CIF Count=%d/%d",eid,cid,eref,ch,al,hic,lowc);
                                    printbuf(desc,indent+1,NULL,0,"");

                                }
                                break;
                        case 2: {
                                    unsigned short int sref, scid, sid;
                                    unsigned char cid, ecc, local, caid, ncomp, timd, ps, ca, subchid, scty;
                                    int k=1;
                                    char psdesc[16];
                                    char sctydesc[32];

                                    while (k<figlen) {
                                        if (pd == 0) {
                                            sid = f[k] * 256 + f[k+1];
                                            cid = (f[k] & 0xF0) >> 4;
                                            sref = (f[k] & 0x0F) * 256 + f[k+1];
                                            k=k+2;
                                        } else {
                                            sid = f[k] * 256 * 256 * 256 + f[k+1] * 256 * 256 + f[k+2] * 256 + f[k+3];
                                            ecc = f[k];
                                            cid = (f[k+1] & 0xF0) >> 4;
                                            sref = (f[k+1] & 0x0F) * 256 * 256 + f[k+2] * 256 + f[k+3];
                                            k=k+4;
                                        }

                                        local = (f[k] & 0x80) >> 7;
                                        caid  = (f[k] & 0x70) >> 4;
                                        ncomp = f[k] & 0x0F;

                                        if (pd == 0)
                                            sprintf(desc,"Service ID=0x%02X (Country id=%d, Service referemce=%d), Number of components=%d, Local flag=%d, CAID=%d",sid,cid,sref,ncomp,local,caid);
                                        else
                                            sprintf(desc,"Service ID=0x%02X (ECC=%d, Country id=%d, Service referemce=%d), Number of components=%d, Local flag=%d, CAID=%d",sid,ecc,cid,sref,ncomp,local,caid);
                                        printbuf(desc,indent+1,NULL,0,"");

                                        k++;
                                        for (i=0;i<ncomp;i++) {  
                                            unsigned char scomp[2];

                                            memcpy(scomp,f+k,2);
                                            sprintf(desc,"Component[%d]",i);
                                            printbuf(desc,indent+2,scomp,2,"");
                                            timd = (scomp[0] & 0xC0) >> 6;
                                            ps = (scomp[1] & 0x02) >> 1;
                                            ca = scomp[1] & 0x01;
                                            scty = scomp[0] & 0x3F;
                                            subchid = (scomp[1] & 0xFC) >> 2;

                                            if (timd == 3) {
                                                scid = scty*64 + subchid;
                                            }

                                            if (ps == 0) 
                                                strcpy(psdesc,"Secondary service");
                                            else 
                                                strcpy(psdesc,"Primary service");


                                            if (timd == 0) {
                                                //MSC stream audio
                                                if (scty == 0)
                                                    sprintf(sctydesc,"MPEG Foreground sound (%d)",scty);
                                                else if (scty == 1)
                                                    sprintf(sctydesc,"MPEG Background sound (%d)",scty);
                                                else if (scty == 2)
                                                    sprintf(sctydesc,"Multi Chaneel sound (%d)",scty);
                                                else if (scty == 63)
                                                    sprintf(sctydesc,"AAC sound (%d)",scty);
                                                else
                                                    sprintf(sctydesc,"Unknown ASCTy (%d)",scty);

                                                sprintf(desc,"Stream audio mode, %s, %s, SubChannel ID=%02X, CA=%d",psdesc,sctydesc,subchid,ca);
                                                printbuf(desc,indent+3,NULL,0,"");
                                            } else if (timd == 1) {
                                                //MSC stream data 
                                                sprintf(sctydesc,"DSCTy=%d",scty);
                                                sprintf(desc,"Stream data mode, %s, %s, SubChannel ID=%02X, CA=%d",psdesc,sctydesc,subchid,ca);
                                                printbuf(desc,indent+3,NULL,0,"");
                                            } else if (timd == 2) {
                                                // FIDC
                                                sprintf(sctydesc,"DSCTy=%d",scty);
                                                sprintf(desc,"FIDC mode, %s, %s, Fast Information Data Channel ID=%02X, CA=%d",psdesc,sctydesc,subchid,ca);
                                                printbuf(desc,indent+3,NULL,0,"");
                                            } else if (timd == 3) {
                                                // MSC PAcket mode
                                                sprintf(desc,"MSC Packet Mode, %s, Service Component ID=%02X, CA=%d",psdesc,subchid,ca);
                                                printbuf(desc,indent+3,NULL,0,"");
                                            }
                                            k+=2;                                          
                                        }
                                    }
                                }

                                break;

                    }
                }
                break;

        case 1: {// SHORT LABELS
                    unsigned short int ext,oe,charset;
                    unsigned short int flag;
                    char label[17];


                    charset = (f[0] & 0xF0) >> 4;
                    oe = (f[0] & 0x80) >> 3;
                    ext = f[0] & 0x07;
                    sprintf(desc,"FIG %d/%d: OE=%d, Charset=%d",figtype,ext,oe,charset);
                    printbuf(desc,indent,f+1,figlen-1,"");
                    memcpy(label,f+figlen-18,16);
                    label[16]=0x00;
                    flag = f[figlen-2] * 256 +f[figlen-1];

                    switch (ext) {

                        case 0: { // ENSAMBLE LABEL
                                    unsigned short int eid;
                                    eid = f[1] * 256 + f[2];
                                    sprintf(desc,"Ensamble ID 0x%04X label: \"%s\", Short label mask: 0x%04X",eid,label,flag);
                                    printbuf(desc,indent+1,NULL,0,"");
                                }
                                break;

                        case 1: { // Programme LABEL
                                    unsigned short int sid;
                                    sid = f[1] * 256 + f[2];
                                    sprintf(desc,"Service ID 0x%04X label: \"%s\", Short label mask: 0x%04X",sid,label,flag);
                                    printbuf(desc,indent+1,NULL,0,"");
                                }
                                break;

                        case 4: { // Service Component LABEL
                                    unsigned int sid;
                                    unsigned char pd,SCIdS;
                                    pd = (f[1] & 0x80) >> 7;
                                    SCIdS = f[1] & 0x0F;
                                    if (pd==0) 
                                        sid = f[2] * 256 + f[3];
                                    else
                                        sid = f[2] * 256 * 256 * 256 + f[3] * 256 * 256 + f[4] * 256 + f[5];
                                    sprintf(desc,"Service ID  0x%08X , Service Component ID 0x%04X Short, label: \"%s\", label mask: 0x%04X",sid,SCIdS,label,flag);
                                    printbuf(desc,indent+1,NULL,0,"");
                                }
                                break;

                        case 5: { // Data Service LABEL
                                    unsigned int sid;
                                    sid = sid = f[1] * 256 * 256 * 256 + f[2] * 256 * 256 + f[3] * 256 + f[4];
                                    sprintf(desc,"Service ID 0x%08X label: \"%s\", Short label mask: 0x%04X",sid,label,flag);
                                    printbuf(desc,indent+1,NULL,0,"");
                                }
                                break;


                        case 6: { // X-PAD User Application label
                                    unsigned int sid;
                                    unsigned char pd,SCIdS,xpadapp;
                                    char xpadappdesc[16];

                                    pd = (f[1] & 0x80) >> 7;
                                    SCIdS = f[1] & 0x0F;
                                    if (pd==0) {
                                        sid = f[2] * 256 + f[3];
                                        xpadapp = f[4] & 0x1F;
                                    }
                                    else {
                                        sid = f[2] * 256 * 256 * 256 + f[3] * 256 * 256 + f[4] * 256 + f[5];
                                        xpadapp = f[6] & 0x1F;
                                    }

                                    if (xpadapp==2)
                                        strcpy(xpadappdesc,"DLS");
                                    else if (xpadapp==12)
                                        strcpy(xpadappdesc,"MOT");


                                    sprintf(desc,"Service ID  0x%08X , Service Component ID 0x%04X Short, X-PAD App %02X (%s), label: \"%s\", label mask: 0x%04X",sid,SCIdS,xpadapp,xpadappdesc,label,flag);
                                    printbuf(desc,indent+1,NULL,0,"");
                                }
                                break;


                    }

                }
                break;


    }

}

