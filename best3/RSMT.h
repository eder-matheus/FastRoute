#ifndef _RSMT_H_
#define _RSMT_H_

extern int ** V_table;
extern int ** H_table;

extern void gen_brk_RSMT(Bool congestionDriven, Bool reRoute, Bool genTree, Bool newType, Bool noADJ);
extern void fluteNormal(int netID, int d, DTYPE x[], DTYPE y[], int acc, float coeffV, Tree *t);
extern void fluteCongest(int netID, int d, DTYPE x[], DTYPE y[], int acc, float coeffV, Tree *t);

#endif /* _RSMT_H_ */
