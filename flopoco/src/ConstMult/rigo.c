/*
            GENERAL STUFF - Description of the file

This file is an implementation of the algorithm described in [Lef00]
("Multiplication by an integer constant"). It implements the common
subpattern version of the algorithm.

The steps are the following:
    - Convert the constant in binary (here it is done with GMP)
    - Encode it with BoothEncode
    - Call ComputeWeights to find data about the differents possible patterns
    - Find the highest weight by calling FindMaxWeight which will set the 1st
      entry of your array with the best data
    - Pass the best result, that is to say results[0], to FindPattern if the
      constant was unique or to BuildCommonPattern if there were 2 different
      constants. It is useless to call one of those functions if the best
      weight is < 2.
    - Those 2 functions will return a pattern, and set the remainders, you'll
      have to build some data for the final step
    - The final step: BuildSASString, which must be modified for your
      application, the one provided here just displays the SAS string
      on the stdout and is not the best possible.

    Code by Raphaël RIGO / 2003 / Licensed under the GPL
    Minor modifications by Vincent Lefèvre, 2004, 2005
    SPACES project, Loria / INRIA Lorraine.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

/* A little explanation on how the different values are used:
   weight corresponds to the weight (that is to say the number of non-zero
   digits) of the pattern associated with the distance "distance".
   "distance" has 2 meanings: if it is positive, it means the pattern is used
   with pairs N-P / P-N else it uses P-P / N-N pairs.
   The absolute value corresponds to the shifting between the 2 constants
   when str1 == str2 i.e: ABS(distance)==0
   XXXXXXXXXXXXXXXXX
     YYYYYYYYYYYYYYY
   ABS(distance) == 6
       XXXXXXXXXXXXXXXXX
   YYYYYYYYYYYYYYY
   123456

   pos corresponds to the shift of the pattern you must do to have it in the
   correct position in the 1st string pos2 is the same in the 2nd string
   example:
   10987654321098765432109876543210
   P0P0NP0P0000000000P0P0000000000P
     P00P0P0000N00P00P00N
     P00P0P0000000000P
                      0123456789012  : pos
                      321            : pos2
   distance = 29
   pos = 12
   pos2 = 3
   pattern = P00P0P0000000000P
   str1,str2 and pattern hold the indices of the strings
   string1,string2 and patternstr are pointers to strings representing the
   remainders and the pattern
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <gmp.h>

//Define it if you want to use GMP to verify that the algorithm generates
//a valid output
#define GMP_CHECK

//Using a macro is preferable for speed.
#define ABS(a)	   (((a) < 0) ? -(a) : (a))

#define VERBOSE_ALL 2
int verbose;

typedef int (*comparefunc)(const void *, const void *);

typedef struct
{
  int weight;
  int distance;
  int pos,pos2;
  int i, j;
}
weight_r;

typedef struct
{
  int weight; //only used for debugging purpose
  int distance;
  int pos, pos2;
  //indices of the different elements used
  int str1, str2, pattern;
  char *string1, *string2, *patternstr;
}
data_record;

void *my_calloc(size_t n, size_t size)
{
  void *ptr = calloc(n, size);
  if (ptr == NULL)
    {
      fprintf(stderr, "calloc failed!\n");
      exit(EXIT_FAILURE);
    }
  return ptr;
}

void *my_malloc(size_t size)
{
  void *ptr = malloc(size);
  if (ptr == NULL)
    {
      fprintf(stderr, "malloc failed!\n");
      exit(EXIT_FAILURE);
    }
  return ptr;
}

void *my_realloc(void *ptr, size_t size)
{
  ptr = realloc(ptr, size);
  if (ptr == NULL)
    {
      fprintf(stderr, "realloc failed!\n");
      exit(EXIT_FAILURE);
    }
  return ptr;
}

/*
  Removes the leading zeros (as the name could let you think) from string,
  returns the length of the resulting string
*/
int StripLeadingZeros(char *string)
{
  int i, j, length;

  length = strlen(string);
  j = 0;
  while (string[j] == '0')
    j++;
  for (i = j; i <= length; i++)
    string[i-j] = string[i];
  return length - j;
}

int StripEndingZeros(char *string)
{
  int i;

  i = strlen(string) - 1;
  while (string[i] == '0' && i > 0)
    i--;
  if (i == 0 && string[0] == '0')
    i--;
  string[i + 1] = 0;
  return i + 1;
}


void ReverseStr(char *s)
{
  char c;
  int i, j;

  for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
      c = s[i];
      s[i] = s [j];
      s[j] = c;
    }
}

/*Returns the weight (number of non zero elements) of str (booth encoded)*/
int GetWeight(char *str)
{
  int i, nonzero;

  nonzero = 0;
  for (i = 0; i < strlen(str); i++)
    if (str[i] != '0')
      nonzero++;
  return nonzero;
}

//This functions get the non zero digits of booth and store them in nums
//note that the memory is allocated, don't forget too free it afterwards
int GetNonZeroDigits(char *booth,int **nums)
{
  int i, length, nonzero, j;

  length = strlen(booth);

  //First we compute the number of nonzero elements
  nonzero = 0;
  for (i = 0; i < length; i++)
    if (booth[i] != '0')
      nonzero++;
  //We alloc the array to store them
  *nums = my_calloc(sizeof(int), nonzero);

  //Stores the nz elements like this:
  //the indice of the array just means the indice of the nz el
  //the value has 2 significations: the sign tells us if we have a P or a N
  //                                    if > 0 then 'P' else 'N'
  //and the absolute value corresponds to the position of that nz el in the
  //number
  //IMPORTANT: the position is 1-based, ie the 1st element (starting from
  //the left) has indice 1
  //this is due to the sign's meaning
  j = 0;
  for (i = 0; i < length; i++)
    {
      if (booth[i] == 'P')
        (*nums)[j++] = i + 1;
      else if (booth[i] == 'N')
        (*nums)[j++] = - (i + 1);
    }
  return nonzero;
}


/*
  Encodes the binary number stored in binary_str (with MSB bit on the left)
  using Booth Encoding.
  result holds the encoded number.
  returns the length of the encoded number, which can be greater than the
  original number
  don't forget to allocate sufficient space for it :)
*/
int Booth_Encode(char *binary_str, char *result)
{
  int i, len;

  strcpy(result, binary_str);
  ReverseStr(result);
  len = strlen(result);

  i = 0;
  while (i < len)
    {
      if (result[i] == '0')
        i++;
      else
        {
          if (result[i+1] == '1')
            {
              result[i++] = 'N';
              while (result[i] == '1')
                result[i++] = '0';
              result[i] = 'P';
            }
          else
            result[i++] = 'P';
        }
    }

  //We must recompute the length of the string as it can be longer than
  //the binary one
  len = strlen(result);
  ReverseStr(result);
  return len;
}


