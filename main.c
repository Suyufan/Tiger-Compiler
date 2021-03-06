/*
 * main.c
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "absyn.h"
#include "errormsg.h"
#include "temp.h" /* needed by translate.h */
#include "tree.h" /* needed by frame.h */
#include "assem.h"
#include "frame.h" /* needed by translate.h and printfrags prototype */
#include "translate.h"
#include "env.h"
#include "semant.h" /* function prototype for transProg */
#include "canon.h"
#include "prabsyn.h"
#include "printtree.h"
#include "escape.h" /* needed by escape analysis */
#include "parse.h"
#include "codegen.h"
#include "regalloc.h"

extern bool anyErrors;

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, F_frame frame, T_stm body)
{
 	AS_proc proc;
 	T_stmList stmList;
 	AS_instrList iList;

 	F_tempMap = Temp_empty();
 	stmList = C_linearize(body);
 	stmList = C_traceSchedule(C_basicBlocks(stmList));
 	printStmList(stdout, stmList);
 	iList  = F_codegen(frame, stmList); /* 9 */

 	struct RA_result_ ra = *RA_regAlloc(frame, iList);  /* 10, 11 */

 	//fprintf(out, "BEGIN function\n");
 	AS_printInstrList (out, iList, Temp_layerMap(F_tempMap,ra.coloring));
 	//fprintf(out, "END function\n\n");
}

int main(int argc, string *argv)
{
 	A_exp absyn_root;
 	S_table base_env, base_tenv;
 	F_fragList frags;
 	char outfile[100];
 	FILE *out = stdout;

 	if (argc==2) {
   		absyn_root = parse(argv[1]);
   		if (!absyn_root)
     			return 1;

#if 1
   		pr_exp(out, absyn_root, 0); /* print absyn data structure */
   		//fprintf(out, "\n\n");
#endif
		//If you have implemented escape analysis, uncomment this
   		Esc_findEscape(absyn_root); /* set varDec's escape field */

   		frags = SEM_transProg(absyn_root);
   		if (anyErrors) return 1; /* don't continue */

   		/* convert the filename */
   		sprintf(outfile, "%s.s", argv[1]);
   		out = fopen(outfile, "w");
   		/* Chapter 8, 9, 10, 11 & 12 */
   		bool first = TRUE;
   		fprintf(out,".globl tigermain\n");
   		for (;frags;frags=frags->tail)
     			if (frags->head->kind == F_procFrag){
       				doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body);
     			}
     			else if (frags->head->kind == F_stringFrag){
       				if (first){
         				first = FALSE;
         				fprintf(out,".section .rodata\n");
       				}

       				Temp_label label = frags->head->u.stringg.label;
       				string str = frags->head->u.stringg.str;
       				char buf[100];
       				int length = strlen(str);
       				char* x = str;
       				for (;(*x);x++){
         				if (*x == '\\')
         					length--;
       				}
       				fprintf(out,"%s:\n.string \"",Temp_labelstring(label));
       				
       				//printf("str %s size %d\n",str,length);
       				char len[4];
       				memcpy(len, &length, 4);
       				fwrite(len,4,1,out);
       				fprintf(out, "%s\"\n", str);
     			}
   		fclose(out);
   		return 0;
 	}
 	EM_error(0,"usage: tiger file.tig");
 	return 1;
}
