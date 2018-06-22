#define BRIGHT 1
#define RED 31
#define OPER 35
#define PARAM 34
#define OP(op,paramList)   {printf("%c[%d;%dm",27,1,OPER); cerr <<  op; printf("%c[%dm",27,0); cerr<< " "; printf("%c[%d;%dm",27,1,PARAM); cerr << paramList; printf("%c[%dm\n",27,0); } 