/*
    Not used in the program but it can be usefull.
*/
int BoothDecode(char *booth, char *original, int pos)
{
  int i;

  ReverseStr(booth);

  ReverseStr(original);

  printf("%s\n%s\n", booth, original);
  for (i = 0; booth[i]; i++)
    {
      if (booth[i] == 'P')
        booth[i] = '1';

      if (booth[i] == 'N')
        {
          booth[i++] = '1';
          while ((booth[i] == '0' || !booth[i]) && original[i+pos] == '0')
            booth[i++] = '1';
          booth[i] = '0';
        }
    }
  ReverseStr(booth);
  ReverseStr(original);
  return StripLeadingZeros(booth);
}

/*
  Finds the maximum weight and sets the first element of the array with the
  data of the best weight
*/
void FindMaxWeight(weight_r *data, int num)
{
  int i, maxw, ind;

  maxw = 0;
  ind = 0;
  for (i = 0; i < num; i++)
    if (data[i].weight > maxw)
      {
        maxw = data[i].weight;
        ind = i;
      }
  memcpy(&(data[0]), &(data[ind]), sizeof(weight_r));
  return;
}


/*
    Fills a data_record structure with the parameters
    - str1 : indice of the 1st constant
    - str2 : indice of the 2nd constant
    - pattern : indice of the pattern
    - weight : best pattern data
    - string1, string2, patternstr : pointers to the strings
*/

void FillBuildData(data_record *build_data, int str1, int str2, int pattern,
                   weight_r weight, char *string1, char *string2,
                   char *patternstr)
{
  build_data[0].str1 = str1;
  build_data[0].str2 = str2;
  build_data[0].pattern = pattern;
  build_data[0].weight = weight.weight;
  build_data[0].distance = weight.distance;
  build_data[0].pos = weight.pos;
  build_data[0].pos2 = weight.pos2;
  build_data[0].string1 = my_calloc(sizeof(char), strlen(string1) + 1);
  build_data[0].string2 = my_calloc(sizeof(char), strlen(string2) + 1);
  build_data[0].patternstr = my_calloc(sizeof(char), strlen(patternstr) + 1);
  strcpy(build_data[0].string1, string1);
  strcpy(build_data[0].string2, string2);
  strcpy(build_data[0].patternstr, patternstr);
  return;
}

//FOR SINGLE CONSTANT
//Computes the real weights for each distance, the result is stored
//in result, which length must be at least "length"*2
//the weights are stored in this way
//result[0]=weight[-length+1];
//the best is to see it as an array with relatives indices :
//result = result+length
//it returns the number of weights stored in result if the number of
//non zero elements is > 1, returns 0 else
int Compute_Weights_Unique(char *number, weight_r *result, const long length)
{
  unsigned long nonzero;    //number of non zero elements
  int *nzbooth;      //This array will contain the indices of the nz elements
  int i, j, realweight, temp, weightcount, minweight;
  int *weightm;
  char *used;
  //This string will be used to store the pattern associated
  //with the distance we are working on
  char *pattern;

  //Generation of an array with the nonzero elements
  nonzero = GetNonZeroDigits(number, &nzbooth);

  weightcount = 0;
  if (verbose > 0)
    for (i = 0; i < nonzero; i++)
      printf("%d : %d\n", i, nzbooth[i]);

  /* PATTERN SEARCHING : Distances computation*/
  //we allocate a bit more memory to avoid segfaults if bad coding :X
  //we allocated 2*length, ie 2*the number of bits in the constant
  //so we can store the majorants of the distances in a "relative" array
  //negatives indices corresponds to N-P / P-N
  //positives to P-P / N-N

  weightm = my_calloc(sizeof(int), length * 2 + 2);
  weightm += length;
  if (verbose > 0)
    {
      printf("-----------------------\n");
      printf("Calcul des majorants des distances...\n");
    }
  //An efficient way to look for patterns for each distance is to
  //consider only non zero elements, because the weight is often
  //negligible compared to the length of the number
  for (i = 0; i < nonzero - 1; i++)
    {
      for (j = i + 1; j < nonzero; j++)
        {
          //search for P-?
          if (nzbooth[i] > 0)
            {
              //P-P
              if (nzbooth[j] > 0)
                weightm[nzbooth[j] - nzbooth[i]]++;
              else //P-N
                weightm[nzbooth[j] + nzbooth[i]]++;
            }
          else //search for N-?
            {
              //N-N
              if (nzbooth[j] < 0)
                weightm[-nzbooth[j] + nzbooth[i]]++;
              else //N-P
                weightm[-nzbooth[j] - nzbooth[i]]++;
            }
        }
    }

  /* REAL WEIGHT AND PATTERN CALCULATION */
  pattern = my_calloc(sizeof(char), length + 1);
  used = my_calloc(sizeof(char), length + 1);
  if (verbose == VERBOSE_ALL)
    for (i = - length + 1; i < length; i++)
      {
        printf("Distance %d : Majorant du poids :  %d\n", i, weightm[i]);
      }

  //we modify our pointer so we can use result as an array with
  //relatives indices
  //result = result+length-1;
  if (verbose > 0)
    printf("----------------------------\n");
  minweight = 1;
  for (i = - length + 1; i < length; i++)
    {
      //There is no need to check the real weights for distances
      //for which the majorant is < to the current best weight
      if (weightm[i] > minweight)
        {
          memset(pattern, '0', length);
          realweight = 0;
          if (verbose > 0)
            printf("Distance %d: Majorant du poids: %d\n", i, weightm[i]);
          memset(used, 0, length);
          if (i > 0)
            {
              //if i is > 0 we must search P-P /N-N pairs
              for (j = 0; j < nonzero; j++)
                {
                  //correction of the position
                  temp = ABS(nzbooth[j]) - 1;
                  //we must check where we are to avoid buffer overflows
                  if (temp < length - i)
                    {
                      //If we have the same numbers, we add them to the pattern
                      if (number[temp] == number[temp + i] && !used[temp])
                        {
                          pattern[temp] = number[temp];
                          used[temp]++;
                          used[temp + i]++;
                          realweight++;
                        }
                    }
                }
            }
          else
            {
              for (j = 0; j < nonzero; j++)
                {
                  temp = ABS(nzbooth[j]) - 1;
                  if (temp < length + i)
                    {
                      if (!used[temp] &&
                          ((number[temp] == 'N' && number[temp - i] == 'P') ||
                           (number[temp] == 'P' && number[temp - i] == 'N')))
                        {
                          pattern[temp] = number[temp];
                          used[temp]++;
                          used[temp - i]++;
                          realweight++;
                        }
                    }
                }
            }
          pattern[j] = 0;
          if (realweight > minweight)
            {
              result[weightcount].distance = i;
              result[weightcount].weight = realweight;
              weightcount++;
              minweight = realweight;
            }
          if (verbose > 0)
            {
              StripLeadingZeros(pattern);
              StripEndingZeros(pattern);
              printf("Motif associé : %s\nPoids réel : %d\n",
                     pattern, realweight);
            }
        }
    }
  free(weightm - length);
  free(used);
  free(nzbooth);
  free(pattern);
  return weightcount;
}

