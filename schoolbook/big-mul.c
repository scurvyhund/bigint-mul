/* big-mul_latest.c
 *
 * Last revised: 06.13.26
 *
 * Current exec @ ~/bin as 'bm'
 *
 * Program calcs product of 2 values given as arguments on the cmdln using the
 * simple school book algorithm.
 *
 * Currently gives correct results for arbitrarily large unsigned integers.
 *
 * What it does:
 *    1) takes two cmdln args, each representing an unsigned positive integer,
 *       and converts the args to integers
 *    2) calculates the product
 *    3) converts the product integer to printable string characters and prints
 *       to stdout
 *
 * Example:
 *    $> ./big-mul 56011910234567656654300034565 234877590876543211123455430567
 * result:
 *    13155942536288445545076126710938271624364921654225057548355
 *
 * compile:
 *    gcc -Wall -std=c99 -O1 -o big-mul big-mul.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

void array_multiply(const int8_t *, const int8_t *, int, int);

int main(int argc, char* argv[]){

   if(argc < 3) {
      puts("\nCOMMAND LINE ARGS ERROR\n");
      return 1;
   }

   // Validate: non-empty, digits only
   for(int a = 1; a <= 2; a++) {
      if(argv[a][0] == '\0') {
         fprintf(stderr, "Error: argument %d is empty\n", a);
         return 1;
      }
      for(int i = 0; argv[a][i]; i++) {
         if(!isdigit((unsigned char)argv[a][i])) {
            fprintf(stderr, "Error: '%c' is not a digit\n", argv[a][i]);
            return 1;
         }
      }
   }

   int length_1 = strlen(argv[1]);

   // array_1 will hold multiplier digits.
   int8_t array_1[length_1];

   // Convert digit chars to integers.
   for(int i = 0; i < length_1; i++)
      array_1[i] = argv[1][i] - '0';

   int length_2 = strlen(argv[2]);

   // array_2 will be multiplicand.
   int8_t array_2[length_2];

   for(int i = 0; i < length_2; i++)
      array_2[i] = argv[2][i] - '0';

   // Do the math.
   array_multiply(array_1, array_2, length_1, length_2);
   return 0;
}

void array_multiply(const int8_t* array_1, const int8_t* array_2,
                    int array_1_length, int array_2_length) {

   int product_length = array_1_length + array_2_length;
   int8_t product_array[product_length];

   for(int i = 0; i < product_length; i++)
      product_array[i] = 0;

   int8_t carry = 0; int8_t tmp;
   int shl_1 = 0;

   // i*j iterations. 'i' (multiplier index), 'j' (multiplicand index).
   for(int i = array_1_length - 1; i >= 0; i--) {

      // shl_1++ shifts 'k' to next higher base 10 column for each product
      // row as each digit of the multiplier traverses the multiplicand.
      int k = product_length - 1 - shl_1++;

      int j = array_2_length - 1;

      while(j >= 0 || carry > 0) {

         if(j >= 0)
            // Get prod. of 2 digits from factor 1 and factor 2.
            tmp = array_1[i] * array_2[j];
         else
            tmp = 0;

         // If carry from previous tmp add it now.
         tmp += carry;

         // 0 unless tmp is > than 9.
         carry = tmp / 10;

         // Add 1's digit to index k -- possible for product_array[k]
         // to be > 9 here...
         product_array[k] += (tmp % 10);

         carry += (product_array[k] / 10);
         // ...but not here.
         product_array[k] = product_array[k] % 10;

         j--; k--;
      }
   }

   // Find first non-zero digit (skip leading zeros).
   int i = 0;
   while(i < product_length - 1 && product_array[i] == 0)
      i++;

   puts("\n");
   for(; i < product_length; i++)
      putc(product_array[i] + '0', stdout);
   puts("\n");
}
