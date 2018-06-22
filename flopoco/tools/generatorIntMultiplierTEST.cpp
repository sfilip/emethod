#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

int main () {

/**
*
* to run the file:
* in command line:  $ g++ -o generatorIntMultiplierTEST generatorIntMultiplierTEST.cpp
* 					$./generatorIntMultiplierTEST
*
*/
  string target="Virtex5";
  int wX=1;
  int wY=1;
  int wOut=2;
  bool signedIO=0;
  float ratio=1.0;
  bool enableSuperTiles=1;
 int smth=1;
 int smth1=1;
 cout<<"\n\n";
 cout<<"Please answer the following questions:) \n";
 cout<<"Do you want truncated(min(wX,wY)) or full multipliers?\n";
 cout<<"Truncated=0    Full=1\n";
 string trunc;
 getline(cin,trunc);
 
 
 cout<<"\n The minimum size of input:  ";
 string minx;
 getline(cin, minx);
 cout<<"\n The maximum size of input:  ";
 string maxx;
 getline(cin, maxx);
 int minInput,maxInput;
 
 istringstream (minx ) >> minInput;
 istringstream (maxx ) >> maxInput;
 
 
 cout<<"\n Increment(only for second number)):  ";
 string incr;
 getline(cin, incr);
 
 int increment;
 
 istringstream (incr ) >> increment;

  ofstream myfile;
  cout<<"\n\nThis will take a time... be patient \n";
  /*******************************************************************************************************************************************/
  myfile.open ("Virtex5.txt");
  myfile << "#IntMultiplier\n";
  target="Virtex5";

   cout<<"Generating file for Virtex5... \n";
   for(wX=minInput;wX<=maxInput;wX++)
    for(wY=wX;wY<=maxInput;wY=wY+increment)
    {
      
      if(trunc=="1")
      	wOut=wX+wY;
      else
      	wOut=min(wX,wY);	
          
        //signed/Unsigned variations  
       if(smth1==1)
       {  
       		smth1=0;
        	signedIO=1;
       }
       else
       {
       		smth1=1;
       		signedIO=0;
       }

		//ratio
        if((wX>1)&&(wX<10))
          	ratio=0.2;
        else 
          	if((wX>20)&&(wX<35))
          		ratio=0.5;
          	else
           		if((wX>35)&&(wX<40))
	           		ratio=0.7;		
				else
					ratio=1.0;
             
         //supertilesEnabling       
         if(smth==1)
         {
            enableSuperTiles=1;
            smth=0;
         }
         else
         {
         enableSuperTiles=0;
         smth=1;
         }
         
         myfile<<"flopoco -target="<<target<<" IntMultiplier "<<wX<<" "<<wY<<" "<<wOut<<" "<<signedIO<<" "<<ratio<<" "<<enableSuperTiles<<"\n";
                
      }
  myfile.close();

/*******************************************************************************************************************************************/


/*******************************************************************************************************************************************/
  myfile.open ("Virtex4.txt");
  myfile << "#IntMultiplier\n";
  target="Virtex4";
  cout<<"Generating file for Virtex4... \n";
   for(wX=minInput;wX<=maxInput;wX++)
   for(wY=wX;wY<=maxInput;wY=wY+increment)
    {
      
      if(trunc=="1")
      	wOut=wX+wY;
      else
      	wOut=min(wX,wY);	
          
        //signed/Unsigned variations  
       if(smth1==1)
       {  
       		smth1=0;
        	signedIO=1;
       }
       else
       {
       		smth1=1;
       		signedIO=0;
       }

		//ratio
        if((wX>1)&&(wX<10))
          	ratio=0.2;
        else 
          	if((wX>20)&&(wX<35))
          		ratio=0.5;
          	else
           		if((wX>35)&&(wX<40))
	           		ratio=0.7;		
				else
					ratio=1.0;
             
         //supertilesEnabling       
         if(smth==1)
         {
            enableSuperTiles=1;
            smth=0;
         }
         else
         {
         enableSuperTiles=0;
         smth=1;
         }
         
         myfile<<"flopoco -target="<<target<<" IntMultiplier "<<wX<<" "<<wY<<" "<<wOut<<" "<<signedIO<<" "<<ratio<<" "<<enableSuperTiles<<"\n";
                
      }

  myfile.close();
/*******************************************************************************************************************************************/


/*******************************************************************************************************************************************/
  myfile.open ("Spartan3.txt");
  myfile << "#IntMultiplier\n";
  target="Spartan3";
  cout<<"Generating file for Spartan3... \n";
  for(wX=minInput;wX<=maxInput;wX++)
  for(wY=wX;wY<=maxInput;wY=wY+increment)
    {
      
      if(trunc=="1")
      	wOut=wX+wY;
      else
      	wOut=min(wX,wY);	
          
        //signed/Unsigned variations  
       if(smth1==1)
       {  
       		smth1=0;
        	signedIO=1;
       }
       else
       {
       		smth1=1;
       		signedIO=0;
       }

		//ratio
        if((wX>1)&&(wX<10))
          	ratio=0.2;
        else 
          	if((wX>20)&&(wX<35))
          		ratio=0.5;
          	else
           		if((wX>35)&&(wX<40))
	           		ratio=0.7;		
				else
					ratio=1.0;
             
         //supertilesEnabling       
         if(smth==1)
         {
            enableSuperTiles=1;
            smth=0;
         }
         else
         {
         enableSuperTiles=0;
         smth=1;
         }
         
         myfile<<"flopoco -target="<<target<<" IntMultiplier "<<wX<<" "<<wY<<" "<<wOut<<" "<<signedIO<<" "<<ratio<<" "<<enableSuperTiles<<"\n";
                
      }

  myfile.close();
  /*******************************************************************************************************************************************/
 cout<<"\n      *********************************************************************\n";
 cout<<"                       Xilinx files have been generated \n\n";
 cout<<"                       Altera files are generating now \n";
 cout<<"      *********************************************************************\n";

/*******************************************************************************************************************************************/
  myfile.open ("StratixII.txt");
  myfile << "#IntMultiplier\n";
  target="StratixII";
 cout<<"Generating file for Stratix2... \n";
 for(wX=minInput;wX<=maxInput;wX++)
  for(wY=wX;wY<=maxInput;wY=wY+increment)
    {
      
      if(trunc=="1")
      	wOut=wX+wY;
      else
      	wOut=min(wX,wY);	
          
        //signed/Unsigned variations  
       if(smth1==1)
       {  
       		smth1=0;
        	signedIO=1;
       }
       else
       {
       		smth1=1;
       		signedIO=0;
       }

		//ratio
        if((wX>1)&&(wX<10))
          	ratio=0.2;
        else 
          	if((wX>20)&&(wX<35))
          		ratio=0.5;
          	else
           		if((wX>35)&&(wX<40))
	           		ratio=0.7;		
				else
					ratio=1.0;
             
         //supertilesEnabling       
         if(smth==1)
         {
            enableSuperTiles=1;
            smth=0;
         }
         else
         {
         enableSuperTiles=0;
         smth=1;
         }
         
         myfile<<"flopoco -target="<<target<<" IntMultiplier "<<wX<<" "<<wY<<" "<<wOut<<" "<<signedIO<<" "<<ratio<<" "<<enableSuperTiles<<"\n";
                
      }
  myfile.close();
  /*******************************************************************************************************************************************/


/*******************************************************************************************************************************************/
  myfile.open ("StratixIII.txt");
  myfile << "#IntMultiplier\n";
  cout<<"Generating file for Stratix3... \n";
  target="StratixIII";
   for(wX=minInput;wX<=maxInput;wX++)
    for(wY=wX;wY<=maxInput;wY=wY+increment)
    {
      
      if(trunc=="1")
      	wOut=wX+wY;
      else
      	wOut=min(wX,wY);	
          
        //signed/Unsigned variations  
       if(smth1==1)
       {  
       		smth1=0;
        	signedIO=1;
       }
       else
       {
       		smth1=1;
       		signedIO=0;
       }

		//ratio
        if((wX>1)&&(wX<10))
          	ratio=0.2;
        else 
          	if((wX>20)&&(wX<35))
          		ratio=0.5;
          	else
           		if((wX>35)&&(wX<40))
	           		ratio=0.7;		
				else
					ratio=1.0;
             
         //supertilesEnabling       
         if(smth==1)
         {
            enableSuperTiles=1;
            smth=0;
         }
         else
         {
         enableSuperTiles=0;
         smth=1;
         }
         
         myfile<<"flopoco -target="<<target<<" IntMultiplier "<<wX<<" "<<wY<<" "<<wOut<<" "<<signedIO<<" "<<ratio<<" "<<enableSuperTiles<<"\n";
                
      }
     myfile.close();     
     
     
  myfile.open ("StratixIV.txt");
  myfile << "#IntMultiplier\n";
  cout<<"Generating file for Stratix4...\n";
  target="StratixIV";
 for(wX=minInput;wX<=maxInput;wX++)
   for(wY=wX;wY<=maxInput;wY=wY+increment)
    {
      
      if(trunc=="1")
      	wOut=wX+wY;
      else
      	wOut=min(wX,wY);	
          
        //signed/Unsigned variations  
       if(smth1==1)
       {  
       		smth1=0;
        	signedIO=1;
       }
       else
       {
       		smth1=1;
       		signedIO=0;
       }

		//ratio
        if((wX>1)&&(wX<10))
          	ratio=0.2;
        else 
          	if((wX>20)&&(wX<35))
          		ratio=0.5;
          	else
           		if((wX>35)&&(wX<40))
	           		ratio=0.7;		
				else
					ratio=1.0;
             
         //supertilesEnabling       
         if(smth==1)
         {
            enableSuperTiles=1;
            smth=0;
         }
         else
         {
         enableSuperTiles=0;
         smth=1;
         }
         
         myfile<<"flopoco -target="<<target<<" IntMultiplier "<<wX<<" "<<wY<<" "<<wOut<<" "<<signedIO<<" "<<ratio<<" "<<enableSuperTiles<<"\n";
                
      }
     myfile.close();     

printf("\n   *********************************************************************");
printf("\n   GENERATION COMPLETED");
printf("\n   *********************************************************************\n");
return 1;
}