//This function tries to find a common pattern in 2 different constants
//  - number1: the 1st constant (booth encoded)
//  - number2: the 2nd constant (booth encoded)
//  - result: an array of weight_r containing the different distances and
//            associated weights
//  - length1: length of the 1st string
//  - length2: length of the 2nf string
//  - reversed = pointer to an int that will be 0 if the strings 1 & 2
//               haven't been changed because of their weighs and will
//               be 1 in the other case
//
// you may pass the same pointers number1 and number2 to do apply the
// algo on the same constant be aware that calling
// Compute_Weights("PP0P","PP0P",...) wouldn't be ok
int Compute_Weights(char *number1, char *number2, weight_r *result,
                    long length1, long length2, int *reversed)
{
  int i, j, temp;
  int realweight, pos, weightcount, pos2;
  char *pattern;
  int nzc1, nzc2; //number of non zero elements in each number
  int *nz1, *nz2, *tempptr; //nw elements

  //We check if we have the same number
  //if we do, we call the good algo
  if (number1 == number2)
    {
      pos = Compute_Weights_Unique(number1, result, length1);
      return pos;
    }

  nzc1 = nzc2 = 0;
  nzc1 = GetNonZeroDigits(number1, &nz1);
  if (verbose > 0)
    {
      printf("Non zero chars in number1: \n");
      for (i = 0; i < nzc1; i++)
        printf("%d : %d\n", i, nz1[i]);
    }

  nzc2 = GetNonZeroDigits(number2, &nz2);
  if (verbose > 0)
    {
      printf("Non zero chars in number2: \n");
      for (i = 0; i < nzc2; i++)
        printf("%d : %d\n", i, nz2[i]);
    }

  //we must ALWAYS have the 2nd number smaller or equal to the 1st
  if (nzc1 < nzc2)
    {
      //Exchanging the 2 numbers
      pattern = number1;
      number1 = number2;
      number2 = pattern;
      //Exchanging the lengths
      temp = length2;
      length2 = length1;
      length1 = temp;
      //Exchanging the pointers to the non zero digits
      temp = nzc2;
      nzc2 = nzc1;
      nzc1 = temp;
      //Exchanging the nz digits counts
      tempptr = nz2;
      nz2 = nz1;
      nz1 = tempptr;

      *reversed = 1;
    }
  else
    {
      *reversed = 0;
    }

  //corrects the pointer result so we can use relative indices
  //result+=length1+length2-1;

  weightcount = 0;

  pattern = my_calloc(sizeof(char), length2 + 1);
  //i represents the "shift"+-1
  //a shift of +1 represents infact no shift and look for P-P/N-N
  //-1 : 0 shift and look for P-N / N-P
  //see above for a more complete explanation
  for (i = - length1 - length2 + 1; i < length1 + length2; i++)
    {
      realweight = 0;
      pos = 0;
      memset(pattern, '0', sizeof(char) * length2);

      if (verbose == VERBOSE_ALL)
        {
          printf("%s\n", number1);
          for (j = 0; j < length1 - ABS(i) - 1; j++)
            printf(" ");
          if (ABS(i) > length1 - 1)
            printf("%s\n", number2 + ABS(i) - length1 - 1);
          else
            printf("%s\n", number2);
        }
      if (ABS(i) < length1)
        {
          //here we are in this case:
          //111111111111111
          //     222222222
          if (i > 0) //looks for P-P / N-N pairs
            {
              for (j = 0; j < nzc2; j++)
                {
                  if (ABS(nz2[j]) <= i + 1)
                    {
                      if (nz2[j] > 0)
                        temp = nz2[j] + (length1 - i - 1);
                      else
                        temp = nz2[j] - (length1 - i - 1);
                      if (number1[ABS(temp) - 1] == number2[ABS(nz2[j]) - 1])
                        {
                          pos = i - ABS(nz2[j]) + 1;
                          pos2 = length2 - ABS(nz2[j]);
                          realweight++;
                          pattern[ABS(nz2[j]) - 1] = number2[ABS(nz2[j]) - 1];
                        }
                    }
                }
            }
          else if (i < 0) //looks for P-N / N-P pairs
            {
              for (j = 0; j < nzc2; j++)
                {
                  if (ABS(nz2[j]) <= - i + 1)
                    {
                      if (nz2[j] > 0)
                        temp = nz2[j] + (length1 + i - 1);
                      else
                        temp = nz2[j] - (length1 + i - 1);

                      if ((number1[ABS(temp) - 1] == 'P' &&
                           number2[ABS(nz2[j]) - 1] == 'N') ||
                          (number1[ABS(temp) - 1] == 'N' &&
                           number2[ABS(nz2[j]) - 1] == 'P'))
                        {
                          pos = (- i) - ABS(nz2[j]) + 1;
                          pos2 = length2 - ABS(nz2[j]);
                          realweight++;
                          pattern[ABS(nz2[j]) - 1] = number2[ABS(nz2[j]) - 1];
                        }
                    }
                }
            }
        }   //if(ABS(i)<length1)
      else if (ABS(i) > length1)
        {
          //here :
          //          11111111111111111111
          // 2222222222222
          if (i > 0) //looks for P-P / N-N pairs
            {
              for (j = 0; j < nzc2; j++)
                {
                  if (ABS(nz2[j]) > (i - length1 - 1) && ABS(nz2[j]) < i)
                    {
                      if (nz2[j] > 0)
                        temp = nz2[j] - (i - length1 - 1);
                      else
                        temp = nz2[j] + (i - length1 - 1);
                      if (number1[ABS(temp) - 1] == number2[ABS(nz2[j]) - 1])
                        {
                          pos = i - 1 - ABS(nz2[j]);
                          pos2 = length2 - ABS(nz2[j]);
                          realweight++;
                          pattern[ABS(nz2[j]) - 1] = number2[ABS(nz2[j]) - 1];
                        }
                    }
                }
            }
          else if (i < 0) //looks for P-N / N-P pairs
            {
              for (j = 0; j < nzc2; j++)
                {
                  if (ABS(nz2[j]) > (- i - length1 - 1) && ABS(nz2[j]) < - i)
                    {
                      if (nz2[j] > 0)
                        temp = nz2[j] - (- i - length1 - 1);
                      else
                        temp = nz2[j] + (- i - length1 - 1);
                      if ((number1[ABS(temp) - 1] == 'P' &&
                           number2[ABS(nz2[j]) - 1] == 'N') ||
                          (number1[ABS(temp) - 1] == 'N' &&
                           number2[ABS(nz2[j]) - 1] == 'P'))
                        {
                          pos = (- i) - 1 - ABS(nz2[j]);
                          pos2 = length2 - ABS(nz2[j]);
                          realweight++;
                          pattern[ABS(nz2[j]) - 1] = number2[ABS(nz2[j]) - 1];
                        }
                    }
                }
            }
        }   //if(ABS(i)>length1)
      //We remove the leading zeros from the pattern
      if (verbose > 0 && realweight > 1)
        {
          StripLeadingZeros(pattern);
          StripEndingZeros(pattern);
        }
      if (verbose > 0 && ABS(i) != length1)
        printf("Décalage : %3d\tPos : %2d\tPoids : %d\tMotif : %s\n",
               i, pos, realweight, pattern);
      if (realweight > 1)
        {
          result[weightcount].weight = realweight;
          result[weightcount].distance = i;
          result[weightcount].pos = pos;
          result[weightcount].pos2 = pos2;
          weightcount++;
        }
    }
  free(pattern);
  free(nz1);
  free(nz2);
  return weightcount;
}

/* FindPattern: finds the pattern corresponding to the given distance
   - booth: the number, booth encoded
   it contains the remainder at the end
   - data: the datas corresponding to the highest weight available
   - pattern: resulting string that will contain the pattern
   pattern must have at least strlen(booth) elements allocated
   returns the right position of the pattern
*/
int FindPattern(char *booth, const weight_r data, char *pattern)
{
  //distance between the 2 occurences of the pattern
  int distance, i, temp;
  unsigned long numlen, pos;
  //used keeps track of the already used chars
  char *used;
  int *nzbooth;
  int nonzero;

  //we use a local variable to make the code more readable
  distance = data.distance;
  numlen = strlen(booth);

  nonzero = GetNonZeroDigits(booth, &nzbooth);

  //pos will contain the position of the right pattern
  pos = 0;

  //allocates the memory for use
  used = my_calloc(sizeof(char), numlen);

  //reset everything
  memset(pattern, '0', sizeof(char) * numlen);

  //we now run through the number to find our pattern
  if (distance > 0)
    {
      for (i = 0; i < nonzero; i++)
        {
          temp = ABS(nzbooth[i]) - 1;
          if (temp < numlen - distance)
            {
              //the distance is > 0. This means that we must look for
              //P-P / N-N pairs
              if (booth[temp] == booth[temp + distance] && !used[temp])
                {
                  if (!pos)
                    pos = numlen - temp;
                  pattern[temp] = booth[temp];
                  used[temp]++;
                  used[temp + distance]++;
                }
            }
        }
    }
  else
    {
      //Here we must look for N-P / P-N pairs
      for (i = 0; i < nonzero; i++)
        {
          temp = ABS(nzbooth[i]) - 1;
          if (temp < numlen + distance)
            {
              //the distance is > 0. This means that we must look for
              //P-N / N-P pairs
              if (!used[temp] &&
                  ((booth[temp] == 'N' && booth[temp - distance] == 'P') ||
                   (booth[temp] == 'P' && booth[temp - distance] == 'N')))
                {
                  if (!pos)
                    pos = numlen - temp;
                  pattern[temp] = booth[temp];
                  used[temp]++;
                  used[temp - distance]++;
                }
            }
        }
    }
  /* We have our pattern, we can now compute the remainder */
  for (i = 0; i < nonzero; i++)
    {
      temp = ABS(nzbooth[i]) - 1;
      if (temp < numlen - ABS(distance))
        {
          if (booth[temp] == pattern[temp] && booth[temp] != '0')
            {
              booth[temp] = '0';
              booth[temp + ABS(distance)] = '0';
            }
        }
    }
  //We now remove the leading zeros from the pattern
  pos -= StripLeadingZeros(pattern);
  pos += strlen(pattern);
  pos -= StripEndingZeros(pattern);

  //We now remove the leading zeros from the remainder
  StripLeadingZeros(booth);

  if (verbose > 0)
    {
      printf("Pattern: %s\npos: %d\n", pattern, (int) pos);
      printf("Remainder: %s\n", booth);
    }
  free(used);
  free(nzbooth);
  return pos;
}

/*
  Gets the best common pattern in number1 and number2
  - number1: 1st number (booth encoded)
  - number2: 2nd number (booth encoded)
  - data: data about the pattern we want to build
  - pattern: buffer that will contain the pattern
  return values:
  - number1: contains the remainder when the pattern has been substracted
  - number2: same
  return val: nothing.
*/
int BuildCommonPattern(char *number1, char *number2, const weight_r data,
                       char *pattern)
{
  int i, distance, pos;
  int length1, length2;
  char *temp;
  length1 = strlen(number1);
  length2 = strlen(number2);

  if (GetWeight(number1) < GetWeight(number2))
    {
      temp = number1;
      number1 = number2;
      number2 = temp;
      pos = length1;
      length1 = length2;
      length2 = pos;
    }

  distance = data.distance;
  pos = data.pos;

  memset(pattern, '0', length2);
  if (ABS(distance) > length1)
    {
      if (distance > 0)    //P-P / N-N pairs
        {
          //we have to correct our distance
          distance--;
          for (i = distance - length1; i < length2 && i < distance; i++)
            {
              if (number1[i - distance + length1] == number2[i] &&
                  number2[i] != '0')
                {
                  pattern[i - distance + length1] = number2[i];
                  number1[i - distance + length1] = '0';
                  number2[i] = '0';
                }
            }
        }
      else    //P-N / N-P pairs
        {
          distance = - distance - 1; //we correct our distance
          for (i = distance - length1; i < length2 && i < distance; i++)
            {
              if ((number1[i - distance + length1] == 'P' &&
                   number2[i] == 'N') ||
                  (number1[i - distance + length1] == 'N' &&
                   number2[i] == 'P'))
                {
                  pattern[i - distance + length1] =
                    number1[i - distance + length1];
                  number1[i - distance + length1] = '0';
                  number2[i] = '0';
                }
            }
        }
    } //if(ABS(distance)>length1)
  else if (ABS(distance) < length1)
    {
      if (distance > 0)    //P-P / N-N pairs
        {
          //we have to correct our distance
          distance--;
          for (i = 0; i < length2 && i < distance + 2; i++)
            {
              if (number1[i - distance - 2 + length1] == number2[i] &&
                  number2[i] != '0')
                {
                  pattern[i] = number2[i];
                  number1[i - distance - 2 + length1] = '0';
                  number2[i] = '0';
                }
            }
        }
      else    //P-N / N-P pairs
        {
          distance = - distance - 1; //we correct our distance
          for (i = 0; i < length2 && i < distance + 2; i++)
            {
              if ((number1[i - distance - 2 + length1] == 'P' &&
                   number2[i] == 'N') ||
                  (number1[i - distance - 2 + length1] == 'N' &&
                   number2[i] == 'P'))
                {
                  pattern[i] = number1[i - distance - 2 + length1];
                  number1[i - distance - 2 + length1] = '0';
                  number2[i] = '0';
                }
            }
        }
    }   //if(ABS(distance)<length1)
  //We clean the different numbers
  StripLeadingZeros(number1);
  StripLeadingZeros(number2);
  StripLeadingZeros(pattern);
  StripEndingZeros(pattern);

  if (verbose > 0)
    {
      printf("remainder Number 1:%s\n", number1);
      printf("remainder Number 2:%s\n", number2);
      printf("Pattern: %s\n", pattern);
    }
  return 0;
}

/*
    Do the binary method to multiply by a number
    Returns the cost needed to build this number
    Args:
        - str: string (booth) of the number
        - number: indice of the final number
        - done: set to 0 if str is 0, 1 else
        - negated: set to 1 if the number in str is < 0
        - result: used when GMP_CHECK is defined, contains the value of the
                  number
        - indice = minimum indice for temp calcs
*/
int DoSingleNumberSAS(char *str, int number, int *done, int *negated,
                      mpz_t *result, int indice)
{
  char neg;
  int last, j, weight, localcurrent, length, cost;
  char isfirst;
#ifdef GMP_CHECK
  mpz_t un, tempmpz, *res;

  mpz_init_set_ui(un, 1);
  mpz_init(tempmpz);
#endif

  last = 0;
  cost = 0;
  weight = GetWeight(str);
  length = strlen(str);

  if (!weight)
    {
      mpz_set_ui(*result, 0);
      *done = 0;
      return 0;
    }

  if (str[0] == 'N') //negative number
    {
      for (j = 0; j < length; j++)
        if (str[j] == 'P')
          str[j] = 'N';
        else if (str[j] == 'N')
          str[j] = 'P';
      *negated = 1;
    }
  else
    *negated = 0;

  if (weight == 1 && strlen(str) == 1)
    {
      printf("u%d = u0\n", number);
      *done = 1;
#ifdef GMP_CHECK
      mpz_set_ui(*result, 1);
#endif
      return 0;
    }

#ifdef GMP_CHECK
  res = my_malloc(sizeof(mpz_t) * weight);
  for (j = 0; j < weight; j++)
    mpz_init(res[j]);
#endif

  isfirst = 1;
  if (str[length - 1] == '0')
    localcurrent = weight - 1;
  else
    localcurrent = weight - 2;
  if (localcurrent < 0)
    localcurrent = 0;

  if (str[last] == 'N')
    neg = 1;
  else
    neg = 0;
  do
    {
      j = last + 1;
      while (str[j++] == '0')
        ;
      j--;
      if (j - last)
        {
          if (isfirst)
            {
              if (str[j] == 'P' && !neg)
                {
                  if (localcurrent)
                    printf("u%d = u0 << %d + u0\n",
                           indice + localcurrent, j - last);
                  else
                    printf("u%d = u0 << %d + u0\n", number, j - last);
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, un, j - last);
                  mpz_add(res[localcurrent], tempmpz, un);
                  mpz_out_str(stdout, 2, res[localcurrent]);
                  printf("\n");
#endif
                  cost++;
                  *done = 1;
                  localcurrent--;
                  isfirst = 0;
                }
              if (str[j] == 0 && j - last - 1)
                {
                  if (localcurrent)
                    printf("u%d = u0 << %d\n",
                           indice + localcurrent, j - last);
                  else
                    printf("u%d = u0 << %d\n", number, j - last);
#ifdef GMP_CHECK
                  mpz_mul_2exp(res[localcurrent], un, j - last - 1);
                  mpz_out_str(stdout, 2, res[localcurrent]);
                  printf("\n");
#endif
                  localcurrent--;
                  *done = 1;
                  isfirst = 0;
                }
              if (str[j] == 'N' || neg)
                {
                  if (localcurrent)
                    printf("u%d = u0 << %d - u0\n",
                           indice + localcurrent, j - last);
                  else
                    printf("u%d = u0 << %d - u0\n", number, j - last);
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, un, j - last);
                  mpz_sub(res[localcurrent], tempmpz, un);
                  mpz_out_str(stdout, 2, res[localcurrent]);
                  printf("\n");
#endif
                  cost++;
                  neg = 0;
                  *done = 1;
                  localcurrent--;
                  isfirst = 0;
                }
            }
          else
            {
              if (str[j] == 'P' && !neg)
                {
                  if (localcurrent)
                    printf("u%d = u%d << %d + u0\n", indice + localcurrent,
                           indice + localcurrent + 1, j - last);
                  else
                    printf("u%d = u%d << %d + u0\n", number,
                           indice + localcurrent + 1, j - last);
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, res[localcurrent + 1], j - last);
                  mpz_add(res[localcurrent], tempmpz, un);
                  mpz_out_str(stdout, 2, res[localcurrent]);
                  printf("\n");
#endif
                  cost++;
                  localcurrent--;
                  *done = 1;
                }
              if (str[j] == 0 && j - last - 1)
                {
                  if (localcurrent)
                    printf("u%d = u%d << %d\n", indice + localcurrent,
                           indice + localcurrent + 1, j - last - 1);
                  else
                    printf("u%d = u%d << %d\n", number,
                           indice + localcurrent + 1, j - last - 1);
#ifdef GMP_CHECK
                  mpz_mul_2exp(res[localcurrent], res[localcurrent + 1],
                               j - last - 1);
                  mpz_out_str(stdout, 2, res[localcurrent]);
                  printf("\n");
#endif
                  localcurrent--;
                  *done = 1;
                }
              if (str[j] == 'N' || neg)
                {
                  if (localcurrent)
                    printf("u%d = u%d << %d - u0\n", indice + localcurrent,
                           indice + localcurrent + 1, j - last);
                  else
                    printf("u%d = u%d << %d - u0\n", number,
                           indice + localcurrent + 1, j - last);
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, res[localcurrent + 1], j - last);
                  mpz_sub(res[localcurrent], tempmpz, un);
                  mpz_out_str(stdout, 2, res[localcurrent]);
                  printf("\n");
#endif
                  *done = 1;
                  cost++;
                  neg = 0;
                  localcurrent--;
                }
            }
        }
      last = j;
    }
  while (j < length);
#ifdef GMP_CHECK
  mpz_set(*result, res[0]);
  mpz_clear(un);
  mpz_clear(tempmpz);
  for (j = 0; j < weight; j++)
    mpz_clear(res[j]);
  free(res);
#endif
  return cost;
}

//This function builds a Shift-Add-Sub string (SAS) using the data stored
//in build_data
//The result is displayed on the terminal
//It returns the cost
int BuildSASString(data_record *build_data, int num, int numstrings,
                   char *original)
{
  int i, temp, cost, *neg;
  char *computed;
  mpz_t *res, tempmpz;

  res = my_malloc(sizeof(mpz_t) * (++numstrings));
  for (i = 0; i < numstrings; i++)
    mpz_init(res[i]);

  mpz_init(tempmpz);

  neg = my_calloc(sizeof(neg), numstrings);
  //computed is used to check if a number has previously been built
  computed = my_calloc(sizeof(char), numstrings);
  //current holds the number of the current constant we are building
  printf("u0 = x\n");
  cost = 0;
  for (i = num - 1; i >= 0; i--)
    {
      temp = 1;
      //Do we work on a single constant ?
      if (build_data[i].str1 == build_data[i].str2)
        {
          if (!computed[build_data[i].pattern])
            {
              cost += DoSingleNumberSAS(build_data[i].patternstr,
                                        build_data[i].pattern + 1, &temp,
                                        &(neg[build_data[i].pattern]),
                                        &res[build_data[i].pattern + 1],
                                        numstrings);
              computed[build_data[i].pattern]++;
            }
          //Debug code:
          //printf("%s\n",build_data[i].patternstr);
          //Since we work with a single constant, we must build number
          //composed by the 2 occurences of the pattern
          if (ABS(build_data[i].distance) > build_data[i].pos)
            {
              printf("u%d = u%d << %d\n", build_data[i].pattern + 1,
                     build_data[i].pattern + 1,
                     ABS(build_data[i].distance) - build_data[i].pos);
#ifdef GMP_CHECK
              mpz_mul_2exp(res[build_data[i].pattern + 1],
                           res[build_data[i].pattern + 1],
                           ABS(build_data[i].distance) - build_data[i].pos);
              mpz_out_str(stdout, 2, res[build_data[i].pattern + 1]);
              printf("\n");
#endif
            }
          else if (ABS(build_data[i].distance) < build_data[i].pos)
            {
              printf("u%d = u%d << %d\n", build_data[i].pattern + 1,
                     build_data[i].pattern + 1,
                     build_data[i].pos - ABS(build_data[i].distance));
#ifdef GMP_CHECK
              mpz_mul_2exp(res[build_data[i].pattern + 1],
                           res[build_data[i].pattern + 1],
                           build_data[i].pos - ABS(build_data[i].distance));
              mpz_out_str(stdout, 2, res[build_data[i].pattern + 1]);
              printf("\n");
#endif
            }

          if (build_data[i].distance > 0)
            {
              cost++;
              printf("u%d = u%d << %d  + u%d\n", build_data[i].pattern + 1,
                     build_data[i].pattern + 1, ABS(build_data[i].distance),
                     build_data[i].pattern + 1);
#ifdef GMP_CHECK
              mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                           ABS(build_data[i].distance));
              mpz_add(res[build_data[i].pattern + 1], tempmpz,
                      res[build_data[i].pattern + 1]);
              mpz_out_str(stdout, 2, res[build_data[i].pattern + 1]);
              printf("\n");
#endif
            }
          else
            {
              printf("u%d = u%d << %d  - u%d\n", build_data[i].pattern + 1,
                     build_data[i].pattern + 1, ABS(build_data[i].distance),
                     build_data[i].pattern + 1);
#ifdef GMP_CHECK
              mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                           ABS(build_data[i].distance));
              mpz_sub(res[build_data[i].pattern + 1], tempmpz,
                      res[build_data[i].pattern + 1]);
              mpz_out_str(stdout, 2, res[build_data[i].pattern + 1]);
              printf("\n");
#endif
              cost++;
            }
          //We now build our numbers using the pattern
          if (!computed[build_data[i].str1])
            {
              cost += DoSingleNumberSAS(build_data[i].string1,
                                        build_data[i].str1 + 1, &temp,
                                        &(neg[build_data[i].str1]),
                                        &res[build_data[i].str1 + 1],
                                        numstrings);
              computed[build_data[i].str1]++;
            }
          //Debug code:
          //printf("%s\n",build_data[i].string1);
          if (temp)
            {
              cost++;
              if (neg[build_data[i].pattern])
                {
                  if (neg[build_data[i].str1])
                    {
#ifdef GMP_CHECK
                      mpz_neg(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1]);
                      mpz_sub(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1],
                              res[build_data[i].pattern + 1]);
#endif
                      printf("u%d = -u%d - u%d\n", build_data[i].str1 + 1,
                             build_data[i].str1 + 1,
                             build_data[i].pattern + 1);
                      neg[build_data[i].str1] = 0;
                    }
                  else
                    {
#ifdef GMP_CHECK
                      mpz_sub(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1],
                              res[build_data[i].pattern + 1]);
#endif
                      printf("u%d = u%d - u%d\n", build_data[i].str1 + 1,
                             build_data[i].str1 + 1,
                             build_data[i].pattern + 1);
                    }
                }
              else
                {
                  if (neg[build_data[i].str1])
                    {
#ifdef GMP_CHECK
                      mpz_neg(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1]);
                      mpz_add(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1],
                              res[build_data[i].pattern + 1]);
#endif
                      printf("u%d = -u%d + u%d\n", build_data[i].str1 + 1,
                             build_data[i].str1 + 1,
                             build_data[i].pattern + 1);
                      neg[build_data[i].str1] = 0;
                    }
                  else
                    {
#ifdef GMP_CHECK
                      mpz_add(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1],
                              res[build_data[i].pattern + 1]);
#endif
                      printf("u%d = u%d + u%d\n", build_data[i].str1 + 1,
                             build_data[i].str1 + 1,
                             build_data[i].pattern + 1);
                    }
                }
#ifdef GMP_CHECK
              mpz_out_str(stdout, 2, res[build_data[i].str1 + 1]);
              printf("\n");
#endif
            }
          else
            {
              neg[build_data[i].str1] = neg[build_data[i].pattern];
              printf("u%d = u%d\n", build_data[i].str1 + 1,
                     build_data[i].pattern + 1);
#ifdef GMP_CHECK
              mpz_set(res[build_data[i].str1 + 1],
                      res[build_data[i].pattern + 1]);
#endif
              temp++;
            }
        }
      else    //working on 2 different constants
        {
          if (!computed[build_data[i].pattern])
            {
              cost += DoSingleNumberSAS(build_data[i].patternstr,
                                        build_data[i].pattern + 1, &temp,
                                        &(neg[build_data[i].pattern]),
                                        &res[build_data[i].pattern + 1],
                                        numstrings);
              computed[build_data[i].pattern]++;
            }
          //Debug code:
          //printf("%s\n",build_data[i].patternstr);
          //We now build our numbers using the pattern
          if (!computed[build_data[i].str1])
            {
              cost += DoSingleNumberSAS(build_data[i].string1,
                                        build_data[i].str1 + 1, &temp,
                                        &(neg[build_data[i].str1]),
                                        &res[build_data[i].str1 + 1],
                                        numstrings);
              computed[build_data[i].str1]++;
            }
          //Debug code:
          //printf("%s\n",build_data[i].string1);
          if (temp)
            {
              cost++;
#ifdef GMP_CHECK
              mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                           build_data[i].pos);
#endif
              if (neg[build_data[i].pattern])
                {
                  if (neg[build_data[i].str1])
                    {
#ifdef GMP_CHECK
                      mpz_neg(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1]);
                      mpz_sub(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1], tempmpz);
#endif
                      printf("u%d = -u%d - u%d << %d\n",
                             build_data[i].str1 + 1, build_data[i].str1 + 1,
                             build_data[i].pattern + 1, build_data[i].pos);
                      neg[build_data[i].str1] = 0;
                    }
                  else
                    {
                      printf("u%d = u%d - u%d << %d\n",
                             build_data[i].str1 + 1, build_data[i].str1 + 1,
                             build_data[i].pattern + 1, build_data[i].pos);
#ifdef GMP_CHECK
                      mpz_sub(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1], tempmpz);
#endif
                    }
                }
              else
                {
                  if (neg[build_data[i].str1])
                    {
#ifdef GMP_CHECK
                      mpz_neg(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1]);
                      mpz_add(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1], tempmpz);
#endif
                      printf("u%d = -u%d + u%d << %d\n",
                             build_data[i].str1 + 1, build_data[i].str1 + 1,
                             build_data[i].pattern + 1, build_data[i].pos);
                      neg[build_data[i].str1] = 0;
                    }
                  else
                    {
                      printf("u%d = u%d + u%d << %d\n",
                             build_data[i].str1 + 1, build_data[i].str1 + 1,
                             build_data[i].pattern + 1, build_data[i].pos);
#ifdef GMP_CHECK
                      mpz_add(res[build_data[i].str1 + 1],
                              res[build_data[i].str1 + 1], tempmpz);
#endif
                    }
                }
#ifdef GMP_CHECK
              mpz_out_str(stdout, 2, res[build_data[i].str1 + 1]);
              printf("\n");
#endif
            }
          else
            {
#ifdef GMP_CHECK
              mpz_mul_2exp(res[build_data[i].str1 + 1],
                           res[build_data[i].pattern + 1],
                           build_data[i].pos);
#endif
              if (neg[build_data[i].pattern])
                {
                  printf("u%d = - u%d << %d\n", build_data[i].str1 + 1,
                         build_data[i].pattern + 1, build_data[i].pos);
#ifdef GMP_CHECK
                  mpz_neg(res[build_data[i].str1 + 1],
                          res[build_data[i].str1 + 1]);
#endif
                }
              else
                printf("u%d = u%d << %d\n", build_data[i].str1 + 1,
                       build_data[i].pattern + 1, build_data[i].pos);
#ifdef GMP_CHECK
              mpz_out_str(stdout, 2, res[build_data[i].str1 + 1]);
              printf("\n");
#endif
            }
          //We now build our numbers using the pattern
          if (!computed[build_data[i].str2])
            {
              cost += DoSingleNumberSAS(build_data[i].string2,
                                        build_data[i].str2 + 1, &temp,
                                        &(neg[build_data[i].str2]),
                                        &res[build_data[i].str2 + 1],
                                        numstrings);
              computed[build_data[i].str2]++;
            }
          //Debug code:
          //printf("%s\n",build_data[i].string2);
          if (temp)
            {
              cost++;
              if ((build_data[i].distance > 0 &&
                   !neg[build_data[i].pattern]) ||
                  (build_data[i].distance < 0 &&
                   neg[build_data[i].pattern]))
                {
                  if (neg[build_data[i].str2])
                    {
#ifdef GMP_CHECK
                      mpz_neg(res[build_data[i].str2 + 1],
                              res[build_data[i].str2 + 1]);
#endif
                      printf("u%d = -u%d + u%d << %d\n",
                             build_data[i].str2 + 1, build_data[i].str2 + 1,
                             build_data[i].pattern + 1, build_data[i].pos2);
                      neg[build_data[i].str2] = 0;
                    }
                  else
                    printf("u%d = u%d + u%d << %d\n",
                           build_data[i].str2 + 1, build_data[i].str2 + 1,
                           build_data[i].pattern + 1, build_data[i].pos2);
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                               build_data[i].pos2);
                  mpz_add(res[build_data[i].str2 + 1],
                          res[build_data[i].str2 + 1], tempmpz);
#endif
                }
              else
                {
                  if (neg[build_data[i].str2])
                    {
#ifdef GMP_CHECK
                      mpz_neg(res[build_data[i].str2 + 1],
                              res[build_data[i].str2 + 1]);
#endif
                      printf("u%d = -u%d - u%d << %d\n",
                             build_data[i].str2 + 1, build_data[i].str2 + 1,
                             build_data[i].pattern + 1, build_data[i].pos2);
                      neg[build_data[i].str2] = 0;
                    }
                  else
                    printf("u%d = u%d - u%d << %d\n",
                           build_data[i].str2 + 1, build_data[i].str2 + 1,
                           build_data[i].pattern + 1, build_data[i].pos2);
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                               build_data[i].pos2);
                  mpz_sub(res[build_data[i].str2 + 1],
                          res[build_data[i].str2 + 1], tempmpz);
#endif
                }
#ifdef GMP_CHECK
              mpz_out_str(stdout,2,res[build_data[i].str2 + 1]);
              printf("\n");
#endif
            }
          else
            {
              if ((build_data[i].distance > 0 &&
                   !neg[build_data[i].pattern]) ||
                  (build_data[i].distance < 0 &&
                   neg[build_data[i].pattern]))
                {
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                               build_data[i].pos2);
                  mpz_set(res[build_data[i].str2 + 1], tempmpz);
#endif
                  printf("u%d = + u%d << %d\n", build_data[i].str2 + 1,
                         build_data[i].pattern + 1, build_data[i].pos2);
                }
              else
                {
#ifdef GMP_CHECK
                  mpz_mul_2exp(tempmpz, res[build_data[i].pattern + 1],
                               build_data[i].pos2);
                  mpz_neg(tempmpz, tempmpz);
                  mpz_set(res[build_data[i].str2 + 1], tempmpz);
#endif
                  printf("u%d = - u%d << %d\n", build_data[i].str2 + 1,
                         build_data[i].pattern + 1, build_data[i].pos2);
                }
#ifdef GMP_CHECK
              mpz_out_str(stdout, 2, res[build_data[i].str2 + 1]);
              printf("\n");
#endif
            }
        }
    }
  printf("Cost = %d ?\n", cost);
#ifdef GMP_CHECK
  mpz_out_str(stdout, 2, res[1]);
  printf("\n");
  mpz_set_str(tempmpz, original, 2);
  if (mpz_cmp(res[1], tempmpz))
    printf("Erreur !\n");
#endif
  for (i = 0; i < numstrings; i++)
    mpz_clear(res[i]);
  mpz_clear(tempmpz);
  free(res);
  free(computed);
  free(neg);
  return cost;
}

int readint(char *str, char *var)
{
  long int i;
  char *end;

  i = strtol (str, &end, 10);

  if (*end != '\0')
    {
      fprintf (stderr, "error when reading %s\n", var);
      exit (EXIT_FAILURE);
    }

  if (i < INT_MIN || i > INT_MAX || errno == ERANGE)
    {
      fprintf (stderr, "%s is too small or large\n", var);
      exit (EXIT_FAILURE);
    }

  return i;
}

int main(int argc, char *argv[])
{
  mpz_t num;
  char *binary, *booth;
  int temp, length, weightn, i, j, numel, l1, l2, reversed, run, weightsmn;
  int cost;
  weight_r *weights, *weightsm;
  char **strings, *usable;
  time_t seed;
  char *original; //This string is used to keep the original number
  data_record *build_data;
  int numops;

  if (argc <= 2)
    {
    usage:
      fprintf(stderr, "Usage: %s verbose num [numbits] [seed]\n"
              "  verbose: 0,1,2\n"
              "  num: a decimal number\n"
              "  If num is 0, then a random number with 'numbits' bits\n"
              "  is generated, using 'seed' as seed if it is present.\n",
              argv[0]);
      return EXIT_FAILURE;
    }

  verbose = readint(argv[1], "verbose");
  cost = 0;

  //converts the number passed as an argument into a mpz_t
  mpz_init_set_str(num, argv[2], 10);

  if (!mpz_cmp_ui(num, 0))
    {
      gmp_randstate_t randstate;
      int numbits;

      if (argc <= 3)
        goto usage;

      gmp_randinit_default(randstate);
      if (argc >= 5)
        seed = readint(argv[4], "seed");
      else
        seed = time(NULL);
      gmp_randseed_ui(randstate, seed);

      numbits = readint(argv[3], "numbits");
      if (numbits <= 0)
        {
          fprintf(stderr, "numbits must be at least 1\n");
          return EXIT_FAILURE;
        }

      mpz_urandomb(num, randstate, numbits);
      mpz_setbit(num, 0);

      printf("Number of bits: %d\n", (int) mpz_sizeinbase(num, 2));

      gmp_randclear(randstate);
    }
  else
    seed = 0;

  //allocates a string that will contains the binary representation
  //of the number
  temp = (mpz_sizeinbase(num, 2) + 5) * sizeof(char);
  binary = my_calloc(sizeof(char), temp);
  //this one will old the booth encoding of the number
  booth = my_calloc(sizeof(char), temp);

  //converts the number in binary
  mpz_get_str(binary, 2, num);
  length = Booth_Encode(binary, booth);
  length++;

  printf("Number in binary: %s\n", binary);
  printf("Number booth enc: %s\n", booth);

  //We backup our number, so we can use it later
  original = my_calloc(sizeof(char), length + 1);
  strcpy(original, booth);

  //we allocate the pointers to store the elements
  strings = my_calloc(sizeof(char *), length);

  usable = my_calloc(sizeof(char), length);

  //Allocates the memory for the data used to build the mul
  build_data = my_calloc(sizeof(data_record), (length + 0) * 4);
  numops = 0;

  //The first step of the algorithm is to find a pattern in the number
  //we must allocate sufficient space to store the weights
  weights = my_calloc(sizeof(weight_r),(length + 0) * 4);
  weightn = Compute_Weights(booth, booth, weights,
                            strlen(booth), strlen(booth), &reversed);

  FindMaxWeight(weights, weightn);
  if (verbose == VERBOSE_ALL)
    for (i = 0; i < weightn; i++)
      printf("Distance: %04d\tWeight: %d\n",
             weights[i].distance, weights[i].weight);
  //We check if we found sth usable
  if (weights[0].weight <= 1)
    {
      printf("No distance with weight > 1 has been found :(\n");
      cost = DoSingleNumberSAS(booth, 1, &reversed, &temp, &num, 1);
      printf("Cost = %d\n", cost);
      return 0;
    }
  else if (verbose > 0)
    printf("Using distance: %d with associated weight of: %d\n",
           weights[0].distance, weights[0].weight);

  //we compute our first pattern
  strings[0] = booth;
  strings[1] = my_calloc(sizeof(char), length + 1);
  weights[0].pos = FindPattern(strings[0], weights[0], strings[1]);
  if (!strcmp(strings[0], "1"))
    strcpy(strings[0], "");
  //We check if the remainders are 0 or 1, if so, mark that they are not
  //usable for pattern search.
  if (strings[0][0] == 0 || !strcmp(strings[0], "P"))
    usable[0]++;
  if (strings[1][0] == 0 || !strcmp(strings[1], "1"))
    free(strings[1]);
  if (verbose > 0)
    {
      printf("Our pattern: %s\n", strings[1]);
      printf("The remainder: %s\n", strings[0]);
    }

  FillBuildData(&build_data[numops], 0, 0, 1, weights[0],
                strings[0], strings[0], strings[1]);
  numops++;

  cost += 2;

  numel = 2;
  weightsm = NULL;
  run = 1;

  while (run)
    {
      weightsm = my_realloc(weightsm, numel * numel * sizeof(weight_r));
      weightsmn = -1;
      for (i = 0; i < numel; i++)
        for (j = i; j < numel; j++)
          {
            if (!usable[i] && !usable[j])
              {
                if (verbose == VERBOSE_ALL)
                  printf("String %d: %s\nString %d: %s\n",
                         i, strings[i], j, strings[j]);
                else if (verbose > 0)
                  printf("String %d / String %d\n", i, j);

                l1 = strlen(strings[i]);
                l2 = strlen(strings[j]);
                weightn = Compute_Weights(strings[i], strings[j], weights,
                                          l1, l2, &reversed);

                if (weightn > 0)
                  {
                    FindMaxWeight(weights, weightn);
                    memcpy(&(weightsm[++weightsmn]), weights,
                           sizeof(weight_r));
                    if (reversed)
                      {
                        weightsm[weightsmn].i = j;
                        weightsm[weightsmn].j = i;
                      }
                    else
                      {
                        weightsm[weightsmn].i = i;
                        weightsm[weightsmn].j = j;
                      }
                  }
              }
          }

      FindMaxWeight(weightsm, weightsmn);

      if (weightsm[0].weight > 1 && weightsmn > 0)
        {
          strings[numel] = my_calloc(sizeof(char), length);
          i = weightsm[0].i;
          j = weightsm[0].j;
          if (i == j)
            weightsm[0].pos = FindPattern(strings[i], weightsm[0],
                                          strings[numel]);
          else
            BuildCommonPattern(strings[i], strings[j], weightsm[0],
                               strings[numel]);
          //We check if the remainders are 0 or 1, if so, mark that they
          //are not usable for pattern search.
          if (strings[i][0] == 0 || !strcmp(strings[i], "P"))
            usable[i]++;
          if (strings[j][0] == 0 || !strcmp(strings[j], "P"))
            usable[j]++;
          if (strings[numel][0] == 0 || !strcmp(strings[numel], "1"))
            free(strings[numel]);
          else
            {
              FillBuildData(&build_data[numops], i, j, numel, weightsm[0],
                            strings[i], strings[j], strings[numel]);
              numel++;
              numops++;
            }
        }
      else
        run = 0;
    }

  if (verbose > 0)
    {
      for (i = numops - 1; i >= 0; i--)
        {
          printf("Data %d:\n", i);
          printf("Str 1: %d, %s\n", build_data[i].str1,
                 build_data[i].string1);
          printf("Str 2: %d, %s\n", build_data[i].str2,
                 build_data[i].string2);
          printf("Pattr: %d, %s\n", build_data[i].pattern,
                 build_data[i].patternstr);
          printf("Distance: %d\n", build_data[i].distance);
          printf("Pos: %d\n", build_data[i].pos);
          printf("Weight: %d\n\n", build_data[i].weight);
        }
    }

  BuildSASString(build_data, numops, numel, binary);
  if (seed)
    printf("SEED: %d\n", (int) seed);

  //Cleaning
  free(weights);
  for (i = 0; i < length * 4; i++)
    {
      if (build_data[i].string1)
        {
          free(build_data[i].string1);
          free(build_data[i].string2);
          free(build_data[i].patternstr);
        }
    }
  free(build_data);
  for (i = 0; i < numel; i++)
    free(strings[i]);
  free(usable);
  free(strings);
  free(binary);
  free(original);
  free(weightsm);
  mpz_clear(num);
  return 0;
}
